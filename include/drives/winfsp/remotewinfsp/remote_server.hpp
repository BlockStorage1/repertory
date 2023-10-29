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
// NOTE: Most of the WinFSP pass-through code has been modified from:
// https://github.com/billziss-gh/winfsp/blob/master/tst/passthrough-cpp/passthrough-cpp.cpp
#ifndef INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_REMOTE_SERVER_HPP_
#define INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_REMOTE_SERVER_HPP_
#ifdef _WIN32

#include "comm/packet/packet.hpp"
#include "drives/remote/remote_server_base.hpp"
#include "drives/winfsp/i_winfsp_drive.hpp"

namespace repertory {
class app_config;
namespace remote_winfsp {
class remote_server final : public virtual remote_server_base<i_winfsp_drive> {
public:
  remote_server(app_config &config, i_winfsp_drive &drive,
                const std::string &mount_location)
      : remote_server_base(config, drive,
                           utils::string::to_lower(mount_location)) {}

private:
  [[nodiscard]] auto construct_path(std::string path) -> std::string;

  [[nodiscard]] auto populate_file_info(const std::string &api_path,
                                        remote::file_info &file_info)
      -> packet::error_type;

  void populate_stat(const char *path, bool directory, remote::stat &st,
                     const struct _stat64 &st1);

public:
  // FUSE Layer
  auto fuse_access(const char *path, const std::int32_t &mask)
      -> packet::error_type override;

  auto fuse_chflags(const char *path, std::uint32_t flags)
      -> packet::error_type override;

  auto fuse_chmod(const char *path, const remote::file_mode &mode)
      -> packet::error_type override;

  auto fuse_chown(const char *path, const remote::user_id &uid,
                  const remote::group_id &gid) -> packet::error_type override;

  auto fuse_destroy() -> packet::error_type override;

  /*packet::error_type fuse_fallocate(const char *path, const std::int32_t
     &mode, const remote::file_offset &offset, const remote::file_offset
     &length, const remote::file_handle &handle) override ;*/

  auto fuse_fgetattr(const char *path, remote::stat &st, bool &directory,
                     const remote::file_handle &handle)
      -> packet::error_type override;

  auto fuse_fsetattr_x(const char *path, const remote::setattr_x &attr,
                       const remote::file_handle &handle)
      -> packet::error_type override;

  auto fuse_fsync(const char *path, const std::int32_t &datasync,
                  const remote::file_handle &handle)
      -> packet::error_type override;

  auto fuse_ftruncate(const char *path, const remote::file_offset &size,
                      const remote::file_handle &handle)
      -> packet::error_type override;

  auto fuse_getattr(const char *path, remote::stat &st, bool &directory)
      -> packet::error_type override;

  /*packet::error_type fuse_getxattr(const char *path, const char *name, char
  *value, const remote::file_size &size) override ;

  packet::error_type fuse_getxattrOSX(const char *path, const char *name, char
  *value, const remote::file_size &size, std::uint32_t position) override ;*/

  auto fuse_getxtimes(const char *path, remote::file_time &bkuptime,
                      remote::file_time &crtime) -> packet::error_type override;

  auto fuse_init() -> packet::error_type override;

  /*packet::error_type fuse_listxattr(const char *path, char *buffer,
                                         const remote::file_size &size) override
     ;*/

  auto fuse_mkdir(const char *path, const remote::file_mode &mode)
      -> packet::error_type override;

  auto fuse_opendir(const char *path, remote::file_handle &handle)
      -> packet::error_type override;

  auto fuse_create(const char *path, const remote::file_mode &mode,
                   const remote::open_flags &flags, remote::file_handle &handle)
      -> packet::error_type override;

  auto fuse_open(const char *path, const remote::open_flags &flags,
                 remote::file_handle &handle) -> packet::error_type override;

  auto fuse_read(const char *path, char *buffer,
                 const remote::file_size &read_size,
                 const remote::file_offset &read_offset,
                 const remote::file_handle &handle)
      -> packet::error_type override;

  auto fuse_rename(const char *from, const char *to)
      -> packet::error_type override;

  auto fuse_write(const char *path, const char *buffer,
                  const remote::file_size &write_size,
                  const remote::file_offset &write_offset,
                  const remote::file_handle &handle)
      -> packet::error_type override;

  auto fuse_write_base64(const char *path, const char *buffer,
                         const remote::file_size &write_size,
                         const remote::file_offset &write_offset,
                         const remote::file_handle &handle)
      -> packet::error_type override;

  auto fuse_readdir(const char *path, const remote::file_offset &offset,
                    const remote::file_handle &handle, std::string &item_path)
      -> packet::error_type override;

  auto fuse_release(const char *path, const remote::file_handle &handle)
      -> packet::error_type override;

  auto fuse_releasedir(const char *path, const remote::file_handle &handle)
      -> packet::error_type override;

  /*packet::error_type fuse_removexattr(const char *path, const char *name)
   * override ;*/

  auto fuse_rmdir(const char *path) -> packet::error_type override;

  auto fuse_setattr_x(const char *path, remote::setattr_x &attr)
      -> packet::error_type override;

  auto fuse_setbkuptime(const char *path, const remote::file_time &bkuptime)
      -> packet::error_type override;

