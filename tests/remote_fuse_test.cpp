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
#include "test_common.hpp"

#ifdef _WIN32
#define NOT_IMPLEMENTED STATUS_NOT_IMPLEMENTED
#include "drives/winfsp/i_winfsp_drive.hpp"
#include "drives/winfsp/remotewinfsp/remote_server.hpp"
#include "mocks/mock_winfsp_drive.hpp"
#else
#define NOT_IMPLEMENTED -ENOTSUP
#include "drives/fuse/i_fuse_drive.hpp"
#include "drives/fuse/remotefuse/remote_server.hpp"
#include "mocks/mock_fuse_drive.hpp"
#endif
#include "drives/fuse/remotefuse/remote_client.hpp"
#include "utils/utils.hpp"

using namespace repertory;
#ifdef _WIN32
using namespace repertory::remote_winfsp;
#else
using namespace repertory::remote_fuse;
#endif
namespace fuse_test {
static std::string mount_location_;

static void access_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_access.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
    EXPECT_EQ(0, client.fuse_access(api_path.c_str(), 0));
  }

  utils::file::delete_file(test_file);
}

static void create_and_release_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_create_release.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
  }

  utils::file::delete_file(test_file);
}

static void chflags_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_chflags.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
#ifdef __APPLE__
    EXPECT_EQ(0, client.fuse_chflags(api_path.c_str(), 0));

#else
    EXPECT_EQ(NOT_IMPLEMENTED, client.fuse_chflags(api_path.c_str(), 0));
#endif
  }

  utils::file::delete_file(test_file);
}

static void chmod_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_chmod.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
#ifdef _WIN32
    EXPECT_EQ(NOT_IMPLEMENTED, client.fuse_chmod(api_path.c_str(), 0));
#else
    EXPECT_EQ(0, client.fuse_chmod(api_path.c_str(), S_IRUSR | S_IWUSR));
#endif
  }

  utils::file::delete_file(test_file);
}

static void chown_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_chown.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
#ifdef _WIN32
    EXPECT_EQ(NOT_IMPLEMENTED, client.fuse_chown(api_path.c_str(), 0, 0));
#else
    if (getuid() == 0) {
      EXPECT_EQ(0, client.fuse_chown(api_path.c_str(), 0, 0));
    } else {
      EXPECT_EQ(-EPERM, client.fuse_chown(api_path.c_str(), 0, 0));
    }
#endif
  }

  utils::file::delete_file(test_file);
}

static void destroy_test(repertory::remote_fuse::remote_client &client) {
  EXPECT_EQ(0, client.fuse_destroy());
}

/*static void fallocate_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_fallocate.txt");
  const auto api_path = test_file.substr(mountLocation_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::Create | remote::open_flags::ReadWrite, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_fallocate(api_path.c_str(), 0, 0, 100, handle));
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));

    std::uint64_t file_size;
    EXPECT_TRUE(utils::file::get_file_size(test_file, file_size));
    EXPECT_EQ(100, file_size);
  }

  utils::file::delete_file(test_file);
}*/

static void fgetattr_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_fgetattr.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_ftruncate(api_path.c_str(), 100, handle));
    client.set_fuse_uid_gid(10, 11);

    bool directory;
    remote::stat st{};
    EXPECT_EQ(0, client.fuse_fgetattr(api_path.c_str(), st, directory, handle));
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));

    EXPECT_FALSE(directory);
#ifdef _WIN32
    struct _stat64 st1 {};
    _stat64(&test_file[0], &st1);
#else
    struct stat st1 {};
    stat(&test_file[0], &st1);
#endif

    EXPECT_EQ(11, st.st_gid);
    EXPECT_EQ(10, st.st_uid);
    EXPECT_EQ(st1.st_size, st.st_size);
    EXPECT_EQ(st1.st_nlink, st.st_nlink);
    EXPECT_EQ(st1.st_mode, st.st_mode);
    EXPECT_LE(static_cast<remote::file_time>(st1.st_atime), st.st_atimespec / NANOS_PER_SECOND);
    EXPECT_EQ(static_cast<remote::file_time>(st1.st_mtime), st.st_mtimespec / NANOS_PER_SECOND);
    EXPECT_EQ(static_cast<remote::file_time>(st1.st_ctime), st.st_ctimespec / NANOS_PER_SECOND);
    EXPECT_EQ(static_cast<remote::file_time>(st1.st_ctime), st.st_birthtimespec / NANOS_PER_SECOND);
  }

  utils::file::delete_file(test_file);
}

