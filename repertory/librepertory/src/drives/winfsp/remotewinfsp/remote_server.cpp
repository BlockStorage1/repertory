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
// NOTE: Most of the WinFSP pass-through code has been modified from:
// https://github.com/billziss-gh/winfsp/blob/master/tst/passthrough-cpp/passthrough-cpp.cpp
#if defined(_WIN32)

#include "drives/winfsp/remotewinfsp/remote_server.hpp"

#include "app_config.hpp"
#include "comm/packet/client_pool.hpp"
#include "comm/packet/packet_server.hpp"
#include "drives/directory_iterator.hpp"
#include "drives/fuse/remotefuse/i_remote_instance.hpp"
#include "drives/remote/remote_open_file_table.hpp"
#include "drives/winfsp/remotewinfsp/i_remote_instance.hpp"
#include "events/event_system.hpp"
#include "events/types/remote_server_event.hpp"
#include "platform/platform.hpp"
#include "types/remote.hpp"
#include "types/repertory.hpp"
#include "utils/common.hpp"
#include "utils/file.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"
#include "utils/time.hpp"
#include "utils/utils.hpp"

namespace repertory::remote_winfsp {
#define RAISE_REMOTE_WINFSP_SERVER_EVENT(func, file, ret)                      \
  if (config_.get_enable_drive_events() &&                                     \
      (((config_.get_event_level() >= remote_server_event::level) &&           \
        ((ret) != STATUS_SUCCESS)) ||                                          \
       (config_.get_event_level() >= event_level::trace)))                     \
  event_system::instance().raise<remote_server_event>(file, ret, func)

auto remote_server::get_next_handle() -> std::uint64_t {
  if (++next_handle_ == 0U) {
    return ++next_handle_;
  }

  return next_handle_;
}

auto remote_server::construct_path(std::string path) -> std::string {
  path = utils::path::combine(mount_location_, {path});
  if (path == mount_location_) {
    path += '\\';
  }

  return path;
}

auto remote_server::populate_file_info(const std::string &api_path,
                                       remote::file_info &file_info)
    -> packet::error_type {
  return (drive_.populate_file_info(api_path, file_info) == api_error::success)
             ? STATUS_SUCCESS
             : STATUS_OBJECT_NAME_NOT_FOUND;
}

void remote_server::populate_stat(const char *path, bool directory,
                                  remote::stat &r_stat,
                                  const struct _stat64 &unix_st) {
  r_stat.st_nlink = static_cast<remote::file_nlink>(
      directory ? 2 + drive_.get_directory_item_count(
                          utils::path::create_api_path(path))
                : 1);
  r_stat.st_atimespec =
      utils::time::windows_time_t_to_unix_time(unix_st.st_atime);
  r_stat.st_birthtimespec =
      utils::time::windows_time_t_to_unix_time(unix_st.st_ctime);
  r_stat.st_ctimespec =
      utils::time::windows_time_t_to_unix_time(unix_st.st_ctime);
  r_stat.st_mtimespec =
      utils::time::windows_time_t_to_unix_time(unix_st.st_mtime);
  r_stat.st_size = static_cast<remote::file_size>(unix_st.st_size);
  r_stat.st_mode = unix_st.st_mode;
}

// FUSE Layer
auto remote_server::fuse_access(const char *path, const std::int32_t &mask)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path{
      construct_path(path),
  };

  auto windows_mask{
      utils::unix_access_mask_to_windows(mask),
  };

  auto res{
      _access(file_path.c_str(), windows_mask),
  };

  auto ret{
      ((res < 0) ? -errno : 0),
  };

  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_chflags(const char *path, std::uint32_t /*flags*/)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_chmod(const char *path,
                               const remote::file_mode & /*mode*/)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_chown(const char *path,
                               const remote::user_id & /*uid*/,
                               const remote::group_id & /*gid*/)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_destroy() -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, "", 0);
  return 0;
}

/*packet::error_type remote_server::fuse_fallocate(const char *path, const
std::int32_t &mode, const remote::file_offset &offset, const remote::file_offset
&length, const remote::file_handle &handle) { auto file_path =
construct_path(path); auto res = HasOpenFileCompatInfo(handle, EBADF); if (res
== 0) { res = _chsize_s(static_cast<int>(handle), offset + length);
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}*/

