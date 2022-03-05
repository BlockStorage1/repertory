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
#ifndef INCLUDE_DRIVES_FUSE_FUSE_BASE_HPP_
#define INCLUDE_DRIVES_FUSE_FUSE_BASE_HPP_
#ifndef _WIN32

#include "common.hpp"
#include "events/event_system.hpp"
#include "drives/fuse/i_fuse_drive.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
class app_config;
class i_provider;

class fuse_base : public i_fuse_drive {
  E_CONSUMER();

public:
  explicit fuse_base(app_config &config_);

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
  static fuse_base &instance();

private:
  // clang-format off
  struct fuse_operations fuse_ops_ {
    .getattr = fuse_base::getattr_,
    .readlink = nullptr,  // int (*readlink) (const char *, char *, size_t);
    .getdir = nullptr,    // int (*getdir) (const char *, fuse_dirh_t, fuse_dirfil_t);
    .mknod = nullptr,     // int (*mknod) (const char *, mode_t, dev_t);
    .mkdir = fuse_base::mkdir_,
    .unlink = fuse_base::unlink_,
    .rmdir = fuse_base::rmdir_,
    .symlink = nullptr, // int (*symlink) (const char *, const char *);
    .rename = fuse_base::rename_,
    .link = nullptr, // int (*link) (const char *, const char *);
    .chmod = fuse_base::chmod_,
    .chown = fuse_base::chown_,
    .truncate = fuse_base::truncate_,
    .utime = nullptr, // int (*utime) (const char *, struct utimbuf *);
    .open = fuse_base::open_,
    .read = fuse_base::read_,
    .write = fuse_base::write_,
#ifdef __APPLE__
    .statfs = nullptr,
#else // __APPLE__
    .statfs = fuse_base::statfs_,
#endif // __APPLE__
    .flush = nullptr, // int (*flush) (const char *, struct fuse_file_info *);
    .release = fuse_base::release_,
    .fsync = fuse_base::fsync_,
#if HAS_SETXATTR
    .setxattr = fuse_base::setxattr_,
    .getxattr = fuse_base::getxattr_,
    .listxattr = fuse_base::listxattr_,
    .removexattr = fuse_base::removexattr_,
#else // HAS_SETXATTR
    .setxattr = nullptr,
    .getxattr = nullptr,
    .listxattr = nullptr,
    .removexattr = nullptr,
#endif // HAS_SETXATTR
    .opendir = fuse_base::opendir_,
    .readdir = fuse_base::readdir_,
    .releasedir = fuse_base::releasedir_,
    .fsyncdir = nullptr, // int (*fsyncdir) (const char *, int, struct fuse_file_info *);
    .init = fuse_base::init_,
    .destroy = fuse_base::destroy_,
    .access = fuse_base::access_,
    .create = fuse_base::create_,
    .ftruncate = fuse_base::ftruncate_,
    .fgetattr = fuse_base::fgetattr_,
    .lock = nullptr, // int (*lock) (const char *, struct fuse_file_info *, int cmd, struct flock *);
    .utimens = fuse_base::utimens_,
    .bmap = nullptr, // int (*bmap) (const char *, size_t blocksize, uint64_t *idx);
    .flag_nullpath_ok = 0,
    .flag_nopath = 0,
    .flag_utime_omit_ok = 1,
    .flag_reserved = 0,
    .ioctl = nullptr,     // int (*ioctl) (const char *, int cmd, void *arg, struct fuse_file_info *, unsigned int flags, void *data);
    .poll = nullptr,      // int (*poll) (const char *, struct fuse_file_info *, struct fuse_pollhandle *ph, unsigned *reventsp);
    .write_buf = nullptr, // int (*write_buf) (const char *, struct fuse_bufvec *buf, off_t off, struct fuse_file_info *);
    .read_buf = nullptr,  // int (*read_buf) (const char *, struct fuse_bufvec **bufp, size_t size, off_t off, struct fuse_file_info *);
    .flock = nullptr,     // int (*flock) (const char *, struct fuse_file_info *, int op);
    .fallocate = fuse_base::fallocate_
  };
  // clang-format on

private:
  int execute_callback(const std::string &function_name, const char *from, const char *to,
                       const std::function<api_error(const std::string &, const std::string &)> &cb,
                       const bool &disable_logging = false);

  int execute_callback(const std::string &function_name, const char *path,
                       const std::function<api_error(const std::string &)> &cb,
                       const bool &disable_logging = false);

  static void execute_void_callback(const std::string &function_name,
                                    const std::function<void()> &cb);