static void fsetattr_x_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_fsetattr_x.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    remote::setattr_x attr{};
#ifdef __APPLE__
    EXPECT_EQ(0, client.fuse_fsetattr_x(api_path.c_str(), attr, handle));
#else
    EXPECT_EQ(NOT_IMPLEMENTED, client.fuse_fsetattr_x(api_path.c_str(), attr, handle));
#endif
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
  }

  utils::file::delete_file(test_file);
}

static void fsync_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_fsync.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_fsync(api_path.c_str(), 0, handle));
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
  }

  utils::file::delete_file(test_file);
}

static void ftruncate_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_ftruncate.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_ftruncate(api_path.c_str(), 100, handle));
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));

    std::uint64_t file_size;
    EXPECT_TRUE(utils::file::get_file_size(test_file, file_size));
    EXPECT_EQ(100, file_size);
  }

  utils::file::delete_file(test_file);
}

static void getattr_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_getattr.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_ftruncate(api_path.c_str(), 100, handle));
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));

    client.set_fuse_uid_gid(10, 11);

    bool directory;
    remote::stat st{};
    EXPECT_EQ(0, client.fuse_getattr(api_path.c_str(), st, directory));
    EXPECT_FALSE(directory);
#ifdef _WIN32
    struct _stat64 st1 {};
    _stat64(&test_file[0], &st1);
#else
    struct stat st1 {};
    stat(&test_file[0], &st1);
#endif
    EXPECT_EQ(11, st.st_gid);
    EXPECT_EQ(10, st.st_uid);
    EXPECT_EQ(st1.st_size, st.st_size);
    EXPECT_EQ(st1.st_nlink, st.st_nlink);
    EXPECT_EQ(st1.st_mode, st.st_mode);
    EXPECT_LE(static_cast<remote::file_time>(st1.st_atime), st.st_atimespec / NANOS_PER_SECOND);
    EXPECT_EQ(static_cast<remote::file_time>(st1.st_mtime), st.st_mtimespec / NANOS_PER_SECOND);
    EXPECT_EQ(static_cast<remote::file_time>(st1.st_ctime), st.st_ctimespec / NANOS_PER_SECOND);
    EXPECT_EQ(static_cast<remote::file_time>(st1.st_ctime), st.st_birthtimespec / NANOS_PER_SECOND);
  }

  utils::file::delete_file(test_file);
}

/*static void getxattr_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_getxattr.txt");
  const auto api_path = test_file.substr(mountLocation_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::Create | remote::open_flags::ReadWrite, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
#if _WIN32 || !HAS_SETXATTR || __APPLE__
    EXPECT_EQ(NOT_IMPLEMENTED, client.fuse_getxattr(api_path.c_str(), "test", nullptr, 0));
#else
    EXPECT_EQ(-EACCES, client.fuse_getxattr(api_path.c_str(), "test", nullptr, 0));
#endif
  }

  utils::file::delete_file(test_file);
}

static void getxattr_osx_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_getxattr_osx.txt");
  const auto api_path = test_file.substr(mountLocation_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::Create | remote::open_flags::ReadWrite, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
    EXPECT_EQ(NOT_IMPLEMENTED,
              client.fuse_getxattrOSX(api_path.c_str(), "test", nullptr, 0, 0));
  }

  utils::file::delete_file(test_file);
}*/

static void getxtimes_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_getxtimes.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    remote::file_time bkuptime = 0;
    remote::file_time crtime = 0;
#ifdef __APPLE__
    EXPECT_EQ(0, client.fuse_getxtimes(api_path.c_str(), bkuptime, crtime));
#else
    EXPECT_EQ(NOT_IMPLEMENTED, client.fuse_getxtimes(api_path.c_str(), bkuptime, crtime));
