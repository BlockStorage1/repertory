/*
  Copyright <2018-2025> <scott.e.graves@protonmail.com>

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
#ifndef REPERTORY_INCLUDE_COMM_CURL_CURL_COMM_HPP_
#define REPERTORY_INCLUDE_COMM_CURL_CURL_COMM_HPP_

#include "app_config.hpp"
#include "comm/curl/curl_shared.hpp"
#include "comm/curl/multi_request.hpp"
#include "comm/i_http_comm.hpp"
#include "events/event_system.hpp"
#include "events/types/curl_error.hpp"
#include "utils/encryption.hpp"

namespace repertory {
class curl_comm final : public i_http_comm {
public:
  curl_comm() = delete;

  explicit curl_comm(host_config cfg);

  explicit curl_comm(s3_config cfg);

private:
  using write_callback = size_t (*)(char *, size_t, size_t, void *);

  struct read_write_info final {
    data_buffer data{};
    stop_type_callback stop_requested_cb;
  };

  static const write_callback write_data;
  static const write_callback write_headers;
  static constexpr std::uint8_t retry_request_count{5U};

private:
  std::optional<host_config> host_config_;
  std::optional<s3_config> s3_config_;

public:
  [[nodiscard]] static auto create_curl() -> CURL *;

  [[nodiscard]] static auto reset_curl(CURL *curl_handle) -> CURL *;

public:
  [[nodiscard]] static auto construct_url(CURL *curl,
                                          const std::string &relative_path,
                                          const host_config &cfg)
      -> std::string;

  [[nodiscard]] static auto create_host_config(const s3_config &cfg)
      -> host_config;

  [[nodiscard]] static auto url_encode(CURL *curl, const std::string &data,
                                       bool allow_slash) -> std::string;

  template <typename request_type>
  [[nodiscard]] static auto
  make_encrypted_request(const host_config &cfg, const request_type &request,
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
        utils::encryption::generate_key<utils::encryption::hash_256_t>(
            request.decryption_token.value());
    if (not utils::encryption::read_encrypted_range(
            request.range.value(), key,
            [&](data_buffer &ct, std::uint64_t start_offset,
                std::uint64_t end_offset) -> bool {
              auto encrypted_request = request;
              encrypted_request.decryption_token = std::nullopt;
              encrypted_request.range = {{start_offset, end_offset}};
              encrypted_request.response_handler =
                  [&ct](const auto &encrypted_data, long /*response_code*/) {
                    ct = encrypted_data;
                  };
              encrypted_request.total_size = std::nullopt;

              if (not make_request(cfg, encrypted_request, response_code,
                                   stop_requested)) {
                return false;
              }

              if (response_code != http_error_codes::ok) {
                return false;
              }

              return true;
            },
            request.total_size.value(), data)) {
      return false;
    }

    if (request.response_handler.has_value()) {
      request.response_handler.value()(data, response_code);
    }

    return true;
  }

  template <typename request_type>
  [[nodiscard]] static auto
  make_request(const host_config &cfg, const request_type &request,
               long &response_code, stop_type &stop_requested) -> bool {
    REPERTORY_USES_FUNCTION_NAME();

    CURLcode curl_code{};
    const auto do_request = [&]() -> bool {
      if (request.decryption_token.has_value() &&
          not request.decryption_token.value().empty()) {
        return make_encrypted_request(cfg, request, response_code,
                                      stop_requested);
      }

      response_code = 0;

      auto *curl = create_curl();
      if (not request.set_method(curl, stop_requested)) {
        return false;
      }

      if (not cfg.agent_string.empty()) {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, cfg.agent_string.c_str());
      }

      if (request.allow_timeout && cfg.timeout_ms) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, cfg.timeout_ms);
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

      read_write_info write_info{
          {},
          [&stop_requested]() -> bool {
            return stop_requested || app_config::get_stop_requested();
          },
      };
      if (request.response_handler.has_value()) {
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_info);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
      }

      std::string parameters{};
      for (const auto &param : request.query) {
        parameters += (parameters.empty() ? '?' : '&') + param.first + '=' +
                      url_encode(curl, param.second, false);
      }

      if (not cfg.api_password.empty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, cfg.api_user.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, cfg.api_password.c_str());
      } else if (not cfg.api_user.empty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, cfg.api_user.c_str());
      }

      if (request.aws_service.has_value()) {
        curl_easy_setopt(curl, CURLOPT_AWS_SIGV4,
                         request.aws_service.value().c_str());
      }

      curl_slist *header_list{nullptr};
      if (not request.headers.empty()) {
        for (const auto &header : request.headers) {
          header_list = curl_slist_append(
              header_list,
              fmt::format("{}: {}", header.first, header.second).c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
      }

      curl_shared::set_share(curl);

      auto url = construct_url(curl, request.get_path(), cfg) + parameters;
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

      multi_request curl_request(curl, stop_requested);

      curl_code = CURLE_OK;
      curl_request.get_result(curl_code, response_code);

      if (header_list != nullptr) {
        curl_slist_free_all(header_list);
      }

      if (curl_code != CURLE_OK) {
        event_system::instance().raise<curl_error>(curl_code, function_name,
                                                   request.get_type(), url);
        return false;
      }

      if (request.response_handler.has_value()) {
        request.response_handler.value()(write_info.data, response_code);
      }

      return true;
    };

    bool ret{false};
    for (std::uint8_t retry = 0U; !ret && retry < retry_request_count;
         ++retry) {
      ret = do_request();
      if (ret) {
        break;
      }

      if (curl_code == CURLE_COULDNT_RESOLVE_HOST) {
        std::this_thread::sleep_for(1s);
        continue;
      }

      break;
    }

    return ret;
  }

public:
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

  [[nodiscard]] auto make_request(const curl::requests::http_post &post_file,
                                  long &response_code,
                                  stop_type &stop_requested) const
      -> bool override;

  [[nodiscard]] auto make_request(const curl::requests::http_put_file &put_file,
                                  long &response_code,
                                  stop_type &stop_requested) const
      -> bool override;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_COMM_CURL_CURL_COMM_HPP_
