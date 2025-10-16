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
#if FUSE_USE_VERSION < 30
  fuse_ops_.fgetattr = fuse_base::fgetattr_;
#endif // FUSE_USE_VERSION < 30
  fuse_ops_.init = fuse_base::init_;
  fuse_ops_.ioctl = fuse_base::ioctl_;
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
#endif // !defined(__APPLE__)
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

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().access_impl(std::move(api_path), mask);
      });
}

#if defined(__APPLE__)
auto fuse_base::chflags_(const char *path, uint32_t flags) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().chflags_impl(std::move(api_path), flags);
      });
}
#endif // defined(__APPLE__)

#if FUSE_USE_VERSION >= 30
auto fuse_base::chmod_(const char *path, mode_t mode,
                       struct fuse_file_info *f_info) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().chmod_impl(std::move(api_path), mode, f_info);
      });
}
#else  // FUSE_USE_VERSION < 30
auto fuse_base::chmod_(const char *path, mode_t mode) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().chmod_impl(std::move(api_path), mode);
      });
}
#endif // FUSE_USE_VERSION >= 30

#if FUSE_USE_VERSION >= 30
auto fuse_base::chown_(const char *path, uid_t uid, gid_t gid,
                       struct fuse_file_info *f_info) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().chown_impl(std::move(api_path), uid, gid, f_info);
      });
}
#else  // FUSE_USE_VERSION < 30
auto fuse_base::chown_(const char *path, uid_t uid, gid_t gid) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().chown_impl(std::move(api_path), uid, gid);
      });
}
#endif // FUSE_USE_VERSION >= 30

auto fuse_base::create_(const char *path, mode_t mode,
                        struct fuse_file_info *f_info) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().create_impl(std::move(api_path), mode, f_info);
      });
}

void fuse_base::destroy_(void *ptr) {
  REPERTORY_USES_FUNCTION_NAME();

  execute_void_callback(function_name, [&]() { instance().destroy_impl(ptr); });
}

void fuse_base::destroy_impl(void * /* ptr */) {
  REPERTORY_USES_FUNCTION_NAME();

  if (not foreground_) {
    repertory::project_cleanup();
  }
}

void fuse_base::display_options(
    [[maybe_unused]] std::vector<const char *> args) {
#if FUSE_USE_VERSION >= 30
  fuse_cmdline_help();
#else  // FUSE_USE_VERSION < 30
  struct fuse_operations fuse_ops{};
  fuse_main(args.size(),
            reinterpret_cast<char **>(const_cast<char **>(args.data())),
            &fuse_ops, nullptr);
#endif // FUSE_USE_VERSION >= 30

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
  auto from_api_file =
      utils::path::create_api_path(from == nullptr ? "" : from);
  auto to_api_file = utils::path::create_api_path(to == nullptr ? "" : to);
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
  auto api_path = utils::path::create_api_path(path == nullptr ? "" : path);
  auto res = utils::from_api_error(cb(api_path));
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
                           off_t length, struct fuse_file_info *f_info) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().fallocate_impl(std::move(api_path), mode, offset,
                                         length, f_info);
      });
}

#if FUSE_USE_VERSION < 30
auto fuse_base::fgetattr_(const char *path, struct stat *u_stat,
                          struct fuse_file_info *f_info) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().fgetattr_impl(std::move(api_path), u_stat, f_info);
      });
}
#endif // FUSE_USE_VERSION < 30

#if defined(__APPLE__)
auto fuse_base::fsetattr_x_(const char *path, struct setattr_x *attr,
                            struct fuse_file_info *f_info) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().fsetattr_x_impl(std::move(api_path), attr, f_info);
      });
}
#endif // defined(__APPLE__)

auto fuse_base::fsync_(const char *path, int datasync,
                       struct fuse_file_info *f_info) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().fsync_impl(std::move(api_path), datasync, f_info);
      });
}

