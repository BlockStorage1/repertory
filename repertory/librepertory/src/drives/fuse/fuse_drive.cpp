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

#include "drives/fuse/fuse_drive.hpp"

#include "app_config.hpp"
#include "drives/directory_cache.hpp"
#include "drives/directory_iterator.hpp"
#include "drives/eviction.hpp"
#include "drives/fuse/remotefuse/remote_server.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/consumers/logging_consumer.hpp"
#include "events/event_system.hpp"
#include "events/types/drive_mount_result.hpp"
#include "events/types/drive_mounted.hpp"
#include "events/types/drive_stop_timed_out.hpp"
#include "events/types/drive_unmount_pending.hpp"
#include "events/types/drive_unmounted.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "rpc/server/full_server.hpp"
#include "types/repertory.hpp"
#include "types/startup_exception.hpp"
#include "utils/base64.hpp"
#include "utils/collection.hpp"
#include "utils/common.hpp"
#include "utils/config.hpp"
#include "utils/error_utils.hpp"
#include "utils/polling.hpp"
#include "utils/time.hpp"
#include "utils/utils.hpp"

namespace repertory {
fuse_drive::fuse_drive(app_config &config, lock_data &lock_data,
                       i_provider &provider)
    : fuse_drive_base(config), lock_data_(lock_data), provider_(provider) {}

#if defined(__APPLE__)
auto fuse_drive::chflags_impl(std::string api_path, uint32_t flags)
    -> api_error {
  return check_and_perform(api_path, X_OK, [&](api_meta_map &) -> api_error {
    return provider_.set_item_meta(api_path, META_OSXFLAGS,
                                   std::to_string(flags));
  });
}
#endif // defined(__APPLE__)

#if FUSE_USE_VERSION >= 30
auto fuse_drive::chmod_impl(std::string api_path, mode_t mode,
                            struct fuse_file_info * /*f_info*/) -> api_error {
#else  // FUSE_USE_VERSION < 30
auto fuse_drive::chmod_impl(std::string api_path, mode_t mode) -> api_error {
#endif // FUSE_USE_VERSION >= 30
  return check_and_perform(api_path, X_OK, [&](api_meta_map &) -> api_error {
    if (((mode & (S_ISGID | S_ISUID | S_ISVTX)) != 0) &&
        (get_effective_uid() != 0)) {
      return api_error::permission_denied;
    }

    return provider_.set_item_meta(api_path, META_MODE, std::to_string(mode));
  });
}

#if FUSE_USE_VERSION >= 30
auto fuse_drive::chown_impl(std::string api_path, uid_t uid, gid_t gid,
                            struct fuse_file_info * /*f_info*/) -> api_error {
#else
auto fuse_drive::chown_impl(std::string api_path, uid_t uid, gid_t gid)
    -> api_error {
#endif
  REPERTORY_USES_FUNCTION_NAME();

  return check_and_perform(
      api_path, X_OK, [&](api_meta_map &meta) -> api_error {
        meta.clear();
        if (uid != static_cast<uid_t>(-1)) {
          if (get_effective_uid() != 0 && get_effective_uid() != uid) {
            utils::error::raise_error(
                function_name, fmt::format("failed user|{}|{}",
                                           get_effective_uid(), getuid()));
            return api_error::permission_denied;
          }

          meta[META_UID] = std::to_string(uid);
        }

        if (gid != static_cast<gid_t>(-1)) {
          if (get_effective_uid() != 0 &&
              not utils::is_uid_member_of_group(get_effective_uid(), gid)) {
            utils::error::raise_error(
                function_name, fmt::format("failed group|{}|{}",
                                           get_effective_gid(), getgid()));
            return api_error::permission_denied;
          }

          meta[META_GID] = std::to_string(gid);
        }

        if (meta.empty()) {
          return api_error::success;
        }

        return provider_.set_item_meta(api_path, meta);
      });
}

auto fuse_drive::create_impl(std::string api_path, mode_t mode,
                             struct fuse_file_info *f_info) -> api_error {
  f_info->fh = static_cast<std::uint64_t>(INVALID_HANDLE_VALUE);

  auto is_append_op = ((f_info->flags & O_APPEND) == O_APPEND);
  auto is_create_op = ((f_info->flags & O_CREAT) == O_CREAT);
  auto is_directory_op = ((f_info->flags & O_DIRECTORY) == O_DIRECTORY);
  auto is_exclusive = ((f_info->flags & O_EXCL) == O_EXCL);
  auto is_read_write_op = ((f_info->flags & O_RDWR) == O_RDWR);
  auto is_truncate_op = ((f_info->flags & O_TRUNC) == O_TRUNC);
  auto is_write_only_op = ((f_info->flags & O_WRONLY) == O_WRONLY);

  if (is_create_op && is_append_op && is_truncate_op) {
    return api_error::invalid_operation;
  }

  auto res = check_parent_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }

  if (is_create_op) {
    res = check_access(api_path, W_OK);
    if (res == api_error::item_not_found) {
      res = check_parent_access(api_path, W_OK);
    }
  } else {
    res = check_access(api_path, R_OK);
    if (res == api_error::item_not_found) {
      res = check_parent_access(api_path, R_OK);
    }
  }

  if (res != api_error::success) {
    return res;
  }

  if ((is_write_only_op || is_read_write_op) && provider_.is_read_only()) {
    return api_error::permission_denied;
  }

  if (is_create_op && is_directory_op) {
    return api_error::invalid_operation;
  }

  bool file_exists{};
  res = provider_.is_file(api_path, file_exists);
  if (res != api_error::success) {
    return res;
  }