auto remote_server::fuse_fgetattr(const char *path, remote::stat &r_stat,
                                  bool &directory,
                                  const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  r_stat = {};

  auto file_path = construct_path(path);

  auto res{
      has_compat_open_info(handle, EBADF),
  };
  if (res == 0) {
    directory = utils::file::directory(file_path).exists();
    struct _stat64 unix_st{};
    res = _fstat64(static_cast<int>(handle), &unix_st);
    if (res == 0) {
      populate_stat(path, directory, r_stat, unix_st);
    }
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_fsetattr_x(const char *path,
                                    const remote::setattr_x & /*attr*/,
                                    const remote::file_handle & /*handle*/)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_fsync(const char *path,
                               const std::int32_t & /*datasync*/,
                               const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);

  auto res{
      has_compat_open_info(handle, EBADF),
  };
  if (res == 0) {
    res = -1;
    errno = EBADF;
    auto *os_handle =
        reinterpret_cast<HANDLE>(_get_osfhandle(static_cast<int>(handle)));
    if (os_handle != INVALID_HANDLE_VALUE) {
      errno = EFAULT;
      if (::FlushFileBuffers(os_handle) != 0) {
        res = 0;
        errno = 0;
      }
    }
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_ftruncate(const char *path,
                                   const remote::file_offset &size,
                                   const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);

  auto res{
      has_compat_open_info(handle, EBADF),
  };
  if (res == 0) {
    res = -1;
    errno = EBADF;
    auto *os_handle =
        reinterpret_cast<HANDLE>(_get_osfhandle(static_cast<int>(handle)));
    if (os_handle != INVALID_HANDLE_VALUE) {
      errno = EFAULT;
      FILE_END_OF_FILE_INFO eof_inf{};
      eof_inf.EndOfFile.QuadPart = static_cast<LONGLONG>(size);
      if (::SetFileInformationByHandle(os_handle, FileEndOfFileInfo, &eof_inf,
                                       sizeof(FILE_END_OF_FILE_INFO)) != 0) {
        res = 0;
        errno = 0;
      }
    }
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_getattr(const char *path, remote::stat &r_st,
                                 bool &directory) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  r_st = {};

  auto file_path = construct_path(path);

  directory = utils::file::directory(file_path).exists();

  struct _stat64 st1{};
  auto res{
      _stat64(file_path.c_str(), &st1),
  };
  if (res == 0) {
    populate_stat(path, directory, r_st, st1);
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

/*packet::error_type remote_server::fuse_getxattr(const char *path, const char
*name, char *value, const remote::file_size &size) { auto file_path =
construct_path(path); auto ret = STATUS_NOT_IMPLEMENTED;
RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret); return ret;
}

packet::error_type remote_server::fuse_getxattr_osx(const char *path, const char
*name, char *value, const remote::file_size &size, std::uint32_t position) {
auto file_path = construct_path(path); auto ret =
STATUS_NOT_IMPLEMENTED; RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name,
file_path, ret); return ret;
}*/

auto remote_server::fuse_getxtimes(const char *path,
                                   remote::file_time & /*bkuptime*/,
                                   remote::file_time & /*crtime*/)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_init() -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, "", 0);
  return 0;
}

/*packet::error_type remote_server::fuse_listxattr(const char *path, char
*buffer, const remote::file_size &size) { auto file_path =
construct_path(path); auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}*/

auto remote_server::fuse_mkdir(const char *path,
                               const remote::file_mode & /*mode*/)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res{
      _mkdir(file_path.c_str()),
  };
  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_opendir(const char *path, remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto unicode_file_path = utils::string::from_utf8(file_path);
  auto res{-1};
  errno = ENOENT;

  if (::PathIsDirectoryW(unicode_file_path.c_str()) != 0) {
    auto iter = std::make_shared<directory_iterator>(
        drive_.get_directory_items(utils::path::create_api_path(path)));

    handle = get_next_handle();
    directory_cache_.set_directory(path, handle, iter);

    res = 0;
    errno = 0;
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_create(const char *path, const remote::file_mode &mode,
                                const remote::open_flags &flags,
                                remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto ret = -1;

  auto file_path = construct_path(path);
  auto directory_op = ((flags & remote::open_flags::directory) ==
                       remote::open_flags::directory);
  std::int32_t perms = 0;
  auto open_flags =
      utils::unix_open_flags_to_flags_and_perms(mode, flags, perms);
  auto create_op = (open_flags & _O_CREAT);

  if ((create_op != 0) && directory_op) {
    ret = -EINVAL;
  } else if (directory_op) {
    ret = -EACCES;
  } else {
    int file = -1;
    auto res{
        _sopen_s(&file, file_path.c_str(), open_flags, _SH_DENYNO, perms),
    };
    if (res == 0) {
      handle = static_cast<remote::file_handle>(file);
      ret = 0;
      set_compat_open_info(handle, file_path);
    } else {
      ret = -errno;
    }
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_open(const char *path, const remote::open_flags &flags,
                              remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res{-1};

  if ((flags & remote::open_flags::directory) ==
      remote::open_flags::directory) {
    errno = EACCES;
  } else {
    std::int32_t perms = 0;
    auto open_flags =
        utils::unix_open_flags_to_flags_and_perms(0, flags, perms) &
        (~_O_CREAT);
    int file = -1;
    res = _sopen_s(&file, file_path.c_str(), open_flags, _SH_DENYNO, 0);
    if (res == 0) {
      handle = static_cast<remote::file_handle>(file);
      set_compat_open_info(handle, file_path);
    } else {
      res = -1;
    }
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_read(const char *path, char *buffer,
                              const remote::file_size &read_size,
                              const remote::file_offset &read_offset,
                              const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto &data = *reinterpret_cast<data_buffer *>(buffer);
  auto res{0};
  if (read_size > std::numeric_limits<std::size_t>::max()) {
    res = -1;
    errno = ERANGE;
  } else if ((res = has_compat_open_info(handle, EBADF)) == 0) {
    res = static_cast<std::decay_t<decltype(res)>>(_lseeki64(
        static_cast<int>(handle), static_cast<__int64>(read_offset), SEEK_SET));
    if (res != -1) {
      data.resize(read_size);
      res = read(static_cast<int>(handle), data.data(),
                 static_cast<unsigned int>(data.size()));
      if (res == -1) {
        data.resize(0U);
      } else if (data.size() != static_cast<std::size_t>(res)) {
        data.resize(static_cast<std::size_t>(res));
      }
    }
  }

  auto ret = ((res < 0) ? -errno : static_cast<int>(data.size()));
  if (ret < 0) {
    RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  }
  return ret;
}

auto remote_server::fuse_rename(const char *from, const char *to)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto from_path = utils::path::combine(mount_location_, {from});
  auto to_path = utils::path::combine(mount_location_, {to});
  auto res{
      rename(from_path.c_str(), to_path.c_str()),
  };

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, from + std::string("|") + to,
                                   ret);
  return ret;
}

auto remote_server::fuse_write(const char *path, const char *buffer,
                               const remote::file_size &write_size,
                               const remote::file_offset &write_offset,
                               const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  std::size_t bytes_written{};
  auto res{0};
  if (write_size > std::numeric_limits<std::size_t>::max()) {
    res = -1;
    errno = ERANGE;
  } else {
    res = has_compat_open_info(handle, EBADF);
    if (res == 0) {
      res = static_cast<std::decay_t<decltype(res)>>(
          _lseeki64(static_cast<int>(handle),
                    static_cast<__int64>(write_offset), SEEK_SET));
      if (res != -1) {
        res = write(static_cast<int>(handle), buffer,
                    static_cast<unsigned int>(write_size));
        if (res != -1) {
          bytes_written = static_cast<std::size_t>(res);
        }
      }
    }
  }

  auto ret = ((res < 0) ? -errno : static_cast<int>(bytes_written));
  if (ret < 0) {
    RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  }
  return ret;
}

auto remote_server::fuse_write_base64(
    const char * /*path*/, const char * /*buffer*/,
    const remote::file_size & /*write_size*/,
    const remote::file_offset & /*write_offset*/,
    const remote::file_handle & /*handle*/) -> packet::error_type {
  // DOES NOTHING
  return 0;
}

auto remote_server::fuse_readdir(const char *path,
                                 const remote::file_offset &offset,
                                 const remote::file_handle &handle,
                                 std::string &item_path) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res{0};
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
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_release(const char *path,
                                 const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);

  remove_compat_open_info(handle);
  auto res{
      _close(static_cast<int>(handle)),
  };

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_releasedir(const char *path,
                                    const remote::file_handle & /*handle*/)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, 0);

  return 0;
}

/*packet::error_type remote_server::fuse_removexattr(const char *path, const
char *name) { auto file_path = construct_path(path); auto ret =
STATUS_NOT_IMPLEMENTED; RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name,
file_path, ret); return ret;
}*/

auto remote_server::fuse_rmdir(const char *path) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res{
      _rmdir(file_path.c_str()),
  };
  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_setattr_x(const char *path,
                                   remote::setattr_x & /*attr*/)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_setbkuptime(const char *path,
                                     const remote::file_time & /*bkuptime*/)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_setchgtime(const char *path,
                                    const remote::file_time & /*chgtime*/)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_setcrtime(const char *path,
                                   const remote::file_time & /*crtime*/)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_setvolname(const char *volname) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, volname, 0);
  return 0;
}

/*packet::error_type remote_server::fuse_setxattr(const char *path, const char
*name, const char *value, const remote::file_size &size, const std::int32_t
&flags) { auto file_path = construct_path(path); auto ret =
STATUS_NOT_IMPLEMENTED; RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name,
file_path, ret); return ret;
}

packet::error_type remote_server::fuse_setxattr_osx(const char *path, const char
*name, const char *value, const remote::file_size &size, const std::int32_t
&flags, const std::uint32_t &position) { auto file_path =
construct_path(path); auto ret = STATUS_NOT_IMPLEMENTED;
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}*/

auto remote_server::fuse_statfs(const char *path, std::uint64_t frsize,
                                remote::statfs &r_stat) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);

  auto total_bytes = drive_.get_total_drive_space();
  auto total_used = drive_.get_used_drive_space();
  auto used_blocks = utils::divide_with_ceiling(
      total_used, static_cast<std::uint64_t>(frsize));
  r_stat.f_files = 4294967295;
  r_stat.f_blocks = utils::divide_with_ceiling(
      total_bytes, static_cast<std::uint64_t>(frsize));
  r_stat.f_bavail = r_stat.f_bfree =
      r_stat.f_blocks == 0U ? 0 : (r_stat.f_blocks - used_blocks);
  r_stat.f_ffree = r_stat.f_favail =
      r_stat.f_files - drive_.get_total_item_count();

  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, 0);
  return 0;
}