#endif
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
  }

  utils::file::delete_file(test_file);
}

static void init_test(repertory::remote_fuse::remote_client &client) {
  EXPECT_EQ(0, client.fuse_init());
}

/*static void listxattr_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_listxattr.txt");
  const auto api_path = test_file.substr(mountLocation_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::Create | remote::open_flags::ReadWrite, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
#if _WIN32 || !HAS_SETXATTR
    EXPECT_EQ(NOT_IMPLEMENTED, client.fuse_listxattr(api_path.c_str(), nullptr, 0));
#else
    EXPECT_EQ(-EIO, client.fuse_listxattr(api_path.c_str(), nullptr, 0));
#endif
  }

  utils::file::delete_file(test_file);
}*/

static void mkdir_test(repertory::remote_fuse::remote_client &client) {
  const auto test_directory = utils::path::absolute("./fuse_remote/fuse_remote_mkdir");
  const auto api_path = test_directory.substr(mount_location_.size());
  utils::file::delete_directory(test_directory);

#ifdef _WIN32
  EXPECT_EQ(0, client.fuse_mkdir(api_path.c_str(), 0));
#else
  EXPECT_EQ(0, client.fuse_mkdir(api_path.c_str(), S_IRWXU));
#endif
  EXPECT_TRUE(utils::file::is_directory(test_directory));

  utils::file::delete_directory(test_directory);
}

static void open_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_open.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
#ifdef _WIN32
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
#else
  const auto ret =
      client.fuse_create(api_path.c_str(), S_IRWXU,
                         remote::open_flags::create | remote::open_flags::read_write, handle);
#endif
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    remote::file_handle handle2;
    EXPECT_EQ(0, client.fuse_open(api_path.c_str(), remote::open_flags::read_write, handle2));
    EXPECT_NE(handle, handle2);
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle2));
  }

  utils::file::delete_file(test_file);
}

static void opendir_and_releasedir_test(repertory::remote_fuse::remote_client &client) {
  const auto test_directory = utils::path::absolute("./fuse_remote/fuse_remote_opendir");
  const auto api_path = test_directory.substr(mount_location_.size());
  utils::file::delete_directory(test_directory);

#ifdef _WIN32
  EXPECT_EQ(0, client.fuse_mkdir(api_path.c_str(), 0));
#else
  EXPECT_EQ(0, client.fuse_mkdir(api_path.c_str(), S_IRWXU));
#endif
  EXPECT_TRUE(utils::file::is_directory(test_directory));

  remote::file_handle handle = 0;
  EXPECT_EQ(0, client.fuse_opendir(api_path.c_str(), handle));
  EXPECT_EQ(0, client.fuse_releasedir(api_path.c_str(), handle));

  utils::file::delete_directory(test_directory);
}

static void read_and_write_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_read_write.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(10, client.fuse_write(api_path.c_str(), "1234567890", 10, 0, handle));
    std::vector<char> buffer(10);
    EXPECT_EQ(10, client.fuse_read(api_path.c_str(), &buffer[0], 10, 0, handle));
    EXPECT_EQ(0, memcmp("1234567890", &buffer[0], 10));
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
  }

  utils::file::delete_file(test_file);
}

static void read_and_write_base64_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_read_write_base64.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    const auto data = macaron::Base64::Encode("1234567890");
    EXPECT_EQ(10, client.fuse_write_base64(api_path.c_str(), &data[0], data.size(), 0, handle));
    std::vector<char> buffer(10);
    EXPECT_EQ(10, client.fuse_read(api_path.c_str(), &buffer[0], 10, 0, handle));
    EXPECT_EQ(0, memcmp("1234567890", &buffer[0], 10));
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
  }

  utils::file::delete_file(test_file);
}