  bool dir_exists{};
  if (not file_exists) {
    res = provider_.is_directory(api_path, dir_exists);
    if (res != api_error::success) {
      return res;
    }
  }

  if (is_create_op) {
    if (dir_exists) {
      return api_error::directory_exists;
    }

    if (is_exclusive && file_exists) {
      return api_error::item_exists;
    }
  } else {
    if (is_directory_op ? file_exists : dir_exists) {
      return is_directory_op ? api_error::item_exists
                             : api_error::directory_exists;
    }

    if (not(is_directory_op ? dir_exists : file_exists)) {
      return is_directory_op ? api_error::directory_not_found
                             : api_error::item_not_found;
    }

    if ((is_exclusive || is_truncate_op) && not file_exists) {
      return api_error::item_not_found;
    }
  }

  std::uint64_t handle{};
  {
    std::shared_ptr<i_open_file> open_file;
    if (is_create_op) {
      auto now = utils::time::get_time_now();
#if defined(__APPLE__)
      auto osx_flags = static_cast<std::uint32_t>(f_info->flags);
#else  // !defined(__APPLE__)
      auto osx_flags = 0U;
#endif // defined(__APPLE__)

      auto meta = create_meta_attributes(
          now, FILE_ATTRIBUTE_ARCHIVE, now, now, is_directory_op,
          get_effective_gid(), "", mode, now, osx_flags, 0U,
          utils::path::combine(config_.get_cache_directory(),
                               {utils::create_uuid_string()}),
          get_effective_uid(), now);

      res = fm_->create(api_path, meta, f_info->flags, handle, open_file);
      if ((res != api_error::item_exists) && (res != api_error::success)) {
        return res;
      }
    } else {
      res = fm_->open(api_path, is_directory_op, f_info->flags, handle,
                      open_file);
      if (res != api_error::success) {
        return res;
      }
    }
  }

  f_info->fh = handle;
  if (is_truncate_op) {
#if FUSE_USE_VERSION >= 30
    res = truncate_impl(api_path, 0, f_info);
    if (res != api_error::success) {
#else  // FUSE_USE_VERSION < 30
    res = ftruncate_impl(api_path, 0, f_info);
    if (res != api_error::success) {
#endif // FUSE_USE_VERSION >= 30
      fm_->close(handle);
      f_info->fh = static_cast<std::uint64_t>(INVALID_HANDLE_VALUE);
      errno = std::abs(utils::from_api_error(res));
      return res;
    }
  }

  return api_error::success;
}

void fuse_drive::stop_all() {
  REPERTORY_USES_FUNCTION_NAME();

  mutex_lock lock(stop_all_mtx_);

  auto future = std::async(std::launch::async, [this]() {
    remote_server_.reset();

    if (server_) {
      server_->stop();
    }

    polling::instance().stop();

    if (eviction_) {
      eviction_->stop();
    }

    if (fm_) {
      fm_->stop();
    }

    provider_.stop();

    directory_cache_.reset();
    eviction_.reset();
    server_.reset();

    fm_.reset();
  });

  if (future.wait_for(30s) == std::future_status::timeout) {
    event_system::instance().raise<drive_stop_timed_out>(function_name);
    app_config::set_stop_requested();
    future.wait();
  }

  if (not lock_data_.set_mount_state(false, "", -1)) {
    utils::error::raise_error(function_name, "failed to set mount state");
  }
}

void fuse_drive::destroy_impl(void *ptr) {
  REPERTORY_USES_FUNCTION_NAME();

  event_system::instance().raise<drive_unmount_pending>(function_name,
                                                        get_mount_location());

  stop_all();

  config_.save();

  fuse_base::destroy_impl(ptr);

  event_system::instance().raise<drive_unmounted>(function_name,
                                                  get_mount_location());
}

auto fuse_drive::fallocate_impl(std::string /*api_path*/, int mode,
                                off_t offset, off_t length,
                                struct fuse_file_info *f_info) -> api_error {
  std::shared_ptr<i_open_file> open_file;
  if (not fm_->get_open_file(f_info->fh, true, open_file)) {
    return api_error::invalid_handle;
  }

  auto res = check_writeable(open_file->get_open_data(f_info->fh),
                             api_error::invalid_handle);
  if (res != api_error::success) {
    return res;
  }

  res = check_open_flags(open_file->get_open_data(f_info->fh),
                         O_WRONLY | O_APPEND, api_error::invalid_handle);
  if (res != api_error::success) {
    return res;
  }

  i_open_file::native_operation_callback allocator;

  auto new_file_size = static_cast<std::uint64_t>(offset + length);
#if defined(__APPLE__)
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

    allocator = [&](native_handle handle) -> api_error {
      return fcntl(handle, F_PREALLOCATE, &fstore) >= 0 ? api_error::success
                                                        : api_error::os_error;
    };
  } else {
    return api_error::not_supported;
  }
#else  // __APPLE__
  allocator = [&](native_handle handle) -> api_error {
    return (fallocate(handle, mode, offset, length) == -1) ? api_error::os_error
                                                           : api_error::success;
  };

  if ((mode & FALLOC_FL_KEEP_SIZE) == FALLOC_FL_KEEP_SIZE) {
    new_file_size = open_file->get_file_size();
  }
#endif // __APPLE__

  return open_file->native_operation(new_file_size, allocator);
}

auto fuse_drive::fgetattr_impl(std::string api_path, struct stat *u_stat,
                               struct fuse_file_info *f_info) -> api_error {
  std::shared_ptr<i_open_file> open_file;
  if (not fm_->get_open_file(f_info->fh, false, open_file)) {
    return api_error::invalid_handle;
  }

