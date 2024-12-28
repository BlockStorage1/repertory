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
#ifndef REPERTORY_INCLUDE_TYPES_FILE_I_FILE_HPP_
#define REPERTORY_INCLUDE_TYPES_FILE_I_FILE_HPP_

#include "utils/config.hpp"

#include "utils/types/file/i_fs_item.hpp"

namespace repertory::utils::file {
struct i_file : public i_fs_item {
  using fs_file_t = std::unique_ptr<i_file>;

  virtual ~i_file() = default;

  virtual void close() = 0;

  virtual void flush() const = 0;

  [[nodiscard]] virtual auto get_handle() const -> native_handle = 0;

  [[nodiscard]] virtual auto get_read_buffer_size() const -> std::uint32_t = 0;

  [[nodiscard]] auto is_directory_item() const -> bool override {
    return false;
  }

  [[nodiscard]] virtual auto is_read_only() const -> bool = 0;

  [[nodiscard]] virtual auto read(data_buffer &data, std::uint64_t offset,
                                  std::size_t *total_read = nullptr) -> bool {
    return read(data.data(), data.size(), offset, total_read);
  }

  [[nodiscard]] virtual auto
  read(unsigned char *data, std::size_t to_read, std::uint64_t offset,
       std::size_t *total_read = nullptr) -> bool = 0;

  [[nodiscard]] virtual auto
  read_all(data_buffer &data, std::uint64_t offset,
           std::size_t *total_read = nullptr) -> bool;

  virtual auto set_read_buffer_size(std::uint32_t size) -> std::uint32_t = 0;

  [[nodiscard]] virtual auto size() const -> std::optional<std::uint64_t> = 0;

  [[nodiscard]] virtual auto truncate() -> bool { return truncate(0U); }

  [[nodiscard]] virtual auto truncate(std::size_t size) -> bool = 0;

  [[nodiscard]] virtual auto
  write(const data_buffer &data, std::uint64_t offset,
        std::size_t *total_written = nullptr) -> bool {
    return write(data.data(), data.size(), offset, total_written);
  }

  [[nodiscard]] virtual auto
  write(const unsigned char *data, std::size_t to_write, std::size_t offset,
        std::size_t *total_written = nullptr) -> bool = 0;

protected:
  i_file() noexcept = default;

  i_file(const i_file &) noexcept = default;

  i_file(i_file &&) noexcept = default;

  auto operator=(i_file &&) noexcept -> i_file & = default;

  auto operator=(const i_file &) noexcept -> i_file & = default;
};
} // namespace repertory::utils::file

#endif // REPERTORY_INCLUDE_TYPES_FILE_I_FILE_HPP_
