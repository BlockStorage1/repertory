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
#ifndef REPERTORY_INCLUDE_DRIVES_FUSE_FUSE_DRIVE_HPP_
#define REPERTORY_INCLUDE_DRIVES_FUSE_FUSE_DRIVE_HPP_
#if !defined(_WIN32)

#include "drives/fuse/fuse_drive_base.hpp"
#include "file_manager/file_manager.hpp"

namespace repertory {
class app_config;
class console_consumer;
class directory_cache;
class eviction;
class full_server;
class i_provider;
class lock_data;
class logging_consumer;
namespace remote_fuse {
class remote_server;
}

class fuse_drive final : public fuse_drive_base {
public:
  fuse_drive(app_config &config, lock_data &lock_data, i_provider &provider);

  ~fuse_drive() override = default;

public:
  fuse_drive(const fuse_drive &) = delete;
  fuse_drive(fuse_drive &&) = delete;

  auto operator=(const fuse_drive &) -> fuse_drive & = delete;
  auto operator=(fuse_drive &&) -> fuse_drive & = delete;

private:
  lock_data &lock_data_;
  i_provider &provider_;

  std::shared_ptr<console_consumer> console_consumer_;
  std::shared_ptr<directory_cache> directory_cache_;
  std::shared_ptr<eviction> eviction_;
  std::shared_ptr<file_manager> fm_;
  std::shared_ptr<logging_consumer> logging_consumer_;
  std::shared_ptr<remote_fuse::remote_server> remote_server_;
  std::shared_ptr<full_server> server_;
  std::mutex stop_all_mtx_;
  bool was_mounted_{false};

private:
  void update_accessed_time(std::string_view api_path);

  void stop_all();

protected:
#if defined(__APPLE__)
  [[nodiscard]] auto chflags_impl(std::string api_path, uint32_t flags)
      -> api_error override;
#endif // defined(__APPLE__{}

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] auto chmod_impl(std::string api_path, mode_t mode,
                                struct fuse_file_info *f_info)
      -> api_error override;
#else  // FUSE_USE_VERSION < 30
  [[nodiscard]] auto chmod_impl(std::string api_path, mode_t mode)
      -> api_error override;
#endif // FUSE_USE_VERSION >= 30

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] auto chown_impl(std::string api_path, uid_t uid, gid_t gid,
                                struct fuse_file_info *f_info)
      -> api_error override;
#else  // FUSE_USE_VERSION < 30
  [[nodiscard]] auto chown_impl(std::string api_path, uid_t uid, gid_t gid)
      -> api_error override;
#endif // FUSE_USE_VERSION >= 30

  [[nodiscard]] auto create_impl(std::string api_path, mode_t mode,
                                 struct fuse_file_info *f_info)
      -> api_error override;

  void destroy_impl(void *ptr) override;

  [[nodiscard]] auto fallocate_impl(std::string api_path, int mode,
                                    off_t offset, off_t length,
                                    struct fuse_file_info *f_info)
      -> api_error override;

  [[nodiscard]] auto fgetattr_impl(std::string api_path, struct stat *u_stat,
                                   struct fuse_file_info *f_info)
      -> api_error override;

#if defined(__APPLE__)
  [[nodiscard]] auto fsetattr_x_impl(std::string api_path,
                                     struct setattr_x *attr,
                                     struct fuse_file_info *f_info)
      -> api_error override;
#endif // defined(__APPLE__)

  [[nodiscard]] auto fsync_impl(std::string api_path, int datasync,
                                struct fuse_file_info *f_info)
      -> api_error override;

#if FUSE_USE_VERSION < 30
  [[nodiscard]] auto ftruncate_impl(std::string api_path, off_t size,
                                    struct fuse_file_info *f_info)
      -> api_error override;
#endif // FUSE_USE_VERSION < 30

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] auto getattr_impl(std::string api_path, struct stat *u_stat,
                                  struct fuse_file_info *f_info)
      -> api_error override;
#else  // FUSE_USE_VERSION < 30
  [[nodiscard]] auto getattr_impl(std::string api_path, struct stat *u_stat)
      -> api_error override;