  auto is_unlinked{
      not open_file->is_directory() && open_file->is_unlinked(),
  };

  api_meta_map meta{};
  if (is_unlinked) {
    meta = open_file->get_unlinked_meta();
  } else {
    auto res = provider_.get_item_meta(api_path, meta);
    if (res != api_error::success) {
      return res;
    }
  }

  fuse_drive_base::populate_stat(api_path, open_file->get_file_size(), meta,
                                 open_file->is_directory(), provider_, u_stat);
  if (is_unlinked) {
    u_stat->st_nlink = 0;
  }

  return api_error::success;
}

#if defined(__APPLE__)
auto fuse_drive::fsetattr_x_impl(std::string api_path, struct setattr_x *attr,
                                 struct fuse_file_info *f_info) -> api_error {
  std::shared_ptr<i_open_file> f;
  if (not fm_->get_open_file(f_info->fh, false, f)) {
    return api_error::invalid_handle;
  }

  return setattr_x_impl(api_path, attr);
}
#endif // __APPLE__

auto fuse_drive::fsync_impl(std::string /*api_path*/, int datasync,
                            struct fuse_file_info *f_info) -> api_error {
  std::shared_ptr<i_open_file> open_file;
  if (not fm_->get_open_file(f_info->fh, false, open_file)) {
    return api_error::invalid_handle;
  }

  return open_file->native_operation([&datasync](int handle) -> api_error {
    if (handle != REPERTORY_INVALID_HANDLE) {
#if defined(__APPLE__)
      if ((datasync == 0 ? fsync(handle) : fcntl(handle, F_FULLFSYNC)) == -1) {
#else  // __APPLE__
      if ((datasync == 0 ? fsync(handle) : fdatasync(handle)) == -1) {
#endif // __APPLE__
        return api_error::os_error;
      }
    }
    return api_error::success;
  });
}

#if FUSE_USE_VERSION < 30
auto fuse_drive::ftruncate_impl(std::string /*api_path*/, off_t size,
                                struct fuse_file_info *f_info) -> api_error {
  std::shared_ptr<i_open_file> f;
  if (not fm_->get_open_file(f_info->fh, true, f)) {
    return api_error::invalid_handle;
  }

  auto res =
      check_writeable(f->get_open_data(f_info->fh), api_error::invalid_handle);
  if (res != api_error::success) {
    return res;
  }

  return f->resize(size);
}
#endif

auto fuse_drive::get_directory_item_count(std::string_view api_path) const
    -> std::uint64_t {
  return provider_.get_directory_item_count(api_path);
}

auto fuse_drive::get_directory_items(std::string_view api_path) const
    -> directory_item_list {
  REPERTORY_USES_FUNCTION_NAME();

  directory_item_list list{};
  auto res = provider_.get_directory_items(api_path, list);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(function_name, api_path, res,
                                       "failed to get directory items");
  }

  return list;
}

auto fuse_drive::get_file_size(std::string_view api_path) const
    -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  std::uint64_t file_size{};
  auto res = provider_.get_file_size(api_path, file_size);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(function_name, api_path, res,
                                       "failed to get file size from provider");
  }

  return file_size;
}

auto fuse_drive::get_item_meta(std::string_view api_path,
                               api_meta_map &meta) const -> api_error {
  return provider_.get_item_meta(api_path, meta);
}

auto fuse_drive::get_item_meta(std::string_view api_path, std::string_view name,
                               std::string &value) const -> api_error {
  api_meta_map meta{};
  auto ret = get_item_meta(api_path, meta);
  if (ret == api_error::success) {
    value = meta[std::string{name}];
  }

  return ret;
}

auto fuse_drive::get_item_stat(std::uint64_t handle,
                               struct stat64 *u_stat) const -> api_error {
  std::shared_ptr<i_open_file> open_file;
  if (not fm_->get_open_file(handle, false, open_file)) {
    return api_error::invalid_handle;
  }

  api_meta_map meta{};
  if (open_file->is_unlinked()) {
    meta = open_file->get_unlinked_meta();
  } else {
    auto ret = provider_.get_item_meta(open_file->get_api_path(), meta);
    if (ret != api_error::success) {
      return ret;
    }
  }

  fuse_drive_base::populate_stat(open_file->get_api_path(),
                                 open_file->get_file_size(), meta,
                                 open_file->is_directory(), provider_, u_stat);
  return api_error::success;
}

#if FUSE_USE_VERSION >= 30
auto fuse_drive::getattr_impl(std::string api_path, struct stat *u_stat,
                              struct fuse_file_info *f_info) -> api_error {
  if (f_info != nullptr && f_info->fh != 0 &&
      f_info->fh != static_cast<std::uint64_t>(REPERTORY_INVALID_HANDLE)) {
    return fgetattr_impl(api_path, u_stat, f_info);
  }
#else
auto fuse_drive::getattr_impl(std::string api_path, struct stat *u_stat)
    -> api_error {
#endif
  auto res = check_parent_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }

  api_meta_map meta{};
  res = provider_.get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  fuse_drive_base::populate_stat(
      api_path, utils::string::to_uint64(meta[META_SIZE]), meta,
      utils::string::to_bool(meta[META_DIRECTORY]), provider_, u_stat);

  return api_error::success;
}

auto fuse_drive::get_total_drive_space() const -> std::uint64_t {
  return provider_.get_total_drive_space();
}

auto fuse_drive::get_total_item_count() const -> std::uint64_t {
  return provider_.get_total_item_count();
}

auto fuse_drive::get_used_drive_space() const -> std::uint64_t {
  return provider_.get_used_drive_space();
}

