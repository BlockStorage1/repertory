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
#ifndef INCLUDE_UTILS_STRING_UTILS_HPP_
#define INCLUDE_UTILS_STRING_UTILS_HPP_

#include <boost/dynamic_bitset.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace repertory::utils::string {
// Prototypes
constexpr auto begins_with(std::string_view str, std::string_view val) -> bool {
  return (str.find(val) == 0u);
}

constexpr auto contains(std::string_view str, std::string_view search) -> bool {
  return (str.find(search) != std::string_view::npos);
}

[[nodiscard]] /* constexpr c++20 */ auto ends_with(std::string_view str,
                                                   std::string_view val)
    -> bool;

[[nodiscard]] auto from_bool(bool val) -> std::string;

[[nodiscard]] auto from_dynamic_bitset(const boost::dynamic_bitset<> &bitset)
    -> std::string;

[[nodiscard]] auto from_utf8(const std::string &str) -> std::wstring;

[[nodiscard]] /* constexpr c++20 */ auto is_numeric(std::string_view s) -> bool;

[[nodiscard]] auto join(const std::vector<std::string> &arr, const char &delim)
    -> std::string;

auto left_trim(std::string &s) -> std::string &;

auto left_trim(std::string &s, const char &c) -> std::string &;

auto replace(std::string &src, const char &character, const char &with)
    -> std::string &;

auto replace(std::string &src, const std::string &find, const std::string &with,
             size_t start_pos = 0) -> std::string &;

[[nodiscard]] auto replace_copy(std::string src, const char &character,
                                const char &with) -> std::string;

[[nodiscard]] auto replace_copy(std::string src, const std::string &find,
                                const std::string &with, size_t start_pos = 0)
    -> std::string;

auto right_trim(std::string &s) -> std::string &;

auto right_trim(std::string &s, const char &c) -> std::string &;

[[nodiscard]] auto split(const std::string &str, const char &delim,
                         bool should_trim = true) -> std::vector<std::string>;

[[nodiscard]] auto to_bool(std::string val) -> bool;

[[nodiscard]] auto to_double(const std::string &str) -> double;

[[nodiscard]] auto to_dynamic_bitset(const std::string &val)
    -> boost::dynamic_bitset<>;

[[nodiscard]] auto to_lower(std::string str) -> std::string;

[[nodiscard]] auto to_int32(const std::string &val) -> std::int32_t;

[[nodiscard]] auto to_int64(const std::string &val) -> std::int64_t;

[[nodiscard]] auto to_size_t(const std::string &val) -> std::size_t;

[[nodiscard]] auto to_uint8(const std::string &val) -> std::uint8_t;

[[nodiscard]] auto to_uint16(const std::string &val) -> std::uint16_t;

[[nodiscard]] auto to_uint32(const std::string &val) -> std::uint32_t;

[[nodiscard]] auto to_uint64(const std::string &val) -> std::uint64_t;

[[nodiscard]] auto to_upper(std::string str) -> std::string;

[[nodiscard]] auto to_utf8(std::string str) -> std::string;

[[nodiscard]] auto to_utf8(const std::wstring &str) -> std::string;

auto trim(std::string &str) -> std::string &;

auto trim(std::string &str, const char &c) -> std::string &;

[[nodiscard]] auto trim_copy(std::string str) -> std::string;

[[nodiscard]] auto trim_copy(std::string str, const char &c) -> std::string;
} // namespace repertory::utils::string

#endif // INCLUDE_UTILS_STRING_UTILS_HPP_
