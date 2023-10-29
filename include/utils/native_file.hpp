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
#ifndef INCLUDE_UTILS_NATIVEFILE_HPP_
#define INCLUDE_UTILS_NATIVEFILE_HPP_

#include "types/repertory.hpp"

namespace repertory {
class native_file final {
public:
  using native_file_ptr = std::shared_ptr<native_file>;

public:
  [[nodiscard]] static auto attach(native_handle handle) -> native_file_ptr {
    return std::shared_ptr<native_file>(new native_file(handle));
  }

  [[nodiscard]] static auto clone(const native_file_ptr &nativeFile)
      -> native_file_ptr;

  [[nodiscard]] static auto create_or_open(const std::string &source_path,
                                           bool should_chmod,
                                           native_file_ptr &nf) -> api_error;

  [[nodiscard]] static auto create_or_open(const std::string &source_path,
                                           native_file_ptr &nf) -> api_error;

  [[nodiscard]] static auto open(const std::string &source_path,
                                 native_file_ptr &nf) -> api_error;

  [[nodiscard]] static auto open(const std::string &source_path,
                                 bool should_chmod, native_file_ptr &nf)
      -> api_error;

private:
  explicit native_file(const native_handle &handle) : handle_(handle) {}

public:
  ~native_file();

private:
  native_handle handle_;

private:
  bool auto_close{false};
#ifdef _WIN32
  std::recursive_mutex read_write_mutex_;
#endif

public:
  [[nodiscard]] auto allocate(std::uint64_t file_size) -> bool;

  void close();

  [[nodiscard]] auto copy_from(const native_file_ptr &source) -> bool;

  [[nodiscard]] auto copy_from(const std::string &path) -> bool;

  void flush();

  [[nodiscard]] auto get_file_size(std::uint64_t &file_size) -> bool;

  [[nodiscard]] auto get_handle() -> native_handle;

#ifdef _WIN32
  [[nodiscard]] auto read_bytes(char *buffer, std::size_t read_size,
                                std::uint64_t read_offset,
                                std::size_t &bytes_read) -> bool;
#else
  [[nodiscard]] auto read_bytes(char *buffer, std::size_t read_size,
                                std::uint64_t read_offset,
                                std::size_t &bytes_read) -> bool;
#endif
  void set_auto_close(bool b) { auto_close = b; }

  [[nodiscard]] auto truncate(std::uint64_t file_size) -> bool;

#ifdef _WIN32
  [[nodiscard]] auto write_bytes(const char *buffer, std::size_t write_size,
                                 std::uint64_t write_offset,
                                 std::size_t &bytes_written) -> bool;
#else
  [[nodiscard]] auto write_bytes(const char *buffer, std::size_t write_size,
                                 std::uint64_t write_offset,
                                 std::size_t &bytes_written) -> bool;
#endif
};

using native_file_ptr = native_file::native_file_ptr;
} // namespace repertory

#endif // INCLUDE_UTILS_NATIVEFILE_HPP_