void fuse_drive::get_volume_info(UINT64 &total_size, UINT64 &free_size,
                                 std::string &volume_label) const {
  total_size = provider_.get_total_drive_space();
  free_size = total_size - provider_.get_used_drive_space();
  volume_label = utils::create_volume_label(config_.get_provider_type());
}

#if defined(__APPLE__)
auto fuse_drive::getxtimes_impl(std::string api_path, struct timespec *bkuptime,
                                struct timespec *crtime) -> api_error {
  if (not(bkuptime && crtime)) {
    return api_error::bad_address;
  }

  auto res = check_parent_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }

  api_meta_map meta{};
  res = provider_.get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  get_timespec_from_meta(meta, META_CREATION, *crtime);
  get_timespec_from_meta(meta, META_BACKUP, *bkuptime);

  return api_error::success;
}
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
auto fuse_drive::init_impl(struct fuse_conn_info *conn, struct fuse_config *cfg)
    -> void * {
#else
auto fuse_drive::init_impl(struct fuse_conn_info *conn) -> void * {
#endif
  REPERTORY_USES_FUNCTION_NAME();

#if FUSE_USE_VERSION >= 30
  auto *ret = fuse_drive_base::init_impl(conn, cfg);
#else
  auto *ret = fuse_drive_base::init_impl(conn);
#endif

  try {
    if (console_enabled_) {
      console_consumer_ =
          std::make_unique<console_consumer>(config_.get_event_level());
    }

    logging_consumer_ = std::make_unique<logging_consumer>(
        config_.get_event_level(), config_.get_log_directory());
    event_system::instance().start();
    was_mounted_ = true;

    fm_ = std::make_unique<file_manager>(config_, provider_);
    server_ = std::make_unique<full_server>(config_, provider_, *fm_);
    if (not provider_.is_read_only()) {
      eviction_ = std::make_unique<eviction>(provider_, config_, *fm_);
    }

    directory_cache_ = std::make_unique<directory_cache>();
    server_->start();

    if (not provider_.start(
            [this](bool directory, api_file &file) -> api_error {
              return provider_meta_handler(provider_, directory, file);
            },
            fm_.get())) {
      throw startup_exception("provider is offline");
    }

    fm_->start();
    if (eviction_) {
      eviction_->start();
    }

    if (config_.get_remote_mount().enable) {
      remote_server_ = std::make_unique<remote_fuse::remote_server>(
          config_, *this, get_mount_location());
    }

    polling::instance().start(&config_);

    if (not lock_data_.set_mount_state(true, get_mount_location(), getpid())) {
      utils::error::raise_error(function_name, "failed to set mount state");
    }

    event_system::instance().raise<drive_mounted>(function_name,
                                                  get_mount_location());
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "exception during fuse init");

    destroy_impl(this);

    fuse_exit(fuse_get_context()->fuse);
  }

  return ret;
}

auto fuse_drive::ioctl_impl(std::string /* api_path */, int cmd, void *arg,
                            struct fuse_file_info *f_info) -> api_error {
  if (cmd == repertory_ioctl_fd_command) {
    if (arg == nullptr) {
      return api_error::invalid_operation;
    }

    std::memcpy(arg, &f_info->fh, sizeof(f_info->fh));
    return api_error::success;
  }

  return api_error::no_tty;
}

auto fuse_drive::is_processing(std::string_view api_path) const -> bool {
  return fm_->is_processing(api_path);
}

auto fuse_drive::mkdir_impl(std::string api_path, mode_t mode) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  auto res = check_parent_access(api_path, W_OK | X_OK);
  if (res != api_error::success) {
    return res;
  }

  auto now = utils::time::get_time_now();
  auto meta = create_meta_attributes(now, FILE_ATTRIBUTE_DIRECTORY, now, now,
                                     true, get_effective_gid(), "", mode, now,
                                     0U, 0U, "", get_effective_uid(), now);
  res = provider_.create_directory(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  if (api_path != "/") {
    res = provider_.set_item_meta(utils::path::get_parent_api_path(api_path),
                                  META_MODIFIED, std::to_string(now));
    if (res != api_error::success) {
      utils::error::raise_api_path_error(
          function_name, api_path, res,
          "failed to set directory modified time");
    }
  }

  return api_error::success;
}

void fuse_drive::notify_fuse_main_exit(int &ret) {
  REPERTORY_USES_FUNCTION_NAME();

  if (not was_mounted_) {
    return;
  }

  event_system::instance().raise<drive_mount_result>(
      function_name, get_mount_location(), std::to_string(ret));
  event_system::instance().stop();
  logging_consumer_.reset();
  console_consumer_.reset();
}

auto fuse_drive::open_impl(std::string api_path, struct fuse_file_info *f_info)
    -> api_error {

  f_info->flags &= (~O_CREAT);
  return create_impl(api_path, 0, f_info);
}

auto fuse_drive::opendir_impl(std::string api_path,
                              struct fuse_file_info *f_info) -> api_error {
  f_info->fh = static_cast<std::uint64_t>(INVALID_HANDLE_VALUE);

  auto mask = (O_RDONLY != (f_info->flags & O_ACCMODE) ? W_OK : R_OK) | X_OK;
  auto res = check_access(api_path, mask);
  if (res != api_error::success) {
    return res;
  }

  res = check_parent_access(api_path, mask);
  if (res != api_error::success) {
    return res;
  }

  bool exists{};
  res = provider_.is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (not exists) {
    return api_error::directory_not_found;
  }

  if ((f_info->flags & O_APPEND) == O_APPEND ||
      (f_info->flags & O_EXCL) == O_EXCL) {
    return api_error::directory_exists;
  }

  directory_item_list list{};
  res = provider_.get_directory_items(api_path, list);
  if (res != api_error::success) {
    return res;
  }

  auto iter = std::make_shared<directory_iterator>(std::move(list));
  f_info->fh = fm_->get_next_handle();
  directory_cache_->set_directory(api_path, f_info->fh, iter);

  return api_error::success;
}

