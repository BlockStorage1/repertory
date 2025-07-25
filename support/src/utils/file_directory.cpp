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
#include "utils/file_directory.hpp"

#include "utils/common.hpp"
#include "utils/error.hpp"
#include "utils/string.hpp"
#include "utils/unix.hpp"
#include "utils/windows.hpp"

namespace {
auto traverse_directory(
    std::string_view path,
    std::function<bool(repertory::utils::file::directory)> directory_action,
    std::function<bool(repertory::utils::file::file)> file_action,
    repertory::stop_type *stop_requested) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  auto res{true};

  const auto is_stop_requested = [&stop_requested]() -> bool {
    return (stop_requested != nullptr && *stop_requested);
  };

#if defined(_WIN32)
  WIN32_FIND_DATAA fd{};
  auto search = repertory::utils::path::combine(path, {"*.*"});
  auto find = ::FindFirstFileA(search.c_str(), &fd);
  if (find == INVALID_HANDLE_VALUE) {
    throw repertory::utils::error::create_exception(
        function_name,
        {
            "failed to open directory",
            std::to_string(repertory::utils::get_last_error_code()),
            path,
        });
  }

  do {
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if ((std::string_view(fd.cFileName) != ".") &&
          (std::string_view(fd.cFileName) != "..")) {
        res = directory_action(repertory::utils::file::directory{
            repertory::utils::path::combine(path, {fd.cFileName}),
            stop_requested,
        });
      }
    } else {
      res = file_action(repertory::utils::file::file(
          repertory::utils::path::combine(path, {fd.cFileName})));
    }
  } while (res && (::FindNextFileA(find, &fd) != 0) && !is_stop_requested());

  ::FindClose(find);
#else  // !defined(_WIN32)
  auto *root = opendir(std::string{path}.c_str());
  if (root == nullptr) {
    throw repertory::utils::error::create_exception(
        function_name,
        {
            "failed to open directory",
            std::to_string(repertory::utils::get_last_error_code()),
            path,
        });
  }

  struct dirent *de{nullptr};
  while (res && (de = readdir(root)) && !is_stop_requested()) {
    if (de->d_type == DT_DIR) {
      if ((std::string_view(de->d_name) == ".") ||
          (std::string_view(de->d_name) == "..")) {
        continue;
      }

      res = directory_action(repertory::utils::file::directory(
          repertory::utils::path::combine(path, {de->d_name})));
    } else {
      res = file_action(repertory::utils::file::file(
          repertory::utils::path::combine(path, {de->d_name})));
    }
  }

  closedir(root);
#endif // defined(_WIN32)

  return res;
}
} // namespace

namespace repertory::utils::file {
auto directory::copy_to(std::string_view new_path, bool overwrite) const
    -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    throw utils::error::create_exception(
        function_name, {
                           "failed to copy directory",
                           "not implemented",
                           utils::string::from_bool(overwrite),
                           new_path,
                           path_,
                       });
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto directory::count(bool recursive) const -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    std::uint64_t ret{0U};

    traverse_directory(
        path_,
        [&ret, &recursive](auto dir_item) -> bool {
          if (recursive) {
            ret += dir_item.count(true);
          }

          ++ret;
          return true;
        },
        [&ret](auto /* file_item */) -> bool {
          ++ret;
          return true;
        },
        stop_requested_);

    return ret;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return 0U;
}

auto directory::create_directory(std::string_view path) const
    -> fs_directory_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    auto abs_path = utils::path::combine(path_, {path});
    if (directory{abs_path, stop_requested_}.exists()) {
      return std::make_unique<directory>(abs_path);
    }

#if defined(_WIN32)
    auto res = ::SHCreateDirectory(nullptr,
                                   utils::string::from_utf8(abs_path).c_str());
    if (res != ERROR_SUCCESS) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to create directory",
                                               std::to_string(res),
                                               abs_path,
                                           });
    }
#else  // !defined(_WIN32)
    auto ret{true};
    auto paths =
        utils::string::split(abs_path, utils::path::directory_seperator, false);

    std::string current_path;
    for (std::size_t idx = 0U;
         !is_stop_requested() && ret && (idx < paths.size()); ++idx) {
      if (paths.at(idx).empty()) {
        current_path = utils::path::directory_seperator;
        continue;
      }

      current_path = utils::path::combine(current_path, {paths.at(idx)});
      auto status = mkdir(current_path.c_str(), S_IRWXU);
      ret = ((status == 0) || (errno == EEXIST));
    }