#endif // FUSE_USE_VERSION >= 30

#if defined(__APPLE__)
  [[nodiscard]] auto getxtimes_impl(std::string api_path,
                                    struct timespec *bkuptime,
                                    struct timespec *crtime)
      -> api_error override;
#endif // defined(__APPLE__)

#if FUSE_USE_VERSION >= 30
  auto init_impl(struct fuse_conn_info *conn, struct fuse_config *cfg)
      -> void * override;
#else  // FUSE_USE_VERSION < 30
  auto init_impl(struct fuse_conn_info *conn) -> void * override;
#endif // FUSE_USE_VERSION >= 30

  [[nodiscard]] auto ioctl_impl(std::string api_path, int cmd, void *arg,
                                struct fuse_file_info *f_info)
      -> api_error override;

  [[nodiscard]] auto mkdir_impl(std::string api_path, mode_t mode)
      -> api_error override;

  void notify_fuse_main_exit(int &ret) override;

  [[nodiscard]] auto open_impl(std::string api_path,
                               struct fuse_file_info *f_info)
      -> api_error override;

  [[nodiscard]] auto opendir_impl(std::string api_path,
                                  struct fuse_file_info *f_info)
      -> api_error override;

  [[nodiscard]] auto read_impl(std::string api_path, char *buffer,
                               size_t read_size, off_t read_offset,
                               struct fuse_file_info *f_info,
                               std::size_t &bytes_read) -> api_error override;

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] auto readdir_impl(std::string api_path, void *buf,
                                  fuse_fill_dir_t fuse_fill_dir, off_t offset,
                                  struct fuse_file_info *f_info,
                                  fuse_readdir_flags flags)
      -> api_error override;
#else  // FUSE_USE_VERSION < 30
  [[nodiscard]] auto readdir_impl(std::string api_path, void *buf,
                                  fuse_fill_dir_t fuse_fill_dir, off_t offset,
                                  struct fuse_file_info *f_info)
      -> api_error override;
#endif // FUSE_USE_VERSION >= 30

  [[nodiscard]] auto release_impl(std::string api_path,
                                  struct fuse_file_info *f_info)
      -> api_error override;

  [[nodiscard]] auto releasedir_impl(std::string api_path,
                                     struct fuse_file_info *f_info)
      -> api_error override;

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] auto rename_impl(std::string from_api_path,
                                 std::string to_api_path, unsigned int flags)
      -> api_error override;
#else  // FUSE_USE_VERSION < 30
  [[nodiscard]] auto rename_impl(std::string from_api_path,
                                 std::string to_api_path) -> api_error override;
#endif // FUSE_USE_VERSION >= 30

  [[nodiscard]] auto rmdir_impl(std::string api_path) -> api_error override;

#if defined(HAS_SETXATTR)
  [[nodiscard]] auto getxattr_common(std::string api_path, const char *name,
                                     char *value, size_t size,
                                     int &attribute_size, uint32_t *position)
      -> api_error;

#if defined(__APPLE__)
  [[nodiscard]] auto getxattr_impl(std::string api_path, const char *name,
                                   char *value, size_t size, uint32_t position,
                                   int &attribute_size) -> api_error override;
#else  // !defined(__APPLE__)
  [[nodiscard]] auto getxattr_impl(std::string api_path, const char *name,
                                   char *value, size_t size,
                                   int &attribute_size) -> api_error override;
#endif // defined(__APPLE__)

  [[nodiscard]] auto listxattr_impl(std::string api_path, char *buffer,
                                    size_t size, int &required_size,
                                    bool &return_size) -> api_error override;

  [[nodiscard]] auto removexattr_impl(std::string api_path, const char *name)
      -> api_error override;

#if defined(__APPLE__)
  [[nodiscard]] auto setxattr_impl(std::string api_path, const char *name,
                                   const char *value, size_t size, int flags,
                                   uint32_t position) -> api_error override;
#else  // !defined(__APPLE__)
  [[nodiscard]] auto setxattr_impl(std::string api_path, const char *name,
                                   const char *value, size_t size, int flags)
      -> api_error override;