auto fuse_drive::read_impl(std::string api_path, char *buffer, size_t read_size,
                           off_t read_offset, struct fuse_file_info *f_info,
                           std::size_t &bytes_read) -> api_error {
  std::shared_ptr<i_open_file> open_file;
  if (not fm_->get_open_file(f_info->fh, false, open_file)) {
    return api_error::item_not_found;
  }
  if (open_file->is_directory()) {
    return api_error::directory_exists;
  }

  auto res = check_readable(open_file->get_open_data(f_info->fh),
                            api_error::invalid_handle);
  if (res != api_error::success) {
    return res;
  }

  data_buffer data;
  res =
      open_file->read(read_size, static_cast<std::uint64_t>(read_offset), data);
  bytes_read = data.size();
  if (bytes_read != 0U) {
    std::memcpy(buffer, data.data(), data.size());
    data.clear();
    update_accessed_time(api_path);
  }

  return res;
}

#if FUSE_USE_VERSION >= 30
auto fuse_drive::readdir_impl(std::string api_path, void *buf,
                              fuse_fill_dir_t fuse_fill_dir, off_t offset,
                              struct fuse_file_info *f_info,
                              fuse_readdir_flags /*flags*/) -> api_error {
#else
auto fuse_drive::readdir_impl(std::string api_path, void *buf,
                              fuse_fill_dir_t fuse_fill_dir, off_t offset,
                              struct fuse_file_info *f_info) -> api_error {
#endif
  auto res = check_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }

  auto iter = directory_cache_->get_directory(f_info->fh);
  if (iter == nullptr) {
    return api_error::invalid_handle;
  }

  while (res == api_error::success) {
    res =
        (iter->fill_buffer(
             static_cast<remote::file_offset>(offset++), fuse_fill_dir, buf,
             [this](std::string_view cur_api_path, std::uint64_t cur_file_size,
                    const api_meta_map &meta, bool directory,
                    struct stat *u_stat) {
               fuse_drive_base::populate_stat(cur_api_path, cur_file_size, meta,
                                              directory, provider_, u_stat);
             }) == 0)
            ? api_error::success
            : api_error::os_error;
  }

  if ((res == api_error::os_error) && ((errno == 120) || (errno == ENOMEM))) {
    errno = 0;
    return api_error::success;
  }

  return res;
}

auto fuse_drive::release_impl(std::string /*api_path*/,
                              struct fuse_file_info *f_info) -> api_error {
  fm_->close(f_info->fh);
  return api_error::success;
}

auto fuse_drive::releasedir_impl(std::string /*api_path*/,
                                 struct fuse_file_info *f_info) -> api_error {
  auto iter = directory_cache_->get_directory(f_info->fh);
  if (iter == nullptr) {
    return api_error::invalid_handle;
  }

  directory_cache_->remove_directory(f_info->fh);
  return api_error::success;
}

auto fuse_drive::rename_directory(std::string_view from_api_path,
                                  std::string_view to_api_path) -> int {
  auto res = fm_->rename_directory(from_api_path, to_api_path);
  errno = std::abs(utils::from_api_error(res));
  return (res == api_error::success) ? 0 : -1;
}

auto fuse_drive::rename_file(std::string_view from_api_path,
                             std::string_view to_api_path, bool overwrite)
    -> int {
  auto res = fm_->rename_file(from_api_path, to_api_path, overwrite);
  errno = std::abs(utils::from_api_error(res));
  return (res == api_error::success) ? 0 : -1;
}

#if FUSE_USE_VERSION >= 30
auto fuse_drive::rename_impl(std::string from_api_path, std::string to_api_path,
                             unsigned int /*flags*/) -> api_error {
#else
auto fuse_drive::rename_impl(std::string from_api_path, std::string to_api_path)
    -> api_error {
#endif
  auto res = check_parent_access(to_api_path, W_OK | X_OK);
  if (res != api_error::success) {
    return res;
  }

  res = check_parent_access(from_api_path, W_OK | X_OK);
  if (res != api_error::success) {
    return res;
  }

  bool file{};
  res = provider_.is_file(from_api_path, file);
  if (res != api_error::success) {
    return res;
  }

  bool directory{};
  res = provider_.is_directory(from_api_path, directory);
  if (res != api_error::success) {
    return res;
  }

  return file        ? fm_->rename_file(from_api_path, to_api_path, true)
         : directory ? fm_->rename_directory(from_api_path, to_api_path)
                     : api_error::item_not_found;
}

auto fuse_drive::rmdir_impl(std::string api_path) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();
  utils::error::handle_info(function_name, "called");

  auto res = check_parent_access(api_path, W_OK | X_OK);
  if (res != api_error::success) {
    return res;
  }

  res = fm_->remove_directory(api_path);
  if (res != api_error::success) {
    return res;
  }

  directory_cache_->remove_directory(api_path);
  return api_error::success;
}

