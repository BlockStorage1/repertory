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

#include "drives/fuse/remotefuse/remote_fuse_drive.hpp"
#include "app_config.hpp"
#include "drives/fuse/events.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/consumers/logging_consumer.hpp"
#include "events/events.hpp"
#include "events/event_system.hpp"
#include "platform/platform.hpp"
#include "rpc/server/server.hpp"
#include "types/remote.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"

namespace repertory::remote_fuse {
app_config *remote_fuse_drive::remote_fuse_impl::config_ = nullptr;
lock_data *remote_fuse_drive::remote_fuse_impl::lock_ = nullptr;
std::string *remote_fuse_drive::remote_fuse_impl::mount_location_ = nullptr;
bool remote_fuse_drive::remote_fuse_impl::console_enabled_ = false;
bool remote_fuse_drive::remote_fuse_impl::was_mounted_ = false;
std::optional<gid_t> remote_fuse_drive::remote_fuse_impl::forced_gid_;
std::optional<uid_t> remote_fuse_drive::remote_fuse_impl::forced_uid_;
std::optional<mode_t> remote_fuse_drive::remote_fuse_impl::forced_umask_;
remote_instance_factory *remote_fuse_drive::remote_fuse_impl::factory_ = nullptr;
std::unique_ptr<logging_consumer> remote_fuse_drive::remote_fuse_impl::logging_consumer_;
std::unique_ptr<console_consumer> remote_fuse_drive::remote_fuse_impl::console_consumer_;
std::unique_ptr<i_remote_instance> remote_fuse_drive::remote_fuse_impl::remote_instance_;
std::unique_ptr<server> remote_fuse_drive::remote_fuse_impl::server_;

void remote_fuse_drive::remote_fuse_impl::tear_down(const int &ret) {
  if (was_mounted_) {
    event_system::instance().raise<drive_mount_result>(std::to_string(ret));
    event_system::instance().stop();
    logging_consumer_.reset();
    console_consumer_.reset();
  }
}

void remote_fuse_drive::remote_fuse_impl::populate_stat(const remote::stat &r,
                                                        const bool &directory, struct stat &st) {
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
    const auto blockSizeStat = static_cast<std::uint64_t>(512);
    const auto blockSize = static_cast<std::uint64_t>(4096);
    const auto size =
        utils::divide_with_ceiling(static_cast<std::uint64_t>(st.st_size), blockSize) * blockSize;
    st.st_blocks =
        std::max(blockSize / blockSizeStat, utils::divide_with_ceiling(size, blockSizeStat));
  }

  st.st_gid = r.st_gid;
  st.st_mode = (directory ? S_IFDIR : S_IFREG) | r.st_mode;

  st.st_nlink = r.st_nlink;
  st.st_size = r.st_size;
  st.st_uid = r.st_uid;
}

int remote_fuse_drive::remote_fuse_impl::repertory_access(const char *path, int mask) {
  return remote_instance_->fuse_access(path, mask);
}

#ifdef __APPLE__
int remote_fuse_drive::remote_fuse_impl::repertory_chflags(const char *path, uint32_t flags) {
  return remote_instance_->fuse_chflags(path, flags);
}
#endif

int remote_fuse_drive::remote_fuse_impl::repertory_chmod(const char *path, mode_t mode) {
  return remote_instance_->fuse_chmod(path, mode);
}

int remote_fuse_drive::remote_fuse_impl::repertory_chown(const char *path, uid_t uid, gid_t gid) {
  return remote_instance_->fuse_chown(path, uid, gid);
}

int remote_fuse_drive::remote_fuse_impl::repertory_create(const char *path, mode_t mode,
                                                          struct fuse_file_info *fi) {
  return remote_instance_->fuse_create(path, mode, remote::open_flags(fi->flags), fi->fh);
}

void remote_fuse_drive::remote_fuse_impl::repertory_destroy(void * /*ptr*/) {
  event_system::instance().raise<drive_unmount_pending>(*mount_location_);
  if (server_) {
    server_->stop();
    server_.reset();
  }

  remote_instance_->fuse_destroy();
  remote_instance_.reset();

  lock_->set_mount_state(false, "", -1);
  event_system::instance().raise<drive_mounted>(*mount_location_);
}

