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
#include "utils/path.hpp"

#include "utils/common.hpp"
#include "utils/error.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"
#include "utils/unix.hpp"

namespace {
[[nodiscard]] auto resolve(std::string path) -> std::string {
  REPERTORY_USES_FUNCTION_NAME();

#if defined(_WIN32)
  if (repertory::utils::string::contains(path, "~\\")) {
    repertory::utils::string::replace(path, "~\\", "%USERPROFILE%\\");
  }

  if (repertory::utils::string::contains(path, "~/")) {
    repertory::utils::string::replace(path, "~/", "%USERPROFILE%\\");
  }

  if (repertory::utils::string::contains(path, "%")) {
    auto size = ::ExpandEnvironmentStringsA(path.c_str(), nullptr, 0);

    std::string dest;
    dest.resize(size);
    ::ExpandEnvironmentStringsA(path.c_str(), dest.data(),
                                static_cast<DWORD>(dest.size()));
    path = dest.c_str();
  }
#else  // !defined (_WIN32)
  if (repertory::utils::string::contains(path, "~\\")) {
    repertory::utils::string::replace(path, "~\\", "~/");
  }

  if (repertory::utils::string::contains(path, "~/")) {
    std::string home{};
    auto res =
        repertory::utils::use_getpwuid(getuid(), [&home](struct passwd *pw) {
          home = (pw->pw_dir ? pw->pw_dir : "");
          if (home.empty() ||
              ((home == repertory::utils::path::slash) && (getuid() != 0))) {
            home = repertory::utils::path::combine("/home", {pw->pw_name});
          }
        });
    if (not res) {
      throw repertory::utils::error::create_exception(function_name,
                                                      {
                                                          "failed to getpwuid",
                                                          res.reason,
                                                      });
    }

    path = repertory::utils::string::replace(path, "~/", home + "/");
  }
#endif // defined (_WIN32)

  return repertory::utils::path::finalize(path);
}
} // namespace