#if defined(HAS_SETXATTR)
auto fuse_drive::getxattr_common(std::string api_path, const char *name,
                                 char *value, size_t size, int &attribute_size,
                                 uint32_t *position) -> api_error {
  std::string attribute_name;
#if defined(__APPLE__)
  auto res = parse_xattr_parameters(name, value, size, *position,
                                    attribute_name, api_path);
#else  // __APPLE__
  auto res =
      parse_xattr_parameters(name, value, size, attribute_name, api_path);
#endif // __APPLE__

  if (res != api_error::success) {
    return res;
  }

  res = check_parent_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }

  api_meta_map meta;
  res = provider_.get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  if (meta.find(attribute_name) == meta.end()) {
    return api_error::xattr_not_found;
  }

  auto data = macaron::Base64::Decode(meta.at(attribute_name));
  if ((position == nullptr) || (*position < data.size())) {
    attribute_size = static_cast<int>(data.size());
    if (size == 0U) {
      return api_error::success;
    }

    if (size < data.size()) {
      return api_error::xattr_buffer_small;
    }

    std::memcpy(value, data.data(), data.size());
  }

  return api_error::success;
}

#if defined(__APPLE__)
auto fuse_drive::getxattr_impl(std::string api_path, const char *name,
                               char *value, size_t size, uint32_t position,
                               int &attribute_size) -> api_error {
  return getxattr_common(api_path, name, value, size, attribute_size,
                         &position);
}
#else  // __APPLE__
auto fuse_drive::getxattr_impl(std::string api_path, const char *name,
                               char *value, size_t size, int &attribute_size)
    -> api_error {
  return getxattr_common(api_path, name, value, size, attribute_size, nullptr);
}
#endif // __APPLE__

auto fuse_drive::listxattr_impl(std::string api_path, char *buffer, size_t size,
                                int &required_size, bool &return_size)
    -> api_error {
  auto check_size = (size == 0);

  auto res = check_parent_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }

  api_meta_map meta;
  res = provider_.get_item_meta(api_path, meta);
  if (res == api_error::success) {
    for (const auto &meta_item : meta) {
      if (utils::collection::excludes(META_USED_NAMES, meta_item.first)) {
        auto attribute_name = meta_item.first;
#if defined(__APPLE__)
        if (attribute_name != G_KAUTH_FILESEC_XATTR) {
#endif
          auto attribute_name_size = strlen(attribute_name.c_str()) + 1U;
          if (size >= attribute_name_size) {
            std::memcpy(&buffer[required_size], attribute_name.data(),
                        attribute_name_size);
            size -= attribute_name_size;
          } else {
            res = api_error::xattr_buffer_small;
          }

          required_size += static_cast<int>(attribute_name_size);
#if defined(__APPLE__)
        }
#endif
      }
    }
  }

  return_size = ((res == api_error::success) ||
                 ((res == api_error::xattr_buffer_small) && check_size));

  return res;
}

auto fuse_drive::removexattr_impl(std::string api_path, const char *name)
    -> api_error {
  std::string attribute_name;
#if defined(__APPLE__)
  auto res = parse_xattr_parameters(name, 0, attribute_name, api_path);
#else
  auto res = parse_xattr_parameters(name, attribute_name, api_path);
#endif
  if (res != api_error::success) {
    return res;
  }

  return check_and_perform(
      api_path, X_OK, [&](api_meta_map &meta) -> api_error {
        if ((meta.find(name) != meta.end()) &&
            (utils::collection::excludes(META_USED_NAMES, name))) {
          return provider_.remove_item_meta(api_path, attribute_name);
        }

        return api_error::xattr_not_found;
      });
}

#if defined(__APPLE__)
auto fuse_drive::setxattr_impl(std::string api_path, const char *name,
                               const char *value, size_t size, int flags,
                               uint32_t position) -> api_error {
#else // __APPLE__
auto fuse_drive::setxattr_impl(std::string api_path, const char *name,
                               const char *value, size_t size, int flags)
    -> api_error {
#endif
  std::string attribute_name;
#if defined(__APPLE__)
  auto res = parse_xattr_parameters(name, value, size, position, attribute_name,
                                    api_path);
#else  // __APPLE__
  auto res =
      parse_xattr_parameters(name, value, size, attribute_name, api_path);
#endif // __APPLE__
  if (res != api_error::success) {
    return res;
  }

  auto attribute_namespace =
      utils::string::contains(attribute_name, ".")
          ? utils::string::split(attribute_name, '.', false)[0U]
          : "";
  if ((attribute_name.size() > XATTR_NAME_MAX) || (size > XATTR_SIZE_MAX)) {
    return api_error::xattr_too_big;
  }

  if (utils::string::contains(attribute_name, " .") ||
      utils::string::contains(attribute_name, ". ")
#if !defined(__APPLE__)
      || utils::collection::excludes(utils::attribute_namespaces,
                                     attribute_namespace)
#endif
  ) {
    return api_error::not_supported;
  }

  api_meta_map meta;
  res = provider_.get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  res = check_parent_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }

  res = check_owner(meta);
  if (res != api_error::success) {
    return res;
  }

  if (flags == XATTR_CREATE) {
    if (meta.find(attribute_name) != meta.end()) {
      return api_error::xattr_exists;
    }
  } else if (flags == XATTR_REPLACE) {
    if (meta.find(attribute_name) == meta.end()) {
      return api_error::xattr_not_found;
    }
  }

  return provider_.set_item_meta(
      api_path, attribute_name,
      macaron::Base64::Encode(reinterpret_cast<const unsigned char *>(value),
                              size));
}
#endif // HAS_SETXATTR

void fuse_drive::set_item_meta(std::string_view api_path, std::string_view key,
                               std::string_view value) {
  REPERTORY_USES_FUNCTION_NAME();

  auto res = provider_.set_item_meta(api_path, key, value);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(
        function_name, api_path, res,
        fmt::format("failed to set item meta|key|{}", key));
  }
}

