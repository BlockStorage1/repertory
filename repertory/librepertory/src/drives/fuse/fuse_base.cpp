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

#include "drives/fuse/fuse_base.hpp"

#include "app_config.hpp"
#include "events/event_system.hpp"
#include "events/types/fuse_args_parsed.hpp"
#include "events/types/fuse_event.hpp"
#include "events/types/unmount_requested.hpp"
#include "events/types/unmount_result.hpp"
#include "initialize.hpp"
#include "platform/platform.hpp"
#include "utils/collection.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"

namespace repertory {
auto fuse_base::instance() -> fuse_base & {
  return *reinterpret_cast<fuse_base *>(fuse_get_context()->private_data);
}

fuse_base::fuse_base(app_config &config) : config_(config) {
  fuse_ops_.access = fuse_base::access_;
  fuse_ops_.chmod = fuse_base::chmod_;
  fuse_ops_.chown = fuse_base::chown_;
  fuse_ops_.create = fuse_base::create_;
  fuse_ops_.destroy = fuse_base::destroy_;
  fuse_ops_.fallocate = fuse_base::fallocate_;
  fuse_ops_.fsync = fuse_base::fsync_;
  fuse_ops_.getattr = fuse_base::getattr_;
  fuse_ops_.init = fuse_base::init_;
  fuse_ops_.mkdir = fuse_base::mkdir_;
  fuse_ops_.open = fuse_base::open_;
  fuse_ops_.opendir = fuse_base::opendir_;
  fuse_ops_.read = fuse_base::read_;
  fuse_ops_.readdir = fuse_base::readdir_;
  fuse_ops_.release = fuse_base::release_;
  fuse_ops_.releasedir = fuse_base::releasedir_;
  fuse_ops_.rename = fuse_base::rename_;
  fuse_ops_.rmdir = fuse_base::rmdir_;
  fuse_ops_.truncate = fuse_base::truncate_;
#if !defined(__APPLE__)
  fuse_ops_.statfs = fuse_base::statfs_;
#endif // __APPLE__
  fuse_ops_.unlink = fuse_base::unlink_;
  fuse_ops_.utimens = fuse_base::utimens_;
  fuse_ops_.write = fuse_base::write_;
#if defined(HAS_SETXATTR)
  fuse_ops_.getxattr = fuse_base::getxattr_;
  fuse_ops_.listxattr = fuse_base::listxattr_;
  fuse_ops_.removexattr = fuse_base::removexattr_;
  fuse_ops_.setxattr = fuse_base::setxattr_;
#endif // defined(HAS_SETXATTR)
#if defined(__APPLE__)
  fuse_ops_.chflags = fuse_base::chflags_;
  fuse_ops_.fsetattr_x = fuse_base::fsetattr_x_;
  fuse_ops_.getxtimes = fuse_base::getxtimes_;
  fuse_ops_.setattr_x = fuse_base::setattr_x_;
  fuse_ops_.setbkuptime = fuse_base::setbkuptime_;
  fuse_ops_.setchgtime = fuse_base::setchgtime_;
  fuse_ops_.setcrtime = fuse_base::setcrtime_;
  fuse_ops_.setvolname = fuse_base::setvolname_;
  fuse_ops_.statfs_x = fuse_base::statfs_x_;
#endif // defined(__APPLE__)
#if FUSE_USE_VERSION < 30
  fuse_ops_.flag_nullpath_ok = 0;
  fuse_ops_.flag_nopath = 0;
  fuse_ops_.flag_utime_omit_ok = 1;
  fuse_ops_.flag_reserved = 0;
#endif // FUSE_USE_VERSION < 30

  E_SUBSCRIBE(unmount_requested, [this](const unmount_requested &) {
    std::thread([this]() { this->shutdown(); }).detach();
  });
}

fuse_base::~fuse_base() { E_CONSUMER_RELEASE(); }

auto fuse_base::access_(const char *path, int mask) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().access_impl(std::move(api_path), mask);
      });
}

#if defined(__APPLE__)
auto fuse_base::chflags_(const char *path, uint32_t flags) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().chflags_impl(std::move(api_path), flags);
      });
}
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
auto fuse_base::chmod_(const char *path, mode_t mode, struct fuse_file_info *fi)
    -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().chmod_impl(std::move(api_path), mode, fi);
      });
}
#else
auto fuse_base::chmod_(const char *path, mode_t mode) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().chmod_impl(std::move(api_path), mode);
      });
}
#endif

