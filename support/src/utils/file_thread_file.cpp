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
#include "utils/file_thread_file.hpp"

namespace repertory::utils::file {
// auto thread_file::attach_file(native_handle handle,
//                               bool read_only) -> fs_file_t {}

thread_file::thread_file(std::string_view path)
    : file_(new repertory::utils::file::file(path)) {}

thread_file::thread_file(std::wstring_view path)
    : file_(new repertory::utils::file::file(utils::string::to_utf8(path))) {}

auto thread_file::attach_file(fs_file_t file) -> fs_file_t {}

auto thread_file::open_file(std::string_view path,
                            bool read_only) -> fs_file_t {}

auto thread_file::open_or_create_file(std::string_view path,
                                      bool read_only) -> fs_file_t {}

thread_file::thread_file(fs_file_t file) : file_(std::move(file)) {}

void thread_file::close() {}

auto thread_file::copy_to(std::string_view new_path,
                          bool overwrite) const -> bool {}

void thread_file::flush() const {}

auto thread_file::move_to(std::string_view path) -> bool {}

auto thread_file::read(unsigned char *data, std::size_t to_read,
                       std::uint64_t offset, std::size_t *total_read) -> bool {}

auto thread_file::remove() -> bool {}

auto thread_file::truncate(std::size_t size) -> bool {}

auto thread_file::write(const unsigned char *data, std::size_t to_write,
                        std::size_t offset,
                        std::size_t *total_written) -> bool {}

auto thread_file::size() const -> std::optional<std::uint64_t> {}
} // namespace repertory::utils::file