static void readdir_test(repertory::remote_fuse::remote_client &client) {
  const auto test_directory = utils::path::absolute("./fuse_remote/fuse_remote_readdir");
  const auto api_path = test_directory.substr(mount_location_.size());
  utils::file::delete_directory(test_directory);

#ifdef _WIN32
  EXPECT_EQ(0, client.fuse_mkdir(api_path.c_str(), 0));
#else
  EXPECT_EQ(0, client.fuse_mkdir(api_path.c_str(), S_IRWXU));
#endif
  EXPECT_TRUE(utils::file::is_directory(test_directory));

  remote::file_handle handle = 0;
  EXPECT_EQ(0, client.fuse_opendir(api_path.c_str(), handle));

  std::string item_path;
  EXPECT_EQ(0, client.fuse_readdir(api_path.c_str(), 0, handle, item_path));
  EXPECT_STREQ(".", item_path.c_str());

  EXPECT_EQ(0, client.fuse_readdir(api_path.c_str(), 1, handle, item_path));
  EXPECT_STREQ("..", item_path.c_str());

  EXPECT_EQ(0, client.fuse_releasedir(api_path.c_str(), handle));

  utils::file::delete_directory(test_directory);
}

/*static void removexattr_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_removexattr.txt");
  const auto api_path = test_file.substr(mountLocation_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::Create | remote::open_flags::ReadWrite, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
#if _WIN32 || !HAS_SETXATTR
    EXPECT_EQ(NOT_IMPLEMENTED, client.fuse_removexattr(api_path.c_str(), "test"));
#else
    EXPECT_EQ(-EACCES, client.fuse_removexattr(api_path.c_str(), "test"));
#endif
  }

  utils::file::delete_file(test_file);
}*/

static void rename_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_rename.txt");
  const auto renamed_test_file = utils::path::absolute("./fuse_remote/fuse_remote_rename2.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  const auto renamed_api_path = renamed_test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);
  utils::file::delete_file(renamed_test_file);

  remote::file_handle handle;
#ifdef _WIN32
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
#else
  const auto ret =
      client.fuse_create(api_path.c_str(), S_IRWXU,
                         remote::open_flags::create | remote::open_flags::read_write, handle);
#endif

  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
    EXPECT_EQ(0, client.fuse_rename(api_path.c_str(), renamed_api_path.c_str()));
    EXPECT_FALSE(utils::file::is_file(test_file));
    EXPECT_TRUE(utils::file::is_file(renamed_test_file));
  }

  utils::file::delete_file(test_file);
  utils::file::delete_file(renamed_test_file);
}

static void rmdir_test(repertory::remote_fuse::remote_client &client) {
  const auto test_directory = utils::path::absolute("./fuse_remote/fuse_remote_rmdir");
  const auto api_path = test_directory.substr(mount_location_.size());
  utils::file::delete_directory(test_directory);

#ifdef _WIN32
  EXPECT_EQ(0, client.fuse_mkdir(api_path.c_str(), 0));
#else
  EXPECT_EQ(0, client.fuse_mkdir(api_path.c_str(), S_IRWXU));
#endif
  EXPECT_TRUE(utils::file::is_directory(test_directory));

  EXPECT_EQ(0, client.fuse_rmdir(api_path.c_str()));
  EXPECT_FALSE(utils::file::is_directory(test_directory));

  utils::file::delete_directory(test_directory);
}

static void setattr_x_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_setattr_x.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));

    remote::setattr_x attr{};
#ifdef __APPLE__
    EXPECT_EQ(0, client.fuse_setattr_x(api_path.c_str(), attr));
#else
    EXPECT_EQ(NOT_IMPLEMENTED, client.fuse_setattr_x(api_path.c_str(), attr));
#endif
  }

  utils::file::delete_file(test_file);
}

static void setbkuptime_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_setbkuptime.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));

    remote::file_time ts = 0;
#ifdef __APPLE__
    EXPECT_EQ(0, client.fuse_setbkuptime(api_path.c_str(), ts));
#else
    EXPECT_EQ(NOT_IMPLEMENTED, client.fuse_setbkuptime(api_path.c_str(), ts));
#endif
  }

  utils::file::delete_file(test_file);
}

static void setchgtime_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_setchgtime.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));

    remote::file_time ts = 0;
#ifdef __APPLE__
    EXPECT_EQ(0, client.fuse_setchgtime(api_path.c_str(), ts));
#else
    EXPECT_EQ(NOT_IMPLEMENTED, client.fuse_setchgtime(api_path.c_str(), ts));