#if FUSE_USE_VERSION < 30
auto fuse_base::ftruncate_(const char *path, off_t size,
                           struct fuse_file_info *f_info) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().ftruncate_impl(std::move(api_path), size, f_info);
      });
}
#endif // FUSE_USE_VERSION < 30

#if FUSE_USE_VERSION >= 30
auto fuse_base::getattr_(const char *path, struct stat *u_stat,
                         struct fuse_file_info *f_info) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().getattr_impl(std::move(api_path), u_stat, f_info);
      });
}
#else  // FUSE_USE_VERSION < 30
auto fuse_base::getattr_(const char *path, struct stat *u_stat) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().getattr_impl(std::move(api_path), u_stat);
      });
}
#endif // FUSE_USE_VERSION >= 30

#if defined(__APPLE__)
auto fuse_base::getxtimes_(const char *path, struct timespec *bkuptime,
                           struct timespec *crtime) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().getxtimes_impl(std::move(api_path), bkuptime, crtime);
      });
}
#endif // defined(__APPLE__)

#if FUSE_USE_VERSION >= 30
auto fuse_base::init_(struct fuse_conn_info *conn, struct fuse_config *cfg)
    -> void * {
  REPERTORY_USES_FUNCTION_NAME();

  return execute_void_pointer_callback(function_name, [&]() -> void * {
    return instance().init_impl(conn, cfg);
  });
}
#else  // FUSE_USE_VERSION < 30
auto fuse_base::init_(struct fuse_conn_info *conn) -> void * {
  REPERTORY_USES_FUNCTION_NAME();

  return execute_void_pointer_callback(
      function_name, [&]() -> void * { return instance().init_impl(conn); });
}
#endif // FUSE_USE_VERSION >= 30

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
    event_system::instance().raise<unmount_requested>(function_name);
    return this;
  }

  if (not foreground_ && not repertory::project_initialize()) {
    utils::error::raise_error(function_name, "failed to initialize repertory");
    event_system::instance().raise<unmount_requested>(function_name);
    repertory::project_cleanup();
  }

  return this;
}

auto fuse_base::ioctl_(const char *path, int cmd, void *arg,
                       struct fuse_file_info *f_info, unsigned int /* flags */,
                       void * /* data */) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().ioctl_impl(std::move(api_path), cmd, arg, f_info);
      });
}

auto fuse_base::mkdir_(const char *path, mode_t mode) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().mkdir_impl(std::move(api_path), mode);
      });
}

