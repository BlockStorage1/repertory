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
#ifndef REPERTORY_INCLUDE_UTILS_STRING_HPP_
#define REPERTORY_INCLUDE_UTILS_STRING_HPP_

#include "utils/config.hpp"

namespace repertory::utils::string {
template <typename string_t> struct chain_replace_with_hex;

template <typename string_t>
[[nodiscard]] inline auto
char_to_hex(typename string_t::value_type character) -> string_t;

[[nodiscard]] inline auto begins_with(std::string_view str,
                                      std::string_view val) -> bool;

[[nodiscard]] inline auto begins_with(std::wstring_view str,
                                      std::wstring_view val) -> bool;

template <typename string_t>
[[nodiscard]] inline auto case_insensitive_find_string(string_t in_str,
                                                       string_t for_str) ->
    typename string_t::const_iterator;

[[nodiscard]] inline auto contains(std::string_view str,
                                   std::string_view search) -> bool;

[[nodiscard]] inline auto contains(std::wstring_view str,
                                   std::wstring_view search) -> bool;

[[nodiscard]] inline auto ends_with(std::string_view str,
                                    std::string_view val) -> bool;

[[nodiscard]] inline auto ends_with(std::wstring_view str,
                                    std::wstring_view val) -> bool;

[[nodiscard]] auto from_bool(bool val) -> std::string;

#if defined(PROJECT_ENABLE_BOOST)
[[nodiscard]] auto
from_dynamic_bitset(const boost::dynamic_bitset<> &bitset) -> std::string;
#endif // defined(PROJECT_ENABLE_BOOST)

[[nodiscard]] auto from_utf8(std::string_view str) -> std::wstring;

[[nodiscard]] inline auto is_numeric(std::string_view str) -> bool;

[[nodiscard]] inline auto is_numeric(std::wstring_view str) -> bool;

template <typename string_t>
[[nodiscard]] inline auto join(const std::vector<string_t> &arr,
                               typename string_t::value_type delim) -> string_t;

template <typename string_t>
auto left_trim(string_t &str,
               typename string_t::value_type trim_ch = ' ') -> string_t &;

template <typename string_t>
inline auto replace(string_t &src, typename string_t::value_type character,
                    typename string_t::value_type with,
                    std::size_t start_pos = 0U) -> string_t &;

template <typename string_t>
inline auto replace(string_t &src,
                    std::basic_string_view<typename string_t::value_type> find,
                    std::basic_string_view<typename string_t::value_type> with,
                    std::size_t start_pos = 0U) -> string_t &;

template <typename string_t>
[[nodiscard]] inline auto replace_copy(string_t src,
                                       typename string_t::value_type character,
                                       typename string_t::value_type with,
                                       std::size_t start_pos = 0U) -> string_t;

template <typename string_t>
[[nodiscard]] inline auto
replace_copy(string_t src,
             std::basic_string_view<typename string_t::value_type> find,
             std::basic_string_view<typename string_t::value_type> with,
             std::size_t start_pos = 0U) -> string_t;

template <typename string_t>
[[nodiscard]] inline auto
replace_with_hex(string_t &str, typename string_t::value_type character)
    -> chain_replace_with_hex<string_t>;

template <typename string_t>
inline auto
right_trim(string_t &str,
           typename string_t::value_type trim_ch = ' ') -> string_t &;

[[nodiscard]] inline auto split(std::string_view str, char delim,
                                bool should_trim) -> std::vector<std::string>;

[[nodiscard]] inline auto split(std::wstring_view str, wchar_t delim,
                                bool should_trim) -> std::vector<std::wstring>;

[[nodiscard]] inline auto split(std::string_view str, std::string_view delim,
                                bool should_trim) -> std::vector<std::string>;

[[nodiscard]] inline auto split(std::wstring_view str, std::wstring_view delim,
                                bool should_trim) -> std::vector<std::wstring>;

#if defined(PROJECT_ENABLE_SFML)
auto replace_sf(sf::String &src, const sf::String &find, const sf::String &with,
                std::size_t start_pos = 0U) -> sf::String &;

[[nodiscard]] auto split_sf(sf::String str, wchar_t delim,
                            bool should_trim) -> std::vector<sf::String>;
#endif // defined(PROJECT_ENABLE_SFML)

[[nodiscard]] auto to_bool(std::string val) -> bool;

[[nodiscard]] auto to_double(const std::string &str) -> double;

#if defined(PROJECT_ENABLE_BOOST)
[[nodiscard]] auto
to_dynamic_bitset(const std::string &val) -> boost::dynamic_bitset<>;
#endif // defined(PROJECT_ENABLE_BOOST)

template <typename string_t>
[[nodiscard]] inline auto to_lower(string_t str) -> string_t;

[[nodiscard]] auto to_int32(const std::string &val) -> std::int32_t;

[[nodiscard]] auto to_int64(const std::string &val) -> std::int64_t;

[[nodiscard]] auto to_size_t(const std::string &val) -> std::size_t;

[[nodiscard]] auto to_uint8(const std::string &val) -> std::uint8_t;

[[nodiscard]] auto to_uint16(const std::string &val) -> std::uint16_t;

[[nodiscard]] auto to_uint32(const std::string &val) -> std::uint32_t;

[[nodiscard]] auto to_uint64(const std::string &val) -> std::uint64_t;

template <typename string_t>
[[nodiscard]] inline auto to_upper(string_t str) -> string_t;

[[nodiscard]] auto to_utf8(std::string_view str) -> std::string;

[[nodiscard]] auto to_utf8(std::wstring_view str) -> std::string;

template <typename string_t>
[[nodiscard]] inline auto zero_pad(string_t str, std::size_t count) -> string_t;

template <typename string_t> struct chain_replace_with_hex final {
  explicit chain_replace_with_hex(string_t &value) : str(value) {}

  chain_replace_with_hex(const chain_replace_with_hex &) = delete;

  chain_replace_with_hex(chain_replace_with_hex &&) = delete;

  ~chain_replace_with_hex() = default;

  auto operator=(const chain_replace_with_hex &) -> chain_replace_with_hex & =
                                                        delete;

  auto
  operator=(chain_replace_with_hex &&) -> chain_replace_with_hex & = delete;

  auto operator()(typename string_t::value_type character)
      -> chain_replace_with_hex {
    return replace_with_hex<string_t>(str, character);
  }

  string_t &str;
};

template <typename string_t>
inline auto trim(string_t &str,
                 typename string_t::value_type trim_ch = ' ') -> string_t &;

template <typename string_t>
[[nodiscard]] inline auto
trim_copy(string_t str,
          typename string_t::value_type trim_ch = ' ') -> string_t;

template <typename char_t>
[[nodiscard]] inline auto
begins_with_t(std::basic_string_view<char_t> str,
              std::basic_string_view<char_t> val) -> bool {
  return (str.find(val) == 0U);
}

inline auto begins_with(std::string_view str, std::string_view val) -> bool {
  return begins_with_t<std::string_view::value_type>(str, val);
}

inline auto begins_with(std::wstring_view str, std::wstring_view val) -> bool {
  return begins_with_t<std::wstring_view::value_type>(str, val);
}

template <typename string_t>
inline auto case_insensitive_find_string(string_t in_str, string_t for_str) ->
    typename string_t::const_iterator {
  static const auto compare_chars =
      [](typename string_t::value_type char_a,
         typename string_t::value_type char_b) -> bool {
    return (std::tolower(char_a) == std::tolower(char_b));
  };

  return (std::search(in_str.begin(), in_str.end(), for_str.begin(),
                      for_str.end(), compare_chars));
}

template <typename string_t>
inline auto char_to_hex(typename string_t::value_type character) -> string_t {
  std::basic_stringstream<typename string_t::value_type> stream;
  stream << '%' << std::setfill<typename string_t::value_type>('0')
         << std::setw(sizeof(character)) << std::hex
         << static_cast<std::uint32_t>(character);
  return stream.str();
}

template <typename char_t>
[[nodiscard]] inline auto
contains_t(std::basic_string_view<char_t> str,
           std::basic_string_view<char_t> search) -> bool {
  return (str.find(search) != std::basic_string_view<char_t>::npos);
}

inline auto contains(std::string_view str, std::string_view search) -> bool {
  return contains_t<std::string_view::value_type>(str, search);
}

inline auto contains(std::wstring_view str, std::wstring_view search) -> bool {
  return contains_t<std::wstring_view::value_type>(str, search);
}

template <typename char_t>
[[nodiscard]] inline auto
ends_with_t(std::basic_string_view<char_t> str,
            std::basic_string_view<char_t> val) -> bool {
  if (val.size() > str.size()) {
    return false;
  }

  return std::equal(val.rbegin(), val.rend(), str.rbegin());
}

inline auto ends_with(std::string_view str, std::string_view val) -> bool {
  return ends_with_t<std::string_view::value_type>(str, val);
}

inline auto ends_with(std::wstring_view str, std::wstring_view val) -> bool {
  return ends_with_t<std::wstring_view::value_type>(str, val);
}

template <typename char_t>
[[nodiscard]] inline auto
is_numeric_t(std::basic_string_view<char_t> str) -> bool {
  if ((str.length() > 1U) && (str.at(0U) == '+' || str.at(0U) == '-')) {
    str = str.substr(1U);
  }

  if (str.empty()) {
    return false;
  }

  auto has_decimal{false};
  return std::find_if(str.begin(), str.end(),
                      [&has_decimal](auto &&cur_ch) -> bool {
                        if (has_decimal && cur_ch == '.') {
                          return true;
                        }

                        has_decimal = has_decimal || cur_ch == '.';
                        if (has_decimal) {
                          return false;
                        }

                        return std::isdigit(cur_ch) == 0;
                      }) == str.end();
}

inline auto is_numeric(std::string_view str) -> bool {
  return is_numeric_t<std::string_view::value_type>(str);
}

inline auto is_numeric(std::wstring_view str) -> bool {
  return is_numeric_t<std::wstring_view::value_type>(str);
}

template <typename string_t>
inline auto join(const std::vector<string_t> &arr,
                 typename string_t::value_type delim) -> string_t {
  if (arr.empty()) {
    return {};
  }

  return std::accumulate(
      std::next(arr.begin()), arr.end(), arr[0U],
      [&delim](auto str, const auto &cur) { return str + delim + cur; });
}

template <typename string_t>
auto left_trim(string_t &str,
               typename string_t::value_type trim_ch) -> string_t & {
  str.erase(0, str.find_first_not_of(trim_ch));
  return str;
}

template <typename string_t>
inline auto replace(string_t &src, typename string_t::value_type character,
                    typename string_t::value_type with,
                    std::size_t start_pos) -> string_t & {
  if (start_pos < src.size()) {
    std::replace(std::next(src.begin(), start_pos), src.end(), character, with);
  }

  return src;
}

template <typename string_t>
inline auto replace(string_t &src,
                    std::basic_string_view<typename string_t::value_type> find,
                    std::basic_string_view<typename string_t::value_type> with,
                    std::size_t start_pos) -> string_t & {
  if (start_pos < src.size()) {
    while ((start_pos = src.find(find, start_pos)) != string_t::npos) {
      src.replace(start_pos, find.size(), with);
      start_pos += with.size();
    }
  }

  return src;
}

template <typename string_t>
inline auto replace_copy(string_t src, typename string_t::value_type character,
                         typename string_t::value_type with,
                         std::size_t start_pos) -> string_t {
  return replace(src, character, with, start_pos);
}

template <typename string_t>
inline auto
replace_copy(string_t src,
             std::basic_string_view<typename string_t::value_type> find,
             std::basic_string_view<typename string_t::value_type> with,
             std::size_t start_pos) -> string_t {
  return replace(src, find, with, start_pos);
}

template <typename string_t>
inline auto replace_with_hex(string_t &str,
                             typename string_t::value_type character)
    -> chain_replace_with_hex<string_t> {
  return chain_replace_with_hex(
      replace(str, string_t{1U, character}, char_to_hex<string_t>(character)));
}

template <typename string_t>
inline auto right_trim(string_t &str,
                       typename string_t::value_type trim_ch) -> string_t & {
  str.erase(str.find_last_not_of(trim_ch) + 1);
  return str;
}

template <typename string_t> inline auto to_lower(string_t str) -> string_t {
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  return str;
}

template <typename string_t> inline auto to_upper(string_t str) -> string_t {
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  return str;
}

template <typename string_t>
inline auto trim(string_t &str,
                 typename string_t::value_type trim_ch) -> string_t & {
  return right_trim(left_trim(str, trim_ch), trim_ch);
}

template <typename string_t>
inline auto trim_copy(string_t str,
                      typename string_t::value_type trim_ch) -> string_t {
  return trim(str, trim_ch);
}

template <typename string_t>
[[nodiscard]] inline auto
split_t(std::basic_string_view<typename string_t::value_type> str,
        typename string_t::value_type delim,
        bool should_trim) -> std::vector<string_t> {
  std::vector<string_t> ret;
  std::basic_stringstream<typename string_t::value_type> stream{string_t{str}};

  string_t val;
  while (std::getline(stream, val, delim)) {
    if (should_trim) {
      trim(val);
    }
    ret.push_back(std::move(val));
  }

  return ret;
}

inline auto split(std::string_view str, char delim,
                  bool should_trim) -> std::vector<std::string> {
  return split_t<std::string>(str, delim, should_trim);
}

inline auto split(std::wstring_view str, wchar_t delim,
                  bool should_trim) -> std::vector<std::wstring> {
  return split_t<std::wstring>(str, delim, should_trim);
}

template <typename string_t>
[[nodiscard]] inline auto
split_t(std::basic_string_view<typename string_t::value_type> str,
        std::basic_string_view<typename string_t::value_type> delim,
        bool should_trim) -> std::vector<string_t> {
  auto result = std::views::split(str, delim);

  std::vector<string_t> ret{};
  for (auto &&word : result) {
    auto val = string_t{word.begin(), word.end()};
    if (should_trim) {
      trim(val);
    }
    ret.push_back(std::move(val));
  }

  return ret;
}

inline auto split(std::string_view str, std::string_view delim,
                  bool should_trim) -> std::vector<std::string> {
  return split_t<std::string>(str, delim, should_trim);
}

inline auto split(std::wstring_view str, std::wstring_view delim,
                  bool should_trim) -> std::vector<std::wstring> {
  return split_t<std::wstring>(str, delim, should_trim);
}

template <typename string_t>
inline auto zero_pad(string_t str, std::size_t count) -> string_t {
  str.insert(str.begin(), count - str.length(), '0');
  return str;
}
} // namespace repertory::utils::string

#endif // REPERTORY_INCLUDE_UTILS_STRING_HPP_
