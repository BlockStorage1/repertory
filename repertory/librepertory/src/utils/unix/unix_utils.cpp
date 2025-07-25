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

#include "utils/unix/unix_utils.hpp"

#include "utils/error_utils.hpp"

namespace repertory::utils {
auto from_api_error(api_error err) -> int {
  switch (err) {
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
#if defined(__APPLE__)
    return -EBADMSG;
#else
    return -EREMOTEIO;
#endif
  case api_error::error:
    return -EIO;
  case api_error::item_exists:
    return -EEXIST;
  case api_error::file_in_use:
    return -EBUSY;
  case api_error::invalid_handle:
    return -EBADF;
  case api_error::invalid_operation:
    return -EINVAL;
  case api_error::item_not_found:
    return -ENOENT;
  case api_error::name_too_long:
    return -ENAMETOOLONG;
  case api_error::out_of_memory:
    return -ENOMEM;
  case api_error::no_disk_space:
    return -ENOSPC;
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
    return -ENETDOWN;
  case api_error::xattr_buffer_small:
    return -ERANGE;
  case api_error::xattr_exists:
    return -EEXIST;
  case api_error::xattr_not_found:
#if defined(__APPLE__)
    return -ENOATTR;
#else
    return -ENODATA;
#endif
  case api_error::xattr_too_big:
#if defined(__APPLE__)
    return -ENAMETOOLONG;
#else
    return -E2BIG;
#endif
  default:
    return -EIO;
  }
}

auto to_api_error(int err) -> api_error {
  switch (abs(err)) {
  case 0:
    return api_error::success;
  case EBADF:
    return api_error::invalid_handle;
  case EACCES:
    return api_error::access_denied;
  case EFAULT:
    return api_error::bad_address;
  case EOF:
    return api_error::directory_end_of_files;
  case EISDIR:
    return api_error::directory_exists;
  case ENOTEMPTY:
    return api_error::directory_not_empty;
  case ENOTDIR:
    return api_error::directory_not_found;
#if defined(__APPLE__)
  case EBADMSG:
    return api_error::download_failed;
#else
  case EREMOTEIO:
    return api_error::download_failed;
#endif
  case EIO:
    return api_error::error;
  case EEXIST:
    return api_error::item_exists;
  case EBUSY:
    return api_error::file_in_use;
  case EINVAL:
    return api_error::invalid_operation;
  case ENAMETOOLONG:
    return api_error::name_too_long;
  case ENOENT:
    return api_error::item_not_found;
  case ENOMEM:
    return api_error::out_of_memory;
  case EPERM:
    return api_error::permission_denied;
  case ENOSPC:
    return api_error::no_disk_space;
  case ENOTSUP:
    return api_error::not_supported;
  case ENOSYS:
    return api_error::not_implemented;
  case ENETDOWN:
    return api_error::upload_failed;
  case ERANGE:
    return api_error::xattr_buffer_small;
#if defined(__APPLE__)
  case ENOATTR:
    return api_error::xattr_not_found;
#else
  case ENODATA:
    return api_error::xattr_not_found;
#endif
#if defined(__APPLE__)
  case ENAMETOOLONG:
    return api_error::xattr_too_big;
#else
  case E2BIG:
    return api_error::xattr_too_big;
#endif
  default:
    return api_error::error;
  }
}

auto unix_error_to_windows(int err) -> std::uint32_t {
  switch (err) {
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
    return STATUS_OBJECT_NAME_EXISTS;
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
    return STATUS_OBJECT_NAME_NOT_FOUND;
  default:
    return STATUS_INTERNAL_ERROR;
  }
}

void windows_create_to_unix(const UINT32 &create_options,
                            const UINT32 &granted_access, std::uint32_t &flags,
                            remote::file_mode &mode) {
  mode = S_IRUSR | S_IWUSR;
  flags = O_CREAT | O_RDWR;
  if ((create_options & FILE_DIRECTORY_FILE) != 0U) {
    mode |= S_IXUSR;
    flags = O_DIRECTORY;
  }

  if (((granted_access & GENERIC_EXECUTE) != 0U) ||
      ((granted_access & FILE_GENERIC_EXECUTE) != 0U) ||
      ((granted_access & FILE_EXECUTE) != 0U)) {
    mode |= (S_IXUSR);
  }
}
} // namespace repertory::utils

#endif // !_WIN32
