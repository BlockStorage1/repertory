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
#ifndef INCLUDE_UTILS_NATIVEFILE_HPP_
#define INCLUDE_UTILS_NATIVEFILE_HPP_

#include "common.hpp"
#include "types/repertory.hpp"

namespace repertory {
class native_file final {
public:
  typedef std::shared_ptr<native_file> native_file_ptr;

public:
  static native_file_ptr attach(OSHandle handle) {
    return native_file_ptr(new native_file(handle));
  }

  static native_file_ptr clone(const native_file_ptr &nativeFile);

  static api_error create_or_open(const std::string &source_path, native_file_ptr &nf);

  static api_error open(const std::string &source_path, native_file_ptr &nf);

private:
  explicit native_file(const OSHandle &handle) : handle_(handle) {}

public:
  ~native_file() = default;

private:
  OSHandle handle_;
#ifdef _WIN32
  std::recursive_mutex read_write_mutex_;
#endif

public:
  bool allocate(const std::uint64_t &file_size);

  void close();

  bool copy_from(const native_file_ptr &source);

  bool copy_from(const std::string &path);

  void flush();

  bool get_file_size(std::uint64_t &file_size);

  OSHandle get_handle() { return handle_; }

#ifdef _WIN32
  bool read_bytes(char *buffer, const std::size_t &read_size, const std::uint64_t &read_offset,
                  std::size_t &bytes_read);
#else
  bool read_bytes(char *buffer, const std::size_t &read_size, const std::uint64_t &read_offset,
                  std::size_t &bytes_read);
#endif
  bool truncate(const std::uint64_t &file_size);

#ifdef _WIN32
  bool write_bytes(const char *buffer, const std::size_t &write_size,
                   const std::uint64_t &write_offset, std::size_t &bytes_written);
#else
  bool write_bytes(const char *buffer, const std::size_t &write_size,
                   const std::uint64_t &write_offset, std::size_t &bytes_written);
#endif
};

typedef native_file::native_file_ptr native_file_ptr;
} // namespace repertory

#endif // INCLUDE_UTILS_NATIVEFILE_HPP_
