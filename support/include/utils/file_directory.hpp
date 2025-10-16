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
#ifndef REPERTORY_INCLUDE_UTILS_FILE_DIRECTORY_HPP_
#define REPERTORY_INCLUDE_UTILS_FILE_DIRECTORY_HPP_

#include "utils/file.hpp"

namespace repertory::utils::file {
class directory final : public i_directory {
public:
  using directory_t = std::unique_ptr<directory>;

  directory() noexcept = default;

  directory(std::string_view path, stop_type *stop_requested = nullptr)
      : path_(utils::path::absolute(path)), stop_requested_(stop_requested) {}

  directory(std::wstring_view path, stop_type *stop_requested = nullptr)
      : path_(utils::path::absolute(utils::string::to_utf8(path))),
        stop_requested_(stop_requested) {}

  directory(const directory &) noexcept = delete;

  directory(directory &&move_dir) noexcept = default;

  ~directory() override = default;

private:
  std::string path_;
  stop_type *stop_requested_{nullptr};

public:
  [[nodiscard]] auto copy_to(std::string_view new_path, bool overwrite) const
      -> bool override;

  [[nodiscard]] auto count(bool recursive = false) const
      -> std::uint64_t override;

  [[nodiscard]] auto create_directory(std::string_view path = "") const
      -> fs_directory_t override;

  [[nodiscard]] auto create_file(std::string_view file_name,
                                 bool read_only) const -> fs_file_t override;

  [[nodiscard]] auto exists() const -> bool override;

  [[nodiscard]] auto get_directory(std::string_view path) const
      -> fs_directory_t override;

  [[nodiscard]] auto get_directories() const
      -> std::vector<fs_directory_t> override;

  [[nodiscard]] auto get_file(std::string_view path) const
      -> fs_file_t override;

  [[nodiscard]] auto get_files() const -> std::vector<fs_file_t> override;

  [[nodiscard]] auto get_items() const -> std::vector<fs_item_t> override;

  [[nodiscard]] auto get_path() const -> std::string override { return path_; }

  [[nodiscard]] auto is_stop_requested() const -> bool override;

  [[nodiscard]] auto is_symlink() const -> bool override;

  [[nodiscard]] auto move_to(std::string_view new_path) -> bool override;

  [[nodiscard]] auto remove() -> bool override;

  [[nodiscard]] auto remove_recursively() -> bool override;

  [[nodiscard]] auto size(bool recursive = false) const
      -> std::uint64_t override;

public:
  auto operator=(const directory &) noexcept -> directory & = delete;

  auto operator=(directory &&move_dir) noexcept -> directory & = default;

  [[nodiscard]] operator bool() const override { return exists(); }
};

// INFO: has test
template <typename string_t>
[[nodiscard]] inline auto directory_exists_in_path_t(
    std::basic_string_view<typename string_t::value_type> path,
    std::basic_string_view<typename string_t::value_type> sub_directory)
    -> bool {
  return directory(utils::path::combine(path, {sub_directory})).exists();
}

// INFO: has test
inline auto directory_exists_in_path(std::string_view path,
                                     std::string_view sub_directory) -> bool {
  return directory_exists_in_path_t<std::string>(path, sub_directory);
}

// INFO: has test
inline auto directory_exists_in_path(std::wstring_view path,
                                     std::wstring_view sub_directory) -> bool {
  return directory_exists_in_path_t<std::wstring>(path, sub_directory);
}
} // namespace repertory::utils::file

#endif // REPERTORY_INCLUDE_UTILS_FILE_DIRECTORY_HPP_
