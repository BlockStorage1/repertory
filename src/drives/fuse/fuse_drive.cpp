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
#ifndef _WIN32

#include "drives/fuse/fuse_drive.hpp"

#include "app_config.hpp"
#include "drives/directory_cache.hpp"
#include "drives/directory_iterator.hpp"
#include "drives/eviction.hpp"
#include "drives/fuse/events.hpp"
#include "drives/fuse/remotefuse/remote_server.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/consumers/logging_consumer.hpp"
#include "events/event_system.hpp"
#include "events/events.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "rpc/server/full_server.hpp"
#include "types/repertory.hpp"
#include "types/startup_exception.hpp"
#include "utils/Base64.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/polling.hpp"
#include "utils/utils.hpp"

namespace repertory {
fuse_drive::fuse_drive(app_config &config, lock_data &lock_data,
                       i_provider &provider)
    : fuse_drive_base(config), lock_data_(lock_data), provider_(provider) {}

#ifdef __APPLE__
api_error fuse_drive::chflags_impl(std::string api_path, uint32_t flags) {
  return check_and_perform(api_path, X_OK, [&](api_meta_map &) -> api_error {
    return provider_.set_item_meta(api_path, META_OSXFLAGS,
                                   std::to_string(flags));
  });
}
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
auto fuse_drive::chmod_impl(std::string api_path, mode_t mode,
                            struct fuse_file_info * /*fi*/) -> api_error {
#else
auto fuse_drive::chmod_impl(std::string api_path, mode_t mode) -> api_error {
#endif
  return check_and_perform(api_path, X_OK, [&](api_meta_map &) -> api_error {
    return provider_.set_item_meta(api_path, META_MODE, std::to_string(mode));
  });
}

#if FUSE_USE_VERSION >= 30
auto fuse_drive::chown_impl(std::string api_path, uid_t uid, gid_t gid,
                            struct fuse_file_info * /*fi*/) -> api_error {
#else
auto fuse_drive::chown_impl(std::string api_path, uid_t uid, gid_t gid)
    -> api_error {
#endif
  return check_and_perform(api_path, X_OK,
                           [&](api_meta_map &meta) -> api_error {
                             meta.clear();
                             if (uid != static_cast<uid_t>(-1)) {
                               meta[META_UID] = std::to_string(uid);
                             }

                             if (gid != static_cast<gid_t>(-1)) {
                               meta[META_GID] = std::to_string(gid);
                             }

                             if (not meta.empty()) {
                               return provider_.set_item_meta(api_path, meta);
                             }

                             return api_error::success;
                           });
}

auto fuse_drive::create_impl(std::string api_path, mode_t mode,
                             struct fuse_file_info *fi) -> api_error {
  fi->fh = 0u;

  const auto is_directory_op = ((fi->flags & O_DIRECTORY) == O_DIRECTORY);
  const auto is_create_op = ((fi->flags & O_CREAT) == O_CREAT);
  const auto is_truncate_op =
      ((fi->flags & O_TRUNC) &&
       ((fi->flags & O_WRONLY) || (fi->flags & O_RDWR)));

  if ((fi->flags & O_WRONLY) || (fi->flags & O_RDWR)) {
    const auto res = provider_.is_file_writeable(api_path)
                         ? api_error::success
                         : api_error::permission_denied;
    if (res != api_error::success) {
      return res;
    }
  }

  auto res = check_parent_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }

  if (is_create_op) {
    if ((res = check_access(api_path, W_OK)) == api_error::item_not_found) {
      res = check_parent_access(api_path, W_OK);
    }
  } else if ((res = check_access(api_path, R_OK)) ==
             api_error::item_not_found) {
    res = check_parent_access(api_path, R_OK);
  }

  if (res != api_error::success) {
    return res;
  }

  if (is_create_op && is_directory_op) {
    return api_error::invalid_operation;
  }