#endif // defined(_WIN32)

    return std::make_unique<directory>(abs_path);
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return nullptr;
}

auto directory::exists() const -> bool {
#if defined(_WIN32)
  return ::PathIsDirectoryA(path_.c_str()) != 0;
#else  // !defined(_WIN32)
  struct stat64 st{};
  return (stat64(path_.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
#endif // defined(_WIN32)

  return false;
}

auto directory::get_directory(std::string_view path) const -> fs_directory_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    auto dir_path = utils::path::combine(path_, {path});
    return fs_directory_t{
        directory{dir_path, stop_requested_}.exists()
            ? new directory(dir_path, stop_requested_)
            : nullptr,
    };
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return nullptr;
}

auto directory::get_directories() const -> std::vector<fs_directory_t> {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    std::vector<fs_directory_t> ret{};

    traverse_directory(
        path_,
        [this, &ret](auto dir_item) -> bool {
          ret.emplace_back(fs_directory_t{
              new directory(dir_item.get_path(), stop_requested_),
          });

          return true;
        },
        [](auto /* file_item */) -> bool { return true; }, stop_requested_);

    return ret;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return {};
}

auto directory::create_file(std::string_view file_name, bool read_only) const
    -> fs_file_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    auto file_path = utils::path::combine(path_, {file_name});
    return file::open_or_create_file(file_path, read_only);
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return nullptr;
}

auto directory::get_file(std::string_view path) const -> fs_file_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    auto file_path = utils::path::combine(path_, {path});
    return fs_file_t{
        file{file_path}.exists() ? new file(file_path) : nullptr,
    };
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return nullptr;
}

auto directory::get_files() const -> std::vector<fs_file_t> {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    std::vector<fs_file_t> ret{};

    traverse_directory(
        path_, [](auto /* dir_item */) -> bool { return true; },
        [&ret](auto file_item) -> bool {
          ret.emplace_back(fs_file_t{
              new file(file_item.get_path()),
          });
          return true;
        },
        stop_requested_);

    return ret;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return {};
}

auto directory::get_items() const -> std::vector<fs_item_t> {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    std::vector<fs_item_t> ret{};

    traverse_directory(
        path_,
        [this, &ret](auto dir_item) -> bool {
          ret.emplace_back(fs_item_t{
              new directory(dir_item.get_path(), stop_requested_),
          });
          return true;
        },
        [&ret](auto file_item) -> bool {
          ret.emplace_back(fs_item_t{
              new file(file_item.get_path()),
          });
          return true;
        },
        stop_requested_);

    return ret;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return {};
}

auto directory::is_stop_requested() const -> bool {
  return (stop_requested_ != nullptr) && *stop_requested_;
}

auto directory::is_symlink() const -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    return std::filesystem::is_symlink(path_);
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto directory::move_to(std::string_view new_path) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to move directory",
                                             "not implemented",
                                             new_path,
                                             path_,
                                         });
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto directory::remove() -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  return utils::retry_action([this]() -> bool {
    try {
#if defined(_WIN32)
      auto ret = not exists() || (::RemoveDirectoryA(path_.c_str()) != 0);
#else  // !defined(_WIN32)
      auto ret = not exists() || (rmdir(path_.c_str()) == 0);
#endif // defined(_WIN32)
      if (not ret) {
        utils::error::handle_error(function_name,
                                   utils::error::create_error_message({
                                       "failed to remove directory",
                                       path_,
                                   }));
      }

      return ret;
    } catch (const std::exception &e) {
      utils::error::handle_exception(function_name, e);
    } catch (...) {
      utils::error::handle_exception(function_name);
    }

    return false;
  });
}

auto directory::remove_recursively() -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not exists()) {
      return true;
    }

    if (not traverse_directory(
            path_,
            [](auto dir_item) -> bool { return dir_item.remove_recursively(); },
            [](auto file_item) -> bool { return file_item.remove(); },
            stop_requested_)) {
      return false;
    }

    return remove();
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto directory::size(bool recursive) const -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    std::uint64_t ret{0U};

    traverse_directory(
        path_,
        [&ret, &recursive](auto dir_item) -> bool {
          if (recursive) {
            ret += dir_item.size(true);
          }

          return true;
        },
        [&ret](auto file_item) -> bool {
          auto cur_size = file_item.size();
          if (cur_size.has_value()) {
            ret += cur_size.value();
          }
          return true;
        },
        stop_requested_);

    return ret;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return 0U;
}
} // namespace repertory::utils::file
