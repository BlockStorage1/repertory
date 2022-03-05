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

#include "drives/fuse/fuse_base.hpp"
#include "app_config.hpp"
#include "drives/fuse/events.hpp"
#include "events/events.hpp"
#include "events/event_system.hpp"
#include "providers/i_provider.hpp"
#include "utils/path_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
fuse_base &fuse_base::instance() {
  return *reinterpret_cast<fuse_base *>(fuse_get_context()->private_data);
}

fuse_base::fuse_base(app_config &config) : config_(config) {
#ifdef __APPLE__
  fuse_ops_.chflags = fuse_base::chflags_;
  fuse_ops_.fsetattr_x = fuse_base::fsetattr_x_;
  fuse_ops_.getxtimes = fuse_base::getxtimes_;
  fuse_ops_.setattr_x = fuse_base::setattr_x_;
  fuse_ops_.setbkuptime = fuse_base::setbkuptime_;
  fuse_ops_.setchgtime = fuse_base::setchgtime_;
  fuse_ops_.setcrtime = fuse_base::setcrtime_;
  fuse_ops_.setvolname = fuse_base::setvolname_;
  fuse_ops_.statfs_x = fuse_base::statfs_x_;
#endif // __APPLE__

  E_SUBSCRIBE_EXACT(unmount_requested, [this](const unmount_requested &) {
    std::thread([this]() { this->shutdown(); }).detach();
  });
}

fuse_base::~fuse_base() { E_CONSUMER_RELEASE(); }

int fuse_base::access_(const char *path, int mask) {
  return instance().instance().execute_callback(__FUNCTION__, path,
                                                [&](const std::string &api_path) -> api_error {
                                                  return instance().access_impl(api_path, mask);
                                                });
}

#ifdef __APPLE__
int fuse_base::chflags_(const char *path, uint32_t flags) {
  return instance().instance().execute_callback(__FUNCTION__, path,
                                                [&](const std::string &api_path) -> api_error {
                                                  return instance().chflags_impl(api_path, flags);
                                                });
}
#endif // __APPLE__

api_error fuse_base::check_access(const std::string &api_path, int mask) const {
  api_meta_map meta;
  const auto res = get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  // Always allow root
  if (fuse_get_context()->uid != 0) {
    // Always allow forced user
    if (not forced_uid_.has_value() || (fuse_get_context()->uid != get_effective_uid())) {
      // Always allow if checking file exists
      if (F_OK != mask) {
        const auto effective_uid =
            (forced_uid_.has_value() ? forced_uid_.value() : get_uid_from_meta(meta));
        const auto effective_gid =
            (forced_gid_.has_value() ? forced_gid_.value() : get_gid_from_meta(meta));

        // Create file mode
        mode_t effective_mode = forced_umask_.has_value()
                                    ? ((S_IRWXU | S_IRWXG | S_IRWXO) & (~forced_umask_.value()))
                                    : get_mode_from_meta(meta);

        // Create access mask
        mode_t active_mask = S_IRWXO;
        if (fuse_get_context()->uid == effective_uid) {
          active_mask |= S_IRWXU;
        }
        if (fuse_get_context()->gid == effective_gid) {
          active_mask |= S_IRWXG;
        }
        if (utils::is_uid_member_of_group(fuse_get_context()->uid, effective_gid)) {
          active_mask |= S_IRWXG;
        }

        // Calculate effective file mode
        effective_mode &= active_mask;

        // Check allow execute
        if ((mask & X_OK) == X_OK) {
          if ((effective_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0) {
            return api_error::permission_denied;
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
      }
    }
  }

  return api_error::success;
}

api_error fuse_base::check_and_perform(const std::string &api_path, int parent_mask,
                                       const std::function<api_error(api_meta_map &meta)> &action) {
  api_meta_map meta;
  auto ret = get_item_meta(api_path, meta);
  if (ret != api_error::success) {
    return ret;
  }

  if ((ret = check_parent_access(api_path, parent_mask)) != api_error::success) {
    return ret;
  }

  if ((ret = check_owner(meta)) != api_error::success) {
    return ret;
  }

  return action(meta);
}

api_error fuse_base::check_open_flags(const int &flags, const int &mask,
                                      const api_error &fail_error) {
  return ((flags & mask) ? fail_error : api_error::success);
}

api_error fuse_base::check_owner(const api_meta_map &meta) const {
  // Always allow root
  if ((fuse_get_context()->uid != 0) &&
      // Always allow forced UID
      (not forced_uid_.has_value() || (fuse_get_context()->uid != get_effective_uid())) &&
      // Check if current uid matches file uid
      (get_uid_from_meta(meta) != get_effective_uid())) {
    return api_error::permission_denied;
  }

  return api_error::success;
}

api_error fuse_base::check_parent_access(const std::string &api_path, int mask) const {
  auto ret = api_error::success;

  // Ignore root
  if (api_path != "/") {
    if ((mask & X_OK) == X_OK) {
      for (auto parent = utils::path::get_parent_directory(api_path);
           (ret == api_error::success) && not parent.empty();
           parent = utils::path::get_parent_directory(parent)) {
        if (((ret = check_access(parent, X_OK)) == api_error::success) && (parent == "/")) {
          break;
        }
      }
    }

    if (ret == api_error::success) {
      mask &= (~X_OK);
      if (mask != 0) {
        ret = check_access(utils::path::get_parent_directory(api_path), mask);
      }
    }
  }

  return ret;
}

api_error fuse_base::check_readable(const int &flags, const api_error &fail_error) {
  const auto mode = (flags & O_ACCMODE);
  return ((mode == O_WRONLY) ? fail_error : api_error::success);
}

api_error fuse_base::check_writeable(const int &flags, const api_error &fail_error) {
  return ((flags & O_ACCMODE) ? api_error::success : fail_error);
}

int fuse_base::chmod_(const char *path, mode_t mode) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().chmod_impl(api_path, mode);
                                     });
}

