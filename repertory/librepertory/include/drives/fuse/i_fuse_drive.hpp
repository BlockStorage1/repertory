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
#ifndef REPERTORY_INCLUDE_DRIVES_FUSE_I_FUSE_DRIVE_HPP_
#define REPERTORY_INCLUDE_DRIVES_FUSE_I_FUSE_DRIVE_HPP_
#if !defined(_WIN32)

#include "types/repertory.hpp"

namespace repertory {
class i_fuse_drive {
  INTERFACE_SETUP(i_fuse_drive);

public:
  [[nodiscard]] virtual auto check_owner(const std::string &api_path) const
      -> api_error = 0;

  [[nodiscard]] virtual auto check_parent_access(const std::string &api_path,
                                                 int mask) const
      -> api_error = 0;

  [[nodiscard]] virtual auto
  get_directory_item_count(const std::string &api_path) const
      -> std::uint64_t = 0;

  [[nodiscard]] virtual auto
  get_directory_items(const std::string &api_path) const
      -> directory_item_list = 0;

  [[nodiscard]] virtual auto get_file_size(const std::string &api_path) const
      -> std::uint64_t = 0;

  [[nodiscard]] virtual auto get_item_meta(const std::string &api_path,
                                           api_meta_map &meta) const
      -> api_error = 0;

  [[nodiscard]] virtual auto get_item_meta(const std::string &api_path,
                                           const std::string &name,
                                           std::string &value) const
      -> api_error = 0;

  [[nodiscard]] virtual auto get_total_drive_space() const -> std::uint64_t = 0;

  [[nodiscard]] virtual auto get_total_item_count() const -> std::uint64_t = 0;

  [[nodiscard]] virtual auto get_used_drive_space() const -> std::uint64_t = 0;

  virtual void get_volume_info(UINT64 &total_size, UINT64 &free_size,
                               std::string &volume_label) const = 0;

  [[nodiscard]] virtual auto is_processing(const std::string &api_path) const
      -> bool = 0;

  [[nodiscard]] virtual auto rename_directory(const std::string &from_api_path,
                                              const std::string &to_api_path)
      -> int = 0;

  [[nodiscard]] virtual auto rename_file(const std::string &from_api_path,
                                         const std::string &to_api_path,
                                         bool overwrite) -> int = 0;

  virtual void set_item_meta(const std::string &api_path,
                             const std::string &key,
                             const std::string &value) = 0;

  virtual void set_item_meta(const std::string &api_path,
                             const api_meta_map &meta) = 0;
};
} // namespace repertory

#endif // !defined(_WIN32)
#endif // REPERTORY_INCLUDE_DRIVES_FUSE_I_FUSE_DRIVE_HPP_
