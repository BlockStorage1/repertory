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

#include "utils/unix/unix_utils.hpp"
#include "utils/utils.hpp"

namespace repertory::utils {
#ifndef __APPLE__
std::uint64_t convert_to_uint64(const pthread_t &t) { return static_cast<std::uint64_t>(t); }
#endif

int get_last_error_code() { return errno; }

std::uint64_t get_thread_id() { return convert_to_uint64(pthread_self()); }

bool is_uid_member_of_group(const uid_t &uid, const gid_t &gid) {
  auto *pw = getpwuid(uid);
  std::vector<gid_t> groups;

  int group_count = 0;
  if (getgrouplist(pw->pw_name, pw->pw_gid, nullptr, &group_count) < 0) {
    groups.resize(static_cast<unsigned long>(group_count));
#ifdef __APPLE__
    getgrouplist(pw->pw_name, pw->pw_gid, reinterpret_cast<int *>(&groups[0]), &group_count);
#else
    getgrouplist(pw->pw_name, pw->pw_gid, &groups[0], &group_count);
#endif
  }

  return collection_includes(groups, gid);
}

int translate_api_error(const api_error &e) {
  switch (e) {
  case api_error::access_denied:
    return -EACCES;
  case api_error::bad_address:
    return -EFAULT;
  case api_error::directory_end_of_files:
    return -EOF;
  case api_error::directory_exists:
    return -EISDIR;
  case api_error::directory_not_empty:
    return -ENOTEMPTY;
  case api_error::directory_not_found:
    return -ENOTDIR;
  case api_error::download_failed:
#if __APPLE__
    return -EBADMSG;
#else
    return -EREMOTEIO;
#endif
  case api_error::error:
    return -EIO;
  case api_error::file_exists:
    return -EEXIST;
  case api_error::file_in_use:
    return -EBUSY;
  case api_error::invalid_operation:
    return -EINVAL;
  case api_error::item_not_found:
    return -ENOENT;
  case api_error::item_is_file:
    return -ENOTDIR;
  case api_error::os_error:
    return -errno;
  case api_error::permission_denied:
    return -EPERM;
  case api_error::success:
    return 0;
  case api_error::not_supported:
    return -ENOTSUP;
  case api_error::not_implemented:
    return -ENOSYS;
  case api_error::upload_failed:
#if __APPLE__
    return -EBADMSG;
#else
    return -EREMOTEIO;
#endif
  case api_error::xattr_buffer_small:
    return -ERANGE;
  case api_error::xattr_exists:
    return -EEXIST;
  case api_error::xattr_invalid_namespace:
    return -ENOTSUP;
  case api_error::xattr_not_found:
#ifdef __APPLE__
    return -ENOATTR;
#else
    return -ENODATA;
#endif
  case api_error::xattr_too_big:
#ifdef __APPLE__
    return -ENAMETOOLONG;
#else
    return -E2BIG;
#endif
#ifdef __APPLE__
  case api_error::XAttrOSXInvalid:
    return -EINVAL;
#endif
  default:
    return -EIO;
  }
}

void set_last_error_code(int error_code) { errno = error_code; }

std::int32_t unix_error_to_windows(const int &e) {
  switch (e) {
  case 0:
    return STATUS_SUCCESS;
  case EACCES:
  case EPERM:
    return STATUS_ACCESS_DENIED;
  case EBADF:
    return STATUS_INVALID_HANDLE;
  case EBUSY:
    return STATUS_DEVICE_BUSY;
  case EEXIST:
    return STATUS_OBJECT_NAME_EXISTS;
  case EFAULT:
    return STATUS_INVALID_ADDRESS;
  case EFBIG:
    return STATUS_FILE_TOO_LARGE;
  case EINVAL:
    return STATUS_INVALID_PARAMETER;
  case EIO:
    return STATUS_UNEXPECTED_IO_ERROR;
  case EISDIR:
    return STATUS_FILE_IS_A_DIRECTORY;
  case EMFILE:
    return STATUS_INSUFFICIENT_RESOURCES;
  case ENOENT:
    return STATUS_OBJECT_NAME_NOT_FOUND;
  case ENOEXEC:
    return STATUS_INVALID_IMAGE_FORMAT;
  case ENOMEM:
    return STATUS_NO_MEMORY;
  case ENOSPC:
    return STATUS_DEVICE_INSUFFICIENT_RESOURCES;
  case ENOTDIR:
    return STATUS_OBJECT_PATH_INVALID;
  default:
    return STATUS_INTERNAL_ERROR;
  }
}

UINT64 unix_time_to_windows_time(const remote::file_time &ts) {
  return (ts / 100ull) + 116444736000000000ull;
}

void windows_create_to_unix(const UINT32 &create_options, const UINT32 &granted_access,
                            std::uint32_t &flags, remote::file_mode &mode) {
  mode = S_IRUSR | S_IWUSR;
  flags = O_CREAT | O_RDWR;
  if (create_options & FILE_DIRECTORY_FILE) {
    mode |= S_IXUSR;
    flags = O_DIRECTORY;
  }

  if ((granted_access & GENERIC_EXECUTE) || (granted_access & FILE_GENERIC_EXECUTE) ||
      (granted_access & FILE_EXECUTE)) {
    mode |= (S_IXUSR);
  }
}

remote::file_time windows_time_to_unix_time(const std::uint64_t &t) {
  return (t - 116444736000000000ull) * 100ull;
}
} // namespace repertory::utils

#endif // !_WIN32
