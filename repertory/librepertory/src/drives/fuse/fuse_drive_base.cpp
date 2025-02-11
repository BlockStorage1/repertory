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

#include "drives/fuse/fuse_drive_base.hpp"

#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "utils/common.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"

namespace repertory {
auto fuse_drive_base::access_impl(std::string api_path, int mask) -> api_error {
  return check_access(api_path, mask);
}

auto fuse_drive_base::check_access(const std::string &api_path,
                                   int mask) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  api_meta_map meta;
  auto res = get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  // Always allow root
  auto current_uid = get_current_uid();
  if (current_uid == 0) {
    return api_error::success;
  }

  // Always allow forced user
  if (forced_uid_.has_value() && (current_uid == get_effective_uid())) {
    return api_error::success;
  }

  // Always allow if checking file exists
  if (F_OK == mask) {
    return api_error::success;
  }

  auto effective_uid =
      (forced_uid_.has_value() ? forced_uid_.value() : get_uid_from_meta(meta));
  auto effective_gid =
      (forced_gid_.has_value() ? forced_gid_.value() : get_gid_from_meta(meta));

  // Create file mode
  mode_t effective_mode =
      forced_umask_.has_value()
          ? ((S_IRWXU | S_IRWXG | S_IRWXO) & (~forced_umask_.value()))
          : get_mode_from_meta(meta);

  // Create access mask
  mode_t active_mask = S_IRWXO;
  if (current_uid == effective_uid) {
    active_mask |= S_IRWXU;
  }

  if (get_current_gid() == effective_gid ||
      utils::is_uid_member_of_group(current_uid, effective_gid)) {
    active_mask |= S_IRWXG;
  }

  // Calculate effective file mode
  effective_mode &= active_mask;

