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

#include "drives/fuse/remotefuse/remote_fuse_drive.hpp"

#include "app_config.hpp"
#include "drives/fuse/events.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/consumers/logging_consumer.hpp"
#include "events/event_system.hpp"
#include "events/events.hpp"
#include "platform/platform.hpp"
#include "rpc/server/server.hpp"
#include "types/remote.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/utils.hpp"

namespace repertory::remote_fuse {
auto remote_fuse_drive::access_impl(std::string api_path, int mask)
    -> api_error {
  return utils::to_api_error(
      remote_instance_->fuse_access(api_path.c_str(), mask));
}

#ifdef __APPLE__
api_error remote_fuse_drive::chflags_impl(std::string api_path,
                                          uint32_t flags) {
  return utils::to_api_error(
      remote_instance_->fuse_chflags(api_path.c_str(), flags));
}
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::chmod_impl(std::string api_path, mode_t mode,
                                   struct fuse_file_info * /*fi*/)
    -> api_error {
#else
auto remote_fuse_drive::chmod_impl(std::string api_path, mode_t mode)
    -> api_error {
#endif
  return utils::to_api_error(
      remote_instance_->fuse_chmod(api_path.c_str(), mode));
}

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::chown_impl(std::string api_path, uid_t uid, gid_t gid,
                                   struct fuse_file_info * /*fi*/)
    -> api_error {
#else
auto remote_fuse_drive::chown_impl(std::string api_path, uid_t uid, gid_t gid)
    -> api_error {
#endif
  return utils::to_api_error(
      remote_instance_->fuse_chown(api_path.c_str(), uid, gid));
}

auto remote_fuse_drive::create_impl(std::string api_path, mode_t mode,
                                    struct fuse_file_info *fi) -> api_error {
  return utils::to_api_error(remote_instance_->fuse_create(
      api_path.c_str(), mode, remote::create_open_flags(fi->flags), fi->fh));
}

void remote_fuse_drive::destroy_impl(void * /*ptr*/) {
  event_system::instance().raise<drive_unmount_pending>(get_mount_location());

  if (server_) {
    server_->stop();
    server_.reset();
  }

  if (remote_instance_) {
    const auto res = remote_instance_->fuse_destroy();
    if (res != 0) {
      utils::error::raise_error(__FUNCTION__,
                                "remote fuse_destroy() failed|err|" +
                                    std::to_string(res));
    }
    remote_instance_.reset();
  }

  if (not lock_data_.set_mount_state(false, "", -1)) {
    utils::error::raise_error(__FUNCTION__, "failed to set mount state");
  }

  event_system::instance().raise<drive_unmounted>(get_mount_location());
}

auto remote_fuse_drive::fgetattr_impl(std::string api_path, struct stat *st,
                                      struct fuse_file_info *fi) -> api_error {
  remote::stat r{};
  auto directory = false;

  const auto res =
      remote_instance_->fuse_fgetattr(api_path.c_str(), r, directory, fi->fh);
  if (res == 0) {
    populate_stat(r, directory, *st);
  }

  return utils::to_api_error(res);
}

#ifdef __APPLE__
api_error remote_fuse_drive::fsetattr_x_impl(std::string api_path,
                                             struct setattr_x *attr,
                                             struct fuse_file_info *fi) {
  remote::setattr_x attributes{};
  attributes.valid = attr->valid;
  attributes.mode = attr->mode;
  attributes.uid = attr->uid;
  attributes.gid = attr->gid;
  attributes.size = attr->size;
  attributes.acctime =
      ((attr->acctime.tv_sec * NANOS_PER_SECOND) + attr->acctime.tv_nsec);
  attributes.modtime =
      ((attr->modtime.tv_sec * NANOS_PER_SECOND) + attr->modtime.tv_nsec);
  attributes.crtime =
      ((attr->crtime.tv_sec * NANOS_PER_SECOND) + attr->crtime.tv_nsec);
  attributes.chgtime =
      ((attr->chgtime.tv_sec * NANOS_PER_SECOND) + attr->chgtime.tv_nsec);
  attributes.bkuptime =
      ((attr->bkuptime.tv_sec * NANOS_PER_SECOND) + attr->bkuptime.tv_nsec);
  attributes.flags = attr->flags;
  return utils::to_api_error(
      remote_instance_->fuse_fsetattr_x(api_path.c_str(), attributes, fi->fh));
}
#endif

auto remote_fuse_drive::fsync_impl(std::string api_path, int datasync,
                                   struct fuse_file_info *fi) -> api_error {

  return utils::to_api_error(
      remote_instance_->fuse_fsync(api_path.c_str(), datasync, fi->fh));
}

#if FUSE_USE_VERSION < 30
auto remote_fuse_drive::ftruncate_impl(std::string api_path, off_t size,
                                       struct fuse_file_info *fi) -> api_error {
  return utils::to_api_error(
      remote_instance_->fuse_ftruncate(api_path.c_str(), size, fi->fh));
}
#endif

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::getattr_impl(std::string api_path, struct stat *st,
                                     struct fuse_file_info * /*fi*/)
    -> api_error {
#else
auto remote_fuse_drive::getattr_impl(std::string api_path, struct stat *st)
    -> api_error {
#endif
  bool directory = false;
  remote::stat r{};

  const auto res =
      remote_instance_->fuse_getattr(api_path.c_str(), r, directory);
  if (res == 0) {
    populate_stat(r, directory, *st);
  }

  return utils::to_api_error(res);
}

#ifdef __APPLE__
api_error remote_fuse_drive::getxtimes_impl(std::string api_path,
                                            struct timespec *bkuptime,
                                            struct timespec *crtime) {
  if (not(bkuptime && crtime)) {
    return utils::to_api_error(-EFAULT);
  }

  remote::file_time repertory_bkuptime = 0u;
  remote::file_time repertory_crtime = 0u;
  const auto res = remote_instance_->fuse_getxtimes(
      api_path.c_str(), repertory_bkuptime, repertory_crtime);
  if (res == 0) {
    bkuptime->tv_nsec =
        static_cast<long>(repertory_bkuptime % NANOS_PER_SECOND);
    bkuptime->tv_sec = repertory_bkuptime / NANOS_PER_SECOND;
    crtime->tv_nsec = static_cast<long>(repertory_crtime % NANOS_PER_SECOND);
    crtime->tv_sec = repertory_crtime / NANOS_PER_SECOND;
  }

  return utils::to_api_error(res);
}
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::init_impl(struct fuse_conn_info *conn,
                                  struct fuse_config *cfg) -> void * {
#else
auto remote_fuse_drive::init_impl(struct fuse_conn_info *conn) -> void * {
#endif
  utils::file::change_to_process_directory();
  was_mounted_ = true;

  if (console_enabled_) {
    console_consumer_ = std::make_shared<console_consumer>();
  }
  logging_consumer_ = std::make_shared<logging_consumer>(
      config_.get_log_directory(), config_.get_event_level());
  event_system::instance().start();

  if (not lock_data_.set_mount_state(true, get_mount_location(), getpid())) {
    utils::error::raise_error(__FUNCTION__, "failed to set mount state");
  }

  remote_instance_ = factory_();
  remote_instance_->set_fuse_uid_gid(getuid(), getgid());

  if (remote_instance_->fuse_init() != 0) {
    utils::error::raise_error(__FUNCTION__,
                              "failed to connect to remote server");
    event_system::instance().raise<unmount_requested>();
  } else {
    server_ = std::make_shared<server>(config_);
    server_->start();
    event_system::instance().raise<drive_mounted>(get_mount_location());
  }

#if FUSE_USE_VERSION >= 30
  return fuse_base::init_impl(conn, cfg);
#else
  return fuse_base::init_impl(conn);
#endif
}

auto remote_fuse_drive::mkdir_impl(std::string api_path, mode_t mode)
    -> api_error {
  return utils::to_api_error(
      remote_instance_->fuse_mkdir(api_path.c_str(), mode));
}

void remote_fuse_drive::notify_fuse_main_exit(int &ret) {
  if (was_mounted_) {
    event_system::instance().raise<drive_mount_result>(std::to_string(ret));
    event_system::instance().stop();
    logging_consumer_.reset();
    console_consumer_.reset();
  }
}

auto remote_fuse_drive::open_impl(std::string api_path,
                                  struct fuse_file_info *fi) -> api_error {
  return utils::to_api_error(remote_instance_->fuse_open(
      api_path.c_str(), remote::create_open_flags(fi->flags), fi->fh));
}

auto remote_fuse_drive::opendir_impl(std::string api_path,
                                     struct fuse_file_info *fi) -> api_error {

  return utils::to_api_error(
      remote_instance_->fuse_opendir(api_path.c_str(), fi->fh));
}

void remote_fuse_drive::populate_stat(const remote::stat &r, bool directory,
                                      struct stat &st) {
  memset(&st, 0, sizeof(struct stat));

#ifdef __APPLE__
  st.st_blksize = 0;

  st.st_atimespec.tv_nsec = r.st_atimespec % NANOS_PER_SECOND;
  st.st_atimespec.tv_sec = r.st_atimespec / NANOS_PER_SECOND;

  st.st_birthtimespec.tv_nsec = r.st_birthtimespec % NANOS_PER_SECOND;
  st.st_birthtimespec.tv_sec = r.st_birthtimespec / NANOS_PER_SECOND;

  st.st_ctimespec.tv_nsec = r.st_ctimespec % NANOS_PER_SECOND;
  st.st_ctimespec.tv_sec = r.st_ctimespec / NANOS_PER_SECOND;

  st.st_mtimespec.tv_nsec = r.st_mtimespec % NANOS_PER_SECOND;
  st.st_mtimespec.tv_sec = r.st_mtimespec / NANOS_PER_SECOND;

  st.st_flags = r.st_flags;
#else
  st.st_blksize = 4096;

  st.st_atim.tv_nsec = r.st_atimespec % NANOS_PER_SECOND;
  st.st_atim.tv_sec = r.st_atimespec / NANOS_PER_SECOND;

  st.st_ctim.tv_nsec = r.st_ctimespec % NANOS_PER_SECOND;
  st.st_ctim.tv_sec = r.st_ctimespec / NANOS_PER_SECOND;

  st.st_mtim.tv_nsec = r.st_mtimespec % NANOS_PER_SECOND;
  st.st_mtim.tv_sec = r.st_mtimespec / NANOS_PER_SECOND;
#endif
  if (not directory) {
    const auto block_size_stat = static_cast<std::uint64_t>(512u);
    const auto block_size = static_cast<std::uint64_t>(4096u);
    const auto size = utils::divide_with_ceiling(
                          static_cast<std::uint64_t>(st.st_size), block_size) *
                      block_size;
    st.st_blocks = std::max(block_size / block_size_stat,
                            utils::divide_with_ceiling(size, block_size_stat));
  }

  st.st_gid = r.st_gid;
  st.st_mode = (directory ? S_IFDIR : S_IFREG) | r.st_mode;

  st.st_nlink = r.st_nlink;
  st.st_size = r.st_size;
  st.st_uid = r.st_uid;
}

auto remote_fuse_drive::read_impl(std::string api_path, char *buffer,
                                  size_t read_size, off_t read_offset,
                                  struct fuse_file_info *fi,
                                  std::size_t &bytes_read) -> api_error {
  auto res = remote_instance_->fuse_read(api_path.c_str(), buffer, read_size,
                                         read_offset, fi->fh);
  if (res >= 0) {
    bytes_read = res;
    return api_error::success;
  }

  return utils::to_api_error(res);
}

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::readdir_impl(std::string api_path, void *buf,
                                     fuse_fill_dir_t fuse_fill_dir,
                                     off_t offset, struct fuse_file_info *fi,
                                     fuse_readdir_flags /*flags*/)
    -> api_error {
#else
auto remote_fuse_drive::readdir_impl(std::string api_path, void *buf,
                                     fuse_fill_dir_t fuse_fill_dir,
                                     off_t offset, struct fuse_file_info *fi)
    -> api_error {
#endif
  std::string item_path;
  int res = 0;
  while ((res = remote_instance_->fuse_readdir(api_path.c_str(), offset, fi->fh,
                                               item_path)) == 0) {
    if ((item_path != ".") && (item_path != "..")) {
      item_path = utils::path::strip_to_file_name(item_path);
    }

#if FUSE_USE_VERSION >= 30
    if (fuse_fill_dir(buf, item_path.c_str(), nullptr, ++offset,
                      static_cast<fuse_fill_dir_flags>(0)) != 0) {
#else
    if (fuse_fill_dir(buf, item_path.c_str(), nullptr, ++offset) != 0) {
#endif
      break;
    }
  }

  if (res == -120) {
    res = 0;
  }

  return utils::to_api_error(res);
}

auto remote_fuse_drive::release_impl(std::string api_path,
                                     struct fuse_file_info *fi) -> api_error {
  return utils::to_api_error(
      remote_instance_->fuse_release(api_path.c_str(), fi->fh));
}

auto remote_fuse_drive::releasedir_impl(std::string api_path,
                                        struct fuse_file_info *fi)
    -> api_error {
  return utils::to_api_error(
      remote_instance_->fuse_releasedir(api_path.c_str(), fi->fh));
}

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::rename_impl(std::string from_api_path,
                                    std::string to_api_path,
                                    unsigned int /*flags*/) -> api_error {
#else
auto remote_fuse_drive::rename_impl(std::string from_api_path,
                                    std::string to_api_path) -> api_error {
#endif
  return utils::to_api_error(remote_instance_->fuse_rename(
      from_api_path.c_str(), to_api_path.c_str()));
}

auto remote_fuse_drive::rmdir_impl(std::string api_path) -> api_error {
  return utils::to_api_error(remote_instance_->fuse_rmdir(api_path.c_str()));
}

#ifdef __APPLE__
api_error remote_fuse_drive::setattr_x_impl(std::string api_path,
                                            struct setattr_x *attr) {
  remote::setattr_x attributes{};
  attributes.valid = attr->valid;
  attributes.mode = attr->mode;
  attributes.uid = attr->uid;
  attributes.gid = attr->gid;
  attributes.size = attr->size;
  attributes.acctime =
      ((attr->acctime.tv_sec * NANOS_PER_SECOND) + attr->acctime.tv_nsec);
  attributes.modtime =
      ((attr->modtime.tv_sec * NANOS_PER_SECOND) + attr->modtime.tv_nsec);
  attributes.crtime =
      ((attr->crtime.tv_sec * NANOS_PER_SECOND) + attr->crtime.tv_nsec);
  attributes.chgtime =
      ((attr->chgtime.tv_sec * NANOS_PER_SECOND) + attr->chgtime.tv_nsec);
  attributes.bkuptime =
      ((attr->bkuptime.tv_sec * NANOS_PER_SECOND) + attr->bkuptime.tv_nsec);
  attributes.flags = attr->flags;
  return utils::to_api_error(
      remote_instance_->fuse_setattr_x(api_path.c_str(), attributes));
}

api_error remote_fuse_drive::setbkuptime_impl(std::string api_path,
                                              const struct timespec *bkuptime) {
  remote::file_time repertory_bkuptime =
      ((bkuptime->tv_sec * NANOS_PER_SECOND) + bkuptime->tv_nsec);
  return utils::to_api_error(
      remote_instance_->fuse_setbkuptime(api_path.c_str(), repertory_bkuptime));
}

api_error remote_fuse_drive::setchgtime_impl(std::string api_path,
                                             const struct timespec *chgtime) {
  remote::file_time repertory_chgtime =
      ((chgtime->tv_sec * NANOS_PER_SECOND) + chgtime->tv_nsec);
  return utils::to_api_error(
      remote_instance_->fuse_setchgtime(api_path.c_str(), repertory_chgtime));
}

api_error remote_fuse_drive::setcrtime_impl(std::string api_path,
                                            const struct timespec *crtime) {
  remote::file_time repertory_crtime =
      ((crtime->tv_sec * NANOS_PER_SECOND) + crtime->tv_nsec);
  return utils::to_api_error(
      remote_instance_->fuse_setcrtime(api_path.c_str(), repertory_crtime));
}

api_error remote_fuse_drive::setvolname_impl(const char *volname) {
  return utils::to_api_error(remote_instance_->fuse_setvolname(volname));
}

api_error remote_fuse_drive::statfs_x_impl(std::string api_path,
                                           struct statfs *stbuf) {
  auto res = statfs(config_.get_data_directory().c_str(), stbuf);
  if (res == 0) {
    remote::statfs_x r{};
    if ((res = remote_instance_->fuse_statfs_x(api_path.c_str(), stbuf->f_bsize,
                                               r)) == 0) {
      stbuf->f_blocks = r.f_blocks;
      stbuf->f_bavail = r.f_bavail;
      stbuf->f_bfree = r.f_bfree;
      stbuf->f_ffree = r.f_ffree;
      stbuf->f_files = r.f_files;
      stbuf->f_owner = getuid();
      strncpy(&stbuf->f_mntonname[0u], get_mount_location().c_str(), MNAMELEN);
      strncpy(&stbuf->f_mntfromname[0u], &r.f_mntfromname[0], MNAMELEN);
    }
  } else {
    res = -errno;
  }

  return utils::to_api_error(res);
}
#else  // __APPLE__
auto remote_fuse_drive::statfs_impl(std::string api_path, struct statvfs *stbuf)
    -> api_error {
  auto res = statvfs(config_.get_data_directory().c_str(), stbuf);
  if (res == 0) {
    remote::statfs r{};
    if ((res = remote_instance_->fuse_statfs(api_path.c_str(), stbuf->f_frsize,
                                             r)) == 0) {
      stbuf->f_blocks = r.f_blocks;
      stbuf->f_bavail = r.f_bavail;
      stbuf->f_bfree = r.f_bfree;
      stbuf->f_ffree = r.f_ffree;
      stbuf->f_favail = r.f_favail;
      stbuf->f_files = r.f_files;
    }
  } else {
    res = -errno;
  }

  return utils::to_api_error(res);
}
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::truncate_impl(std::string api_path, off_t size,
                                      struct fuse_file_info * /*fi*/)
    -> api_error {
#else
auto remote_fuse_drive::truncate_impl(std::string api_path, off_t size)
    -> api_error {
#endif
  return utils::to_api_error(
      remote_instance_->fuse_truncate(api_path.c_str(), size));
}

auto remote_fuse_drive::unlink_impl(std::string api_path) -> api_error {
  return utils::to_api_error(remote_instance_->fuse_unlink(api_path.c_str()));
}

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::utimens_impl(std::string api_path,
                                     const struct timespec tv[2],
                                     struct fuse_file_info * /*fi*/)
    -> api_error {
#else
auto remote_fuse_drive::utimens_impl(std::string api_path,
                                     const struct timespec tv[2]) -> api_error {
#endif
  remote::file_time rtv[2u] = {0};
  if (tv) {
    rtv[0u] = tv[0u].tv_nsec + (tv[0u].tv_sec * NANOS_PER_SECOND);
    rtv[1u] = tv[1u].tv_nsec + (tv[1u].tv_sec * NANOS_PER_SECOND);
  }

  return utils::to_api_error(remote_instance_->fuse_utimens(
      api_path.c_str(), rtv, tv ? tv[0u].tv_nsec : 0u,
      tv ? tv[1u].tv_nsec : 0u));
}

auto remote_fuse_drive::write_impl(std::string api_path, const char *buffer,
                                   size_t write_size, off_t write_offset,
                                   struct fuse_file_info *fi,
                                   std::size_t &bytes_written) -> api_error {
  const auto res = remote_instance_->fuse_write(
      api_path.c_str(), buffer, write_size, write_offset, fi->fh);
  if (res >= 0) {
    bytes_written = res;
    return api_error::success;
  }

  return utils::to_api_error(res);
}
} // namespace repertory::remote_fuse

#endif // _WIN32