/*int remote_fuse_drive::remote_fuse_impl::repertory_fallocate(const char *path, int mode, off_t
offset, off_t length, struct fuse_file_info *fi) { return remote_instance_->fuse_fallocate(path,
mode, offset, length, fi->fh);
}*/

int remote_fuse_drive::remote_fuse_impl::repertory_fgetattr(const char *path, struct stat *st,
                                                            struct fuse_file_info *fi) {
  remote::stat r{};

  bool directory = false;
  auto ret = remote_instance_->fuse_fgetattr(path, r, directory, fi->fh);
  if (ret == 0) {
    populate_stat(r, directory, *st);
  }
  return ret;
}

#ifdef __APPLE__
int remote_fuse_drive::remote_fuse_impl::repertory_fsetattr_x(const char *path,
                                                              struct setattr_x *attr,
                                                              struct fuse_file_info *fi) {
  remote::setattr_x attrRepertory{};
  attrRepertory.valid = attr->valid;
  attrRepertory.mode = attr->mode;
  attrRepertory.uid = attr->uid;
  attrRepertory.gid = attr->gid;
  attrRepertory.size = attr->size;
  attrRepertory.acctime = ((attr->acctime.tv_sec * NANOS_PER_SECOND) + attr->acctime.tv_nsec);
  attrRepertory.modtime = ((attr->modtime.tv_sec * NANOS_PER_SECOND) + attr->modtime.tv_nsec);
  attrRepertory.crtime = ((attr->crtime.tv_sec * NANOS_PER_SECOND) + attr->crtime.tv_nsec);
  attrRepertory.chgtime = ((attr->chgtime.tv_sec * NANOS_PER_SECOND) + attr->chgtime.tv_nsec);
  attrRepertory.bkuptime = ((attr->bkuptime.tv_sec * NANOS_PER_SECOND) + attr->bkuptime.tv_nsec);
  attrRepertory.flags = attr->flags;
  return remote_instance_->fuse_fsetattr_x(path, attrRepertory, fi->fh);
}
#endif

int remote_fuse_drive::remote_fuse_impl::repertory_fsync(const char *path, int datasync,
                                                         struct fuse_file_info *fi) {
  return remote_instance_->fuse_fsync(path, datasync, fi->fh);
}

int remote_fuse_drive::remote_fuse_impl::repertory_ftruncate(const char *path, off_t size,
                                                             struct fuse_file_info *fi) {
  return remote_instance_->fuse_ftruncate(path, size, fi->fh);
}

int remote_fuse_drive::remote_fuse_impl::repertory_getattr(const char *path, struct stat *st) {
  bool directory = false;
  remote::stat r{};
  const auto ret = remote_instance_->fuse_getattr(path, r, directory);
  if (ret == 0) {
    populate_stat(r, directory, *st);
  }
  return ret;
}

#ifdef __APPLE__
int remote_fuse_drive::remote_fuse_impl::repertory_getxtimes(const char *path,
                                                             struct timespec *bkuptime,
                                                             struct timespec *crtime) {
  if (not(bkuptime && crtime)) {
    return -EFAULT;
  } else {
    remote::file_time repertory_bkuptime = 0;
    remote::file_time repertory_crtime = 0;
    const auto ret = remote_instance_->fuse_getxtimes(path, repertory_bkuptime, repertory_crtime);
    if (ret == 0) {
      bkuptime->tv_nsec = static_cast<long>(repertory_bkuptime % NANOS_PER_SECOND);
      bkuptime->tv_sec = repertory_bkuptime / NANOS_PER_SECOND;
      crtime->tv_nsec = static_cast<long>(repertory_crtime % NANOS_PER_SECOND);
      crtime->tv_sec = repertory_crtime / NANOS_PER_SECOND;
    }

    return ret;
  }
}
#endif

