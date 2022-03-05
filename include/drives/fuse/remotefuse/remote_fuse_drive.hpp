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
#ifndef INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_FUSE_DRIVE_HPP_
#define INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_FUSE_DRIVE_HPP_
#ifndef _WIN32

#include "common.hpp"
#include "drives/fuse/remotefuse/i_remote_instance.hpp"
#include "events/event_system.hpp"

namespace repertory {
class app_config;
class console_consumer;
class logging_consumer;
class lock_data;
class server;

namespace remote_fuse {
class remote_fuse_drive final {
  E_CONSUMER();

public:
  remote_fuse_drive(app_config &config, lock_data &lock, remote_instance_factory factory);

  ~remote_fuse_drive() { E_CONSUMER_RELEASE(); }

private:
  app_config &config_;
  lock_data &lock_;
  remote_instance_factory factory_;
  std::string mount_location_;

private:
  static void shutdown(std::string mount_location);

private:
  class remote_fuse_impl final {
  public:
    static app_config *config_;
    static lock_data *lock_;
    static std::string *mount_location_;
    static remote_instance_factory *factory_;
    static std::unique_ptr<console_consumer> console_consumer_;
    static std::unique_ptr<logging_consumer> logging_consumer_;
    static std::unique_ptr<i_remote_instance> remote_instance_;
    static std::unique_ptr<server> server_;
    static std::optional<gid_t> forced_gid_;
    static std::optional<uid_t> forced_uid_;
    static std::optional<mode_t> forced_umask_;
    static bool console_enabled_;
    static bool was_mounted_;

  public:
    static void tear_down(const int &ret);

  private:
    static void populate_stat(const remote::stat &r, const bool &directory, struct stat &st);

  public:
    static int repertory_access(const char *path, int mask);

#ifdef __APPLE__
    static int repertory_chflags(const char *path, uint32_t flags);
#endif

    static int repertory_chmod(const char *path, mode_t mode);

    static int repertory_chown(const char *path, uid_t uid, gid_t gid);

    static int repertory_create(const char *path, mode_t mode, struct fuse_file_info *fi);

    static void repertory_destroy(void * /*ptr*/);

    /*static int repertory_fallocate(const char *path, int mode, off_t offset, off_t length,
                                   struct fuse_file_info *fi) ;*/

    static int repertory_fgetattr(const char *path, struct stat *st, struct fuse_file_info *fi);

#ifdef __APPLE__
    static int repertory_fsetattr_x(const char *path, struct setattr_x *attr,
                                    struct fuse_file_info *fi);
#endif

    static int repertory_fsync(const char *path, int datasync, struct fuse_file_info *fi);

    static int repertory_ftruncate(const char *path, off_t size, struct fuse_file_info *fi);

    static int repertory_getattr(const char *path, struct stat *st);

#ifdef __APPLE__
    static int repertory_getxtimes(const char *path, struct timespec *bkuptime,
                                   struct timespec *crtime);
#endif

    static void *repertory_init(struct fuse_conn_info *conn);

    static int repertory_mkdir(const char *path, mode_t mode);

    static int repertory_open(const char *path, struct fuse_file_info *fi);

    static int repertory_opendir(const char *path, struct fuse_file_info *fi);

    static int repertory_read(const char *path, char *buffer, size_t readSize, off_t readOffset,
                              struct fuse_file_info *fi);

    static int repertory_readdir(const char *path, void *buf, fuse_fill_dir_t fuseFillDir,
                                 off_t offset, struct fuse_file_info *fi);

    static int repertory_release(const char *path, struct fuse_file_info *fi);

    static int repertory_releasedir(const char *path, struct fuse_file_info *fi);

    static int repertory_rename(const char *from, const char *to);

    static int repertory_rmdir(const char *path);
/*
#ifdef HAS_SETXATTR
#ifdef __APPLE__
    static int repertory_getxattr(const char *path, const char *name, char *value, size_t size,
                                  uint32_t position) ;
#else
    static int repertory_getxattr(const char *path, const char *name, char *value, size_t size) ;

#endif
    static int repertory_listxattr(const char *path, char *buffer, size_t size) ;

    static int repertory_removexattr(const char *path, const char *name) ;
#ifdef __APPLE__
    static int repertory_setxattr(const char *path, const char *name, const char *value,
                                  size_t size, int flags, uint32_t position) ;
#else
    static int repertory_setxattr(const char *path, const char *name, const char *value,
                                  size_t size, int flags) ;
#endif
#endif
 */
#ifdef __APPLE__
    static int repertory_setattr_x(const char *path, struct setattr_x *attr);

