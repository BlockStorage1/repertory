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
#ifdef _WIN32

#include "drives/winfsp/remotewinfsp/remote_server.hpp"
#include "comm/packet/client_pool.hpp"
#include "comm/packet/packet_server.hpp"
#include "app_config.hpp"
#include "drives/directory_iterator.hpp"
#include "drives/fuse/remotefuse/i_remote_instance.hpp"
#include "drives/remote/remote_open_file_table.hpp"
#include "drives/winfsp/remotewinfsp/i_remote_instance.hpp"
#include "events/events.hpp"
#include "events/event_system.hpp"
#include "types/remote.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"

namespace repertory::remote_winfsp {
#define RAISE_REMOTE_WINFSP_SERVER_EVENT(func, file, ret)                                          \
  if (config_.get_enable_drive_events() &&                                                         \
      (((config_.get_event_level() >= RemoteWinFSPServerEvent::level) &&                           \
        (ret != STATUS_SUCCESS)) ||                                                                \
       (config_.get_event_level() >= event_level::verbose)))                                       \
  event_system::instance().raise<RemoteWinFSPServerEvent>(func, file, ret)

// clang-format off
E_SIMPLE3(RemoteWinFSPServerEvent, debug, true, 
  std::string, Function, FUNC, E_STRING, 
  std::string, ApiFilePath, AP, E_STRING, 
  packet::error_type, Result, RES, E_FROM_INT32
);
// clang-format on

std::string remote_server::construct_path(std::string path) {
  path = utils::path::combine(mount_location_, {path});
  if (path == mount_location_) {
    path += '\\';
  }

  return path;
}

packet::error_type remote_server::populate_file_info(const std::string &api_path,
                                                     remote::file_info &file_info) {
  return (drive_.populate_file_info(api_path, file_info) == api_error::success)
             ? STATUS_SUCCESS
             : STATUS_OBJECT_NAME_NOT_FOUND;
}

void remote_server::populate_stat(const char *path, const bool &directory, remote::stat &st,
                                  const struct _stat64 &st1) {
  st.st_nlink = static_cast<remote::file_nlink>(
      directory ? 2 + drive_.get_directory_item_count(utils::path::create_api_path(path)) : 1);
  st.st_atimespec = utils::time64_to_unix_time(st1.st_atime);
  st.st_birthtimespec = utils::time64_to_unix_time(st1.st_ctime);
  st.st_ctimespec = utils::time64_to_unix_time(st1.st_ctime);
  st.st_mtimespec = utils::time64_to_unix_time(st1.st_mtime);
  st.st_size = st1.st_size;
  st.st_mode = st1.st_mode;
}

// FUSE Layer
packet::error_type remote_server::fuse_access(const char *path, const std::int32_t &mask) {
  const auto file_path = construct_path(path);
  const auto windows_mask = utils::unix_access_mask_to_windows(mask);
  const auto res = _access(file_path.c_str(), windows_mask);
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_chflags(const char *path, const std::uint32_t &flags) {
  const auto file_path = construct_path(path);
  const auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_chmod(const char *path, const remote::file_mode &mode) {
  const auto file_path = construct_path(path);
  const auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_chown(const char *path, const remote::user_id &uid,
                                             const remote::group_id &gid) {
  const auto file_path = construct_path(path);
  const auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_destroy() {
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, "", 0);
  return 0;
}

/*packet::error_type remote_server::fuse_fallocate(const char *path, const std::int32_t &mode,
                                       const remote::file_offset &offset,
                                       const remote::file_offset &length,
                                       const remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  auto res = HasOpenFileCompatInfo(handle, EBADF);
  if (res == 0) {
    res = _chsize_s(static_cast<int>(handle), offset + length);
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}*/

packet::error_type remote_server::fuse_fgetattr(const char *path, remote::stat &st, bool &directory,
                                                const remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  memset(&st, 0, sizeof(remote::stat));

  auto res = has_compat_open_info(handle, EBADF);
  if (res == 0) {
    directory = utils::file::is_directory(file_path);
    struct _stat64 st1 {};
    if ((res = _fstat64(static_cast<int>(handle), &st1)) == 0) {
      populate_stat(path, directory, st, st1);
    }
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_fsetattr_x(const char *path, const remote::setattr_x &attr,
                                                  const remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  const auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_fsync(const char *path, const std::int32_t &datasync,
                                             const remote::file_handle &handle) {
  const auto file_path = construct_path(path);

  auto res = has_compat_open_info(handle, EBADF);
  if (res == 0) {
    res = -1;
    errno = EBADF;
    auto os_handle = reinterpret_cast<HANDLE>(_get_osfhandle(static_cast<int>(handle)));
    if (os_handle != INVALID_HANDLE_VALUE) {
      errno = EFAULT;
      if (::FlushFileBuffers(os_handle)) {
        res = 0;
        errno = 0;
      }
    }
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_ftruncate(const char *path, const remote::file_offset &size,
                                                 const remote::file_handle &handle) {
  const auto file_path = construct_path(path);

  auto res = has_compat_open_info(handle, EBADF);
  if (res == 0) {
    res = -1;
    errno = EBADF;
    auto os_handle = reinterpret_cast<HANDLE>(_get_osfhandle(static_cast<int>(handle)));
    if (os_handle != INVALID_HANDLE_VALUE) {
      errno = EFAULT;
      FILE_END_OF_FILE_INFO fi{};
      fi.EndOfFile.QuadPart = size;
      if (::SetFileInformationByHandle(os_handle, FileEndOfFileInfo, &fi,
                                       sizeof(FILE_END_OF_FILE_INFO))) {
        res = 0;
        errno = 0;
      }
    }
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_getattr(const char *path, remote::stat &st,
                                               bool &directory) {
  const auto file_path = construct_path(path);
  memset(&st, 0, sizeof(remote::stat));

  directory = utils::file::is_directory(file_path);

  struct _stat64 st1 {};
  const auto res = _stat64(file_path.c_str(), &st1);
  if (res == 0) {
    populate_stat(path, directory, st, st1);
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

/*packet::error_type remote_server::fuse_getxattr(const char *path, const char *name, char
*value, const remote::file_size &size) { const auto file_path = construct_path(path); const auto
ret = STATUS_NOT_IMPLEMENTED; RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret); return
ret;
}

packet::error_type remote_server::fuse_getxattr_osx(const char *path, const char *name, char
*value, const remote::file_size &size, const std::uint32_t &position) { const auto file_path =
construct_path(path); const auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}*/

packet::error_type remote_server::fuse_getxtimes(const char *path, remote::file_time &bkuptime,
                                                 remote::file_time &crtime) {
  const auto file_path = construct_path(path);
  const auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_init() {
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, "", 0);
  return 0;
}

/*packet::error_type remote_server::fuse_listxattr(const char *path, char *buffer,
                                       const remote::file_size &size) {
  const auto file_path = construct_path(path);
  const auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}*/

packet::error_type remote_server::fuse_mkdir(const char *path, const remote::file_mode &mode) {
  const auto file_path = construct_path(path);
  const auto res = _mkdir(file_path.c_str());
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_opendir(const char *path, remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  const auto unicode_file_path = utils::string::from_utf8(file_path);
  auto res = -1;
  errno = ENOENT;

  if (::PathIsDirectoryW(unicode_file_path.c_str())) {
    auto list = drive_.get_directory_items(utils::path::create_api_path(path));
    handle = static_cast<remote::file_handle>(
        reinterpret_cast<std::uintptr_t>(new directory_iterator(std::move(list))));
    res = 0;
    errno = 0;
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_create(const char *path, const remote::file_mode &mode,
                                              const remote::open_flags &flags,
                                              remote::file_handle &handle) {
  auto ret = -1;

  const auto file_path = construct_path(path);
  const auto directory_op =
      ((flags & remote::open_flags::directory) == remote::open_flags::directory);
  std::int32_t perms = 0;
  const auto open_flags = utils::unix_open_flags_to_flags_and_perms(mode, flags, perms);
  const auto create_op = (open_flags & _O_CREAT);

  if (create_op && directory_op) {
    ret = -EINVAL;
  } else if (directory_op) {
    ret = -EACCES;
  } else {
    int fd = -1;
    const auto res = _sopen_s(&fd, file_path.c_str(), open_flags, _SH_DENYNO, perms);
    if (res == 0) {
      handle = fd;
      ret = 0;
      set_compat_open_info(handle);
    } else {
      ret = -errno;
    }
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_open(const char *path, const remote::open_flags &flags,
                                            remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  auto res = -1;

  if ((flags & remote::open_flags::directory) == remote::open_flags::directory) {
    errno = EACCES;
  } else {
    std::int32_t perms = 0;
    const auto open_flags =
        utils::unix_open_flags_to_flags_and_perms(0, flags, perms) & (~_O_CREAT);
    int fd = -1;
    res = _sopen_s(&fd, file_path.c_str(), open_flags, _SH_DENYNO, 0);
    if (res == 0) {
      handle = fd;
      set_compat_open_info(handle);
    } else {
      res = -1;
    }
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_read(const char *path, char *buffer,
                                            const remote::file_size &readSize,
                                            const remote::file_offset &readOffset,
                                            const remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  auto &b = *reinterpret_cast<std::vector<char> *>(buffer);
  auto res = 0;
  if (readSize > std::numeric_limits<std::size_t>::max()) {
    res = -1;
    errno = ERANGE;
  } else if ((res = has_compat_open_info(handle, EBADF)) == 0) {
    res = -1;
    errno = EBADF;
    auto os_handle = reinterpret_cast<HANDLE>(_get_osfhandle(static_cast<int>(handle)));
    if (os_handle != INVALID_HANDLE_VALUE) {
      errno = EFAULT;

      auto file = native_file::attach(os_handle);
      std::uint64_t file_size = 0u;
      if (file->get_file_size(file_size)) {
        b.resize(
            utils::calculate_read_size(file_size, static_cast<std::size_t>(readSize), readOffset));
        if (b.size() > 0) {
          std::size_t bytes_read = 0u;
          if (file->read_bytes(&b[0u], b.size(), readOffset, bytes_read)) {
            res = 0;
            errno = 0;
          }
        } else {
          res = 0;
          errno = 0;
        }
      }
    }
  }

  const auto ret = ((res < 0) ? -errno : static_cast<int>(b.size()));
  if (ret < 0) {
    RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  }
  return ret;
}

packet::error_type remote_server::fuse_rename(const char *from, const char *to) {
  const auto from_path = utils::path::combine(mount_location_, {from});
  const auto to_path = utils::path::combine(mount_location_, {to});
  const auto res = rename(from_path.c_str(), to_path.c_str());

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, from + std::string("|") + to, ret);
  return ret;
}

packet::error_type remote_server::fuse_write(const char *path, const char *buffer,
                                             const remote::file_size &write_size,
                                             const remote::file_offset &write_offset,
                                             const remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  std::size_t bytes_written = 0u;
  auto res = 0;
  if (write_size > std::numeric_limits<std::size_t>::max()) {
    res = -1;
    errno = ERANGE;
  } else {
    if ((res = has_compat_open_info(handle, EBADF)) == 0) {
      res = -1;
      errno = EBADF;
      auto os_handle = reinterpret_cast<HANDLE>(_get_osfhandle(static_cast<int>(handle)));
      if (os_handle != INVALID_HANDLE_VALUE) {
        errno = EFAULT;
        if ((write_size == 0) ||
            native_file::attach(os_handle)->write_bytes(
                buffer, static_cast<std::size_t>(write_size), write_offset, bytes_written)) {
          res = 0;
          errno = 0;
        }
      }
    }
  }

  const auto ret = ((res < 0) ? -errno : static_cast<int>(bytes_written));
  if (ret < 0) {
    RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  }
  return ret;
}

packet::error_type remote_server::fuse_write_base64(const char *path, const char *buffer,
                                                    const remote::file_size &write_size,
                                                    const remote::file_offset &write_offset,
                                                    const remote::file_handle &handle) {
  // DOES NOTHING
  return 0;
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
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_release(const char *path,
                                               const remote::file_handle &handle) {
  const auto file_path = construct_path(path);

  remove_compat_open_info(handle);
  const auto res = _close(static_cast<int>(handle));

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_releasedir(const char *path,
                                                  const remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, 0);

  return 0;
}

/*packet::error_type remote_server::fuse_removexattr(const char *path, const char *name)
{ const auto file_path = construct_path(path); const auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}*/

packet::error_type remote_server::fuse_rmdir(const char *path) {
  const auto file_path = construct_path(path);
  const auto res = _rmdir(file_path.c_str());
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_setattr_x(const char *path, remote::setattr_x &attr) {
  const auto file_path = construct_path(path);
  const auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_setbkuptime(const char *path,
                                                   const remote::file_time &bkuptime) {
  const auto file_path = construct_path(path);
  const auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_setchgtime(const char *path,
                                                  const remote::file_time &chgtime) {
  const auto file_path = construct_path(path);
  const auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_setcrtime(const char *path,
                                                 const remote::file_time &crtime) {
  const auto file_path = construct_path(path);
  const auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_setvolname(const char *volname) {
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, volname, 0);
  return 0;
}

/*packet::error_type remote_server::fuse_setxattr(const char *path, const char *name, const
char *value, const remote::file_size &size, const std::int32_t &flags) { const auto file_path =
construct_path(path); const auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_setxattr_osx(const char *path, const char *name, const
char *value, const remote::file_size &size, const std::int32_t &flags, const std::uint32_t
&position) { const auto file_path = construct_path(path); const auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
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

  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, 0);
  return 0;
}

packet::error_type remote_server::fuse_statfs_x(const char *path, const std::uint64_t &bsize,
                                                remote::statfs_x &st) {
  const auto file_path = construct_path(path);

  const auto total_bytes = drive_.get_total_drive_space();
  const auto total_used = drive_.get_used_drive_space();
  const auto used_blocks =
      utils::divide_with_ceiling(total_used, static_cast<std::uint64_t>(bsize));
  st.f_blocks = utils::divide_with_ceiling(total_bytes, static_cast<std::uint64_t>(bsize));
  st.f_bavail = st.f_bfree = st.f_blocks ? (st.f_blocks - used_blocks) : 0;
  st.f_files = 4294967295;
  st.f_ffree = st.f_favail = st.f_files - drive_.get_total_item_count();
  strncpy(&st.f_mntfromname[0u], (utils::create_volume_label(config_.get_provider_type())).c_str(),
          1024);

  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, 0);
  return 0;
}

packet::error_type remote_server::fuse_truncate(const char *path, const remote::file_offset &size) {
  const auto file_path = construct_path(path);
  const auto unicode_file_path = utils::string::from_utf8(file_path);
  auto res = -1;
  errno = ENOENT;

  const auto flags_and_attributes =
      FILE_FLAG_BACKUP_SEMANTICS | (::PathIsDirectoryW(unicode_file_path.c_str())
                                        ? FILE_ATTRIBUTE_DIRECTORY | FILE_FLAG_POSIX_SEMANTICS
                                        : 0);
  const auto os_handle =
      ::CreateFileW(unicode_file_path.c_str(), FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                    flags_and_attributes, 0);
  if (os_handle != INVALID_HANDLE_VALUE) {
    errno = EFAULT;
    FILE_END_OF_FILE_INFO fi{};
    fi.EndOfFile.QuadPart = size;
    if (::SetFileInformationByHandle(os_handle, FileEndOfFileInfo, &fi,
                                     sizeof(FILE_END_OF_FILE_INFO))) {
      res = 0;
      errno = 0;
    }
    ::CloseHandle(os_handle);
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_unlink(const char *path) {
  const auto file_path = construct_path(path);
  const auto res = _unlink(file_path.c_str());
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::fuse_utimens(const char *path, const remote::file_time *tv,
                                               const std::uint64_t &op0, const std::uint64_t &op1) {
  const auto file_path = construct_path(path);
  const auto unicode_file_path = utils::string::from_utf8(file_path);

  auto res = -1;
  errno = ENOENT;

  const auto flags_and_attributes =
      FILE_FLAG_BACKUP_SEMANTICS | (::PathIsDirectoryW(unicode_file_path.c_str())
                                        ? FILE_ATTRIBUTE_DIRECTORY | FILE_FLAG_POSIX_SEMANTICS
                                        : 0);
  const auto os_handle = ::CreateFileW(unicode_file_path.c_str(), FILE_WRITE_ATTRIBUTES,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                       nullptr, OPEN_EXISTING, flags_and_attributes, 0);
  if (os_handle != INVALID_HANDLE_VALUE) {
    FILETIME access_time{};
    FILETIME write_time{};
    FILETIME *access_time_ptr = nullptr;
    FILETIME *write_time_ptr = nullptr;

    if (not tv[0u] || (op0 == UTIME_NOW)) {
      const auto now = utils::get_file_time_now();
      access_time.dwHighDateTime = ((now >> 32u) & 0xFFFFFFFF);
      access_time.dwLowDateTime = now & 0xFFFFFFFF;
      access_time_ptr = &access_time;
    } else if (op0 != UTIME_OMIT) {
      utils::unix_time_to_filetime(tv[0u], access_time);
      access_time_ptr = &access_time;
    }

    if (not tv[1] || (op1 == UTIME_NOW)) {
      const auto now = utils::get_file_time_now();
      write_time.dwHighDateTime = ((now >> 32u) & 0xFFFFFFFF);
      write_time.dwLowDateTime = now & 0xFFFFFFFF;
      write_time_ptr = &write_time;
    } else if (op1 != UTIME_OMIT) {
      utils::unix_time_to_filetime(tv[1], write_time);
      write_time_ptr = &write_time;
    }

    errno = EFAULT;
    if (::SetFileTime(os_handle, nullptr, access_time_ptr, write_time_ptr)) {
      res = 0;
      errno = 0;
    }
    ::CloseHandle(os_handle);
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

// JSON Layer
packet::error_type remote_server::json_create_directory_snapshot(const std::string &path,
                                                                 json &json_data) {
  const auto file_path = construct_path(path);

  auto res = -1;
  errno = ENOENT;

  if (utils::file::is_directory(file_path)) {
    auto list = drive_.get_directory_items(utils::path::create_api_path(path));
    auto *iterator = new directory_iterator(std::move(list));
    json_data["path"] = path;
    json_data["handle"] =
        static_cast<remote::file_handle>(reinterpret_cast<std::uintptr_t>(iterator));
    json_data["page_count"] =
        utils::divide_with_ceiling(iterator->get_count(), REPERTORY_DIRECTORY_PAGE_SIZE);
    res = 0;
    errno = 0;
  }

  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
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
  json_data["handle"] =
      static_cast<remote::file_handle>(reinterpret_cast<std::uintptr_t>(iterator));
  json_data["path"] = path;
  json_data["page"] = page;
  json_data["page_count"] =
      utils::divide_with_ceiling(iterator->get_count(), REPERTORY_DIRECTORY_PAGE_SIZE);
  const auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type
remote_server::json_release_directory_snapshot(const std::string &path,
                                               const remote::file_handle &handle) {
  const auto file_path = construct_path(path);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, 0);

  return 0;
}

// WinFSP Layer
packet::error_type remote_server::winfsp_can_delete(PVOID file_desc, PWSTR file_name) {
  const auto handle = reinterpret_cast<HANDLE>(file_desc);

  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    FILE_DISPOSITION_INFO dispositionInfo = {TRUE};
    ret = ::SetFileInformationByHandle(handle, FileDispositionInfo, &dispositionInfo,
                                       sizeof(FILE_DISPOSITION_INFO))
              ? STATUS_SUCCESS
              : FspNtStatusFromWin32(::GetLastError());
  }
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, get_open_file_path(file_desc), ret);
  return ret;
}

packet::error_type remote_server::winfsp_cleanup(PVOID file_desc, PWSTR file_name, UINT32 flags,
                                                 BOOLEAN &was_closed) {
  const auto file_path = get_open_file_path(file_desc);
  was_closed = FALSE;
  const auto handle = reinterpret_cast<HANDLE>(file_desc);
  const auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    if (flags & FileSystemBase::CleanupDelete) {
      ::CloseHandle(handle);
      remove_open_info(file_desc);
      was_closed = TRUE;
    }
  }
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::winfsp_close(PVOID file_desc) {
  const auto file_path = get_open_file_path(file_desc);

  const auto handle = reinterpret_cast<HANDLE>(file_desc);
  const auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    ::CloseHandle(handle);
    remove_open_info(file_desc);
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, file_path, ret);
  return ret;
}

packet::error_type remote_server::winfsp_create(PWSTR file_name, UINT32 create_options,
                                                UINT32 granted_access, UINT32 attributes,
                                                UINT64 allocation_size, PVOID *file_desc,
                                                remote::file_info *file_info,
                                                std::string &normalized_name, BOOLEAN &exists) {
  const auto file_path = utils::string::from_utf8(
      utils::path::combine(mount_location_, {utils::string::to_utf8(file_name)}));
  exists = utils::file::is_file(
      utils::path::combine(mount_location_, {utils::string::to_utf8(file_name)}));

  auto create_flags = FILE_FLAG_BACKUP_SEMANTICS;
  if (create_options & FILE_DIRECTORY_FILE) {
    create_flags |= FILE_FLAG_POSIX_SEMANTICS;
    attributes |= FILE_ATTRIBUTE_DIRECTORY;
  } else {
    attributes &= ~FILE_ATTRIBUTE_DIRECTORY;
  }

  if (not attributes) {
    attributes = FILE_ATTRIBUTE_NORMAL;
  }

  const auto handle = ::CreateFileW(file_path.c_str(), granted_access,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                    CREATE_NEW, create_flags | attributes, 0);
  if (handle != INVALID_HANDLE_VALUE) {
    *file_desc = reinterpret_cast<PVOID>(handle);
    normalized_name = utils::string::to_utf8(file_name);
    set_open_info(*file_desc, open_info{0, "", nullptr, utils::string::to_utf8(file_path)});
  }

  const auto ret =
      (handle == INVALID_HANDLE_VALUE)
          ? FspNtStatusFromWin32(::GetLastError())
          : populate_file_info(construct_api_path(get_open_file_path(handle)), *file_info);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, utils::string::to_utf8(file_path), ret);
  return ret;
}

packet::error_type remote_server::winfsp_flush(PVOID file_desc, remote::file_info *file_info) {
  const auto handle = reinterpret_cast<HANDLE>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    if ((ret = ::FlushFileBuffers(handle) ? ret : FspNtStatusFromWin32(::GetLastError())) ==
        STATUS_SUCCESS) {
      ret = populate_file_info(construct_api_path(get_open_file_path(handle)), *file_info);
    }
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, get_open_file_path(file_desc), ret);
  return ret;
}

packet::error_type remote_server::winfsp_get_file_info(PVOID file_desc,
                                                       remote::file_info *file_info) {
  const auto handle = reinterpret_cast<HANDLE>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    ret = populate_file_info(construct_api_path(get_open_file_path(handle)), *file_info);
  }
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, get_open_file_path(file_desc), ret);
  return ret;
}

packet::error_type remote_server::winfsp_get_security_by_name(PWSTR file_name, PUINT32 attributes,
                                                              std::uint64_t *descriptor_size,
                                                              std::wstring &string_descriptor) {
  const auto file_path = utils::string::from_utf8(
      utils::path::combine(mount_location_, {utils::string::to_utf8(file_name)}));

  auto ret = STATUS_BUFFER_OVERFLOW;
  if (not descriptor_size || (*descriptor_size <= std::numeric_limits<SIZE_T>::max())) {
    auto *descriptor = descriptor_size ? static_cast<PSECURITY_DESCRIPTOR>(::LocalAlloc(
                                             LPTR, static_cast<SIZE_T>(*descriptor_size)))
                                       : nullptr;
    if (((ret = drive_.get_security_by_name(file_name, attributes, descriptor, descriptor_size)) ==
         STATUS_SUCCESS) &&
        descriptor_size) {
      ULONG str_size = 0u;
      const auto security_info = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                                 DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
      LPWSTR str;
      if (::ConvertSecurityDescriptorToStringSecurityDescriptorW(descriptor, SDDL_REVISION_1,
                                                                 security_info, &str, &str_size)) {
        string_descriptor = std::wstring(str, wcslen(str));
        ::LocalFree(str);
      } else {
        ret = FspNtStatusFromWin32(::GetLastError());
      }
    }
    ::LocalFree(descriptor);
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, utils::string::to_utf8(file_path), ret);
  return ret;
}

packet::error_type remote_server::winfsp_get_volume_info(UINT64 &total_size, UINT64 &free_size,
                                                         std::string &volume_label) {
  drive_.get_volume_info(total_size, free_size, volume_label);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, volume_label, STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

packet::error_type remote_server::winfsp_mounted(const std::wstring &location) {
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, utils::string::to_utf8(location), STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

packet::error_type remote_server::winfsp_open(PWSTR file_name, UINT32 create_options,
                                              UINT32 granted_access, PVOID *file_desc,
                                              remote::file_info *file_info,
                                              std::string &normalized_name) {
  const auto file_path = utils::string::from_utf8(
      utils::path::combine(mount_location_, {utils::string::to_utf8(file_name)}));

  auto create_flags = FILE_FLAG_BACKUP_SEMANTICS;
  if (create_options & FILE_DELETE_ON_CLOSE) {
    create_flags |= FILE_FLAG_DELETE_ON_CLOSE;
  }

  const auto handle = ::CreateFileW(file_path.c_str(), granted_access,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
                                    OPEN_EXISTING, create_flags, 0);
  if (handle != INVALID_HANDLE_VALUE) {
    *file_desc = reinterpret_cast<PVOID>(handle);
    normalized_name = utils::string::to_utf8(file_name);
    set_open_info(*file_desc, open_info{0, "", nullptr, utils::string::to_utf8(file_path)});
  }

  const auto ret =
      (handle == INVALID_HANDLE_VALUE)
          ? FspNtStatusFromWin32(::GetLastError())
          : populate_file_info(construct_api_path(get_open_file_path(handle)), *file_info);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, utils::string::to_utf8(file_path), ret);
  return ret;
}

packet::error_type remote_server::winfsp_overwrite(PVOID file_desc, UINT32 attributes,
                                                   BOOLEAN replace_attributes,
                                                   UINT64 allocation_size,
                                                   remote::file_info *file_info) {
  const auto handle = reinterpret_cast<HANDLE>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    if (replace_attributes) {
      if (not attributes) {
        attributes = FILE_ATTRIBUTE_NORMAL;
      }
      FILE_BASIC_INFO basic_info{};
      basic_info.FileAttributes = attributes;
      if (not ::SetFileInformationByHandle(handle, FileBasicInfo, &basic_info,
                                           sizeof(FILE_BASIC_INFO))) {
        ret = FspNtStatusFromWin32(::GetLastError());
      }
    } else if (attributes) {
      FILE_ATTRIBUTE_TAG_INFO tag_info{};
      if (not ::GetFileInformationByHandleEx(handle, FileAttributeTagInfo, &tag_info,
                                             sizeof(FILE_ATTRIBUTE_TAG_INFO))) {
        ret = FspNtStatusFromWin32(::GetLastError());
      } else {
        FILE_BASIC_INFO basic_info{};
        basic_info.FileAttributes =
            attributes | (tag_info.FileAttributes & ~(FILE_ATTRIBUTE_NORMAL));
        if ((basic_info.FileAttributes != tag_info.FileAttributes)) {
          if (not ::SetFileInformationByHandle(handle, FileBasicInfo, &basic_info,
                                               sizeof(FILE_BASIC_INFO))) {
            ret = FspNtStatusFromWin32(::GetLastError());
          }
        }
      }
    }

    if (ret == STATUS_SUCCESS) {
      FILE_ALLOCATION_INFO allocationInfo{};
      ret = ::SetFileInformationByHandle(handle, FileAllocationInfo, &allocationInfo,
                                         sizeof(FILE_ALLOCATION_INFO))
                ? populate_file_info(construct_api_path(get_open_file_path(handle)), *file_info)
                : FspNtStatusFromWin32(::GetLastError());
    }
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, get_open_file_path(file_desc), ret);
  return ret;
}

packet::error_type remote_server::winfsp_read(PVOID file_desc, PVOID buffer, UINT64 offset,
                                              UINT32 length, PUINT32 bytes_transferred) {
  *bytes_transferred = 0u;

  const auto handle = reinterpret_cast<HANDLE>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if ((ret == STATUS_SUCCESS) && length) {
    OVERLAPPED overlapped{};
    overlapped.Offset = static_cast<DWORD>(offset);
    overlapped.OffsetHigh = static_cast<DWORD>(offset >> 32);

    ret = ::ReadFile(handle, buffer, length, reinterpret_cast<LPDWORD>(bytes_transferred),
                     &overlapped)
              ? STATUS_SUCCESS
              : FspNtStatusFromWin32(::GetLastError());
  }
  if (ret != STATUS_SUCCESS) {
    RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, get_open_file_path(file_desc), ret);
  }

  return ret;
}

packet::error_type remote_server::winfsp_read_directory(PVOID file_desc, PWSTR pattern,
                                                        PWSTR marker, json &item_list) {
  auto ret = STATUS_INVALID_HANDLE;
  item_list.clear();

  open_info ofi{};
  if (get_open_info(file_desc, ofi)) {
    const auto api_path = utils::path::create_api_path(ofi.path.substr(mount_location_.size()));
    auto items = drive_.get_directory_items(api_path);
    directory_iterator iterator(std::move(items));
    auto offset = marker ? iterator.get_next_directory_offset(utils::path::create_api_path(
                               utils::path::combine(api_path, {utils::string::to_utf8(marker)})))
                         : 0;
    json item;
    while (iterator.get_json(offset++, item) == 0) {
      item_list.emplace_back(item);
    }
    ret = STATUS_SUCCESS;
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, get_open_file_path(file_desc), ret);
  return ret;
}

packet::error_type remote_server::winfsp_rename(PVOID file_desc, PWSTR file_name,
                                                PWSTR new_file_name, BOOLEAN replace_if_exists) {
  const auto from_path = utils::string::from_utf8(
      utils::path::combine(mount_location_, {utils::string::to_utf8(file_name)}));
  const auto to_path = utils::string::from_utf8(
      utils::path::combine(mount_location_, {utils::string::to_utf8(new_file_name)}));
  const auto ret = ::MoveFileExW(from_path.c_str(), to_path.c_str(),
                                 replace_if_exists ? MOVEFILE_REPLACE_EXISTING : 0)
                       ? STATUS_SUCCESS
                       : FspNtStatusFromWin32(::GetLastError());

  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, utils::string::to_utf8(from_path + L"|" + to_path),
                                   ret);
  return ret;
}

packet::error_type remote_server::winfsp_set_basic_info(PVOID file_desc, UINT32 attributes,
                                                        UINT64 creation_time,
                                                        UINT64 last_access_time,
                                                        UINT64 last_write_time, UINT64 change_time,
                                                        remote::file_info *file_info) {
  const auto handle = reinterpret_cast<HANDLE>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    if (attributes == INVALID_FILE_ATTRIBUTES) {
      attributes = 0;
    } else if (not attributes) {
      attributes = FILE_ATTRIBUTE_NORMAL;
    }

    FILE_BASIC_INFO basic_info{};
    basic_info.FileAttributes = attributes;
    basic_info.CreationTime.QuadPart = creation_time;
    basic_info.LastAccessTime.QuadPart = last_access_time;
    basic_info.LastWriteTime.QuadPart = last_write_time;
    basic_info.ChangeTime.QuadPart = change_time;
    ret = ::SetFileInformationByHandle(handle, FileBasicInfo, &basic_info, sizeof(FILE_BASIC_INFO))
              ? populate_file_info(construct_api_path(get_open_file_path(handle)), *file_info)
              : FspNtStatusFromWin32(::GetLastError());
  }
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, get_open_file_path(file_desc), ret);
  return ret;
}

packet::error_type remote_server::winfsp_set_file_size(PVOID file_desc, UINT64 new_size,
                                                       BOOLEAN set_allocation_size,
                                                       remote::file_info *file_info) {
  const auto handle = reinterpret_cast<HANDLE>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    if (set_allocation_size) {
      FILE_ALLOCATION_INFO allocation_info{};
      allocation_info.AllocationSize.QuadPart = new_size;
      if (not ::SetFileInformationByHandle(handle, FileAllocationInfo, &allocation_info,
                                           sizeof(FILE_ALLOCATION_INFO))) {
        ret = FspNtStatusFromWin32(::GetLastError());
      }
    } else {
      FILE_END_OF_FILE_INFO end_of_file_info{};
      end_of_file_info.EndOfFile.QuadPart = new_size;
      if (not ::SetFileInformationByHandle(handle, FileEndOfFileInfo, &end_of_file_info,
                                           sizeof(FILE_END_OF_FILE_INFO))) {
        ret = FspNtStatusFromWin32(::GetLastError());
      }
    }

    ret = (ret == STATUS_SUCCESS)
              ? populate_file_info(construct_api_path(get_open_file_path(handle)), *file_info)
              : ret;
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, get_open_file_path(file_desc), ret);
  return ret;
}

packet::error_type remote_server::winfsp_unmounted(const std::wstring &location) {
  RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, utils::string::to_utf8(location), STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

packet::error_type remote_server::winfsp_write(PVOID file_desc, PVOID buffer, UINT64 offset,
                                               UINT32 length, BOOLEAN write_to_end,
                                               BOOLEAN constrained_io, PUINT32 bytes_transferred,
                                               remote::file_info *file_info) {
  const auto handle = reinterpret_cast<HANDLE>(file_desc);
  *bytes_transferred = 0;
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    if (length) {
      auto should_write = true;

      LARGE_INTEGER file_size{};
      OVERLAPPED overlapped{};
      if (constrained_io) {
        if (not ::GetFileSizeEx(handle, &file_size)) {
          ret = FspNtStatusFromWin32(::GetLastError());
        } else if (offset >= static_cast<UINT64>(file_size.QuadPart)) {
          ret = STATUS_SUCCESS;
          should_write = false;
        } else if (offset + length > static_cast<UINT64>(file_size.QuadPart)) {
          length = static_cast<UINT32>(static_cast<UINT64>(file_size.QuadPart) - offset);
        }
      }

      if (should_write && (ret == STATUS_SUCCESS)) {
        overlapped.Offset = static_cast<DWORD>(offset);
        overlapped.OffsetHigh = static_cast<DWORD>(offset >> 32u);
        ret = ::WriteFile(handle, buffer, length, reinterpret_cast<LPDWORD>(bytes_transferred),
                          &overlapped)
                  ? populate_file_info(construct_api_path(get_open_file_path(handle)), *file_info)
                  : FspNtStatusFromWin32(::GetLastError());
      }
    } else {
      ret = populate_file_info(construct_api_path(get_open_file_path(handle)), *file_info);
    }
  }

  if (ret != STATUS_SUCCESS) {
    RAISE_REMOTE_WINFSP_SERVER_EVENT(__FUNCTION__, get_open_file_path(file_desc), ret);
  }
  return ret;
}

packet::error_type remote_server::winfsp_get_dir_buffer(PVOID file_desc, PVOID *&ptr) {
  return STATUS_INVALID_HANDLE;
}
} // namespace repertory::remote_winfsp

#endif // _WIN32