int fuse_base::chown_(const char *path, uid_t uid, gid_t gid) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().chown_impl(api_path, uid, gid);
                                     });
}

int fuse_base::create_(const char *path, mode_t mode, struct fuse_file_info *fi) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().create_impl(api_path, mode, fi);
                                     });
}

void fuse_base::destroy_(void *ptr) {
  execute_void_callback(__FUNCTION__, [&]() { instance().destroy_impl(ptr); });
}

void fuse_base::display_options(int argc, char *argv[]) {
  struct fuse_operations fuse_ops {};
  fuse_main(argc, argv, &fuse_ops, nullptr);
  std::cout << std::endl;
}

void fuse_base::display_version_information(int argc, char *argv[]) {
  struct fuse_operations fuse_ops {};
  fuse_main(argc, argv, &fuse_ops, nullptr);
}

int fuse_base::execute_callback(const std::string &function_name, const char *from, const char *to,
                                const std::function<api_error(const std::string &from_api_file,
                                                              const std::string &to_api_path)> &cb,
                                const bool &disable_logging) {
  const auto from_api_file = utils::path::create_api_path(from ? from : "");
  const auto to_api_file = utils::path::create_api_path(to ? to : "");
  const auto res = utils::translate_api_error(cb(from_api_file, to_api_file));
  raise_fuse_event(function_name, "from|" + from_api_file + "|to|" + to_api_file, res,
                   disable_logging);
  return res;
}

int fuse_base::execute_callback(const std::string &function_name, const char *path,
                                const std::function<api_error(const std::string &api_path)> &cb,
                                const bool &disable_logging) {
  const auto api_path = utils::path::create_api_path(path ? path : "");
  const auto res = utils::translate_api_error(cb(api_path));
  raise_fuse_event(function_name, api_path, res, disable_logging);
  return res;
}

void fuse_base::execute_void_callback(const std::string &function_name,
                                      const std::function<void()> &cb) {
  cb();

  instance().raise_fuse_event(function_name, "", 0, false);
}

void *fuse_base::execute_void_pointer_callback(const std::string &function_name,
                                               const std::function<void *()> &cb) {
  auto *ret = cb();

  instance().raise_fuse_event(function_name, "", 0, false);

  return ret;
}

int fuse_base::fallocate_(const char *path, int mode, off_t offset, off_t length,
                          struct fuse_file_info *fi) {
  return instance().execute_callback(
      __FUNCTION__, path, [&](const std::string &api_path) -> api_error {
        return instance().fallocate_impl(api_path, mode, offset, length, fi);
      });
}

int fuse_base::fgetattr_(const char *path, struct stat *st, struct fuse_file_info *fi) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().fgetattr_impl(api_path, st, fi);
                                     });
}

#ifdef __APPLE__
int fuse_base::fsetattr_x_(const char *path, struct setattr_x *attr, struct fuse_file_info *fi) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().fsetattr_x_impl(api_path, attr, fi);
                                     });
}
#endif // __APPLE__

