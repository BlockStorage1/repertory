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
#if !defined(_WIN32)

#include "drives/fuse/remotefuse/remote_server.hpp"

#include "app_config.hpp"
#include "comm/packet/packet.hpp"
#include "drives/directory_iterator.hpp"
#include "drives/remote/remote_open_file_table.hpp"
#include "events/event_system.hpp"
#include "events/types/remote_server_event.hpp"
#include "platform/platform.hpp"
#include "types/remote.hpp"
#include "types/repertory.hpp"
#include "utils/common.hpp"
#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/time.hpp"
#include "utils/utils.hpp"

namespace repertory::remote_fuse {
#define RAISE_REMOTE_FUSE_SERVER_EVENT(func, file, ret)                        \
  if (config_.get_enable_drive_events() &&                                     \
      (((config_.get_event_level() >= remote_server_event::level) &&           \
        ((ret) < 0)) ||                                                        \
       (config_.get_event_level() >= event_level::trace)))                     \
  event_system::instance().raise<remote_server_event>(file, ret, func)

remote_server::remote_server(app_config &config, i_fuse_drive &drive,
                             std::string_view mount_location)
    : remote_server_base(config, drive, mount_location) {}

auto remote_server::construct_path(std::string_view path) -> std::string {
  auto ret = utils::path::combine(mount_location_, {path});
  if (ret == mount_location_) {
    ret += '/';
  }

  return ret;
}

auto remote_server::construct_path(std::wstring_view path) -> std::string {
  return construct_path(utils::string::to_utf8(path));
}

auto remote_server::empty_as_zero(const json &data) -> std::string {
  if (data.empty() || data.get<std::string>().empty()) {
    return "0";
  }
  return data.get<std::string>();
}

auto remote_server::get_next_handle() -> std::uint64_t {
  if (++next_handle_ == 0U) {
    return ++next_handle_;
  }

  return next_handle_;
}

auto remote_server::populate_file_info(std::string_view api_path,
                                       remote::file_info &r_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  std::string meta_attributes;
  auto directory = utils::file::directory(construct_path(api_path)).exists();

  auto res = drive_.get_item_meta(api_path, META_ATTRIBUTES, meta_attributes);
  if (res == api_error::success) {
    if (meta_attributes.empty()) {
      meta_attributes = directory ? std::to_string(FILE_ATTRIBUTE_DIRECTORY)
                                  : std::to_string(FILE_ATTRIBUTE_ARCHIVE);
      drive_.set_item_meta(api_path, META_ATTRIBUTES, meta_attributes);
    }

    auto attributes = utils::string::to_uint32(meta_attributes);
    auto file_size = directory ? 0U : drive_.get_file_size(api_path);
    populate_file_info(api_path, file_size, attributes, r_info);
    return STATUS_SUCCESS;
  }

  return static_cast<packet::error_type>(STATUS_OBJECT_NAME_NOT_FOUND);
}

void remote_server::populate_file_info(std::string_view api_path,
                                       UINT64 file_size, UINT32 attributes,
                                       remote::file_info &r_info) {
  REPERTORY_USES_FUNCTION_NAME();

  api_meta_map meta{};
  auto res = drive_.get_item_meta(api_path, meta);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(function_name, api_path, res,
                                       "get item meta failed");
  }
  r_info.AllocationSize =
      utils::divide_with_ceiling(file_size, WINFSP_ALLOCATION_UNIT) *
      WINFSP_ALLOCATION_UNIT;
  r_info.ChangeTime = utils::time::unix_time_to_windows_time(
      utils::string::to_uint64(empty_as_zero(meta[META_CHANGED])));
  r_info.CreationTime = utils::time::unix_time_to_windows_time(
      utils::string::to_uint64(empty_as_zero(meta[META_CREATION])));
  r_info.EaSize = 0;
  r_info.FileAttributes = attributes;
  r_info.FileSize = file_size;
  r_info.HardLinks = 0;
  r_info.IndexNumber = 0;
  r_info.LastAccessTime = utils::time::unix_time_to_windows_time(
      utils::string::to_uint64(empty_as_zero(meta[META_ACCESSED])));
  r_info.LastWriteTime = utils::time::unix_time_to_windows_time(
      utils::string::to_uint64(empty_as_zero(meta[META_WRITTEN])));
  if (meta[META_WRITTEN].empty() || (meta[META_WRITTEN] == "0") ||
      (meta[META_WRITTEN] == "116444736000000000")) {
    drive_.set_item_meta(api_path, META_WRITTEN, meta[META_MODIFIED]);
    r_info.LastWriteTime = r_info.ChangeTime;
  }
  r_info.ReparseTag = 0;
}

void remote_server::populate_stat(const struct stat64 &u_stat,
                                  remote::stat &r_stat) {
  r_stat = {};

#if defined(__APPLE__)
  r_stat.st_flags = u_stat.st_flags;

  r_stat.st_atimespec =
      u_stat.st_atimespec.tv_nsec +
      (u_stat.st_atimespec.tv_sec * utils::time::NANOS_PER_SECOND);
  r_stat.st_birthtimespec =
      u_stat.st_birthtimespec.tv_nsec +
      (u_stat.st_birthtimespec.tv_sec * utils::time::NANOS_PER_SECOND);
  r_stat.st_ctimespec =
      u_stat.st_ctimespec.tv_nsec +
      (u_stat.st_ctimespec.tv_sec * utils::time::NANOS_PER_SECOND);
  r_stat.st_mtimespec =
      u_stat.st_mtimespec.tv_nsec +
      (u_stat.st_mtimespec.tv_sec * utils::time::NANOS_PER_SECOND);
#else  // !defined(__APPLE__)
  r_stat.st_flags = 0;

  r_stat.st_atimespec = static_cast<remote::file_time>(u_stat.st_atim.tv_nsec) +
                        (static_cast<remote::file_time>(u_stat.st_atim.tv_sec) *
                         utils::time::NANOS_PER_SECOND);

  r_stat.st_birthtimespec =
      static_cast<remote::file_time>(u_stat.st_ctim.tv_nsec) +
      (static_cast<remote::file_time>(u_stat.st_ctim.tv_sec) *
       utils::time::NANOS_PER_SECOND);

  r_stat.st_ctimespec = static_cast<remote::file_time>(u_stat.st_ctim.tv_nsec) +
                        (static_cast<remote::file_time>(u_stat.st_ctim.tv_sec) *
                         utils::time::NANOS_PER_SECOND);

  r_stat.st_mtimespec = static_cast<remote::file_time>(u_stat.st_mtim.tv_nsec) +
                        (static_cast<remote::file_time>(u_stat.st_mtim.tv_sec) *
                         utils::time::NANOS_PER_SECOND);
#endif // defined(__APPLE__)
  r_stat.st_blksize = static_cast<remote::block_size>(u_stat.st_blksize);
  r_stat.st_blocks = static_cast<remote::block_count>(u_stat.st_blocks);
  r_stat.st_gid = u_stat.st_gid;
  r_stat.st_mode = static_cast<remote::file_mode>(u_stat.st_mode);
  r_stat.st_nlink = static_cast<remote::file_nlink>(u_stat.st_nlink);
  r_stat.st_size = static_cast<remote::file_size>(u_stat.st_size);
  r_stat.st_uid = u_stat.st_uid;
}