#if FUSE_USE_VERSION >= 30
auto fuse_base::chown_(const char *path, uid_t uid, gid_t gid,
                       struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().chown_impl(std::move(api_path), uid, gid, fi);
      });
}
#else
auto fuse_base::chown_(const char *path, uid_t uid, gid_t gid) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().chown_impl(std::move(api_path), uid, gid);
      });
}
#endif

auto fuse_base::create_(const char *path, mode_t mode,
                        struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().create_impl(std::move(api_path), mode, fi);
      });
}

void fuse_base::destroy_(void *ptr) {
  REPERTORY_USES_FUNCTION_NAME();

  execute_void_callback(function_name, [&]() { instance().destroy_impl(ptr); });
}

void fuse_base::destroy_impl(void * /* ptr */) {
  if (not console_enabled_) {
    repertory::project_cleanup();
  }
}

void fuse_base::display_options(
    [[maybe_unused]] std::vector<const char *> args) {
#if FUSE_USE_VERSION >= 30
  fuse_cmdline_help();
#else
  struct fuse_operations fuse_ops{};
  fuse_main(args.size(),
            reinterpret_cast<char **>(const_cast<char **>(args.data())),
            &fuse_ops, nullptr);
#endif

  std::cout << std::endl;
}

void fuse_base::display_version_information(std::vector<const char *> args) {
  struct fuse_operations fuse_ops{};
  fuse_main(static_cast<int>(args.size()),
            reinterpret_cast<char **>(const_cast<char **>(args.data())),
            &fuse_ops, nullptr);
}

auto fuse_base::execute_callback(
    std::string_view function_name, const char *from, const char *to,
    const std::function<api_error(std::string from_api_file,
                                  std::string to_api_path)> &cb,
    bool disable_logging) -> int {
  auto from_api_file = utils::path::create_api_path(from ? from : "");
  auto to_api_file = utils::path::create_api_path(to ? to : "");
  auto res = utils::from_api_error(cb(from_api_file, to_api_file));
  raise_fuse_event(function_name,
                   "from|" + from_api_file + "|to|" + to_api_file, res,
                   disable_logging);
  return res;
}

auto fuse_base::execute_callback(
    std::string_view function_name, const char *path,
    const std::function<api_error(std::string api_path)> &cb,
    bool disable_logging) -> int {
  const auto api_path = utils::path::create_api_path(path ? path : "");
  const auto res = utils::from_api_error(cb(api_path));
  raise_fuse_event(function_name, api_path, res, disable_logging);
  return res;
}

void fuse_base::execute_void_callback(std::string_view function_name,
                                      const std::function<void()> &cb) {
  cb();

  instance().raise_fuse_event(function_name, "", 0, false);
}

auto fuse_base::execute_void_pointer_callback(std::string_view function_name,
                                              const std::function<void *()> &cb)
    -> void * {
  auto *ret = cb();

  instance().raise_fuse_event(function_name, "", 0, false);

  return ret;
}

auto fuse_base::fallocate_(const char *path, int mode, off_t offset,
                           off_t length, struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().fallocate_impl(std::move(api_path), mode, offset,
                                         length, fi);
      });
}

#if FUSE_USE_VERSION < 30
auto fuse_base::fgetattr_(const char *path, struct stat *st,
                          struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().fgetattr_impl(std::move(api_path), st, fi);
      });
}
#endif

#if defined(__APPLE__)
auto fuse_base::fsetattr_x_(const char *path, struct setattr_x *attr,
                            struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().fsetattr_x_impl(std::move(api_path), attr, fi);
      });
}
#endif // __APPLE__

auto fuse_base::fsync_(const char *path, int datasync,
                       struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().fsync_impl(std::move(api_path), datasync, fi);
      });
}

#if FUSE_USE_VERSION < 30
auto fuse_base::ftruncate_(const char *path, off_t size,
                           struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().ftruncate_impl(std::move(api_path), size, fi);
      });
}
#endif

#if FUSE_USE_VERSION >= 30
auto fuse_base::getattr_(const char *path, struct stat *st,
                         struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().getattr_impl(std::move(api_path), st, fi);
      });
}
#else
auto fuse_base::getattr_(const char *path, struct stat *st) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().getattr_impl(std::move(api_path), st);
      });
}
#endif