  static void *execute_void_pointer_callback(const std::string &function_name,
                                             const std::function<void *()> &cb);

  void raise_fuse_event(std::string function_name, const std::string &api_file, const int &ret,
                        const bool &disable_logging);

private:
  static int access_(const char *path, int mask);

#ifdef __APPLE__
  static int chflags_(const char *path, uint32_t flags);
#endif // __APPLE__

  static int chmod_(const char *path, mode_t mode);

  static int chown_(const char *path, uid_t uid, gid_t gid);

  static int create_(const char *path, mode_t mode, struct fuse_file_info *fi);

  static void destroy_(void *ptr);

  static int fallocate_(const char *path, int mode, off_t offset, off_t length,
                        struct fuse_file_info *fi);

  static int fgetattr_(const char *path, struct stat *st, struct fuse_file_info *fi);

#ifdef __APPLE__
  static int fsetattr_x_(const char *path, struct setattr_x *attr, struct fuse_file_info *fi);
#endif // __APPLE__

  static int fsync_(const char *path, int datasync, struct fuse_file_info *fi);

  static int ftruncate_(const char *path, off_t size, struct fuse_file_info *fi);

  static int getattr_(const char *path, struct stat *st);

#ifdef __APPLE__
  static int getxtimes_(const char *path, struct timespec *bkuptime, struct timespec *crtime);
#endif // __APPLE__

  static void *init_(struct fuse_conn_info *conn);

  static int mkdir_(const char *path, mode_t mode);

  static int open_(const char *path, struct fuse_file_info *fi);

  static int opendir_(const char *path, struct fuse_file_info *fi);

  static int read_(const char *path, char *buffer, size_t read_size, off_t read_offset,
                   struct fuse_file_info *fi);

  static int readdir_(const char *path, void *buf, fuse_fill_dir_t fuse_fill_dir, off_t offset,
                      struct fuse_file_info *fi);

  static int release_(const char *path, struct fuse_file_info *fi);

  static int releasedir_(const char *path, struct fuse_file_info *fi);

  static int rename_(const char *from, const char *to);

  static int rmdir_(const char *path);

#ifdef HAS_SETXATTR
#ifdef __APPLE__
  static int getxattr_(const char *path, const char *name, char *value, size_t size,
                       uint32_t position);

#else  // __APPLE__
  static int getxattr_(const char *path, const char *name, char *value, size_t size);
#endif // __APPLE__

  static int listxattr_(const char *path, char *buffer, size_t size);

  static int removexattr_(const char *path, const char *name);

#ifdef __APPLE__
  static int setxattr_(const char *path, const char *name, const char *value, size_t size,
                       int flags, uint32_t position);

#else  // __APPLE__
  static int setxattr_(const char *path, const char *name, const char *value, size_t size,
                       int flags);
#endif // __APPLE__
#endif // HAS_SETXATTR

#ifdef __APPLE__
  static int setattr_x_(const char *path, struct setattr_x *attr);

  static int setbkuptime_(const char *path, const struct timespec *bkuptime);

  static int setchgtime_(const char *path, const struct timespec *chgtime);

  static int setcrtime_(const char *path, const struct timespec *crtime);

  static int setvolname_(const char *volname);

  static int statfs_x_(const char *path, struct statfs *stbuf);

#else  // __APPLE__
  static int statfs_(const char *path, struct statvfs *stbuf);
#endif // __APPLE__

  static int truncate_(const char *path, off_t size);

  static int unlink_(const char *path);

  static int utimens_(const char *path, const struct timespec tv[2]);

  static int write_(const char *path, const char *buffer, size_t write_size, off_t write_offset,
                    struct fuse_file_info *fi);

protected:
  api_error check_access(const std::string &api_path, int mask) const;

  api_error check_and_perform(const std::string &api_path, int parent_mask,
                              const std::function<api_error(api_meta_map &meta)> &action);

  uid_t get_effective_uid() const;

  gid_t get_effective_gid() const;

  static api_error check_open_flags(const int &flags, const int &mask, const api_error &fail_error);

  api_error check_owner(const api_meta_map &meta) const;

  static api_error check_readable(const int &flags, const api_error &fail_error);

  static api_error check_writeable(const int &flags, const api_error &fail_error);

#ifdef __APPLE__
  static __uint32_t get_flags_from_meta(const api_meta_map &meta);
#endif // __APPLE__

  static gid_t get_gid_from_meta(const api_meta_map &meta);

  static mode_t get_mode_from_meta(const api_meta_map &meta);