void *remote_fuse_drive::remote_fuse_impl::repertory_init(struct fuse_conn_info *conn) {
  utils::file::change_to_process_directory();
#ifdef __APPLE__
  conn->want |= FUSE_CAP_VOL_RENAME;
  conn->want |= FUSE_CAP_XTIMES;
#endif
  conn->want |= FUSE_CAP_BIG_WRITES;

  if (console_enabled_) {
    console_consumer_ = std::make_unique<console_consumer>();
  }
  logging_consumer_ =
      std::make_unique<logging_consumer>(config_->get_log_directory(), config_->get_event_level());
  event_system::instance().start();

  was_mounted_ = true;
  lock_->set_mount_state(true, *mount_location_, getpid());

  remote_instance_ = (*factory_)();
  remote_instance_->set_fuse_uid_gid(getuid(), getgid());

  if (remote_instance_->fuse_init() != 0) {
    event_system::instance().raise<repertory_exception>(__FUNCTION__,
                                                        "Failed to connect to remote server");
    event_system::instance().raise<unmount_requested>();
  } else {
    server_ = std::make_unique<server>(*config_);
    server_->start();
    event_system::instance().raise<drive_mounted>(*mount_location_);
  }

  return nullptr;
}

int remote_fuse_drive::remote_fuse_impl::repertory_mkdir(const char *path, mode_t mode) {
  return remote_instance_->fuse_mkdir(path, mode);
}

int remote_fuse_drive::remote_fuse_impl::repertory_open(const char *path,
                                                        struct fuse_file_info *fi) {
  return remote_instance_->fuse_open(path, remote::open_flags(fi->flags), fi->fh);
}

int remote_fuse_drive::remote_fuse_impl::repertory_opendir(const char *path,
                                                           struct fuse_file_info *fi) {
  return remote_instance_->fuse_opendir(path, fi->fh);
}

int remote_fuse_drive::remote_fuse_impl::repertory_read(const char *path, char *buffer,
                                                        size_t readSize, off_t readOffset,
                                                        struct fuse_file_info *fi) {
  return remote_instance_->fuse_read(path, buffer, readSize, readOffset, fi->fh);
}

int remote_fuse_drive::remote_fuse_impl::repertory_readdir(const char *path, void *buf,
                                                           fuse_fill_dir_t fuseFillDir,
                                                           off_t offset,
                                                           struct fuse_file_info *fi) {
  std::string item_path;
  int ret = 0;
  while ((ret = remote_instance_->fuse_readdir(path, offset, fi->fh, item_path)) == 0) {
    if ((item_path != ".") && (item_path != "..")) {
      item_path = utils::path::strip_to_file_name(item_path);
    }
    if (fuseFillDir(buf, &item_path[0], nullptr, ++offset) != 0) {
      break;
    }
  }

  if (ret == -120) {
    ret = 0;
  }

  return ret;
}

int remote_fuse_drive::remote_fuse_impl::repertory_release(const char *path,
                                                           struct fuse_file_info *fi) {
  return remote_instance_->fuse_release(path, fi->fh);
}

int remote_fuse_drive::remote_fuse_impl::repertory_releasedir(const char *path,
                                                              struct fuse_file_info *fi) {
  return remote_instance_->fuse_releasedir(path, fi->fh);
}

int remote_fuse_drive::remote_fuse_impl::repertory_rename(const char *from, const char *to) {
  return remote_instance_->fuse_rename(from, to);
}

int remote_fuse_drive::remote_fuse_impl::repertory_rmdir(const char *path) {
  return remote_instance_->fuse_rmdir(path);
}
/*
#ifdef HAS_SETXATTR
#ifdef __APPLE__
    int remote_fuse_drive::remote_fuse_impl::repertory_getxattr(const char *path, const char *name,
char *value, size_t size, uint32_t position) { return remote_instance_->fuse_getxattr_osx(path,
name, value, size, position);
    }
#else
    int remote_fuse_drive::remote_fuse_impl::repertory_getxattr(const char *path, const char *name,
char *value, size_t size) { return remote_instance_->fuse_getxattr(path, name, value, size);
    }

#endif
    int remote_fuse_drive::remote_fuse_impl::repertory_listxattr(const char *path, char *buffer,
size_t size) { return remote_instance_->fuse_listxattr(path, buffer, size);
    }

    remote_fuse_drive::remote_fuse_impl::int repertory_removexattr(const char *path, const char
*name) { return remote_instance_->fuse_removexattr(path, name);
    }
#ifdef __APPLE__
    int remote_fuse_drive::remote_fuse_impl::repertory_setxattr(const char *path, const char *name,
const char *value, size_t size, int flags, uint32_t position) { return
remote_instance_->fuse_setxattr_osx(path, name, value, size, flags, position);
    }
#else
    int remote_fuse_drive::remote_fuse_impl::repertory_setxattr(const char *path, const char *name,
const char *value, size_t size, int flags) { return remote_instance_->fuse_setxattr(path, name,
value, size, flags);
    }
#endif
#endif
 */
