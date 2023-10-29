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
#ifndef INCLUDE_DRIVES_FUSE_FUSE_BASE_HPP_
#define INCLUDE_DRIVES_FUSE_FUSE_BASE_HPP_
#ifndef _WIN32

#include "events/event_system.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
class app_config;
class i_provider;

class fuse_base {
  E_CONSUMER();

public:
  explicit fuse_base(app_config &config);

  virtual ~fuse_base();

protected:
  app_config &config_;

private:
  std::string mount_location_;

protected:
  bool atime_enabled_ = true;
  bool console_enabled_ = false;
  std::optional<gid_t> forced_gid_;
  std::optional<uid_t> forced_uid_;
  std::optional<mode_t> forced_umask_;

private:
  static auto instance() -> fuse_base &;

private:
  struct fuse_operations fuse_ops_ {};

private:
  [[nodiscard]] auto execute_callback(
      const std::string &function_name, const char *from, const char *to,
      const std::function<api_error(const std::string &, const std::string &)>
          &cb,
      bool disable_logging = false) -> int;

  [[nodiscard]] auto
  execute_callback(const std::string &function_name, const char *path,
                   const std::function<api_error(const std::string &)> &cb,
                   bool disable_logging = false) -> int;

  static void execute_void_callback(const std::string &function_name,
                                    const std::function<void()> &cb);

  static auto execute_void_pointer_callback(const std::string &function_name,
                                            const std::function<void *()> &cb)
      -> void *;

  void raise_fuse_event(std::string function_name, const std::string &api_file,
                        int ret, bool disable_logging);

private:
  [[nodiscard]] static auto access_(const char *path, int mask) -> int;

#ifdef __APPLE__
  [[nodiscard]] static auto chflags_(const char *path, uint32_t flags) -> int;
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] static auto chmod_(const char *path, mode_t mode,
                                   struct fuse_file_info *fi) -> int;
#else
  [[nodiscard]] static auto chmod_(const char *path, mode_t mode) -> int;
#endif

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] static auto chown_(const char *path, uid_t uid, gid_t gid,
                                   struct fuse_file_info *fi) -> int;
#else
  [[nodiscard]] static auto chown_(const char *path, uid_t uid, gid_t gid)
      -> int;
#endif

  [[nodiscard]] static auto create_(const char *path, mode_t mode,
                                    struct fuse_file_info *fi) -> int;

  static void destroy_(void *ptr);

  [[nodiscard]] static auto fallocate_(const char *path, int mode, off_t offset,
                                       off_t length, struct fuse_file_info *fi)
      -> int;

#if FUSE_USE_VERSION < 30
  [[nodiscard]] static auto fgetattr_(const char *path, struct stat *st,
                                      struct fuse_file_info *fi) -> int;
#endif

#ifdef __APPLE__
  [[nodiscard]] static auto fsetattr_x_(const char *path,
                                        struct setattr_x *attr,
                                        struct fuse_file_info *fi) -> int;
#endif // __APPLE__

  [[nodiscard]] static auto fsync_(const char *path, int datasync,
                                   struct fuse_file_info *fi) -> int;

  [[nodiscard]] static auto ftruncate_(const char *path, off_t size,
                                       struct fuse_file_info *fi) -> int;

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] static auto getattr_(const char *path, struct stat *st,
                                     struct fuse_file_info *fi) -> int;
#else
  [[nodiscard]] static auto getattr_(const char *path, struct stat *st) -> int;
#endif

#ifdef __APPLE__
  [[nodiscard]] static auto getxtimes_(const char *path,
                                       struct timespec *bkuptime,
                                       struct timespec *crtime) -> int;
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] static auto init_(struct fuse_conn_info *conn,
                                  struct fuse_config *cfg) -> void *;
#else
  [[nodiscard]] static auto init_(struct fuse_conn_info *conn) -> void *;
#endif

  [[nodiscard]] static auto mkdir_(const char *path, mode_t mode) -> int;

  [[nodiscard]] static auto open_(const char *path, struct fuse_file_info *fi)
      -> int;

  [[nodiscard]] static auto opendir_(const char *path,
                                     struct fuse_file_info *fi) -> int;

  [[nodiscard]] static auto read_(const char *path, char *buffer,
                                  size_t read_size, off_t read_offset,
                                  struct fuse_file_info *fi) -> int;

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] static auto readdir_(const char *path, void *buf,
                                     fuse_fill_dir_t fuse_fill_dir,
                                     off_t offset, struct fuse_file_info *fi,
                                     fuse_readdir_flags flags) -> int;