  if (not is_create_op) {
    bool dir_exists{};
    res = provider_.is_directory(api_path, dir_exists);
    if (res != api_error::success) {
      return res;
    }

    bool file_exists{};
    res = provider_.is_file(api_path, file_exists);
    if (res != api_error::success) {
      return res;
    }
    if (not(is_directory_op ? dir_exists : file_exists)) {
      return (is_directory_op ? api_error::directory_not_found
                              : api_error::item_not_found);
    }
  }

  std::uint64_t handle = 0u;
  std::shared_ptr<i_open_file> f;
  if (is_create_op) {
    const auto now = utils::get_file_time_now();
#ifdef __APPLE__
    const auto osx_flags = static_cast<std::uint32_t>(fi->flags);
#else
    const auto osx_flags = std::uint32_t(0u);
#endif

    auto meta = create_meta_attributes(
        now, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE, now, now,
        is_directory_op, "", get_effective_gid(), "", mode, now, 0u, osx_flags,
        0u,
        utils::path::combine(config_.get_cache_directory(),
                             {utils::create_uuid_string()}),
        get_effective_uid(), now);

    res = fm_->create(api_path, meta, fi->flags, handle, f);
    if ((res != api_error::item_exists) && (res != api_error::success)) {
      return res;
    }
  } else if (((res = fm_->open(api_path, is_directory_op, fi->flags, handle,
                               f)) != api_error::success)) {
    return res;
  }

  fi->fh = handle;
  if (is_truncate_op) {
#if FUSE_USE_VERSION >= 30
    if ((res = truncate_impl(api_path, 0, fi)) != api_error::success) {
#else
    if ((res = ftruncate_impl(api_path, 0, fi)) != api_error::success) {
#endif
      fm_->close(handle);
      fi->fh = 0u;
      errno = std::abs(utils::from_api_error(res));
      return res;
    }
  }

  return api_error::success;
}

void fuse_drive::destroy_impl(void *) {
  event_system::instance().raise<drive_unmount_pending>(get_mount_location());

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

  if (directory_cache_) {
    directory_cache_->stop();
  }

  directory_cache_.reset();
  eviction_.reset();
  server_.reset();

  fm_.reset();

  event_system::instance().raise<drive_unmounted>(get_mount_location());

  config_.save();

  if (not lock_data_.set_mount_state(false, "", -1)) {
    utils::error::raise_error(__FUNCTION__, "failed to set mount state");
  }
}

auto fuse_drive::fallocate_impl(std::string /*api_path*/, int mode,
                                off_t offset, off_t length,
                                struct fuse_file_info *fi) -> api_error {
  std::shared_ptr<i_open_file> f;
  if (not fm_->get_open_file(fi->fh, true, f)) {
    return api_error::invalid_handle;
  }

  auto res =
      check_writeable(f->get_open_data(fi->fh), api_error::invalid_handle);
  if (res != api_error::success) {
    return res;
  }

  if ((res = check_open_flags(f->get_open_data(fi->fh), O_WRONLY | O_APPEND,
                              api_error::invalid_handle)) !=
      api_error::success) {
    return res;
  }

  i_open_file::native_operation_callback allocator;

#ifdef __APPLE__
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
#endif // __APPLE__

  return f->native_operation(offset + length, allocator);
}

auto fuse_drive::fgetattr_impl(std::string api_path, struct stat *st,
                               struct fuse_file_info *fi) -> api_error {
  std::shared_ptr<i_open_file> f;
  if (not fm_->get_open_file(fi->fh, false, f)) {
    return api_error::invalid_handle;
  }

  api_meta_map meta{};
  auto res = provider_.get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  bool directory{};
  res = provider_.is_directory(api_path, directory);
  if (res != api_error::success) {
    return res;
  }
  fuse_drive_base::populate_stat(api_path, f->get_file_size(), meta, directory,
                                 provider_, st);