  // Check allow execute
  if ((mask & X_OK) == X_OK) {
    if ((effective_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0) {
      return api_error::access_denied;
    }
  }

  // Check allow write
  if ((mask & W_OK) == W_OK) {
    if ((effective_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0) {
      return api_error::access_denied;
    }
  }

  // Check allow read
  if ((mask & R_OK) == R_OK) {
    if ((effective_mode & (S_IRUSR | S_IRGRP | S_IROTH)) == 0) {
      return api_error::access_denied;
    }
  }

  if (effective_mode == 0) {
    // Deny access if effective mode is 0
    return api_error::access_denied;
  }

  return api_error::success;
}

auto fuse_drive_base::check_and_perform(
    const std::string &api_path, int parent_mask,
    const std::function<api_error(api_meta_map &meta)> &action) -> api_error {
  api_meta_map meta;
  auto ret = get_item_meta(api_path, meta);
  if (ret != api_error::success) {
    return ret;
  }

  ret = check_parent_access(api_path, parent_mask);
  if (ret != api_error::success) {
    return ret;
  }

  ret = check_owner(meta);
  if (ret != api_error::success) {
    return ret;
  }

  return action(meta);
}

auto fuse_drive_base::check_open_flags(
    int flags, int mask, const api_error &fail_error) -> api_error {
  return (((flags & mask) == 0) ? api_error::success : fail_error);
}

auto fuse_drive_base::check_owner(const std::string &api_path) const
    -> api_error {
  api_meta_map meta{};
  auto ret = get_item_meta(api_path, meta);
  if (ret == api_error::success) {
    ret = check_owner(meta);
  }

  return ret;
}

auto fuse_drive_base::check_owner(const api_meta_map &meta) const -> api_error {
  // Always allow root
  auto current_uid = get_current_uid();
  if ((current_uid != 0) &&
      // Always allow forced UID
      (not forced_uid_.has_value() || (current_uid != get_effective_uid())) &&
      // Check if current uid matches file uid
      (get_uid_from_meta(meta) != get_effective_uid())) {
    return api_error::permission_denied;
  }

  return api_error::success;
}

auto fuse_drive_base::check_parent_access(const std::string &api_path,
                                          int mask) const -> api_error {
  auto ret = api_error::success;

  // Ignore root
  if (api_path != "/") {
    if ((mask & X_OK) == X_OK) {
      for (auto parent = utils::path::get_parent_path(api_path);
           (ret == api_error::success) && not parent.empty();
           parent = utils::path::get_parent_path(parent)) {
        ret = check_access(parent, X_OK);
        if ((ret == api_error::success) && (parent == "/")) {
          break;
        }
      }
    }

    if (ret == api_error::success) {
      mask &= (~X_OK);
      if (mask != 0) {
        ret = check_access(utils::path::get_parent_path(api_path), mask);
      }
    }
  }

  return ret;
}

auto fuse_drive_base::check_readable(int flags,
                                     const api_error &fail_error) -> api_error {
  auto mode = (flags & O_ACCMODE);
  return ((mode == O_WRONLY) ? fail_error : api_error::success);
}

auto fuse_drive_base::check_writeable(int flags, const api_error &fail_error)
    -> api_error {
  return (((flags & O_ACCMODE) == 0) ? fail_error : api_error::success);
}

auto fuse_drive_base::get_current_gid() const -> gid_t {
  auto *ctx = fuse_get_context();
  return (ctx == nullptr) ? getgid() : ctx->gid;
}

auto fuse_drive_base::get_current_uid() const -> uid_t {
  auto *ctx = fuse_get_context();
  return (ctx == nullptr) ? getuid() : ctx->uid;
}

auto fuse_drive_base::get_effective_gid() const -> gid_t {
  return forced_gid_.has_value() ? forced_gid_.value() : get_current_gid();
}

auto fuse_drive_base::get_effective_uid() const -> uid_t {
  return forced_uid_.has_value() ? forced_uid_.value() : get_current_uid();
}

#if defined(__APPLE__)
auto fuse_drive_base::get_flags_from_meta(const api_meta_map &meta)
    -> __uint32_t {
  return utils::string::to_uint32(meta.at(META_OSXFLAGS));
}
#endif // defined(__APPLE__)

auto fuse_drive_base::get_gid_from_meta(const api_meta_map &meta) -> gid_t {
  return static_cast<gid_t>(utils::string::to_uint32(meta.at(META_GID)));
}

auto fuse_drive_base::get_mode_from_meta(const api_meta_map &meta) -> mode_t {
  return static_cast<mode_t>(utils::string::to_uint32(meta.at(META_MODE)));
}

void fuse_drive_base::get_timespec_from_meta(const api_meta_map &meta,
                                             const std::string &name,
                                             struct timespec &ts) {
  auto meta_time = utils::string::to_uint64(meta.at(name));
  ts.tv_nsec =
      static_cast<std::int64_t>(meta_time % utils::time::NANOS_PER_SECOND);
  ts.tv_sec =
      static_cast<std::int64_t>(meta_time / utils::time::NANOS_PER_SECOND);
}

auto fuse_drive_base::get_uid_from_meta(const api_meta_map &meta) -> uid_t {
  return static_cast<uid_t>(utils::string::to_uint32(meta.at(META_UID)));
}

#if defined(__APPLE__)
auto fuse_drive_base::parse_xattr_parameters(
    const char *name, const uint32_t &position, std::string &attribute_name,
    const std::string &api_path) -> api_error {
#else  // !defined(__APPLE__)
auto fuse_drive_base::parse_xattr_parameters(
    const char *name, std::string &attribute_name,
    const std::string &api_path) -> api_error {
#endif // defined(__APPLE__)
  auto res = api_path.empty() ? api_error::bad_address : api_error::success;
  if (res != api_error::success) {
    return res;
  }

  if (name == nullptr) {
    return api_error::bad_address;
  }

  attribute_name = std::string(name);
#if defined(__APPLE__)
  if (attribute_name == A_KAUTH_FILESEC_XATTR) {
    char new_name[MAXPATHLEN] = {0};
    memcpy(new_name, A_KAUTH_FILESEC_XATTR, sizeof(A_KAUTH_FILESEC_XATTR));
    memcpy(new_name, G_PREFIX, sizeof(G_PREFIX) - 1);
    attribute_name = new_name;
  } else if (attribute_name.empty() ||
             ((attribute_name != XATTR_RESOURCEFORK_NAME) && (position != 0))) {
    return api_error::invalid_operation;
  }
#endif // defined(__APPLE__)

  return api_error::success;
}

#if defined(__APPLE__)
auto fuse_drive_base::parse_xattr_parameters(
    const char *name, const char *value, size_t size, const uint32_t &position,
    std::string &attribute_name, const std::string &api_path) -> api_error {
  auto res = parse_xattr_parameters(name, position, attribute_name, api_path);
#else  // !defined(__APPLE__)
auto fuse_drive_base::parse_xattr_parameters(
    const char *name, const char *value, size_t size,
    std::string &attribute_name, const std::string &api_path) -> api_error {
  auto res = parse_xattr_parameters(name, attribute_name, api_path);
#endif // defined(__APPLE__)
  if (res != api_error::success) {
    return res;
  }

  return (value == nullptr
              ? ((size == 0U) ? api_error::success : api_error::bad_address)
              : api_error::success);
}

void fuse_drive_base::populate_stat(const std::string &api_path,
                                    std::uint64_t size_or_count,
                                    const api_meta_map &meta, bool directory,
                                    i_provider &provider, struct stat *st) {
  std::memset(st, 0, sizeof(struct stat));
  st->st_nlink = static_cast<nlink_t>(
      directory ? 2 + (size_or_count == 0U
                           ? provider.get_directory_item_count(api_path)
                           : size_or_count)
                : 1);
  if (directory) {
    st->st_blocks = 0;
  } else {
    st->st_size = static_cast<off_t>(size_or_count);
    static const auto block_size_stat = static_cast<std::uint64_t>(512U);
    static const auto block_size = static_cast<std::uint64_t>(4096U);
    auto size = utils::divide_with_ceiling(
                    static_cast<std::uint64_t>(st->st_size), block_size) *
                block_size;
    st->st_blocks = static_cast<blkcnt_t>(
        std::max(block_size / block_size_stat,
                 utils::divide_with_ceiling(size, block_size_stat)));
  }
  st->st_gid = get_gid_from_meta(meta);
  st->st_mode = (directory ? S_IFDIR : S_IFREG) | get_mode_from_meta(meta);
  st->st_uid = get_uid_from_meta(meta);
#if defined(__APPLE__)
  st->st_blksize = 0;
  st->st_flags = get_flags_from_meta(meta);

  set_timespec_from_meta(meta, META_MODIFIED, st->st_mtimespec);
  set_timespec_from_meta(meta, META_CREATION, st->st_birthtimespec);
  set_timespec_from_meta(meta, META_CHANGED, st->st_ctimespec);
  set_timespec_from_meta(meta, META_ACCESSED, st->st_atimespec);
#else  // !defined(__APPLE__)
  st->st_blksize = 4096;

  set_timespec_from_meta(meta, META_MODIFIED, st->st_mtim);
  set_timespec_from_meta(meta, META_CREATION, st->st_ctim);
  set_timespec_from_meta(meta, META_ACCESSED, st->st_atim);
#endif // defined(__APPLE__)
}

void fuse_drive_base::set_timespec_from_meta(const api_meta_map &meta,
                                             const std::string &name,
                                             struct timespec &ts) {
  auto meta_time = utils::string::to_uint64(meta.at(name));
  ts.tv_nsec =
      static_cast<std::int64_t>(meta_time % utils::time::NANOS_PER_SECOND);
  ts.tv_sec =
      static_cast<std::int64_t>(meta_time / utils::time::NANOS_PER_SECOND);
}
} // namespace repertory

#endif // _WIN32