#else
  [[nodiscard]] static auto readdir_(const char *path, void *buf,
                                     fuse_fill_dir_t fuse_fill_dir,
                                     off_t offset, struct fuse_file_info *fi)
      -> int;
#endif

  [[nodiscard]] static auto release_(const char *path,
                                     struct fuse_file_info *fi) -> int;

  [[nodiscard]] static auto releasedir_(const char *path,
                                        struct fuse_file_info *fi) -> int;

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] static auto rename_(const char *from, const char *to,
                                    unsigned int flags) -> int;
#else
  [[nodiscard]] static auto rename_(const char *from, const char *to) -> int;
#endif

  [[nodiscard]] static auto rmdir_(const char *path) -> int;

#ifdef HAS_SETXATTR
#ifdef __APPLE__
  [[nodiscard]] static auto getxattr_(const char *path, const char *name,
                                      char *value, size_t size,
                                      uint32_t position) -> int;

#else  // __APPLE__
  [[nodiscard]] static auto getxattr_(const char *path, const char *name,
                                      char *value, size_t size) -> int;
#endif // __APPLE__

  [[nodiscard]] static auto listxattr_(const char *path, char *buffer,
                                       size_t size) -> int;

  [[nodiscard]] static auto removexattr_(const char *path, const char *name)
      -> int;

#ifdef __APPLE__
  [[nodiscard]] static auto setxattr_(const char *path, const char *name,
                                      const char *value, size_t size, int flags,
                                      uint32_t position) -> int;

#else  // __APPLE__
  [[nodiscard]] static auto setxattr_(const char *path, const char *name,
                                      const char *value, size_t size, int flags)
      -> int;
#endif // __APPLE__
#endif // HAS_SETXATTR

#ifdef __APPLE__
  [[nodiscard]] static auto setattr_x_(const char *path, struct setattr_x *attr)
      -> int;

  [[nodiscard]] static auto setbkuptime_(const char *path,
                                         const struct timespec *bkuptime)
      -> int;

  [[nodiscard]] static auto setchgtime_(const char *path,
                                        const struct timespec *chgtime) -> int;

  [[nodiscard]] static auto setcrtime_(const char *path,
                                       const struct timespec *crtime) -> int;

  [[nodiscard]] static auto setvolname_(const char *volname) -> int;

  [[nodiscard]] static auto statfs_x_(const char *path, struct statfs *stbuf)
      -> int;

#else  // __APPLE__
  [[nodiscard]] static auto statfs_(const char *path, struct statvfs *stbuf)
      -> int;
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] static auto truncate_(const char *path, off_t size,
                                      struct fuse_file_info *fi) -> int;
#else
  [[nodiscard]] static auto truncate_(const char *path, off_t size) -> int;
#endif

  [[nodiscard]] static auto unlink_(const char *path) -> int;

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] static auto utimens_(const char *path,
                                     const struct timespec tv[2],
                                     struct fuse_file_info *fi) -> int;
#else
  [[nodiscard]] static auto utimens_(const char *path,
                                     const struct timespec tv[2]) -> int;
#endif

  [[nodiscard]] static auto write_(const char *path, const char *buffer,
                                   size_t write_size, off_t write_offset,
                                   struct fuse_file_info *fi) -> int;

protected:
  [[nodiscard]] virtual auto access_impl(std::string /*api_path*/, int /*mask*/)
      -> api_error {
    return api_error::not_implemented;
  }

#ifdef __APPLE__
  [[nodiscard]] virtual auto chflags_impl(std::string /*api_path*/,
                                          uint32_t /*flags*/) -> api_error {
    return api_error::not_implemented;
  }
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] virtual auto chmod_impl(std::string /*api_path*/,
                                        mode_t /*mode*/,
                                        struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }
#else
  [[nodiscard]] virtual auto chmod_impl(std::string /*api_path*/,
                                        mode_t /*mode*/) -> api_error {
    return api_error::not_implemented;
  }
#endif

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] virtual auto chown_impl(std::string /*api_path*/, uid_t /*uid*/,
                                        gid_t /*gid*/,
                                        struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }
#else
  [[nodiscard]] virtual auto chown_impl(std::string /*api_path*/, uid_t /*uid*/,
                                        gid_t /*gid*/) -> api_error {
    return api_error::not_implemented;
  }
