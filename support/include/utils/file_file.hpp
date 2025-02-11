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
#ifndef REPERTORY_INCLUDE_UTILS_FILE_FILE_HPP_
#define REPERTORY_INCLUDE_UTILS_FILE_FILE_HPP_

#include "utils/file.hpp"

namespace repertory::utils::file {
class file final : public i_file {
public:
  // [[nodiscard]] static auto
  // attach_file(native_handle handle,
  //             bool read_only = false) -> fs_file_t;

  // INFO: has test
  [[nodiscard]] static auto open_file(std::string_view path,
                                      bool read_only = false) -> fs_file_t;

  [[nodiscard]] static auto open_file(std::wstring_view path,
                                      bool read_only = false) -> fs_file_t {
    return open_file(utils::string::to_utf8(path), read_only);
  }

  // INFO: has test
  [[nodiscard]] static auto
  open_or_create_file(std::string_view path,
                      bool read_only = false) -> fs_file_t;

  [[nodiscard]] static auto
  open_or_create_file(std::wstring_view path,
                      bool read_only = false) -> fs_file_t {
    return open_or_create_file(utils::string::to_utf8(path), read_only);
  }

public:
  file() noexcept = default;

  file(std::string_view path)
      : file_(nullptr), path_(utils::path::absolute(path)) {}

  file(std::wstring_view path)
      : file_(nullptr),
        path_(utils::path::absolute(utils::string::to_utf8(path))) {}

private:
  file(file_t file_ptr, std::string_view path, bool read_only)
      : file_(std::move(file_ptr)), path_(path), read_only_(read_only) {}

public:
  file(const file &) = delete;

  file(file &&move_file) noexcept
      : file_(std::move(move_file.file_)),
        path_(std::move(move_file.path_)),
        read_only_(move_file.read_only_) {}

  ~file() override { close(); }

private:
  file_t file_;
  std::string path_;
  bool read_only_{false};

private:
  std::atomic_uint32_t read_buffer_size{65536U};

private:
  void open();

public:
  void close() override;

  [[nodiscard]] auto copy_to(std::string_view new_path,
                             bool overwrite) const -> bool override;

  [[nodiscard]] auto exists() const -> bool override;

  void flush() const override;

  [[nodiscard]] auto get_handle() const -> native_handle override;

  [[nodiscard]] auto get_path() const -> std::string override { return path_; }

  [[nodiscard]] auto get_read_buffer_size() const -> std::uint32_t override {
    return read_buffer_size;
  }

  [[nodiscard]] auto is_read_only() const -> bool override {
    return read_only_;
  }

  [[nodiscard]] auto is_symlink() const -> bool override;

  [[nodiscard]] auto move_to(std::string_view new_path) -> bool override;

  [[nodiscard]] auto read(unsigned char *data, std::size_t to_read,
                          std::uint64_t offset,
                          std::size_t *total_read = nullptr) -> bool override;

  [[nodiscard]] auto remove() -> bool override;

  auto set_read_buffer_size(std::uint32_t size) -> std::uint32_t override {
    read_buffer_size = size;
    return read_buffer_size;
  }

#if defined(PROJECT_ENABLE_LIBSODIUM)
  [[nodiscard]] auto sha256() -> std::optional<std::string>;

#endif // defined(PROJECT_ENABLE_LIBSODIUM)

  [[nodiscard]] auto size() const -> std::optional<std::uint64_t> override;

  [[nodiscard]] auto truncate(std::size_t size) -> bool override;

  [[nodiscard]] auto
  write(const unsigned char *data, std::size_t to_write, std::size_t offset,
        std::size_t *total_written = nullptr) -> bool override;

public:
  auto operator=(const file &) noexcept -> file & = delete;

  auto operator=(file &&move_file) noexcept -> file & {
    if (&move_file != this) {
      file_ = std::move(move_file.file_);
      path_ = std::move(move_file.path_);
      read_only_ = move_file.read_only_;
    }

    return *this;
  }

  [[nodiscard]] operator bool() const override { return file_ != nullptr; }
};

// INFO: has test
template <typename string_t>
[[nodiscard]] inline auto file_exists_in_path_t(
    std::basic_string_view<typename string_t::value_type> path,
    std::basic_string_view<typename string_t::value_type> file_name) -> bool {
  return file(utils::path::combine(path, {file_name})).exists();
}

// INFO: has test
inline auto file_exists_in_path(std::string_view path,
                                std::string_view file_name) -> bool {
  return file_exists_in_path_t<std::string>(path, file_name);
}

// INFO: has test
inline auto file_exists_in_path(std::wstring_view path,
                                std::wstring_view file_name) -> bool {
  return file_exists_in_path_t<std::wstring>(path, file_name);
}
} // namespace repertory::utils::file

#endif // REPERTORY_INCLUDE_UTILS_FILE_FILE_HPP_