void fuse_drive::set_item_meta(std::string_view api_path,
                               const api_meta_map &meta) {
  REPERTORY_USES_FUNCTION_NAME();

  auto res = provider_.set_item_meta(api_path, meta);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(function_name, api_path, res,
                                       "failed to set item meta");
  }
}

#if defined(__APPLE__)
auto fuse_drive::setattr_x_impl(std::string api_path, struct setattr_x *attr)
    -> api_error {
  bool exists{};
  auto res = provider_.is_file(api_path, exists);
  if (res != api_error::success) {
    return res;
  }

  if (not exists) {
    res = provider_.is_directory(api_path, exists);
    if (res != api_error::success) {
      return res;
    }
    if (not exists) {
      return api_error::item_not_found;
    }
  }

  if (SETATTR_WANTS_MODE(attr)) {
    res = chmod_impl(api_path, attr->mode);
    if (res != api_error::success) {
      return res;
    }
  }

  uid_t uid = -1;
  if (SETATTR_WANTS_UID(attr)) {
    uid = attr->uid;
  }

  gid_t gid = -1;
  if (SETATTR_WANTS_GID(attr)) {
    gid = attr->gid;
  }

  if ((uid != -1) || (gid != -1)) {
    res = chown_impl(api_path, uid, gid);
    if (res != api_error::success) {
      return res;
    }
  }

  if (SETATTR_WANTS_SIZE(attr)) {
    res = truncate_impl(api_path, attr->size);
    if (res != api_error::success) {
      return res;
    }
  }

  if (SETATTR_WANTS_MODTIME(attr)) {
    struct timespec ts[2];
    if (SETATTR_WANTS_ACCTIME(attr)) {
      if (not fuse_drive_base::validate_timespec(attr->acctime)) {
        return api_error::invalid_operation;
      }

      ts[0].tv_sec = attr->acctime.tv_sec;
      ts[0].tv_nsec = attr->acctime.tv_nsec;
    } else {
      if (not fuse_drive_base::validate_timespec(attr->modtime)) {
        return api_error::invalid_operation;
      }

      struct timeval tv{};
      gettimeofday(&tv, nullptr);
      ts[0].tv_sec = tv.tv_sec;
      ts[0].tv_nsec = tv.tv_usec * 1000;
    }
    ts[1].tv_sec = attr->modtime.tv_sec;
    ts[1].tv_nsec = attr->modtime.tv_nsec;

    res = utimens_impl(api_path, ts);
    if (res != api_error::success) {
      return res;
    }
  }

  if (SETATTR_WANTS_CRTIME(attr)) {
    res = setcrtime_impl(api_path, &attr->crtime);
    if (res != api_error::success) {
      return res;
    }
  }

  if (SETATTR_WANTS_CHGTIME(attr)) {
    res = setchgtime_impl(api_path, &attr->chgtime);
    if (res != api_error::success) {
      return res;
    }
  }

  if (SETATTR_WANTS_BKUPTIME(attr)) {
    res = setbkuptime_impl(api_path, &attr->bkuptime);
    if (res != api_error::success) {
      return res;
    }
  }

  if (SETATTR_WANTS_FLAGS(attr)) {
    res = chflags_impl(api_path, attr->flags);
    if (res != api_error::success) {
      return res;
    }
  }

  return api_error::success;
}

auto fuse_drive::setbkuptime_impl(std::string api_path,
                                  const struct timespec *bkuptime)
    -> api_error {
  return check_and_perform(
      api_path, X_OK, [&](api_meta_map &meta) -> api_error {
        auto nanos = bkuptime->tv_nsec +
                     (bkuptime->tv_nsec * utils::time::NANOS_PER_SECOND);
        return provider_.set_item_meta(api_path, META_BACKUP,
                                       std::to_string(nanos));
      });
}

auto fuse_drive::setchgtime_impl(std::string api_path,
                                 const struct timespec *chgtime) -> api_error {
  return check_and_perform(
      api_path, X_OK, [&](api_meta_map &meta) -> api_error {
        auto nanos = chgtime->tv_nsec +
                     (chgtime->tv_nsec * utils::time::NANOS_PER_SECOND);
        return provider_.set_item_meta(api_path, META_CHANGED,
                                       std::to_string(nanos));
      });
}

auto fuse_drive::setcrtime_impl(std::string api_path,
                                const struct timespec *crtime) -> api_error {
  return check_and_perform(
      api_path, X_OK, [&](api_meta_map &meta) -> api_error {
        auto nanos =
            crtime->tv_nsec + (crtime->tv_nsec * utils::time::NANOS_PER_SECOND);
        return provider_.set_item_meta(api_path, META_CREATION,
                                       std::to_string(nanos));
      });
}

auto fuse_drive::setvolname_impl(const char * /*volname*/) -> api_error {
  return api_error::success;
}