#if defined(__APPLE__)
auto fuse_base::getxtimes_(const char *path, struct timespec *bkuptime,
                           struct timespec *crtime) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().getxtimes_impl(std::move(api_path), bkuptime, crtime);
      });
}
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
auto fuse_base::init_(struct fuse_conn_info *conn, struct fuse_config *cfg)
    -> void * {
  REPERTORY_USES_FUNCTION_NAME();

  return execute_void_pointer_callback(function_name, [&]() -> void * {
    return instance().init_impl(conn, cfg);
  });
}
#else
auto fuse_base::init_(struct fuse_conn_info *conn) -> void * {
  REPERTORY_USES_FUNCTION_NAME();

  return execute_void_pointer_callback(
      function_name, [&]() -> void * { return instance().init_impl(conn); });
}
#endif

#if FUSE_USE_VERSION >= 30
auto fuse_base::init_impl([[maybe_unused]] struct fuse_conn_info *conn,
                          struct fuse_config *cfg) -> void * {
#else  // FUSE_USE_VERSION < 30
auto fuse_base::init_impl(struct fuse_conn_info *conn) -> void * {
#endif // FUSE_USE_VERSION >= 30
  REPERTORY_USES_FUNCTION_NAME();

#if defined(__APPLE__)
  conn->want |= FUSE_CAP_VOL_RENAME;
  conn->want |= FUSE_CAP_XTIMES;
#endif // defined(__APPLE__)

#if FUSE_USE_VERSION >= 30
  cfg->nullpath_ok = 0;
  cfg->hard_remove = 1;
#endif // FUSE_USE_VERSION >= 30

  if (not utils::file::change_to_process_directory()) {
    utils::error::raise_error(function_name,
                              "failed to change to process directory");
    event_system::instance().raise<unmount_requested>();
    return this;
  }

  if (not console_enabled_ && not repertory::project_initialize()) {
    utils::error::raise_error(function_name, "failed to initialize repertory");
    event_system::instance().raise<unmount_requested>();
  }

  return this;
}

auto fuse_base::mkdir_(const char *path, mode_t mode) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().mkdir_impl(std::move(api_path), mode);
      });
}

auto fuse_base::mount(std::vector<std::string> args) -> int {
  auto ret = parse_args(args);
  if (ret == 0) {
    std::vector<const char *> fuse_argv(args.size());
    for (std::size_t i = 0u; i < args.size(); ++i) {
      fuse_argv[i] = args[i].c_str();
    }

    {
      struct fuse_args fa = FUSE_ARGS_INIT(
          static_cast<int>(fuse_argv.size()),
          reinterpret_cast<char **>(const_cast<char **>(fuse_argv.data())));

      char *mount_location{nullptr};
#if FUSE_USE_VERSION >= 30
      struct fuse_cmdline_opts opts{};
      fuse_parse_cmdline(&fa, &opts);
      mount_location = opts.mountpoint;
#else
      fuse_parse_cmdline(&fa, &mount_location, nullptr, nullptr);
#endif

      if (mount_location) {
        mount_location_ = mount_location;
        free(mount_location);
      }
    }

    notify_fuse_args_parsed(args);

#if FUSE_USE_VERSION < 30
    umask(0);
#endif

    if (not console_enabled_) {
      repertory::project_cleanup();
    }

    ret = fuse_main(
        static_cast<int>(fuse_argv.size()),
        reinterpret_cast<char **>(const_cast<char **>(fuse_argv.data())),
        &fuse_ops_, this);
    notify_fuse_main_exit(ret);
  }

  return ret;
}

auto fuse_base::open_(const char *path, struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().open_impl(std::move(api_path), fi);
      });
}

auto fuse_base::opendir_(const char *path, struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().opendir_impl(std::move(api_path), fi);
      });
}

auto fuse_base::read_(const char *path, char *buffer, size_t read_size,
                      off_t read_offset, struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  std::size_t bytes_read{};
  const auto res = instance().execute_callback(
      function_name, path,
      [&](std::string api_path) -> api_error {
        return instance().read_impl(std::move(api_path), buffer, read_size,
                                    read_offset, fi, bytes_read);
      },
      true);
  return (res == 0) ? static_cast<int>(bytes_read) : res;
}

