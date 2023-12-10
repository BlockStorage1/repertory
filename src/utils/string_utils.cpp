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
#include "utils/string_utils.hpp"

namespace repertory::utils::string {
/* constexpr c++20 */ auto ends_with(std::string_view str, std::string_view val)
    -> bool {
  if (val.size() > str.size()) {
    return false;
  }
  return std::equal(val.rbegin(), val.rend(), str.rbegin());
}

auto from_bool(bool val) -> std::string { return std::to_string(val); }

auto from_dynamic_bitset(const boost::dynamic_bitset<> &bitset) -> std::string {
  std::stringstream ss;
  boost::archive::text_oarchive archive(ss);
  archive << bitset;
  return ss.str();
}

auto from_utf8(const std::string &str) -> std::wstring {
  return str.empty()
             ? L""
             : std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>()
                   .from_bytes(str);
}

/* constexpr c++20 */ auto is_numeric(std::string_view s) -> bool {
  if ((s.length() > 1u) && (s[0u] == '+' || s[0u] == '-')) {
    s = s.substr(1u);
  }

  if (s.empty()) {
    return false;
  }

  auto has_decimal = false;
  return std::find_if(
             s.begin(), s.end(),
             [&has_decimal](const std::string_view::value_type &c) -> bool {
               if (has_decimal && c == '.') {
                 return true;
               }
               if ((has_decimal = has_decimal || c == '.')) {
                 return false;
               }
               return not std::isdigit(c);
             }) == s.end();
}

auto join(const std::vector<std::string> &arr, const char &delim)
    -> std::string {
  if (arr.empty()) {
    return "";
  }

  return std::accumulate(
      std::next(arr.begin()), arr.end(), arr[0u],
      [&delim](auto s, const auto &v) { return s + delim + v; });
}

auto left_trim(std::string &s) -> std::string & { return left_trim(s, ' '); }

auto left_trim(std::string &s, const char &c) -> std::string & {
  s.erase(0, s.find_first_not_of(c));
  return s;
}

auto replace(std::string &src, const char &character, const char &with)
    -> std::string & {
  std::replace(src.begin(), src.end(), character, with);
  return src;
}

auto replace(std::string &src, const std::string &find, const std::string &with,
             size_t start_pos) -> std::string & {
  if (!src.empty() && (start_pos < src.size())) {
    while ((start_pos = src.find(find, start_pos)) != std::string::npos) {
      src.replace(start_pos, find.size(), with);
      start_pos += with.size();
    }
  }
  return src;
}

auto replace_copy(std::string src, const char &character, const char &with)
    -> std::string {
  std::replace(src.begin(), src.end(), character, with);
  return src;
}

auto replace_copy(std::string src, const std::string &find,
                  const std::string &with, size_t start_pos) -> std::string {
  return replace(src, find, with, start_pos);
}

auto right_trim(std::string &s) -> std::string & { return right_trim(s, ' '); }

auto right_trim(std::string &s, const char &c) -> std::string & {
  s.erase(s.find_last_not_of(c) + 1);
  return s;
}

auto split(const std::string &str, const char &delim, bool should_trim)
    -> std::vector<std::string> {
  std::vector<std::string> ret;
  std::stringstream ss(str);
  std::string item;
  while (std::getline(ss, item, delim)) {
    ret.push_back(should_trim ? trim(item) : item);
  }
  return ret;
}

auto to_bool(std::string val) -> bool {
  auto b = false;

  trim(val);
  if (is_numeric(val)) {
    if (contains(val, ".")) {
      b = (to_double(val) != 0.0);
    } else {
      std::istringstream(val) >> b;
    }
  } else {
    std::istringstream(to_lower(val)) >> std::boolalpha >> b;
  }

  return b;
}

auto to_double(const std::string &str) -> double { return std::stod(str); }

auto to_dynamic_bitset(const std::string &val) -> boost::dynamic_bitset<> {
  std::stringstream ss(val);
  boost::dynamic_bitset<> bitset;
  boost::archive::text_iarchive archive(ss);
  archive >> bitset;
  return bitset;
}

auto to_int32(const std::string &val) -> std::int32_t { return std::stoi(val); }

auto to_int64(const std::string &val) -> std::int64_t {
  return std::stoll(val);
}

auto to_lower(std::string str) -> std::string {
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  return str;
}

auto to_size_t(const std::string &val) -> std::size_t {
  return static_cast<std::size_t>(std::stoull(val));
}

auto to_uint8(const std::string &val) -> std::uint8_t {
  return static_cast<std::uint8_t>(std::stoul(val));
}

auto to_uint16(const std::string &val) -> std::uint16_t {
  return static_cast<std::uint16_t>(std::stoul(val));
}

auto to_uint32(const std::string &val) -> std::uint32_t {
  return static_cast<std::uint32_t>(std::stoul(val));
}

auto to_uint64(const std::string &val) -> std::uint64_t {
  return std::stoull(val);
}

auto to_upper(std::string str) -> std::string {
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  return str;
}

auto to_utf8(std::string str) -> std::string { return str; }

auto to_utf8(const std::wstring &str) -> std::string {
  return str.empty()
             ? ""
             : std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>()
                   .to_bytes(str);
}

auto trim(std::string &str) -> std::string & {
  return right_trim(left_trim(str));
}

auto trim(std::string &str, const char &c) -> std::string & {
  return right_trim(left_trim(str, c), c);
}

auto trim_copy(std::string str) -> std::string {
  return right_trim(left_trim(str));
}

auto trim_copy(std::string str, const char &c) -> std::string {
  return right_trim(left_trim(str, c), c);
}
} // namespace repertory::utils::string