namespace repertory::utils::path {
auto absolute(std::string_view path) -> std::string {
  std::string abs_path{path};
  if (abs_path.empty()) {
    return abs_path;
  }

  abs_path = resolve(abs_path);
#if defined(_WIN32)
  if (not utils::string::contains(abs_path, dot)) {
    return abs_path;
  }

  std::string temp;
  temp.resize(repertory::max_path_length + 1U);
  ::GetFullPathNameA(abs_path.c_str(), static_cast<DWORD>(temp.size()),
                     temp.data(), nullptr);
#else  // !defined(_WIN32)
  if (not utils::string::contains(abs_path, dot) ||
      utils::string::begins_with(abs_path, slash)) {
    return abs_path;
  }

  auto found{false};
  std::string tmp{abs_path};
  do {
    auto *res = realpath(tmp.c_str(), nullptr);
    if (res != nullptr) {
      abs_path =
          res + std::string{directory_seperator} + abs_path.substr(tmp.size());
      free(res);
      found = true;
    } else if (tmp == dot) {
      found = true;
    } else {
      tmp = dirname(tmp.data());
    }
  } while (not found);
#endif // defined(_WIN32)

  return finalize(abs_path);
}

auto absolute(std::wstring_view path) -> std::wstring {
  return utils::string::from_utf8(absolute(utils::string::to_utf8(path)));
}

auto exists(std::string_view path) -> bool {
  return utils::file::file{path}.exists() ||
         utils::file::directory{path}.exists();
}

auto exists(std::wstring_view path) -> bool {
  return exists(utils::string::to_utf8(path));
}

auto find_program_in_path(const std::string &name_without_extension)
    -> std::string {
  static std::mutex mtx{};
  static std::unordered_map<std::string, std::string> found_items{};

  mutex_lock lock(mtx);
  if (found_items.contains(name_without_extension)) {
    return found_items.at(name_without_extension);
  }

  auto path = utils::get_environment_variable("PATH");
  if (path.empty()) {
    return path;
  }

#if defined(_WIN32)
  static constexpr std::array<std::string_view, 4U> extension_list{
      ".bat",
      ".cmd",
      ".exe",
      ".ps1",
  };
  static constexpr auto split_char = ';';
#else  // !defined(_WIN32)
  static constexpr std::array<std::string_view, 2U> extension_list{
      "",
      ".sh",
  };
  static constexpr auto split_char = ':';
#endif // defined(_WIN32)

  auto search_path_list = utils::string::split(path, split_char, false);
  for (auto &&search_path : search_path_list) {
    for (auto &&extension : extension_list) {
      auto exec_path = combine(
          search_path, {name_without_extension + std::string{extension}});
      if (utils::file::file(exec_path).exists()) {
        found_items[name_without_extension] = exec_path;
        return exec_path;
      }
    }
  }

  return "";
}

[[nodiscard]] auto
find_program_in_path(std::wstring_view name_without_extension) -> std::wstring {
  return utils::string::from_utf8(
      find_program_in_path(utils::string::to_utf8(name_without_extension)));
}

auto get_parent_path(std::string_view path) -> std::string {
  auto abs_path = absolute(path);

#if defined(_WIN32)
  ::PathRemoveFileSpecA(abs_path.data());
  abs_path = abs_path.c_str();
#else  // !defined(_WIN32)
  abs_path = std::filesystem::path{abs_path}.parent_path().string();
#endif // defined(_WIN32)

  return finalize(abs_path);
}

auto get_parent_path(std::wstring_view path) -> std::wstring {
  return utils::string::from_utf8(
      get_parent_path(utils::string::to_utf8(path)));
}

auto get_relative_path(std::string_view path, std::string_view root_path)
    -> std::string {
  auto abs_path = absolute(path);
  auto abs_root_path =
      absolute(root_path) + std::string{get_directory_seperator<char>()};
#if defined(_WIN32)
  if (utils::string::to_lower(abs_path).starts_with(
          utils::string::to_lower(abs_root_path))) {
#else  // !defined(_WIN32)
  if (abs_path.starts_with(abs_root_path)) {
#endif // defined(_WIN32)
    return abs_path.substr(abs_root_path.size());
  }

  return abs_path;
}

auto get_relative_path(std::wstring_view path, std::wstring_view root_path)
    -> std::wstring {
  return utils::string::from_utf8(get_relative_path(
      utils::string::to_utf8(path), utils::string::to_utf8(root_path)));
}

auto contains_trash_directory(std::string_view path) -> bool {
  auto parts = utils::string::split(utils::string::to_lower(absolute(path)),
                                    get_directory_seperator<char>(), false);

  return std::find_if(parts.begin(), parts.end(), [](auto &&part) -> bool {
           return utils::string::begins_with(part, ".trash-") ||
                  part == ".trashes" || part == "$recycle.bin";
         }) != parts.end();
}

auto contains_trash_directory(std::wstring_view path) -> bool {
  return contains_trash_directory(utils::string::to_utf8(path));
}

auto make_file_uri(std::string_view path) -> std::string {
  auto abs_path = absolute(path);
#if defined(_WIN32)
  utils::string::replace(abs_path, backslash, slash);
  abs_path = std::string{slash} + abs_path;
#endif // defined(_WIN32)
  return "file://" + abs_path;
}

auto make_file_uri(std::wstring_view path) -> std::wstring {
  return utils::string::from_utf8(make_file_uri(utils::string::to_utf8(path)));
}

auto strip_to_file_name(std::string path) -> std::string {
  if (path == "." || path == "..") {
    return path;
  }

#if defined(_WIN32)
  return ::PathFindFileNameA(path.c_str());
#else  // !defined(_WIN32)
  return utils::string::contains(path, slash) ? basename(path.data()) : path;
#endif // defined(_WIN32)
}

auto strip_to_file_name(std::wstring path) -> std::wstring {
  return utils::string::from_utf8(
      strip_to_file_name(utils::string::to_utf8(path)));
}

auto unmake_file_uri(std::string_view uri) -> std::string {
  static constexpr std::array<std::string_view, 23U> escape_characters = {
      {
          " ", "<", ">", "#", "%", "+", "{", "}", "|", "\\", "^", "~",
          "[", "]", "`", ";", "/", "?", ":", "@", "=", "&",  "$",
      }};
  static constexpr std::array<std::string_view, 23U>
      escape_sequences_lower = {{
          "%20", "%3c", "%3e", "%23", "%25", "%2b", "%7b", "%7d",
          "%7c", "%5c", "%5e", "%7e", "%5b", "%5d", "%60", "%3b",
          "%2f", "%3f", "%3a", "%40", "%3d", "%26", "%24",
      }};
  static constexpr std::array<std::string_view, 23U>
      escape_sequences_upper = {{
          "%20", "%3C", "%3E", "%23", "%25", "%2B", "%7B", "%7D",
          "%7C", "%5C", "%5E", "%7E", "%5B", "%5D", "%60", "%3B",
          "%2F", "%3F", "%3A", "%40", "%3D", "%26", "%24",
      }};

  std::string ret_uri{uri};
#if defined(_WIN32)
  ret_uri = ret_uri.substr(8U);
#else
  ret_uri = ret_uri.substr(7U);
#endif

  for (std::size_t idx = 0U; idx < escape_characters.size(); idx++) {
    utils::string::replace(ret_uri, escape_sequences_lower.at(idx),
                           escape_characters.at(idx));
    utils::string::replace(ret_uri, escape_sequences_upper.at(idx),
                           escape_characters.at(idx));
  }

  return absolute(ret_uri);
}

auto unmake_file_uri(std::wstring_view uri) -> std::wstring {
  return utils::string::from_utf8(unmake_file_uri(utils::string::to_utf8(uri)));
}
} // namespace repertory::utils::path