int fuse_base::fsync_(const char *path, int datasync, struct fuse_file_info *fi) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().fsync_impl(api_path, datasync, fi);
                                     });
}

int fuse_base::ftruncate_(const char *path, off_t size, struct fuse_file_info *fi) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().ftruncate_impl(api_path, size, fi);
                                     });
}

uid_t fuse_base::get_effective_uid() const {
  return forced_uid_.has_value() ? forced_uid_.value() : fuse_get_context()->uid;
}

uid_t fuse_base::get_effective_gid() const {
  return forced_gid_.has_value() ? forced_gid_.value() : fuse_get_context()->gid;
}

#ifdef __APPLE__
__uint32_t fuse_base::get_flags_from_meta(const api_meta_map &meta) {
  return utils::string::to_uint32(meta.at(META_OSXFLAGS));
}
#endif // __APPLE__

gid_t fuse_base::get_gid_from_meta(const api_meta_map &meta) {
  return static_cast<gid_t>(utils::string::to_uint32(meta.at(META_GID)));
}

mode_t fuse_base::get_mode_from_meta(const api_meta_map &meta) {
  return static_cast<mode_t>(utils::string::to_uint32(meta.at(META_MODE)));
}

void fuse_base::get_timespec_from_meta(const api_meta_map &meta, const std::string &name,
                                       struct timespec &ts) {
  const auto t = utils::string::to_uint64(meta.at(name));
  ts.tv_nsec = t % NANOS_PER_SECOND;
  ts.tv_sec = t / NANOS_PER_SECOND;
}

uid_t fuse_base::get_uid_from_meta(const api_meta_map &meta) {
  return static_cast<uid_t>(utils::string::to_uint32(meta.at(META_UID)));
}

int fuse_base::getattr_(const char *path, struct stat *st) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().getattr_impl(api_path, st);
                                     });
}

#ifdef __APPLE__
int fuse_base::getxtimes_(const char *path, struct timespec *bkuptime, struct timespec *crtime) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().getxtimes_impl(api_path, bkuptime, crtime);
                                     });
}
#endif // __APPLE__

void *fuse_base::init_(struct fuse_conn_info *conn) {
  return execute_void_pointer_callback(__FUNCTION__,
                                       [&]() -> void * { return instance().init_impl(conn); });
}

void *fuse_base::init_impl(struct fuse_conn_info *conn) {
#ifdef __APPLE__
  conn->want |= FUSE_CAP_VOL_RENAME;
  conn->want |= FUSE_CAP_XTIMES;
#endif // __APPLE__
  conn->want |= FUSE_CAP_BIG_WRITES;

  return this;
}

int fuse_base::mkdir_(const char *path, mode_t mode) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().mkdir_impl(api_path, mode);
                                     });
}

int fuse_base::mount(std::vector<std::string> args) {
  auto ret = parse_args(args);
  if (ret == 0) {
    std::vector<const char *> fuse_argv(args.size());
    for (std::size_t i = 0u; i < args.size(); i++) {
      fuse_argv[i] = args[i].c_str();
    }

    {
      struct fuse_args fa =
          FUSE_ARGS_INIT(static_cast<int>(fuse_argv.size()), (char **)&fuse_argv[0]);

      char *mount_location = nullptr;
      fuse_parse_cmdline(&fa, &mount_location, nullptr, nullptr);
      if (mount_location) {
        mount_location_ = mount_location;
        free(mount_location);
      }
    }

    notify_fuse_args_parsed(args);

    umask(0);
    ret = fuse_main(static_cast<int>(fuse_argv.size()), (char **)&fuse_argv[0], &fuse_ops_, this);
    notify_fuse_main_exit(ret);
  }

  return ret;
}

int fuse_base::open_(const char *path, struct fuse_file_info *fi) {
  return instance().execute_callback(
      __FUNCTION__, path,
      [&](const std::string &api_path) -> api_error { return instance().open_impl(api_path, fi); });
}

int fuse_base::opendir_(const char *path, struct fuse_file_info *fi) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().opendir_impl(api_path, fi);
                                     });
}