auto fuse_base::mount([[maybe_unused]] std::vector<std::string> orig_args,
                      std::vector<std::string> args,
                      [[maybe_unused]] provider_type prov,
                      [[maybe_unused]] std::string_view unique_id) -> int {
  auto ret{parse_args(args)};
  if (ret != 0) {
    return ret;
  }

  std::vector<const char *> fuse_argv(args.size());
  for (std::size_t idx{0U}; idx < args.size(); ++idx) {
    fuse_argv[idx] = args[idx].c_str();
  }

  {
    struct fuse_args f_args = FUSE_ARGS_INIT(
        static_cast<int>(fuse_argv.size()),
        reinterpret_cast<char **>(const_cast<char **>(fuse_argv.data())));

    char *mount_location{nullptr};
#if FUSE_USE_VERSION >= 30
    struct fuse_cmdline_opts opts{};
    fuse_parse_cmdline(&f_args, &opts);
    mount_location = opts.mountpoint;
#else  // FUSE_USE_VERSION < 30
    ret = fuse_parse_cmdline(&f_args, &mount_location, nullptr, nullptr);
    if (ret != 0) {
      std::cerr << "FATAL: Failed to process fuse command line options"
                << std::endl;
      return -1;
    }
#endif // FUSE_USE_VERSION >= 30

    if (mount_location != nullptr) {
      mount_location_ = mount_location;
      free(mount_location);
      mount_location = nullptr;
    }
  }

#if defined(__APPLE__)
  label_ = std::format("com.fifthgrid.blockstorage.repertory.{}.{}",
                       provider_type_to_string(prov), unique_id);
  if (not foreground_) {
    if (not utils::file::change_to_process_directory()) {
      std::cerr << "FATAL: Failed to change to process directory" << std::endl;
      return -1;
    }

    orig_args[0U] = utils::path::combine(".", {REPERTORY});
    orig_args.insert(std::next(orig_args.begin()), "-f");

    utils::plist_cfg cfg{};
    cfg.args = orig_args;
    cfg.keep_alive = false;
    cfg.keep_alive = false;
    cfg.label = label_;
    cfg.plist_path = utils::path::combine("~", {"/Library/LaunchAgents"});
    cfg.run_at_load = false;
    cfg.stderr_log = fmt::format("/tmp/repertory_{}_{}.err",
                                 provider_type_to_string(prov), unique_id);
    cfg.stdout_log = fmt::format("/tmp/repertory_{}_{}.out",
                                 provider_type_to_string(prov), unique_id);
    cfg.working_dir = utils::path::absolute(".");

    if (not utils::generate_launchd_plist(cfg, true)) {
      std::cerr << fmt::format("FATAL: Failed to generate plist|{}", label_)
                << std::endl;
      return -1;
    }

    ret = utils::launchctl_command(label_, utils::launchctl_type::bootout);
    if (ret != 0) {
      std::cout << fmt::format("WARN: Failed to bootout {}/{}", getuid(),
                               label_)
                << std::endl;
    }

    ret = utils::launchctl_command(label_, utils::launchctl_type::bootstrap);
    if (ret != 0) {
      std::cout << fmt::format("WARN: Failed to bootstrap {}/{}", getuid(),
                               label_)
                << std::endl;
    }

    ret = utils::launchctl_command(label_, utils::launchctl_type::kickstart);
    if (ret != 0) {
      std::cerr << fmt::format("FATAL: Failed to kickstart {}/{}", getuid(),
                               label_)
                << std::endl;
    }

    return ret;
  }
#endif //  defined(__APPLE__)

  if (not foreground_) {
    repertory::project_cleanup();
  }

  notify_fuse_args_parsed(args);

#if FUSE_USE_VERSION < 30
  umask(0);
#endif // FUSE_USE_VERSION < 30

  ret = fuse_main(
      static_cast<int>(fuse_argv.size()),
      reinterpret_cast<char **>(const_cast<char **>(fuse_argv.data())),
      &fuse_ops_, this);
  notify_fuse_main_exit(ret);

  return ret;
}

auto fuse_base::open_(const char *path, struct fuse_file_info *f_info) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().open_impl(std::move(api_path), f_info);
      });
}

auto fuse_base::opendir_(const char *path, struct fuse_file_info *f_info)
    -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().opendir_impl(std::move(api_path), f_info);
      });
}

auto fuse_base::read_(const char *path, char *buffer, size_t read_size,
                      off_t read_offset, struct fuse_file_info *f_info) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  std::size_t bytes_read{};
  auto res = instance().execute_callback(
      function_name, path,
      [&](std::string api_path) -> api_error {
        return instance().read_impl(std::move(api_path), buffer, read_size,
                                    read_offset, f_info, bytes_read);
      },
      true);
  return (res == 0) ? static_cast<int>(bytes_read) : res;
}

#if FUSE_USE_VERSION >= 30
auto fuse_base::readdir_(const char *path, void *buf,
                         fuse_fill_dir_t fuse_fill_dir, off_t offset,
                         struct fuse_file_info *f_info,
                         fuse_readdir_flags flags) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().readdir_impl(std::move(api_path), buf, fuse_fill_dir,
                                       offset, f_info, flags);
      });
}
#else  // FUSE_USE_VERSION < 30
auto fuse_base::readdir_(const char *path, void *buf,
                         fuse_fill_dir_t fuse_fill_dir, off_t offset,
                         struct fuse_file_info *f_info) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().readdir_impl(std::move(api_path), buf, fuse_fill_dir,
                                       offset, f_info);
      });
}
#endif // FUSE_USE_VERSION >= 30

