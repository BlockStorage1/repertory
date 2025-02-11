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
#ifndef REPERTORY_INCLUDE_TYPES_FILE_I_DIRECTORY_HPP_
#define REPERTORY_INCLUDE_TYPES_FILE_I_DIRECTORY_HPP_

#include "utils/config.hpp"

#include "utils/types/file/i_file.hpp"
#include "utils/types/file/i_fs_item.hpp"

namespace repertory::utils::file {
struct i_directory : public i_fs_item {
  using fs_directory_t = std::unique_ptr<i_directory>;
  using fs_file_t = i_file::fs_file_t;

  virtual ~i_directory() = default;

  [[nodiscard]] virtual auto
  count(bool recursive = false) const -> std::uint64_t = 0;

  [[nodiscard]] virtual auto
  create_directory(std::string_view path = "") const -> fs_directory_t = 0;

  [[nodiscard]] virtual auto create_file(std::string_view file_name,
                                         bool read_only) const -> fs_file_t = 0;

  [[nodiscard]] virtual auto
  get_directory(std::string_view path) const -> fs_directory_t = 0;

  [[nodiscard]] virtual auto
  get_directories() const -> std::vector<fs_directory_t> = 0;

  [[nodiscard]] virtual auto
  get_file(std::string_view path) const -> fs_file_t = 0;

  [[nodiscard]] virtual auto get_files() const -> std::vector<fs_file_t> = 0;

  [[nodiscard]] virtual auto get_items() const -> std::vector<fs_item_t> = 0;

  [[nodiscard]] auto is_directory_item() const -> bool override { return true; }

  [[nodiscard]] virtual auto is_stop_requested() const -> bool = 0;

  [[nodiscard]] virtual auto remove_recursively() -> bool = 0;

  [[nodiscard]] virtual auto
  size(bool recursive = false) const -> std::uint64_t = 0;

protected:
  i_directory() noexcept = default;

  i_directory(const i_directory &) noexcept = default;

  i_directory(i_directory &&) noexcept = default;

  auto operator=(i_directory &&) noexcept -> i_directory & = default;

  auto operator=(const i_directory &) noexcept -> i_directory & = default;
};
} // namespace repertory::utils::file

#endif // REPERTORY_INCLUDE_TYPES_FILE_I_DIRECTORY_HPP_