int fuse_base::read_(const char *path, char *buffer, size_t read_size, off_t read_offset,
                     struct fuse_file_info *fi) {
  std::size_t bytes_read{};
  const auto res = instance().execute_callback(
      __FUNCTION__, path,
      [&](const std::string &api_path) -> api_error {
        return instance().read_impl(api_path, buffer, read_size, read_offset, fi, bytes_read);
      },
      true);
  return (res == 0) ? static_cast<int>(bytes_read) : res;
}

int fuse_base::readdir_(const char *path, void *buf, fuse_fill_dir_t fuse_fill_dir, off_t offset,
                        struct fuse_file_info *fi) {
  return instance().execute_callback(
      __FUNCTION__, path, [&](const std::string &api_path) -> api_error {
        return instance().readdir_impl(api_path, buf, fuse_fill_dir, offset, fi);
      });
}

int fuse_base::release_(const char *path, struct fuse_file_info *fi) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().release_impl(api_path, fi);
                                     });
}

int fuse_base::releasedir_(const char *path, struct fuse_file_info *fi) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().releasedir_impl(api_path, fi);
                                     });
}

int fuse_base::rename_(const char *from, const char *to) {
  return instance().execute_callback(
      __FUNCTION__, from, to,
      [&](const std::string &from_api_file, const std::string &to_api_path) -> api_error {
        return instance().rename_impl(from_api_file, to_api_path);
      });
}

int fuse_base::rmdir_(const char *path) {
  return instance().execute_callback(
      __FUNCTION__, path,
      [&](const std::string &api_path) -> api_error { return instance().rmdir_impl(api_path); });
}

#ifdef HAS_SETXATTR
#ifdef __APPLE__
int fuse_base::getxattr_(const char *path, const char *name, char *value, size_t size,
                         uint32_t position) {
  int attribute_size = 0;
  const auto res = instance().execute_callback(
      __FUNCTION__, path, [&](const std::string &api_path) -> api_error {
        return instance().getxattr_impl(api_path, name, value, size, position, attribute_size);
      });

  return res == 0 ? attribute_size : res;
}
#else  // __APPLE__
int fuse_base::getxattr_(const char *path, const char *name, char *value, size_t size) {
  int attribute_size = 0;
  const auto res = instance().execute_callback(
      __FUNCTION__, path, [&](const std::string &api_path) -> api_error {
        return instance().getxattr_impl(api_path, name, value, size, attribute_size);
      });

  return res == 0 ? attribute_size : res;
}
#endif // __APPLE__

int fuse_base::listxattr_(const char *path, char *buffer, size_t size) {
  int required_size = 0;
  bool return_size = false;

  const auto res = instance().execute_callback(
      __FUNCTION__, path, [&](const std::string &api_path) -> api_error {
        return instance().listxattr_impl(api_path, buffer, size, required_size, return_size);
      });

  return return_size ? required_size : res;
}

