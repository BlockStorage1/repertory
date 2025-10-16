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
#include "utils/file_file.hpp"

#include "utils/collection.hpp"
#include "utils/common.hpp"
#include "utils/error.hpp"
#include "utils/path.hpp"

namespace {
[[nodiscard]] auto get_file_size(std::string_view path,
                                 std::uint64_t &file_size) -> bool {
  auto abs_path = repertory::utils::path::absolute(path);
  file_size = 0U;

#if defined(_WIN32)
  struct _stat64 u_stat{};
  auto res = _stat64(std::string{path}.c_str(), &u_stat);
  if (res != 0) {
    return false;
  }

  file_size = static_cast<std::uint64_t>(u_stat.st_size);
  return true;
#else  // !defined(_WIN32)
  std::error_code ec{};
  file_size = std::filesystem::file_size(abs_path, ec);
  return (ec.value() == 0);
#endif // defined(_WIN32)
}

[[nodiscard]] auto is_file(std::string_view path) -> bool {
  auto abs_path = repertory::utils::path::absolute(path);

#if defined(_WIN32)
  return ((::PathFileExistsA(abs_path.c_str()) != 0) &&
          (::PathIsDirectoryA(abs_path.c_str()) == 0));
#else  // !defined(_WIN32)
  struct stat64 u_stat{};
  return (stat64(abs_path.c_str(), &u_stat) == 0 &&
          not S_ISDIR(u_stat.st_mode));
#endif // defined(_WIN32)
}
} // namespace

