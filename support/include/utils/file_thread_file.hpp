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
#ifndef REPERTORY_INCLUDE_UTILS_FILE_THREAD_FILE_HPP_
#define REPERTORY_INCLUDE_UTILS_FILE_THREAD_FILE_HPP_

#include "utils/file.hpp"

namespace repertory::utils::file {
class thread_file final : public i_file {
public:
  // [[nodiscard]] static auto
  // attach_file(native_handle handle,
  //             bool read_only = false) -> fs_file_t;

  [[nodiscard]] static auto attach_file(fs_file_t file) -> fs_file_t;

  [[nodiscard]] static auto open_file(std::string_view path,
                                      bool read_only = false) -> fs_file_t;

  [[nodiscard]] static auto open_file(std::wstring_view path,
                                      bool read_only = false) -> fs_file_t {
    return open_file(utils::string::to_utf8(path), read_only);
  }

  [[nodiscard]] static auto
  open_or_create_file(std::string_view path,
                      bool read_only = false) -> fs_file_t;

  [[nodiscard]] static auto
  open_or_create_file(std::wstring_view path,
                      bool read_only = false) -> fs_file_t {
    return open_or_create_file(utils::string::to_utf8(path), read_only);
  }

public:
  thread_file() noexcept = default;

  thread_file(std::string_view path);

  thread_file(std::wstring_view path);

protected:
  thread_file(fs_file_t file);

public:
  thread_file(const thread_file &) = delete;

  thread_file(thread_file &&move_file) noexcept
      : file_(std::move(move_file.file_)) {}

  ~thread_file() override { close(); }

private:
  fs_file_t file_;

public:
  void close() override;

  [[nodiscard]] auto copy_to(std::string_view new_path,
                             bool overwrite) const -> bool override;

  [[nodiscard]] auto exists() const -> bool override { return file_->exists(); }

  void flush() const override;

  [[nodiscard]] auto get_handle() const -> native_handle override {
    return file_->get_handle();
  }

  [[nodiscard]] auto get_path() const -> std::string override {
    return file_->get_path();
  }

  [[nodiscard]] auto get_read_buffer_size() const -> std::uint32_t override {
    return file_->get_read_buffer_size();
  }

  [[nodiscard]] auto
  get_time(time_type type) const -> std::optional<std::uint64_t> override {
    return file_->get_time(type);
  }

  [[nodiscard]] auto is_read_only() const -> bool override {
    return file_->is_read_only();
  }

  [[nodiscard]] auto is_symlink() const -> bool override {
    return file_->is_symlink();
  }

  [[nodiscard]] auto move_to(std::string_view new_path) -> bool override;

  [[nodiscard]] auto read(unsigned char *data, std::size_t to_read,
                          std::uint64_t offset,
                          std::size_t *total_read = nullptr) -> bool override;

  [[nodiscard]] auto remove() -> bool override;

  auto set_read_buffer_size(std::uint32_t size) -> std::uint32_t override {
    return file_->set_read_buffer_size(size);
  }

  [[nodiscard]] auto size() const -> std::optional<std::uint64_t> override;

  [[nodiscard]] auto truncate(std::size_t size) -> bool override;

  [[nodiscard]] auto
  write(const unsigned char *data, std::size_t to_write, std::size_t offset,
        std::size_t *total_written = nullptr) -> bool override;

public:
  [[nodiscard]] operator bool() const override {
    return static_cast<bool>(*file_);
  }

  auto operator=(const thread_file &) noexcept -> thread_file & = delete;

  auto operator=(thread_file &&move_file) noexcept -> thread_file & {
    if (&move_file != this) {
      file_ = std::move(move_file.file_);
    }

    return *this;
  }
};
} // namespace repertory::utils::file

#endif // REPERTORY_INCLUDE_UTILS_FILE_THREAD_FILE_HPP_
