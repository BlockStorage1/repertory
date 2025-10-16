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
#include "utils/file_utils.hpp"

#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"

namespace repertory::utils::file {
auto get_directory_files(std::string_view path, bool oldest_first,
                         bool recursive) -> std::deque<std::string> {
  REPERTORY_USES_FUNCTION_NAME();

  auto abs_path = utils::path::absolute(path);
  std::deque<std::string> ret;
  std::unordered_map<std::string, std::uint64_t> lookup;
#if defined(_WIN32)
  WIN32_FIND_DATA fd{};
  auto search = utils::path::combine(abs_path, {"*.*"});
  auto find = ::FindFirstFileA(search.c_str(), &fd);
  if (find != INVALID_HANDLE_VALUE) {
    try {
      do {
        auto full_path = utils::path::combine(abs_path, {fd.cFileName});
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
            FILE_ATTRIBUTE_DIRECTORY) {
          std::string name{fd.cFileName};
          if (name == "." || name == ".." || not recursive) {
            continue;
          }

          auto sub_files =
              get_directory_files(full_path, oldest_first, recursive);
          ret.insert(ret.end(), sub_files.begin(), sub_files.end());
          continue;
        }

        ULARGE_INTEGER li{};
        li.HighPart = fd.ftLastWriteTime.dwHighDateTime;
        li.LowPart = fd.ftLastWriteTime.dwLowDateTime;
        lookup[full_path] = li.QuadPart;
        ret.emplace_back(full_path);
      } while (::FindNextFileA(find, &fd) != 0);
    } catch (const std::exception &e) {
      utils::error::raise_error(function_name, e,
                                "failed to get directory files");
    }
    ::FindClose(find);

    std::sort(ret.begin(), ret.end(), [&](auto &&path1, auto &&path2) -> bool {
      return (oldest_first != 0) == (lookup[path1] < lookup[path2]);
    });
  }
#else // !defined(_WIN32)
  auto *root = opendir(abs_path.c_str());
  if (root != nullptr) {
    try {
      struct dirent *entry{};
      while ((entry = readdir(root)) != nullptr) {
        std::string name{entry->d_name};
        if (entry->d_type == DT_DIR) {
          if (name == "." || name == ".." || not recursive) {
            continue;
          }

          auto sub_files = get_directory_files(
              utils::path::combine(abs_path, {name}), oldest_first, recursive);
          ret.insert(ret.end(), sub_files.begin(), sub_files.end());
          continue;
        }

        ret.emplace_back(utils::path::combine(abs_path, {name}));
      }
    } catch (const std::exception &e) {
      utils::error::raise_error(function_name, e,
                                "failed to get directory files");
    }

    closedir(root);

    const auto add_to_lookup = [&](std::string_view lookup_path) {
      if (lookup.find(std::string{lookup_path}) == lookup.end()) {
        struct stat u_stat{};
        stat(std::string{lookup_path}.c_str(), &u_stat);
#if defined(__APPLE__)
        lookup[std::string{lookup_path}] =
            (static_cast<std::uint64_t>(u_stat.st_mtimespec.tv_sec) *
             utils::time::NANOS_PER_SECOND) +
            static_cast<std::uint64_t>(u_stat.st_mtimespec.tv_nsec);
#else  // !defined(__APPLE__)
        lookup[std::string{lookup_path}] =
            (static_cast<std::uint64_t>(u_stat.st_mtim.tv_sec) *
             utils::time::NANOS_PER_SECOND) +
            static_cast<std::uint64_t>(u_stat.st_mtim.tv_nsec);
#endif // defined(__APPLE__)
      }
    };

    std::sort(ret.begin(), ret.end(), [&](auto &&path1, auto &&path2) -> bool {
      add_to_lookup(path1);
      add_to_lookup(path2);
      return (oldest_first != 0) == (lookup.at(path1) < lookup.at(path2));
    });
  }
#endif // defined(_WIN32)

  return ret;
}

auto read_file_lines(std::string_view path) -> std::vector<std::string> {
  std::vector<std::string> ret;
  if (utils::file::file(path).exists()) {
    std::ifstream stream(std::string{path});
    std::string current_line;
    while (not stream.eof() && std::getline(stream, current_line)) {
      ret.emplace_back(utils::string::right_trim(current_line, '\r'));
    }
    stream.close();
  }

  return ret;
}

auto reset_modified_time(std::string_view path) -> bool {
  auto ret{false};
#if defined(_WIN32)
  SYSTEMTIME st{};
  ::GetSystemTime(&st);

  FILETIME ft{};
  if ((ret = !!::SystemTimeToFileTime(&st, &ft))) {
    auto handle = ::CreateFileA(
        std::string{path}.c_str(), FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
        OPEN_EXISTING, 0, nullptr);
    if ((ret = (handle != INVALID_HANDLE_VALUE))) {
      ret = !!::SetFileTime(handle, nullptr, &ft, &ft);
      ::CloseHandle(handle);
    }
  }
#else  // !defined(_WIN32)
  auto desc = open(std::string{path}.c_str(), O_RDWR);
  ret = (desc != -1);
  if (ret) {
    ret = futimens(desc, nullptr) == 0;
    close(desc);
  }
#endif // defined(_WIN32)

  return ret;
}
} // namespace repertory::utils::file