namespace repertory::utils::file {
// auto file::attach_file(native_handle handle,
//                        bool read_only) -> fs_file_t {
// REPERTORY_USES_FUNCTION_NAME();
//
//   try {
//     std::string path;
//
// #if defined(_WIN32)
//     path.resize(repertory::max_path_length + 1U);
//     ::GetFinalPathNameByHandleA(handle, path.data(),
//                                 static_cast<DWORD>(path.size()),
//                                 FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
// #else // !defined(_WIN32)
//     path.resize(repertory::max_path_length + 1U);
//
// #if defined(__APPLE__)
//     fcntl(handle, F_GETPATH, source_path.data());
// #else  // !defined(__APPLE__)
//     readlink(("/proc/self/fd/" + std::to_string(handle)).c_str(),
//     path.data(),
//              path.size());
// #endif // defined(__APPLE__)
// #endif // defined(_WIN32)
//
//     path = path.c_str();
//
// #if defined(_WIN32)
//     auto *ptr = _fdopen(
//         static_cast<int>(_open_osfhandle(reinterpret_cast<intptr_t>(handle),
//                                          read_only ? _O_RDONLY : _O_RDWR)),
//         read_only ? "rb" : "rb+");
// #else  // !defined(_WIN32)
//     auto *ptr = fdopen(handle, read_only ? "rb" : "rb+");
// #endif // defined(_WIN32)
//
//     return fs_file_t(new file{
//         file_t{ptr},
//         utils::path::absolute(path),
//         read_only,
//     });
//   } catch (const std::exception &e) {
//     utils::error::handle_exception(function_name, e);
//   } catch (...) {
//     utils::error::handle_exception(function_name);
//   }
//
//   return nullptr;
// }

void file::open() {
  REPERTORY_USES_FUNCTION_NAME();

  if (not is_file(path_)) {
    throw utils::error::create_exception(function_name, {
                                                            "file not found",
                                                            path_,
                                                        });
  }

#if defined(_WIN32)
  file_ = file_t{
      _fsopen(path_.c_str(), read_only_ ? "rb" : "rb+", _SH_DENYNO),
      file_deleter(),
  };
#else  // !defined(_WIN32)
  file_ = file_t{
      fopen(path_.c_str(), read_only_ ? "rb" : "rb+"),
      file_deleter(),
  };
#endif // defined(_WIN32)
}

auto file::open_file(std::string_view path, bool read_only) -> fs_file_t {
  REPERTORY_USES_FUNCTION_NAME();

  auto *ptr = new file{
      nullptr,
      utils::path::absolute(path),
      read_only,
  };
  auto new_file = fs_file_t(ptr);

  try {
    ptr->open();
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return new_file;
}

auto file::open_or_create_file(std::string_view path, bool read_only)
    -> fs_file_t {
  auto abs_path = utils::path::absolute(path);
  if (not is_file(abs_path)) {
#if defined(_WIN32)
    int old_mode{};
    _umask_s(077, &old_mode);
#else  // !defined(_WIN32)
    auto old_mode = umask(077);
#endif // defined(_WIN32)

#if defined(_WIN32)
    auto *ptr = _fsopen(abs_path.c_str(), "ab+", _SH_DENYNO);
#else  // !defined(_WIN32)
    auto *ptr = fopen(abs_path.c_str(), "ab+");
#endif // defined(_WIN32)

    if (ptr != nullptr) {
      fclose(ptr);
    }

#if defined(_WIN32)
    _umask_s(old_mode, nullptr);
#else  // !defined(_WIN32)
    umask(old_mode);
#endif // defined(_WIN32)
  }

  return open_file(abs_path, read_only);
}

void file::close() { file_.reset(); }

auto file::copy_to(std::string_view new_path, bool overwrite) const -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    auto to_path = utils::path::absolute(new_path);
    if (directory(to_path).exists()) {
      return false;
    }

#if defined(_WIN32)
    return ::CopyFileA(path_.c_str(), to_path.c_str(),
                       overwrite ? TRUE : FALSE) != 0;
#else  // !defined(_WIN32)
    return std::filesystem::copy_file(
        path_, to_path,
        overwrite ? std::filesystem::copy_options::overwrite_existing
                  : std::filesystem::copy_options::skip_existing);
#endif // defined(_WIN32)
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto file::exists() const -> bool { return is_file(path_); }

void file::flush() const {
  if (file_) {
    fflush(file_.get());
  }
}

auto file::get_handle() const -> native_handle {
  if (file_) {
#if defined(_WIN32)
    return reinterpret_cast<native_handle>(
        _get_osfhandle(_fileno(file_.get())));
#else  // !defined(_WIN32)
    return fileno(file_.get());
#endif // defined(_WIN32)
  }

  return INVALID_HANDLE_VALUE;
}

auto file::is_symlink() const -> bool {
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

auto file::move_to(std::string_view path) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  auto abs_path = utils::path::absolute(path);

  auto reopen{false};
  if (file_) {
    reopen = true;
    close();
  }

  auto success{false};
#if defined(_WIN32)
  success = ::MoveFileExA(path_.c_str(), abs_path.c_str(),
                          MOVEFILE_REPLACE_EXISTING) != 0;
#else  // !defined(_WIN32)
  std::error_code ec{};
  std::filesystem::rename(path_, abs_path, ec);
  success = ec.value() == 0;
#endif // defined(_WIN32)

  if (success) {
    path_ = abs_path;
  }

  if (reopen) {
    try {
      open();
      return success;
    } catch (const std::exception &e) {
      utils::error::handle_exception(function_name, e);
    } catch (...) {
      utils::error::handle_exception(function_name);
    }
  }

  return false;
}

auto file::read(unsigned char *data, std::size_t to_read, std::uint64_t offset,
                std::size_t *total_read) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  if (total_read != nullptr) {
    (*total_read) = 0U;
  }

  try {
    if (not file_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "file is not open for reading",
                                               path_,
                                           });
    }

