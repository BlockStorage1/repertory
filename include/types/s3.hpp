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
#ifndef INCLUDE_TYPES_S3_HPP_
#define INCLUDE_TYPES_S3_HPP_
#if defined(REPERTORY_ENABLE_S3)

#include "types/repertory.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
namespace utils::aws {
#if _WIN32
[[nodiscard]] inline auto format_time(std::uint64_t t) -> std::uint64_t {
  FILETIME ft{};
  utils::unix_time_to_filetime(t, ft);
  return static_cast<std::uint64_t>(ft.dwHighDateTime) << 32u |
         ft.dwLowDateTime;
}
#else  // _WIN32
[[nodiscard]] inline auto format_time(std::uint64_t t) -> std::uint64_t {
  return t;
}
#endif // _WIN32
} // namespace utils::aws

using get_key_callback = std::function<std::string()>;

using get_api_file_token_callback =
    std::function<std::string(const std::string &api_path)>;

using get_name_callback = std::function<std::string(
    const std::string &key, const std::string &object_name)>;

using get_size_callback = std::function<std::uint64_t()>;

using get_token_callback = std::function<std::string()>;

using set_key_callback = std::function<api_error(const std::string &key)>;

using list_directories_result = api_file_list;

using list_files_result = api_file_list;

using list_objects_result = std::vector<directory_item>;

struct head_object_result {
  std::uint64_t content_length{};
  std::string content_type{};
  std::uint64_t last_modified{};

  inline auto from_headers(http_headers headers) -> head_object_result & {
    content_length = utils::string::to_uint64(headers["content-length"]);
    content_type = headers["content-type"];
    auto date = headers["last-modified"];
    if (not date.empty()) {
      struct tm tm1 {};
      // Mon, 17 Dec 2012 02:14:10 GMT
#ifdef _WIN32
      utils::strptime(date.c_str(), "%a, %d %b %Y %H:%M:%S %Z", &tm1);
#else
      strptime(date.c_str(), "%a, %d %b %Y %H:%M:%S %Z", &tm1);
#endif
      last_modified =
          static_cast<std::uint64_t>(mktime(&tm1)) * NANOS_PER_SECOND;
    }
    return *this;
  }
};

} // namespace repertory

#endif
#endif // INCLUDE_TYPES_S3_HPP_
