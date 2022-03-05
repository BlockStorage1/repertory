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
// NOTE: Most of the WinFSP pass-through code has been modified from:
// https://github.com/billziss-gh/winfsp/blob/master/tst/passthrough-cpp/passthrough-cpp.cpp
#ifndef INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_REMOTE_SERVER_HPP_
#define INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_REMOTE_SERVER_HPP_
#ifdef _WIN32

#include "common.hpp"
#include "comm/packet/packet.hpp"
#include "drives/remote/remote_server_base.hpp"
#include "drives/winfsp/i_winfsp_drive.hpp"

namespace repertory {
class app_config;
namespace remote_winfsp {
class remote_server final : public virtual remote_server_base<i_winfsp_drive> {
public:
  remote_server(app_config &config, i_winfsp_drive &drive, const std::string &mount_location)
      : remote_server_base(config, drive, mount_location) {}

private:
  std::string construct_path(std::string path);

  packet::error_type populate_file_info(const std::string &api_path, remote::file_info &file_info);

  void populate_stat(const char *path, const bool &directory, remote::stat &st,
                     const struct _stat64 &st1);

public:
  // FUSE Layer
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
                                 const remote::open_flags &flags,
                                 remote::file_handle &handle) override;

  packet::error_type fuse_open(const char *path, const remote::open_flags &flags,
                               remote::file_handle &handle) override;

  packet::error_type fuse_read(const char *path, char *buffer, const remote::file_size &read_size,
                               const remote::file_offset &read_offset,
                               const remote::file_handle &handle) override;

  packet::error_type fuse_rename(const char *from, const char *to) override;

  packet::error_type fuse_write(const char *path, const char *buffer,
                                const remote::file_size &write_size,
                                const remote::file_offset &write_offset,
                                const remote::file_handle &handle) override;

  packet::error_type fuse_write_base64(const char *path, const char *buffer,
                                       const remote::file_size &write_size,
                                       const remote::file_offset &write_offset,
                                       const remote::file_handle &handle) override;

  packet::error_type fuse_readdir(const char *path, const remote::file_offset &offset,
                                  const remote::file_handle &handle,
                                  std::string &item_path) override;

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

  void set_fuse_uid_gid(const remote::user_id &, const remote::group_id &) override {}

  // JSON Layer
  packet::error_type json_create_directory_snapshot(const std::string &path,
                                                    json &json_data) override;

  packet::error_type json_read_directory_snapshot(const std::string &path,
                                                  const remote::file_handle &handle,
                                                  const std::uint32_t &page,
                                                  json &json_data) override;

  packet::error_type json_release_directory_snapshot(const std::string &path,
                                                     const remote::file_handle &handle) override;

  // WinFSP Layer
  packet::error_type winfsp_can_delete(PVOID file_desc, PWSTR file_name) override;

  packet::error_type winfsp_cleanup(PVOID file_desc, PWSTR file_name, UINT32 flags,
                                    BOOLEAN &was_closed) override;

  packet::error_type winfsp_close(PVOID file_desc) override;

  packet::error_type winfsp_create(PWSTR file_name, UINT32 create_options, UINT32 granted_access,
                                   UINT32 attributes, UINT64 allocation_size, PVOID *file_desc,
                                   remote::file_info *file_info, std::string &normalized_name,
                                   BOOLEAN &exists) override;

  packet::error_type winfsp_flush(PVOID file_desc, remote::file_info *file_info) override;

  packet::error_type winfsp_get_dir_buffer(PVOID file_desc, PVOID *&ptr) override;

  packet::error_type winfsp_get_file_info(PVOID file_desc, remote::file_info *file_info) override;

  packet::error_type winfsp_get_security_by_name(PWSTR file_name, PUINT32 attributes,
                                                 std::uint64_t *descriptor_size,
                                                 std::wstring &string_descriptor) override;

  packet::error_type winfsp_get_volume_info(UINT64 &total_size, UINT64 &free_size,
                                            std::string &volume_label) override;

  packet::error_type winfsp_mounted(const std::wstring &location) override;

  packet::error_type winfsp_open(PWSTR file_name, UINT32 create_options, UINT32 granted_access,
                                 PVOID *file_desc, remote::file_info *file_info,
                                 std::string &normalized_name) override;

  packet::error_type winfsp_overwrite(PVOID file_desc, UINT32 attributes,
                                      BOOLEAN replace_attributes, UINT64 allocation_size,
                                      remote::file_info *file_info) override;

  packet::error_type winfsp_read(PVOID file_desc, PVOID buffer, UINT64 offset, UINT32 length,
                                 PUINT32 bytes_transferred) override;

  packet::error_type winfsp_read_directory(PVOID file_desc, PWSTR pattern, PWSTR marker,
                                           json &item_list) override;

  packet::error_type winfsp_rename(PVOID file_desc, PWSTR file_name, PWSTR new_file_name,
                                   BOOLEAN replace_if_exists) override;

  packet::error_type winfsp_set_basic_info(PVOID file_desc, UINT32 attributes, UINT64 creation_time,
                                           UINT64 last_access_time, UINT64 last_write_time,
                                           UINT64 change_time,
                                           remote::file_info *file_info) override;

  packet::error_type winfsp_set_file_size(PVOID file_desc, UINT64 newSize,
                                          BOOLEAN set_allocation_size,
                                          remote::file_info *file_info) override;

  packet::error_type winfsp_unmounted(const std::wstring &location) override;

  packet::error_type winfsp_write(PVOID file_desc, PVOID buffer, UINT64 offset, UINT32 length,
                                  BOOLEAN write_to_end, BOOLEAN constrained_io,
                                  PUINT32 bytes_transferred, remote::file_info *file_info) override;
};
} // namespace remote_winfsp
} // namespace repertory

#endif // _WIN32
#endif // INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_REMOTE_SERVER_HPP_
