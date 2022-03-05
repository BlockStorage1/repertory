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

#include "drives/fuse/fuse_drive.hpp"
#include "app_config.hpp"
#include "download/download_manager.hpp"
#include "drives/directory_cache.hpp"
#include "drives/directory_iterator.hpp"
#include "drives/eviction.hpp"
#include "drives/fuse/events.hpp"
#include "drives/fuse/remotefuse/remote_server.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/consumers/logging_consumer.hpp"
#include "events/events.hpp"
#include "events/event_system.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "rpc/server/full_server.hpp"
#include "types/repertory.hpp"
#include "types/startup_exception.hpp"
#include "utils/Base64.hpp"
#include "utils/file_utils.hpp"
#include "utils/global_data.hpp"
#include "utils/polling.hpp"
#include "utils/utils.hpp"

namespace repertory {
fuse_drive::fuse_drive(app_config &config, lock_data &lock_data, i_provider &provider)
    : fuse_base(config), lock_data_(lock_data), provider_(provider) {}

#ifdef __APPLE__
api_error fuse_drive::chflags_impl(std::string api_path, uint32_t flags) {
  return check_and_perform(api_path, X_OK, [&](api_meta_map &) -> api_error {
    return oft_->set_item_meta(api_path, META_OSXFLAGS, utils::string::from_uint32(flags));
  });
}
#endif // __APPLE__

api_error fuse_drive::chmod_impl(std::string api_path, mode_t mode) {
  return check_and_perform(api_path, X_OK, [&](api_meta_map &) -> api_error {
    return oft_->set_item_meta(api_path, META_MODE, utils::string::from_uint32(mode));
  });
}

api_error fuse_drive::chown_impl(std::string api_path, uid_t uid, gid_t gid) {
  return check_and_perform(api_path, X_OK, [&](api_meta_map &meta) -> api_error {
    meta.clear();
    if (uid != static_cast<uid_t>(-1)) {
      meta.insert({META_UID, utils::string::from_uint32(uid)});
    }

    if (gid != static_cast<gid_t>(-1)) {
      meta.insert({META_GID, utils::string::from_uint32(gid)});
    }

    if (not meta.empty()) {
      return oft_->set_item_meta(api_path, meta);
    }

    return api_error::success;
  });
}

api_error fuse_drive::create_impl(std::string api_path, mode_t mode, struct fuse_file_info *fi) {
  fi->fh = 0u;

  const auto is_directory_op = ((fi->flags & O_DIRECTORY) == O_DIRECTORY);
  const auto is_create_op = ((fi->flags & O_CREAT) == O_CREAT);
  const auto is_truncate_op =
      ((fi->flags & O_TRUNC) && ((fi->flags & O_WRONLY) || (fi->flags & O_RDWR)));

  if ((fi->flags & O_WRONLY) || (fi->flags & O_RDWR)) {
    const auto res =
        provider_.is_file_writeable(api_path) ? api_error::success : api_error::permission_denied;
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
  } else if ((res = check_access(api_path, R_OK)) == api_error::item_not_found) {
    res = check_parent_access(api_path, R_OK);
  }

  if (res != api_error::success) {
    return res;
  }

  if (is_create_op && is_directory_op) {
    return api_error::invalid_operation;
  }

  if (not is_create_op &&
      not(is_directory_op ? provider_.is_directory(api_path) : provider_.is_file(api_path))) {
    return (is_directory_op ? api_error::directory_not_found : api_error::item_not_found);
  }

  if (is_create_op) {
    const auto create_date = std::to_string(utils::get_file_time_now());
    api_meta_map meta = {{META_CREATION, create_date},
                         {META_MODIFIED, create_date},
                         {META_ACCESSED, create_date},
                         {META_WRITTEN, create_date},
#ifdef __APPLE__
                         {META_OSXFLAGS, utils::string::from_uint32(fi->flags)},
                         {META_BACKUP, "0"},
                         {META_CHANGED, create_date},
#endif
                         {META_MODE, utils::string::from_uint32(mode)},
                         {META_UID, utils::string::from_uint32(get_effective_uid())},
                         {META_GID, utils::string::from_uint32(get_effective_gid())}};

    res = provider_.create_file(api_path, meta);
    if ((res != api_error::file_exists) && (res != api_error::success)) {
      return res;
    }
  }

  std::uint64_t handle = 0u;
  if (((res = oft_->Open(api_path, is_directory_op, fi->flags, handle)) != api_error::success)) {
    return res;
  }

  fi->fh = handle;
  if (is_truncate_op) {
    if ((res = ftruncate_impl(api_path, 0, fi)) != api_error::success) {
      oft_->close(handle);
      fi->fh = 0u;
      errno = std::abs(utils::translate_api_error(res));
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

  if (download_manager_) {
    download_manager_->stop();
  }

  polling::instance().stop();

  if (eviction_) {
    eviction_->stop();
  }

  if (oft_) {
    oft_->stop();
  }

  provider_.stop();

  if (directory_cache_) {
    directory_cache_->stop();
  }

  directory_cache_.reset();
  oft_.reset();
  download_manager_.reset();
  eviction_.reset();
  server_.reset();

  event_system::instance().raise<drive_unmounted>(get_mount_location());

  config_.save();

  lock_data_.set_mount_state(false, "", -1);
}

api_error fuse_drive::fallocate_impl(std::string /*api_path*/, int mode, off_t offset, off_t length,
                                     struct fuse_file_info *fi) {
  filesystem_item *fsi = nullptr;
  if (not oft_->get_open_file(fi->fh, fsi)) {
    return api_error::invalid_handle;
  }

  auto res = check_writeable(fsi->open_data[fi->fh], api_error::invalid_handle);
  if (res != api_error::success) {
    return res;
  }

  if ((res = check_open_flags(fsi->open_data[fi->fh], O_WRONLY | O_APPEND,
                              api_error::invalid_handle)) != api_error::success) {
    return res;
  }

  i_download::allocator_callback allocator;

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

    allocator = [&]() -> api_error {
      return fcntl(fsi->handle, F_PREALLOCATE, &fstore) >= 0 ? api_error::success
                                                             : api_error::os_error;
    };
  } else {
    return api_error::not_supported;
  }
#else  // __APPLE__
  allocator = [&]() -> api_error {
    return (fallocate(fsi->handle, mode, offset, length) == -1) ? api_error::os_error
                                                                : api_error::success;
  };
#endif // __APPLE__

  return download_manager_->allocate(fi->fh, *fsi, offset + length, allocator);
}

api_error fuse_drive::fgetattr_impl(std::string api_path, struct stat *st,
                                    struct fuse_file_info *fi) {
  filesystem_item *fsi = nullptr;
  if (not oft_->get_open_file(fi->fh, fsi)) {
    return api_error::invalid_handle;
  }

  std::uint64_t file_size{};
  api_meta_map meta{};
  const auto res = oft_->derive_item_data(api_path, fsi->directory, file_size, meta);
  if (res != api_error::success) {
    return res;
  }

  fuse_base::populate_stat(api_path, file_size, meta, fsi->directory, provider_, st);

  return api_error::success;
}

#ifdef __APPLE__
api_error fuse_drive::fsetattr_x_impl(std::string api_path, struct setattr_x *attr,
                                      struct fuse_file_info *fi) {
  filesystem_item *fsi = nullptr;
  if (not oft_->get_open_file(fi->fh, fsi)) {
    return api_error::invalid_handle;
  }

  return setattr_x_impl(api_path, attr);
}
#endif // __APPLE__

api_error fuse_drive::fsync_impl(std::string /*api_path*/, int datasync,
                                 struct fuse_file_info *fi) {
  filesystem_item *fsi = nullptr;
  if (not oft_->get_open_file(fi->fh, fsi)) {
    return api_error::invalid_handle;
  }

  if (fsi->handle != REPERTORY_INVALID_HANDLE) {
#ifdef __APPLE__
    if ((datasync ? fcntl(fsi->handle, F_FULLFSYNC) : fsync(fsi->handle)) == -1) {
#else  // __APPLE__
    if ((datasync ? fdatasync(fsi->handle) : fsync(fsi->handle)) == -1) {
#endif // __APPLE__
      return api_error::os_error;
    }
  }

  return api_error::success;
}

api_error fuse_drive::ftruncate_impl(std::string /*api_path*/, off_t size,
                                     struct fuse_file_info *fi) {
  filesystem_item *fsi = nullptr;
  if (not oft_->get_open_file(fi->fh, fsi)) {
    return api_error::invalid_handle;
  }

  const auto res = check_writeable(fsi->open_data[fi->fh], api_error::invalid_handle);
  if (res != api_error::success) {
    return res;
  }

  return download_manager_->resize(fi->fh, *fsi, size);
}

std::uint64_t fuse_drive::get_directory_item_count(const std::string &api_path) const {
  return provider_.get_directory_item_count(api_path);
}

directory_item_list fuse_drive::get_directory_items(const std::string &api_path) const {
  directory_item_list di{};
  provider_.get_directory_items(api_path, di);
  return di;
}

std::uint64_t fuse_drive::get_file_size(const std::string &api_path) const {
  std::uint64_t file_size{};
  oft_->derive_file_size(api_path, file_size);
  return file_size;
}

api_error fuse_drive::get_item_meta(const std::string &api_path, api_meta_map &meta) const {
  return oft_->derive_item_data(api_path, meta);
}

api_error fuse_drive::get_item_meta(const std::string &api_path, const std::string &name,
                                    std::string &value) const {
  api_meta_map meta{};
  const auto ret = get_item_meta(api_path, meta);
  if (ret == api_error::success) {
    value = meta[name];
  }

  return ret;
}

api_error fuse_drive::getattr_impl(std::string api_path, struct stat *st) {
  const auto parent = utils::path::get_parent_api_path(api_path);

  auto res = check_parent_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }

  auto found = false;
  directory_cache_->execute_action(parent, [&](directory_iterator &iter) {
    directory_item di{};
    if ((found = (iter.get_directory_item(api_path, di) == api_error::success))) {
      oft_->update_directory_item(di);
      fuse_base::populate_stat(api_path, di.size, di.meta, di.directory, provider_, st);
    }
  });

  if (not found) {
    std::uint64_t file_size{};
    api_meta_map meta{};
    const auto directory = provider_.is_directory(api_path);
    if ((res = oft_->derive_item_data(api_path, directory, file_size, meta)) !=
        api_error::success) {
      return res;
    }
    fuse_base::populate_stat(api_path, file_size, meta, directory, provider_, st);
  }

  return api_error::success;
}

std::uint64_t fuse_drive::get_total_drive_space() const {
  return provider_.get_total_drive_space();
}

std::uint64_t fuse_drive::get_total_item_count() const { return provider_.get_total_item_count(); }

std::uint64_t fuse_drive::get_used_drive_space() const { return provider_.get_used_drive_space(); }

void fuse_drive::get_volume_info(UINT64 &total_size, UINT64 &free_size,
                                 std::string &volume_label) const {
  total_size = provider_.get_total_drive_space();
  free_size = total_size - provider_.get_used_drive_space();
  volume_label = utils::create_volume_label(config_.get_provider_type());
}

#ifdef __APPLE__
api_error fuse_drive::getxtimes_impl(std::string api_path, struct timespec *bkuptime,
                                     struct timespec *crtime) {
  if (not(bkuptime && crtime)) {
    return api_error::bad_address;
  }

  auto res = check_parent_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }

  api_meta_map meta{};
  if ((res = oft_->derive_item_data(api_path, meta)) != api_error::success) {
    return res;
  }

  get_timespec_from_meta(meta, META_CREATION, *crtime);
  get_timespec_from_meta(meta, META_BACKUP, *bkuptime);

  return api_error::success;
}
#endif // __APPLE__

void *fuse_drive::init_impl(struct fuse_conn_info *conn) {
  utils::file::change_to_process_directory();

  if (console_enabled_) {
    console_consumer_ = std::make_unique<console_consumer>();
  }

  logging_consumer_ =
      std::make_unique<logging_consumer>(config_.get_log_directory(), config_.get_event_level());
  event_system::instance().start();
  was_mounted_ = true;

  polling::instance().start(&config_);

  download_manager_ = std::make_unique<download_manager>(
      config_,
      [this](const std::string &path, const std::size_t &size, const std::uint64_t &offset,
             std::vector<char> &data, const bool &stop_requested) -> api_error {
        return provider_.read_file_bytes(path, size, offset, data, stop_requested);
      });

  oft_ = std::make_unique<open_file_table<open_file_data>>(provider_, config_, *download_manager_);

  server_ = std::make_unique<full_server>(config_, provider_, *oft_);

  eviction_ = std::make_unique<eviction>(provider_, config_, *oft_);

  directory_cache_ = std::make_unique<directory_cache>();

  try {
    directory_cache_->start();

    server_->start();

    if (provider_.start(
            [this](const std::string &api_path, const std::string &api_parent,
                   const std::string &source, const bool &directory,
                   const std::uint64_t &create_date, const std::uint64_t &access_data,
                   const std::uint64_t &modified_data, const std::uint64_t &changed_data) -> void {
              event_system::instance().raise<filesystem_item_added>(api_path, api_parent,
                                                                    directory);
              provider_.set_item_meta(
                  api_path,
                  {{META_CREATION, std::to_string(create_date)},
                   {META_MODIFIED, std::to_string(modified_data)},
                   {META_WRITTEN, std::to_string(modified_data)},
                   {META_ACCESSED, std::to_string(access_data)},
                   {META_OSXFLAGS, "0"},
                   {META_BACKUP, "0"},
                   {META_CHANGED, std::to_string(changed_data)},
                   {META_MODE, utils::string::from_uint32(directory ? S_IRUSR | S_IWUSR | S_IXUSR
                                                                    : S_IRUSR | S_IWUSR)},
                   {META_UID, utils::string::from_uint32(getuid())},
                   {META_GID, utils::string::from_uint32(getgid())}});
              if (not directory && not source.empty()) {
                provider_.set_source_path(api_path, source);
              }
            },
            oft_.get())) {
      throw startup_exception("provider is offline");
    }

    // Always start download manager after provider for restore support
    download_manager_->start(oft_.get());

    oft_->start();

    eviction_->start();

    // Force root creation
    api_meta_map meta{};
    provider_.get_item_meta("/", meta);

    if (config_.get_enable_remote_mount()) {
      remote_server_ =
          std::make_unique<remote_fuse::remote_server>(config_, *this, get_mount_location());
    }

    lock_data_.set_mount_state(true, get_mount_location(), getpid());
    event_system::instance().raise<drive_mounted>(get_mount_location());
  } catch (const std::exception &e) {
    event_system::instance().raise<repertory_exception>(__FUNCTION__, e.what());

    destroy_impl(this);

    fuse_exit(fuse_get_context()->fuse);
  }

  return fuse_base::init_impl(conn);
}

bool fuse_drive::is_processing(const std::string &api_path) const {
  return download_manager_->is_processing(api_path);
}

api_error fuse_drive::mkdir_impl(std::string api_path, mode_t mode) {
  auto res = check_parent_access(api_path, W_OK | X_OK);
  if (res != api_error::success) {
    return res;
  }

  const auto create_date = std::to_string(utils::get_file_time_now());
  if ((res = provider_.create_directory(
           api_path, {{META_CREATION, create_date},
                      {META_MODIFIED, create_date},
                      {META_WRITTEN, create_date},
                      {META_ACCESSED, create_date},
                      {META_OSXFLAGS, "0"},
                      {META_BACKUP, "0"},
                      {META_CHANGED, create_date},
                      {META_MODE, utils::string::from_uint32(mode)},
                      {META_UID, utils::string::from_uint32(get_effective_uid())},
                      {META_GID, utils::string::from_uint32(get_effective_gid())}})) !=
      api_error::success) {
    return res;
  }

  if (api_path != "/") {
    oft_->set_item_meta(utils::path::get_parent_api_path(api_path), META_MODIFIED, create_date);
  }

  return api_error::success;
}

api_error fuse_drive::open_impl(std::string api_path, struct fuse_file_info *fi) {
  fi->flags &= (~O_CREAT);
  return create_impl(api_path, 0, fi);
}

api_error fuse_drive::opendir_impl(std::string api_path, struct fuse_file_info *fi) {
  const auto mask = (O_RDONLY != (fi->flags & O_ACCMODE) ? W_OK : R_OK) | X_OK;
  auto res = check_access(api_path, mask);
  if (res != api_error::success) {
    return res;
  }

  if ((res = check_parent_access(api_path, mask)) != api_error::success) {
    return res;
  }

  if (not provider_.is_directory(api_path)) {
    return api_error::item_is_file;
  }

  directory_item_list dl{};
  if ((res = provider_.get_directory_items(api_path, dl)) != api_error::success) {
    return res;
  }

  auto *iter = new directory_iterator(std::move(dl));
  fi->fh = reinterpret_cast<std::uint64_t>(iter);
  directory_cache_->set_directory(api_path, iter);

  return api_error::success;
}

void fuse_drive::populate_stat(const directory_item &di, struct stat &st) const {
  fuse_base::populate_stat(di.api_path, di.size, di.meta, di.directory, provider_, &st);
}

api_error fuse_drive::read_impl(std::string api_path, char *buffer, size_t read_size,
                                off_t read_offset, struct fuse_file_info *fi,
                                std::size_t &bytes_read) {
  filesystem_item *fsi = nullptr;
  if (not oft_->get_open_file(fi->fh, fsi)) {
    return api_error::item_not_found;
  }

  auto res = check_readable(fsi->open_data[fi->fh], api_error::invalid_handle);
  if (res != api_error::success) {
    return res;
  }

  std::vector<char> data;
  res = download_manager_->read_bytes(fi->fh, *fsi, read_size,
                                      static_cast<std::uint64_t>(read_offset), data);
  if ((bytes_read = data.size())) {
    memcpy(buffer, &data[0], data.size());
    data.clear();
    update_accessed_time(api_path);
  }

  return res;
}

api_error fuse_drive::readdir_impl(std::string api_path, void *buf, fuse_fill_dir_t fuse_fill_dir,
                                   off_t offset, struct fuse_file_info *fi) {
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
               offset++, fuse_fill_dir, buf,
               [this](const std::string &api_path, const std::uint64_t &file_size,
                      const api_meta_map &meta, const bool &directory, struct stat *st) {
                 fuse_base::populate_stat(api_path, file_size, meta, directory, provider_, st);
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

api_error fuse_drive::release_impl(std::string /*api_path*/, struct fuse_file_info *fi) {
  oft_->close(fi->fh);
  return api_error::success;
}

api_error fuse_drive::releasedir_impl(std::string /*api_path*/, struct fuse_file_info *fi) {
  auto *iter = reinterpret_cast<directory_iterator *>(fi->fh);
  if (not iter) {
    return api_error::invalid_handle;
  }

  directory_cache_->remove_directory(iter);
  delete iter;

  return api_error::success;
}

int fuse_drive::rename_directory(const std::string &from_api_path, const std::string &to_api_path) {
  const auto res = oft_->rename_directory(from_api_path, to_api_path);
  errno = std::abs(utils::translate_api_error(res));
  return (res == api_error::success) ? 0 : -1;
}

int fuse_drive::rename_file(const std::string &from_api_path, const std::string &to_api_path,
                            const bool &overwrite) {
  const auto res = oft_->rename_file(from_api_path, to_api_path, overwrite);
  errno = std::abs(utils::translate_api_error(res));
  return (res == api_error::success) ? 0 : -1;
}

api_error fuse_drive::rename_impl(std::string from_api_path, std::string to_api_path) {
  auto res = check_parent_access(to_api_path, W_OK | X_OK);
  if (res != api_error::success) {
    return res;
  }

  if ((res = check_parent_access(from_api_path, W_OK | X_OK)) != api_error::success) {
    return res;
  }

  return provider_.is_file(from_api_path) ? oft_->rename_file(from_api_path, to_api_path)
         : provider_.is_directory(from_api_path)
             ? oft_->rename_directory(from_api_path, to_api_path)
             : api_error::item_not_found;
}

api_error fuse_drive::rmdir_impl(std::string api_path) {
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
api_error fuse_drive::getxattr_common(std::string api_path, const char *name, char *value,
                                      size_t size, int &attribute_size, uint32_t *position) {
  std::string attribute_name;
#ifdef __APPLE__
  auto res = parse_xattr_parameters(name, value, size, *position, attribute_name, api_path);
#else  // __APPLE__
  auto res = parse_xattr_parameters(name, value, size, attribute_name, api_path);
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
      utils::path::get_parent_api_path(api_path), [&](directory_iterator &iterator) {
        directory_item di{};
        if ((found = (iterator.get_directory_item(api_path, di) == api_error::success))) {
          oft_->update_directory_item(di);
          meta = di.meta;
        }
      });

  if (found || ((res = oft_->derive_item_data(api_path, meta)) == api_error::success)) {
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
api_error fuse_drive::getxattr_impl(std::string api_path, const char *name, char *value,
                                    size_t size, uint32_t position, int &attribute_size) {
  return getxattr_common(api_path, name, value, size, attribute_size, &position);
}
#else  // __APPLE__
api_error fuse_drive::getxattr_impl(std::string api_path, const char *name, char *value,
                                    size_t size, int &attribute_size) {
  return getxattr_common(api_path, name, value, size, attribute_size, nullptr);
}
#endif // __APPLE__

api_error fuse_drive::listxattr_impl(std::string api_path, char *buffer, size_t size,
                                     int &required_size, bool &return_size) {
  const auto check_size = (size == 0);

  auto res = check_parent_access(api_path, X_OK);
  if (res != api_error::success) {
    return res;
  }

  api_meta_map meta;
  if ((res = oft_->derive_item_data(api_path, meta)) == api_error::success) {
    for (const auto &kv : meta) {
      if (utils::collection_excludes(META_USED_NAMES, kv.first)) {
        auto attribute_name = kv.first;
#ifdef __APPLE__
        if (attribute_name != G_KAUTH_FILESEC_XATTR) {
#endif
          const auto attribute_name_size = attribute_name.size();
          if (size >= attribute_name_size) {
            strncpy(&buffer[required_size], attribute_name.c_str(), attribute_name_size);
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

  return_size =
      ((res == api_error::success) || ((res == api_error::xattr_buffer_small) && check_size));

  return res;
}

api_error fuse_drive::removexattr_impl(std::string api_path, const char *name) {
  std::string attribute_name;
#ifdef __APPLE__
  auto res = parse_xattr_parameters(name, 0, attribute_name, api_path);
#else
  auto res = parse_xattr_parameters(name, attribute_name, api_path);
#endif
  if (res != api_error::success) {
    return res;
  }

  return check_and_perform(api_path, X_OK, [&](api_meta_map &meta) -> api_error {
    if (meta.find(name) != meta.end()) {
      return oft_->remove_xattr_meta(api_path, attribute_name);
    }

    return api_error::xattr_not_found;
  });
}

#ifdef __APPLE__
api_error fuse_drive::setxattr_impl(std::string api_path, const char *name, const char *value,
                                    size_t size, int flags, uint32_t position) {
  a return api_error::not_implemented;
}
#else  // __APPLE__
api_error fuse_drive::setxattr_impl(std::string api_path, const char *name, const char *value,
                                    size_t size, int flags) {
  std::string attribute_name;
  auto res = parse_xattr_parameters(name, value, size, attribute_name, api_path);
  if (res != api_error::success) {
    return res;
  }
  const auto attribute_namespace = utils::string::contains(attribute_name, ".")
                                       ? utils::string::split(attribute_name, '.', false)[0u]
                                       : "";
  if ((attribute_name.size() > XATTR_NAME_MAX) || (size > XATTR_SIZE_MAX)) {
    return api_error::xattr_too_big;
  }

  if (utils::string::contains(attribute_name, " .") ||
      utils::string::contains(attribute_name, ". ") ||
      utils::collection_excludes(utils::attribute_namespaces, attribute_namespace)) {
    return api_error::xattr_invalid_namespace;
  }

  api_meta_map meta;
  if ((res = oft_->derive_item_data(api_path, meta)) != api_error::success) {
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

  return oft_->set_item_meta(api_path, attribute_name, macaron::Base64::Encode(value, size));
}
#endif // __APPLE__
#endif // HAS_SETXATTR

void fuse_drive::set_item_meta(const std::string &api_path, const std::string &key,
                               const std::string &value) {
  oft_->set_item_meta(api_path, key, value);
}

#ifdef __APPLE__
api_error fuse_drive::setattr_x_impl(std::string api_path, struct setattr_x *attr) {
  if (not(provider_.is_file(api_path) || provider_.is_directory(api_path))) {
    return api_error::item_not_found;
  }

  if (SETATTR_WANTS_MODE(attr)) {
    const auto res = chmod_impl(api_path, attr->mode);
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
    const auto res = chown_impl(api_path, uid, gid);
    if (res != api_error::success) {
      return res;
    }
  }

  if (SETATTR_WANTS_SIZE(attr)) {
    const auto res = truncate_impl(api_path, attr->size);
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

    const auto res = utimens_impl(api_path, ts);
    if (res != api_error::success) {
      return res;
    }
  }

  if (SETATTR_WANTS_CRTIME(attr)) {
    const auto res = setcrtime_impl(api_path, &attr->crtime);
    if (res != api_error::success) {
      return res;
    }
  }

  if (SETATTR_WANTS_CHGTIME(attr)) {
    const auto res = setchgtime_impl(api_path, &attr->chgtime);
    if (res != api_error::success) {
      return res;
    }
  }

  if (SETATTR_WANTS_BKUPTIME(attr)) {
    const auto res = setbkuptime_impl(api_path, &attr->bkuptime);
    if (res != api_error::success) {
      return res;
    }
  }

  if (SETATTR_WANTS_FLAGS(attr)) {
    const auto res = chflags_impl(api_path, attr->flags);
    if (res != api_error::success) {
      return res;
    }
  }

  return api_error::success;
}

api_error fuse_drive::setbkuptime_impl(std::string api_path, const struct timespec *bkuptime) {
  return check_and_perform(api_path, X_OK, [&](api_meta_map &meta) -> api_error {
    const auto nanos = bkuptime->tv_nsec + (bkuptime->tv_nsec * NANOS_PER_SECOND);
    return oft_->set_item_meta(api_path, META_BACKUP, utils::string::from_uint64(nanos));
  });
}

api_error fuse_drive::setchgtime_impl(std::string api_path, const struct timespec *chgtime) {
  return check_and_perform(api_path, X_OK, [&](api_meta_map &meta) -> api_error {
    const auto nanos = chgtime->tv_nsec + (chgtime->tv_nsec * NANOS_PER_SECOND);
    return oft_->set_item_meta(api_path, META_CHANGED, utils::string::from_uint64(nanos));
  });
}

api_error fuse_drive::setcrtime_impl(std::string api_path, const struct timespec *crtime) {
  return check_and_perform(api_path, X_OK, [&](api_meta_map &meta) -> api_error {
    const auto nanos = crtime->tv_nsec + (crtime->tv_nsec * NANOS_PER_SECOND);
    return oft_->set_item_meta(api_path, META_CREATION, utils::string::from_uint64(nanos));
  });
}

api_error fuse_drive::setvolname_impl(const char * /*volname*/) { return api_error::success; }

api_error fuse_drive::statfs_x_impl(std::string /*api_path*/, struct statfs *stbuf) {
  if (statfs(&config_.get_cache_directory()[0], stbuf) != 0) {
    return api_error::os_error;
  }

  const auto total_bytes = provider_.get_total_drive_space();
  const auto total_used = provider_.get_used_drive_space();
  const auto used_blocks =
      utils::divide_with_ceiling(total_used, static_cast<std::uint64_t>(stbuf->f_bsize));
  stbuf->f_blocks =
      utils::divide_with_ceiling(total_bytes, static_cast<std::uint64_t>(stbuf->f_bsize));
  stbuf->f_bavail = stbuf->f_bfree = stbuf->f_blocks ? (stbuf->f_blocks - used_blocks) : 0;
  stbuf->f_files = 4294967295;
  stbuf->f_ffree = stbuf->f_files - provider_.get_total_item_count();
  stbuf->f_owner = getuid();
  strncpy(&stbuf->f_mntonname[0], get_mount_location().c_str(), MNAMELEN);
  strncpy(&stbuf->f_mntfromname[0],
          (utils::create_volume_label(config_.get_provider_type())).c_str(), MNAMELEN);

  return api_error::success;
}

#else  // __APPLE__
api_error fuse_drive::statfs_impl(std::string /*api_path*/, struct statvfs *stbuf) {
  if (statvfs(&config_.get_cache_directory()[0], stbuf) != 0) {
    return api_error::os_error;
  }

  const auto total_bytes = provider_.get_total_drive_space();
  const auto total_used = provider_.get_used_drive_space();
  const auto used_blocks =
      utils::divide_with_ceiling(total_used, static_cast<std::uint64_t>(stbuf->f_frsize));
  stbuf->f_files = 4294967295;
  stbuf->f_blocks =
      utils::divide_with_ceiling(total_bytes, static_cast<std::uint64_t>(stbuf->f_frsize));
  stbuf->f_bavail = stbuf->f_bfree = stbuf->f_blocks ? (stbuf->f_blocks - used_blocks) : 0;
  stbuf->f_ffree = stbuf->f_favail = stbuf->f_files - provider_.get_total_item_count();

  return api_error::success;
}
#endif // __APPLE__

api_error fuse_drive::truncate_impl(std::string api_path, off_t size) {
  auto res =
      provider_.is_file_writeable(api_path) ? api_error::success : api_error::permission_denied;
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
  if ((res = oft_->Open(api_path, false, ofd, handle)) != api_error::success) {
    return res;
  }

  const auto cleanup = [this, &handle](const api_error &res) -> api_error {
    oft_->close(handle);
    return res;
  };

  filesystem_item *fsi = nullptr;
  if (not oft_->get_open_file(handle, fsi)) {
    return cleanup(api_error::item_not_found);
  }

  return cleanup(download_manager_->resize(handle, *fsi, size));
}

api_error fuse_drive::unlink_impl(std::string api_path) {
  if (provider_.is_directory(api_path)) {
    return api_error::directory_exists;
  }

  const auto res = check_parent_access(api_path, W_OK | X_OK);
  if (res != api_error::success) {
    return res;
  }

  return oft_->remove_file(api_path);
}

void fuse_drive::update_directory_item(directory_item &di) const {
  oft_->update_directory_item(di);
}

api_error fuse_drive::utimens_impl(std::string api_path, const struct timespec tv[2]) {
  api_meta_map meta;
  auto res = get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  if ((res = check_owner(meta)) != api_error::success) {
    return res;
  }

  meta.clear();
  if (not tv || (tv[0].tv_nsec == UTIME_NOW)) {
    meta.insert({META_ACCESSED, std::to_string(utils::get_file_time_now())});
  } else if (tv[0].tv_nsec != UTIME_OMIT) {
    const auto val = tv[0].tv_nsec + (tv[0].tv_sec * NANOS_PER_SECOND);
    meta.insert({META_ACCESSED, std::to_string(val)});
  }

  if (not tv || (tv[1].tv_nsec == UTIME_NOW)) {
    meta.insert({META_MODIFIED, std::to_string(utils::get_file_time_now())});
  } else if (tv[1].tv_nsec != UTIME_OMIT) {
    const auto val = tv[1].tv_nsec + (tv[1].tv_sec * NANOS_PER_SECOND);
    meta.insert({META_MODIFIED, std::to_string(val)});
  }

  if (not meta.empty()) {
    return oft_->set_item_meta(api_path, meta);
  }

  return api_error::success;
}

api_error fuse_drive::write_impl(std::string /*api_path*/
                                 ,
                                 const char *buffer, size_t write_size, off_t write_offset,
                                 struct fuse_file_info *fi, std::size_t &bytes_written) {
  filesystem_item *fsi = nullptr;
  if (not oft_->get_open_file(fi->fh, fsi)) {
    return api_error::item_not_found;
  }

  auto res = check_writeable(fsi->open_data[fi->fh], api_error::invalid_handle);
  if (res != api_error::success) {
    return res;
  }

  if (write_size > 0) {
    if (fsi->open_data[fi->fh] & O_APPEND) {
      write_offset = fsi->size;
    }

    std::vector<char> data(write_size);
    std::memcpy(&data[0], buffer, write_size);
    return download_manager_->write_bytes(fi->fh, *fsi, static_cast<std::uint64_t>(write_offset),
                                          std::move(data), bytes_written);
  }

  return api_error::success;
}

void fuse_drive::notify_fuse_args_parsed(const std::vector<std::string> &args) {
  event_system::instance().raise<fuse_args_parsed>(std::accumulate(
      args.begin(), args.end(), std::string(), [](std::string command_line, const auto &arg) {
        command_line += (command_line.empty() ? arg : (" " + std::string(arg)));
        return command_line;
      }));
}

void fuse_drive::notify_fuse_main_exit(int &ret) {
  if (was_mounted_) {
    event_system::instance().raise<drive_mount_result>(std::to_string(ret));
    event_system::instance().stop();
    logging_consumer_.reset();
    console_consumer_.reset();
  }
}

int fuse_drive::shutdown() {
  const auto ret = fuse_base::shutdown();
  event_system::instance().raise<unmount_result>(get_mount_location(), std::to_string(ret));
  return ret;
}

void fuse_drive::update_accessed_time(const std::string &api_path) {
  if (atime_enabled_) {
    oft_->set_item_meta(api_path, META_ACCESSED, std::to_string(utils::get_file_time_now()));
  }
}
} // namespace repertory

#endif // _WIN32