#endif
  }

  utils::file::delete_file(test_file);
}

static void setcrtime_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_setcrtime.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));

    remote::file_time ts = 0;
#ifdef __APPLE__
    EXPECT_EQ(0, client.fuse_setcrtime(api_path.c_str(), ts));
#else
    EXPECT_EQ(NOT_IMPLEMENTED, client.fuse_setcrtime(api_path.c_str(), ts));
#endif
  }

  utils::file::delete_file(test_file);
}

static void setvolname_test(repertory::remote_fuse::remote_client &client) {
  EXPECT_EQ(0, client.fuse_setvolname("moose"));
}

/*static void setxattr_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_setxattr.txt");
  const auto api_path = test_file.substr(mountLocation_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::Create | remote::open_flags::ReadWrite, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
#if _WIN32 || !HAS_SETXATTR
    EXPECT_EQ(NOT_IMPLEMENTED,
              client.fuse_setxattr(api_path.c_str(), "test", "moose", 5, 0));
#else
    EXPECT_EQ(-EACCES, client.fuse_setxattr(api_path.c_str(), "test", "moose", 5, 0));
#endif
  }

  utils::file::delete_file(test_file);
}

static void setxattr_osx_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_setxattr_osx.txt");
  const auto api_path = test_file.substr(mountLocation_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::Create | remote::open_flags::ReadWrite, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {

    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
    EXPECT_EQ(NOT_IMPLEMENTED,
              client.fuse_setxattr_osx(api_path.c_str(), "test", "moose", 5, 0, 0));
  }

  utils::file::delete_file(test_file);
}*/

#ifdef _WIN32
static void test_statfs(repertory::remote_fuse::remote_client &client,
                        const i_winfsp_drive &drive) {
#else
static void test_statfs(repertory::remote_fuse::remote_client &client, const i_fuse_drive &drive) {
#endif
  const auto test_file = utils::path::absolute("./fuse_remote/");
  const auto api_path = test_file.substr(mount_location_.size());

  remote::statfs st{};
  EXPECT_EQ(0, client.fuse_statfs(api_path.c_str(), 4096, st));

  const auto total_bytes = drive.get_total_drive_space();
  const auto total_used = drive.get_used_drive_space();
  const auto used_blocks = utils::divide_with_ceiling(total_used, static_cast<std::uint64_t>(4096));
  EXPECT_EQ(utils::divide_with_ceiling(total_bytes, static_cast<std::uint64_t>(4096)), st.f_blocks);
  EXPECT_EQ(st.f_blocks ? (st.f_blocks - used_blocks) : 0, st.f_bavail);
  EXPECT_EQ(st.f_bavail, st.f_bfree);
  EXPECT_EQ(4294967295, st.f_files);
  EXPECT_EQ(4294967295 - drive.get_total_item_count(), st.f_favail);
  EXPECT_EQ(st.f_favail, st.f_ffree);
}

#ifdef _WIN32
static void statfs_x_test(repertory::remote_fuse::remote_client &client,
                          const i_winfsp_drive &drive) {
#else

static void statfs_x_test(repertory::remote_fuse::remote_client &client,
                          const i_fuse_drive &drive) {
#endif
  const auto test_file = utils::path::absolute("./fuse_remote/");
  const auto api_path = test_file.substr(mount_location_.size());

  remote::statfs_x st{};
  EXPECT_EQ(0, client.fuse_statfs_x(api_path.c_str(), 4096, st));
  EXPECT_STREQ(&st.f_mntfromname[0], utils::create_volume_label(provider_type::remote).c_str());

  const auto total_bytes = drive.get_total_drive_space();
  const auto total_used = drive.get_used_drive_space();
  const auto used_blocks = utils::divide_with_ceiling(total_used, static_cast<std::uint64_t>(4096));
  EXPECT_EQ(utils::divide_with_ceiling(total_bytes, static_cast<std::uint64_t>(4096)), st.f_blocks);
  EXPECT_EQ(st.f_blocks ? (st.f_blocks - used_blocks) : 0, st.f_bavail);
  EXPECT_EQ(st.f_bavail, st.f_bfree);
  EXPECT_EQ(4294967295, st.f_files);
  EXPECT_EQ(4294967295 - drive.get_total_item_count(), st.f_favail);
  EXPECT_EQ(st.f_favail, st.f_ffree);
}

static void truncate_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_truncate.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
#ifdef _WIN32
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
#else
  const auto ret =
      client.fuse_create(api_path.c_str(), S_IRWXU,
                         remote::open_flags::create | remote::open_flags::read_write, handle);
#endif
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));

    EXPECT_EQ(0, client.fuse_truncate(api_path.c_str(), 100));

    std::uint64_t file_size;
    EXPECT_TRUE(utils::file::get_file_size(test_file, file_size));
    EXPECT_EQ(100, file_size);
  }

  utils::file::delete_file(test_file);
}