auto remote_server::fuse_access(const char *path, std::int32_t mask)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto api_path = utils::path::create_api_path(path);
  auto file_path = construct_path(path);

  auto res = access(file_path.c_str(), mask);
  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_chflags(const char *path, std::uint32_t flags)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto api_path = utils::path::create_api_path(path);
  auto file_path = construct_path(path);

  auto ret = -EACCES;
  if (drive_.check_parent_access(api_path, X_OK) == api_error::success) {
    ret = -EPERM;
    if (drive_.check_owner(api_path) == api_error::success) {
      ret = 0;
      drive_.set_item_meta(api_path, META_OSXFLAGS, std::to_string(flags));
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_chmod(const char *path, remote::file_mode mode)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res = chmod(file_path.c_str(), mode);
  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_chown(const char *path, remote::user_id uid,
                               remote::group_id gid) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res = chown(file_path.c_str(), uid, gid);
  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_create(const char *path, remote::file_mode mode,
                                const remote::open_flags &flags,
                                remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res = open(file_path.c_str(),
                  static_cast<int>(remote::create_os_open_flags(flags)), mode);
  if (res >= 0) {
    handle = static_cast<remote::file_handle>(res);
    set_open_info(res, open_info{
                           .client_id = "",
                           .directory_buffer = nullptr,
                           .handles = {},
                           .path = file_path,
                       });
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_destroy() -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, "", 0);
  return 0;
}

/*packet::error_type remote_server::fuse_fallocate(const char *path, const
std::int32_t &mode, remote::file_offset offset, const remote::file_offset
&length, remote::file_handle handle) { const auto file_path =
ConstructPath(path); auto ret = HasOpenFileInfo(handle, -EBADF); if (ret == 0) {
#if defined(__APPLE__)
    ret = STATUS_NOT_IMPLEMENTED;

    fstore_t fstore = {0};
    if (not(mode & PREALLOCATE)) {
      if (mode & ALLOCATECONTIG) {
        fstore.fst_flags |= F_ALLOCATECONTIG;
      }

      if (mode & ALLOCATEALL) {
        fstore.fst_flags |= F_ALLOCATEALL;
      }

      if (mode & ALLOCATEFROMPEOF) {
        fstore.fst_posmode = F_PEOFPOSMODE;
      } else if (mode & ALLOCATEFROMVOL) {
        fstore.fst_posmode = F_VOLPOSMODE;
      }

      fstore.fst_offset = offset;
      fstore.fst_length = length;

      const auto res = fcntl(static_cast<native_handle>(handle), F_PREALLOCATE,
&fstore); ret = ((res < 0) ? -errno : 0);
    }
#else
    const auto res = fallocate(static_cast<native_handle>(handle), mode, offset,
length); ret = ((res < 0) ? -errno : 0); #endif
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}*/

auto remote_server::fuse_fgetattr(const char * /* path */, remote::stat &r_stat,
                                  bool &directory, remote::file_handle handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  r_stat = {};

  auto res = -1;
  auto file_path = get_open_file_path(static_cast<native_handle>(handle));
  if (file_path.empty()) {
    errno = EBADF;
  } else {
    struct stat64 u_stat{};
    res = fstat64(static_cast<native_handle>(handle), &u_stat);
    if (res == -1 && errno == ESTALE) {
      std::uint64_t internal_handle{};
      res = ioctl(static_cast<native_handle>(handle),
                  repertory_ioctl_fd_command, &internal_handle);
      if (res == 0) {
        auto err = drive_.get_item_stat(internal_handle, &u_stat);
        if (err != api_error::success) {
          res = -1;
          errno = std::abs(utils::from_api_error(err));
        }
      }
    }

    if (res == 0) {
      directory = S_ISDIR(u_stat.st_mode);
      populate_stat(u_stat, r_stat);
    }
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_fsetattr_x(const char *path,
                                    const remote::setattr_x &attr,
                                    remote::file_handle handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto api_path = utils::path::create_api_path(path);
  auto file_path = construct_path(path);

  auto res = 0;
  if (SETATTR_WANTS_MODE(&attr)) {
    res = (handle == static_cast<std::uint64_t>(REPERTORY_INVALID_HANDLE))
              ? chmod(file_path.c_str(), attr.mode)
              : fchmod(static_cast<native_handle>(handle), attr.mode);
  }

  if (res >= 0) {
    auto uid = static_cast<uid_t>(-1);
    auto gid = static_cast<gid_t>(-1);
    if (SETATTR_WANTS_UID(&attr)) {
      uid = attr.uid;
    }

    if (SETATTR_WANTS_GID(&attr)) {
      gid = attr.gid;
    }

    if ((uid != static_cast<std::uint32_t>(-1)) ||
        (gid != static_cast<std::uint32_t>(-1))) {
      res = lchown(file_path.c_str(), uid, gid);
    }
  }

  if (res >= 0) {
    if (SETATTR_WANTS_SIZE(&attr)) {
      res = (handle == static_cast<std::uint64_t>(REPERTORY_INVALID_HANDLE))
                ? truncate(file_path.c_str(), static_cast<off_t>(attr.size))
                : ftruncate(static_cast<native_handle>(handle),
                            static_cast<off_t>(attr.size));
    }
  }

  if (res >= 0) {
    if (SETATTR_WANTS_BKUPTIME(&attr)) {
      res = fuse_setbkuptime(path, attr.bkuptime);
      errno = abs(res);
    }
  }

  if (res >= 0) {
    if (SETATTR_WANTS_CHGTIME(&attr)) {
      res = fuse_setchgtime(path, attr.chgtime);
      errno = abs(res);
    }
  }

  if (res >= 0) {
    if (SETATTR_WANTS_CRTIME(&attr)) {
      res = fuse_setcrtime(path, attr.crtime);
      errno = abs(res);
    }
  }

  if (res >= 0) {
    if (SETATTR_WANTS_MODTIME(&attr)) {
      struct timeval val[2];
      if (SETATTR_WANTS_ACCTIME(&attr)) {
        val[0U].tv_sec =
            static_cast<time_t>(attr.acctime / utils::time::NANOS_PER_SECOND);
        val[0U].tv_usec = static_cast<time_t>(
            (attr.acctime % utils::time::NANOS_PER_SECOND) / 1000);
      } else {
        gettimeofday(&val[0U], nullptr);
      }

      val[1U].tv_sec =
          static_cast<time_t>(attr.modtime / utils::time::NANOS_PER_SECOND);
      val[1U].tv_usec = static_cast<time_t>(
          (attr.modtime % utils::time::NANOS_PER_SECOND) / 1000);

      res = utimes(file_path.c_str(), &val[0U]);
    }
  }

  if (res >= 0) {
    if (SETATTR_WANTS_FLAGS(&attr)) {
      res = fuse_chflags(path, attr.flags);
      errno = abs(res);
    }
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_fsync(const char *path, std::int32_t datasync,
                               remote::file_handle handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);

#if defined(__APPLE__)
  auto res = datasync ? fcntl(static_cast<native_handle>(handle), F_FULLFSYNC)
                      : fsync(static_cast<native_handle>(handle));
#else  // !defined(__APPLE__)
  auto res = datasync ? fdatasync(static_cast<native_handle>(handle))
                      : fsync(static_cast<native_handle>(handle));
#endif // defined(__APPLE__)

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_ftruncate(const char *path, remote::file_offset size,
                                   remote::file_handle handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res =
      ftruncate(static_cast<native_handle>(handle), static_cast<off_t>(size));

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_getattr(const char *path, remote::stat &r_stat,
                                 bool &directory) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto api_path = utils::path::create_api_path(path);
  auto file_path = construct_path(api_path);

  r_stat = {};

  struct stat64 u_stat{};
  auto res = stat64(file_path.c_str(), &u_stat);
  if (res == 0) {
    directory = S_ISDIR(u_stat.st_mode);
    populate_stat(u_stat, r_stat);
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

/*packet::error_type remote_server::fuse_getxattr(const char *path, const char
*name, char *value, remote::file_size size) { const auto api_path =
utils::path::create_api_path(path); const auto file_path =
ConstructPath(api_path); const auto parentApiPath =
utils::path::get_parent_api_path(api_path);

#if defined(__APPLE__) || !defined(HAS_SETXATTR)
  auto ret = STATUS_NOT_IMPLEMENTED;
#else
  auto found = false;
  auto res = -1;
  errno = EACCES;
  if (drive_.CheckParentAccess(api_path, X_OK) == api_error::success) {
    res = errno = 0;
    directoryCacheManager_.execute_action(
        parentApiPath, [&](directory_iterator &iter) {
          directory_item directoryItem{};
          if ((found = (iter.Getdirectory_item(api_path, directoryItem) ==
                        api_error::success))) {
            drive_.Updatedirectory_item(directoryItem);
            res = -ENODATA;
            if (directoryItem.MetaMap.find(name) != directoryItem.MetaMap.end())
{ const auto data = macaron::Base64::Decode(directoryItem.MetaMap[name]); res =
static_cast<native_handle>(data.size()); if (size) { res = -ERANGE; if (size >=
data.size()) { memcpy(value, &data[0], data.size()); res = 0;
                }
              }
            }
          }
        });

    if (not found) {
      res = getxattr(file_path.c_str(), name, value, size);
    }
  }
  auto ret = found ? res : (((res < 0) ? -errno : 0));
#endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name,
                                 name ? std::string(name) + ":" + file_path :
filePath, ret); return ret;
}

packet::error_type remote_server::fuse_getxattrOSX(const char *path, const char
*name, char *value, remote::file_size size, std::uint32_t position) {
const auto file_path = ConstructPath(path); #if defined(__APPLE__) &&
defined(HAS_SETXATTR)
  // TODO: CheckParentAccess(api_path, X_OK)
  // TODO: Use iterator cache
  const auto res = getxattr(file_path.c_str(), name, value, size, position,
FSOPT_NOFOLLOW); auto ret = ((res < 0) ? -errno : 0); #else auto ret =
STATUS_NOT_IMPLEMENTED; #endif RAISE_REMOTE_FUSE_SERVER_EVENT(function_name,
file_path, ret); return ret;
}*/

auto remote_server::fuse_getxtimes(const char *path,
                                   remote::file_time &bkuptime,
                                   remote::file_time &crtime)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto api_path = utils::path::create_api_path(path);
  auto file_path = construct_path(path);

  auto ret = -EACCES;
  if (drive_.check_parent_access(api_path, X_OK) == api_error::success) {
    std::string value;
    auto res = drive_.get_item_meta(api_path, META_BACKUP, value);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(function_name, api_path, res,
                                         "get backup item meta failed");
    }
    bkuptime = value.empty() ? 0U : utils::string::to_uint64(value);

    res = drive_.get_item_meta(api_path, META_CREATION, value);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(function_name, api_path, res,
                                         "get creation time from meta failed");
    }
    crtime = value.empty() ? 0U : utils::string::to_uint64(value);

    ret = 0;
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_init() -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, "", 0);
  return 0;
}

/*packet::error_type remote_server::fuse_listxattr(const char *path, char
*buffer, remote::file_size size) { const auto file_path =
ConstructPath(path); #if defined(HAS_SETXATTR) #if defined(__APPLE__) const auto
res = listxattr(file_path.c_str(), buffer, size, FSOPT_NOFOLLOW); #else const
auto res = listxattr(file_path.c_str(), buffer, size); #endif auto ret = ((res <
0) ? -errno : 0); #else auto ret = STATUS_NOT_IMPLEMENTED; #endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}*/

auto remote_server::fuse_mkdir(const char *path, remote::file_mode mode)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res = mkdir(file_path.c_str(), mode);
  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_open(const char *path, const remote::open_flags &flags,
                              remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res = open(file_path.c_str(),
                  static_cast<int>(remote::create_os_open_flags(flags)));
  if (res >= 0) {
    handle = static_cast<remote::file_handle>(res);
    set_open_info(res, open_info{
                           .client_id = "",
                           .directory_buffer = nullptr,
                           .handles = {},
                           .path = file_path,
                       });
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_opendir(const char *path, remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);

  auto res = -1;
  errno = ENOENT;

  if (utils::file::directory(file_path).exists()) {
    auto iter = std::make_shared<directory_iterator>(
        drive_.get_directory_items(utils::path::create_api_path(path)));

    handle = get_next_handle();
    directory_cache_.set_directory(path, handle, iter);

    res = 0;
    errno = 0;
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_read(const char *path, char *buffer,
                              remote::file_size read_size,
                              remote::file_offset read_offset,
                              remote::file_handle handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto &data = *reinterpret_cast<data_buffer *>(buffer);

  data.resize(read_size);
  auto bytes_read = pread64(static_cast<native_handle>(handle), data.data(),
                            read_size, static_cast<off_t>(read_offset));

  auto ret = ((bytes_read < 0) ? -errno : bytes_read);
  if (ret < 0) {
    RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  }

  return static_cast<packet::error_type>(ret);
}

auto remote_server::fuse_rename(const char *from, const char *to)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto from_path = utils::path::combine(mount_location_, {from});
  auto to_path = utils::path::combine(mount_location_, {to});
  auto res = rename(from_path.c_str(), to_path.c_str());
  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, from + std::string("|") + to,
                                 ret);
  return ret;
}

auto remote_server::fuse_readdir(const char *path, remote::file_offset offset,
                                 remote::file_handle handle,
                                 std::string &item_path) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res = 0;
  if (offset > std::numeric_limits<std::size_t>::max()) {
    errno = ERANGE;
    res = -1;
  } else {
    auto iter = directory_cache_.get_directory(handle);
    if (iter == nullptr) {
      res = -1;
      errno = EFAULT;
    } else {
      res = iter->get(static_cast<std::size_t>(offset), item_path);
    }
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_release(const char *path, remote::file_handle handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet::error_type ret = 0;

  auto file_path = construct_path(path);
  auto res = close(static_cast<native_handle>(handle));
  remove_open_info(static_cast<native_handle>(handle));

  ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_releasedir(const char *path,
                                    remote::file_handle handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);

  directory_cache_.remove_directory(handle);

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, 0);
  return 0;
}

/*packet::error_type remote_server::fuse_removexattr(const char *path, const
char *name) { const auto file_path = ConstructPath(path); #if
defined(HAS_SETXATTR) #if defined(__APPLE__) const auto res =
removexattr(file_path.c_str(), name, FSOPT_NOFOLLOW); #else const auto res =
removexattr(file_path.c_str(), name); #endif auto ret = ((res < 0) ? -errno :
0); #else auto ret = STATUS_NOT_IMPLEMENTED; #endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}*/

auto remote_server::fuse_rmdir(const char *path) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res = rmdir(file_path.c_str());
  if (res == 0) {
    directory_cache_.remove_directory(utils::path::create_api_path(path));
  }
  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_setattr_x(const char *path, remote::setattr_x &attr)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto ret = fuse_fsetattr_x(
      path, attr, static_cast<remote::file_handle>(REPERTORY_INVALID_HANDLE));
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_setbkuptime(const char *path,
                                     remote::file_time bkuptime)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto api_path = utils::path::create_api_path(path);
  auto file_path = construct_path(path);

  auto ret = -EACCES;
  if (drive_.check_parent_access(api_path, X_OK) == api_error::success) {
    ret = -EPERM;
    if (drive_.check_owner(api_path) == api_error::success) {
      ret = 0;
      drive_.set_item_meta(api_path, META_BACKUP, std::to_string(bkuptime));
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_setchgtime(const char *path, remote::file_time chgtime)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto api_path = utils::path::create_api_path(path);
  auto file_path = construct_path(path);

  auto ret = -EACCES;
  if (drive_.check_parent_access(api_path, X_OK) == api_error::success) {
    ret = -EPERM;
    if (drive_.check_owner(api_path) == api_error::success) {
      ret = 0;
      drive_.set_item_meta(api_path, META_CHANGED, std::to_string(chgtime));
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_setcrtime(const char *path, remote::file_time crtime)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto api_path = utils::path::create_api_path(path);
  auto file_path = construct_path(path);

  auto ret = -EACCES;
  if (drive_.check_parent_access(api_path, X_OK) == api_error::success) {
    ret = -EPERM;
    if (drive_.check_owner(api_path) == api_error::success) {
      ret = 0;
      drive_.set_item_meta(api_path, META_CREATION, std::to_string(crtime));
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_setvolname(const char *volname) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, volname, 0);
  return 0;
}

/*packet::error_type remote_server::fuse_setxattr(const char *path, const char
*name, const char *value, remote::file_size size, const std::int32_t
&flags) { const auto file_path = ConstructPath(path); #if defined(__APPLE__{} ||
!defined(HAS_SETXATTR) auto ret = STATUS_NOT_IMPLEMENTED; #else const auto res =
setxattr(file_path.c_str(), name, value, size, flags); auto ret = ((res < 0) ?
-errno : 0); #endif RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path,
ret); return ret;
}

packet::error_type remote_server::fuse_setxattrOSX(const char *path, const char
*name, const char *value, remote::file_size size, const std::int32_t
&flags, const std::uint32_t &position) { const auto file_path =
ConstructPath(path); #if defined(__APPLE__) && defined(HAS_SETXATTR) const auto
res = setxattr(file_path.c_str(), name, value, size, position, flags); auto ret
=
((res < 0) ? -errno : 0); #else auto ret = STATUS_NOT_IMPLEMENTED; #endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}*/

auto remote_server::fuse_statfs(const char *path, std::uint64_t frsize,
                                remote::statfs &r_stat) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);

  auto total_bytes = drive_.get_total_drive_space();
  auto total_used = drive_.get_used_drive_space();
  auto used_blocks = utils::divide_with_ceiling(total_used, frsize);
  r_stat.f_files = 4294967295;
  r_stat.f_blocks = utils::divide_with_ceiling(total_bytes, frsize);
  r_stat.f_bavail = r_stat.f_bfree =
      r_stat.f_blocks ? (r_stat.f_blocks - used_blocks) : 0;
  r_stat.f_ffree = r_stat.f_favail =
      r_stat.f_files - drive_.get_total_item_count();

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, 0);
  return 0;
}

auto remote_server::fuse_statfs_x(const char *path, std::uint64_t bsize,
                                  remote::statfs_x &r_stat)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);

  auto total_bytes = drive_.get_total_drive_space();
  auto total_used = drive_.get_used_drive_space();
  auto used_blocks = utils::divide_with_ceiling(total_used, bsize);
  r_stat.f_files = 4294967295;
  r_stat.f_blocks = utils::divide_with_ceiling(total_bytes, bsize);
  r_stat.f_bavail = r_stat.f_bfree =
      r_stat.f_blocks ? (r_stat.f_blocks - used_blocks) : 0;
  r_stat.f_ffree = r_stat.f_favail =
      r_stat.f_files - drive_.get_total_item_count();
  std::memset(r_stat.f_mntfromname.data(), 0, r_stat.f_mntfromname.size());
  strncpy(r_stat.f_mntfromname.data(),
          (utils::create_volume_label(config_.get_provider_type())).c_str(),
          r_stat.f_mntfromname.size() - 1U);

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, 0);
  return 0;
}

auto remote_server::fuse_truncate(const char *path, remote::file_offset size)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res = truncate(file_path.c_str(), static_cast<off_t>(size));
  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_unlink(const char *path) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res = unlink(file_path.c_str());
  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_utimens(const char *path, const remote::file_time *tv,
                                 std::uint64_t op0, std::uint64_t op1)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);

  std::array<struct timespec, 2U> tv2{};
  const auto process_timespec = [](auto cur_op, const auto &src, auto &dst) {
    if ((cur_op == UTIME_NOW) || (cur_op == UTIME_OMIT)) {
      dst.tv_nsec = static_cast<time_t>(cur_op);
      dst.tv_sec = 0;
      return;
    }

    dst.tv_nsec = static_cast<time_t>(src % utils::time::NANOS_PER_SECOND);
    dst.tv_sec = static_cast<time_t>(src / utils::time::NANOS_PER_SECOND);
  };

  process_timespec(op0, tv[0U], tv2[0U]);
  process_timespec(op1, tv[1U], tv2[1U]);

  auto res = utimensat(0, file_path.c_str(), tv2.data(), AT_SYMLINK_NOFOLLOW);
  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_write(const char *path, const char *buffer,
                               remote::file_size write_size,
                               remote::file_offset write_offset,
                               remote::file_handle handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);

  auto bytes_written = pwrite64(static_cast<native_handle>(handle), buffer,
                                write_size, static_cast<off_t>(write_offset));

  auto ret = ((bytes_written < 0) ? -errno : bytes_written);
  if (ret < 0) {
    RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  }

  return static_cast<packet::error_type>(ret);
}

auto remote_server::fuse_write_base64(const char * /*path*/,
                                      const char * /*buffer*/,
                                      remote::file_size /*write_size*/,
                                      remote::file_offset /*write_offset*/,
                                      remote::file_handle /*handle*/)
    -> packet::error_type {
  // DOES NOTHING
  return 0;
}

// WinFSP Layer
auto remote_server::winfsp_can_delete(PVOID file_desc, PWSTR file_name)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto relative_path = utils::string::to_utf8(file_name);
  auto file_path = construct_path(relative_path);
  auto ret = static_cast<packet::error_type>(
      has_open_info(static_cast<native_handle>(
                        reinterpret_cast<remote::file_handle>(file_desc)),
                    STATUS_INVALID_HANDLE));
  if (ret == STATUS_SUCCESS) {
    ret = static_cast<packet::error_type>(
        utils::file::directory(file_path).exists()
            ? drive_.get_directory_item_count(
                  utils::path::create_api_path(relative_path)) == 0U
                  ? STATUS_SUCCESS
                  : STATUS_DIRECTORY_NOT_EMPTY

            : STATUS_SUCCESS);
  }
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::winfsp_cleanup(PVOID /*file_desc*/, PWSTR file_name,
                                   UINT32 flags, BOOLEAN &was_deleted)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto relative_path = utils::string::to_utf8(file_name);
  auto file_path = construct_path(relative_path);
  was_deleted = 0U;

  auto directory = utils::file::directory(file_path).exists();
  if (flags & FileSystemBase::FspCleanupDelete) {
    remove_all(file_path);
    was_deleted = 1U;

    if (directory) {
      if (drive_.get_directory_item_count(
              utils::path::create_api_path(relative_path)) == 0U) {
        rmdir(file_path.c_str());
      }
    } else {
      unlink(file_path.c_str());
    }
  } else {
    auto api_path = utils::path::create_api_path(relative_path);
    if ((flags & FileSystemBase::FspCleanupSetArchiveBit) && not directory) {
      api_meta_map meta;
      if (drive_.get_item_meta(api_path, meta) == api_error::success) {
        drive_.set_item_meta(
            api_path, META_ATTRIBUTES,
            std::to_string(utils::get_attributes_from_meta(meta) |
                           FILE_ATTRIBUTE_ARCHIVE));
      }
    }

    if (flags & (FileSystemBase::FspCleanupSetLastAccessTime |
                 FileSystemBase::FspCleanupSetLastWriteTime |
                 FileSystemBase::FspCleanupSetChangeTime)) {
      auto file_time_now = utils::time::get_time_now();
      if (flags & FileSystemBase::FspCleanupSetLastAccessTime) {
        drive_.set_item_meta(api_path, META_ACCESSED,
                             std::to_string(file_time_now));
      }

      if (flags & FileSystemBase::FspCleanupSetLastWriteTime) {
        drive_.set_item_meta(api_path, META_WRITTEN,
                             std::to_string(file_time_now));
      }

      if (flags & FileSystemBase::FspCleanupSetChangeTime) {
        drive_.set_item_meta(api_path, META_MODIFIED,
                             std::to_string(file_time_now));
      }
    }

    if (flags & FileSystemBase::FspCleanupSetAllocationSize) {
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, 0);
  return 0;
}

auto remote_server::winfsp_close(PVOID file_desc) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  std::string file_path;
  auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  if (has_open_info(static_cast<native_handle>(handle),
                    STATUS_INVALID_HANDLE) == STATUS_SUCCESS) {
    file_path = get_open_file_path(static_cast<native_handle>(handle));
    close(static_cast<native_handle>(handle));
    remove_open_info(static_cast<native_handle>(handle));
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

auto remote_server::winfsp_create(PWSTR file_name, UINT32 create_options,
                                  UINT32 granted_access, UINT32 attributes,
                                  UINT64 /*allocation_size*/, PVOID *file_desc,
                                  remote::file_info *r_info,
                                  std::string &normalized_name, BOOLEAN &exists)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto relative_path = utils::string::to_utf8(file_name);
  auto file_path = construct_path(relative_path);
  exists = utils::file::file{file_path}.exists() ||
                   utils::file::directory{file_path}.exists()
               ? 1
               : 0;
  auto ret{static_cast<packet::error_type>(STATUS_SUCCESS)};
  if (exists == 0U) {
    if ((create_options & FILE_DIRECTORY_FILE) != 0U) {
      attributes |= FILE_ATTRIBUTE_DIRECTORY;
    } else {
      attributes &= static_cast<UINT32>(~FILE_ATTRIBUTE_DIRECTORY);
      attributes |= FILE_ATTRIBUTE_ARCHIVE;
    }

    remote::file_mode mode{0U};
    std::uint32_t flags{0U};
    utils::windows_create_to_unix(create_options, granted_access, flags, mode);

    auto res{0};
    if ((create_options & FILE_DIRECTORY_FILE) != 0U) {
      res = mkdir(file_path.c_str(), mode);
      if (res >= 0) {
        res = open(file_path.c_str(), static_cast<int>(flags));
      }
    } else {
      res = open(file_path.c_str(), static_cast<int>(flags), mode);
    }

    if (res >= 0) {
      *file_desc = reinterpret_cast<PVOID>(res);
      drive_.set_item_meta(construct_api_path(file_path), META_ATTRIBUTES,
                           std::to_string(attributes));
      set_open_info(res, open_info{
                             .client_id = "",
                             .directory_buffer = nullptr,
                             .handles = {},
                             .path = file_path,
                         });

      auto api_path = utils::path::create_api_path(relative_path);
      normalized_name = utils::string::replace_copy(api_path, '/', '\\');
      populate_file_info(api_path, 0, attributes, *r_info);
    }

    ret = static_cast<packet::error_type>(
        utils::unix_error_to_windows((res < 0) ? errno : 0));
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::winfsp_flush(PVOID file_desc, remote::file_info *r_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = static_cast<packet::error_type>(
      has_open_info(static_cast<native_handle>(handle), STATUS_INVALID_HANDLE));
  if (ret == STATUS_SUCCESS) {
    ret = (fsync(static_cast<native_handle>(handle)) < 0)
              ? static_cast<packet::error_type>(
                    utils::unix_error_to_windows(errno))
              : populate_file_info(construct_api_path(get_open_file_path(
                                       static_cast<native_handle>(handle))),
                                   *r_info);
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(
      function_name, get_open_file_path(static_cast<native_handle>(handle)),
      ret);
  return ret;
}

auto remote_server::winfsp_get_file_info(PVOID file_desc,
                                         remote::file_info *r_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = static_cast<packet::error_type>(
      has_open_info(static_cast<native_handle>(handle), STATUS_INVALID_HANDLE));
  if (ret == STATUS_SUCCESS) {
    ret = populate_file_info(construct_api_path(get_open_file_path(
                                 static_cast<native_handle>(handle))),
                             *r_info);
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(
      function_name, get_open_file_path(static_cast<native_handle>(handle)),
      ret);
  return ret;
}

auto remote_server::winfsp_get_security_by_name(
    PWSTR file_name, PUINT32 attributes,
    std::uint64_t * /*security_descriptor_size*/,
    std::wstring & /*str_descriptor*/) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);
  auto file_path = construct_path(file_name);
  if (utils::file::file(file_path).exists() ||
      (utils::file::directory(file_path).exists())) {
    if (attributes) {
      remote::file_info r_info{};
      if ((ret = populate_file_info(construct_api_path(file_path), r_info)) ==
          STATUS_SUCCESS) {
        *attributes = r_info.FileAttributes;
      }
    }
  } else {
    ret = static_cast<packet::error_type>(STATUS_OBJECT_NAME_NOT_FOUND);
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, construct_path(file_name), ret);
  return ret;
}

auto remote_server::winfsp_get_volume_info(UINT64 &total_size,
                                           UINT64 &free_size,
                                           std::string &volume_label)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  drive_.get_volume_info(total_size, free_size, volume_label);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, volume_label, STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

auto remote_server::winfsp_mounted(std::wstring_view location)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  RAISE_REMOTE_FUSE_SERVER_EVENT(
      function_name, utils::string::to_utf8(location), STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

auto remote_server::winfsp_open(PWSTR file_name, UINT32 create_options,
                                UINT32 granted_access, PVOID *file_desc,
                                remote::file_info *r_info,
                                std::string &normalized_name)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto relative_path = utils::string::to_utf8(file_name);
  auto file_path = construct_path(relative_path);
  auto directory = utils::file::directory(file_path).exists();
  if (directory) {
    create_options |= FILE_DIRECTORY_FILE;
  }

  remote::file_mode mode{};
  std::uint32_t flags{};
  utils::windows_create_to_unix(create_options, granted_access, flags, mode);
  flags &= static_cast<std::uint32_t>(~O_CREAT);

  auto res = open(file_path.c_str(), static_cast<int>(flags));
  if (res >= 0) {
    *file_desc = reinterpret_cast<PVOID>(res);
    set_open_info(res, open_info{
                           .client_id = "",
                           .directory_buffer = nullptr,
                           .handles = {},
                           .path = file_path,
                       });

    auto api_path = utils::path::create_api_path(relative_path);
    normalized_name = utils::string::replace_copy(api_path, '/', '\\');
    res = populate_file_info(api_path, *r_info);
    if (res != STATUS_SUCCESS) {
      utils::error::raise_error(function_name,
                                "populate file info failed|err|" +
                                    std::to_string(res));
    }
  }

  auto ret = static_cast<packet::error_type>(
      utils::unix_error_to_windows((res < 0) ? errno : 0));
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::winfsp_overwrite(PVOID file_desc, UINT32 attributes,
                                     BOOLEAN replace_attributes,
                                     UINT64 /*allocation_size*/,
                                     remote::file_info *r_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = static_cast<packet::error_type>(
      has_open_info(static_cast<native_handle>(handle), STATUS_INVALID_HANDLE));
  if (ret == STATUS_SUCCESS) {
    auto api_path = construct_api_path(
        get_open_file_path(static_cast<native_handle>(handle)));
    auto res = ftruncate(static_cast<native_handle>(handle), 0);
    if (res >= 0) {
      auto set_attributes = false;
      if (replace_attributes) {
        set_attributes = true;
      } else if (attributes) {
        std::string current_attributes;
        auto err =
            drive_.get_item_meta(api_path, META_ATTRIBUTES, current_attributes);
        if (err != api_error::success) {
          utils::error::raise_api_path_error(function_name, api_path, err,
                                             "get attributes from meta failed");
        }
        if (current_attributes.empty()) {
          current_attributes = "0";
        }

        attributes = attributes | utils::string::to_uint32(current_attributes);
        set_attributes = true;
      }

      if (set_attributes) {
        attributes &= static_cast<UINT32>(~FILE_ATTRIBUTE_NORMAL);
        if (attributes == 0U) {
          attributes = FILE_ATTRIBUTE_ARCHIVE;
        }
        drive_.set_item_meta(api_path, META_ATTRIBUTES,
                             std::to_string(attributes));
      }
      ret = populate_file_info(construct_api_path(get_open_file_path(
                                   static_cast<native_handle>(handle))),
                               *r_info);
    } else {
      ret =
          static_cast<packet::error_type>(utils::unix_error_to_windows(errno));
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(
      function_name, get_open_file_path(static_cast<native_handle>(handle)),
      ret);
  return ret;
}

auto remote_server::winfsp_read(PVOID file_desc, PVOID buffer, UINT64 offset,
                                UINT32 length, PUINT32 bytes_transferred)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  *bytes_transferred = 0U;

  auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = static_cast<packet::error_type>(
      has_open_info(static_cast<native_handle>(handle), STATUS_INVALID_HANDLE));
  if (ret == STATUS_SUCCESS) {
    auto res = pread64(static_cast<native_handle>(handle), buffer, length,
                       static_cast<off_t>(offset));
    if (res >= 0) {
      *bytes_transferred = static_cast<UINT32>(res);
      if (*bytes_transferred == 0U) {
        ret = static_cast<packet::error_type>(STATUS_END_OF_FILE);
      }
    } else {
      ret =
          static_cast<packet::error_type>(utils::unix_error_to_windows(errno));
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(
      function_name, get_open_file_path(static_cast<native_handle>(handle)),
      ret);
  return ret;
}

auto remote_server::winfsp_read_directory(PVOID file_desc, PWSTR /*pattern*/,
                                          PWSTR marker, json &item_list)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  item_list.clear();

  auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = static_cast<packet::error_type>(
      has_open_info(static_cast<native_handle>(handle), STATUS_INVALID_HANDLE));
  if (ret == STATUS_SUCCESS) {
    auto api_path = construct_api_path(
        get_open_file_path(static_cast<native_handle>(handle)));
    directory_iterator iterator(drive_.get_directory_items(api_path));
    auto offset = marker == nullptr
                      ? 0
                      : iterator.get_next_directory_offset(
                            utils::path::create_api_path(utils::path::combine(
                                api_path, {utils::string::to_utf8(marker)})));
    json item;
    while (iterator.get_json(offset++, item) == 0) {
      try {
        item_list.emplace_back(update_to_windows_format(api_path, item));
      } catch (const std::exception &e) {
        utils::error::raise_error(function_name, e, "exception occurred");
      }
    }
    ret = STATUS_SUCCESS;
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(
      function_name, get_open_file_path(static_cast<native_handle>(handle)),
      ret);
  return ret;
}

auto remote_server::winfsp_rename(PVOID /*file_desc*/, PWSTR file_name,
                                  PWSTR new_file_name,
                                  BOOLEAN replace_if_exists)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto relative_path = utils::string::to_utf8(file_name);
  auto file_path = construct_path(relative_path);
  auto new_relative_path = utils::string::to_utf8(new_file_name);
  auto new_file_path = construct_path(new_relative_path);

  packet::error_type ret{};
  auto res{-1};
  errno = ENOENT;
  if (utils::file::file(file_path).exists()) {
    res = drive_.rename_file(construct_api_path(file_path),
                             construct_api_path(new_file_path),
                             replace_if_exists != 0U);
    ret = ((res < 0) ? static_cast<packet::error_type>(
                           utils::unix_error_to_windows(errno))
                     : 0);
  } else {
    auto dir{utils::file::directory(file_path)};
    if (dir.exists()) {
      auto count{dir.count(false)};
      if (count == 0U) {
        res = drive_.rename_directory(construct_api_path(file_path),
                                      construct_api_path(new_file_path));
        ret = ((res < 0)
                   ? errno == EISDIR
                         ? static_cast<packet::error_type>(STATUS_ACCESS_DENIED)
                         : static_cast<packet::error_type>(
                               utils::unix_error_to_windows(errno))
                   : 0);
      } else {
        ret = static_cast<packet::error_type>(STATUS_ACCESS_DENIED);
      }
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path + "|" + new_file_path,
                                 ret);
  return ret;
}

auto remote_server::winfsp_set_basic_info(
    PVOID file_desc, UINT32 attributes, UINT64 creation_time,
    UINT64 last_access_time, UINT64 last_write_time, UINT64 change_time,
    remote::file_info *r_info) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = static_cast<packet::error_type>(
      has_open_info(static_cast<native_handle>(handle), STATUS_INVALID_HANDLE));
  if (ret == STATUS_SUCCESS) {
    auto file_path = get_open_file_path(static_cast<native_handle>(handle));
    if (attributes == INVALID_FILE_ATTRIBUTES) {
      attributes = 0;
    } else if (attributes == 0) {
      attributes = utils::file::directory(file_path).exists()
                       ? FILE_ATTRIBUTE_DIRECTORY
                       : FILE_ATTRIBUTE_ARCHIVE;
    }

    auto api_path = construct_api_path(file_path);
    api_meta_map meta;
    if (attributes != 0U) {
      attributes &= static_cast<UINT32>(~FILE_ATTRIBUTE_NORMAL);
      if (attributes == 0U) {
        attributes |= static_cast<UINT32>(FILE_ATTRIBUTE_ARCHIVE);
      }
      drive_.set_item_meta(api_path, META_ATTRIBUTES,
                           std::to_string(attributes));
    }

    if ((creation_time != 0U) && (creation_time != max_time)) {
      drive_.set_item_meta(
          api_path, META_CREATION,
          std::to_string(
              utils::time::windows_time_to_unix_time(creation_time)));
    }

    if ((last_access_time != 0U) && (last_access_time != max_time)) {
      drive_.set_item_meta(
          api_path, META_ACCESSED,
          std::to_string(
              utils::time::windows_time_to_unix_time(last_access_time)));
    }

    if ((last_write_time != 0U) && (last_write_time != max_time)) {
      drive_.set_item_meta(
          api_path, META_WRITTEN,
          std::to_string(
              utils::time::windows_time_to_unix_time(last_write_time)));
    }

    if ((change_time != 0U) && (change_time != max_time)) {
      drive_.set_item_meta(
          api_path, META_MODIFIED,
          std::to_string(utils::time::windows_time_to_unix_time(change_time)));
    }

    ret = populate_file_info(api_path, *r_info);
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(
      function_name, get_open_file_path(static_cast<native_handle>(handle)),
      ret);
  return ret;
}

auto remote_server::winfsp_set_file_size(PVOID file_desc, UINT64 new_size,
                                         BOOLEAN set_allocation_size,
                                         remote::file_info *r_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = static_cast<packet::error_type>(
      has_open_info(static_cast<native_handle>(handle), STATUS_INVALID_HANDLE));
  if (ret == STATUS_SUCCESS) {
    auto res = set_allocation_size == 0U
                   ? ftruncate(static_cast<native_handle>(handle),
                               static_cast<off_t>(new_size))
                   : 0;
    ret = ((res < 0) ? static_cast<packet::error_type>(
                           utils::unix_error_to_windows(errno))
                     : 0);
    if (ret == 0) {
      ret = populate_file_info(construct_api_path(get_open_file_path(
                                   static_cast<native_handle>(handle))),
                               *r_info);
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(
      function_name, get_open_file_path(static_cast<native_handle>(handle)),
      ret);
  return ret;
}

auto remote_server::winfsp_unmounted(std::wstring_view location)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  RAISE_REMOTE_FUSE_SERVER_EVENT(
      function_name, utils::string::to_utf8(location), STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

auto remote_server::winfsp_write(PVOID file_desc, PVOID buffer, UINT64 offset,
                                 UINT32 length, BOOLEAN write_to_end,
                                 BOOLEAN constrained_io,
                                 PUINT32 bytes_transferred,
                                 remote::file_info *r_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  *bytes_transferred = 0U;

  auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = static_cast<packet::error_type>(
      has_open_info(static_cast<native_handle>(handle), STATUS_INVALID_HANDLE));
  if (ret == STATUS_SUCCESS) {
    auto api_path = construct_api_path(
        get_open_file_path(static_cast<native_handle>(handle)));
    auto file_size = drive_.get_file_size(api_path);
    if (write_to_end != 0U) {
      offset = file_size;
    }

    auto should_write = true;
    if (constrained_io != 0U) {
      if (offset >= file_size) {
        ret = STATUS_SUCCESS;
        should_write = false;
      } else if ((offset + length) > file_size) {
        length = static_cast<UINT32>(file_size - offset);
      }
    }

    if (should_write) {
      if (length > 0) {
        auto res = pwrite64(static_cast<native_handle>(handle), buffer, length,
                            static_cast<off_t>(offset));
        if (res >= 0) {
          *bytes_transferred = static_cast<UINT32>(res);
          ret = populate_file_info(construct_api_path(get_open_file_path(
                                       static_cast<native_handle>(handle))),
                                   *r_info);
        } else {
          ret = static_cast<packet::error_type>(
              utils::unix_error_to_windows(errno));
        }
      }
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(
      function_name, get_open_file_path(static_cast<native_handle>(handle)),
      ret);
  return ret;
}

auto remote_server::json_create_directory_snapshot(std::string_view path,
                                                   json &json_data)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto api_path = utils::path::create_api_path(path);
  auto file_path = construct_path(api_path);

  auto res = -1;
  errno = ENOENT;

  if (utils::file::directory(file_path).exists()) {
    auto iter = std::make_shared<directory_iterator>(
        drive_.get_directory_items(api_path));
    auto handle = get_next_handle();
    directory_cache_.set_directory(api_path, handle, iter);

    json_data["handle"] = handle;
    json_data["path"] = path;
    json_data["page_count"] = utils::divide_with_ceiling(
        iter->get_count(), default_remote_directory_page_size);
    res = 0;
    errno = 0;
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::json_read_directory_snapshot(std::string_view path,
                                                 remote::file_handle handle,
                                                 std::uint32_t page,
                                                 json &json_data)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  int res{-EBADF};

  auto file_path = construct_path(path);
  auto iter = directory_cache_.get_directory(handle);
  if (iter != nullptr) {
    std::size_t offset{};
    json item_json;
    while ((json_data["directory_list"].size() <
            default_remote_directory_page_size) &&
           (res = iter->get_json((page * default_remote_directory_page_size) +
                                     offset++,
                                 item_json)) == 0) {
      json_data["directory_list"].emplace_back(item_json);
    }
    json_data["handle"] = handle;
    json_data["path"] = path;
    json_data["page"] = page;
    json_data["page_count"] = utils::divide_with_ceiling(
        iter->get_count(), default_remote_directory_page_size);
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::json_release_directory_snapshot(std::string_view path,
                                                    remote::file_handle handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  directory_cache_.remove_directory(handle);
  RAISE_REMOTE_FUSE_SERVER_EVENT(function_name, file_path, 0);
  return 0;
}

auto remote_server::update_to_windows_format(std::string_view root_api_path,
                                             json &item) -> json & {
  REPERTORY_USES_FUNCTION_NAME();

  auto api_path = item[JSON_API_PATH].get<std::string>();
  if (api_path == "." || api_path == "..") {
    auto orig_api_path{api_path};

    api_path = api_path == "."
                   ? root_api_path
                   : utils::path::get_parent_api_path(root_api_path);

    api_meta_map meta;
    auto res = drive_.get_item_meta(api_path, meta);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(
          function_name, api_path, res,
          fmt::format("failed to get '{}' meta", orig_api_path));
      return item;
    }

    item[JSON_DIRECTORY] = true;
    meta[META_DIRECTORY] = utils::string::from_bool(true);
    item[JSON_META] = meta;
  }

  auto update_meta{false};
  const auto ensure_set = [&item, &update_meta](std::string_view name,
                                                std::string_view default_value,
                                                bool as_time) {
    if (not item[JSON_META].contains(name) || item[JSON_META][name].empty() ||
        (as_time && (item[JSON_META][name].template get<std::string>() == "0" ||
                     item[JSON_META][name].template get<std::string>() ==
                         std::to_string(utils::time::WIN32_TIME_CONVERSION)))) {
      item[JSON_META][name] = default_value;
      update_meta = true;
    }
  };

  auto default_time = std::to_string(utils::time::get_time_now());
  ensure_set(META_ACCESSED, default_time, true);
  ensure_set(META_CREATION, default_time, true);
  ensure_set(META_MODIFIED, default_time, true);
  ensure_set(META_CHANGED, item[JSON_META][META_MODIFIED].get<std::string>(),
             true);
  ensure_set(META_WRITTEN, item[JSON_META][META_MODIFIED].get<std::string>(),
             true);
  ensure_set(META_ATTRIBUTES, "0", false);

  auto attributes = utils::string::to_uint32(
      item[JSON_META][META_ATTRIBUTES].get<std::string>());
  if (item[JSON_DIRECTORY].get<bool>()) {
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY) {
      attributes |= FILE_ATTRIBUTE_DIRECTORY;
      attributes &= static_cast<std::uint32_t>(~FILE_ATTRIBUTE_ARCHIVE);
      item[JSON_META][META_ATTRIBUTES] = std::to_string(attributes);
      update_meta = true;
    }
  } else if ((attributes & FILE_ATTRIBUTE_DIRECTORY) ==
                 FILE_ATTRIBUTE_DIRECTORY ||
             attributes == 0U) {
    attributes |= FILE_ATTRIBUTE_ARCHIVE;
    attributes &= static_cast<std::uint32_t>(~FILE_ATTRIBUTE_DIRECTORY);
    item[JSON_META][META_ATTRIBUTES] = std::to_string(attributes);
    update_meta = true;
  }

  if (update_meta) {
    drive_.set_item_meta(api_path, item[JSON_META].get<api_meta_map>());
  }

  return item;
}
} // namespace repertory::remote_fuse

#endif // _WIN32
