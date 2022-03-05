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
#ifndef INCLUDE_DRIVES_FUSE_I_FUSE_DRIVE_HPP_
#define INCLUDE_DRIVES_FUSE_I_FUSE_DRIVE_HPP_
#ifndef _WIN32

#include "common.hpp"
#include "types/repertory.hpp"

namespace repertory {
class i_fuse_drive {
  INTERFACE_SETUP(i_fuse_drive);

public:
  virtual api_error check_parent_access(const std::string &api_path, int mask) const = 0;

  virtual std::uint64_t get_directory_item_count(const std::string &api_path) const = 0;

  virtual directory_item_list get_directory_items(const std::string &api_path) const = 0;

  virtual std::uint64_t get_file_size(const std::string &api_path) const = 0;

  virtual api_error get_item_meta(const std::string &api_path, api_meta_map &meta) const = 0;

  virtual api_error get_item_meta(const std::string &api_path, const std::string &name,
                                  std::string &value) const = 0;

  virtual std::uint64_t get_total_drive_space() const = 0;

  virtual std::uint64_t get_total_item_count() const = 0;

  virtual std::uint64_t get_used_drive_space() const = 0;

  virtual void get_volume_info(UINT64 &total_size, UINT64 &free_size,
                               std::string &volume_label) const = 0;

  virtual bool is_processing(const std::string &api_path) const = 0;

  virtual void populate_stat(const directory_item &di, struct stat &st) const = 0;

  virtual int rename_directory(const std::string &from_api_path,
                               const std::string &to_api_path) = 0;

  virtual int rename_file(const std::string &from_api_path, const std::string &to_api_path,
                          const bool &overwrite) = 0;

  virtual void set_item_meta(const std::string &api_path, const std::string &key,
                             const std::string &value) = 0;

  virtual void update_directory_item(directory_item &di) const = 0;
};
} // namespace repertory

#endif
#endif // INCLUDE_DRIVES_FUSE_I_FUSE_DRIVE_HPP_
