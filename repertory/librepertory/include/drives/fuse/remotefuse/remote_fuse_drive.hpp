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
#ifndef REPERTORY_INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_FUSE_DRIVE_HPP_
#define REPERTORY_INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_FUSE_DRIVE_HPP_
#if !defined(_WIN32)

#include "drives/fuse/fuse_base.hpp"
#include "drives/fuse/remotefuse/i_remote_instance.hpp"
#include "events/event_system.hpp"

namespace repertory {
class app_config;
class console_consumer;
class logging_consumer;
class lock_data;
class server;

namespace remote_fuse {
class remote_fuse_drive final : public fuse_base {
public:
  remote_fuse_drive(app_config &config, remote_instance_factory factory,
                    lock_data &lock)
      : fuse_base(config), factory_(std::move(factory)), lock_data_(lock) {}

  ~remote_fuse_drive() override = default;

private:
  remote_instance_factory factory_;
  lock_data &lock_data_;
  std::shared_ptr<console_consumer> console_consumer_;
  std::shared_ptr<logging_consumer> logging_consumer_;
  std::shared_ptr<i_remote_instance> remote_instance_;
  std::shared_ptr<server> server_;
  bool was_mounted_ = false;

private:
  void populate_stat(const remote::stat &r_stat, bool directory,
                     struct stat &unix_st);

protected:
  [[nodiscard]] auto access_impl(std::string api_path,
                                 int mask) -> api_error override;

#if defined(__APPLE__)
  [[nodiscard]] auto chflags_impl(std::string api_path,
                                  uint32_t flags) -> api_error override;
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] auto
  chmod_impl(std::string api_path, mode_t mode,
             struct fuse_file_info *f_info) -> api_error override;
#else
  [[nodiscard]] auto chmod_impl(std::string api_path,
                                mode_t mode) -> api_error override;
#endif

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] auto
  chown_impl(std::string api_path, uid_t uid, gid_t gid,
             struct fuse_file_info *f_info) -> api_error override;
#else
  [[nodiscard]] auto chown_impl(std::string api_path, uid_t uid,
                                gid_t gid) -> api_error override;
#endif

  [[nodiscard]] auto
  create_impl(std::string api_path, mode_t mode,
              struct fuse_file_info *f_info) -> api_error override;

  void destroy_impl(void * /*ptr*/) override;

  [[nodiscard]] auto
  fgetattr_impl(std::string api_path, struct stat *unix_st,
                struct fuse_file_info *f_info) -> api_error override;

#if defined(__APPLE__)
  [[nodiscard]] auto
  fsetattr_x_impl(std::string api_path, struct setattr_x *attr,
                  struct fuse_file_info *f_info) -> api_error override;
#endif // __APPLE__

  [[nodiscard]] auto
  fsync_impl(std::string api_path, int datasync,
             struct fuse_file_info *f_info) -> api_error override;

#if FUSE_USE_VERSION < 30
  [[nodiscard]] auto
  ftruncate_impl(std::string api_path, off_t size,
                 struct fuse_file_info *f_info) -> api_error override;
#endif

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] auto
  getattr_impl(std::string api_path, struct stat *unix_st,
               struct fuse_file_info *f_info) -> api_error override;
#else
  [[nodiscard]] auto getattr_impl(std::string api_path,
                                  struct stat *unix_st) -> api_error override;
#endif

#if defined(__APPLE__)
  [[nodiscard]] auto
  getxtimes_impl(std::string api_path, struct timespec *bkuptime,
                 struct timespec *crtime) -> api_error override;
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
  auto init_impl(struct fuse_conn_info *conn,
                 struct fuse_config *cfg) -> void * override;
#else
  auto init_impl(struct fuse_conn_info *conn) -> void * override;
#endif

  [[nodiscard]] auto mkdir_impl(std::string api_path,
                                mode_t mode) -> api_error override;

  void notify_fuse_main_exit(int &ret) override;

  [[nodiscard]] auto
  open_impl(std::string api_path,
            struct fuse_file_info *f_info) -> api_error override;

  [[nodiscard]] auto
  opendir_impl(std::string api_path,
               struct fuse_file_info *f_info) -> api_error override;

  [[nodiscard]] auto read_impl(std::string api_path, char *buffer,
                               size_t read_size, off_t read_offset,
                               struct fuse_file_info *f_info,
                               std::size_t &bytes_read) -> api_error override;

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] auto
  readdir_impl(std::string api_path, void *buf, fuse_fill_dir_t fuse_fill_dir,
               off_t offset, struct fuse_file_info *f_info,
               fuse_readdir_flags flags) -> api_error override;
#else
  [[nodiscard]] auto
  readdir_impl(std::string api_path, void *buf, fuse_fill_dir_t fuse_fill_dir,
               off_t offset,
               struct fuse_file_info *f_info) -> api_error override;
#endif

  [[nodiscard]] auto
  release_impl(std::string api_path,
               struct fuse_file_info *f_info) -> api_error override;

  [[nodiscard]] auto
  releasedir_impl(std::string api_path,
                  struct fuse_file_info *f_info) -> api_error override;

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] auto rename_impl(std::string from_api_path,
                                 std::string to_api_path,
                                 unsigned int flags) -> api_error override;
#else
  [[nodiscard]] auto rename_impl(std::string from_api_path,
                                 std::string to_api_path) -> api_error override;
#endif

  [[nodiscard]] auto rmdir_impl(std::string api_path) -> api_error override;

#if defined(__APPLE__)
  [[nodiscard]] auto
  setattr_x_impl(std::string api_path,
                 struct setattr_x *attr) -> api_error override;

  [[nodiscard]] auto
  setbkuptime_impl(std::string api_path,
                   const struct timespec *bkuptime) -> api_error override;

  [[nodiscard]] auto
  setchgtime_impl(std::string api_path,
                  const struct timespec *chgtime) -> api_error override;

  [[nodiscard]] auto
  setcrtime_impl(std::string api_path,
                 const struct timespec *crtime) -> api_error override;

  [[nodiscard]] virtual auto
  setvolname_impl(const char *volname) -> api_error override;

  [[nodiscard]] auto statfs_x_impl(std::string api_path,
                                   struct statfs *stbuf) -> api_error override;

#else  // __APPLE__
  [[nodiscard]] auto statfs_impl(std::string api_path,
                                 struct statvfs *stbuf) -> api_error override;
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] auto
  truncate_impl(std::string api_path, off_t size,
                struct fuse_file_info *f_info) -> api_error override;
#else
  [[nodiscard]] auto truncate_impl(std::string api_path,
                                   off_t size) -> api_error override;
#endif

  [[nodiscard]] auto unlink_impl(std::string api_path) -> api_error override;

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] auto
  utimens_impl(std::string api_path, const struct timespec tv[2],
               struct fuse_file_info *f_info) -> api_error override;
#else
  [[nodiscard]] auto
  utimens_impl(std::string api_path,
               const struct timespec tv[2]) -> api_error override;
#endif

  [[nodiscard]] auto
  write_impl(std::string api_path, const char *buffer, size_t write_size,
             off_t write_offset, struct fuse_file_info *f_info,
             std::size_t &bytes_written) -> api_error override;
};
} // namespace remote_fuse
} // namespace repertory

#endif // _WIN32
#endif // REPERTORY_INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_FUSE_DRIVE_HPP_