static void unlink_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_unlink.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));
    EXPECT_EQ(0, client.fuse_unlink(api_path.c_str()));
    EXPECT_FALSE(utils::file::is_file(test_file));
  }

  utils::file::delete_file(test_file);
}

static void utimens_test(repertory::remote_fuse::remote_client &client) {
  const auto test_file = utils::path::absolute("./fuse_remote/fuse_remote_utimens.txt");
  const auto api_path = test_file.substr(mount_location_.size());
  utils::file::delete_file(test_file);

  remote::file_handle handle;
  const auto ret = client.fuse_create(
      api_path.c_str(), 0, remote::open_flags::create | remote::open_flags::read_write, handle);
  EXPECT_EQ(0, ret);
  if (ret == 0) {
    EXPECT_EQ(0, client.fuse_release(api_path.c_str(), handle));

    remote::file_time tv[2] = {0};
    EXPECT_EQ(0, client.fuse_utimens(api_path.c_str(), tv, 0, 0));
  }

  utils::file::delete_file(test_file);
}

TEST(remote_fuse, all_tests) {
  std::uint16_t port = 0u;
  const auto found_port = utils::get_next_available_port(20000u, port);
  EXPECT_TRUE(found_port);
  if (found_port) {
    console_consumer c;

    app_config config(provider_type::remote, "./fuse_remote");
    config.set_remote_host_name_or_ip("localhost");
    config.set_remote_port(port);
    config.set_remote_token("testtoken");
    config.set_enable_drive_events(true);
    config.set_event_level(event_level::verbose);

    event_system::instance().start();
#ifdef _WIN32
    mount_location_ = std::string(__FILE__).substr(0, 2);
    mock_winfsp_drive drive(mount_location_);
    remote_server server(config, drive, mount_location_);
#else
    mount_location_ = utils::path::absolute(".");
    mock_fuse_drive drive(mount_location_);
    remote_server server(config, drive, mount_location_);
#endif
    std::thread([&]() {
      repertory::remote_fuse::remote_client client(config);

      create_and_release_test(client);
      access_test(client);
      chflags_test(client);
      chmod_test(client);
      chown_test(client);
      destroy_test(client);
      fgetattr_test(client);
      fsetattr_x_test(client);
      fsync_test(client);
      ftruncate_test(client);
      getattr_test(client);
      getxtimes_test(client);
      init_test(client);
      mkdir_test(client);
      open_test(client);
      opendir_and_releasedir_test(client);
      read_and_write_base64_test(client);
      read_and_write_test(client);
      readdir_test(client);
      rename_test(client);
      rmdir_test(client);
      setattr_x_test(client);
      setbkuptime_test(client);
      setchgtime_test(client);
      setcrtime_test(client);
      setvolname_test(client);
      statfs_x_test(client, drive);
      test_statfs(client, drive);
      truncate_test(client);
      unlink_test(client);
      utimens_test(client);
      // fallocate_test(client);
      // getxattr_osx_test(client);
      // getxattr_test(client);
      // listxattr_test(client);
      // removexattr_test(client);
      // setxattr_osx_test(client);
      // setxattr_test(client);
    }).join();
  }

  event_system::instance().stop();
  utils::file::delete_directory_recursively("./fuse_remote");
}
} // namespace fuse_test
