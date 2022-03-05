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
#ifndef INCLUDE_UTILS_STRING_UTILS_HPP_
#define INCLUDE_UTILS_STRING_UTILS_HPP_

#include <string>
#include <vector>
#include <boost/dynamic_bitset.hpp>

namespace repertory::utils::string {
// Prototypes
bool begins_with(const std::string &str, const std::string &val);

bool contains(const std::string &str, const std::string &search);

bool ends_with(const std::string &str, const std::string &val);

std::string from_bool(const bool &val);

std::string from_double(const double &value);

std::string from_dynamic_bitset(const boost::dynamic_bitset<> &bitset);

std::string from_int32(const std::int32_t &val);

std::string from_int64(const std::int64_t &val);

std::string from_uint8(const std::uint8_t &val);

std::string from_uint16(const std::uint16_t &val);

std::string from_uint32(const std::uint32_t &val);

std::string from_uint64(const std::uint64_t &val);

std::wstring from_utf8(const std::string &str);

bool is_numeric(const std::string &s);

std::string join(const std::vector<std::string> &arr, const char &delim);

std::string &left_trim(std::string &s);

std::string &left_trim(std::string &s, const char &c);

std::string &replace(std::string &src, const char &character, const char &with);

std::string &replace(std::string &src, const std::string &find, const std::string &with,
                     size_t startPos = 0);

std::string replace_copy(std::string src, const char &character, const char &with);

std::string replace_copy(std::string src, const std::string &find, const std::string &with,
                         size_t startPos = 0);

std::string &right_trim(std::string &s);

std::string &right_trim(std::string &s, const char &c);

std::vector<std::string> split(const std::string &str, const char &delim,
                               const bool &should_trim = true);

bool to_bool(std::string val);

double to_double(const std::string &str);

boost::dynamic_bitset<> to_dynamic_bitset(const std::string &val);

std::string to_lower(std::string str);

std::int32_t to_int32(const std::string &val);

std::int64_t to_int64(const std::string &val);

std::uint8_t to_uint8(const std::string &val);

std::uint16_t to_uint16(const std::string &val);

std::uint32_t to_uint32(const std::string &val);

std::uint64_t to_uint64(const std::string &val);

std::string to_upper(std::string str);

const std::string &to_utf8(const std::string &str);

std::string to_utf8(const std::wstring &str);

std::string &trim(std::string &str);

std::string &trim(std::string &str, const char &c);

std::string trim_copy(std::string str);

std::string trim_copy(std::string str, const char &c);
} // namespace repertory::utils::string

#endif // INCLUDE_UTILS_STRING_UTILS_HPP_