auto fuse_base::release_(const char *path, struct fuse_file_info *f_info)
    -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().release_impl(std::move(api_path), f_info);
      });
}

auto fuse_base::releasedir_(const char *path, struct fuse_file_info *f_info)
    -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().releasedir_impl(std::move(api_path), f_info);
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
#else  // FUSE_USE_VERSION < 30
auto fuse_base::rename_(const char *from, const char *to) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, from, to,
      [&](std::string from_api_file, std::string to_api_path) -> api_error {
        return instance().rename_impl(std::move(from_api_file),
                                      std::move(to_api_path));
      });
}
#endif // FUSE_USE_VERSION >= 30

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
  auto res = instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().getxattr_impl(std::move(api_path), name, value, size,
                                        position, attribute_size);
      });

  return res == 0 ? attribute_size : res;
}
#else  // !defined(__APPLE__)
auto fuse_base::getxattr_(const char *path, const char *name, char *value,
                          size_t size) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  int attribute_size = 0;
  auto res = instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().getxattr_impl(std::move(api_path), name, value, size,
                                        attribute_size);
      });

  return res == 0 ? attribute_size : res;
}
#endif // defined(__APPLE__)

auto fuse_base::listxattr_(const char *path, char *buffer, size_t size) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  int required_size = 0;
  bool return_size = false;

  auto res = instance().execute_callback(
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
  auto force_no_console{false};
  for (std::size_t idx{1U}; !force_no_console && (idx < args.size()); ++idx) {
    if (args[idx] == "-nc") {
      force_no_console = true;
      console_enabled_ = false;
    }
  }
  utils::collection::remove_element(args, "-nc");

  for (std::size_t idx{1U}; idx < args.size(); ++idx) {
    if (args[idx] == "-f") {
      foreground_ = true;
    } else if (args[idx].starts_with("-o")) {
      std::string options;
      if (args[idx].size() == 2U) {
        if ((idx + 1) < args.size()) {
          options = args[++idx];
        }
      } else {
        options = args[idx].substr(2);
      }

      auto option_parts = utils::string::split(options, ',', true);
      for (const auto &option : option_parts) {
        if (option.starts_with("gid")) {
          auto parts = utils::string::split(option, '=', true);
          if (parts.size() == 2U) {
            auto *gid = getgrnam(parts[1U].c_str());
            if (gid == nullptr) {
              gid = getgrgid(utils::string::to_uint32(parts[1U]));
            }
            if ((getgid() != 0) && (gid->gr_gid == 0)) {
              std::cerr << "FATAL: 'gid=0' requires running as root"
                        << std::endl;
              return -1;
            }

            forced_gid_ = gid->gr_gid;
          }
        } else if (option.starts_with("noatime")) {
          atime_enabled_ = false;
        } else if (option.starts_with("uid")) {
          auto parts = utils::string::split(option, '=', true);
          if (parts.size() == 2U) {
            auto *uid = getpwnam(parts[1U].c_str());
            if (uid == nullptr) {
              uid = getpwuid(utils::string::to_uint32(parts[1]));
            }
            if ((getuid() != 0) && (uid->pw_uid == 0)) {
              std::cerr << "FATAL: 'uid=0' requires running as root"
                        << std::endl;
              return -1;
            }

            forced_uid_ = uid->pw_uid;
          }
        } else if (option.starts_with("umask")) {
          auto parts = utils::string::split(option, '=', true);
          if (parts.size() == 2U) {
            static const auto match_number_regex = std::regex("[0-9]+");
            try {
              if (not std::regex_match(parts[1], match_number_regex)) {
                throw std::runtime_error("invalid syntax");
              }

              forced_umask_ = utils::string::to_uint32(parts[1]);
            } catch (...) {
              std::cerr << ("FATAL: '" + option + "' invalid syntax")
                        << std::endl;
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

  auto res = instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().setxattr_impl(std::move(api_path), name, value, size,
                                        flags, position);
      });
  if (res != 0) {
    errno = std::abs(res);
  }

  return res;
}
#else  // !defined(__APPLE__)
auto fuse_base::setxattr_(const char *path, const char *name, const char *value,
                          size_t size, int flags) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  auto res = instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().setxattr_impl(std::move(api_path), name, value, size,
                                        flags);
      });
  if (res != 0) {
    errno = std::abs(res);
  }

  return res;
}
#endif // defined(__APPLE__)
#endif // defined(HAS_SETXATTR)

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
#else  // !defined(__APPLE__)
auto fuse_base::statfs_(const char *path, struct statvfs *stbuf) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().statfs_impl(std::move(api_path), stbuf);
      });
}
#endif // defined(__APPLE__)