#endif

  [[nodiscard]] virtual auto create_impl(std::string /*api_path*/,
                                         mode_t /*mode*/,
                                         struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }

  virtual void destroy_impl(void * /*ptr*/) { return; }

  [[nodiscard]] virtual auto
  fallocate_impl(std::string /*api_path*/, int /*mode*/, off_t /*offset*/,
                 off_t /*length*/, struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }

  [[nodiscard]] virtual auto fgetattr_impl(std::string /*api_path*/,
                                           struct stat * /*st*/,
                                           struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }

#ifdef __APPLE__
  [[nodiscard]] virtual auto fsetattr_x_impl(std::string /*api_path*/,
                                             struct setattr_x * /*attr*/,
                                             struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }
#endif // __APPLE__

  [[nodiscard]] virtual auto fsync_impl(std::string /*api_path*/,
                                        int /*datasync*/,
                                        struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }

#if FUSE_USE_VERSION < 30
  [[nodiscard]] virtual auto ftruncate_impl(std::string /*api_path*/,
                                            off_t /*size*/,
                                            struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }
#endif

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] virtual auto getattr_impl(std::string /*api_path*/,
                                          struct stat * /*st*/,
                                          struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }
#else
  [[nodiscard]] virtual auto getattr_impl(std::string /*api_path*/,
                                          struct stat * /*st*/) -> api_error {
    return api_error::not_implemented;
  }
#endif

#ifdef __APPLE__
  [[nodiscard]] virtual auto getxtimes_impl(std::string /*api_path*/,
                                            struct timespec * /*bkuptime*/,
                                            struct timespec * /*crtime*/)
      -> api_error {
    return api_error::not_implemented;
  }
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
  virtual auto init_impl(struct fuse_conn_info *conn, struct fuse_config *cfg)
      -> void *;
#else
  virtual auto init_impl(struct fuse_conn_info *conn) -> void *;
#endif

  [[nodiscard]] virtual auto mkdir_impl(std::string /*api_path*/,
                                        mode_t /*mode*/) -> api_error {
    return api_error::not_implemented;
  }

  [[nodiscard]] virtual auto open_impl(std::string /*api_path*/,
                                       struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }

  [[nodiscard]] virtual auto opendir_impl(std::string /*api_path*/,
                                          struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }

  [[nodiscard]] virtual auto
  read_impl(std::string /*api_path*/, char * /*buffer*/, size_t /*read_size*/,
            off_t /*read_offset*/, struct fuse_file_info * /*fi*/,
            std::size_t & /*bytes_read*/) -> api_error {
    return api_error::not_implemented;
  }

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] virtual auto
  readdir_impl(std::string /*api_path*/, void * /*buf*/,
               fuse_fill_dir_t /*fuse_fill_dir*/, off_t /*offset*/,
               struct fuse_file_info * /*fi*/, fuse_readdir_flags /*flags*/)
      -> api_error {
    return api_error::not_implemented;
  }
#else
  [[nodiscard]] virtual auto
  readdir_impl(std::string /*api_path*/, void * /*buf*/,
               fuse_fill_dir_t /*fuse_fill_dir*/, off_t /*offset*/,
               struct fuse_file_info * /*fi*/) -> api_error {
    return api_error::not_implemented;
  }
#endif

  [[nodiscard]] virtual auto release_impl(std::string /*api_path*/,
                                          struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }

  [[nodiscard]] virtual auto releasedir_impl(std::string /*api_path*/,
                                             struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] virtual auto rename_impl(std::string /*from_api_path*/,
                                         std::string /*to_api_path*/,
                                         unsigned int /*flags*/) -> api_error {
    return api_error::not_implemented;
  }
#else
  [[nodiscard]] virtual auto rename_impl(std::string /*from_api_path*/,
                                         std::string /*to_api_path*/)
      -> api_error {
    return api_error::not_implemented;
  }
#endif

  [[nodiscard]] virtual auto rmdir_impl(std::string /*api_path*/) -> api_error {
    return api_error::not_implemented;
  }

#ifdef HAS_SETXATTR
#ifdef __APPLE__
  [[nodiscard]] virtual auto
  getxattr_impl(std::string /*api_path*/, const char * /*name*/,
                char * /*value*/, size_t /*size*/, uint32_t /*position*/,
                int & /*attribute_size*/) -> api_error {
    return api_error::not_implemented;
  }
#else  // __APPLE__
  [[nodiscard]] virtual auto
  getxattr_impl(std::string /*api_path*/, const char * /*name*/,
                char * /*value*/, size_t /*size*/, int & /*attribute_size*/)
      -> api_error {
    return api_error::not_implemented;
  }
