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
#ifndef _WIN32
#include "drives/fuse/remotefuse/remote_server.hpp"
#include "comm/packet/client_pool.hpp"
#include "comm/packet/packet.hpp"
#include "comm/packet/packet_server.hpp"
#include "app_config.hpp"
#include "drives/directory_iterator.hpp"
#include "drives/fuse/remotefuse/i_remote_instance.hpp"
#include "drives/remote/remote_open_file_table.hpp"
#include "drives/winfsp/remotewinfsp/i_remote_instance.hpp"
#include "events/events.hpp"
#include "events/event_system.hpp"
#include "types/remote.hpp"
#include "utils/Base64.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"

namespace repertory::remote_fuse {
#define RAISE_REMOTE_FUSE_SERVER_EVENT(func, file, ret)                                            \
  if (config_.get_enable_drive_events() &&                                                         \
      (((config_.get_event_level() >= remote_fuse_server_event::level) && (ret < 0)) ||            \
       (config_.get_event_level() >= event_level::verbose)))                                       \
  event_system::instance().raise<remote_fuse_server_event>(func, file, ret)

// clang-format off
E_SIMPLE3(remote_fuse_server_event, debug, true,
  std::string, function, func, E_STRING,
  std::string, api_path, ap, E_STRING,
  packet::error_type, result, res, E_FROM_INT32
);
// clang-format on

remote_server::remote_server(app_config &config, i_fuse_drive &drive,
                             const std::string &mount_location)
    : remote_server_base(config, drive, mount_location) {}

std::string remote_server::construct_path(std::string path) {
  path = utils::path::combine(mount_location_, {path});
  if (path == mount_location_) {
    path += '/';
  }

  return path;
}

std::string remote_server::construct_path(const std::wstring &path) {
  return construct_path(utils::string::to_utf8(path));
}

void remote_server::delete_open_directory(void *dir) {
  directory_cache_.remove_directory(reinterpret_cast<directory_iterator *>(dir));
  remote_server_base::delete_open_directory(dir);
}

std::string remote_server::empty_as_zero(const json &data) {
  if (data.empty() || data.get<std::string>().empty()) {
    return "0";
  }
  return data.get<std::string>();
}

packet::error_type remote_server::populate_file_info(const std::string &api_path,
                                                     remote::file_info &file_info) {
  std::string meta_attributes;
  auto error = drive_.get_item_meta(api_path, META_ATTRIBUTES, meta_attributes);
  if (error == api_error::success) {
    if (meta_attributes.empty()) {
      meta_attributes = utils::file::is_directory(construct_path(api_path))
                            ? utils::string::from_uint32(FILE_ATTRIBUTE_DIRECTORY)
                            : utils::string::from_uint32(FILE_ATTRIBUTE_NORMAL);
      drive_.set_item_meta(api_path, META_ATTRIBUTES, meta_attributes);
    }
    const auto attributes = utils::string::to_uint32(meta_attributes);
    const auto file_size = drive_.get_file_size(api_path);
    populate_file_info(api_path, file_size, attributes, file_info);
    return STATUS_SUCCESS;
  }

  return STATUS_OBJECT_NAME_NOT_FOUND;
}

void remote_server::populate_file_info(const std::string &api_path, const UINT64 &file_size,
                                       const UINT32 &attributes, remote::file_info &file_info) {
  api_meta_map meta{};
  drive_.get_item_meta(api_path, meta);
  file_info.AllocationSize =
      utils::divide_with_ceiling(file_size, WINFSP_ALLOCATION_UNIT) * WINFSP_ALLOCATION_UNIT;
  file_info.ChangeTime = utils::unix_time_to_windows_time(
      utils::string::to_uint64(empty_as_zero(meta[META_MODIFIED])));
  file_info.CreationTime = utils::unix_time_to_windows_time(
      utils::string::to_uint64(empty_as_zero(meta[META_CREATION])));
  file_info.EaSize = 0;
  file_info.FileAttributes = attributes;
  file_info.FileSize = file_size;
  file_info.HardLinks = 0;
  file_info.IndexNumber = 0;
  file_info.LastAccessTime = utils::unix_time_to_windows_time(
      utils::string::to_uint64(empty_as_zero(meta[META_ACCESSED])));
  file_info.LastWriteTime =
      utils::unix_time_to_windows_time(utils::string::to_uint64(empty_as_zero(meta[META_WRITTEN])));
  if (meta[META_WRITTEN].empty() || (meta[META_WRITTEN] == "0") ||
      (meta[META_WRITTEN] == "116444736000000000")) {
    drive_.set_item_meta(api_path, META_WRITTEN, meta[META_MODIFIED]);
    file_info.LastWriteTime = file_info.ChangeTime;
  }
  file_info.ReparseTag = 0;
}

void remote_server::populate_stat(const struct stat &st1, remote::stat &st) {
  memset(&st, 0, sizeof(st));
#ifdef __APPLE__
  st.st_flags = st1.st_flags;

  st.st_atimespec = st1.st_atimespec.tv_nsec + (st1.st_atimespec.tv_sec * NANOS_PER_SECOND);
  st.st_birthtimespec =
      st1.st_birthtimespec.tv_nsec + (st1.st_birthtimespec.tv_sec * NANOS_PER_SECOND);
  st.st_ctimespec = st1.st_ctimespec.tv_nsec + (st1.st_ctimespec.tv_sec * NANOS_PER_SECOND);
  st.st_mtimespec = st1.st_mtimespec.tv_nsec + (st1.st_mtimespec.tv_sec * NANOS_PER_SECOND);
#else
  st.st_flags = 0;

  st.st_atimespec = st1.st_atim.tv_nsec + (st1.st_atim.tv_sec * NANOS_PER_SECOND);
  st.st_birthtimespec = st1.st_ctim.tv_nsec + (st1.st_ctim.tv_sec * NANOS_PER_SECOND);
  st.st_ctimespec = st1.st_ctim.tv_nsec + (st1.st_ctim.tv_sec * NANOS_PER_SECOND);
  st.st_mtimespec = st1.st_mtim.tv_nsec + (st1.st_mtim.tv_sec * NANOS_PER_SECOND);
#endif
  st.st_blksize = st1.st_blksize;
  st.st_blocks = st1.st_blocks;
  st.st_gid = st1.st_gid;
  st.st_mode = st1.st_mode;
  st.st_nlink = st1.st_nlink;
  st.st_size = st1.st_size;
  st.st_uid = st1.st_uid;
}

packet::error_type remote_server::fuse_access(const char *path, const std::int32_t &mask) {
  const auto file_path = construct_path(path);
  const auto res = access(file_path.c_str(), mask);
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_chflags(const char *path, const std::uint32_t &flags) {
  const auto file_path = construct_path(path);
#ifdef __APPLE__
  const auto res = chflags(file_path.c_str(), flags);
  const auto ret = ((res < 0) ? -errno : 0);
#else
  const auto ret = STATUS_NOT_IMPLEMENTED;
#endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_chmod(const char *path, const remote::file_mode &mode) {
  const auto file_path = construct_path(path);
  const auto res = chmod(file_path.c_str(), mode);
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_chown(const char *path, const remote::user_id &uid,
                                             const remote::group_id &gid) {
  const auto file_path = construct_path(path);
  const auto res = chown(file_path.c_str(), uid, gid);
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_create(const char *path, const remote::file_mode &mode,
                                              const remote::open_flags &flags,
                                              remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  const auto res = open(file_path.c_str(), remote::create_os_open_flags(flags), mode);
  if (res >= 0) {
    handle = res;
    set_open_info(res, open_info{0, "", nullptr, file_path});
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_destroy() {
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, "", 0);
  return 0;
}

/*packet::error_type remote_server::fuse_fallocate(const char *path, const std::int32_t &mode,
                                       const remote::file_offset &offset,
                                       const remote::file_offset &length,
                                       const remote::file_handle &handle) {
  const auto file_path = ConstructPath(path);
  auto ret = HasOpenFileInfo(handle, -EBADF);
  if (ret == 0) {
#ifdef __APPLE__
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

      const auto res = fcntl(static_cast<int>(handle), F_PREALLOCATE, &fstore);
      ret = ((res < 0) ? -errno : 0);
    }
#else
    const auto res = fallocate(static_cast<int>(handle), mode, offset, length);
    ret = ((res < 0) ? -errno : 0);
#endif
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}*/

packet::error_type remote_server::fuse_fgetattr(const char *path, remote::stat &st, bool &directory,
                                                const remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  memset(&st, 0, sizeof(remote::stat));

  auto res = has_open_info(handle, EBADF);
  if (res == 0) {
    directory = utils::file::is_directory(file_path);
    struct stat st1 {};
    if ((res = fstat(static_cast<int>(handle), &st1)) == 0) {
      populate_stat(st1, st);
    }
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_fsetattr_x(const char *path, const remote::setattr_x &attr,
                                                  const remote::file_handle &handle) {
  const auto file_path = construct_path(path);
#ifdef __APPLE__
  auto res = 0;
  if (SETATTR_WANTS_MODE(&attr)) {
    res = lchmod(file_path.c_str(), attr.mode);
  }

  if (res >= 0) {
    uid_t uid = -1;
    gid_t gid = -1;
    if (SETATTR_WANTS_UID(&attr)) {
      uid = attr.uid;
    }

    if (SETATTR_WANTS_GID(&attr)) {
      gid = attr.gid;
    }

    if ((uid != -1) || (gid != -1)) {
      res = lchown(file_path.c_str(), uid, gid);
    }
  }

  if (res >= 0) {
    if (SETATTR_WANTS_SIZE(&attr)) {
      res = (handle == REPERTORY_INVALID_HANDLE) ? truncate(file_path.c_str(), attr.size)
                                                 : ftruncate(handle, attr.size);
    }
  }

  if (res >= 0) {
    if (SETATTR_WANTS_BKUPTIME(&attr)) {
      struct attrlist attributes {};
      attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
      attributes.reserved = 0;
      attributes.commonattr = ATTR_CMN_BKUPTIME;
      attributes.dirattr = 0;
      attributes.fileattr = 0;
      attributes.forkattr = 0;
      attributes.volattr = 0;

      struct timespec bkuptime {};
      bkuptime.tv_sec = attr.bkuptime / NANOS_PER_SECOND;
      bkuptime.tv_nsec = attr.bkuptime % NANOS_PER_SECOND;
      res = setattrlist(file_path.c_str(), &attributes, &bkuptime, sizeof(struct timespec),
                        FSOPT_NOFOLLOW);
    }
  }

  if (res >= 0) {
    if (SETATTR_WANTS_CHGTIME(&attr)) {
      struct attrlist attributes {};
      attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
      attributes.reserved = 0;
      attributes.commonattr = ATTR_CMN_CHGTIME;
      attributes.dirattr = 0;
      attributes.fileattr = 0;
      attributes.forkattr = 0;
      attributes.volattr = 0;

      struct timespec chgtime {};
      chgtime.tv_sec = attr.chgtime / NANOS_PER_SECOND;
      chgtime.tv_nsec = attr.chgtime % NANOS_PER_SECOND;
      res = setattrlist(file_path.c_str(), &attributes, &chgtime, sizeof(struct timespec),
                        FSOPT_NOFOLLOW);
    }
  }

  if (res >= 0) {
    if (SETATTR_WANTS_CRTIME(&attr)) {
      struct attrlist attributes {};
      attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
      attributes.reserved = 0;
      attributes.commonattr = ATTR_CMN_CRTIME;
      attributes.dirattr = 0;
      attributes.fileattr = 0;
      attributes.forkattr = 0;
      attributes.volattr = 0;

      struct timespec crtime {};
      crtime.tv_sec = attr.crtime / NANOS_PER_SECOND;
      crtime.tv_nsec = attr.crtime % NANOS_PER_SECOND;
      res = setattrlist(file_path.c_str(), &attributes, &crtime, sizeof(struct timespec),
                        FSOPT_NOFOLLOW);
    }
  }

  if (res >= 0) {
    if (SETATTR_WANTS_MODTIME(&attr)) {
      struct timeval tv[2];
      if (!SETATTR_WANTS_ACCTIME(&attr)) {
        gettimeofday(&tv[0], nullptr);
      } else {
        tv[0].tv_sec = attr.acctime / NANOS_PER_SECOND;
        tv[0].tv_usec = (attr.acctime % NANOS_PER_SECOND) / 1000;
      }
      tv[0].tv_sec = attr.acctime / NANOS_PER_SECOND;
      tv[0].tv_usec = (attr.acctime % NANOS_PER_SECOND) / 1000;
      res = utimes(file_path.c_str(), tv);
    }
  }

  if (res >= 0) {
    if (SETATTR_WANTS_FLAGS(&attr)) {
      res = chflags(file_path.c_str(), attr.flags);
    }
  }

  const auto ret = ((res < 0) ? -errno : 0);
#else
  const auto ret = STATUS_NOT_IMPLEMENTED;
#endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_fsync(const char *path, const std::int32_t &datasync,
                                             const remote::file_handle &handle) {
  const auto file_path = construct_path(path);

  auto res = has_open_info(handle, EBADF);
  if (res == 0) {
#ifdef __APPLE__
    res = datasync ? fcntl(static_cast<int>(handle), F_FULLFSYNC) : fsync(static_cast<int>(handle));
#else
    res = datasync ? fdatasync(static_cast<int>(handle)) : fsync(static_cast<int>(handle));
#endif
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_ftruncate(const char *path, const remote::file_offset &size,
                                                 const remote::file_handle &handle) {
  const auto file_path = construct_path(path);

  auto res = has_open_info(handle, EBADF);
  if (res == 0) {
    res = ftruncate(static_cast<int>(handle), size);
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_getattr(const char *path, remote::stat &st,
                                               bool &directory) {
  const auto api_path = utils::path::create_api_path(path);
  const auto file_path = construct_path(api_path);
  const auto parent_api_path = utils::path::get_parent_api_path(api_path);
  memset(&st, 0, sizeof(remote::stat));

  auto res = -1;
  errno = EACCES;
  if (drive_.check_parent_access(api_path, X_OK) == api_error::success) {
    res = errno = 0;

    auto found = false;
    directory_cache_.execute_action(
        parent_api_path, [&](directory_iterator &directoryItemIterator) {
          directory_item di{};
          if (directoryItemIterator.get_directory_item(api_path, di) == api_error::success) {
            directory = di.directory;
            drive_.update_directory_item(di);

            struct stat st1 {};
            drive_.populate_stat(di, st1);

            populate_stat(st1, st);
            found = true;
          }
        });

    if (not found) {
      directory = utils::file::is_directory(file_path);
      struct stat st1 {};
      if ((res = stat(file_path.c_str(), &st1)) == 0) {
        populate_stat(st1, st);
      }
    }
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

/*packet::error_type remote_server::fuse_getxattr(const char *path, const char *name, char
*value, const remote::file_size &size) { const auto api_path =
utils::path::create_api_path(path); const auto file_path = ConstructPath(api_path); const
auto parentApiPath = utils::path::get_parent_api_path(api_path);

#if __APPLE__ || !HAS_SETXATTR
  const auto ret = STATUS_NOT_IMPLEMENTED;
#else
  auto found = false;
  auto res = -1;
  errno = EACCES;
  if (drive_.CheckParentAccess(api_path, X_OK) == api_error::success) {
    res = errno = 0;
    directoryCacheManager_.execute_action(
        parentApiPath, [&](directory_iterator &directoryItemIterator) {
          directory_item directoryItem{};
          if ((found = (directoryItemIterator.Getdirectory_item(api_path, directoryItem) ==
                        api_error::success))) {
            drive_.Updatedirectory_item(directoryItem);
            res = -ENODATA;
            if (directoryItem.MetaMap.find(name) != directoryItem.MetaMap.end()) {
              const auto data = macaron::Base64::Decode(directoryItem.MetaMap[name]);
              res = static_cast<int>(data.size());
              if (size) {
                res = -ERANGE;
                if (size >= data.size()) {
                  memcpy(value, &data[0], data.size());
                  res = 0;
                }
              }
            }
          }
        });

    if (not found) {
      res = getxattr(file_path.c_str(), name, value, size);
    }
  }
  const auto ret = found ? res : (((res < 0) ? -errno : 0));
#endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__,
                                 name ? std::string(name) + ":" + file_path : filePath, ret);
  return ret;
}

packet::error_type remote_server::fuse_getxattrOSX(const char *path, const char *name, char
*value, const remote::file_size &size, const std::uint32_t &position) { const auto file_path =
ConstructPath(path); #if __APPLE__ && HAS_SETXATTR
  // TODO: CheckParentAccess(api_path, X_OK)
  // TODO: Use iterator cache
  const auto res = getxattr(file_path.c_str(), name, value, size, position, FSOPT_NOFOLLOW);
  const auto ret = ((res < 0) ? -errno : 0);
#else
  const auto ret = STATUS_NOT_IMPLEMENTED;
#endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}*/

packet::error_type remote_server::fuse_getxtimes(const char *path, remote::file_time &bkuptime,
                                                 remote::file_time &crtime) {
  const auto file_path = construct_path(path);
#ifdef __APPLE__
  struct attrlist attributes {};
  attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
  attributes.reserved = 0;
  attributes.commonattr = 0;
  attributes.dirattr = 0;
  attributes.fileattr = 0;
  attributes.forkattr = 0;
  attributes.volattr = 0;

  struct xtimeattrbuf {
    uint32_t size;
    struct timespec xtime;
  } __attribute__((packed));
  struct xtimeattrbuf buf {};

  attributes.commonattr = ATTR_CMN_BKUPTIME;
  auto res = getattrlist(file_path.c_str(), &attributes, &buf, sizeof(buf), FSOPT_NOFOLLOW);
  if (res == 0) {
    struct timespec ts {};
    memcpy(&ts, &(buf.xtime), sizeof(struct timespec));
    bkuptime = ts.tv_nsec + (ts.tv_sec * NANOS_PER_SECOND);

    attributes.commonattr = ATTR_CMN_CRTIME;
    res = getattrlist(file_path.c_str(), &attributes, &buf, sizeof(buf), FSOPT_NOFOLLOW);
    if (res == 0) {
      memcpy(&ts, &(buf.xtime), sizeof(struct timespec));
      crtime = ts.tv_nsec + (ts.tv_sec * NANOS_PER_SECOND);
    }
  }

  const auto ret = ((res < 0) ? -errno : 0);
#else
  const auto ret = STATUS_NOT_IMPLEMENTED;
#endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_init() {
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, "", 0);
  return 0;
}

/*packet::error_type remote_server::fuse_listxattr(const char *path, char *buffer,
                                       const remote::file_size &size) {
  const auto file_path = ConstructPath(path);
#ifdef HAS_SETXATTR
#ifdef __APPLE__
  const auto res = listxattr(file_path.c_str(), buffer, size, FSOPT_NOFOLLOW);
#else
  const auto res = listxattr(file_path.c_str(), buffer, size);
#endif
  const auto ret = ((res < 0) ? -errno : 0);
#else
  const auto ret = STATUS_NOT_IMPLEMENTED;
#endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}*/

packet::error_type remote_server::fuse_mkdir(const char *path, const remote::file_mode &mode) {
  const auto file_path = construct_path(path);
  const auto res = mkdir(file_path.c_str(), mode);
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_open(const char *path, const remote::open_flags &flags,
                                            remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  const auto res = open(file_path.c_str(), remote::create_os_open_flags(flags));
  if (res >= 0) {
    handle = res;
    set_open_info(res, open_info{0, "", nullptr, file_path});
  }
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_opendir(const char *path, remote::file_handle &handle) {
  const auto file_path = construct_path(path);

  auto res = -1;
  errno = ENOENT;

  if (utils::file::is_directory(file_path)) {
    auto list = drive_.get_directory_items(utils::path::create_api_path(path));
    auto *iterator = new directory_iterator(std::move(list));

    directory_cache_.set_directory(path, iterator);

    handle = reinterpret_cast<remote::file_handle>(iterator);
    res = 0;
    errno = 0;
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_read(const char *path, char *buffer,
                                            const remote::file_size &read_size,
                                            const remote::file_offset &read_offset,
                                            const remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  auto &b = *reinterpret_cast<std::vector<char> *>(buffer);
  auto res = has_open_info(handle, EBADF);
  if (res == 0) {
    b.resize(read_size);
    res = pread64(static_cast<int>(handle), &b[0], read_size, read_offset);
  }

  const auto ret = ((res < 0) ? -errno : res);
  if (ret < 0) {
    RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  }
  return ret;
}

packet::error_type remote_server::fuse_rename(const char *from, const char *to) {
  const auto from_path = utils::path::combine(mount_location_, {from});
  const auto to_path = utils::path::combine(mount_location_, {to});
  const auto res = rename(from_path.c_str(), to_path.c_str());
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, from + std::string("|") + to, ret);
  return ret;
}

packet::error_type remote_server::fuse_readdir(const char *path, const remote::file_offset &offset,
                                               const remote::file_handle &handle,
                                               std::string &itemPath) {
  const auto file_path = construct_path(path);
  auto res = 0;
  if (offset > std::numeric_limits<std::size_t>::max()) {
    errno = ERANGE;
    res = -1;
  } else {
    auto *iterator = reinterpret_cast<directory_iterator *>(handle);
    if (iterator) {
      res = iterator->get(static_cast<std::size_t>(offset), itemPath);
    } else {
      res = -1;
      errno = EFAULT;
    }
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_release(const char *path,
                                               const remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  const auto res = close(static_cast<int>(handle));
  remove_open_info(handle);

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_releasedir(const char *path,
                                                  const remote::file_handle &handle) {
  const auto file_path = construct_path(path);

  auto *iterator = reinterpret_cast<directory_iterator *>(handle);
  directory_cache_.remove_directory(iterator);

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, 0);
  return 0;
}

/*packet::error_type remote_server::fuse_removexattr(const char *path, const char *name) {
  const auto file_path = ConstructPath(path);
#ifdef HAS_SETXATTR
#ifdef __APPLE__
  const auto res = removexattr(file_path.c_str(), name, FSOPT_NOFOLLOW);
#else
  const auto res = removexattr(file_path.c_str(), name);
#endif
  const auto ret = ((res < 0) ? -errno : 0);
#else
  const auto ret = STATUS_NOT_IMPLEMENTED;
#endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}*/

packet::error_type remote_server::fuse_rmdir(const char *path) {
  const auto file_path = construct_path(path);
  const auto res = rmdir(file_path.c_str());
  if (res == 0) {
    directory_cache_.remove_directory(utils::path::create_api_path(path));
  }
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_setattr_x(const char *path, remote::setattr_x &attr) {
  const auto file_path = construct_path(path);
#ifdef __APPLE__
  const auto ret = fuse_fsetattr_x(path, attr, REPERTORY_INVALID_HANDLE);
#else
  const auto ret = STATUS_NOT_IMPLEMENTED;
#endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_setbkuptime(const char *path,
                                                   const remote::file_time &bkuptime) {
  const auto file_path = construct_path(path);
#ifdef __APPLE__
  struct attrlist attributes {};
  attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
  attributes.reserved = 0;
  attributes.commonattr = ATTR_CMN_BKUPTIME;
  attributes.dirattr = 0;
  attributes.fileattr = 0;
  attributes.forkattr = 0;
  attributes.volattr = 0;

  struct timespec ts {};
  ts.tv_nsec = bkuptime % NANOS_PER_SECOND;
  ts.tv_sec = bkuptime / NANOS_PER_SECOND;
  const auto res = setattrlist(file_path.c_str(), &attributes, (void *)&ts, sizeof(struct timespec),
                               FSOPT_NOFOLLOW);
  const auto ret = ((res < 0) ? -errno : 0);
#else
  const auto ret = STATUS_NOT_IMPLEMENTED;
#endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_setchgtime(const char *path,
                                                  const remote::file_time &chgtime) {
  const auto file_path = construct_path(path);
#ifdef __APPLE__
  struct attrlist attributes {};
  attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
  attributes.reserved = 0;
  attributes.commonattr = ATTR_CMN_BKUPTIME;
  attributes.dirattr = 0;
  attributes.fileattr = 0;
  attributes.forkattr = 0;
  attributes.volattr = 0;

  struct timespec ts {};
  ts.tv_nsec = chgtime % NANOS_PER_SECOND;
  ts.tv_sec = chgtime / NANOS_PER_SECOND;
  const auto res = setattrlist(file_path.c_str(), &attributes, (void *)&ts, sizeof(struct timespec),
                               FSOPT_NOFOLLOW);
  const auto ret = ((res < 0) ? -errno : 0);
#else
  const auto ret = STATUS_NOT_IMPLEMENTED;
#endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_setcrtime(const char *path,
                                                 const remote::file_time &crtime) {
  const auto file_path = construct_path(path);
#ifdef __APPLE__
  struct attrlist attributes {};
  attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
  attributes.reserved = 0;
  attributes.commonattr = ATTR_CMN_BKUPTIME;
  attributes.dirattr = 0;
  attributes.fileattr = 0;
  attributes.forkattr = 0;
  attributes.volattr = 0;

  struct timespec ts {};
  ts.tv_nsec = crtime % NANOS_PER_SECOND;
  ts.tv_sec = crtime / NANOS_PER_SECOND;
  const auto res = setattrlist(file_path.c_str(), &attributes, (void *)&ts, sizeof(struct timespec),
                               FSOPT_NOFOLLOW);
  const auto ret = ((res < 0) ? -errno : 0);
#else
  const auto ret = STATUS_NOT_IMPLEMENTED;
#endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_setvolname(const char *volname) {
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, volname, 0);
  return 0;
}

/*packet::error_type remote_server::fuse_setxattr(const char *path, const char *name, const
char *value, const remote::file_size &size, const std::int32_t &flags) { const auto file_path =
ConstructPath(path); #if __APPLE__ || !HAS_SETXATTR const auto ret = STATUS_NOT_IMPLEMENTED; #else
  const auto res = setxattr(file_path.c_str(), name, value, size, flags);
  const auto ret = ((res < 0) ? -errno : 0);
#endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_setxattrOSX(const char *path, const char *name, const
char *value, const remote::file_size &size, const std::int32_t &flags, const std::uint32_t
&position) { const auto file_path = ConstructPath(path); #if __APPLE__ && HAS_SETXATTR const auto
res = setxattr(file_path.c_str(), name, value, size, position, flags); const auto ret = ((res < 0) ?
-errno : 0); #else const auto ret = STATUS_NOT_IMPLEMENTED; #endif
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}*/

packet::error_type remote_server::fuse_statfs(const char *path, const std::uint64_t &frsize,
                                              remote::statfs &st) {
  const auto file_path = construct_path(path);

  const auto total_bytes = drive_.get_total_drive_space();
  const auto total_used = drive_.get_used_drive_space();
  const auto used_blocks =
      utils::divide_with_ceiling(total_used, static_cast<std::uint64_t>(frsize));
  st.f_files = 4294967295;
  st.f_blocks = utils::divide_with_ceiling(total_bytes, static_cast<std::uint64_t>(frsize));
  st.f_bavail = st.f_bfree = st.f_blocks ? (st.f_blocks - used_blocks) : 0;
  st.f_ffree = st.f_favail = st.f_files - drive_.get_total_item_count();

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, 0);
  return 0;
}

packet::error_type remote_server::fuse_statfs_x(const char *path, const std::uint64_t &bsize,
                                                remote::statfs_x &st) {
  const auto file_path = construct_path(path);

  const auto total_bytes = drive_.get_total_drive_space();
  const auto total_used = drive_.get_used_drive_space();
  const auto used_blocks =
      utils::divide_with_ceiling(total_used, static_cast<std::uint64_t>(bsize));
  st.f_files = 4294967295;
  st.f_blocks = utils::divide_with_ceiling(total_bytes, static_cast<std::uint64_t>(bsize));
  st.f_bavail = st.f_bfree = st.f_blocks ? (st.f_blocks - used_blocks) : 0;
  st.f_ffree = st.f_favail = st.f_files - drive_.get_total_item_count();
  strncpy(&st.f_mntfromname[0], (utils::create_volume_label(config_.get_provider_type())).c_str(),
          1024);

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, 0);
  return 0;
}

packet::error_type remote_server::fuse_truncate(const char *path, const remote::file_offset &size) {
  const auto file_path = construct_path(path);
  const auto res = truncate(file_path.c_str(), size);
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_unlink(const char *path) {
  const auto file_path = construct_path(path);
  const auto res = unlink(file_path.c_str());
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_utimens(const char *path, const remote::file_time *tv,
                                               const std::uint64_t &op0, const std::uint64_t &op1) {
  const auto file_path = construct_path(path);

  struct timespec tv2[2] = {{0, 0}};
  if ((op0 == UTIME_NOW) || (op0 == UTIME_OMIT)) {
    tv2[0].tv_nsec = op0;
    tv2[0].tv_sec = 0;
  } else {
    tv2[0].tv_nsec = tv[0] % NANOS_PER_SECOND;
    tv2[0].tv_sec = tv[0] / NANOS_PER_SECOND;
  }

  if ((op1 == UTIME_NOW) || (op1 == UTIME_OMIT)) {
    tv2[1].tv_nsec = op1;
    tv2[1].tv_sec = 0;
  } else {
    tv2[1].tv_nsec = tv[1] % NANOS_PER_SECOND;
    tv2[1].tv_sec = tv[1] / NANOS_PER_SECOND;
  }

  const auto res = utimensat(0, file_path.c_str(), tv2, AT_SYMLINK_NOFOLLOW);
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_write(const char *path, const char *buffer,
                                             const remote::file_size &write_size,
                                             const remote::file_offset &write_offset,
                                             const remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  auto res = has_open_info(handle, EBADF);
  if (res == 0) {
    res = pwrite64(static_cast<int>(handle), buffer, write_size, write_offset);
  }

  const auto ret = ((res < 0) ? -errno : res);
  if (ret < 0) {
    RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  }
  return ret;
}

packet::error_type remote_server::fuse_write_base64(const char * /*path*/, const char * /*buffer*/,
                                                    const remote::file_size & /*write_size*/,
                                                    const remote::file_offset & /*write_offset*/,
                                                    const remote::file_handle & /*handle*/) {
  // DOES NOTHING
  return 0;
}

// WinFSP Layer
packet::error_type remote_server::winfsp_can_delete(PVOID file_desc, PWSTR file_name) {
  const auto relativePath = utils::string::to_utf8(file_name);
  const auto file_path = construct_path(relativePath);
  auto ret = has_open_info(reinterpret_cast<remote::file_handle>(file_desc), STATUS_INVALID_HANDLE);
  if (ret == 0) {
    ret =
        utils::file::is_directory(file_path)
            ? (drive_.get_directory_item_count(utils::path::create_api_path(relativePath))
                   ? STATUS_DIRECTORY_NOT_EMPTY
                   : STATUS_SUCCESS)
            : (drive_.is_processing(utils::path::create_api_path(relativePath)) ? STATUS_DEVICE_BUSY
                                                                                : STATUS_SUCCESS);
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::winfsp_cleanup(PVOID file_desc, PWSTR file_name, UINT32 flags,
                                                 BOOLEAN &was_closed) {
  const auto relative_path = utils::string::to_utf8(file_name);
  const auto file_path = construct_path(relative_path);
  const auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  was_closed = 0;

  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == 0) {
    if (flags & FileSystemBase::FspCleanupDelete) {
      close(handle);
      remove_open_info(handle);
      was_closed = 1;
      if (utils::file::is_directory(file_path)) {
        rmdir(file_path.c_str());
      } else {
        unlink(file_path.c_str());
      }
    } else {
      const auto api_path = utils::path::create_api_path(relative_path);
      if (flags &
          (FileSystemBase::FspCleanupSetLastAccessTime |
           FileSystemBase::FspCleanupSetLastWriteTime | FileSystemBase::FspCleanupSetChangeTime)) {
        const auto fileTimeNow = utils::get_time_now();
        if (flags & FileSystemBase::FspCleanupSetLastAccessTime) {
          drive_.set_item_meta(api_path, META_ACCESSED, std::to_string(fileTimeNow));
        }

        if (flags & FileSystemBase::FspCleanupSetLastWriteTime) {
          drive_.set_item_meta(api_path, META_WRITTEN, std::to_string(fileTimeNow));
        }

        if (flags & FileSystemBase::FspCleanupSetChangeTime) {
          drive_.set_item_meta(api_path, META_MODIFIED, std::to_string(fileTimeNow));
        }
      }

      if (flags & FileSystemBase::FspCleanupSetAllocationSize) {
      }
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::winfsp_close(PVOID file_desc) {
  const auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  if (handle != static_cast<std::uint64_t>(REPERTORY_INVALID_HANDLE)) {
    close(handle);
    remove_open_info(handle);
  }
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, get_open_file_path(handle), STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

packet::error_type remote_server::winfsp_create(PWSTR file_name, UINT32 create_options,
                                                UINT32 granted_access, UINT32 attributes,
                                                UINT64 /*allocationSize*/, PVOID *file_desc,
                                                remote::file_info *file_info,
                                                std::string &normalized_name, BOOLEAN &exists) {
  const auto relative_path = utils::string::to_utf8(file_name);
  const auto file_path = construct_path(relative_path);
  exists = utils::file::is_file(file_path);

  if (create_options & FILE_DIRECTORY_FILE) {
    attributes |= FILE_ATTRIBUTE_DIRECTORY;
  } else {
    attributes &= ~FILE_ATTRIBUTE_DIRECTORY;
  }

  if (not attributes) {
    attributes = FILE_ATTRIBUTE_NORMAL;
  }

  remote::file_mode mode = 0;
  std::uint32_t flags = 0;
  utils::windows_create_to_unix(create_options, granted_access, flags, mode);

  int res = 0;
  if (create_options & FILE_DIRECTORY_FILE) {
    if ((res = mkdir(file_path.c_str(), mode)) >= 0) {
      res = open(file_path.c_str(), flags);
    }
  } else {
    res = open(file_path.c_str(), flags, mode);
  }

  if (res >= 0) {
    *file_desc = reinterpret_cast<PVOID>(res);
    drive_.set_item_meta(construct_api_path(file_path), META_ATTRIBUTES,
                         utils::string::from_uint32(attributes));
    set_open_info(res, open_info{0, "", nullptr, file_path});

    const auto api_path = utils::path::create_api_path(relative_path);
    normalized_name = utils::string::replace_copy(api_path, '/', '\\');
    populate_file_info(api_path, 0, attributes, *file_info);
  }

  const auto ret = utils::unix_error_to_windows((res < 0) ? errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::winfsp_flush(PVOID file_desc, remote::file_info *file_info) {
  const auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == 0) {
    ret = (fsync(static_cast<int>(handle)) < 0)
              ? utils::unix_error_to_windows(errno)
              : populate_file_info(construct_api_path(get_open_file_path(handle)), *file_info);
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, get_open_file_path(handle), ret);
  return ret;
}

packet::error_type remote_server::winfsp_get_file_info(PVOID file_desc,
                                                       remote::file_info *file_info) {
  const auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == 0) {
    ret = populate_file_info(construct_api_path(get_open_file_path(handle)), *file_info);
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, get_open_file_path(handle), ret);
  return ret;
}

packet::error_type
remote_server::winfsp_get_security_by_name(PWSTR file_name, PUINT32 attributes,
                                           std::uint64_t * /*securityDescriptorSize*/,
                                           std::wstring & /*strDescriptor*/) {
  auto ret = STATUS_SUCCESS;
  const auto file_path = construct_path(file_name);
  if (utils::file::is_file(file_path) || (utils::file::is_directory(file_path))) {
    if (attributes) {
      remote::file_info file_info{};
      if ((ret = populate_file_info(construct_api_path(file_path), file_info)) == STATUS_SUCCESS) {
        *attributes = file_info.FileAttributes;
      }
    }
  } else {
    ret = STATUS_OBJECT_NAME_NOT_FOUND;
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, construct_path(file_name), ret);
  return ret;
}

packet::error_type remote_server::winfsp_get_volume_info(UINT64 &total_size, UINT64 &free_size,
                                                         std::string &volume_label) {
  drive_.get_volume_info(total_size, free_size, volume_label);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, volume_label, STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

packet::error_type remote_server::winfsp_mounted(const std::wstring &location) {
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, utils::string::to_utf8(location), STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

packet::error_type remote_server::winfsp_open(PWSTR file_name, UINT32 create_options,
                                              UINT32 granted_access, PVOID *file_desc,
                                              remote::file_info *file_info,
                                              std::string &normalized_name) {
  const auto relative_path = utils::string::to_utf8(file_name);
  const auto file_path = construct_path(relative_path);
  const auto directory = utils::file::is_directory(file_path);
  if (directory) {
    create_options |= FILE_DIRECTORY_FILE;
  }

  remote::file_mode mode = 0;
  std::uint32_t flags = 0u;
  utils::windows_create_to_unix(create_options, granted_access, flags, mode);
  flags &= ~(O_CREAT);

  const auto res = open(file_path.c_str(), flags);
  if (res >= 0) {
    *file_desc = reinterpret_cast<PVOID>(res);
    set_open_info(res, open_info{0, "", nullptr, file_path});

    const auto api_path = utils::path::create_api_path(relative_path);
    normalized_name = utils::string::replace_copy(api_path, '/', '\\');
    populate_file_info(api_path, *file_info);
  }

  const auto ret = utils::unix_error_to_windows((res < 0) ? errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::winfsp_overwrite(PVOID file_desc, UINT32 attributes,
                                                   BOOLEAN replace_attributes,
                                                   UINT64 /*allocationSize*/,
                                                   remote::file_info *file_info) {
  const auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == 0) {
    const auto api_path = construct_api_path(get_open_file_path(handle));
    const auto res = ftruncate(handle, 0);
    if (res >= 0) {
      auto set_attributes = false;
      if (replace_attributes) {
        set_attributes = true;
      } else if (attributes) {
        std::string current_attributes;
        drive_.get_item_meta(api_path, META_ATTRIBUTES, current_attributes);
        if (current_attributes.empty()) {
          current_attributes = "0";
        }

        attributes = attributes | utils::string::to_uint32(current_attributes);
        set_attributes = true;
      }

      if (set_attributes) {
        attributes &= ~(FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_NORMAL);
        if (not attributes) {
          attributes = FILE_ATTRIBUTE_NORMAL;
        }
        drive_.set_item_meta(api_path, META_ATTRIBUTES, std::to_string(attributes));
      }
      ret = populate_file_info(construct_api_path(get_open_file_path(handle)), *file_info);
    } else {
      ret = utils::unix_error_to_windows(errno);
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, get_open_file_path(handle), ret);
  return ret;
}

packet::error_type remote_server::winfsp_read(PVOID file_desc, PVOID buffer, UINT64 offset,
                                              UINT32 length, PUINT32 bytes_transferred) {
  *bytes_transferred = 0;

  const auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == 0) {
    const auto res = pread64(handle, buffer, length, offset);
    if (res >= 0) {
      *bytes_transferred = res;
    } else {
      ret = utils::unix_error_to_windows(errno);
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, get_open_file_path(handle), ret);
  return ret;
}

packet::error_type remote_server::winfsp_read_directory(PVOID file_desc, PWSTR /*pattern*/,
                                                        PWSTR marker, json &item_list) {
  item_list.clear();

  const auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == 0) {
    const auto api_path = construct_api_path(get_open_file_path(handle));
    auto list = drive_.get_directory_items(api_path);
    directory_iterator iterator(std::move(list));
    auto offset = marker ? iterator.get_next_directory_offset(utils::path::create_api_path(
                               utils::path::combine(api_path, {utils::string::to_utf8(marker)})))
                         : 0;
    json item;
    while (iterator.get_json(offset++, item) == 0) {
      try {
        item_list.emplace_back(update_to_windows_format(item));
      } catch (const std::exception &e) {
        event_system::instance().raise<repertory_exception>(
            __FUNCTION__, e.what() ? e.what() : "Exception processing directory item");
      }
    }
    ret = STATUS_SUCCESS;
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, get_open_file_path(handle), ret);
  return ret;
}

packet::error_type remote_server::winfsp_rename(PVOID /*file_desc*/, PWSTR file_name,
                                                PWSTR new_file_name, BOOLEAN replace_if_exists) {
  const auto relative_path = utils::string::to_utf8(file_name);
  const auto file_path = construct_path(relative_path);
  const auto new_relative_path = utils::string::to_utf8(new_file_name);
  const auto new_file_path = construct_path(new_relative_path);

  auto res = -1;
  errno = ENOENT;
  if (utils::file::is_file(file_path)) {
    res = drive_.rename_file(construct_api_path(file_path), construct_api_path(new_file_path),
                             !!replace_if_exists);
  } else if (utils::file::is_directory(file_path)) {
    res = drive_.rename_directory(construct_api_path(file_path), construct_api_path(new_file_path));
  }

  const auto ret = ((res < 0) ? utils::unix_error_to_windows(errno) : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path + "|" + new_file_path, ret);
  return ret;
}

packet::error_type remote_server::winfsp_set_basic_info(PVOID file_desc, UINT32 attributes,
                                                        UINT64 creation_time,
                                                        UINT64 last_access_time,
                                                        UINT64 last_write_time, UINT64 change_time,
                                                        remote::file_info *file_info) {
  const auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == 0) {
    const auto file_path = get_open_file_path(handle);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
      attributes = 0;
    } else if (attributes == 0) {
      attributes =
          utils::file::is_directory(file_path) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
    }

    const auto api_path = construct_api_path(file_path);
    api_meta_map meta;
    if (attributes) {
      if ((attributes & FILE_ATTRIBUTE_NORMAL) && (attributes != FILE_ATTRIBUTE_NORMAL)) {
        attributes &= ~(FILE_ATTRIBUTE_NORMAL);
      }
      drive_.set_item_meta(api_path, META_ATTRIBUTES, std::to_string(attributes));
    }

    if (creation_time && (creation_time != 0xFFFFFFFFFFFFFFFF)) {
      drive_.set_item_meta(api_path, META_CREATION,
                           std::to_string(utils::windows_time_to_unix_time(creation_time)));
    }

    if (last_access_time && (last_access_time != 0xFFFFFFFFFFFFFFFF)) {
      drive_.set_item_meta(api_path, META_ACCESSED,
                           std::to_string(utils::windows_time_to_unix_time(last_access_time)));
    }

    if (last_write_time && (last_write_time != 0xFFFFFFFFFFFFFFFF)) {
      drive_.set_item_meta(api_path, META_WRITTEN,
                           std::to_string(utils::windows_time_to_unix_time(last_write_time)));
    }

    if (change_time && (change_time != 0xFFFFFFFFFFFFFFFF)) {
      drive_.set_item_meta(api_path, META_MODIFIED,
                           std::to_string(utils::windows_time_to_unix_time(change_time)));
    }

    ret = populate_file_info(api_path, *file_info);
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, get_open_file_path(handle), ret);
  return ret;
}

packet::error_type remote_server::winfsp_set_file_size(PVOID file_desc, UINT64 new_size,
                                                       BOOLEAN set_allocation_size,
                                                       remote::file_info *file_info) {
  const auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == 0) {
    const auto res = set_allocation_size ? 0 : ftruncate(handle, new_size);
    ret = ((res < 0) ? utils::unix_error_to_windows(errno) : 0);
    if (ret == 0) {
      ret = populate_file_info(construct_api_path(get_open_file_path(handle)), *file_info);
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, get_open_file_path(handle), ret);
  return ret;
}

packet::error_type remote_server::winfsp_unmounted(const std::wstring &location) {
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, utils::string::to_utf8(location), STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

packet::error_type remote_server::winfsp_write(PVOID file_desc, PVOID buffer, UINT64 offset,
                                               UINT32 length, BOOLEAN write_to_end,
                                               BOOLEAN constrained_io, PUINT32 bytes_transferred,
                                               remote::file_info *file_info) {
  *bytes_transferred = 0;
  const auto handle = reinterpret_cast<remote::file_handle>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == 0) {
    const auto api_path = construct_api_path(get_open_file_path(handle));
    const auto file_size = drive_.get_file_size(api_path);
    if (write_to_end) {
      offset = file_size;
    }

    auto should_write = true;
    if (constrained_io) {
      if (offset >= file_size) {
        ret = STATUS_SUCCESS;
        should_write = false;
      } else if ((offset + length) > file_size) {
        length = static_cast<UINT64>(file_size - offset);
      }
    }

    if (should_write) {
      if (length > 0) {
        const auto res = pwrite64(handle, buffer, length, offset);
        if (res >= 0) {
          *bytes_transferred = res;
          ret = populate_file_info(construct_api_path(get_open_file_path(handle)), *file_info);
        } else {
          ret = utils::unix_error_to_windows(errno);
        }
      }
    }
  }

  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, get_open_file_path(handle), ret);
  return ret;
}

packet::error_type remote_server::json_create_directory_snapshot(const std::string &path,
                                                                 json &json_data) {
  const auto api_path = utils::path::create_api_path(path);
  const auto file_path = construct_path(api_path);

  auto res = -1;
  errno = ENOENT;

  if (utils::file::is_directory(file_path)) {
    auto list = drive_.get_directory_items(api_path);
    auto *iterator = new directory_iterator(std::move(list));

    directory_cache_.set_directory(api_path, iterator);

    json_data["handle"] = reinterpret_cast<remote::file_handle>(iterator);
    json_data["path"] = path;
    json_data["page_count"] =
        utils::divide_with_ceiling(iterator->get_count(), REPERTORY_DIRECTORY_PAGE_SIZE);
    res = 0;
    errno = 0;
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::json_read_directory_snapshot(const std::string &path,
                                                               const remote::file_handle &handle,
                                                               const std::uint32_t &page,
                                                               json &json_data) {
  const auto file_path = construct_path(path);
  auto *iterator = reinterpret_cast<directory_iterator *>(handle);
  std::size_t offset = 0u;
  int res;
  json item_json;
  while ((json_data["directory_list"].size() < REPERTORY_DIRECTORY_PAGE_SIZE) &&
         (res = iterator->get_json((page * REPERTORY_DIRECTORY_PAGE_SIZE) + offset++, item_json)) ==
             0) {
    json_data["directory_list"].emplace_back(item_json);
  }
  json_data["handle"] = reinterpret_cast<remote::file_handle>(iterator);
  json_data["path"] = path;
  json_data["page"] = page;
  json_data["page_count"] =
      utils::divide_with_ceiling(iterator->get_count(), REPERTORY_DIRECTORY_PAGE_SIZE);
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type
remote_server::json_release_directory_snapshot(const std::string &path,
                                               const remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  directory_cache_.remove_directory(reinterpret_cast<directory_iterator *>(handle));
  RAISE_REMOTE_FUSE_SERVER_EVENT(__FUNCTION__, file_path, 0);
  return 0;
}

json &remote_server::update_to_windows_format(json &item) {
  const auto api_path = item["path"].get<std::string>();
  if (item["meta"][META_WRITTEN].empty() ||
      (item["meta"][META_WRITTEN].get<std::string>() == "0") ||
      (item["meta"][META_WRITTEN].get<std::string>() == "116444736000000000")) {
    drive_.set_item_meta(api_path, META_WRITTEN, item["meta"][META_MODIFIED].get<std::string>());
    item["meta"][META_WRITTEN] = std::to_string(utils::unix_time_to_windows_time(
        utils::string::to_uint64(empty_as_zero(item["meta"][META_MODIFIED]))));
  } else {
    item["meta"][META_WRITTEN] = std::to_string(utils::unix_time_to_windows_time(
        utils::string::to_uint64(empty_as_zero(item["meta"][META_WRITTEN]))));
  }
  item["meta"][META_MODIFIED] = std::to_string(utils::unix_time_to_windows_time(
      utils::string::to_uint64(empty_as_zero(item["meta"][META_MODIFIED]))));
  item["meta"][META_CREATION] = std::to_string(utils::unix_time_to_windows_time(
      utils::string::to_uint64(empty_as_zero(item["meta"][META_CREATION]))));
  item["meta"][META_ACCESSED] = std::to_string(utils::unix_time_to_windows_time(
      utils::string::to_uint64(empty_as_zero(item["meta"][META_ACCESSED]))));
  if (item["meta"][META_ATTRIBUTES].empty()) {
    item["meta"][META_ATTRIBUTES] = item["directory"].get<bool>()
                                        ? utils::string::from_uint32(FILE_ATTRIBUTE_DIRECTORY)
                                        : utils::string::from_uint32(FILE_ATTRIBUTE_NORMAL);
    drive_.set_item_meta(api_path, META_ATTRIBUTES,
                         item["meta"][META_ATTRIBUTES].get<std::string>());
  }

  return item;
}
} // namespace repertory::remote_fuse
#endif // _WIN32