  auto fuse_setchgtime(const char *path, const remote::file_time &chgtime)
      -> packet::error_type override;

  auto fuse_setcrtime(const char *path, const remote::file_time &crtime)
      -> packet::error_type override;

  auto fuse_setvolname(const char *volname) -> packet::error_type override;

  /*packet::error_type fuse_setxattr(const char *path, const char *name, const
  char *value, const remote::file_size &size, const std::int32_t &flags)
  override ;

  packet::error_type fuse_setxattr_osx(const char *path, const char *name, const
  char *value, const remote::file_size &size, const std::int32_t &flags,
                                           std::uint32_t position) override ;*/

  auto fuse_statfs(const char *path, std::uint64_t frsize, remote::statfs &st)
      -> packet::error_type override;

  auto fuse_statfs_x(const char *path, std::uint64_t bsize,
                     remote::statfs_x &st) -> packet::error_type override;

  auto fuse_truncate(const char *path, const remote::file_offset &size)
      -> packet::error_type override;

  auto fuse_unlink(const char *path) -> packet::error_type override;

  auto fuse_utimens(const char *path, const remote::file_time *tv,
                    std::uint64_t op0, std::uint64_t op1)
      -> packet::error_type override;

  void set_fuse_uid_gid(const remote::user_id &,
                        const remote::group_id &) override {}

  // JSON Layer
  auto json_create_directory_snapshot(const std::string &path, json &json_data)
      -> packet::error_type override;

  auto json_read_directory_snapshot(const std::string &path,
                                    const remote::file_handle &handle,
                                    std::uint32_t page, json &json_data)
      -> packet::error_type override;

  auto json_release_directory_snapshot(const std::string &path,
                                       const remote::file_handle &handle)
      -> packet::error_type override;

  // WinFSP Layer
  auto winfsp_can_delete(PVOID file_desc, PWSTR file_name)
      -> packet::error_type override;

  auto winfsp_cleanup(PVOID file_desc, PWSTR file_name, UINT32 flags,
                      BOOLEAN &was_closed) -> packet::error_type override;

  auto winfsp_close(PVOID file_desc) -> packet::error_type override;

  auto winfsp_create(PWSTR file_name, UINT32 create_options,
                     UINT32 granted_access, UINT32 attributes,
                     UINT64 allocation_size, PVOID *file_desc,
                     remote::file_info *file_info, std::string &normalized_name,
                     BOOLEAN &exists) -> packet::error_type override;

  auto winfsp_flush(PVOID file_desc, remote::file_info *file_info)
      -> packet::error_type override;

  auto winfsp_get_dir_buffer(PVOID file_desc, PVOID *&ptr)
      -> packet::error_type override;

  auto winfsp_get_file_info(PVOID file_desc, remote::file_info *file_info)
      -> packet::error_type override;

  auto winfsp_get_security_by_name(PWSTR file_name, PUINT32 attributes,
                                   std::uint64_t *descriptor_size,
                                   std::wstring &string_descriptor)
      -> packet::error_type override;

  auto winfsp_get_volume_info(UINT64 &total_size, UINT64 &free_size,
                              std::string &volume_label)
      -> packet::error_type override;

  auto winfsp_mounted(const std::wstring &location)
      -> packet::error_type override;

  auto winfsp_open(PWSTR file_name, UINT32 create_options,
                   UINT32 granted_access, PVOID *file_desc,
                   remote::file_info *file_info, std::string &normalized_name)
      -> packet::error_type override;

  auto winfsp_overwrite(PVOID file_desc, UINT32 attributes,
                        BOOLEAN replace_attributes, UINT64 allocation_size,
                        remote::file_info *file_info)
      -> packet::error_type override;

  auto winfsp_read(PVOID file_desc, PVOID buffer, UINT64 offset, UINT32 length,
                   PUINT32 bytes_transferred) -> packet::error_type override;

  auto winfsp_read_directory(PVOID file_desc, PWSTR pattern, PWSTR marker,
                             json &item_list) -> packet::error_type override;

  auto winfsp_rename(PVOID file_desc, PWSTR file_name, PWSTR new_file_name,
                     BOOLEAN replace_if_exists) -> packet::error_type override;

  auto winfsp_set_basic_info(PVOID file_desc, UINT32 attributes,
                             UINT64 creation_time, UINT64 last_access_time,
                             UINT64 last_write_time, UINT64 change_time,
                             remote::file_info *file_info)
      -> packet::error_type override;

  auto winfsp_set_file_size(PVOID file_desc, UINT64 newSize,
                            BOOLEAN set_allocation_size,
                            remote::file_info *file_info)
      -> packet::error_type override;

  auto winfsp_unmounted(const std::wstring &location)
      -> packet::error_type override;

  auto winfsp_write(PVOID file_desc, PVOID buffer, UINT64 offset, UINT32 length,
                    BOOLEAN write_to_end, BOOLEAN constrained_io,
                    PUINT32 bytes_transferred, remote::file_info *file_info)
      -> packet::error_type override;
};
} // namespace remote_winfsp
} // namespace repertory

#endif // _WIN32
#endif // INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_REMOTE_SERVER_HPP_