#if FUSE_USE_VERSION >= 30
auto fuse_base::readdir_(const char *path, void *buf,
                         fuse_fill_dir_t fuse_fill_dir, off_t offset,
                         struct fuse_file_info *fi, fuse_readdir_flags flags)
    -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().readdir_impl(std::move(api_path), buf, fuse_fill_dir,
                                       offset, fi, flags);
      });
}
#else
auto fuse_base::readdir_(const char *path, void *buf,
                         fuse_fill_dir_t fuse_fill_dir, off_t offset,
                         struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().readdir_impl(std::move(api_path), buf, fuse_fill_dir,
                                       offset, fi);
      });
}
#endif

auto fuse_base::release_(const char *path, struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().release_impl(std::move(api_path), fi);
      });
}

auto fuse_base::releasedir_(const char *path, struct fuse_file_info *fi)
    -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().releasedir_impl(std::move(api_path), fi);
      });
}

#if FUSE_USE_VERSION >= 30
auto fuse_base::rename_(const char *from, const char *to, unsigned int flags)
    -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, from, to,
      [&](std::string from_api_file, std::string to_api_path) -> api_error {
        return instance().rename_impl(std::move(from_api_file),
                                      std::move(to_api_path), flags);
      });
}
#else
auto fuse_base::rename_(const char *from, const char *to) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, from, to,
      [&](std::string from_api_file, std::string to_api_path) -> api_error {
        return instance().rename_impl(std::move(from_api_file),
                                      std::move(to_api_path));
      });
}
#endif

auto fuse_base::rmdir_(const char *path) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().rmdir_impl(std::move(api_path));
      });
}

#if defined(HAS_SETXATTR)
#if defined(__APPLE__)
auto fuse_base::getxattr_(const char *path, const char *name, char *value,
                          size_t size, uint32_t position) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  int attribute_size = 0;
  const auto res = instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().getxattr_impl(std::move(api_path), name, value, size,
                                        position, attribute_size);
      });

  return res == 0 ? attribute_size : res;
}
#else  // __APPLE__
auto fuse_base::getxattr_(const char *path, const char *name, char *value,
                          size_t size) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  int attribute_size = 0;
  const auto res = instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().getxattr_impl(std::move(api_path), name, value, size,
                                        attribute_size);
      });

  return res == 0 ? attribute_size : res;
}
#endif // __APPLE__

auto fuse_base::listxattr_(const char *path, char *buffer, size_t size) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  int required_size = 0;
  bool return_size = false;

  const auto res = instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().listxattr_impl(std::move(api_path), buffer, size,
                                         required_size, return_size);
      });

  return return_size ? required_size : res;
}

void fuse_base::notify_fuse_args_parsed(const std::vector<std::string> &args) {
  REPERTORY_USES_FUNCTION_NAME();

  event_system::instance().raise<fuse_args_parsed>(
      std::accumulate(args.begin(), args.end(), std::string(),
                      [](auto &&command_line, auto &&arg) -> auto {
                        command_line +=
                            (command_line.empty() ? arg
                                                  : (" " + std::string(arg)));
                        return command_line;
                      }),
      function_name);
}

auto fuse_base::parse_args(std::vector<std::string> &args) -> int {
  auto force_no_console = false;
  for (std::size_t i = 1u; !force_no_console && (i < args.size()); ++i) {
    if (args[i] == "-nc") {
      force_no_console = true;
    }
  }
  utils::collection::remove_element(args, "-nc");

  for (std::size_t i = 1u; i < args.size(); ++i) {
    if (args[i] == "-f") {
      console_enabled_ = not force_no_console;
    } else if (args[i].find("-o") == 0) {
      std::string options = "";
      if (args[i].size() == 2u) {
        if ((i + 1) < args.size()) {
          options = args[++i];
        }
      } else {
        options = args[i].substr(2);
      }

      const auto option_parts = utils::string::split(options, ',', true);
      for (const auto &option : option_parts) {
        if (option.find("gid") == 0) {
          const auto parts = utils::string::split(option, '=', true);
          if (parts.size() == 2u) {
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
          const auto parts = utils::string::split(option, '=', true);
          if (parts.size() == 2u) {
            auto *uid = getpwnam(parts[1u].c_str());
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
          const auto parts = utils::string::split(option, '=', true);
          if (parts.size() == 2u) {
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

void fuse_base::raise_fuse_event(std::string_view function_name,
                                 std::string_view api_path, int ret,
                                 bool disable_logging) {
  if ((ret >= 0) && disable_logging) {
    return;
  }

  if (not config_.get_enable_drive_events()) {
    return;
  }

  if (((config_.get_event_level() >= fuse_event::level) && (ret != 0)) ||
      (config_.get_event_level() >= event_level::trace)) {
    std::string func{function_name};
    event_system::instance().raise<fuse_event>(
        api_path, ret, utils::string::right_trim(func, '_'));
  }
}

auto fuse_base::removexattr_(const char *path, const char *name) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().removexattr_impl(std::move(api_path), name);
      });
}

#if defined(__APPLE__)
auto fuse_base::setxattr_(const char *path, const char *name, const char *value,
                          size_t size, int flags, uint32_t position) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  const auto res = instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().setxattr_impl(std::move(api_path), name, value, size,
                                        flags, position);
      });
  if (res != 0) {
    errno = std::abs(res);
  }

  return res;
}
#else  // __APPLE__
auto fuse_base::setxattr_(const char *path, const char *name, const char *value,
                          size_t size, int flags) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  const auto res = instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().setxattr_impl(std::move(api_path), name, value, size,
                                        flags);
      });
  if (res != 0) {
    errno = std::abs(res);
  }

  return res;
}
#endif // __APPLE__
#endif // HAS_SETXATTR

