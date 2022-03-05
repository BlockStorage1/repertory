/*
  Copyright <2018-2022> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute,
  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
  OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef INCLUDE_COMM_CURL_CURL_COMM_HPP_
#define INCLUDE_COMM_CURL_CURL_COMM_HPP_

#include "common.hpp"
#include "comm/curl/multi_request.hpp"
#include "comm/curl/session_manager.hpp"
#include "comm/i_comm.hpp"
#include "app_config.hpp"

namespace repertory {
class curl_resolver;
namespace utils::encryption {
class encrypting_reader;
}
struct curl_setup;
struct raw_write_data;

class curl_comm : public virtual i_comm {
public:
  typedef size_t (*curl_read_callback)(char *, size_t, size_t, void *);
  typedef size_t (*curl_write_callback)(char *, size_t, size_t, void *);

  static curl_read_callback read_data_callback_;
  static curl_write_callback write_data_callback_;
  static curl_write_callback write_header_callback_;
  static curl_write_callback write_null_callback_;
  static curl_write_callback write_string_callback_;

public:
  explicit curl_comm(const app_config &config) : config_(config) {}

  ~curl_comm() override = default;

private:
  const app_config &config_;
  session_manager session_manager_;

public:
  static std::string construct_url(CURL *curl_handle, const std::string &relative_path,
                                   const host_config &hc);

  static bool create_auth_session(CURL *&curl_handle, const app_config &config, host_config hc,
                                  std::string &session);

  static std::string http_range_to_string(const http_range &range);

  static void release_auth_session(const app_config &config, host_config hc,
                                   const std::string &session);

  static void update_auth_session(CURL *curl_handle, const app_config &config,
                                  const std::string &session);

private:
  CURL *common_curl_setup(const std::string &path, curl_setup &setup, std::string &url,
                          std::string &fields);

  CURL *common_curl_setup(CURL *curl_handle, const std::string &path, curl_setup &setup,
                          std::string &url, std::string &fields);

  template <typename begin, typename end>
  api_error execute_binary_operation(CURL *curl_handle, const std::string &url,
                                     std::vector<char> &data, json &error,
                                     const bool &stop_requested,
                                     const CURLcode &default_code = CURLE_OK) {
    auto curl_code = default_code;
    long http_code = 400;
    execute_operation<begin>(curl_handle, url, curl_code, http_code, stop_requested);

    const auto ret = process_binary_response(url, curl_code, http_code, data, error);
    if (config_.get_event_level() >= end::level) {
      event_system::instance().raise<end>(url, curl_code, http_code,
                                          ((ret == api_error::success) ? "" : error.dump(2)));
    }

    return ret;
  }

  template <typename begin, typename end>
  api_error execute_json_operation(CURL *curl_handle, const std::string &url,
                                   const std::string &result, json &data, json &error,
                                   const bool &stop_requested,
                                   const CURLcode &default_code = CURLE_OK) {
    auto curl_code = default_code;
    long http_code = 400;
    execute_operation<begin>(curl_handle, url, curl_code, http_code, stop_requested);

    const auto ret = process_json_response(url, curl_code, http_code, result, data, error);
    if (config_.get_event_level() >= end::level) {
      event_system::instance().raise<end>(url, curl_code, http_code,
                                          ((ret == api_error::success) ? "" : error.dump(2)));
    }

    return ret;
  }

  template <typename begin>
  void execute_operation(CURL *curl_handle, const std::string &url, CURLcode &curl_code,
                         long &http_code, const bool &stop_requested) {
    if (config_.get_event_level() >= begin::level) {
      event_system::instance().raise<begin>(url);
    }

    if (curl_code == CURLE_OK) {
      multi_request request(curl_handle, stop_requested);
      request.get_result(curl_code, http_code);
    }
  }

  api_error get_or_post(const host_config &hc, const bool &post, const std::string &path,
                        const http_parameters &parameters, json &data, json &error,
                        http_headers *headers = nullptr,
                        std::function<void(CURL *curl_handle)> cb = nullptr);

  api_error get_range(const host_config &hc, const std::string &path,
                      const std::uint64_t &data_size, const http_parameters &parameters,
                      const std::string &encryption_token, std::vector<char> &data,
                      const http_ranges &ranges, json &error, http_headers *headers,
                      const bool &stop_requested);

  api_error get_range_unencrypted(const host_config &hc, const std::string &path,
                                  const http_parameters &parameters, std::vector<char> &data,
                                  const http_ranges &ranges, json &error, http_headers *headers,
                                  const bool &stop_requested);

  api_error process_binary_response(const std::string &url, const CURLcode &res,
                                    const long &http_code, std::vector<char> data, json &error);

  api_error process_json_response(const std::string &url, const CURLcode &res,
                                  const long &http_code, const std::string &result, json &data,
                                  json &error);

  api_error process_response(const std::string &url, const CURLcode &res, const long &http_code,
                             const std::size_t &data_size,
                             const std::function<std::string()> &to_string_converter,
                             const std::function<void()> &success_handler, json &error) const;

  static std::string url_encode(CURL *curl_handle, const std::string &data,
                                const bool &allow_slash = false);

public:
  api_error get(const std::string &path, json &data, json &error) override {
    return get_or_post(config_.get_host_config(), false, path, {}, data, error);
  }

  api_error get(const host_config &hc, const std::string &path, json &data, json &error) override {
    return get_or_post(hc, false, path, {}, data, error);
  }

  api_error get(const std::string &path, const http_parameters &parameters, json &data,
                json &error) override {
    return get_or_post(config_.get_host_config(), false, path, parameters, data, error);
  }

  api_error get(const host_config &hc, const std::string &path, const http_parameters &parameters,
                json &data, json &error) override {
    return get_or_post(hc, false, path, parameters, data, error);
  }

  api_error get_range(const std::string &path, const std::uint64_t &data_size,
                      const http_parameters &parameters, const std::string &encryption_token,
                      std::vector<char> &data, const http_ranges &ranges, json &error,
                      const bool &stop_requested) override {
    return get_range(config_.get_host_config(), path, data_size, parameters, encryption_token, data,
                     ranges, error, nullptr, stop_requested);
  }

  api_error get_range(const host_config &hc, const std::string &path,
                      const std::uint64_t &data_size, const http_parameters &parameters,
                      const std::string &encryption_token, std::vector<char> &data,
                      const http_ranges &ranges, json &error, const bool &stop_requested) override {
    return get_range(hc, path, data_size, parameters, encryption_token, data, ranges, error,
                     nullptr, stop_requested);
  }

  api_error get_range_and_headers(const std::string &path, const std::uint64_t &dataSize,
                                  const http_parameters &parameters,
                                  const std::string &encryption_token, std::vector<char> &data,
                                  const http_ranges &ranges, json &error, http_headers &headers,
                                  const bool &stop_requested) override {
    return get_range(config_.get_host_config(), path, dataSize, parameters, encryption_token, data,
                     ranges, error, &headers, stop_requested);
  }

  api_error get_range_and_headers(const host_config &hc, const std::string &path,
                                  const std::uint64_t &dataSize, const http_parameters &parameters,
                                  const std::string &encryption_token, std::vector<char> &data,
                                  const http_ranges &ranges, json &error, http_headers &headers,
                                  const bool &stop_requested) override {
    return get_range(hc, path, dataSize, parameters, encryption_token, data, ranges, error,
                     &headers, stop_requested);
  }

  api_error get_raw(const std::string &path, const http_parameters &parameters,
                    std::vector<char> &data, json &error, const bool &stop_requested) override {
    return get_raw(config_.get_host_config(), path, parameters, data, error, stop_requested);
  }

  api_error get_raw(const host_config &hc, const std::string &path,
                    const http_parameters &parameters, std::vector<char> &data, json &error,
                    const bool &stop_requested) override;

  api_error post(const std::string &path, json &data, json &error) override {
    return get_or_post(config_.get_host_config(), true, path, {}, data, error);
  }

  api_error post(const host_config &hc, const std::string &path, json &data, json &error) override {
    return get_or_post(hc, true, path, {}, data, error);
  }

  api_error post(const std::string &path, const http_parameters &parameters, json &data,
                 json &error) override {
    return get_or_post(config_.get_host_config(), true, path, parameters, data, error);
  }

  api_error post(const host_config &hc, const std::string &path, const http_parameters &parameters,
                 json &data, json &error) override {
    return get_or_post(hc, true, path, parameters, data, error);
  }

  api_error post_file(const std::string &path, const std::string &source_path,
                      const http_parameters &parameters, json &data, json &error,
                      const bool &stop_requested) override {
    return post_file(config_.get_host_config(), path, source_path, parameters, data, error,
                     stop_requested);
  }

  api_error post_file(const host_config &hc, const std::string &path,
                      const std::string &source_path, const http_parameters &parameters, json &data,
                      json &error, const bool &stop_requested) override;

  api_error post_multipart_file(const std::string &path, const std::string &file_name,
                                const std::string &source_path, const std::string &encryption_token,
                                json &data, json &error, const bool &stop_requested) override {
    return post_multipart_file(config_.get_host_config(), path, file_name, source_path,
                               encryption_token, data, error, stop_requested);
  }

  api_error post_multipart_file(const host_config &hc, const std::string &path,
                                const std::string &file_name, const std::string &source_path,
                                const std::string &encryption_token, json &data, json &error,
                                const bool &stop_requested) override;

  bool tus_upload(host_config hc, const std::string &source_path, const std::string &file_name,
                  std::uint64_t file_size, const std::string &location, std::string &skylink,
                  const bool &stop_requested, utils::encryption::encrypting_reader *reader);

  bool tus_upload_create(host_config hc, const std::string &file_name,
                         const std::uint64_t &file_size, std::string &location);
};
} // namespace repertory

#endif // INCLUDE_COMM_CURL_CURL_COMM_HPP_
