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
#ifndef REPERTORY_INCLUDE_UTILS_ERROR_UTILS_HPP_
#define REPERTORY_INCLUDE_UTILS_ERROR_UTILS_HPP_

#include "types/repertory.hpp"

namespace repertory::utils::error {
void raise_error(std::string_view function, std::string_view msg);

void raise_error(std::string_view function, api_error err,
                 std::string_view msg);

void raise_error(std::string_view function, const std::exception &exception);

void raise_error(std::string_view function, const std::exception &exception,
                 std::string_view msg);

void raise_error(std::string_view function, std::int64_t err,
                 std::string_view msg);

void raise_error(std::string_view function, const json &err,
                 std::string_view msg);

void raise_error(std::string_view function, api_error err,
                 std::string_view file_path, std::string_view msg);

void raise_error(std::string_view function, std::int64_t err,
                 std::string_view file_path, std::string_view msg);

void raise_error(std::string_view function, const std::exception &exception,
                 std::string_view file_path, std::string_view msg);

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          std::string_view msg);

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          api_error err, std::string_view msg);

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          const std::exception &exception);

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          const std::exception &exception,
                          std::string_view msg);

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          std::int64_t err, std::string_view msg);

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          const json &err, std::string_view msg);

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          std::string_view source_path, api_error err,
                          std::string_view msg);

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          std::string_view source_path, std::int64_t err,
                          std::string_view msg);

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          std::string_view source_path,
                          const std::exception &exception,
                          std::string_view msg);

void raise_url_error(std::string_view function, std::string_view url,
                     CURLcode err, std::string_view msg);

void raise_url_error(std::string_view function, std::string_view url,
                     std::string_view source_path,
                     const std::exception &exception);

void raise_url_error(std::string_view function, std::string_view url,
                     std::string_view source_path,
                     const std::exception &exception, std::string_view msg);

} // namespace repertory::utils::error

#endif // REPERTORY_INCLUDE_UTILS_ERROR_UTILS_HPP_