#if FUSE_USE_VERSION >= 30
auto fuse_base::truncate_(const char *path, off_t size,
                          struct fuse_file_info *f_info) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().truncate_impl(std::move(api_path), size, f_info);
      });
}
#else  // FUSE_USE_VERSION < 30
auto fuse_base::truncate_(const char *path, off_t size) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().truncate_impl(std::move(api_path), size);
      });
}
#endif // FUSE_USE_VERSION >= 30

auto fuse_base::unlink_(const char *path) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().unlink_impl(std::move(api_path));
      });
}

auto fuse_base::unmount(std::string_view mount_location) -> int {
  REPERTORY_USES_FUNCTION_NAME();

#if defined(__APPLE__)
  if (not foreground_ &&
      not utils::remove_launchd_plist(
          utils::path::combine("~", {"/Library/LaunchAgents"}), label_, true)) {
    utils::error::raise_error(
        function_name,
        fmt::format("failed to remove launchd entry|label|{}", label_));
  }
  auto cmd = fmt::format("umount \"{}\" >/dev/null 2>&1", mount_location);
#else // !defined(__APPLE__)
#if FUSE_USE_VERSION >= 30
  auto cmd =
      fmt::format("fusermount3 -u \"{}\" >/dev/null 2>&1", mount_location);
#else  // FUSE_USE_VERSION < 30
  auto cmd =
      fmt::format("fusermount -u \"{}\" >/dev/null 2>&1", mount_location);
#endif // FUSE_USE_VERSION >= 30
#endif // defined(__APPLE__)

  return system(cmd.c_str());
#if defined(__APPLE__)
#endif // defined(__APPLE__)
}

#if FUSE_USE_VERSION >= 30
auto fuse_base::utimens_(const char *path, const struct timespec tv[2],
                         struct fuse_file_info *f_info) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().utimens_impl(std::move(api_path), tv, f_info);
      });
}
#else  // FUSE_USE_VERSION < 30
auto fuse_base::utimens_(const char *path, const struct timespec tv[2]) -> int {
  REPERTORY_USES_FUNCTION_NAME();

  return instance().execute_callback(
      function_name, path, [&](std::string api_path) -> api_error {
        return instance().utimens_impl(std::move(api_path), tv);
      });
}
#endif // FUSE_USE_VERSION >= 30

auto fuse_base::write_(const char *path, const char *buffer, size_t write_size,
                       off_t write_offset, struct fuse_file_info *f_info)
    -> int {
  REPERTORY_USES_FUNCTION_NAME();

  std::size_t bytes_written{};

  auto res = instance().execute_callback(
      function_name, path,
      [&](std::string api_path) -> api_error {
        return instance().write_impl(std::move(api_path), buffer, write_size,
                                     write_offset, f_info, bytes_written);
      },
      true);
  return (res == 0) ? static_cast<int>(bytes_written) : res;
}
} // namespace repertory

#endif // !defined(_WIN32)
