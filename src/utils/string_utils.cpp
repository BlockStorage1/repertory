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
#include "utils/string_utils.hpp"
#include "common.hpp"

namespace repertory::utils::string {
bool begins_with(const std::string &str, const std::string &val) { return (str.find(val) == 0); }

bool contains(const std::string &str, const std::string &search) {
  return (str.find(search) != std::string::npos);
}

bool ends_with(const std::string &str, const std::string &val) {
  if (val.size() > str.size()) {
    return false;
  }
  return std::equal(val.rbegin(), val.rend(), str.rbegin());
}

std::string from_bool(const bool &val) { return std::to_string(val); }

std::string from_double(const double &value) { return std::to_string(value); }

std::string from_dynamic_bitset(const boost::dynamic_bitset<> &bitset) {
  std::stringstream ss;
  boost::archive::text_oarchive archive(ss);
  archive << bitset;
  return ss.str();
}

std::string from_int32(const std::int32_t &val) { return std::to_string(val); }

std::string from_int64(const std::int64_t &val) { return std::to_string(val); }

std::string from_uint8(const std::uint8_t &val) { return std::to_string(val); }

std::string from_uint16(const std::uint16_t &val) { return std::to_string(val); }

std::string from_uint32(const std::uint32_t &val) { return std::to_string(val); }

std::string from_uint64(const std::uint64_t &val) { return std::to_string(val); }

std::wstring from_utf8(const std::string &str) {
  return str.empty() ? L""
                     : std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(str);
}

bool is_numeric(const std::string &s) {
  static const auto r = std::regex(R"(^(\+|\-)?(([0-9]*)|(([0-9]*)\.([0-9]*)))$)");
  return std::regex_match(s, r);
}

std::string join(const std::vector<std::string> &arr, const char &delim) {
  if (arr.empty()) {
    return "";
  }

  return std::accumulate(std::next(arr.begin()), arr.end(), arr[0u],
                         [&delim](auto s, const auto &v) { return s + delim + v; });
}

std::string &left_trim(std::string &s) { return left_trim(s, ' '); }

std::string &left_trim(std::string &s, const char &c) {
  s.erase(0, s.find_first_not_of(c));
  return s;
}

std::string &replace(std::string &src, const char &character, const char &with) {
  std::replace(src.begin(), src.end(), character, with);
  return src;
}

std::string &replace(std::string &src, const std::string &find, const std::string &with,
                     size_t startPos) {
  if (!src.empty() && (startPos < src.size())) {
    while ((startPos = src.find(find, startPos)) != std::string::npos) {
      src.replace(startPos, find.size(), with);
      startPos += with.size();
    }
  }
  return src;
}

std::string replace_copy(std::string src, const char &character, const char &with) {
  std::replace(src.begin(), src.end(), character, with);
  return src;
}

std::string replace_copy(std::string src, const std::string &find, const std::string &with,
                         size_t startPos) {
  return replace(src, find, with, startPos);
}

std::string &right_trim(std::string &s) { return right_trim(s, ' '); }

std::string &right_trim(std::string &s, const char &c) {
  s.erase(s.find_last_not_of(c) + 1);
  return s;
}

std::vector<std::string> split(const std::string &str, const char &delim, const bool &should_trim) {
  std::vector<std::string> ret;
  std::stringstream ss(str);
  std::string item;
  while (std::getline(ss, item, delim)) {
    ret.push_back(should_trim ? trim(item) : item);
  }
  return ret;
}

bool to_bool(std::string val) {
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

double to_double(const std::string &str) { return std::stod(str); }

boost::dynamic_bitset<> to_dynamic_bitset(const std::string &val) {
  std::stringstream ss(val);
  boost::dynamic_bitset<> bitset;
  boost::archive::text_iarchive archive(ss);
  archive >> bitset;
  return bitset;
}

std::int32_t to_int32(const std::string &val) { return std::stoi(val); }

std::int64_t to_int64(const std::string &val) { return std::stoll(val); }

std::string to_lower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  return str;
}

std::uint8_t to_uint8(const std::string &val) { return static_cast<std::uint8_t>(std::stoul(val)); }

std::uint16_t to_uint16(const std::string &val) {
  return static_cast<std::uint16_t>(std::stoul(val));
}

std::uint32_t to_uint32(const std::string &val) {
  return static_cast<std::uint32_t>(std::stoul(val));
}

std::uint64_t to_uint64(const std::string &val) { return std::stoull(val); }

std::string to_upper(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  return str;
}

const std::string &to_utf8(const std::string &str) { return str; }

std::string to_utf8(const std::wstring &str) {
  return str.empty() ? ""
                     : std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(str);
}

std::string &trim(std::string &str) { return right_trim(left_trim(str)); }

std::string &trim(std::string &str, const char &c) { return right_trim(left_trim(str, c), c); }

std::string trim_copy(std::string str) { return right_trim(left_trim(str)); }

std::string trim_copy(std::string str, const char &c) { return right_trim(left_trim(str, c), c); }
} // namespace repertory::utils::string
