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
#ifndef INCLUDE_COMM_CURL_CURL_REQUESTS_HTTP_PUT_FILE_HPP_
#define INCLUDE_COMM_CURL_CURL_REQUESTS_HTTP_PUT_FILE_HPP_

#include "comm/curl/requests/http_request_base.hpp"
#include "utils/encrypting_reader.hpp"

namespace repertory::curl::requests {
struct http_put_file final : http_request_base {
  ~http_put_file() override = default;

  std::optional<std::string> encryption_token{};
  std::string file_name{};
  mutable std::shared_ptr<utils::encryption::encrypting_reader> reader{};
  std::string source_path{};

  [[nodiscard]] auto get_path() const -> std::string override;

  [[nodiscard]] auto set_method(CURL *curl, stop_type &stop_requested) const
      -> bool override;

private:
  mutable std::shared_ptr<read_file_info> read_info{};
};
} // namespace repertory::curl::requests

#endif // INCLUDE_COMM_CURL_CURL_REQUESTS_HTTP_PUT_FILE_HPP_
