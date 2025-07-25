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
#ifndef REPERTORY_INCLUDE_COMM_CURL_REQUESTS_HTTP_REQUEST_BASE_HPP_
#define REPERTORY_INCLUDE_COMM_CURL_REQUESTS_HTTP_REQUEST_BASE_HPP_

#include "types/repertory.hpp"
#include "utils/file.hpp"

namespace repertory::curl::requests {
using curl_response_callback =
    std::function<void(const data_buffer &data, long response_code)>;

struct read_file_info final {
  stop_type &stop_requested;
  std::unique_ptr<utils::file::i_file> file{};
  std::uint64_t offset{};
};

[[nodiscard]] auto curl_file_reader(char *buffer, size_t size, size_t nitems,
                                    void *instream) -> size_t;

struct http_request_base {
  http_request_base() = default;
  http_request_base(const http_request_base &) = default;
  http_request_base(http_request_base &&) = default;

  auto operator=(const http_request_base &) -> http_request_base & = default;
  auto operator=(http_request_base &&) -> http_request_base & = default;

  virtual ~http_request_base() = default;

  bool allow_timeout{};
  std::optional<std::string> aws_service;
  std::optional<std::string> decryption_token{};
  mutable http_headers headers{};
  std::string path{};
  http_query_parameters query{};
  std::optional<http_range> range{};
  std::optional<curl_response_callback> response_handler;
  std::optional<http_headers> response_headers;
  std::optional<std::uint64_t> total_size{};

  [[nodiscard]] virtual auto get_path() const -> std::string { return path; }

  [[nodiscard]] virtual auto get_type() const -> std::string = 0;

  [[nodiscard]] virtual auto set_method(CURL *curl,
                                        stop_type &stop_requested) const
      -> bool = 0;
};
} // namespace repertory::curl::requests

#endif // REPERTORY_INCLUDE_COMM_CURL_REQUESTS_HTTP_REQUEST_BASE_HPP_