auto fuse_drive::statfs_x_impl(std::string /*api_path*/, struct statfs *stbuf)
    -> api_error {
  if (statfs(&config_.get_cache_directory()[0], stbuf) != 0) {
    return api_error::os_error;
  }

  auto total_bytes = provider_.get_total_drive_space();
  auto total_used = provider_.get_used_drive_space();
  auto used_blocks = utils::divide_with_ceiling(
      total_used, static_cast<std::uint64_t>(stbuf->f_bsize));
  stbuf->f_blocks = utils::divide_with_ceiling(
      total_bytes, static_cast<std::uint64_t>(stbuf->f_bsize));
  stbuf->f_bavail = stbuf->f_bfree =
      stbuf->f_blocks ? (stbuf->f_blocks - used_blocks) : 0;
  stbuf->f_files = 4294967295;
  stbuf->f_ffree = stbuf->f_files - provider_.get_total_item_count();
  stbuf->f_owner = getuid();
  strncpy(&stbuf->f_mntonname[0U], get_mount_location().c_str(),
          get_mount_location().size());
  strncpy(&stbuf->f_mntfromname[0U],
          (utils::create_volume_label(config_.get_provider_type())).c_str(),
          sizeof(stbuf->f_mntfromname) - 1U);

  return api_error::success;
}
#else  // __APPLE__
auto fuse_drive::statfs_impl(std::string /*api_path*/, struct statvfs *stbuf)
    -> api_error {
  if (statvfs(config_.get_cache_directory().data(), stbuf) != 0) {
    return api_error::os_error;
  }

  auto total_bytes = provider_.get_total_drive_space();
  auto total_used = provider_.get_used_drive_space();
  auto used_blocks = utils::divide_with_ceiling(total_used, stbuf->f_frsize);
  stbuf->f_files = 4294967295;
  stbuf->f_blocks = utils::divide_with_ceiling(total_bytes, stbuf->f_frsize);
  stbuf->f_bavail = stbuf->f_bfree =
      stbuf->f_blocks == 0U ? 0 : (stbuf->f_blocks - used_blocks);
  stbuf->f_ffree = stbuf->f_favail =
      stbuf->f_files - provider_.get_total_item_count();

  return api_error::success;
}
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
auto fuse_drive::truncate_impl(std::string api_path, off_t size,
                               struct fuse_file_info * /*f_info*/)
    -> api_error {
#else
auto fuse_drive::truncate_impl(std::string api_path, off_t size) -> api_error {
#endif
  auto res = provider_.is_read_only() ? api_error::permission_denied
                                      : api_error::success;
  if (res != api_error::success) {
    return res;
  }
  res = check_parent_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }
  res = check_access(api_path, W_OK);
  if (res != api_error::success) {
    return res;
  }

  std::uint64_t handle{};
  {
    open_file_data ofd{O_RDWR};
    std::shared_ptr<i_open_file> open_file;
    res = fm_->open(api_path, false, ofd, handle, open_file);
    if (res != api_error::success) {
      return res;
    }

    if (not fm_->get_open_file(handle, true, open_file)) {
      return api_error::invalid_handle;
    }

    res = open_file->resize(static_cast<std::uint64_t>(size));
  }

  fm_->close(handle);
  return res;
}

auto fuse_drive::unlink_impl(std::string api_path) -> api_error {
  bool exists{};
  auto res = provider_.is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    return api_error::directory_exists;
  }

  res = check_parent_access(api_path, W_OK | X_OK);
  if (res != api_error::success) {
    return res;
  }

  return fm_->remove_file(api_path);
}

#if FUSE_USE_VERSION >= 30
auto fuse_drive::utimens_impl(std::string api_path, const struct timespec tv[2],
                              struct fuse_file_info * /*f_info*/) -> api_error {
#else
auto fuse_drive::utimens_impl(std::string api_path, const struct timespec tv[2])
    -> api_error {
#endif
  if (not validate_timespec(tv[0]) || not validate_timespec(tv[1])) {
    return api_error::invalid_operation;
  }

  api_meta_map meta;
  auto res = provider_.get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  res = check_owner(meta);
  if (res != api_error::success) {
    return res;
  }

  meta.clear();

  const auto process_timespec = [&meta, &tv](const auto &src,
                                             std::string attr) {
    if ((tv == nullptr) || (src.tv_nsec == UTIME_NOW)) {
      meta[attr] = std::to_string(utils::time::get_time_now());
      return;
    }

    if (src.tv_nsec != UTIME_OMIT) {
      meta[attr] = std::to_string(
          src.tv_nsec +
          (src.tv_sec * static_cast<std::decay_t<decltype(src.tv_sec)>>(
                            utils::time::NANOS_PER_SECOND)));
    }
  };

  process_timespec(tv[0U], META_ACCESSED);
  process_timespec(tv[1U], META_MODIFIED);

  if (not meta.empty()) {
    return provider_.set_item_meta(api_path, meta);
  }

  return api_error::success;
}

auto fuse_drive::write_impl(std::string /*api_path*/
                            ,
                            const char *buffer, size_t write_size,
                            off_t write_offset, struct fuse_file_info *f_info,
                            std::size_t &bytes_written) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  std::shared_ptr<i_open_file> open_file;
  if (not fm_->get_open_file(f_info->fh, true, open_file)) {
    return api_error::item_not_found;
  }

  if (open_file->is_directory()) {
    return api_error::directory_exists;
  }

  auto res = check_writeable(open_file->get_open_data(f_info->fh),
                             api_error::invalid_handle);
  if (res != api_error::success) {
    return res;
  }

  if (write_size > 0) {
    if ((open_file->get_open_data(f_info->fh) & O_APPEND) != 0) {
      write_offset = static_cast<off_t>(open_file->get_file_size());
    }

    data_buffer data(write_size);
    std::memcpy(data.data(), buffer, write_size);
    return open_file->write(static_cast<std::uint64_t>(write_offset),
                            std::move(data), bytes_written);
  }

  return api_error::success;
}

void fuse_drive::update_accessed_time(std::string_view api_path) {
  REPERTORY_USES_FUNCTION_NAME();

  if (atime_enabled_) {
    auto res = provider_.set_item_meta(
        api_path, META_ACCESSED, std::to_string(utils::time::get_time_now()));
    if (res != api_error::success) {
      utils::error::raise_api_path_error(function_name, api_path, res,
                                         "failed to set accessed time");
    }
  }
}
} // namespace repertory

#endif // !defined(_WIN32)
