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
#ifndef INCLUDE_DRIVES_FUSE_FUSE_DRIVE_HPP_
#define INCLUDE_DRIVES_FUSE_FUSE_DRIVE_HPP_
#ifndef _WIN32

#include "common.hpp"
#include "drives/fuse/fuse_base.hpp"
#include "drives/open_file_table.hpp"

namespace repertory {
class i_provider;
class app_config;
class console_consumer;
class directory_cache;
class download_manager;
class eviction;
class full_server;
class lock_data;
class logging_consumer;
namespace remote_fuse {
class remote_server;
}

class fuse_drive final : public fuse_base {
public:
  fuse_drive(app_config &config, lock_data &lock_data, i_provider &provider);

  ~fuse_drive() override = default;

private:
  lock_data &lock_data_;
  i_provider &provider_;

  std::shared_ptr<console_consumer> console_consumer_;
  std::shared_ptr<directory_cache> directory_cache_;
  std::shared_ptr<download_manager> download_manager_;
  std::shared_ptr<eviction> eviction_;
  std::shared_ptr<logging_consumer> logging_consumer_;
  std::shared_ptr<open_file_table<open_file_data>> oft_;
  std::shared_ptr<remote_fuse::remote_server> remote_server_;
  std::shared_ptr<full_server> server_;
  bool was_mounted_ = false;

protected:
#ifdef __APPLE__
  api_error chflags_impl(std::string api_path, uint32_t flags) override;
#endif // __APPLE__

  api_error chmod_impl(std::string api_path, mode_t mode) override;

  api_error chown_impl(std::string api_path, uid_t uid, gid_t gid) override;

  api_error create_impl(std::string api_path, mode_t mode, struct fuse_file_info *fi) override;

  void destroy_impl(void *ptr) override;

  api_error fallocate_impl(std::string api_path, int mode, off_t offset, off_t length,
                           struct fuse_file_info *fi) override;

  api_error fgetattr_impl(std::string api_path, struct stat *st,
                          struct fuse_file_info *fi) override;

#ifdef __APPLE__
  api_error fsetattr_x_impl(std::string api_path, struct setattr_x *attr,
                            struct fuse_file_info *fi) override;
#endif // __APPLE__

  api_error fsync_impl(std::string api_path, int datasync, struct fuse_file_info *fi) override;

  api_error ftruncate_impl(std::string api_path, off_t size, struct fuse_file_info *fi) override;

  api_error getattr_impl(std::string api_path, struct stat *st) override;

#ifdef __APPLE__
  api_error getxtimes_impl(std::string api_path, struct timespec *bkuptime,
                           struct timespec *crtime) override;
#endif // __APPLE__

  void *init_impl(struct fuse_conn_info *conn) override;

  api_error mkdir_impl(std::string api_path, mode_t mode) override;

  api_error open_impl(std::string api_path, struct fuse_file_info *fi) override;

  api_error opendir_impl(std::string api_path, struct fuse_file_info *fi) override;

  api_error read_impl(std::string api_path, char *buffer, size_t read_size, off_t read_offset,
                      struct fuse_file_info *fi, std::size_t &bytes_read) override;

  api_error readdir_impl(std::string api_path, void *buf, fuse_fill_dir_t fuse_fill_dir,
                         off_t offset, struct fuse_file_info *fi) override;

  api_error release_impl(std::string api_path, struct fuse_file_info *fi) override;

  api_error releasedir_impl(std::string api_path, struct fuse_file_info *fi) override;

  api_error rename_impl(std::string from_api_path, std::string to_api_path) override;

  api_error rmdir_impl(std::string api_path) override;

#ifdef HAS_SETXATTR
  api_error getxattr_common(std::string api_path, const char *name, char *value, size_t size,
                            int &attribute_size, uint32_t *position);

#ifdef __APPLE__
  api_error getxattr_impl(std::string api_path, const char *name, char *value, size_t size,
                          uint32_t position, int &attribute_size) override;
#else  // __APPLE__
  api_error getxattr_impl(std::string api_path, const char *name, char *value, size_t size,
                          int &attribute_size) override;
#endif // __APPLE__

  api_error listxattr_impl(std::string api_path, char *buffer, size_t size, int &required_size,
                           bool &return_size) override;

  api_error removexattr_impl(std::string api_path, const char *name) override;

#ifdef __APPLE__
  api_error setxattr_impl(std::string api_path, const char *name, const char *value, size_t size,
                          int flags, uint32_t position) override;
#else  // __APPLE__
  api_error setxattr_impl(std::string api_path, const char *name, const char *value, size_t size,
                          int flags) override;
#endif // __APPLE__
#endif // HAS_SETXATTR

#ifdef __APPLE__
  api_error setattr_x_impl(std::string api_path, struct setattr_x *attr) override;

  api_error setbkuptime_impl(std::string api_path, const struct timespec *bkuptime) override;

  api_error setchgtime_impl(std::string api_path, const struct timespec *chgtime) override;

  api_error setcrtime_impl(std::string api_path, const struct timespec *crtime) override;

  api_error setvolname_impl(const char *volname) override;

  api_error statfs_x_impl(std::string api_path, struct statfs *stbuf) override;
#else  // __APPLE__
  api_error statfs_impl(std::string api_path, struct statvfs *stbuf) override;
#endif // __APPLE__

  api_error truncate_impl(std::string api_path, off_t size) override;

  api_error unlink_impl(std::string api_path) override;

  api_error utimens_impl(std::string api_path, const struct timespec tv[2]) override;

  api_error write_impl(std::string api_path, const char *buffer, size_t write_size,
                       off_t write_offset, struct fuse_file_info *fi,
                       std::size_t &bytes_written) override;

protected:
  void notify_fuse_args_parsed(const std::vector<std::string> &args) override;

  void notify_fuse_main_exit(int &ret) override;

  int shutdown() override;

  void update_accessed_time(const std::string &api_path);

public:
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
} // namespace repertory

#endif // _WIN32
#endif // INCLUDE_DRIVES_FUSE_FUSE_DRIVE_HPP_
