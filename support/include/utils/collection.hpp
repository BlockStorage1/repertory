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
#ifndef REPERTORY_INCLUDE_UTILS_COLLECTION_HPP_
#define REPERTORY_INCLUDE_UTILS_COLLECTION_HPP_

#include "utils/config.hpp"

#include "utils/error.hpp"
#include "utils/string.hpp"

namespace repertory::utils::collection {
template <typename col_t>
[[nodiscard]] inline auto excludes(const col_t &collection,
                                   const typename col_t::value_type &val)
    -> bool;

template <typename col_t>
[[nodiscard]] inline auto includes(const col_t &collection,
                                   const typename col_t::value_type &val)
    -> bool;

template <typename val_t>
[[nodiscard]] inline auto from_hex_string(std::string_view str, val_t &val)
    -> bool;

template <typename val_t>
[[nodiscard]] inline auto from_hex_string(std::wstring_view str, val_t &val)
    -> bool;
template <typename col_t>
inline auto remove_element(col_t &collection,
                           const typename col_t::value_type &value) -> col_t &;

template <typename col_t>
[[nodiscard]] inline auto to_hex_string(const col_t &collection) -> std::string;

template <typename col_t>
[[nodiscard]] inline auto to_hex_wstring(const col_t &collection)
    -> std::wstring;

template <typename col_t>
inline auto excludes(const col_t &collection,
                     const typename col_t::value_type &val) -> bool {
  return std::find(collection.begin(), collection.end(), val) ==
         collection.end();
}

template <typename col_t>
inline auto includes(const col_t &collection,
                     const typename col_t::value_type &val) -> bool {
  return std::find(collection.begin(), collection.end(), val) !=
         collection.end();
}

template <typename val_t>
[[nodiscard]] inline auto from_hex_string_t(std::string_view str, val_t &val)
    -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  static constexpr const auto base16{16};

  try {
    val.clear();

    std::string fmt_val{str};
    utils::string::trim(fmt_val);
    if (fmt_val.empty()) {
      return true;
    }

    fmt_val = utils::string::to_lower(fmt_val);
    if (utils::string::begins_with(fmt_val, "0x")) {
      fmt_val = fmt_val.substr(2U);
    }

    if (fmt_val.empty()) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "hex string is invalid",
                                               str,
                                           });
    }

    if (fmt_val.length() % 2U) {
      fmt_val = '0' + fmt_val;
    }

    auto iter = std::find_if_not(
        fmt_val.begin(), fmt_val.end(), [](auto cur_char) -> bool {
          auto check = static_cast<std::uint32_t>(cur_char);
          return ((check >= 48U && check <= 57U) ||
                  (check >= 97U && check <= 102U));
        });
    if (iter != fmt_val.end()) {
      auto invalid_idx{std::distance(fmt_val.begin(), iter)};
      throw std::range_error(utils::error::create_error_message({
          function_name,
          "invalid character in hex string",
          std::to_string(invalid_idx),
          std::string(1U, str.at(invalid_idx)),
          str,
      }));
    }

    val.resize(fmt_val.length() / 2U);
    for (std::size_t idx = 0U; idx < fmt_val.length(); idx += 2U) {
      val.at(idx / 2U) = static_cast<typename val_t::value_type>(
          std::strtoul(fmt_val.substr(idx, 2U).c_str(), nullptr, base16));
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  val.clear();
  return false;
}
template <typename val_t>
inline auto from_hex_string(std::string_view str, val_t &val) -> bool {
  return from_hex_string_t<val_t>(str, val);
}

template <typename val_t>
inline auto from_hex_string(std::wstring_view str, val_t &val) -> bool {
  return from_hex_string_t<val_t>(utils::string::to_utf8(str), val);
}

template <typename col_t>
inline auto remove_element(col_t &collection,
                           const typename col_t::value_type &value) -> col_t & {
  collection.erase(std::remove(collection.begin(), collection.end(), value),
                   collection.end());
  return collection;
}

template <typename col_t>
inline auto to_hex_string(const col_t &collection) -> std::string {
  static_assert(sizeof(typename col_t::value_type) == 1U,
                "value_type must be 1 byte in size");
  static constexpr const auto mask = 0xFF;

  std::stringstream stream{};
  for (auto &&val : collection) {
    stream << std::setfill('0') << std::setw(2) << std::hex
           << (static_cast<std::uint32_t>(val) & mask);
  }

  return stream.str();
}

template <typename col_t>
inline auto to_hex_wstring(const col_t &collection) -> std::wstring {
  return utils::string::from_utf8(to_hex_string<col_t>(collection));
}
} // namespace repertory::utils::collection

#endif // REPERTORY_INCLUDE_UTILS_COLLECTION_HPP_