  static void get_timespec_from_meta(const api_meta_map &meta, const std::string &name,
                                     struct timespec &ts);

  static uid_t get_uid_from_meta(const api_meta_map &meta);

  static void populate_stat(const std::string &api_path, const std::uint64_t &size_or_count,
                            const api_meta_map &meta, const bool &directory, i_provider &provider,
                            struct stat *st);

  static void set_timespec_from_meta(const api_meta_map &meta, const std::string &name,
                                     struct timespec &ts);

protected:
  virtual api_error access_impl(std::string api_path, int mask) {
    return check_access(api_path, mask);
  }

#ifdef __APPLE__
  virtual api_error chflags_impl(std::string /*api_path*/, uint32_t /*flags*/) {
    return api_error::not_implemented;
  }
#endif // __APPLE__

  virtual api_error chmod_impl(std::string /*api_path*/, mode_t /*mode*/) {
    return api_error::not_implemented;
  }

  virtual api_error chown_impl(std::string /*api_path*/, uid_t /*uid*/, gid_t /*gid*/) {
    return api_error::not_implemented;
  }

  virtual api_error create_impl(std::string /*api_path*/, mode_t /*mode*/,
                                struct fuse_file_info * /*fi*/) {
    return api_error::not_implemented;
  }

  virtual void destroy_impl(void * /*ptr*/) { return; }

  virtual api_error fallocate_impl(std::string /*api_path*/, int /*mode*/, off_t /*offset*/,
                                   off_t /*length*/, struct fuse_file_info * /*fi*/) {
    return api_error::not_implemented;
  }

  virtual api_error fgetattr_impl(std::string /*api_path*/, struct stat * /*st*/,
                                  struct fuse_file_info * /*fi*/) {
    return api_error::not_implemented;
  }

  virtual api_error fsetattr_x_impl(std::string /*api_path*/, struct setattr_x * /*attr*/,
                                    struct fuse_file_info * /*fi*/) {
    return api_error::not_implemented;
  }

  virtual api_error fsync_impl(std::string /*api_path*/, int /*datasync*/,
                               struct fuse_file_info * /*fi*/) {
    return api_error::not_implemented;
  }

  virtual api_error ftruncate_impl(std::string /*api_path*/, off_t /*size*/,
                                   struct fuse_file_info * /*fi*/) {
    return api_error::not_implemented;
  }

  virtual api_error getattr_impl(std::string /*api_path*/, struct stat * /*st*/) {
    return api_error::not_implemented;
  }

#ifdef __APPLE__
  virtual api_error getxtimes_impl(std::string /*api_path*/, struct timespec * /*bkuptime*/,
                                   struct timespec * /*crtime*/) {
    return api_error::not_implemented;
  }
#endif // __APPLE__

  virtual void *init_impl(struct fuse_conn_info *conn);

  virtual api_error mkdir_impl(std::string /*api_path*/, mode_t /*mode*/) {
    return api_error::not_implemented;
  }

  virtual api_error open_impl(std::string /*api_path*/, struct fuse_file_info * /*fi*/) {
    return api_error::not_implemented;
  }

  virtual api_error opendir_impl(std::string /*api_path*/, struct fuse_file_info * /*fi*/) {
    return api_error::not_implemented;
  }

  virtual api_error read_impl(std::string /*api_path*/, char * /*buffer*/, size_t /*read_size*/,
                              off_t /*read_offset*/, struct fuse_file_info * /*fi*/,
                              std::size_t & /*bytes_read*/) {
    return api_error::not_implemented;
  }

  virtual api_error readdir_impl(std::string /*api_path*/, void * /*buf*/,
                                 fuse_fill_dir_t /*fuse_fill_dir*/, off_t /*offset*/,
                                 struct fuse_file_info * /*fi*/) {
    return api_error::not_implemented;
  }

  virtual api_error release_impl(std::string /*api_path*/, struct fuse_file_info * /*fi*/) {
    return api_error::not_implemented;
  }

  virtual api_error releasedir_impl(std::string /*api_path*/, struct fuse_file_info * /*fi*/) {
    return api_error::not_implemented;
  }

  virtual api_error rename_impl(std::string /*from_api_path*/, std::string /*to_api_path*/) {
    return api_error::not_implemented;
  }