    static int repertory_setbkuptime(const char *path, const struct timespec *bkuptime);

    static int repertory_setchgtime(const char *path, const struct timespec *chgtime);

    static int repertory_setcrtime(const char *path, const struct timespec *crtime);

    static int repertory_setvolname(const char *volname);

    static int repertory_statfs_x(const char *path, struct statfs *stbuf);
#else

    static int repertory_statfs(const char *path, struct statvfs *stbuf);

#endif

    static int repertory_truncate(const char *path, off_t size);

    static int repertory_unlink(const char *path);

    static int repertory_utimens(const char *path, const struct timespec tv[2]);

    static int repertory_write(const char *path, const char *buffer, size_t writeSize,
                               off_t writeOffset, struct fuse_file_info *fi);
  };

private:
  // clang-format off
  struct fuse_operations fuse_ops_ {
    .getattr = remote_fuse_impl::repertory_getattr,
    .readlink = nullptr,   // int (*readlink) (const char *, char *, size_t);
    .getdir = nullptr, // int (*getdir) (const char *, fuse_dirh_t, fuse_dirfil_t);
    .mknod = nullptr,  // int (*mknod) (const char *, mode_t, dev_t);
    .mkdir = remote_fuse_impl::repertory_mkdir,
    .unlink = remote_fuse_impl::repertory_unlink,
    .rmdir = remote_fuse_impl::repertory_rmdir,
    .symlink = nullptr, // int (*symlink) (const char *, const char *);
    .rename = remote_fuse_impl::repertory_rename,
    .link = nullptr, // int (*link) (const char *, const char *);
    .chmod = remote_fuse_impl::repertory_chmod,
    .chown = remote_fuse_impl::repertory_chown,
    .truncate = remote_fuse_impl::repertory_truncate,
    .utime = nullptr, // int (*utime) (const char *, struct utimbuf *);
    .open = remote_fuse_impl::repertory_open,
    .read = remote_fuse_impl::repertory_read,
    .write = remote_fuse_impl::repertory_write,
#ifdef __APPLE__
    .statfs = nullptr,
#else
    .statfs = remote_fuse_impl::repertory_statfs,
#endif
    .flush = nullptr, // int (*flush) (const char *, struct fuse_file_info *);
    .release = remote_fuse_impl::repertory_release,
    .fsync = remote_fuse_impl::repertory_fsync,
#if HAS_SETXATTR
    .setxattr = nullptr,     // remote_fuse_impl::repertory_setxattr,
    .getxattr = nullptr,    // remote_fuse_impl::repertory_getxattr,
    .listxattr = nullptr,   // remote_fuse_impl::repertory_listxattr,
    .removexattr = nullptr, // remote_fuse_impl::repertory_removexattr,
#else
    .setxattr = nullptr,
    .getxattr = nullptr,
    .listxattr = nullptr,
    .removexattr = nullptr,
#endif
    .opendir = remote_fuse_impl::repertory_opendir,
    .readdir = remote_fuse_impl::repertory_readdir,
    .releasedir = remote_fuse_impl::repertory_releasedir,
    .fsyncdir = nullptr, // int (*fsyncdir) (const char *, int, struct fuse_file_info *);
    .init = remote_fuse_impl::repertory_init,
    .destroy = remote_fuse_impl::repertory_destroy,
    .access = remote_fuse_impl::repertory_access,
    .create = remote_fuse_impl::repertory_create,
    .ftruncate = remote_fuse_impl::repertory_ftruncate,
    .fgetattr = remote_fuse_impl::repertory_fgetattr,
    .lock = nullptr, // int (*lock) (const char *, struct fuse_file_info *, int cmd, struct flock *);
    .utimens = remote_fuse_impl::repertory_utimens,
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
    .fallocate = nullptr  // remote_fuse_impl::repertory_fallocate,
  };
  // clang-format on

public:
  int mount(std::vector<std::string> drive_args);

  static void display_options(int argc, char *argv[]);

  static void display_version_information(int argc, char *argv[]);
};
} // namespace remote_fuse
} // namespace repertory

#endif // _WIN32
#endif // INCLUDE_DRIVES_FUSE_REMOTEFUSE_REMOTE_FUSE_DRIVE_HPP_
