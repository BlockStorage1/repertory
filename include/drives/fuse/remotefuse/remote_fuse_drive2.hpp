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
#ifndef INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_FUSE_DRIVE2_HPP_
#define INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_FUSE_DRIVE2_HPP_
#ifndef _WIN32
#if 0

#include "common.hpp"
#include "drives/fuse/fuse_base.hpp"
#include "drives/fuse/remotefuse/i_remote_instance.hpp"
#include "events/event_system.hpp"

namespace repertory {
class app_config;
class console_consumer;
class logging_consumer;
class lock_data;
class server;

namespace utils {
api_error to_api_error(packet::error_type e) { return api_error::success; }
} // namespace utils

namespace remote_fuse {
class remote_fuse_drive2 final : public fuse_base {
  E_CONSUMER();

public:
  ~remote_fuse_drive2() override = default;

private:
  std::unique_ptr<i_remote_instance> remote_instance_;

protected:
  api_error access_impl(std::string api_path, int mask) override;

#ifdef __APPLE__
  api_error chflags_impl(std::string api_path, uint32_t flags) override;
#endif // __APPLE__

  api_error chmod_impl(std::string api_path, mode_t mode) override;

public:
  api_error check_parent_access(const std::string &api_path, int mask) const override;

  std::uint64_t get_directory_item_count(const std::string &api_path) const override;

  directory_item_list get_directory_items(const std::string &api_path) const override;

  std::uint64_t get_file_size(const std::string &api_path) const override;

  api_error get_item_meta(const std::string &api_path, api_meta_map &meta) const override;

  api_error get_item_meta(const std::string &api_path, const std::string &name,
                          std::string &value) const override;

  std::uint64_t get_total_drive_space() const override;

  std::uint64_t get_total_item_count() const override;

  std::uint64_t get_used_drive_space() const override;

  void get_volume_info(UINT64 &total_size, UINT64 &free_size,
                       std::string &volume_label) const override;

  bool is_processing(const std::string &api_path) const override;

  void populate_stat(const directory_item &di, struct stat &st) const override;

  int rename_directory(const std::string &from_api_path, const std::string &to_api_path) override;

  int rename_file(const std::string &from_api_path, const std::string &to_api_path,
                  const bool &overwrite) override;

  void set_item_meta(const std::string &api_path, const std::string &key,
                     const std::string &value) override;

  void update_directory_item(directory_item &di) const override;
};
} // namespace remote_fuse
} // namespace repertory

#endif // 0
#endif // _WIN32
#endif // INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_FUSE_DRIVE2_HPP_
