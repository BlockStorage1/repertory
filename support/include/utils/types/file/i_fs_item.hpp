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
#ifndef REPERTORY_INCLUDE_TYPES_FILE_I_FS_ITEM_HPP_
#define REPERTORY_INCLUDE_TYPES_FILE_I_FS_ITEM_HPP_

#include "utils/config.hpp"

#include "utils/error.hpp"
#include "utils/string.hpp"

namespace repertory::utils::file {
enum class time_type {
  accessed,
  created,
  modified,
  written,
};

struct file_times final {
  std::uint64_t accessed{};
  std::uint64_t created{};
  std::uint64_t modified{};
  std::uint64_t written{};

  [[nodiscard]] auto get(time_type type) const -> std::uint64_t {
    REPERTORY_USES_FUNCTION_NAME();

    switch (type) {
    case time_type::accessed:
      return accessed;
    case time_type::created:
      return created;
    case time_type::modified:
      return modified;
    case time_type::written:
      return written;
    }

    throw utils::error::create_exception(function_name,
                                         {
                                             "type_type not supported",
                                         });
  }
};

struct i_fs_item {
  using fs_item_t = std::unique_ptr<i_fs_item>;

  virtual ~i_fs_item() = default;

  [[nodiscard]] virtual auto copy_to(std::string_view to_path,
                                     bool overwrite) const -> bool = 0;

  [[nodiscard]] virtual auto copy_to(std::wstring_view new_path,
                                     bool overwrite) -> bool {
    return copy_to(utils::string::to_utf8(new_path), overwrite);
  }

  [[nodiscard]] virtual auto exists() const -> bool = 0;

  [[nodiscard]] virtual auto get_path() const -> std::string = 0;

  [[nodiscard]] virtual auto
  get_time(time_type type) const -> std::optional<std::uint64_t>;

  [[nodiscard]] virtual auto is_directory_item() const -> bool = 0;

  [[nodiscard]] auto is_file_item() const -> bool {
    return not is_directory_item();
  }

  [[nodiscard]] virtual auto is_symlink() const -> bool = 0;

  [[nodiscard]] virtual auto move_to(std::string_view new_path) -> bool = 0;

  [[nodiscard]] virtual auto move_to(std::wstring_view new_path) -> bool {
    return move_to(utils::string::to_utf8(new_path));
  }

  [[nodiscard]] virtual auto remove() -> bool = 0;

public:
  [[nodiscard]] virtual operator bool() const = 0;

protected:
  i_fs_item() noexcept = default;

  i_fs_item(const i_fs_item &) noexcept = default;

  i_fs_item(i_fs_item &&) noexcept = default;

  auto operator=(i_fs_item &&) noexcept -> i_fs_item & = default;

  auto operator=(const i_fs_item &) noexcept -> i_fs_item & = default;
};
} // namespace repertory::utils::file

#endif // REPERTORY_INCLUDE_TYPES_FILE_I_FS_ITEM_HPP_