auto remote_server::fuse_statfs_x(const char *path, std::uint64_t bsize,
                                  remote::statfs_x &r_stat)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);

  auto total_bytes = drive_.get_total_drive_space();
  auto total_used = drive_.get_used_drive_space();
  auto used_blocks =
      utils::divide_with_ceiling(total_used, static_cast<std::uint64_t>(bsize));
  r_stat.f_blocks = utils::divide_with_ceiling(
      total_bytes, static_cast<std::uint64_t>(bsize));
  r_stat.f_bavail = r_stat.f_bfree =
      r_stat.f_blocks == 0U ? 0 : (r_stat.f_blocks - used_blocks);
  r_stat.f_files = 4294967295;
  r_stat.f_ffree = r_stat.f_favail =
      r_stat.f_files - drive_.get_total_item_count();
  strncpy(r_stat.f_mntfromname.data(),
          (utils::create_volume_label(config_.get_provider_type())).c_str(),
          r_stat.f_mntfromname.size() - 1U);

  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, 0);
  return 0;
}

auto remote_server::fuse_truncate(const char *path,
                                  const remote::file_offset &size)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto unicode_file_path = utils::string::from_utf8(file_path);
  auto res{-1};
  errno = ENOENT;

  auto flags_and_attributes =
      FILE_FLAG_BACKUP_SEMANTICS |
      (::PathIsDirectoryW(unicode_file_path.c_str()) != 0
           ? FILE_ATTRIBUTE_DIRECTORY | FILE_FLAG_POSIX_SEMANTICS
           : 0);
  auto *os_handle = ::CreateFileW(
      unicode_file_path.c_str(), FILE_GENERIC_READ | FILE_GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
      OPEN_EXISTING, static_cast<DWORD>(flags_and_attributes), nullptr);
  if (os_handle != INVALID_HANDLE_VALUE) {
    errno = EFAULT;
    FILE_END_OF_FILE_INFO eof_inf{};
    eof_inf.EndOfFile.QuadPart = static_cast<LONGLONG>(size);
    if (::SetFileInformationByHandle(os_handle, FileEndOfFileInfo, &eof_inf,
                                     sizeof(FILE_END_OF_FILE_INFO)) != 0) {
      res = 0;
      errno = 0;
    }
    ::CloseHandle(os_handle);
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_unlink(const char *path) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto res{
      _unlink(file_path.c_str()),
  };
  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::fuse_utimens(const char *path, const remote::file_time *tv,
                                 std::uint64_t op0, std::uint64_t op1)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  auto unicode_file_path = utils::string::from_utf8(file_path);

  auto res{-1};
  errno = ENOENT;

  auto flags_and_attributes =
      FILE_FLAG_BACKUP_SEMANTICS |
      (::PathIsDirectoryW(unicode_file_path.c_str()) != 0
           ? FILE_ATTRIBUTE_DIRECTORY | FILE_FLAG_POSIX_SEMANTICS
           : 0);
  auto *os_handle = ::CreateFileW(
      unicode_file_path.c_str(), FILE_WRITE_ATTRIBUTES,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
      OPEN_EXISTING, static_cast<DWORD>(flags_and_attributes), nullptr);
  if (os_handle != INVALID_HANDLE_VALUE) {
    FILETIME access_time{};
    FILETIME write_time{};
    FILETIME *access_time_ptr{nullptr};
    FILETIME *write_time_ptr{nullptr};

    const auto proccess_timespec = [](auto op, const auto &src, auto &dst,
                                      auto *&dst_ptr) {
      if ((src == 0U) || (op == UTIME_NOW)) {
        auto now =
            utils::time::unix_time_to_windows_time(utils::time::get_time_now());
        dst.dwHighDateTime = static_cast<DWORD>((now >> 32U) & 0xFFFFFFFF);
        dst.dwLowDateTime = now & 0xFFFFFFFF;
        dst_ptr = &dst;
        return;
      }

      if (op != UTIME_OMIT) {
        dst = utils::time::unix_time_to_filetime(src);
        dst_ptr = &dst;
      }
    };
    proccess_timespec(op0, tv[0U], access_time, access_time_ptr);
    proccess_timespec(op1, tv[1U], write_time, write_time_ptr);

    errno = EFAULT;
    if (::SetFileTime(os_handle, nullptr, access_time_ptr, write_time_ptr) !=
        0) {
      res = 0;
      errno = 0;
    }

    ::CloseHandle(os_handle);
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

// JSON Layer
auto remote_server::json_create_directory_snapshot(const std::string &path,
                                                   json &json_data)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);

  auto res{-1};
  errno = ENOENT;

  if (utils::file::directory(file_path).exists()) {
    auto iter = std::make_shared<directory_iterator>(
        drive_.get_directory_items(utils::path::create_api_path(path)));
    auto handle = get_next_handle();
    directory_cache_.set_directory(path, handle, iter);

    json_data["path"] = path;
    json_data["handle"] = handle;
    json_data["page_count"] = utils::divide_with_ceiling(
        iter->get_count(), REPERTORY_DIRECTORY_PAGE_SIZE);
    res = 0;
    errno = 0;
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::json_read_directory_snapshot(
    const std::string &path, const remote::file_handle &handle,
    std::uint32_t page, json &json_data) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);

  int res{-EBADF};
  auto iter = directory_cache_.get_directory(handle);
  if (iter != nullptr) {
    std::size_t offset{};
    json item_json;
    while (
        (json_data["directory_list"].size() < REPERTORY_DIRECTORY_PAGE_SIZE) &&
        (res = iter->get_json((page * REPERTORY_DIRECTORY_PAGE_SIZE) + offset++,
                              item_json)) == 0) {
      json_data["directory_list"].emplace_back(item_json);
    }
    json_data["handle"] = handle;
    json_data["path"] = path;
    json_data["page"] = page;
    json_data["page_count"] = utils::divide_with_ceiling(
        iter->get_count(), REPERTORY_DIRECTORY_PAGE_SIZE);
  }

  auto ret = ((res < 0) ? -errno : 0);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::json_release_directory_snapshot(
    const std::string &path, const remote::file_handle & /*handle*/)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = construct_path(path);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, 0);

  return 0;
}

