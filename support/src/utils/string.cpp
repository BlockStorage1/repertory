/*
  Copyright <2018-2024> <scott.e.graves@protonmail.com>

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
#include "utils/string.hpp"

namespace repertory::utils::string {
auto from_bool(bool val) -> std::string {
  return std::to_string(static_cast<int>(val));
}

#if defined(PROJECT_ENABLE_BOOST)
auto from_dynamic_bitset(const boost::dynamic_bitset<> &bitset) -> std::string {
  std::stringstream stream;
  boost::archive::text_oarchive archive(stream);
  archive << bitset;
  return stream.str();
}
#endif // defined(PROJECT_ENABLE_BOOST)

auto from_utf8(std::string_view str) -> std::wstring {
  return str.empty()
             ? L""
             : std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>()
                   .from_bytes(std::string{str});
}

#if defined(PROJECT_ENABLE_SFML)
auto replace_sf(sf::String &src, const sf::String &find, const sf::String &with,
                std::size_t start_pos) -> sf::String & {
  if (not src.isEmpty() && (start_pos < src.getSize())) {
    while ((start_pos = src.find(find, start_pos)) != std::string::npos) {
      src.replace(start_pos, find.getSize(), with);
      start_pos += with.getSize();
    }
  }

  return src;
}

auto split_sf(sf::String str, wchar_t delim,
              bool should_trim) -> std::vector<sf::String> {
  auto result = std::views::split(str.toWideString(), delim);

  std::vector<sf::String> ret{};
  for (auto &&word : result) {
    auto val = std::wstring{word.begin(), word.end()};
    if (should_trim) {
      trim(val);
    }
    ret.emplace_back(val);
  }

  return ret;
}
#endif // defined(PROJECT_ENABLE_SFML)

auto to_bool(std::string val) -> bool {
  auto ret = false;

  trim(val);
  if (is_numeric(val)) {
    if (contains(val, ".")) {
      ret = (to_double(val) != 0.0);
    } else {
      std::istringstream(val) >> ret;
    }
  } else {
    std::istringstream(to_lower(val)) >> std::boolalpha >> ret;
  }

  return ret;
}

auto to_double(const std::string &str) -> double { return std::stod(str); }

#if defined(PROJECT_ENABLE_BOOST)
auto to_dynamic_bitset(const std::string &val) -> boost::dynamic_bitset<> {
  std::stringstream stream(val);
  boost::dynamic_bitset<> bitset;
  boost::archive::text_iarchive archive(stream);
  archive >> bitset;
  return bitset;
}
#endif // defined(PROJECT_ENABLE_BOOST)

auto to_int32(const std::string &val) -> std::int32_t { return std::stoi(val); }

auto to_int64(const std::string &val) -> std::int64_t {
  return std::stoll(val);
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

auto to_utf8(std::string_view str) -> std::string { return std::string{str}; }

auto to_utf8(std::wstring_view str) -> std::string {
  return str.empty()
             ? ""
             : std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>()
                   .to_bytes(std::wstring{str});
}
} // namespace repertory::utils::string