int fuse_base::parse_args(std::vector<std::string> &args) {
  auto force_no_console = false;
  for (std::size_t i = 1; !force_no_console && (i < args.size()); i++) {
    if (args[i] == "-nc") {
      force_no_console = true;
    }
  }
  utils::remove_element_from(args, "-nc");

  for (std::size_t i = 1; i < args.size(); i++) {
    if (args[i] == "-f") {
      console_enabled_ = not force_no_console;
    } else if (args[i].find("-o") == 0) {
      std::string options = "";
      if (args[i].size() == 2) {
        if ((i + 1) < args.size()) {
          options = args[++i];
        }
      } else {
        options = args[i].substr(2);
      }

      const auto option_parts = utils::string::split(options, ',');
      for (const auto &option : option_parts) {
        if (option.find("gid") == 0) {
          const auto parts = utils::string::split(option, '=');
          if (parts.size() == 2) {
            auto gid = getgrnam(parts[1].c_str());
            if (not gid) {
              gid = getgrgid(utils::string::to_uint32(parts[1]));
            }
            if ((getgid() != 0) && (gid->gr_gid == 0)) {
              std::cerr << "'gid=0' requires running as root" << std::endl;
              return -1;
            } else {
              forced_gid_ = gid->gr_gid;
            }
          }
        } else if (option.find("noatime") == 0) {
          atime_enabled_ = false;
        } else if (option.find("uid") == 0) {
          const auto parts = utils::string::split(option, '=');
          if (parts.size() == 2) {
            auto uid = getpwnam(parts[1].c_str());
            if (not uid) {
              uid = getpwuid(utils::string::to_uint32(parts[1]));
            }
            if ((getuid() != 0) && (uid->pw_uid == 0)) {
              std::cerr << "'uid=0' requires running as root" << std::endl;
              return -1;
            } else {
              forced_uid_ = uid->pw_uid;
            }
          }
        } else if (option.find("umask") == 0) {
          const auto parts = utils::string::split(option, '=');
          if (parts.size() == 2) {
            static const auto match_number_regex = std::regex("[0-9]+");
            try {
              if (not std::regex_match(parts[1], match_number_regex)) {
                throw std::runtime_error("invalid syntax");
              } else {
                forced_umask_ = utils::string::to_uint32(parts[1]);
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

  return 0;
}

#ifdef __APPLE__
api_error fuse_base::parse_xattr_parameters(const char *name, const uint32_t &position,
                                            std::string &attribute_name,
                                            const std::string &api_path) {
#else
api_error fuse_base::parse_xattr_parameters(const char *name, std::string &attribute_name,
                                            const std::string &api_path) {
#endif
  auto res = api_path.empty() ? api_error::bad_address : api_error::success;
  if (res != api_error::success) {
    return res;
  }

  if (not name) {
    return api_error::bad_address;
  }

  attribute_name = std::string(name);
#ifdef __APPLE__
  if (attribute_name == A_KAUTH_FILESEC_XATTR) {
    char new_name[MAXPATHLEN] = {0};
    memcpy(new_name, A_KAUTH_FILESEC_XATTR, sizeof(A_KAUTH_FILESEC_XATTR));
    memcpy(new_name, G_PREFIX, sizeof(G_PREFIX) - 1);
    attribute_name = new_name;
  } else if (attribute_name.empty() ||
             ((attribute_name != XATTR_RESOURCEFORK_NAME) && (position != 0))) {
    return api_error::xattr_osx_invalid;
  }
#endif

  return api_error::success;
}

#ifdef __APPLE__
api_error fuse_base::parse_xattr_parameters(const char *name, const char *value, size_t size,
                                            const uint32_t &position, std::string &attribute_name,
                                            const std::string &api_path) {
  auto res = parse_xattr_parameters(name, position, attribute_name, api_path);
#else
api_error fuse_base::parse_xattr_parameters(const char *name, const char *value, size_t size,
                                            std::string &attribute_name,
                                            const std::string &api_path) {
  auto res = parse_xattr_parameters(name, attribute_name, api_path);
#endif
  if (res != api_error::success) {
    return res;
  }

  return (value ? api_error::success : (size ? api_error::bad_address : api_error::success));
}

void fuse_base::populate_stat(const std::string &api_path, const std::uint64_t &size_or_count,
                              const api_meta_map &meta, const bool &directory, i_provider &provider,
                              struct stat *st) {
  memset(st, 0, sizeof(struct stat));
  st->st_nlink =
      (directory ? 2 + (size_or_count ? size_or_count : provider.get_directory_item_count(api_path))
                 : 1);
  if (directory) {
    st->st_blocks = 0;
  } else {
    st->st_size = size_or_count;
    static const auto blockSizeStat = static_cast<std::uint64_t>(512u);
    static const auto blockSize = static_cast<std::uint64_t>(4096u);
    const auto size =
        utils::divide_with_ceiling(static_cast<std::uint64_t>(st->st_size), blockSize) * blockSize;
    st->st_blocks =
        std::max(blockSize / blockSizeStat, utils::divide_with_ceiling(size, blockSizeStat));
  }
  st->st_gid = get_gid_from_meta(meta);
  st->st_mode = (directory ? S_IFDIR : S_IFREG) | get_mode_from_meta(meta);
  st->st_uid = get_uid_from_meta(meta);
#ifdef __APPLE__
  st->st_blksize = 0;
  st->st_flags = get_flags_from_meta(meta);

  set_timespec_from_meta(meta, META_MODIFIED, st->st_mtimespec);
  set_timespec_from_meta(meta, META_CREATION, st->st_birthtimespec);
  set_timespec_from_meta(meta, META_CHANGED, st->st_ctimespec);
  set_timespec_from_meta(meta, META_ACCESSED, st->st_atimespec);
#else  // __APPLE__
  st->st_blksize = 4096;

  set_timespec_from_meta(meta, META_MODIFIED, st->st_mtim);
  set_timespec_from_meta(meta, META_CREATION, st->st_ctim);
  set_timespec_from_meta(meta, META_ACCESSED, st->st_atim);
#endif // __APPLE__
}

void fuse_base::raise_fuse_event(std::string function_name, const std::string &api_path,
                                 const int &ret, const bool &disable_logging) {
  if ((ret >= 0) && disable_logging) {
    return;
  }

  if (not config_.get_enable_drive_events()) {
    return;
  }

  if (((config_.get_event_level() >= fuse_event::level) && (ret != 0)) ||
      (config_.get_event_level() >= event_level::verbose)) {
    event_system::instance().raise<fuse_event>(utils::string::right_trim(function_name, '_'),
                                               api_path, ret);
  }
}

int fuse_base::removexattr_(const char *path, const char *name) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().removexattr_impl(api_path, name);
                                     });
}

void fuse_base::set_timespec_from_meta(const api_meta_map &meta, const std::string &name,
                                       struct timespec &ts) {
  const auto t = utils::string::to_uint64(meta.at(name));
  ts.tv_nsec = t % NANOS_PER_SECOND;
  ts.tv_sec = t / NANOS_PER_SECOND;
}

#ifdef __APPLE__
int fuse_base::setxattr_(const char *path, const char *name, const char *value, size_t size,
                         int flags, uint32_t position) {
  const auto res = instance().execute_callback(
      __FUNCTION__, path, [&](const std::string &api_path) -> api_error {
        return instance().setxattr_impl(api_path, name, value, size, flags, position);
      });
  if (res != 0) {
    errno = std::abs(res);
  }

  return res;
}
#else  // __APPLE__
int fuse_base::setxattr_(const char *path, const char *name, const char *value, size_t size,
                         int flags) {
  const auto res = instance().execute_callback(
      __FUNCTION__, path, [&](const std::string &api_path) -> api_error {
        return instance().setxattr_impl(api_path, name, value, size, flags);
      });
  if (res != 0) {
    errno = std::abs(res);
  }

  return res;
}
#endif // __APPLE__
#endif // HAS_SETXATTR

int fuse_base::shutdown() {
#if __APPLE__
  const auto unmount = "umount \"" + mount_location_ + "\" >/dev/null 2>&1";
#else
  const auto unmount = "fusermount -u \"" + mount_location_ + "\" >/dev/null 2>&1";
#endif

  return system(unmount.c_str());
}

#ifdef __APPLE__
int fuse_base::setattr_x_(const char *path, struct setattr_x *attr) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().setattr_x_impl(api_path, attr);
                                     });
}

int fuse_base::setbkuptime_(const char *path, const struct timespec *bkuptime) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().setbkuptime_impl(api_path, bkuptime);
                                     });
}

