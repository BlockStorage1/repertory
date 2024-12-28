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
#ifndef REPERTORY_TEST_INCLUDE_FIXTURES_MOCKS_MOCK_FUSE_DRIVE_HPP_
#define REPERTORY_TEST_INCLUDE_FIXTURES_MOCKS_MOCK_FUSE_DRIVE_HPP_
#if !defined(_WIN32)

#include "test_common.hpp"

#include "drives/fuse/i_fuse_drive.hpp"
#include "types/remote.hpp"
#include "types/repertory.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"
#include "utils/time.hpp"
#include "utils/utils.hpp"

namespace repertory {
class mock_fuse_drive final : public virtual i_fuse_drive {
public:
  explicit mock_fuse_drive(std::string mount_location)
      : mount_location_(std::move(mount_location)) {}

private:
  std::string mount_location_;
  std::unordered_map<std::string, api_meta_map> meta_;

public:
  auto check_owner(const std::string &) const -> api_error override {
    return api_error::success;
  }

  auto check_parent_access(const std::string &,
                           int) const -> api_error override {
    return api_error::success;
  }

  auto get_directory_item_count(const std::string &) const
      -> std::uint64_t override {
    return 1;
  }

  auto get_directory_items(const std::string &) const
      -> directory_item_list override {
    directory_item_list list{};

    directory_item dir_item{};
    dir_item.api_path = ".";
    dir_item.directory = true;
    dir_item.size = 0;
    dir_item.meta = {
        {META_ATTRIBUTES, "16"},
        {META_MODIFIED, std::to_string(utils::time::get_time_now())},
        {META_WRITTEN, std::to_string(utils::time::get_time_now())},
        {META_ACCESSED, std::to_string(utils::time::get_time_now())},
        {META_CREATION, std::to_string(utils::time::get_time_now())}};
    list.emplace_back(dir_item);

    dir_item.api_path = "..";
    list.emplace_back(dir_item);

    return list;
  }

  auto get_file_size(const std::string &) const -> std::uint64_t override {
    return 0U;
  }

  auto get_item_meta(const std::string &api_path,
                     api_meta_map &meta) const -> api_error override {
    meta = const_cast<mock_fuse_drive *>(this)->meta_[api_path];
    return api_error::success;
  }

  auto get_item_meta(const std::string &api_path, const std::string &name,
                     std::string &value) const -> api_error override {
    value = const_cast<mock_fuse_drive *>(this)->meta_[api_path][name];
    if (value.empty()) {
      value = "0";
    }
    return api_error::success;
  }

  auto get_total_drive_space() const -> std::uint64_t override {
    return 100ULL * 1024ULL * 1024ULL;
  }

  auto get_total_item_count() const -> std::uint64_t override { return 0U; }

  auto get_used_drive_space() const -> std::uint64_t override { return 0U; }

  void get_volume_info(UINT64 &total_size, UINT64 &free_size,
                       std::string &volume_label) const override {
    free_size = 100u;
    total_size = 200u;
    volume_label = "TestVolumeLabel";
  }

  auto rename_directory(const std::string &from_api_path,
                        const std::string &to_api_path) -> int override {
    const auto from_file_path =
        utils::path::combine(mount_location_, {from_api_path});
    const auto to_file_path =
        utils::path::combine(mount_location_, {to_api_path});
    return rename(from_file_path.c_str(), to_file_path.c_str());
  }

  auto rename_file(const std::string &from_api_path,
                   const std::string &to_api_path,
                   bool overwrite) -> int override {
    const auto from_file_path =
        utils::path::combine(mount_location_, {from_api_path});
    const auto to_file_path =
        utils::path::combine(mount_location_, {to_api_path});

    if (overwrite) {
      if (not utils::file::file(to_file_path).remove()) {
        return -1;
      }
    } else if (utils::file::directory(to_file_path).exists() ||
               utils::file::file(to_file_path).exists()) {
      errno = EEXIST;
      return -1;
    }

    return rename(from_file_path.c_str(), to_file_path.c_str());
  }

  auto is_processing(const std::string &) const -> bool override {
    return false;
  }

  void set_item_meta(const std::string &api_path, const std::string &key,
                     const std::string &value) override {
    meta_[api_path][key] = value;
  }
};
} // namespace repertory

#endif // !defined(_WIN32)
#endif // REPERTORY_TEST_INCLUDE_FIXTURES_MOCKS_MOCK_FUSE_DRIVE_HPP_