// WinFSP Layer
auto remote_server::winfsp_can_delete(PVOID file_desc, PWSTR /*file_name*/)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto *handle = reinterpret_cast<HANDLE>(file_desc);

  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    FILE_DISPOSITION_INFO dispositionInfo{TRUE};
    ret = ::SetFileInformationByHandle(handle, FileDispositionInfo,
                                       &dispositionInfo,
                                       sizeof(FILE_DISPOSITION_INFO)) != 0
              ? STATUS_SUCCESS
              : FspNtStatusFromWin32(::GetLastError());
  }
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, get_open_file_path(file_desc),
                                   ret);
  return ret;
}

auto remote_server::winfsp_cleanup(PVOID file_desc, PWSTR /*file_name*/,
                                   UINT32 flags, BOOLEAN &was_deleted)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = get_open_file_path(file_desc);
  was_deleted = FALSE;
  auto *handle = reinterpret_cast<HANDLE>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    if ((flags & FspCleanupDelete) != 0U) {
      remove_and_close_all(file_desc);
      was_deleted = TRUE;
    }
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_server::winfsp_close(PVOID file_desc) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = get_open_file_path(file_desc);

  auto *handle = reinterpret_cast<HANDLE>(file_desc);
  if (has_open_info(handle, STATUS_INVALID_HANDLE) == STATUS_SUCCESS) {
    ::CloseHandle(handle);
    remove_open_info(file_desc);
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, file_path, STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

auto remote_server::winfsp_create(PWSTR file_name, UINT32 create_options,
                                  UINT32 granted_access, UINT32 attributes,
                                  UINT64 /*allocation_size*/, PVOID *file_desc,
                                  remote::file_info *file_info,
                                  std::string &normalized_name, BOOLEAN &exists)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = utils::string::from_utf8(utils::path::combine(
      mount_location_, {utils::string::to_utf8(file_name)}));
  exists = static_cast<BOOLEAN>(
      utils::file::file(
          utils::path::combine(mount_location_,
                               {utils::string::to_utf8(file_name)}))
          .exists());

  attributes |= FILE_FLAG_BACKUP_SEMANTICS;
  if ((create_options & FILE_DIRECTORY_FILE) != 0U) {
    attributes |= FILE_FLAG_POSIX_SEMANTICS;
  }

  if ((create_options & FILE_DELETE_ON_CLOSE) != 0U) {
    attributes |= FILE_FLAG_DELETE_ON_CLOSE;
  }

  attributes |=
      ((create_options & FILE_DIRECTORY_FILE) == 0U ? FILE_ATTRIBUTE_ARCHIVE
                                                    : FILE_ATTRIBUTE_DIRECTORY);
  auto *handle =
      ::CreateFileW(file_path.c_str(), granted_access,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, CREATE_NEW, attributes, nullptr);
  if (handle != INVALID_HANDLE_VALUE) {
    *file_desc = reinterpret_cast<PVOID>(handle);
    normalized_name = utils::string::to_utf8(file_name);
    set_open_info(*file_desc, open_info{
                                  "",
                                  nullptr,
                                  {},
                                  utils::string::to_utf8(file_path),
                              });
  }

  auto ret =
      (handle == INVALID_HANDLE_VALUE)
          ? FspNtStatusFromWin32(::GetLastError())
          : populate_file_info(construct_api_path(get_open_file_path(handle)),
                               *file_info);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name,
                                   utils::string::to_utf8(file_path), ret);
  return ret;
}

