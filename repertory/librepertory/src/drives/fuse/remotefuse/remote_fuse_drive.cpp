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

#include "drives/fuse/remotefuse/remote_fuse_drive.hpp"

#include "app_config.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/consumers/logging_consumer.hpp"
#include "events/event_system.hpp"
#include "events/types/drive_mount_result.hpp"
#include "events/types/drive_mounted.hpp"
#include "events/types/drive_unmount_pending.hpp"
#include "events/types/drive_unmounted.hpp"
#include "events/types/unmount_requested.hpp"
#include "platform/platform.hpp"
#include "rpc/server/server.hpp"
#include "types/remote.hpp"
#include "utils/common.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"
#include "utils/time.hpp"
#include "utils/utils.hpp"

namespace repertory::remote_fuse {
auto remote_fuse_drive::access_impl(std::string api_path, int mask)
    -> api_error {
  return utils::to_api_error(
      remote_instance_->fuse_access(api_path.c_str(), mask));
}

#if defined(__APPLE__)
api_error remote_fuse_drive::chflags_impl(std::string api_path,
                                          uint32_t flags) {
  return utils::to_api_error(
      remote_instance_->fuse_chflags(api_path.c_str(), flags));
}
#endif // defined(__APPLE__)

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::chmod_impl(std::string api_path, mode_t mode,
                                   struct fuse_file_info * /*f_info*/)
    -> api_error {
#else  // FUSE_USE_VERSION < 30
auto remote_fuse_drive::chmod_impl(std::string api_path, mode_t mode)
    -> api_error {
#endif // FUSE_USE_VERSION >= 30
  return utils::to_api_error(remote_instance_->fuse_chmod(
      api_path.c_str(), static_cast<remote::file_mode>(mode)));
}

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::chown_impl(std::string api_path, uid_t uid, gid_t gid,
                                   struct fuse_file_info * /*f_info*/)
    -> api_error {
#else  // FUSE_USE_VERSION < 30
auto remote_fuse_drive::chown_impl(std::string api_path, uid_t uid, gid_t gid)
    -> api_error {
#endif // FUSE_USE_VERSION >= 30
  return utils::to_api_error(
      remote_instance_->fuse_chown(api_path.c_str(), uid, gid));
}

auto remote_fuse_drive::create_impl(std::string api_path, mode_t mode,
                                    struct fuse_file_info *f_info)
    -> api_error {
  return utils::to_api_error(remote_instance_->fuse_create(
      api_path.c_str(), static_cast<remote::file_mode>(mode),
      remote::create_open_flags(static_cast<std::uint32_t>(f_info->flags)),
      f_info->fh));
}

void remote_fuse_drive::destroy_impl(void *ptr) {
  REPERTORY_USES_FUNCTION_NAME();

  event_system::instance().raise<drive_unmount_pending>(function_name,
                                                        get_mount_location());

  if (server_) {
    server_->stop();
    server_.reset();
  }

  if (remote_instance_) {
    auto res = remote_instance_->fuse_destroy();
    if (res != 0) {
      utils::error::raise_error(function_name,
                                "remote fuse_destroy() failed|err|" +
                                    std::to_string(res));
    }
    remote_instance_.reset();
  }

  if (not lock_data_.set_mount_state(false, "", -1)) {
    utils::error::raise_error(function_name, "failed to set mount state");
  }

  event_system::instance().raise<drive_unmounted>(function_name,
                                                  get_mount_location());

  fuse_base::destroy_impl(ptr);
}

auto remote_fuse_drive::fgetattr_impl(std::string api_path,
                                      struct stat *unix_st,
                                      struct fuse_file_info *f_info)
    -> api_error {
  remote::stat r_stat{};
  auto directory = false;

  auto res = remote_instance_->fuse_fgetattr(api_path.c_str(), r_stat,
                                             directory, f_info->fh);
  if (res == 0) {
    populate_stat(r_stat, directory, *unix_st);
  }

  return utils::to_api_error(res);
}

#if defined(__APPLE__)
api_error remote_fuse_drive::fsetattr_x_impl(std::string api_path,
                                             struct setattr_x *attr,
                                             struct fuse_file_info *f_info) {
  remote::setattr_x attributes{};
  attributes.valid = attr->valid;
  attributes.mode = attr->mode;
  attributes.uid = attr->uid;
  attributes.gid = attr->gid;
  attributes.size = attr->size;
  attributes.acctime = ((attr->acctime.tv_sec * utils::time::NANOS_PER_SECOND) +
                        attr->acctime.tv_nsec);
  attributes.modtime = ((attr->modtime.tv_sec * utils::time::NANOS_PER_SECOND) +
                        attr->modtime.tv_nsec);
  attributes.crtime = ((attr->crtime.tv_sec * utils::time::NANOS_PER_SECOND) +
                       attr->crtime.tv_nsec);
  attributes.chgtime = ((attr->chgtime.tv_sec * utils::time::NANOS_PER_SECOND) +
                        attr->chgtime.tv_nsec);
  attributes.bkuptime =
      ((attr->bkuptime.tv_sec * utils::time::NANOS_PER_SECOND) +
       attr->bkuptime.tv_nsec);
  attributes.flags = attr->flags;
  return utils::to_api_error(remote_instance_->fuse_fsetattr_x(
      api_path.c_str(), attributes, f_info->fh));
}
#endif // defined(__APPLE__)

auto remote_fuse_drive::fsync_impl(std::string api_path, int datasync,
                                   struct fuse_file_info *f_info) -> api_error {

  return utils::to_api_error(
      remote_instance_->fuse_fsync(api_path.c_str(), datasync, f_info->fh));
}

#if FUSE_USE_VERSION < 30
auto remote_fuse_drive::ftruncate_impl(std::string api_path, off_t size,
                                       struct fuse_file_info *f_info)
    -> api_error {
  return utils::to_api_error(
      remote_instance_->fuse_ftruncate(api_path.c_str(), size, f_info->fh));
}
#endif // FUSE_USE_VERSION < 30

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::getattr_impl(std::string api_path, struct stat *unix_st,
                                     struct fuse_file_info * /*f_info*/)
    -> api_error {
#else  // FUSE_USE_VERSION < 30
auto remote_fuse_drive::getattr_impl(std::string api_path, struct stat *unix_st)
    -> api_error {
#endif // FUSE_USE_VERSION >= 30
  bool directory = false;
  remote::stat r_stat{};

  auto res =
      remote_instance_->fuse_getattr(api_path.c_str(), r_stat, directory);
  if (res == 0) {
    populate_stat(r_stat, directory, *unix_st);
  }

  return utils::to_api_error(res);
}

#if defined(__APPLE__)
api_error remote_fuse_drive::getxtimes_impl(std::string api_path,
                                            struct timespec *bkuptime,
                                            struct timespec *crtime) {
  if (not(bkuptime && crtime)) {
    return utils::to_api_error(-EFAULT);
  }

  remote::file_time repertory_bkuptime{0U};
  remote::file_time repertory_crtime{0U};
  auto res = remote_instance_->fuse_getxtimes(
      api_path.c_str(), repertory_bkuptime, repertory_crtime);
  if (res == 0) {
    bkuptime->tv_nsec =
        static_cast<long>(repertory_bkuptime % utils::time::NANOS_PER_SECOND);
    bkuptime->tv_sec = repertory_bkuptime / utils::time::NANOS_PER_SECOND;
    crtime->tv_nsec =
        static_cast<long>(repertory_crtime % utils::time::NANOS_PER_SECOND);
    crtime->tv_sec = repertory_crtime / utils::time::NANOS_PER_SECOND;
  }

  return utils::to_api_error(res);
}
#endif // defined(__APPLE__)

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::init_impl(struct fuse_conn_info *conn,
                                  struct fuse_config *cfg) -> void * {
#else  // FUSE_USE_VERSION < 30
auto remote_fuse_drive::init_impl(struct fuse_conn_info *conn) -> void * {
#endif // FUSE_USE_VERSION >= 30
  REPERTORY_USES_FUNCTION_NAME();

#if FUSE_USE_VERSION >= 30
  auto *ret = fuse_base::init_impl(conn, cfg);
#else  // FUSE_USE_VERSION < 30
  auto *ret = fuse_base::init_impl(conn);
#endif // FUSE_USE_VERSION >= 30

  was_mounted_ = true;

  if (console_enabled_) {
    console_consumer_ =
        std::make_shared<console_consumer>(config_.get_event_level());
  }
  logging_consumer_ = std::make_shared<logging_consumer>(
      config_.get_event_level(), config_.get_log_directory());
  event_system::instance().start();

  if (not lock_data_.set_mount_state(true, get_mount_location(), getpid())) {
    utils::error::raise_error(function_name, "failed to set mount state");
  }

  remote_instance_ = factory_();
  remote_instance_->set_fuse_uid_gid(getuid(), getgid());

  if (remote_instance_->fuse_init() != 0) {
    utils::error::raise_error(function_name,
                              "failed to connect to remote server");
    event_system::instance().raise<unmount_requested>(function_name);
  } else {
    server_ = std::make_shared<server>(config_);
    server_->start();
    event_system::instance().raise<drive_mounted>(function_name,
                                                  get_mount_location());
  }

  return ret;
}

auto remote_fuse_drive::mkdir_impl(std::string api_path, mode_t mode)
    -> api_error {
  return utils::to_api_error(remote_instance_->fuse_mkdir(
      api_path.c_str(), static_cast<remote::file_mode>(mode)));
}

void remote_fuse_drive::notify_fuse_main_exit(int &ret) {
  REPERTORY_USES_FUNCTION_NAME();

  if (was_mounted_) {
    event_system::instance().raise<drive_mount_result>(
        function_name, get_mount_location(), std::to_string(ret));
    event_system::instance().stop();
    logging_consumer_.reset();
    console_consumer_.reset();
  }
}

auto remote_fuse_drive::open_impl(std::string api_path,
                                  struct fuse_file_info *f_info) -> api_error {
  return utils::to_api_error(remote_instance_->fuse_open(
      api_path.c_str(),
      remote::create_open_flags(static_cast<std::uint32_t>(f_info->flags)),
      f_info->fh));
}

auto remote_fuse_drive::opendir_impl(std::string api_path,
                                     struct fuse_file_info *f_info)
    -> api_error {
  if ((f_info->flags & O_APPEND) == O_APPEND ||
      (f_info->flags & O_EXCL) == O_EXCL) {
    return api_error::directory_exists;
  }

  return utils::to_api_error(
      remote_instance_->fuse_opendir(api_path.c_str(), f_info->fh));
}

void remote_fuse_drive::populate_stat(const remote::stat &r_stat,
                                      bool directory, struct stat &unix_st) {
  std::memset(&unix_st, 0, sizeof(struct stat));
  unix_st.st_blksize =
      static_cast<decltype(unix_st.st_blksize)>(r_stat.st_blksize);
  unix_st.st_blocks =
      static_cast<decltype(unix_st.st_blocks)>(r_stat.st_blocks);
  unix_st.st_gid = r_stat.st_gid;
  unix_st.st_mode = (directory ? S_IFDIR : S_IFREG) | r_stat.st_mode;
  unix_st.st_nlink = r_stat.st_nlink;
  unix_st.st_size = static_cast<decltype(unix_st.st_size)>(r_stat.st_size);
  unix_st.st_uid = r_stat.st_uid;

#if defined(__APPLE__)
  unix_st.st_atimespec.tv_nsec =
      r_stat.st_atimespec % utils::time::NANOS_PER_SECOND;
  unix_st.st_atimespec.tv_sec =
      r_stat.st_atimespec / utils::time::NANOS_PER_SECOND;

  unix_st.st_birthtimespec.tv_nsec =
      r_stat.st_birthtimespec % utils::time::NANOS_PER_SECOND;
  unix_st.st_birthtimespec.tv_sec =
      r_stat.st_birthtimespec / utils::time::NANOS_PER_SECOND;

  unix_st.st_ctimespec.tv_nsec =
      r_stat.st_ctimespec % utils::time::NANOS_PER_SECOND;
  unix_st.st_ctimespec.tv_sec =
      r_stat.st_ctimespec / utils::time::NANOS_PER_SECOND;

  unix_st.st_mtimespec.tv_nsec =
      r_stat.st_mtimespec % utils::time::NANOS_PER_SECOND;
  unix_st.st_mtimespec.tv_sec =
      r_stat.st_mtimespec / utils::time::NANOS_PER_SECOND;

  unix_st.st_flags = r_stat.st_flags;
#else  // !defined(__APPLE__)
  unix_st.st_atim.tv_nsec = static_cast<suseconds_t>(
      r_stat.st_atimespec % utils::time::NANOS_PER_SECOND);
  unix_st.st_atim.tv_sec = static_cast<suseconds_t>(
      r_stat.st_atimespec / utils::time::NANOS_PER_SECOND);

  unix_st.st_ctim.tv_nsec = static_cast<suseconds_t>(
      r_stat.st_ctimespec % utils::time::NANOS_PER_SECOND);
  unix_st.st_ctim.tv_sec = static_cast<suseconds_t>(
      r_stat.st_ctimespec / utils::time::NANOS_PER_SECOND);

  unix_st.st_mtim.tv_nsec = static_cast<suseconds_t>(
      r_stat.st_mtimespec % utils::time::NANOS_PER_SECOND);
  unix_st.st_mtim.tv_sec = static_cast<suseconds_t>(
      r_stat.st_mtimespec / utils::time::NANOS_PER_SECOND);
#endif // defined(__APPLE__)
}

auto remote_fuse_drive::read_impl(std::string api_path, char *buffer,
                                  size_t read_size, off_t read_offset,
                                  struct fuse_file_info *f_info,
                                  std::size_t &bytes_read) -> api_error {
  auto res = remote_instance_->fuse_read(
      api_path.c_str(), buffer, read_size,
      static_cast<remote::file_offset>(read_offset), f_info->fh);
  if (res >= 0) {
    bytes_read = static_cast<size_t>(res);
    return api_error::success;
  }

  return utils::to_api_error(res);
}

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::readdir_impl(std::string api_path, void *buf,
                                     fuse_fill_dir_t fuse_fill_dir,
                                     off_t offset,
                                     struct fuse_file_info *f_info,
                                     fuse_readdir_flags /* flags */)
    -> api_error {
#else  // FUSE_USE_VERSION < 30
auto remote_fuse_drive::readdir_impl(std::string api_path, void *buf,
                                     fuse_fill_dir_t fuse_fill_dir,
                                     off_t offset,
                                     struct fuse_file_info *f_info)
    -> api_error {
#endif // FUSE_USE_VERSION >= 30

  std::string item_path;
  int res{0};
  while ((res = remote_instance_->fuse_readdir(
              api_path.c_str(), static_cast<remote::file_offset>(offset),
              f_info->fh, item_path)) == 0) {
    std::unique_ptr<struct stat> p_stat{nullptr};
    int stat_res{0};
    if ((item_path == ".") || (item_path == "..")) {
      p_stat = std::make_unique<struct stat>();
      std::memset(p_stat.get(), 0, sizeof(struct stat));
      if (item_path == ".") {
        stat_res =
            stat(utils::path::combine(get_mount_location(), {api_path}).c_str(),
                 p_stat.get());
      } else {
        stat_res =
            stat(utils::path::get_parent_path(
                     utils::path::combine(get_mount_location(), {api_path}))
                     .c_str(),
                 p_stat.get());
      }

      if (stat_res != 0) {
        res = stat_res;
        break;
      }
    } else {
      item_path = utils::path::strip_to_file_name(item_path);
    }

#if FUSE_USE_VERSION >= 30
    if (fuse_fill_dir(buf, item_path.c_str(), p_stat.get(), ++offset,
                      FUSE_FILL_DIR_PLUS) != 0) {
#else  // FUSE_USE_VERSION < 30
    if (fuse_fill_dir(buf, item_path.c_str(), p_stat.get(), ++offset) != 0) {
#endif // FUSE_USE_VERSION >= 30
      break;
    }
  }

  if (res == -120) {
    res = 0;
  }

  return utils::to_api_error(res);
}

auto remote_fuse_drive::release_impl(std::string api_path,
                                     struct fuse_file_info *f_info)
    -> api_error {
  return utils::to_api_error(
      remote_instance_->fuse_release(api_path.c_str(), f_info->fh));
}

auto remote_fuse_drive::releasedir_impl(std::string api_path,
                                        struct fuse_file_info *f_info)
    -> api_error {
  return utils::to_api_error(
      remote_instance_->fuse_releasedir(api_path.c_str(), f_info->fh));
}

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::rename_impl(std::string from_api_path,
                                    std::string to_api_path,
                                    unsigned int /*flags*/) -> api_error {
#else  // FUSE_USE_VERSION < 30
auto remote_fuse_drive::rename_impl(std::string from_api_path,
                                    std::string to_api_path) -> api_error {
#endif // FUSE_USE_VERSION >= 30
  return utils::to_api_error(remote_instance_->fuse_rename(
      from_api_path.c_str(), to_api_path.c_str()));
}

auto remote_fuse_drive::rmdir_impl(std::string api_path) -> api_error {
  return utils::to_api_error(remote_instance_->fuse_rmdir(api_path.c_str()));
}

#if defined(__APPLE__)
api_error remote_fuse_drive::setattr_x_impl(std::string api_path,
                                            struct setattr_x *attr) {
  remote::setattr_x attributes{};
  attributes.valid = attr->valid;
  attributes.mode = attr->mode;
  attributes.uid = attr->uid;
  attributes.gid = attr->gid;
  attributes.size = attr->size;
  attributes.acctime = ((attr->acctime.tv_sec * utils::time::NANOS_PER_SECOND) +
                        attr->acctime.tv_nsec);
  attributes.modtime = ((attr->modtime.tv_sec * utils::time::NANOS_PER_SECOND) +
                        attr->modtime.tv_nsec);
  attributes.crtime = ((attr->crtime.tv_sec * utils::time::NANOS_PER_SECOND) +
                       attr->crtime.tv_nsec);
  attributes.chgtime = ((attr->chgtime.tv_sec * utils::time::NANOS_PER_SECOND) +
                        attr->chgtime.tv_nsec);
  attributes.bkuptime =
      ((attr->bkuptime.tv_sec * utils::time::NANOS_PER_SECOND) +
       attr->bkuptime.tv_nsec);
  attributes.flags = attr->flags;
  return utils::to_api_error(
      remote_instance_->fuse_setattr_x(api_path.c_str(), attributes));
}

api_error remote_fuse_drive::setbkuptime_impl(std::string api_path,
                                              const struct timespec *bkuptime) {
  remote::file_time repertory_bkuptime =
      ((bkuptime->tv_sec * utils::time::NANOS_PER_SECOND) + bkuptime->tv_nsec);
  return utils::to_api_error(
      remote_instance_->fuse_setbkuptime(api_path.c_str(), repertory_bkuptime));
}

api_error remote_fuse_drive::setchgtime_impl(std::string api_path,
                                             const struct timespec *chgtime) {
  remote::file_time repertory_chgtime =
      ((chgtime->tv_sec * utils::time::NANOS_PER_SECOND) + chgtime->tv_nsec);
  return utils::to_api_error(
      remote_instance_->fuse_setchgtime(api_path.c_str(), repertory_chgtime));
}

api_error remote_fuse_drive::setcrtime_impl(std::string api_path,
                                            const struct timespec *crtime) {
  remote::file_time repertory_crtime =
      ((crtime->tv_sec * utils::time::NANOS_PER_SECOND) + crtime->tv_nsec);
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
      strncpy(&stbuf->f_mntonname[0U], get_mount_location().c_str(), MNAMELEN);
      strncpy(&stbuf->f_mntfromname[0U], &r.f_mntfromname[0U], MNAMELEN);
    }
  } else {
    res = -errno;
  }

  return utils::to_api_error(res);
}
#else  // !defined(__APPLE__)
auto remote_fuse_drive::statfs_impl(std::string api_path, struct statvfs *stbuf)
    -> api_error {
  auto res = statvfs(config_.get_data_directory().c_str(), stbuf);
  if (res == 0) {
    remote::statfs r_stat{};
    res = remote_instance_->fuse_statfs(api_path.c_str(), stbuf->f_frsize,
                                        r_stat);
    if (res == 0) {
      stbuf->f_blocks = r_stat.f_blocks;
      stbuf->f_bavail = r_stat.f_bavail;
      stbuf->f_bfree = r_stat.f_bfree;
      stbuf->f_ffree = r_stat.f_ffree;
      stbuf->f_favail = r_stat.f_favail;
      stbuf->f_files = r_stat.f_files;
    }
  } else {
    res = -errno;
  }

  return utils::to_api_error(res);
}
#endif // defined(__APPLE__)

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::truncate_impl(std::string api_path, off_t size,
                                      struct fuse_file_info * /*f_info*/)
    -> api_error {
#else  // FUSE_USE_VERSION < 30
auto remote_fuse_drive::truncate_impl(std::string api_path, off_t size)
    -> api_error {
#endif // FUSE_USE_VERSION >= 30
  return utils::to_api_error(remote_instance_->fuse_truncate(
      api_path.c_str(), static_cast<remote::file_offset>(size)));
}

auto remote_fuse_drive::unlink_impl(std::string api_path) -> api_error {
  return utils::to_api_error(remote_instance_->fuse_unlink(api_path.c_str()));
}

#if FUSE_USE_VERSION >= 30
auto remote_fuse_drive::utimens_impl(std::string api_path,
                                     const struct timespec tv[2],
                                     struct fuse_file_info * /*f_info*/)
    -> api_error {
#else  // FUSE_USE_VERSION < 30
auto remote_fuse_drive::utimens_impl(std::string api_path,
                                     const struct timespec tv[2]) -> api_error {
#endif // FUSE_USE_VERSION >= 30
  remote::file_time rtv[2U] = {0};
  if (tv != nullptr) {
    const auto update_timespec = [](auto &dst, const auto &src) {
      dst = static_cast<remote::file_time>(
          src.tv_nsec +
          (src.tv_sec * static_cast<std::decay_t<decltype(src.tv_sec)>>(
                            utils::time::NANOS_PER_SECOND)));
    };
    update_timespec(rtv[0U], tv[0U]);
    update_timespec(rtv[1U], tv[1U]);
  }

  return utils::to_api_error(remote_instance_->fuse_utimens(
      api_path.c_str(), &rtv[0U],
      static_cast<std::uint64_t>(tv == nullptr ? 0 : tv[0U].tv_nsec),
      static_cast<std::uint64_t>(tv == nullptr ? 0 : tv[1U].tv_nsec)));
}

auto remote_fuse_drive::write_impl(std::string api_path, const char *buffer,
                                   size_t write_size, off_t write_offset,
                                   struct fuse_file_info *f_info,
                                   std::size_t &bytes_written) -> api_error {
  auto res = remote_instance_->fuse_write(
      api_path.c_str(), buffer, write_size,
      static_cast<remote::file_offset>(write_offset), f_info->fh);
  if (res >= 0) {
    bytes_written = static_cast<std::size_t>(res);
    return api_error::success;
  }

  return utils::to_api_error(res);
}
} // namespace repertory::remote_fuse

#endif // !defined(_WIN32)
