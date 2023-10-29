/*
  Copyright <2018-2023> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#ifndef INCLUDE_COMM_CURL_CURL_COMM_HPP_
#define INCLUDE_COMM_CURL_CURL_COMM_HPP_

#include "comm/curl/multi_request.hpp"
#include "comm/i_http_comm.hpp"
#include "utils/encryption.hpp"
#include "utils/utils.hpp"

namespace repertory {
class curl_comm final : public i_http_comm {
public:
  curl_comm() = delete;

  explicit curl_comm(host_config hc);

  explicit curl_comm(s3_config s3);

private:
  using write_callback = size_t (*)(char *, size_t, size_t, void *);

  struct read_write_info final {
    repertory::data_buffer data{};
    repertory::stop_type &stop_requested;
  };

  static const write_callback write_data;
  static const write_callback write_headers;

private:
  std::optional<host_config> host_config_;
  std::optional<s3_config> s3_config_;

private:
  bool use_s3_path_style_{false};

public:
  [[nodiscard]] static auto construct_url(CURL *curl,
                                          const std::string &relative_path,
                                          const host_config &hc) -> std::string;

  [[nodiscard]] static auto create_host_config(const s3_config &config,
                                               bool use_s3_path_style)
      -> host_config;

  [[nodiscard]] static auto url_encode(CURL *curl, const std::string &data,
                                       bool allow_slash) -> std::string;

  template <typename request_type>
  [[nodiscard]] static auto
  make_encrypted_request(const host_config &hc, const request_type &request,
                         long &response_code, stop_type &stop_requested)
      -> bool {
    response_code = 0;

    if (not request.decryption_token.has_value() ||
        request.decryption_token.value().empty()) {
      return false;
    }

    if (not request.range.has_value()) {
      return false;
    }

    if (not request.total_size.has_value()) {
      return false;
    }

    data_buffer data{};
    const auto key =
        utils::encryption::generate_key(request.decryption_token.value());
    const auto result = utils::encryption::read_encrypted_range(
        request.range.value(), key,
        [&](std::vector<char> &ct, std::uint64_t start_offset,
            std::uint64_t end_offset) -> api_error {
          auto encrypted_request = request;
          encrypted_request.decryption_token = std::nullopt;
          encrypted_request.range = {{start_offset, end_offset}};
          encrypted_request.response_handler = [&ct](const auto &encrypted_data,
                                                     long /*response_code*/) {
            ct = encrypted_data;
          };
          encrypted_request.total_size = std::nullopt;

          if (not make_request(hc, encrypted_request, response_code,
                               stop_requested)) {
            return api_error::comm_error;
          }

          if (response_code != 200) {
            return api_error::comm_error;
          }

          return api_error::success;
        },
        request.total_size.value(), data);
    if (result != api_error::success) {
      return false;
    }

    if (request.response_handler.has_value()) {
      request.response_handler.value()(data, response_code);
    }

    return true;
  }

  template <typename request_type>
  [[nodiscard]] static auto
  make_request(const host_config &hc, const request_type &request,
               long &response_code, stop_type &stop_requested) -> bool {
    if (request.decryption_token.has_value() &&
        not request.decryption_token.value().empty()) {
      return make_encrypted_request(hc, request, response_code, stop_requested);
    }

    response_code = 0;

    auto *curl = utils::create_curl();
    if (not request.set_method(curl, stop_requested)) {
      return false;
    }

    if (not hc.agent_string.empty()) {
      curl_easy_setopt(curl, CURLOPT_USERAGENT, hc.agent_string.c_str());
    }

    if (request.allow_timeout && hc.timeout_ms) {
      curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, hc.timeout_ms);
    }

    std::string range_list{};
    if (request.range.has_value()) {
      range_list = std::to_string(request.range.value().begin) + '-' +
                   std::to_string(request.range.value().end);
      curl_easy_setopt(curl, CURLOPT_RANGE, range_list.c_str());
    }

    if (request.response_headers.has_value()) {
      curl_easy_setopt(curl, CURLOPT_HEADERDATA,
                       &request.response_headers.value());
      curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_headers);
    }

    read_write_info write_info{{}, stop_requested};
    if (request.response_handler.has_value()) {
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_info);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    }

    std::string parameters{};
    for (const auto &kv : request.query) {
      parameters += (parameters.empty() ? '?' : '&') + kv.first + '=' +
                    url_encode(curl, kv.second, false);
    }

    if (not hc.api_password.empty()) {
      curl_easy_setopt(curl, CURLOPT_USERNAME, hc.api_user.c_str());
      curl_easy_setopt(curl, CURLOPT_PASSWORD, hc.api_password.c_str());
    } else if (not hc.api_user.empty()) {
      curl_easy_setopt(curl, CURLOPT_USERNAME, hc.api_user.c_str());
    }

    if (request.aws_service.has_value()) {
      curl_easy_setopt(curl, CURLOPT_AWS_SIGV4,
                       request.aws_service.value().c_str());
    }

    auto url = construct_url(curl, request.get_path(), hc) + parameters;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    multi_request curl_request(curl, stop_requested);

    CURLcode curl_code{};
    curl_request.get_result(curl_code, response_code);
    if (curl_code != CURLE_OK) {
      return false;
    }

    if (request.response_handler.has_value()) {
      request.response_handler.value()(write_info.data, response_code);
    }

    return true;
  }

public:
  void enable_s3_path_style(bool enable) override;

  [[nodiscard]] auto make_request(const curl::requests::http_delete &del,
                                  long &response_code,
                                  stop_type &stop_requested) const
      -> bool override;

  [[nodiscard]] auto make_request(const curl::requests::http_get &get,
                                  long &response_code,
                                  stop_type &stop_requested) const
      -> bool override;

  [[nodiscard]] auto make_request(const curl::requests::http_head &head,
                                  long &response_code,
                                  stop_type &stop_requested) const
      -> bool override;

  [[nodiscard]] auto make_request(const curl::requests::http_put_file &put_file,
                                  long &response_code,
                                  stop_type &stop_requested) const
      -> bool override;
};
} // namespace repertory

#endif // INCLUDE_COMM_CURL_CURL_COMM_HPP_