auto remote_server::winfsp_flush(PVOID file_desc, remote::file_info *file_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto *handle = reinterpret_cast<HANDLE>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    ret = ::FlushFileBuffers(handle) ? ret
                                     : FspNtStatusFromWin32(::GetLastError());
    if (ret == STATUS_SUCCESS) {
      ret = populate_file_info(construct_api_path(get_open_file_path(handle)),
                               *file_info);
    }
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, get_open_file_path(file_desc),
                                   ret);
  return ret;
}

auto remote_server::winfsp_get_file_info(PVOID file_desc,
                                         remote::file_info *file_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto *handle = reinterpret_cast<HANDLE>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    ret = populate_file_info(construct_api_path(get_open_file_path(handle)),
                             *file_info);
  }
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, get_open_file_path(file_desc),
                                   ret);
  return ret;
}

auto remote_server::winfsp_get_security_by_name(PWSTR file_name,
                                                PUINT32 attributes,
                                                std::uint64_t *descriptor_size,
                                                std::wstring &string_descriptor)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = utils::string::from_utf8(utils::path::combine(
      mount_location_, {utils::string::to_utf8(file_name)}));

  auto ret = STATUS_BUFFER_OVERFLOW;
  if ((descriptor_size == nullptr) ||
      (*descriptor_size <= std::numeric_limits<SIZE_T>::max())) {
    auto *descriptor = descriptor_size == nullptr
                           ? nullptr
                           : static_cast<PSECURITY_DESCRIPTOR>(::LocalAlloc(
                                 LPTR, static_cast<SIZE_T>(*descriptor_size)));
    ret = drive_.get_security_by_name(file_name, attributes, descriptor,
                                      descriptor_size);
    if ((ret == STATUS_SUCCESS) && (descriptor_size != nullptr)) {
      ULONG str_size{};
      auto security_info =
          OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
          DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
      LPWSTR str{};
      if (::ConvertSecurityDescriptorToStringSecurityDescriptorW(
              descriptor, SDDL_REVISION_1,
              static_cast<SECURITY_INFORMATION>(security_info), &str,
              &str_size) != 0) {
        string_descriptor = std::wstring(str, wcslen(str));
        ::LocalFree(str);
      } else {
        ret = FspNtStatusFromWin32(::GetLastError());
      }
    }
    ::LocalFree(descriptor);
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name,
                                   utils::string::to_utf8(file_path), ret);
  return ret;
}