int fuse_base::setchgtime_(const char *path, const struct timespec *chgtime) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().setchgtime_impl(api_path, chgtime);
                                     });
}

int fuse_base::setcrtime_(const char *path, const struct timespec *crtime) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().setcrtime_impl(api_path, crtime);
                                     });
}

int fuse_base::setvolname_(const char *volname) {
  return instance().execute_callback(__FUNCTION__, volname,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().setvolname_impl(volname);
                                     });
}

int fuse_base::statfs_x_(const char *path, struct statfs *stbuf) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().statfs_x_impl(api_path, stbuf);
                                     });
}
#else  // __APPLE__
int fuse_base::statfs_(const char *path, struct statvfs *stbuf) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().statfs_impl(api_path, stbuf);
                                     });
}
#endif // __APPLE__

int fuse_base::truncate_(const char *path, off_t size) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().truncate_impl(api_path, size);
                                     });
}

int fuse_base::unlink_(const char *path) {
  return instance().execute_callback(
      __FUNCTION__, path,
      [&](const std::string &api_path) -> api_error { return instance().unlink_impl(api_path); });
}

int fuse_base::utimens_(const char *path, const struct timespec tv[2]) {
  return instance().execute_callback(__FUNCTION__, path,
                                     [&](const std::string &api_path) -> api_error {
                                       return instance().utimens_impl(api_path, tv);
                                     });
}

int fuse_base::write_(const char *path, const char *buffer, size_t write_size, off_t write_offset,
                      struct fuse_file_info *fi) {
  std::size_t bytes_written{};

  const auto res = instance().execute_callback(
      __FUNCTION__, path,
      [&](const std::string &api_path) -> api_error {
        return instance().write_impl(api_path, buffer, write_size, write_offset, fi, bytes_written);
      },
      true);
  return (res == 0) ? static_cast<int>(bytes_written) : res;
}
} // namespace repertory

#endif // _WIN32