#ifdef __APPLE__
int remote_fuse_drive::remote_fuse_impl::repertory_setattr_x(const char *path,
                                                             struct setattr_x *attr) {
  remote::setattr_x attributes{};
  attributes.valid = attr->valid;
  attributes.mode = attr->mode;
  attributes.uid = attr->uid;
  attributes.gid = attr->gid;
  attributes.size = attr->size;
  attributes.acctime = ((attr->acctime.tv_sec * NANOS_PER_SECOND) + attr->acctime.tv_nsec);
  attributes.modtime = ((attr->modtime.tv_sec * NANOS_PER_SECOND) + attr->modtime.tv_nsec);
  attributes.crtime = ((attr->crtime.tv_sec * NANOS_PER_SECOND) + attr->crtime.tv_nsec);
  attributes.chgtime = ((attr->chgtime.tv_sec * NANOS_PER_SECOND) + attr->chgtime.tv_nsec);
  attributes.bkuptime = ((attr->bkuptime.tv_sec * NANOS_PER_SECOND) + attr->bkuptime.tv_nsec);
  attributes.flags = attr->flags;
  return remote_instance_->fuse_setattr_x(path, attributes);
}

int remote_fuse_drive::remote_fuse_impl::repertory_setbkuptime(const char *path,
                                                               const struct timespec *bkuptime) {
  remote::file_time repertory_bkuptime =
      ((bkuptime->tv_sec * NANOS_PER_SECOND) + bkuptime->tv_nsec);
  return remote_instance_->fuse_setbkuptime(path, repertory_bkuptime);
}

int remote_fuse_drive::remote_fuse_impl::repertory_setchgtime(const char *path,
                                                              const struct timespec *chgtime) {
  remote::file_time repertory_chgtime = ((chgtime->tv_sec * NANOS_PER_SECOND) + chgtime->tv_nsec);
  return remote_instance_->fuse_setchgtime(path, repertory_chgtime);
}

int remote_fuse_drive::remote_fuse_impl::repertory_setcrtime(const char *path,
                                                             const struct timespec *crtime) {
  remote::file_time repertory_crtime = ((crtime->tv_sec * NANOS_PER_SECOND) + crtime->tv_nsec);
  return remote_instance_->fuse_setcrtime(path, repertory_crtime);
}

int remote_fuse_drive::remote_fuse_impl::repertory_setvolname(const char *volname) {
  return remote_instance_->fuse_setvolname(volname);
}

int remote_fuse_drive::remote_fuse_impl::repertory_statfs_x(const char *path,
                                                            struct statfs *stbuf) {
  auto ret = statfs(config_->get_data_directory().c_str(), stbuf);
  if (ret == 0) {
    remote::statfs_x r{};
    if ((ret = remote_instance_->fuse_statfs_x(path, stbuf->f_bsize, r)) == 0) {
      stbuf->f_blocks = r.f_blocks;
      stbuf->f_bavail = r.f_bavail;
      stbuf->f_bfree = r.f_bfree;
      stbuf->f_ffree = r.f_ffree;
      stbuf->f_files = r.f_files;
      stbuf->f_owner = getuid();
      strncpy(&stbuf->f_mntonname[0], mount_location_->c_str(), MNAMELEN);
      strncpy(&stbuf->f_mntfromname[0], &r.f_mntfromname[0], MNAMELEN);
    }
  } else {
    ret = -errno;
  }
  return ret;
}
#else