    if (fseeko(file_.get(), static_cast<std::int64_t>(offset), SEEK_SET) ==
        -1) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to seek before read",
                                               path_,
                                           });
    }

    std::size_t bytes_read{0U};
    while (bytes_read != to_read) {
      auto res =
          fread(&data[bytes_read], 1U, to_read - bytes_read, file_.get());
      if (not feof(file_.get()) && ferror(file_.get())) {
        throw utils::error::create_exception(function_name,
                                             {
                                                 "failed to read file bytes",
                                                 path_,
                                             });
      }

      if (res == 0) {
        break;
      }

      bytes_read += static_cast<std::size_t>(res);
    }

    if (total_read != nullptr) {
      (*total_read) = bytes_read;
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

#if defined(PROJECT_ENABLE_LIBSODIUM)
auto file::sha256() -> std::optional<std::string> {
  REPERTORY_USES_FUNCTION_NAME();

  auto should_close{false};
  auto read_only{read_only_};
  std::optional<std::string> ret;

  try {
    if (file_ == nullptr) {
      should_close = true;
      read_only_ = true;
      this->open();
    }

    crypto_hash_sha256_state state{};
    auto res = crypto_hash_sha256_init(&state);
    if (res != 0) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to initialize sha256",
                                               std::to_string(res),
                                               path_,
                                           });
    }

    {
      data_buffer buffer(get_read_buffer_size());
      std::uint64_t read_offset{0U};
      std::size_t bytes_read{0U};
      while (i_file::read(buffer, read_offset, &bytes_read)) {
        if (not bytes_read) {
          break;
        }

        read_offset += bytes_read;
        res = crypto_hash_sha256_update(
            &state, reinterpret_cast<const unsigned char *>(buffer.data()),
            bytes_read);
        if (res != 0) {
          throw utils::error::create_exception(function_name,
                                               {
                                                   "failed to update sha256",
                                                   std::to_string(res),
                                                   path_,
                                               });
        }
      }
    }

    std::array<unsigned char, crypto_hash_sha256_BYTES> out{};
    res = crypto_hash_sha256_final(&state, out.data());
    if (res != 0) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to finalize sha256",
                                               std::to_string(res),
                                               path_,
                                           });
    }

    ret = utils::collection::to_hex_string(out);
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  if (should_close) {
    read_only_ = read_only;
    close();
  }

  return ret;
}
#endif // defined(PROJECT_ENABLE_LIBSODIUM)

auto file::remove() -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  close();

  return utils::retry_action([this]() -> bool {
    try {
#if defined(_WIN32)
      auto ret = not exists() || (::DeleteFileA(path_.c_str()) != 0);
#else  // !defined(_WIN32)
      std::error_code ec{};
      auto ret = not exists() || std::filesystem::remove(path_, ec);
#endif // defined(_WIN32)
      if (not ret) {
        utils::error::handle_error(function_name,
                                   utils::error::create_error_message({
                                       "failed to remove file",
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

auto file::truncate(std::size_t size) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  auto reopen{false};
  if (file_) {
    reopen = true;
    close();
  }

  std::error_code ec{};
  std::filesystem::resize_file(path_, size, ec);

  auto success{ec.value() == 0};

  if (reopen) {
    try {
      open();
    } catch (const std::exception &e) {
      utils::error::handle_exception(function_name, e);
      success = false;
    } catch (...) {
      utils::error::handle_exception(function_name);
      success = false;
    }
  }

  return success;
}

auto file::write(const unsigned char *data, std::size_t to_write,
                 std::size_t offset, std::size_t *total_written) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  if (total_written != nullptr) {
    (*total_written) = 0U;
  }

  try {
    if (not file_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "file is not open for writing",
                                               path_,
                                           });
    }

    auto res = fseeko(file_.get(), static_cast<std::int64_t>(offset), SEEK_SET);
    if (res == -1) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to seek before write",
                                               path_,
                                           });
    }

    std::size_t bytes_written{0U};
    while (bytes_written != to_write) {
      auto written =
          fwrite(reinterpret_cast<const char *>(&data[bytes_written]), 1U,
                 to_write - bytes_written, file_.get());
      if (not feof(file_.get()) && ferror(file_.get())) {
        throw utils::error::create_exception(function_name,
                                             {
                                                 "failed to write file bytes",
                                                 path_,
                                             });
      }

      if (written == 0U) {
        break;
      }

      bytes_written += written;
    }

    flush();

    if (total_written != nullptr) {
      (*total_written) = bytes_written;
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto file::size() const -> std::optional<std::uint64_t> {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (file_) {
      if (fseeko(file_.get(), 0, SEEK_END) == -1) {
        throw utils::error::create_exception(function_name,
                                             {
                                                 "failed to seek",
                                                 path_,
                                             });
      }

      auto size = ftello(file_.get());
      if (size == -1) {
        throw utils::error::create_exception(function_name,
                                             {
                                                 "failed to get position",
                                                 path_,
                                             });
      }

      return static_cast<std::uint64_t>(size);
    }

    std::uint64_t size{};
    if (not get_file_size(path_, size)) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to get file size",
                                               path_,
                                           });
    }

    return size;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return std::nullopt;
}
} // namespace repertory::utils::file
