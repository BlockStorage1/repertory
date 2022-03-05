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
#ifndef INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_CLIENT_HPP_
#define INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_CLIENT_HPP_

#include "common.hpp"
#include "comm/packet/packet_client.hpp"
#include "drives/fuse/remotefuse/i_remote_instance.hpp"
#include "events/event_system.hpp"
#include "types/remote.hpp"

namespace repertory {
class app_config;

namespace remote_fuse {
class remote_client final : public i_remote_instance {
public:
  explicit remote_client(const app_config &config);

private:
  const app_config &config_;
  packet_client packet_client_;
  remote::user_id uid_ = 0;
  remote::group_id gid_ = 0;

public:
  packet::error_type fuse_access(const char *path, const std::int32_t &mask) override;

  packet::error_type fuse_chflags(const char *path, const std::uint32_t &flags) override;

  packet::error_type fuse_chmod(const char *path, const remote::file_mode &mode) override;

  packet::error_type fuse_chown(const char *path, const remote::user_id &uid,
                                const remote::group_id &gid) override;

  packet::error_type fuse_destroy() override;

  /*packet::error_type fuse_fallocate(const char *path, const std::int32_t &mode,
                                         const remote::file_offset &offset,
                                         const remote::file_offset &length,
                                         const remote::file_handle &handle) override ;*/

  packet::error_type fuse_fgetattr(const char *path, remote::stat &st, bool &directory,
                                   const remote::file_handle &handle) override;

  packet::error_type fuse_fsetattr_x(const char *path, const remote::setattr_x &attr,
                                     const remote::file_handle &handle) override;

  packet::error_type fuse_fsync(const char *path, const std::int32_t &datasync,
                                const remote::file_handle &handle) override;

  packet::error_type fuse_ftruncate(const char *path, const remote::file_offset &size,
                                    const remote::file_handle &handle) override;

  packet::error_type fuse_getattr(const char *path, remote::stat &st, bool &directory) override;

  /*packet::error_type fuse_getxattr(const char *path, const char *name, char *value,
                                        const remote::file_size &size) override ;

  packet::error_type fuse_getxattrOSX(const char *path, const char *name, char *value,
                                           const remote::file_size &size,
                                           const std::uint32_t &position) override ;*/

  packet::error_type fuse_getxtimes(const char *path, remote::file_time &bkuptime,
                                    remote::file_time &crtime) override;

  packet::error_type fuse_init() override;

  /*packet::error_type fuse_listxattr(const char *path, char *buffer,
                                         const remote::file_size &size) override ;*/

  packet::error_type fuse_mkdir(const char *path, const remote::file_mode &mode) override;

  packet::error_type fuse_opendir(const char *path, remote::file_handle &handle) override;

  packet::error_type fuse_create(const char *path, const remote::file_mode &mode,
                                 const remote::open_flags &flags, remote::file_handle &handle) override;

  packet::error_type fuse_open(const char *path, const remote::open_flags &flags,
                               remote::file_handle &handle) override;

  packet::error_type fuse_read(const char *path, char *buffer, const remote::file_size &read_size,
                               const remote::file_offset &read_offset,
                               const remote::file_handle &handle) override;

  packet::error_type fuse_rename(const char *from, const char *to) override;

  packet::error_type fuse_readdir(const char *path, const remote::file_offset &offset,
                                  const remote::file_handle &handle, std::string &item_path) override;

  packet::error_type fuse_release(const char *path, const remote::file_handle &handle) override;

  packet::error_type fuse_releasedir(const char *path, const remote::file_handle &handle) override;

  /*packet::error_type fuse_removexattr(const char *path, const char *name) override ;*/

  packet::error_type fuse_rmdir(const char *path) override;

  packet::error_type fuse_setattr_x(const char *path, remote::setattr_x &attr) override;

  packet::error_type fuse_setbkuptime(const char *path, const remote::file_time &bkuptime) override;

  packet::error_type fuse_setchgtime(const char *path, const remote::file_time &chgtime) override;

  packet::error_type fuse_setcrtime(const char *path, const remote::file_time &crtime) override;

  packet::error_type fuse_setvolname(const char *volname) override;

  /*packet::error_type fuse_setxattr(const char *path, const char *name, const char *value,
                                        const remote::file_size &size,
                                        const std::int32_t &flags) override ;

  packet::error_type fuse_setxattr_osx(const char *path, const char *name, const char *value,
                                           const remote::file_size &size, const std::int32_t &flags,
                                           const std::uint32_t &position) override ;*/

  packet::error_type fuse_statfs(const char *path, const std::uint64_t &frsize,
                                 remote::statfs &st) override;

  packet::error_type fuse_statfs_x(const char *path, const std::uint64_t &bsize,
                                   remote::statfs_x &st) override;

  packet::error_type fuse_truncate(const char *path, const remote::file_offset &size) override;

  packet::error_type fuse_unlink(const char *path) override;

  packet::error_type fuse_utimens(const char *path, const remote::file_time *tv,
                                  const std::uint64_t &op0, const std::uint64_t &op1) override;

  packet::error_type fuse_write(const char *path, const char *buffer,
                                const remote::file_size &write_size,
                                const remote::file_offset &write_offset,
                                const remote::file_handle &handle) override;

  packet::error_type fuse_write_base64(const char *path, const char *buffer,
                                       const remote::file_size &write_size,
                                       const remote::file_offset &write_offset,
                                       const remote::file_handle &handle) override;

  packet::error_type json_create_directory_snapshot(const std::string &path,
                                                       json &json_data) override;

  packet::error_type json_read_directory_snapshot(const std::string &path,
                                                     const remote::file_handle &handle,
                                                     const std::uint32_t &page,
                                                     json &json_data) override;

  packet::error_type json_release_directory_snapshot(const std::string &path,
                                                        const remote::file_handle &handle) override;

  void set_fuse_uid_gid(const remote::user_id &uid, const remote::group_id &gid) override;
};
} // namespace remote_fuse
} // namespace repertory

#endif // INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_CLIENT_HPP_
