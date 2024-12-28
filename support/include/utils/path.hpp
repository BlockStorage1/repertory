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
#ifndef REPERTORY_INCLUDE_UTILS_PATH_HPP_
#define REPERTORY_INCLUDE_UTILS_PATH_HPP_

#include "utils/config.hpp"
#include "utils/string.hpp"

namespace repertory::utils::path {
inline constexpr const std::string_view backslash{"\\"};
inline constexpr const std::wstring_view backslash_w{L"\\"};
inline constexpr const std::string_view dot{"."};
inline constexpr const std::wstring_view dot_w{L"."};
inline constexpr const std::string_view dot_backslash{".\\"};
inline constexpr const std::wstring_view dot_backslash_w{L".\\"};
inline constexpr const std::string_view dot_slash{"./"};
inline constexpr const std::wstring_view dot_slash_w{L"./"};
inline constexpr const std::string_view long_notation{"\\\\?\\"};
inline constexpr const std::wstring_view long_notation_w{L"\\\\?\\"};
inline constexpr const std::string_view slash{"/"};
inline constexpr const std::wstring_view slash_w{L"/"};
#if defined(_WIN32)
inline constexpr const std::string_view directory_seperator{backslash};
inline constexpr const std::wstring_view directory_seperator_w{backslash_w};
inline constexpr const std::string_view not_directory_seperator{slash};
inline constexpr const std::wstring_view not_directory_seperator_w{slash_w};
inline constexpr const std::string_view unc_notation{"\\\\"};
inline constexpr const std::wstring_view unc_notation_w{L"\\\\"};
#else  // !defined(_WIN32)
inline constexpr const std::string_view directory_seperator{slash};
inline constexpr const std::wstring_view directory_seperator_w{slash_w};
inline constexpr const std::string_view not_directory_seperator{backslash};
inline constexpr const std::wstring_view not_directory_seperator_w{backslash_w};
#endif // defined(_WIN32)

template <typename char_t>
[[nodiscard]] inline constexpr auto
get_backslash() -> std::basic_string_view<char_t>;

template <>
[[nodiscard]] inline constexpr auto
get_backslash<char>() -> std::basic_string_view<char> {
  return backslash;
}

template <>
[[nodiscard]] inline constexpr auto
get_backslash<wchar_t>() -> std::basic_string_view<wchar_t> {
  return backslash_w;
}

template <typename char_t>
[[nodiscard]] inline constexpr auto
get_directory_seperator() -> std::basic_string_view<char_t>;

template <>
[[nodiscard]] inline constexpr auto
get_directory_seperator<char>() -> std::basic_string_view<char> {
  return directory_seperator;
}

template <>
[[nodiscard]] inline constexpr auto
get_directory_seperator<wchar_t>() -> std::basic_string_view<wchar_t> {
  return directory_seperator_w;
}

template <typename char_t>
[[nodiscard]] inline constexpr auto
get_not_directory_seperator() -> std::basic_string_view<char_t>;

template <>
[[nodiscard]] inline constexpr auto
get_not_directory_seperator<char>() -> std::basic_string_view<char> {
  return not_directory_seperator;
}

template <>
[[nodiscard]] inline constexpr auto
get_not_directory_seperator<wchar_t>() -> std::basic_string_view<wchar_t> {
  return not_directory_seperator_w;
}

template <typename char_t>
[[nodiscard]] inline constexpr auto get_dot() -> std::basic_string_view<char_t>;

template <>
[[nodiscard]] inline constexpr auto
get_dot<char>() -> std::basic_string_view<char> {
  return dot;
}

template <>
[[nodiscard]] inline constexpr auto
get_dot<wchar_t>() -> std::basic_string_view<wchar_t> {
  return dot_w;
}

template <typename char_t>
[[nodiscard]] inline constexpr auto
get_dot_backslash() -> std::basic_string_view<char_t>;

template <>
[[nodiscard]] inline constexpr auto
get_dot_backslash<char>() -> std::basic_string_view<char> {
  return dot_backslash;
}

template <>
[[nodiscard]] inline constexpr auto
get_dot_backslash<wchar_t>() -> std::basic_string_view<wchar_t> {
  return dot_backslash_w;
}

template <typename char_t>
[[nodiscard]] inline constexpr auto
get_dot_slash() -> std::basic_string_view<char_t>;

template <>
[[nodiscard]] inline constexpr auto
get_dot_slash<char>() -> std::basic_string_view<char> {
  return dot_slash;
}

template <>
[[nodiscard]] inline constexpr auto
get_dot_slash<wchar_t>() -> std::basic_string_view<wchar_t> {
  return dot_slash_w;
}

template <typename char_t>
[[nodiscard]] inline constexpr auto
get_long_notation() -> std::basic_string_view<char_t>;

template <>
[[nodiscard]] inline constexpr auto
get_long_notation<char>() -> std::basic_string_view<char> {
  return long_notation;
}

template <>
[[nodiscard]] inline constexpr auto
get_long_notation<wchar_t>() -> std::basic_string_view<wchar_t> {
  return long_notation_w;
}

template <typename char_t>
[[nodiscard]] inline constexpr auto
get_slash() -> std::basic_string_view<char_t>;

template <>
[[nodiscard]] inline constexpr auto
get_slash<char>() -> std::basic_string_view<char> {
  return slash;
}

template <>
[[nodiscard]] inline constexpr auto
get_slash<wchar_t>() -> std::basic_string_view<wchar_t> {
  return slash_w;
}

#if defined(_WIN32)
template <typename char_t>
[[nodiscard]] inline constexpr auto
get_unc_notation() -> std::basic_string_view<char_t>;

template <>
[[nodiscard]] inline constexpr auto
get_unc_notation<char>() -> std::basic_string_view<char> {
  return unc_notation;
}

template <>
[[nodiscard]] inline constexpr auto
get_unc_notation<wchar_t>() -> std::basic_string_view<wchar_t> {
  return unc_notation_w;
}
#endif // defined(_WIN32)

template <typename string_t>
[[nodiscard]] inline auto get_current_path() -> string_t;

[[nodiscard]] auto absolute(std::string_view path) -> std::string;

[[nodiscard]] auto absolute(std::wstring_view path) -> std::wstring;

[[nodiscard]] inline auto
combine(std::string_view path,
        const std::vector<std::string_view> &paths) -> std::string;

[[nodiscard]] inline auto
combine(std::wstring_view path,
        const std::vector<std::wstring_view> &paths) -> std::wstring;

[[nodiscard]] auto contains_trash_directory(std::string_view path) -> bool;

[[nodiscard]] auto contains_trash_directory(std::wstring_view path) -> bool;

[[nodiscard]] auto inline create_api_path(std::string_view path) -> std::string;

[[nodiscard]] auto inline create_api_path(std::wstring_view path)
    -> std::wstring;

[[nodiscard]] auto exists(std::string_view path) -> bool;

[[nodiscard]] auto exists(std::wstring_view path) -> bool;

[[nodiscard]] inline auto finalize(std::string_view path) -> std::string;

[[nodiscard]] inline auto finalize(std::wstring_view path) -> std::wstring;

[[nodiscard]] auto
find_program_in_path(const std::string &name_without_extension) -> std::string;

[[nodiscard]] auto
find_program_in_path(std::wstring_view name_without_extension) -> std::wstring;

template <typename string_t>
inline auto
format_path(string_t &path,
            std::basic_string_view<typename string_t::value_type> sep,
            std::basic_string_view<typename string_t::value_type> not_sep)
    -> string_t &;

[[nodiscard]] inline auto
get_parent_api_path(std::string_view path) -> std::string;

[[nodiscard]] inline auto
get_parent_api_path(std::wstring_view path) -> std::wstring;

[[nodiscard]] auto get_parent_path(std::string_view path) -> std::string;

[[nodiscard]] auto get_parent_path(std::wstring_view path) -> std::wstring;

[[nodiscard]] inline auto
get_parts(std::string_view path) -> std::vector<std::string>;

[[nodiscard]] inline auto
get_parts_w(std::wstring_view path) -> std::vector<std::wstring>;

[[nodiscard]] auto get_relative_path(std::string_view path,
                                     std::string_view root_path) -> std::string;

[[nodiscard]] auto
get_relative_path(std::wstring_view path,
                  std::wstring_view root_path) -> std::wstring;

[[nodiscard]] auto make_file_uri(std::string_view path) -> std::string;

[[nodiscard]] auto make_file_uri(std::wstring_view path) -> std::wstring;

[[nodiscard]] auto strip_to_file_name(std::string path) -> std::string;

[[nodiscard]] auto strip_to_file_name(std::wstring path) -> std::wstring;

[[nodiscard]] auto unmake_file_uri(std::string_view uri) -> std::string;

[[nodiscard]] auto unmake_file_uri(std::wstring_view uri) -> std::wstring;

template <typename string_t>
[[nodiscard]] inline auto combine_t(
    std::basic_string_view<typename string_t::value_type> path,
    const std::vector<std::basic_string_view<typename string_t::value_type>>
        &paths) -> string_t {
  auto dir_sep_t =
      string_t{get_directory_seperator<typename string_t::value_type>()};
  return absolute(
      std::accumulate(paths.begin(), paths.end(),
                      std::basic_string<typename string_t::value_type>{path},
                      [&dir_sep_t](auto next_path, auto &&path_part) {
                        if (next_path.empty()) {
                          return string_t{path_part};
                        }

                        return next_path + dir_sep_t + string_t{path_part};
                      }));
}

inline auto combine(std::string_view path,
                    const std::vector<std::string_view> &paths) -> std::string {
  return combine_t<std::string>(path, paths);
}

inline auto
combine(std::wstring_view path,
        const std::vector<std::wstring_view> &paths) -> std::wstring {
  return combine_t<std::wstring>(path, paths);
}

template <typename string_t>
[[nodiscard]] inline auto create_api_path_t(
    std::basic_string_view<typename string_t::value_type> path) -> string_t {
  auto backslash_t = get_backslash<typename string_t::value_type>();
  auto dot_backslash_t = get_dot_backslash<typename string_t::value_type>();
  auto dot_slash_t = get_dot_slash<typename string_t::value_type>();
  auto dot_t = get_dot<typename string_t::value_type>();
  auto slash_t = get_slash<typename string_t::value_type>();

#if defined(_WIN32)
  auto long_notation_t = get_long_notation<typename string_t::value_type>();
  if (utils::string::begins_with(path, long_notation_t)) {
    path = path.substr(long_notation_t.size());
  }
#endif // defined(_WIN32)

  if (path.empty() || path == backslash_t || path == dot_t ||
      path == dot_slash_t || path == slash_t || path == dot_backslash_t) {
    return string_t{slash_t};
  }

  string_t api_path{path};
#if defined(_WIN32)
  if ((api_path.size() >= 2U) && (api_path.at(1U) == ':')) {
    api_path = api_path.substr(2U);
  }
#endif // defined(_WIN32)

  format_path(api_path, slash_t, backslash_t);

  while (utils::string::begins_with(api_path, dot_slash_t)) {
    api_path = api_path.substr(dot_slash_t.size());
  }

  if (api_path.at(0U) != slash_t.at(0U)) {
    return string_t{slash_t} + api_path;
  }

  return api_path;
}

inline auto create_api_path(std::string_view path) -> std::string {
  return create_api_path_t<std::string>(path);
}

inline auto create_api_path(std::wstring_view path) -> std::wstring {
  return create_api_path_t<std::wstring>(path);
}

template <typename string_t>
[[nodiscard]] inline auto finalize_t(
    std::basic_string_view<typename string_t::value_type> path) -> string_t {
  string_t dir_sep_t{get_directory_seperator<typename string_t::value_type>()};
  string_t fmt_path{path};
  if (fmt_path.empty()) {
    return fmt_path;
  }

  format_path(fmt_path, dir_sep_t,
              get_not_directory_seperator<typename string_t::value_type>());

#if defined(_WIN32)
  auto unc_notation_t = get_unc_notation<typename string_t::value_type>();
  if (utils::string::begins_with(fmt_path, unc_notation_t)) {
    return fmt_path;
  }

  auto dot_t = get_dot<typename string_t::value_type>();
  auto dot_sep_t = string_t{dot_t} + dir_sep_t;
  if (fmt_path == dot_t || fmt_path == dot_sep_t) {
    return get_current_path<string_t>();
  }

  if (fmt_path == dir_sep_t) {
#if defined(PROJECT_ENABLE_WIN32_LONG_PATH_NAMES)
    return get_current_path<string_t>().substr(0U, long_notation.size() + 2U);
#else  // !defined(PROJECT_ENABLE_WIN32_LONG_PATH_NAMES)
    return get_current_path<string_t>().substr(0U, 2U);
#endif // defined(PROJECT_ENABLE_WIN32_LONG_PATH_NAMES)
  }

  if (utils::string::begins_with(fmt_path, dir_sep_t)) {
#if defined(PROJECT_ENABLE_WIN32_LONG_PATH_NAMES)
    return get_current_path<string_t>().substr(0U, long_notation.size() + 2U) +
           fmt_path;
#else  // !defined(PROJECT_ENABLE_WIN32_LONG_PATH_NAMES)
    return get_current_path<string_t>().substr(0U, 2U) + fmt_path;
#endif // defined(PROJECT_ENABLE_WIN32_LONG_PATH_NAMES)
  }

  if (utils::string::begins_with(fmt_path, dot_sep_t)) {
    return get_current_path<string_t>() + dir_sep_t + fmt_path.substr(2U);
  }

#if defined(PROJECT_ENABLE_WIN32_LONG_PATH_NAMES)
  return string_t{get_long_notation<typename string_t::value_type>()} +
         fmt_path;
#endif // defined(PROJECT_ENABLE_WIN32_LONG_PATH_NAMES)
#endif // defined(_WIN32)

  return fmt_path;
}

inline auto finalize(std::string_view path) -> std::string {
  return finalize_t<std::string>(path);
}

inline auto finalize(std::wstring_view path) -> std::wstring {
  return finalize_t<std::wstring>(path);
}

template <typename string_t>
inline auto
format_path(string_t &path,
            std::basic_string_view<typename string_t::value_type> sep,
            std::basic_string_view<typename string_t::value_type> not_sep)
    -> string_t & {
  utils::string::replace(path, not_sep, sep);

#if defined(_WIN32)
  auto is_unc{false};
  auto long_notation_t = get_long_notation<typename string_t::value_type>();
  auto unc_notation_t = get_unc_notation<typename string_t::value_type>();
  if (utils::string::begins_with(path, long_notation_t)) {
    path = path.substr(long_notation_t.size());
  } else if (utils::string::begins_with(path, unc_notation_t)) {
    path = path.substr(unc_notation_t.size());
    utils::string::left_trim(path, sep.at(0U));
    is_unc = true;
  }
#endif // defined(_WIN32)

  string_t double_sep(2U, sep.at(0U));
  while (utils::string::contains(path, double_sep)) {
    utils::string::replace(path, double_sep, sep);
  }

  if (path != sep) {
    utils::string::right_trim(path, sep.at(0U));
  }

#if defined(_WIN32)
  if (is_unc) {
    path = string_t{unc_notation_t} + path;
  } else if ((path.size() >= 2U) && (path.at(1U) == ':')) {
    path[0U] = utils::string::to_lower(string_t(1U, path.at(0U))).at(0U);
  }
#endif // defined(_WIN32)

  return path;
}

template <>
[[nodiscard]] inline auto get_current_path<std::string>() -> std::string {
#if defined(_WIN32)
  std::string path;
  path.resize(repertory::max_path_length + 1U);
  ::GetCurrentDirectoryA(static_cast<DWORD>(path.size()), path.data());
  path = path.c_str();
  return finalize(path);
#else  // !defined(_WIN32)
  return finalize(std::filesystem::current_path().string());
#endif // defined(_WIN32)
}

template <>
[[nodiscard]] inline auto get_current_path<std::wstring>() -> std::wstring {
  return utils::string::from_utf8(get_current_path<std::string>());
}

template <typename string_t>
[[nodiscard]] inline auto get_parent_api_path_t(
    std::basic_string_view<typename string_t::value_type> path) -> string_t {
  auto slash_t = get_slash<typename string_t::value_type>();

  string_t ret{path};
  utils::string::right_trim(ret, slash_t.at(0U));

  if (ret == slash_t || ret.empty()) {
    return string_t{path};
  }

  auto sub_path = ret.substr(0, ret.rfind(slash_t) + 1);
  if (sub_path == slash_t) {
    return string_t{sub_path};
  }

  return sub_path;
}

inline auto get_parent_api_path(std::string_view path) -> std::string {
  return create_api_path(get_parent_api_path_t<std::string>(path));
}

inline auto get_parent_api_path(std::wstring_view path) -> std::wstring {
  return create_api_path(get_parent_api_path_t<std::wstring>(path));
}

template <typename string_t>
[[nodiscard]] inline auto
get_parts_t(std::basic_string_view<typename string_t::value_type> path)
    -> std::vector<string_t> {
  return utils::string::split(
      path, get_directory_seperator<typename string_t::value_type>().at(0U),
      false);
}

inline auto get_parts(std::string_view path) -> std::vector<std::string> {
  return get_parts_t<std::string>(path);
}

inline auto get_parts_w(std::wstring_view path) -> std::vector<std::wstring> {
  return get_parts_t<std::wstring>(path);
}
} // namespace repertory::utils::path

#endif // REPERTORY_INCLUDE_UTILS_PATH_HPP_
