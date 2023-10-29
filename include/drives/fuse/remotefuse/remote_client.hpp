/*
  Copyright <2018-2023> <scott.e.graves@protonmail.com>

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
#ifndef INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_CLIENT_HPP_
#define INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_CLIENT_HPP_

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
  [[nodiscard]] auto fuse_access(const char *path, const std::int32_t &mask)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_chflags(const char *path, std::uint32_t flags)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_chmod(const char *path, const remote::file_mode &mode)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_chown(const char *path, const remote::user_id &uid,
                                const remote::group_id &gid)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_destroy() -> packet::error_type override;

  [[nodiscard]] /*packet::error_type fuse_fallocate(const char *path, const
                   std::int32_t &mode, const remote::file_offset &offset, const
                   remote::file_offset &length, const remote::file_handle
                   &handle) override ;*/

  [[nodiscard]] auto
  fuse_fgetattr(const char *path, remote::stat &st, bool &directory,
                const remote::file_handle &handle)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_fsetattr_x(const char *path,
                                     const remote::setattr_x &attr,
                                     const remote::file_handle &handle)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_fsync(const char *path, const std::int32_t &datasync,
                                const remote::file_handle &handle)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_ftruncate(const char *path,
                                    const remote::file_offset &size,
                                    const remote::file_handle &handle)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_getattr(const char *path, remote::stat &st,
                                  bool &directory)
      -> packet::error_type override;

  /*[[nodiscard]] packet::error_type fuse_getxattr(const char *path, const char
  *name, char *value, const remote::file_size &size) override ;

  [[nodiscard]] packet::error_type fuse_getxattrOSX(const char *path, const char
  *name, char *value, const remote::file_size &size, std::uint32_t position)
  override ;*/

  [[nodiscard]] auto fuse_getxtimes(const char *path,
                                    remote::file_time &bkuptime,
                                    remote::file_time &crtime)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_init() -> packet::error_type override;

  /*[[nodiscard]] packet::error_type fuse_listxattr(const char *path, char
   *buffer, const remote::file_size &size) override ;*/

  [[nodiscard]] auto fuse_mkdir(const char *path, const remote::file_mode &mode)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_opendir(const char *path, remote::file_handle &handle)
      -> packet::error_type override;

  [[nodiscard]] auto
  fuse_create(const char *path, const remote::file_mode &mode,
              const remote::open_flags &flags, remote::file_handle &handle)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_open(const char *path,
                               const remote::open_flags &flags,
                               remote::file_handle &handle)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_read(const char *path, char *buffer,
                               const remote::file_size &read_size,
                               const remote::file_offset &read_offset,
                               const remote::file_handle &handle)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_rename(const char *from, const char *to)
      -> packet::error_type override;

  [[nodiscard]] auto
  fuse_readdir(const char *path, const remote::file_offset &offset,
               const remote::file_handle &handle, std::string &item_path)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_release(const char *path,
                                  const remote::file_handle &handle)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_releasedir(const char *path,
                                     const remote::file_handle &handle)
      -> packet::error_type override;

  /*[[nodiscard]] packet::error_type fuse_removexattr(const char *path, const
   * char *name) override
   * ;*/

  [[nodiscard]] auto fuse_rmdir(const char *path)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_setattr_x(const char *path, remote::setattr_x &attr)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_setbkuptime(const char *path,
                                      const remote::file_time &bkuptime)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_setchgtime(const char *path,
                                     const remote::file_time &chgtime)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_setcrtime(const char *path,
                                    const remote::file_time &crtime)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_setvolname(const char *volname)
      -> packet::error_type override;

  [[nodiscard]] /*packet::error_type fuse_setxattr(const char *path, const char
  *name, const char *value, const remote::file_size &size, const std::int32_t
  &flags) override ;

  [[nodiscard]] packet::error_type fuse_setxattr_osx(const char *path, const
  char *name, const char *value, const remote::file_size &size, const
  std::int32_t &flags, std::uint32_t position) override ;*/

  [[nodiscard]] auto
  fuse_statfs(const char *path, std::uint64_t frsize, remote::statfs &st)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_statfs_x(const char *path, std::uint64_t bsize,
                                   remote::statfs_x &st)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_truncate(const char *path,
                                   const remote::file_offset &size)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_unlink(const char *path)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_utimens(const char *path, const remote::file_time *tv,
                                  std::uint64_t op0, std::uint64_t op1)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_write(const char *path, const char *buffer,
                                const remote::file_size &write_size,
                                const remote::file_offset &write_offset,
                                const remote::file_handle &handle)
      -> packet::error_type override;

  [[nodiscard]] auto fuse_write_base64(const char *path, const char *buffer,
                                       const remote::file_size &write_size,
                                       const remote::file_offset &write_offset,
                                       const remote::file_handle &handle)
      -> packet::error_type override;

  [[nodiscard]] auto json_create_directory_snapshot(const std::string &path,
                                                    json &json_data)
      -> packet::error_type override;

  [[nodiscard]] auto json_read_directory_snapshot(
      const std::string &path, const remote::file_handle &handle,
      std::uint32_t page, json &json_data) -> packet::error_type override;

  [[nodiscard]] auto
  json_release_directory_snapshot(const std::string &path,
                                  const remote::file_handle &handle)
      -> packet::error_type override;

  void set_fuse_uid_gid(const remote::user_id &uid,
                        const remote::group_id &gid) override;
};
} // namespace remote_fuse
} // namespace repertory

#endif // INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_CLIENT_HPP_