  return api_error::success;
}

#ifdef __APPLE__
auto fuse_drive::fsetattr_x_impl(std::string api_path, struct setattr_x *attr,
                                 struct fuse_file_info *fi) -> api_error {
  std::shared_ptr<i_open_file> f;
  if (not fm_->get_open_file(fi->fh, false, f)) {
    return api_error::invalid_handle;
  }

  return setattr_x_impl(api_path, attr);
}
#endif // __APPLE__

auto fuse_drive::fsync_impl(std::string /*api_path*/, int datasync,
                            struct fuse_file_info *fi) -> api_error {
  std::shared_ptr<i_open_file> f;
  if (not fm_->get_open_file(fi->fh, false, f)) {
    return api_error::invalid_handle;
  }

  return f->native_operation([&datasync](int handle) -> api_error {
    if (handle != REPERTORY_INVALID_HANDLE) {
#ifdef __APPLE__
      if ((datasync ? fcntl(handle, F_FULLFSYNC) : fsync(handle)) == -1) {
#else  // __APPLE__
      if ((datasync ? fdatasync(handle) : fsync(handle)) == -1) {
#endif // __APPLE__
        return api_error::os_error;
      }
    }
    return api_error::success;
  });
}

#if FUSE_USE_VERSION < 30
api_error fuse_drive::ftruncate_impl(std::string /*api_path*/, off_t size,
                                     struct fuse_file_info *fi) {
  std::shared_ptr<i_open_file> f;
  if (not fm_->get_open_file(fi->fh, true, f)) {
    return api_error::invalid_handle;
  }

  const auto res =
      check_writeable(f->get_open_data(fi->fh), api_error::invalid_handle);
  if (res != api_error::success) {
    return res;
  }

  return f->resize(size);
}
#endif

auto fuse_drive::get_directory_item_count(const std::string &api_path) const
    -> std::uint64_t {
  return provider_.get_directory_item_count(api_path);
}

auto fuse_drive::get_directory_items(const std::string &api_path) const
    -> directory_item_list {
  directory_item_list di{};
  auto res = provider_.get_directory_items(api_path, di);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                       "failed to get directory items");
  }

  return di;
}

auto fuse_drive::get_file_size(const std::string &api_path) const
    -> std::uint64_t {
  std::uint64_t file_size{};
  auto res = provider_.get_file_size(api_path, file_size);
  if (res == api_error::success) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                       "failed to get file size from provider");
  }

  return file_size;
}

auto fuse_drive::get_item_meta(const std::string &api_path,
                               api_meta_map &meta) const -> api_error {
  return provider_.get_item_meta(api_path, meta);
}

auto fuse_drive::get_item_meta(const std::string &api_path,
                               const std::string &name,
                               std::string &value) const -> api_error {
  api_meta_map meta{};
  const auto ret = get_item_meta(api_path, meta);
  if (ret == api_error::success) {
    value = meta[name];
  }

  return ret;
}

