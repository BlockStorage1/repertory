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
#ifndef REPERTORY_INCLUDE_COMM_CURL_REQUESTS_HTTP_GET_HPP_
#define REPERTORY_INCLUDE_COMM_CURL_REQUESTS_HTTP_GET_HPP_

#include "comm/curl/requests/http_request_base.hpp"

namespace repertory::curl::requests {
struct http_get final : http_request_base {
  http_get() = default;
  http_get(const http_get &) = default;
  http_get(http_get &&) = default;
  auto operator=(const http_get &) -> http_get & = default;
  auto operator=(http_get &&) -> http_get & = default;
  ~http_get() override = default;

  [[nodiscard]] auto get_type() const -> std::string override { return "get"; }

  [[nodiscard]] auto set_method(CURL *curl,
                                stop_type & /*stop_requested*/) const
      -> bool override {
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    return true;
  }
};
} // namespace repertory::curl::requests

#endif // REPERTORY_INCLUDE_COMM_CURL_REQUESTS_HTTP_GET_HPP_