int remote_fuse_drive::remote_fuse_impl::repertory_statfs(const char *path, struct statvfs *stbuf) {
  auto ret = statvfs(&config_->get_data_directory()[0], stbuf);
  if (ret == 0) {
    remote::statfs r{};
    if ((ret = remote_instance_->fuse_statfs(path, stbuf->f_frsize, r)) == 0) {
      stbuf->f_blocks = r.f_blocks;
      stbuf->f_bavail = r.f_bavail;
      stbuf->f_bfree = r.f_bfree;
      stbuf->f_ffree = r.f_ffree;
      stbuf->f_favail = r.f_favail;
      stbuf->f_files = r.f_files;
    }
  } else {
    ret = -errno;
  }
  return ret;
}

#endif

int remote_fuse_drive::remote_fuse_impl::repertory_truncate(const char *path, off_t size) {
  return remote_instance_->fuse_truncate(path, size);
}

int remote_fuse_drive::remote_fuse_impl::repertory_unlink(const char *path) {
  return remote_instance_->fuse_unlink(path);
}

int remote_fuse_drive::remote_fuse_impl::repertory_utimens(const char *path,
                                                           const struct timespec tv[2]) {
  remote::file_time rtv[2] = {0};
  if (tv) {
    rtv[0] = tv[0].tv_nsec + (tv[0].tv_sec * NANOS_PER_SECOND);
    rtv[1] = tv[1].tv_nsec + (tv[1].tv_sec * NANOS_PER_SECOND);
  }
  return remote_instance_->fuse_utimens(path, rtv, tv ? tv[0].tv_nsec : 0, tv ? tv[1].tv_nsec : 0);
}

int remote_fuse_drive::remote_fuse_impl::repertory_write(const char *path, const char *buffer,
                                                         size_t writeSize, off_t writeOffset,
                                                         struct fuse_file_info *fi) {
  return remote_instance_->fuse_write(path, buffer, writeSize, writeOffset, fi->fh);
}

remote_fuse_drive::remote_fuse_drive(app_config &config, lock_data &lock,
                                     remote_instance_factory factory)
    : config_(config), lock_(lock), factory_(std::move(factory)) {
  E_SUBSCRIBE_EXACT(unmount_requested, [this](const unmount_requested &) {
    std::thread([this]() { remote_fuse_drive::shutdown(mount_location_); }).detach();
  });
}