#if FUSE_USE_VERSION >= 30
auto fuse_drive::getattr_impl(std::string api_path, struct stat *st,
                              struct fuse_file_info * /*fi*/) -> api_error {
#else
auto fuse_drive::getattr_impl(std::string api_path, struct stat *st)
    -> api_error {
#endif
  const auto parent = utils::path::get_parent_api_path(api_path);

  auto res = check_parent_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }

  auto found = false;
  directory_cache_->execute_action(parent, [&](directory_iterator &iter) {
    directory_item di{};
    if ((found =
             (iter.get_directory_item(api_path, di) == api_error::success))) {
      fuse_drive_base::populate_stat(api_path, di.size, di.meta, di.directory,
                                     provider_, st);
    }
  });

  if (not found) {
    api_meta_map meta{};
    if ((res = provider_.get_item_meta(api_path, meta)) != api_error::success) {
      return res;
    }

    bool directory{};
    res = provider_.is_directory(api_path, directory);
    if (res != api_error::success) {
      return res;
    }
    fuse_drive_base::populate_stat(api_path,
                                   utils::string::to_uint64(meta[META_SIZE]),
                                   meta, directory, provider_, st);
  }

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

#ifdef __APPLE__
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
  if ((res = provider_.get_item_meta(api_path, meta)) != api_error::success) {
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
void *fuse_drive::init_impl(struct fuse_conn_info *conn) {
#endif
  utils::file::change_to_process_directory();

  if (console_enabled_) {
    console_consumer_ = std::make_unique<console_consumer>();
  }

  logging_consumer_ = std::make_unique<logging_consumer>(
      config_.get_log_directory(), config_.get_event_level());
  event_system::instance().start();
  was_mounted_ = true;

  polling::instance().start(&config_);

  fm_ = std::make_unique<file_manager>(config_, provider_);
  server_ = std::make_unique<full_server>(config_, provider_, *fm_);
  if (not provider_.is_direct_only()) {
    eviction_ = std::make_unique<eviction>(provider_, config_, *fm_);
  }
  directory_cache_ = std::make_unique<directory_cache>();

  try {
    directory_cache_->start();
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

    if (config_.get_enable_remote_mount()) {
      remote_server_ = std::make_unique<remote_fuse::remote_server>(
          config_, *this, get_mount_location());
    }

    if (not lock_data_.set_mount_state(true, get_mount_location(), getpid())) {
      utils::error::raise_error(__FUNCTION__, "failed to set mount state");
    }
    event_system::instance().raise<drive_mounted>(get_mount_location());
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception during fuse init");

    destroy_impl(this);

    fuse_exit(fuse_get_context()->fuse);
  }

#if FUSE_USE_VERSION >= 30
  return fuse_drive_base::init_impl(conn, cfg);
#else
  return fuse_drive_base::init_impl(conn);
#endif
}

auto fuse_drive::is_processing(const std::string &api_path) const -> bool {
  return fm_->is_processing(api_path);
}

auto fuse_drive::mkdir_impl(std::string api_path, mode_t mode) -> api_error {
  auto res = check_parent_access(api_path, W_OK | X_OK);
  if (res != api_error::success) {
    return res;
  }

  const auto now = utils::get_file_time_now();
  auto meta = create_meta_attributes(
      now, FILE_ATTRIBUTE_DIRECTORY, now, now, true, "", get_effective_gid(),
      "", mode, now, 0u, 0u, 0u, "", get_effective_uid(), now);
  if ((res = provider_.create_directory(api_path, meta)) !=
      api_error::success) {
    return res;
  }

  if (api_path != "/") {
    if ((res = provider_.set_item_meta(
             utils::path::get_parent_api_path(api_path), META_MODIFIED,
             std::to_string(now))) != api_error::success) {
      utils::error::raise_api_path_error(
          __FUNCTION__, api_path, res, "failed to set directory modified time");
    }
  }

  return api_error::success;
}

void fuse_drive::notify_fuse_main_exit(int &ret) {
  if (was_mounted_) {
    event_system::instance().raise<drive_mount_result>(std::to_string(ret));
    event_system::instance().stop();
    logging_consumer_.reset();
    console_consumer_.reset();
  }
}

auto fuse_drive::open_impl(std::string api_path, struct fuse_file_info *fi)
    -> api_error {
  fi->flags &= (~O_CREAT);
  return create_impl(api_path, 0, fi);
}

auto fuse_drive::opendir_impl(std::string api_path, struct fuse_file_info *fi)
    -> api_error {
  const auto mask = (O_RDONLY != (fi->flags & O_ACCMODE) ? W_OK : R_OK) | X_OK;
  auto res = check_access(api_path, mask);
  if (res != api_error::success) {
    return res;
  }

  if ((res = check_parent_access(api_path, mask)) != api_error::success) {
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

  directory_item_list dl{};
  if ((res = provider_.get_directory_items(api_path, dl)) !=
      api_error::success) {
    return res;
  }

  auto *iter = new directory_iterator(std::move(dl));
  fi->fh = reinterpret_cast<std::uint64_t>(iter);
  directory_cache_->set_directory(api_path, iter);

  return api_error::success;
}

void fuse_drive::populate_stat(const directory_item &di,
                               struct stat &st) const {
  fuse_drive_base::populate_stat(di.api_path, di.size, di.meta, di.directory,
                                 provider_, &st);
}

auto fuse_drive::read_impl(std::string api_path, char *buffer, size_t read_size,
                           off_t read_offset, struct fuse_file_info *fi,
                           std::size_t &bytes_read) -> api_error {
  std::shared_ptr<i_open_file> f;
  if (not fm_->get_open_file(fi->fh, false, f)) {
    return api_error::item_not_found;
  }

  auto res =
      check_readable(f->get_open_data(fi->fh), api_error::invalid_handle);
  if (res != api_error::success) {
    return res;
  }

  // event_system::instance().raise<debug_log>(
  //     __FUNCTION__, api_path, std::to_string(read_size) + ':' +
  //     std::to_string(read_offset));
  data_buffer data;
  res = f->read(read_size, static_cast<std::uint64_t>(read_offset), data);
  // event_system::instance().raise<debug_log>(
  //     __FUNCTION__, api_path, std::to_string(bytes_read) + ':' +
  //     api_error_to_string(res));
  if ((bytes_read = data.size())) {
    std::memcpy(buffer, data.data(), data.size());
    data.clear();
    update_accessed_time(api_path);
  }

  return res;
}

#if FUSE_USE_VERSION >= 30
auto fuse_drive::readdir_impl(std::string api_path, void *buf,
                              fuse_fill_dir_t fuse_fill_dir, off_t offset,
                              struct fuse_file_info *fi,
                              fuse_readdir_flags /*flags*/) -> api_error {
#else
auto fuse_drive::readdir_impl(std::string api_path, void *buf,
                              fuse_fill_dir_t fuse_fill_dir, off_t offset,
                              struct fuse_file_info *fi) -> api_error {
#endif
  auto res = check_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }

  auto *iter = reinterpret_cast<directory_iterator *>(fi->fh);
  if (not iter) {
    return api_error::invalid_handle;
  }

  while (res == api_error::success) {
    res = (iter->fill_buffer(
               static_cast<remote::file_offset>(offset++), fuse_fill_dir, buf,
               [this](const std::string &cur_api_path,
                      std::uint64_t cur_file_size, const api_meta_map &meta,
                      bool directory, struct stat *st) {
                 fuse_drive_base::populate_stat(cur_api_path, cur_file_size,
                                                meta, directory, provider_, st);
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
                              struct fuse_file_info *fi) -> api_error {
  fm_->close(fi->fh);
  return api_error::success;
}

auto fuse_drive::releasedir_impl(std::string /*api_path*/,
                                 struct fuse_file_info *fi) -> api_error {
  auto *iter = reinterpret_cast<directory_iterator *>(fi->fh);
  if (not iter) {
    return api_error::invalid_handle;
  }

  directory_cache_->remove_directory(iter);
  delete iter;

  return api_error::success;
}

auto fuse_drive::rename_directory(const std::string &from_api_path,
                                  const std::string &to_api_path) -> int {
  const auto res = fm_->rename_directory(from_api_path, to_api_path);
  errno = std::abs(utils::from_api_error(res));
  return (res == api_error::success) ? 0 : -1;
}

auto fuse_drive::rename_file(const std::string &from_api_path,
                             const std::string &to_api_path, bool overwrite)
    -> int {
  const auto res = fm_->rename_file(from_api_path, to_api_path, overwrite);
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

  if ((res = check_parent_access(from_api_path, W_OK | X_OK)) !=
      api_error::success) {
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
  auto res = check_parent_access(api_path, W_OK | X_OK);
  if (res != api_error::success) {
    return res;
  }

  if ((res = provider_.remove_directory(api_path)) != api_error::success) {
    return res;
  }

  auto *iter = directory_cache_->remove_directory(api_path);
  if (iter) {
    delete iter;
  }

  return api_error::success;
}

#ifdef HAS_SETXATTR
auto fuse_drive::getxattr_common(std::string api_path, const char *name,
                                 char *value, size_t size, int &attribute_size,
                                 uint32_t *position) -> api_error {
  std::string attribute_name;
#ifdef __APPLE__
  auto res = parse_xattr_parameters(name, value, size, *position,
                                    attribute_name, api_path);
#else  // __APPLE__
  auto res =
      parse_xattr_parameters(name, value, size, attribute_name, api_path);
#endif // __APPLE__

  if (res != api_error::success) {
    return res;
  }

  if ((res = check_parent_access(api_path, X_OK)) != api_error::success) {
    return res;
  }

  api_meta_map meta;
  auto found = false;
  directory_cache_->execute_action(
      utils::path::get_parent_api_path(api_path),
      [&](directory_iterator &iterator) {
        directory_item di{};
        if ((found = (iterator.get_directory_item(api_path, di) ==
                      api_error::success))) {
          meta = di.meta;
        }
      });

  if (found ||
      ((res = provider_.get_item_meta(api_path, meta)) == api_error::success)) {
    res = api_error::xattr_not_found;
    if (meta.find(attribute_name) != meta.end()) {
      const auto data = macaron::Base64::Decode(meta[attribute_name]);
      if (not position || (*position < data.size())) {
        res = api_error::success;
        attribute_size = static_cast<int>(data.size());
        if (size) {
          res = api_error::xattr_buffer_small;
          if (size >= data.size()) {
            memcpy(value, &data[0], data.size());
            return api_error::success;
          }
        }
      }
    }
  }

  return res;
}

#ifdef __APPLE__
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
  const auto check_size = (size == 0);

  auto res = check_parent_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }

  api_meta_map meta;
  if ((res = provider_.get_item_meta(api_path, meta)) == api_error::success) {
    for (const auto &kv : meta) {
      if (utils::collection_excludes(META_USED_NAMES, kv.first)) {
        auto attribute_name = kv.first;
#ifdef __APPLE__
        if (attribute_name != G_KAUTH_FILESEC_XATTR) {
#endif
          const auto attribute_name_size = strlen(attribute_name.c_str()) + 1u;
          if (size >= attribute_name_size) {
            strncpy(&buffer[required_size], attribute_name.c_str(),
                    attribute_name_size);
            size -= attribute_name_size;
          } else {
            res = api_error::xattr_buffer_small;
          }

          required_size += attribute_name_size;
#ifdef __APPLE__
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
#ifdef __APPLE__
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
            (utils::collection_excludes(META_USED_NAMES, name))) {
          return provider_.remove_item_meta(api_path, attribute_name);
        }

        return api_error::xattr_not_found;
      });
}

#ifdef __APPLE__
auto fuse_drive::setxattr_impl(std::string api_path, const char *name,
                               const char *value, size_t size, int flags,
                               uint32_t position) -> api_error {
#else // __APPLE__
auto fuse_drive::setxattr_impl(std::string api_path, const char *name,
                               const char *value, size_t size, int flags)
    -> api_error {
#endif
  std::string attribute_name;
#ifdef __APPLE__
  auto res = parse_xattr_parameters(name, value, size, position, attribute_name,
                                    api_path);
#else  // __APPLE__
  auto res =
      parse_xattr_parameters(name, value, size, attribute_name, api_path);
#endif // __APPLE__
  if (res != api_error::success) {
    return res;
  }

  const auto attribute_namespace =
      utils::string::contains(attribute_name, ".")
          ? utils::string::split(attribute_name, '.', false)[0u]
          : "";
  if ((attribute_name.size() > XATTR_NAME_MAX) || (size > XATTR_SIZE_MAX)) {
    return api_error::xattr_too_big;
  }

  if (utils::string::contains(attribute_name, " .") ||
      utils::string::contains(attribute_name, ". ")
#ifndef __APPLE__
      || utils::collection_excludes(utils::attribute_namespaces,
                                    attribute_namespace)
#endif
  ) {
    return api_error::not_supported;
  }

  api_meta_map meta;
  if ((res = provider_.get_item_meta(api_path, meta)) != api_error::success) {
    return res;
  }

  if ((res = check_parent_access(api_path, X_OK)) != api_error::success) {
    return res;
  }

  if ((res = check_owner(meta)) != api_error::success) {
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

  return provider_.set_item_meta(api_path, attribute_name,
                                 macaron::Base64::Encode(value, size));
}
#endif // HAS_SETXATTR

void fuse_drive::set_item_meta(const std::string &api_path,
                               const std::string &key,
                               const std::string &value) {
  auto res = provider_.set_item_meta(api_path, key, value);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                       "key|" + key + "|value|" + value);
  }
}

#ifdef __APPLE__
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
      ts[0].tv_sec = attr->acctime.tv_sec;
      ts[0].tv_nsec = attr->acctime.tv_nsec;
    } else {
      struct timeval tv {};
      gettimeofday(&tv, NULL);
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
        const auto nanos =
            bkuptime->tv_nsec + (bkuptime->tv_nsec * NANOS_PER_SECOND);
        return provider_.set_item_meta(api_path, META_BACKUP,
                                       std::to_string(nanos));
      });
}

auto fuse_drive::setchgtime_impl(std::string api_path,
                                 const struct timespec *chgtime) -> api_error {
  return check_and_perform(
      api_path, X_OK, [&](api_meta_map &meta) -> api_error {
        const auto nanos =
            chgtime->tv_nsec + (chgtime->tv_nsec * NANOS_PER_SECOND);
        return provider_.set_item_meta(api_path, META_CHANGED,
                                       std::to_string(nanos));
      });
}

auto fuse_drive::setcrtime_impl(std::string api_path,
                                const struct timespec *crtime) -> api_error {
  return check_and_perform(
      api_path, X_OK, [&](api_meta_map &meta) -> api_error {
        const auto nanos =
            crtime->tv_nsec + (crtime->tv_nsec * NANOS_PER_SECOND);
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

  const auto total_bytes = provider_.get_total_drive_space();
  const auto total_used = provider_.get_used_drive_space();
  const auto used_blocks = utils::divide_with_ceiling(
      total_used, static_cast<std::uint64_t>(stbuf->f_bsize));
  stbuf->f_blocks = utils::divide_with_ceiling(
      total_bytes, static_cast<std::uint64_t>(stbuf->f_bsize));
  stbuf->f_bavail = stbuf->f_bfree =
      stbuf->f_blocks ? (stbuf->f_blocks - used_blocks) : 0;
  stbuf->f_files = 4294967295;
  stbuf->f_ffree = stbuf->f_files - provider_.get_total_item_count();
  stbuf->f_owner = getuid();
  strncpy(&stbuf->f_mntonname[0], get_mount_location().c_str(), MNAMELEN);
  strncpy(&stbuf->f_mntfromname[0],
          (utils::create_volume_label(config_.get_provider_type())).c_str(),
          MNAMELEN);

  return api_error::success;
}
#else  // __APPLE__
auto fuse_drive::statfs_impl(std::string /*api_path*/, struct statvfs *stbuf)
    -> api_error {
  if (statvfs(&config_.get_cache_directory()[0], stbuf) != 0) {
    return api_error::os_error;
  }

  const auto total_bytes = provider_.get_total_drive_space();
  const auto total_used = provider_.get_used_drive_space();
  const auto used_blocks = utils::divide_with_ceiling(
      total_used, static_cast<std::uint64_t>(stbuf->f_frsize));
  stbuf->f_files = 4294967295;
  stbuf->f_blocks = utils::divide_with_ceiling(
      total_bytes, static_cast<std::uint64_t>(stbuf->f_frsize));
  stbuf->f_bavail = stbuf->f_bfree =
      stbuf->f_blocks ? (stbuf->f_blocks - used_blocks) : 0;
  stbuf->f_ffree = stbuf->f_favail =
      stbuf->f_files - provider_.get_total_item_count();

  return api_error::success;
}
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
auto fuse_drive::truncate_impl(std::string api_path, off_t size,
                               struct fuse_file_info * /*fi*/) -> api_error {
#else
auto fuse_drive::truncate_impl(std::string api_path, off_t size) -> api_error {
#endif
  auto res = provider_.is_file_writeable(api_path)
                 ? api_error::success
                 : api_error::permission_denied;
  if (res != api_error::success) {
    return res;
  }

  if ((res = check_parent_access(api_path, X_OK)) != api_error::success) {
    return res;
  }

  if ((res = check_access(api_path, W_OK)) != api_error::success) {
    return res;
  }

  open_file_data ofd = O_RDWR;
  std::uint64_t handle = 0u;
  std::shared_ptr<i_open_file> f;
  if ((res = fm_->open(api_path, false, ofd, handle, f)) !=
      api_error::success) {
    return res;
  }

  const auto cleanup = [this, &handle](const api_error &err) -> api_error {
    fm_->close(handle);
    return err;
  };

  return cleanup(f->resize(size));
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
                              struct fuse_file_info * /*fi*/) -> api_error {
#else
auto fuse_drive::utimens_impl(std::string api_path, const struct timespec tv[2])
    -> api_error {
#endif
  api_meta_map meta;
  auto res = provider_.get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  if ((res = check_owner(meta)) != api_error::success) {
    return res;
  }

  meta.clear();
  if (not tv || (tv[0].tv_nsec == UTIME_NOW)) {
    meta[META_ACCESSED] = std::to_string(utils::get_file_time_now());
  } else if (tv[0].tv_nsec != UTIME_OMIT) {
    const auto val = tv[0].tv_nsec + (tv[0].tv_sec * NANOS_PER_SECOND);
    meta[META_ACCESSED] = std::to_string(val);
  }

  if (not tv || (tv[1].tv_nsec == UTIME_NOW)) {
    meta[META_MODIFIED] = std::to_string(utils::get_file_time_now());
  } else if (tv[1].tv_nsec != UTIME_OMIT) {
    const auto val = tv[1].tv_nsec + (tv[1].tv_sec * NANOS_PER_SECOND);
    meta[META_MODIFIED] = std::to_string(val);
  }

  if (not meta.empty()) {
    return provider_.set_item_meta(api_path, meta);
  }

  return api_error::success;
}

auto fuse_drive::write_impl(std::string /*api_path*/
                            ,
                            const char *buffer, size_t write_size,
                            off_t write_offset, struct fuse_file_info *fi,
                            std::size_t &bytes_written) -> api_error {
  std::shared_ptr<i_open_file> f;
  if (not fm_->get_open_file(fi->fh, true, f)) {
    return api_error::item_not_found;
  }

  auto res =
      check_writeable(f->get_open_data(fi->fh), api_error::invalid_handle);
  if (res != api_error::success) {
    return res;
  }

  if (write_size > 0) {
    if (f->get_open_data(fi->fh) & O_APPEND) {
      write_offset = f->get_file_size();
    }

    data_buffer data(write_size);
    std::memcpy(&data[0], buffer, write_size);
    return f->write(static_cast<std::uint64_t>(write_offset), std::move(data),
                    bytes_written);
  }

  return api_error::success;
}

void fuse_drive::update_accessed_time(const std::string &api_path) {
  if (atime_enabled_) {
    auto res = provider_.set_item_meta(
        api_path, META_ACCESSED, std::to_string(utils::get_file_time_now()));
    if (res != api_error::success) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                         "failed to set accessed time");
    }
  }
}
} // namespace repertory

#endif // _WIN32