auto remote_server::winfsp_get_volume_info(UINT64 &total_size,
                                           UINT64 &free_size,
                                           std::string &volume_label)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  drive_.get_volume_info(total_size, free_size, volume_label);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, volume_label, STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

auto remote_server::winfsp_mounted(const std::wstring &location)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  RAISE_REMOTE_WINFSP_SERVER_EVENT(
      function_name, utils::string::to_utf8(location), STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

auto remote_server::winfsp_open(PWSTR file_name, UINT32 create_options,
                                UINT32 granted_access, PVOID *file_desc,
                                remote::file_info *file_info,
                                std::string &normalized_name)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_path = utils::string::from_utf8(utils::path::combine(
      mount_location_, {utils::string::to_utf8(file_name)}));

  auto create_flags = FILE_FLAG_BACKUP_SEMANTICS;
  if ((create_options & FILE_DELETE_ON_CLOSE) != 0U) {
    create_flags |= FILE_FLAG_DELETE_ON_CLOSE;
  }

  auto *handle = ::CreateFileW(
      file_path.c_str(), granted_access,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
      OPEN_EXISTING, static_cast<DWORD>(create_flags), nullptr);
  if (handle != INVALID_HANDLE_VALUE) {
    *file_desc = reinterpret_cast<PVOID>(handle);
    normalized_name = utils::string::to_utf8(file_name);
    set_open_info(*file_desc, open_info{
                                  "",
                                  nullptr,
                                  {},
                                  utils::string::to_utf8(file_path),
                              });
  }

  auto ret =
      (handle == INVALID_HANDLE_VALUE)
          ? FspNtStatusFromWin32(::GetLastError())
          : populate_file_info(construct_api_path(get_open_file_path(handle)),
                               *file_info);
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name,
                                   utils::string::to_utf8(file_path), ret);
  return ret;
}