int remote_fuse_drive::mount(std::vector<std::string> drive_args) {
  remote_fuse_impl::lock_ = &lock_;
  remote_fuse_impl::config_ = &config_;
  remote_fuse_impl::mount_location_ = &mount_location_;
  remote_fuse_impl::factory_ = &factory_;

#ifdef __APPLE__
  fuse_ops_.chflags = remote_fuse_impl::repertory_chflags;
  fuse_ops_.fsetattr_x = remote_fuse_impl::repertory_fsetattr_x;
  fuse_ops_.getxtimes = remote_fuse_impl::repertory_getxtimes;
  fuse_ops_.setattr_x = remote_fuse_impl::repertory_setattr_x;
  fuse_ops_.setbkuptime = remote_fuse_impl::repertory_setbkuptime;
  fuse_ops_.setchgtime = remote_fuse_impl::repertory_setchgtime;
  fuse_ops_.setcrtime = remote_fuse_impl::repertory_setcrtime;
  fuse_ops_.setvolname = remote_fuse_impl::repertory_setvolname;
  fuse_ops_.statfs_x = remote_fuse_impl::repertory_statfs_x;
#endif
  auto force_no_console = false;
  for (std::size_t i = 1u; !force_no_console && (i < drive_args.size()); i++) {
    if (drive_args[i] == "-nc") {
      force_no_console = true;
    }
  }
  utils::remove_element_from(drive_args, "-nc");

  for (std::size_t i = 1u; i < drive_args.size(); i++) {
    if (drive_args[i] == "-f") {
      remote_fuse_impl::console_enabled_ = not force_no_console;
    } else if (drive_args[i].find("-o") == 0) {
      std::string options;
      if (drive_args[i].size() == 2u) {
        if ((i + 1) < drive_args.size()) {
          options = drive_args[++i];
        }
      } else {
        options = drive_args[i].substr(2);
      }

      const auto option_list = utils::string::split(options, ',');
      for (const auto &option : option_list) {
        if (option.find("gid") == 0) {
          const auto parts = utils::string::split(option, '=');
          if (parts.size() == 2u) {
            auto gid = getgrnam(parts[1u].c_str());
            if (not gid) {
              gid = getgrgid(utils::string::to_uint32(parts[1]));
            }
            if ((getgid() != 0) && (gid->gr_gid == 0)) {
              std::cerr << "'gid=0' requires running as root" << std::endl;
              return -1;
            } else {
              remote_fuse_impl::forced_gid_ = gid->gr_gid;
            }
          }
        } else if (option.find("uid") == 0) {
          const auto parts = utils::string::split(option, '=');
          if (parts.size() == 2u) {
            auto uid = getpwnam(parts[1u].c_str());
            if (not uid) {
              uid = getpwuid(utils::string::to_uint32(parts[1]));
            }
            if ((getuid() != 0) && (uid->pw_uid == 0)) {
              std::cerr << "'uid=0' requires running as root" << std::endl;
              return -1;
            } else {
              remote_fuse_impl::forced_uid_ = uid->pw_uid;
            }
          }
        } else if (option.find("umask") == 0) {
          const auto parts = utils::string::split(option, '=');
          if (parts.size() == 2u) {
            static const auto numeric_regex = std::regex("[0-9]+");
            try {
              if (not std::regex_match(parts[1u], numeric_regex)) {
                throw std::runtime_error("invalid syntax");
              } else {
                remote_fuse_impl::forced_umask_ = utils::string::to_uint32(parts[1]);
              }
            } catch (...) {
              std::cerr << ("'" + option + "' invalid syntax") << std::endl;
              return -1;
            }
          }
        }
      }
    }
  }

  std::vector<const char *> fuse_argv(drive_args.size());
  for (std::size_t i = 0u; i < drive_args.size(); i++) {
    fuse_argv[i] = drive_args[i].c_str();
  }

  struct fuse_args args =
      FUSE_ARGS_INIT(static_cast<int>(fuse_argv.size()), (char **)&fuse_argv[0]);
  char *mount_location = nullptr;
  fuse_parse_cmdline(&args, &mount_location, nullptr, nullptr);
  if (mount_location) {
    mount_location_ = mount_location;
    free(mount_location);
  }

  std::string args_string;
  for (const auto *arg : fuse_argv) {
    if (args_string.empty()) {
      args_string += arg;
    } else {
      args_string += (" " + std::string(arg));
    }
  }

  event_system::instance().raise<fuse_args_parsed>(args_string);

  umask(0);
  const auto ret = fuse_main(static_cast<int>(fuse_argv.size()), (char **)&fuse_argv[0], &fuse_ops_,
                             &mount_location);
  remote_fuse_impl::tear_down(ret);
  return ret;
}

void remote_fuse_drive::display_options(int argc, char *argv[]) {
  struct fuse_operations fuse_ops {};
  fuse_main(argc, argv, &fuse_ops, nullptr);
  std::cout << std::endl;
}

void remote_fuse_drive::display_version_information(int argc, char *argv[]) {
  struct fuse_operations fuse_ops {};
  fuse_main(argc, argv, &fuse_ops, nullptr);
}

void remote_fuse_drive::shutdown(std::string mount_location) {
#if __APPLE__
  const auto unmount = "umount \"" + mount_location + "\" >/dev/null 2>&1";
#else
  const auto unmount = "fusermount -u \"" + mount_location + "\" >/dev/null 2>&1";
#endif
  int ret;
  for (std::uint8_t i = 0u; ((ret = system(unmount.c_str())) != 0) && (i < 10u); i++) {
    event_system::instance().raise<unmount_result>(mount_location, std::to_string(ret));
    if (i != 9u) {
      std::this_thread::sleep_for(1s);
    }
  }

  if (ret != 0) {
    ret = kill(getpid(), SIGINT);
  }

  event_system::instance().raise<unmount_result>(mount_location, std::to_string(ret));
}
} // namespace repertory::remote_fuse

#endif // _WIN32