void fuse_base::shutdown() {
  REPERTORY_USES_FUNCTION_NAME();

  auto res = unmount(get_mount_location());
  event_system::instance().raise<unmount_result>(function_name,
                                                 get_mount_location(), res);
}

#if defined(__APPLE__)
auto fuse_base::setattr_x_(const char *path, struct setattr_x *attr) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().setattr_x_impl(std::move(api_path), attr);
      });
}

auto fuse_base::setbkuptime_(const char *path, const struct timespec *bkuptime)
    -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().setbkuptime_impl(std::move(api_path), bkuptime);
      });
}

auto fuse_base::setchgtime_(const char *path, const struct timespec *chgtime)
    -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().setchgtime_impl(std::move(api_path), chgtime);
      });
}

auto fuse_base::setcrtime_(const char *path, const struct timespec *crtime)
    -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().setcrtime_impl(std::move(api_path), crtime);
      });
}

auto fuse_base::setvolname_(const char *volname) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, volname, [&](std::string api_path) -> api_error {
        return instance().setvolname_impl(std::move(volname));
      });
}

auto fuse_base::statfs_x_(const char *path, struct statfs *stbuf) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().statfs_x_impl(std::move(api_path), stbuf);
      });
}
#else  // __APPLE__
auto fuse_base::statfs_(const char *path, struct statvfs *stbuf) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().statfs_impl(std::move(api_path), stbuf);
      });
}
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
auto fuse_base::truncate_(const char *path, off_t size,
                          struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().truncate_impl(std::move(api_path), size, fi);
      });
}
#else
auto fuse_base::truncate_(const char *path, off_t size) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().truncate_impl(std::move(api_path), size);
      });
}
#endif

auto fuse_base::unlink_(const char *path) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().unlink_impl(std::move(api_path));
      });
}

auto fuse_base::unmount(const std::string &mount_location) -> int {
#if defined(__APPLE__)
  const auto cmd = "umount \"" + mount_location + "\" >/dev/null 2>&1";
#else
#if FUSE_USE_VERSION >= 30
  const auto cmd = "fusermount3 -u \"" + mount_location + "\" >/dev/null 2>&1";
#else
  const auto cmd = "fusermount -u \"" + mount_location + "\" >/dev/null 2>&1";
#endif
#endif

  return system(cmd.c_str());
}

#if FUSE_USE_VERSION >= 30
auto fuse_base::utimens_(const char *path, const struct timespec tv[2],
                         struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().utimens_impl(std::move(api_path), tv, fi);
      });
}
#else
auto fuse_base::utimens_(const char *path, const struct timespec tv[2]) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().utimens_impl(std::move(api_path), tv);
      });
}
#endif

auto fuse_base::write_(const char *path, const char *buffer, size_t write_size,
                       off_t write_offset, struct fuse_file_info *fi) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  std::size_t bytes_written{};

  const auto res = instance().execute_callback(
      function_name, path,
      [&](std::string api_path) -> api_error {
        return instance().write_impl(std::move(api_path), buffer, write_size,
                                     write_offset, fi, bytes_written);
      },
      true);
  return (res == 0) ? static_cast<int>(bytes_written) : res;
}
} // namespace repertory

#endif // _WIN32