  virtual api_error rmdir_impl(std::string /*api_path*/) { return api_error::not_implemented; }

#ifdef HAS_SETXATTR
#ifdef __APPLE__
  virtual api_error getxattr_impl(std::string /*api_path*/, const char * /*name*/, char * /*value*/,
                                  size_t /*size*/, uint32_t /*position*/,
                                  int & /*attribute_size*/) {
    return api_error::not_implemented;
  }
#else  // __APPLE__
  virtual api_error getxattr_impl(std::string /*api_path*/, const char * /*name*/, char * /*value*/,
                                  size_t /*size*/, int & /*attribute_size*/) {
    return api_error::not_implemented;
  }
#endif // __APPLE__

  virtual api_error listxattr_impl(std::string /*api_path*/, char * /*buffer*/, size_t /*size*/,
                                   int & /*required_size*/, bool & /*return_size*/) {
    return api_error::not_implemented;
  }

  virtual api_error removexattr_impl(std::string /*api_path*/, const char * /*name*/) {
    return api_error::not_implemented;
  }

#ifdef __APPLE__
  virtual api_error setxattr_impl(std::string /*api_path*/, const char * /*name*/,
                                  const char * /*value*/, size_t /*size*/, int /*flags*/,
                                  uint32_t /*position*/) {
    return api_error::not_implemented;
  }
#else  // __APPLE__
  virtual api_error setxattr_impl(std::string /*api_path*/, const char * /*name*/,
                                  const char * /*value*/, size_t /*size*/, int /*flags*/) {
    return api_error::not_implemented;
  }
#endif // __APPLE__
#endif // HAS_SETXATTR

#ifdef __APPLE__
  virtual api_error setattr_x_impl(std::string /*api_path*/, struct setattr_x * /*attr*/) {
    return api_error::not_implemented;
  }

  virtual api_error setbkuptime_impl(std::string /*api_path*/,
                                     const struct timespec * /*bkuptime*/) {
    return api_error::not_implemented;
  }

  virtual api_error setchgtime_impl(std::string /*api_path*/, const struct timespec * /*chgtime*/) {
    return api_error::not_implemented;
  }

  virtual api_error setcrtime_impl(std::string /*api_path*/, const struct timespec * /*crtime*/) {
    return api_error::not_implemented;
  }

  virtual api_error setvolname_impl(const char * /*volname*/) { return api_error::not_implemented; }

  virtual api_error statfs_x_impl(std::string /*api_path*/, struct statfs * /*stbuf*/) {
    return api_error::not_implemented;
  }
#else  // __APPLE__
  virtual api_error statfs_impl(std::string /*api_path*/, struct statvfs * /*stbuf*/) {
    return api_error::not_implemented;
  }
#endif // __APPLE__

  virtual api_error truncate_impl(std::string /*api_path*/, off_t /*size*/) {
    return api_error::not_implemented;
  }

  virtual api_error unlink_impl(std::string /*api_path*/) { return api_error::not_implemented; }

  virtual api_error utimens_impl(std::string /*api_path*/, const struct timespec /*tv*/[2]) {
    return api_error::not_implemented;
  }

  virtual api_error write_impl(std::string /*api_path*/, const char * /*buffer*/,
                               size_t /*write_size*/, off_t /*write_offset*/,
                               struct fuse_file_info * /*fi*/, std::size_t & /*bytes_written*/) {
    return api_error::not_implemented;
  }

protected:
  virtual void notify_fuse_args_parsed(const std::vector<std::string> & /*args*/) {}

  virtual void notify_fuse_main_exit(int & /*ret*/) {}

  virtual int parse_args(std::vector<std::string> &args);

#ifdef __APPLE__
  api_error parse_xattr_parameters(const char *name, const uint32_t &position,
                                   std::string &attribute_name, const std::string &api_path);
#else
  api_error parse_xattr_parameters(const char *name, std::string &attribute_name,
                                   const std::string &api_path);
#endif

#ifdef __APPLE__
  api_error parse_xattr_parameters(const char *name, const char *value, size_t size,
                                   const uint32_t &position, std::string &attribute_name,
                                   const std::string &api_path);
#else
  api_error parse_xattr_parameters(const char *name, const char *value, size_t size,
                                   std::string &attribute_name, const std::string &api_path);
#endif

  virtual int shutdown();

public:
  static void display_options(int argc, char *argv[]);

  static void display_version_information(int argc, char *argv[]);

  api_error check_parent_access(const std::string &api_path, int mask) const override;

  std::string get_mount_location() const { return mount_location_; }

  int mount(std::vector<std::string> args);
};
} // namespace repertory

#endif // _WIN32
#endif // INCLUDE_DRIVES_FUSE_FUSE_BASE_HPP_