#endif // __APPLE__

  [[nodiscard]] virtual auto
  listxattr_impl(std::string /*api_path*/, char * /*buffer*/, size_t /*size*/,
                 int & /*required_size*/, bool & /*return_size*/) -> api_error {
    return api_error::not_implemented;
  }

  [[nodiscard]] virtual auto removexattr_impl(std::string /*api_path*/,
                                              const char * /*name*/)
      -> api_error {
    return api_error::not_implemented;
  }

#ifdef __APPLE__
  [[nodiscard]] virtual auto setxattr_impl(std::string /*api_path*/,
                                           const char * /*name*/,
                                           const char * /*value*/,
                                           size_t /*size*/, int /*flags*/,
                                           uint32_t /*position*/) -> api_error {
    return api_error::not_implemented;
  }
#else  // __APPLE__
  [[nodiscard]] virtual auto
  setxattr_impl(std::string /*api_path*/, const char * /*name*/,
                const char * /*value*/, size_t /*size*/, int /*flags*/)
      -> api_error {
    return api_error::not_implemented;
  }
#endif // __APPLE__
#endif // HAS_SETXATTR

#ifdef __APPLE__
  [[nodiscard]] virtual auto setattr_x_impl(std::string /*api_path*/,
                                            struct setattr_x * /*attr*/)
      -> api_error {
    return api_error::not_implemented;
  }

  [[nodiscard]] virtual auto
  setbkuptime_impl(std::string /*api_path*/,
                   const struct timespec * /*bkuptime*/) -> api_error {
    return api_error::not_implemented;
  }

  [[nodiscard]] virtual auto
  setchgtime_impl(std::string /*api_path*/, const struct timespec * /*chgtime*/)
      -> api_error {
    return api_error::not_implemented;
  }

  [[nodiscard]] virtual auto setcrtime_impl(std::string /*api_path*/,
                                            const struct timespec * /*crtime*/)
      -> api_error {
    return api_error::not_implemented;
  }

  [[nodiscard]] virtual auto setvolname_impl(const char * /*volname*/)
      -> api_error {
    return api_error::not_implemented;
  }

  [[nodiscard]] virtual auto statfs_x_impl(std::string /*api_path*/,
                                           struct statfs * /*stbuf*/)
      -> api_error {
    return api_error::not_implemented;
  }
#else  // __APPLE__
  [[nodiscard]] virtual auto statfs_impl(std::string /*api_path*/,
                                         struct statvfs * /*stbuf*/)
      -> api_error {
    return api_error::not_implemented;
  }
#endif // __APPLE__

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] virtual auto truncate_impl(std::string /*api_path*/,
                                           off_t /*size*/,
                                           struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }
#else
  [[nodiscard]] virtual auto truncate_impl(std::string /*api_path*/,
                                           off_t /*size*/) -> api_error {
    return api_error::not_implemented;
  }
#endif

  [[nodiscard]] virtual auto unlink_impl(std::string /*api_path*/)
      -> api_error {
    return api_error::not_implemented;
  }

#if FUSE_USE_VERSION >= 30
  [[nodiscard]] virtual auto utimens_impl(std::string /*api_path*/,
                                          const struct timespec /*tv*/[2],
                                          struct fuse_file_info * /*fi*/)
      -> api_error {
    return api_error::not_implemented;
  }
#else
  [[nodiscard]] virtual auto utimens_impl(std::string /*api_path*/,
                                          const struct timespec /*tv*/[2])
      -> api_error {
    return api_error::not_implemented;
  }
#endif

  [[nodiscard]] virtual auto
  write_impl(std::string /*api_path*/, const char * /*buffer*/,
             size_t /*write_size*/, off_t /*write_offset*/,
             struct fuse_file_info * /*fi*/, std::size_t & /*bytes_written*/)
      -> api_error {
    return api_error::not_implemented;
  }

protected:
  virtual void notify_fuse_args_parsed(const std::vector<std::string> &args);

  virtual void notify_fuse_main_exit(int & /*ret*/) {}

  [[nodiscard]] virtual auto parse_args(std::vector<std::string> &args) -> int;

  virtual void shutdown();

public:
  static void display_options(int argc, char *argv[]);

  static void display_version_information(int argc, char *argv[]);

  static auto unmount(const std::string &mount_location) -> int;

  [[nodiscard]] auto get_mount_location() const -> std::string {
    return mount_location_;
  }

  [[nodiscard]] auto mount(std::vector<std::string> args) -> int;
};
} // namespace repertory

#endif // _WIN32
#endif // INCLUDE_DRIVES_FUSE_FUSE_BASE_HPP_
