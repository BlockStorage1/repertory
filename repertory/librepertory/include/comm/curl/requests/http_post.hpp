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
#ifndef REPERTORY_INCLUDE_COMM_CURL_REQUESTS_HTTP_POST_HPP_
#define REPERTORY_INCLUDE_COMM_CURL_REQUESTS_HTTP_POST_HPP_

#include "comm/curl/requests/http_request_base.hpp"

namespace repertory::curl::requests {
struct http_post final : http_request_base {
  http_post() = default;
  http_post(const http_post &) = default;
  http_post(http_post &&) = default;
  auto operator=(const http_post &) -> http_post & = default;
  auto operator=(http_post &&) -> http_post & = default;

  ~http_post() override = default;

  std::optional<nlohmann::json> json;

  [[nodiscard]] auto set_method(CURL *curl,
                                stop_type & /*stop_requested*/) const
      -> bool override;

private:
  mutable std::optional<std::string> json_str;
};
} // namespace repertory::curl::requests

#endif // REPERTORY_INCLUDE_COMM_CURL_REQUESTS_HTTP_POST_HPP_
