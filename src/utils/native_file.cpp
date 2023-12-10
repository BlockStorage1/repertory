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
#include "utils/native_file.hpp"

#include "types/repertory.hpp"
#include "utils/string_utils.hpp"
#include "utils/unix/unix_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
auto native_file::get_handle() -> native_handle { return handle_; }

native_file::~native_file() {
  if (auto_close) {
    close();
  }
}

auto native_file::clone(const native_file_ptr &nf) -> native_file_ptr {
  std::string source_path;

#ifdef _WIN32
  source_path.resize(MAX_PATH + 1);
  ::GetFinalPathNameByHandleA(nf->get_handle(), &source_path[0u], MAX_PATH + 1,
                              FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
#else
  source_path.resize(PATH_MAX + 1);
#ifdef __APPLE__
  fcntl(nf->get_handle(), F_GETPATH, &source_path[0u]);
#else
  readlink(("/proc/self/fd/" + std::to_string(nf->get_handle())).c_str(),
           &source_path[0u], source_path.size());
#endif
#endif
  source_path = source_path.c_str();

  native_file_ptr clone;
  auto res = native_file::open(source_path, clone);
  if (res != api_error::success) {
    throw std::runtime_error("unable to open file|sp|" + source_path + "|err|" +
                             api_error_to_string(res));
  }

  return clone;
}

auto native_file::create_or_open(const std::string &source_path,
                                 [[maybe_unused]] bool should_chmod,
                                 native_file_ptr &nf) -> api_error {
#ifdef _WIN32
  auto handle = ::CreateFileA(source_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, nullptr);
#else
  auto handle = should_chmod ? ::open(source_path.c_str(),
                                      O_CREAT | O_RDWR | O_CLOEXEC, 0600u)
                             : ::open(source_path.c_str(), O_RDWR | O_CLOEXEC);
  if (should_chmod) {
    chmod(source_path.c_str(), 0600u);
  }
#endif
  nf = native_file::attach(handle);
  return ((handle == REPERTORY_INVALID_HANDLE) ? api_error::os_error
                                               : api_error::success);
}

auto native_file::create_or_open(const std::string &source_path,
                                 native_file_ptr &nf) -> api_error {
  return create_or_open(source_path, true, nf);
}

auto native_file::open(const std::string &source_path, native_file_ptr &nf)
    -> api_error {
  return open(source_path, true, nf);
}

auto native_file::open(const std::string &source_path,
                       [[maybe_unused]] bool should_chmod, native_file_ptr &nf)
    -> api_error {
#ifdef _WIN32
  auto handle = ::CreateFileA(source_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);
#else
  auto handle = should_chmod
                    ? ::open(source_path.c_str(), O_RDWR | O_CLOEXEC, 0600u)
                    : ::open(source_path.c_str(), O_RDONLY | O_CLOEXEC);
  if (should_chmod) {
    chmod(source_path.c_str(), 0600u);
  }
#endif
  nf = native_file::attach(handle);
  return ((handle == REPERTORY_INVALID_HANDLE) ? api_error::os_error
                                               : api_error::success);
}

auto native_file::allocate(std::uint64_t file_size) -> bool {
#ifdef _WIN32
  LARGE_INTEGER li{};
  li.QuadPart = file_size;
  return (::SetFilePointerEx(handle_, li, nullptr, FILE_BEGIN) &&
          ::SetEndOfFile(handle_));
#endif
#ifdef __linux__
  return (fallocate(handle_, 0, 0, static_cast<off_t>(file_size)) >= 0);
#endif
#ifdef __APPLE__
  return (ftruncate(handle_, file_size) >= 0);
#endif
}

void native_file::close() {
  if (handle_ != REPERTORY_INVALID_HANDLE) {
#ifdef WIN32
    ::CloseHandle(handle_);
#else
    ::close(handle_);
#endif
    handle_ = REPERTORY_INVALID_HANDLE;
  }
}

auto native_file::copy_from(const native_file_ptr &nf) -> bool {
  auto ret = false;
  std::uint64_t file_size = 0u;
  if ((ret = nf->get_file_size(file_size))) {
    data_buffer buffer;
    buffer.resize(65536u * 2u);
    std::uint64_t offset = 0u;
    while (ret && (file_size > 0u)) {
      std::size_t bytes_read{};
      if ((ret = nf->read_bytes(&buffer[0u], buffer.size(), offset,
                                bytes_read))) {
        std::size_t bytes_written = 0u;
        ret = write_bytes(&buffer[0u], bytes_read, offset, bytes_written);
        file_size -= bytes_read;
        offset += bytes_read;
      }
    }
    flush();
  }

  return ret;
}

auto native_file::copy_from(const std::string &path) -> bool {
  auto ret = false;
  native_file_ptr nf;
  if (native_file::create_or_open(path, nf) == api_error ::success) {
    ret = copy_from(nf);
    nf->close();
  }

  return ret;
}

void native_file::flush() {
#ifdef _WIN32
  recur_mutex_lock l(read_write_mutex_);
  ::FlushFileBuffers(handle_);
#else
  fsync(handle_);
#endif
}

auto native_file::get_file_size(std::uint64_t &file_size) -> bool {
  auto ret = false;
#ifdef _WIN32
  LARGE_INTEGER li{};
  if ((ret = ::GetFileSizeEx(handle_, &li) && (li.QuadPart >= 0))) {
    file_size = static_cast<std::uint64_t>(li.QuadPart);
  }
#else
#if __APPLE__
  struct stat st {};
  if (fstat(handle_, &st) >= 0) {
#else
  struct stat64 st {};
  if (fstat64(handle_, &st) >= 0) {
#endif
    if ((ret = (st.st_size >= 0))) {
      file_size = static_cast<uint64_t>(st.st_size);
    }
  }
#endif
  return ret;
}

#ifdef _WIN32
auto native_file::read_bytes(char *buffer, std::size_t read_size,
                             std::uint64_t read_offset, std::size_t &bytes_read)
    -> bool {
  recur_mutex_lock l(read_write_mutex_);

  auto ret = false;
  bytes_read = 0u;
  LARGE_INTEGER li{};
  li.QuadPart = read_offset;
  if ((ret = !!::SetFilePointerEx(handle_, li, nullptr, FILE_BEGIN))) {
    DWORD current_read = 0u;
    do {
      current_read = 0u;
      ret = !!::ReadFile(handle_, &buffer[bytes_read],
                         static_cast<DWORD>(read_size - bytes_read),
                         &current_read, nullptr);
      bytes_read += current_read;
    } while (ret && (bytes_read < read_size) && (current_read != 0));
  }

  if (ret && (read_size != bytes_read)) {
    ::SetLastError(ERROR_HANDLE_EOF);
  }

  return ret;
}
#else
auto native_file::read_bytes(char *buffer, std::size_t read_size,
                             std::uint64_t read_offset, std::size_t &bytes_read)
    -> bool {
  bytes_read = 0U;
  ssize_t result = 0;
  do {
    result = pread64(handle_, &buffer[bytes_read], read_size - bytes_read,
                     static_cast<off_t>(read_offset + bytes_read));
    if (result > 0) {
      bytes_read += static_cast<size_t>(result);
    }
  } while ((result > 0) && (bytes_read < read_size));

  return (result >= 0);
}
#endif
auto native_file::truncate(std::uint64_t file_size) -> bool {
#ifdef _WIN32
  recur_mutex_lock l(read_write_mutex_);
  LARGE_INTEGER li{};
  li.QuadPart = file_size;
  return (::SetFilePointerEx(handle_, li, nullptr, FILE_BEGIN) &&
          ::SetEndOfFile(handle_));
#else
  return (ftruncate(handle_, static_cast<off_t>(file_size)) >= 0);
#endif
}

#ifdef _WIN32
auto native_file::write_bytes(const char *buffer, std::size_t write_size,
                              std::uint64_t write_offset,
                              std::size_t &bytes_written) -> bool {
  recur_mutex_lock l(read_write_mutex_);

  bytes_written = 0u;
  auto ret = true;

  LARGE_INTEGER li{};
  li.QuadPart = write_offset;
  if ((ret = !!::SetFilePointerEx(handle_, li, nullptr, FILE_BEGIN))) {
    do {
      DWORD current_write = 0u;
      ret = !!::WriteFile(handle_, &buffer[bytes_written],
                          static_cast<DWORD>(write_size - bytes_written),
                          &current_write, nullptr);
      bytes_written += current_write;
    } while (ret && (bytes_written < write_size));
  }

  return ret;
}
#else
auto native_file::write_bytes(const char *buffer, std::size_t write_size,
                              std::uint64_t write_offset,
                              std::size_t &bytes_written) -> bool {
  bytes_written = 0U;
  ssize_t result = 0U;
  do {
    result =
        pwrite64(handle_, &buffer[bytes_written], write_size - bytes_written,
                 static_cast<off_t>(write_offset + bytes_written));
    if (result > 0) {
      bytes_written += static_cast<size_t>(result);
    }
  } while ((result >= 0) && (bytes_written < write_size));

  return (bytes_written == write_size);
}
#endif
} // namespace repertory