auto remote_server::winfsp_overwrite(PVOID file_desc, UINT32 attributes,
                                     BOOLEAN replace_attributes,
                                     UINT64 /*allocation_size*/,
                                     remote::file_info *file_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto *handle = reinterpret_cast<HANDLE>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    attributes |= FILE_ATTRIBUTE_ARCHIVE;

    if (replace_attributes != 0U) {
      FILE_BASIC_INFO basic_info{};
      basic_info.FileAttributes = attributes;
      if (::SetFileInformationByHandle(handle, FileBasicInfo, &basic_info,
                                       sizeof(FILE_BASIC_INFO)) == 0) {
        ret = FspNtStatusFromWin32(::GetLastError());
      }
    } else {
      FILE_ATTRIBUTE_TAG_INFO tag_info{};
      if (::GetFileInformationByHandleEx(handle, FileAttributeTagInfo,
                                         &tag_info,
                                         sizeof(FILE_ATTRIBUTE_TAG_INFO))) {
        attributes |= tag_info.FileAttributes;
      } else {
        ret = FspNtStatusFromWin32(::GetLastError());
      }
    }

    if (ret == STATUS_SUCCESS) {
      FILE_ALLOCATION_INFO allocationInfo{};
      ret =
          ::SetFileInformationByHandle(handle, FileAllocationInfo,
                                       &allocationInfo,
                                       sizeof(FILE_ALLOCATION_INFO)) != 0
              ? populate_file_info(
                    construct_api_path(get_open_file_path(handle)), *file_info)
              : FspNtStatusFromWin32(::GetLastError());
    }
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, get_open_file_path(file_desc),
                                   ret);
  return ret;
}

auto remote_server::winfsp_read(PVOID file_desc, PVOID buffer, UINT64 offset,
                                UINT32 length, PUINT32 bytes_transferred)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  *bytes_transferred = 0U;

  auto *handle = reinterpret_cast<HANDLE>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if ((ret == STATUS_SUCCESS) && (length != 0U)) {
    OVERLAPPED overlapped{};
    overlapped.Offset = static_cast<DWORD>(offset);
    overlapped.OffsetHigh = static_cast<DWORD>(offset >> 32);

    ret = ::ReadFile(handle, buffer, length,
                     reinterpret_cast<LPDWORD>(bytes_transferred),
                     &overlapped) != 0
              ? STATUS_SUCCESS
              : FspNtStatusFromWin32(::GetLastError());
  }
  if (ret != STATUS_SUCCESS) {
    RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name,
                                     get_open_file_path(file_desc), ret);
  }

  return ret;
}

auto remote_server::winfsp_read_directory(PVOID file_desc, PWSTR /*pattern*/,
                                          PWSTR marker, json &item_list)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto ret = STATUS_INVALID_HANDLE;
  item_list.clear();

  open_info ofi{};
  if (get_open_info(file_desc, ofi)) {
    auto api_path =
        utils::path::create_api_path(ofi.path.substr(mount_location_.size()));
    directory_iterator iter(drive_.get_directory_items(api_path));
    auto offset = marker == nullptr
                      ? 0
                      : iter.get_next_directory_offset(
                            utils::path::create_api_path(utils::path::combine(
                                api_path, {utils::string::to_utf8(marker)})));
    json item;
    while (iter.get_json(offset++, item) == 0) {
      item_list.emplace_back(item);
    }
    ret = STATUS_SUCCESS;
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, get_open_file_path(file_desc),
                                   ret);
  return ret;
}

auto remote_server::winfsp_rename(PVOID /*file_desc*/, PWSTR file_name,
                                  PWSTR new_file_name,
                                  BOOLEAN replace_if_exists)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto from_path = utils::string::from_utf8(utils::path::combine(
      mount_location_, {utils::string::to_utf8(file_name)}));
  auto to_path = utils::string::from_utf8(utils::path::combine(
      mount_location_, {utils::string::to_utf8(new_file_name)}));
  auto ret = ::MoveFileExW(
                 from_path.c_str(), to_path.c_str(),
                 replace_if_exists == 0U ? 0 : MOVEFILE_REPLACE_EXISTING) != 0
                 ? STATUS_SUCCESS
                 : FspNtStatusFromWin32(::GetLastError());

  RAISE_REMOTE_WINFSP_SERVER_EVENT(
      function_name, utils::string::to_utf8(from_path + L"|" + to_path), ret);
  return ret;
}