#endif // defined(__APPLE__)
#endif // defined(HAS_SETXATTR{}

#if defined(__APPLE__)
  [[nodiscard]] auto setattr_x_impl(std::string api_path,
                                    struct setattr_x *attr)
      -> api_error override;

  [[nodiscard]] auto setbkuptime_impl(std::string api_path,
                                      const struct timespec *bkuptime)
      -> api_error override;

  [[nodiscard]] auto setchgtime_impl(std::string api_path,
                                     const struct timespec *chgtime)
      -> api_error override;

  [[nodiscard]] auto setcrtime_impl(std::string api_path,
                                    const struct timespec *crtime)
      -> api_error override;

  [[nodiscard]] auto setvolname_impl(const char *volname) -> api_error override;

  [[nodiscard]] auto statfs_x_impl(std::string api_path, struct statfs *stbuf)
      -> api_error override;
#else  // !defined(__APPLE__)
  [[nodiscard]] auto statfs_impl(std::string api_path, struct statvfs *stbuf)
      -> api_error override;
#endif // defined(__APPLE__)

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] auto truncate_impl(std::string api_path, off_t size,
                                   struct fuse_file_info *f_info)
      -> api_error override;
#else  // FUSE_USE_VERSION < 30
  [[nodiscard]] auto truncate_impl(std::string api_path, off_t size)
      -> api_error override;
#endif // FUSE_USE_VERSION >= 30

  [[nodiscard]] auto unlink_impl(std::string api_path) -> api_error override;

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] auto utimens_impl(std::string api_path,
                                  const struct timespec tv[2],
                                  struct fuse_file_info *f_info)
      -> api_error override;
#else  // FUSE_USE_VERSION < 30
  [[nodiscard]] auto utimens_impl(std::string api_path,
                                  const struct timespec tv[2])
      -> api_error override;
#endif // FUSE_USE_VERSION >= 30

  [[nodiscard]] auto write_impl(std::string api_path, const char *buffer,
                                size_t write_size, off_t write_offset,
                                struct fuse_file_info *f_info,
                                std::size_t &bytes_written)
      -> api_error override;

public:
  [[nodiscard]] auto get_directory_item_count(std::string_view api_path) const
      -> std::uint64_t override;

  [[nodiscard]] auto get_directory_items(std::string_view api_path) const
      -> directory_item_list override;

  [[nodiscard]] auto get_file_size(std::string_view api_path) const
      -> std::uint64_t override;

  [[nodiscard]] auto get_item_meta(std::string_view api_path,
                                   api_meta_map &meta) const
      -> api_error override;

  [[nodiscard]] auto get_item_meta(std::string_view api_path,
                                   std::string_view name,
                                   std::string &value) const
      -> api_error override;

  [[nodiscard]] auto get_item_stat(std::uint64_t handle,
                                   struct stat64 *u_stat) const
      -> api_error override;

  [[nodiscard]] auto get_total_drive_space() const -> std::uint64_t override;

  [[nodiscard]] auto get_total_item_count() const -> std::uint64_t override;

  [[nodiscard]] auto get_used_drive_space() const -> std::uint64_t override;

  void get_volume_info(UINT64 &total_size, UINT64 &free_size,
                       std::string &volume_label) const override;

  [[nodiscard]] auto is_processing(std::string_view api_path) const
      -> bool override;

  [[nodiscard]] auto rename_directory(std::string_view from_api_path,
                                      std::string_view to_api_path)
      -> int override;

  [[nodiscard]] auto rename_file(std::string_view from_api_path,
                                 std::string_view to_api_path, bool overwrite)
      -> int override;

  void set_item_meta(std::string_view api_path, std::string_view key,
                     std::string_view value) override;

  void set_item_meta(std::string_view api_path,
                     const api_meta_map &meta) override;
};
} // namespace repertory

#endif // !defined(_WIN32)
#endif // REPERTORY_INCLUDE_DRIVES_FUSE_FUSE_DRIVE_HPP_