auto remote_server::winfsp_set_basic_info(
    PVOID file_desc, UINT32 attributes, UINT64 creation_time,
    UINT64 last_access_time, UINT64 last_write_time, UINT64 change_time,
    remote::file_info *file_info) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto *handle = reinterpret_cast<HANDLE>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    if (attributes == INVALID_FILE_ATTRIBUTES) {
      attributes = 0U;
    } else if (attributes == 0U) {
      attributes = FILE_ATTRIBUTE_NORMAL;
    }

    FILE_BASIC_INFO basic_info{};
    basic_info.FileAttributes = attributes;
    basic_info.CreationTime.QuadPart = static_cast<LONGLONG>(creation_time);
    basic_info.LastAccessTime.QuadPart =
        static_cast<LONGLONG>(last_access_time);
    basic_info.LastWriteTime.QuadPart = static_cast<LONGLONG>(last_write_time);
    basic_info.ChangeTime.QuadPart = static_cast<LONGLONG>(change_time);
    ret = ::SetFileInformationByHandle(handle, FileBasicInfo, &basic_info,
                                       sizeof(FILE_BASIC_INFO)) != 0
              ? populate_file_info(
                    construct_api_path(get_open_file_path(handle)), *file_info)
              : FspNtStatusFromWin32(::GetLastError());
  }
  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, get_open_file_path(file_desc),
                                   ret);
  return ret;
}

auto remote_server::winfsp_set_file_size(PVOID file_desc, UINT64 new_size,
                                         BOOLEAN set_allocation_size,
                                         remote::file_info *file_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto *handle = reinterpret_cast<HANDLE>(file_desc);
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    if (set_allocation_size != 0U) {
      FILE_ALLOCATION_INFO allocation_info{};
      allocation_info.AllocationSize.QuadPart = static_cast<LONGLONG>(new_size);
      if (::SetFileInformationByHandle(handle, FileAllocationInfo,
                                       &allocation_info,
                                       sizeof(FILE_ALLOCATION_INFO)) == 0) {
        ret = FspNtStatusFromWin32(::GetLastError());
      }
    } else {
      FILE_END_OF_FILE_INFO eof_inf{};
      eof_inf.EndOfFile.QuadPart = static_cast<LONGLONG>(new_size);
      if (::SetFileInformationByHandle(handle, FileEndOfFileInfo, &eof_inf,
                                       sizeof(FILE_END_OF_FILE_INFO)) == 0) {
        ret = FspNtStatusFromWin32(::GetLastError());
      }
    }

    ret = (ret == STATUS_SUCCESS)
              ? populate_file_info(
                    construct_api_path(get_open_file_path(handle)), *file_info)
              : ret;
  }

  RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name, get_open_file_path(file_desc),
                                   ret);
  return ret;
}

auto remote_server::winfsp_unmounted(const std::wstring &location)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  RAISE_REMOTE_WINFSP_SERVER_EVENT(
      function_name, utils::string::to_utf8(location), STATUS_SUCCESS);
  return STATUS_SUCCESS;
}

auto remote_server::winfsp_write(PVOID file_desc, PVOID buffer, UINT64 offset,
                                 UINT32 length, BOOLEAN /*write_to_end*/,
                                 BOOLEAN constrained_io,
                                 PUINT32 bytes_transferred,
                                 remote::file_info *file_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto *handle = reinterpret_cast<HANDLE>(file_desc);
  *bytes_transferred = 0U;
  auto ret = has_open_info(handle, STATUS_INVALID_HANDLE);
  if (ret == STATUS_SUCCESS) {
    if (length != 0U) {
      auto should_write = true;

      LARGE_INTEGER file_size{};
      OVERLAPPED overlapped{};
      if (constrained_io != 0U) {
        if (::GetFileSizeEx(handle, &file_size) == 0) {
          ret = FspNtStatusFromWin32(::GetLastError());
        } else if (offset >= static_cast<UINT64>(file_size.QuadPart)) {
          ret = STATUS_SUCCESS;
          should_write = false;
        } else if (offset + length > static_cast<UINT64>(file_size.QuadPart)) {
          length = static_cast<UINT32>(static_cast<UINT64>(file_size.QuadPart) -
                                       offset);
        }
      }

      if (should_write && (ret == STATUS_SUCCESS)) {
        overlapped.Offset = static_cast<DWORD>(offset);
        overlapped.OffsetHigh = static_cast<DWORD>(offset >> 32u);
        ret = ::WriteFile(handle, buffer, length,
                          reinterpret_cast<LPDWORD>(bytes_transferred),
                          &overlapped) != 0
                  ? populate_file_info(
                        construct_api_path(get_open_file_path(handle)),
                        *file_info)
                  : FspNtStatusFromWin32(::GetLastError());
      }
    } else {
      ret = populate_file_info(construct_api_path(get_open_file_path(handle)),
                               *file_info);
    }
  }

  if (ret != STATUS_SUCCESS) {
    RAISE_REMOTE_WINFSP_SERVER_EVENT(function_name,
                                     get_open_file_path(file_desc), ret);
  }
  return ret;
}

auto remote_server::winfsp_get_dir_buffer(PVOID /*file_desc*/, PVOID *& /*ptr*/)
    -> packet::error_type {
  return STATUS_INVALID_HANDLE;
}
} // namespace repertory::remote_winfsp

#endif // defined(_WIN32)
