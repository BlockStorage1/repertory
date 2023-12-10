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
#if !_WIN32

#include "test_common.hpp"

#include "app_config.hpp"
#include "comm/curl/curl_comm.hpp"
#include "drives/fuse/fuse_drive.hpp"
#include "platform/platform.hpp"
#include "providers/s3/s3_provider.hpp"
#include "providers/sia/sia_provider.hpp"
#include "types/repertory.hpp"
#include "utils/event_capture.hpp"
#include "utils/file_utils.hpp"
#include "utils/utils.hpp"

#ifndef ACCESSPERMS
#define ACCESSPERMS (S_IRWXU | S_IRWXG | S_IRWXO) /* 0777 */
#endif

static constexpr const auto SLEEP_SECONDS = 1.5s;

namespace repertory {
static void execute_mount(const std::string &data_directory,
                          const std::vector<std::string> &drive_args,
                          std::thread &mount_thread) {
  auto cmd = "./repertory -dd \"" + data_directory + "\"" + " " +
             utils::string::join(drive_args, ' ');
  std::cout << cmd << std::endl;
  EXPECT_EQ(0, system(cmd.c_str()));
  mount_thread.join();
}

static void execute_unmount(const std::string &mount_location) {
  // filesystem_item fsi{};
  // EXPECT_EQ(api_error::success, provider.get_filesystem_item("/", true,
  // fsi)); EXPECT_STREQ("/", fsi.api_path.c_str());
  // EXPECT_TRUE(fsi.api_parent.empty());
  // EXPECT_TRUE(fsi.directory);
  // EXPECT_EQ(std::uint64_t(0u), fsi.size);
  // EXPECT_TRUE(fsi.source_path.empty());
  //
  // api_meta_map meta{};
  // EXPECT_EQ(api_error::success, provider.get_item_meta("/", meta));
  // for (const auto &kv : meta) {
  //   std::cout << kv.first << '=' << kv.second << std::endl;
  // }

  auto unmounted = false;
  for (int i = 0; not unmounted && (i < 50); i++) {
    unmounted = (fuse_base::unmount(mount_location) == 0);
    if (not unmounted) {
      std::this_thread::sleep_for(100ms);
    }
  }

  EXPECT_TRUE(unmounted);
}

static auto create_file_and_test(const std::string &mount_location,
                                 std::string name) -> std::string {
  std::cout << __FUNCTION__ << std::endl;
  auto file_path =
      utils::path::absolute(utils::path::combine(mount_location, {name}));

  auto fd =
      open(file_path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
  EXPECT_LE(1, fd);
  EXPECT_TRUE(utils::file::is_file(file_path));
  EXPECT_FALSE(utils::file::is_directory(file_path));

  std::uint64_t file_size{};
  EXPECT_TRUE(utils::file::get_file_size(file_path, file_size));
  EXPECT_EQ(0u, file_size);

  EXPECT_EQ(0, close(fd));
  std::this_thread::sleep_for(SLEEP_SECONDS);

  return file_path;
}

static void rmdir_and_test(const std::string &directory_path) {
  std::cout << __FUNCTION__ << std::endl;
  int ret = 0;
  for (auto i = 0; ((ret = rmdir(directory_path.c_str())) != 0) && (i < 20);
       i++) {
    std::this_thread::sleep_for(100ms);
  }

  EXPECT_EQ(0, ret);
  std::this_thread::sleep_for(SLEEP_SECONDS);

  EXPECT_FALSE(utils::file::is_directory(directory_path));
  EXPECT_FALSE(utils::file::is_file(directory_path));
}

static void unlink_file_and_test(const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  int ret = 0;
  for (auto i = 0; ((ret = unlink(file_path.c_str())) != 0) && (i < 20); i++) {
    std::this_thread::sleep_for(100ms);
  }

  EXPECT_EQ(0, ret);

  std::this_thread::sleep_for(SLEEP_SECONDS);
  EXPECT_FALSE(utils::file::is_directory(file_path));
  EXPECT_FALSE(utils::file::is_file(file_path));
}

static void test_chmod(const std::string & /* api_path */,
                       const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  EXPECT_EQ(0, chmod(file_path.c_str(), S_IRUSR | S_IWUSR));
  std::this_thread::sleep_for(SLEEP_SECONDS);

  struct stat64 unix_st {};
  stat64(file_path.c_str(), &unix_st);
  EXPECT_EQ(static_cast<std::uint32_t>(S_IRUSR | S_IWUSR),
            ACCESSPERMS & unix_st.st_mode);
}

static void test_chown(const std::string & /* api_path */,
                       const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  EXPECT_EQ(0, chown(file_path.c_str(), static_cast<uid_t>(-1), 0));
  std::this_thread::sleep_for(SLEEP_SECONDS);

  struct stat64 unix_st {};
  stat64(file_path.c_str(), &unix_st);
  EXPECT_EQ(0U, unix_st.st_gid);

  EXPECT_EQ(0, chown(file_path.c_str(), 0, static_cast<gid_t>(-1)));
  std::this_thread::sleep_for(SLEEP_SECONDS);

  stat64(file_path.c_str(), &unix_st);
  EXPECT_EQ(0U, unix_st.st_gid);
}

static void test_mkdir(const std::string & /* api_path */,
                       const std::string &directory_path) {
  std::cout << __FUNCTION__ << std::endl;
  EXPECT_EQ(0, mkdir(directory_path.c_str(),
                     S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP));

  EXPECT_TRUE(utils::file::is_directory(directory_path));
  EXPECT_FALSE(utils::file::is_file(directory_path));

  struct stat64 unix_st {};
  stat64(directory_path.c_str(), &unix_st);

  EXPECT_EQ(getuid(), unix_st.st_uid);
  EXPECT_EQ(getgid(), unix_st.st_gid);

  EXPECT_EQ(static_cast<std::uint32_t>(S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |
                                       S_IXGRP),
            ACCESSPERMS & unix_st.st_mode);
}

static void test_write_and_read(const std::string & /* api_path */,
                                const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  auto fd =
      open(file_path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
  EXPECT_LE(1, fd);

  std::string data = "TestData";
  EXPECT_EQ(data.size(),
            static_cast<std::size_t>(write(fd, data.data(), data.size())));
  EXPECT_EQ(0, lseek(fd, 0, SEEK_SET));
  fsync(fd);

  data_buffer read_data;
  read_data.resize(data.size());
  EXPECT_EQ(data.size(), static_cast<std::size_t>(
                             read(fd, read_data.data(), read_data.size())));

  EXPECT_EQ(0, memcmp(data.data(), read_data.data(), data.size()));

  EXPECT_EQ(0, close(fd));

  std::this_thread::sleep_for(SLEEP_SECONDS);

  std::uint64_t file_size{};
  EXPECT_TRUE(utils::file::get_file_size(file_path, file_size));
  EXPECT_EQ(data.size(), file_size);

  // filesystem_item fsi{};
  // EXPECT_EQ(api_error::success,
  //           provider.get_filesystem_item(api_path, false, fsi));
  // EXPECT_TRUE(utils::file::get_file_size(fsi.source_path, file_size));
  // EXPECT_EQ(data.size(), file_size);
}

static void test_rename_file(const std::string &from_file_path,
                             const std::string &to_file_path,
                             bool is_rename_supported) {
  std::cout << __FUNCTION__ << std::endl;
  auto fd = open(from_file_path.c_str(), O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
  EXPECT_LE(1, fd);
  close(fd);

  std::this_thread::sleep_for(SLEEP_SECONDS);

  if (is_rename_supported) {
    EXPECT_EQ(0, rename(from_file_path.c_str(), to_file_path.c_str()));
    EXPECT_FALSE(utils::file::is_file(from_file_path));
    EXPECT_TRUE(utils::file::is_file(to_file_path));
  } else {
    EXPECT_EQ(-1, rename(from_file_path.c_str(), to_file_path.c_str()));
    EXPECT_TRUE(utils::file::is_file(from_file_path));
    EXPECT_FALSE(utils::file::is_file(to_file_path));
  }
}

static void test_rename_directory(const std::string &from_dir_path,
                                  const std::string &to_dir_path,
                                  bool is_rename_supported) {
  std::cout << __FUNCTION__ << std::endl;
  EXPECT_EQ(0, mkdir(from_dir_path.c_str(),
                     S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP));
  std::this_thread::sleep_for(SLEEP_SECONDS);

  EXPECT_TRUE(utils::file::is_directory(from_dir_path));
  if (is_rename_supported) {
    EXPECT_EQ(0, rename(from_dir_path.c_str(), to_dir_path.c_str()));
    EXPECT_FALSE(utils::file::is_directory(from_dir_path));
    EXPECT_TRUE(utils::file::is_directory(to_dir_path));
  } else {
    EXPECT_EQ(-1, rename(from_dir_path.c_str(), to_dir_path.c_str()));
    EXPECT_TRUE(utils::file::is_directory(from_dir_path));
    EXPECT_FALSE(utils::file::is_directory(to_dir_path));
  }
}

static void test_truncate(const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  EXPECT_EQ(0, truncate(file_path.c_str(), 10u));

  std::uint64_t file_size{};
  EXPECT_TRUE(utils::file::get_file_size(file_path, file_size));

  EXPECT_EQ(std::uint64_t(10u), file_size);
}

static void test_ftruncate(const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  auto fd = open(file_path.c_str(), O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
  EXPECT_LE(1, fd);

  EXPECT_EQ(0, ftruncate(fd, 10u));

  std::uint64_t file_size{};
  EXPECT_TRUE(utils::file::get_file_size(file_path, file_size));

  EXPECT_EQ(std::uint64_t(10u), file_size);

  close(fd);
}

#ifndef __APPLE__
static void test_fallocate(const std::string &api_path,
                           const std::string &file_path, i_provider &provider) {
  std::cout << __FUNCTION__ << std::endl;
  auto file =
      open(file_path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
  EXPECT_LE(1, file);
  EXPECT_EQ(0, fallocate(file, 0, 0, 16));

  std::uint64_t file_size{};
  EXPECT_TRUE(utils::file::get_file_size(file_path, file_size));
  EXPECT_EQ(16U, file_size);

  EXPECT_EQ(0, close(file));

  file_size = 0U;
  EXPECT_TRUE(utils::file::get_file_size(file_path, file_size));
  EXPECT_EQ(16U, file_size);

  filesystem_item fsi{};
  EXPECT_EQ(api_error::success,
            provider.get_filesystem_item(api_path, false, fsi));
  EXPECT_EQ(16U, fsi.size);
}
#endif

static void test_file_getattr(const std::string & /* api_path */,
                              const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  auto fd =
      open(file_path.c_str(), O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP);
  EXPECT_LE(1, fd);

  EXPECT_EQ(0, close(fd));

  struct stat64 unix_st {};
  EXPECT_EQ(0, stat64(file_path.c_str(), &unix_st));
  EXPECT_EQ(static_cast<std::uint32_t>(S_IRUSR | S_IWUSR | S_IRGRP),
            ACCESSPERMS & unix_st.st_mode);
  EXPECT_FALSE(S_ISDIR(unix_st.st_mode));
  EXPECT_TRUE(S_ISREG(unix_st.st_mode));
}

static void test_directory_getattr(const std::string & /* api_path */,
                                   const std::string &directory_path) {
  std::cout << __FUNCTION__ << std::endl;
  EXPECT_EQ(0, mkdir(directory_path.c_str(),
                     S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP));

  struct stat64 unix_st {};
  EXPECT_EQ(0, stat64(directory_path.c_str(), &unix_st));
  EXPECT_EQ(static_cast<std::uint32_t>(S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |
                                       S_IXGRP),
            ACCESSPERMS & unix_st.st_mode);
  EXPECT_TRUE(S_ISDIR(unix_st.st_mode));
  EXPECT_FALSE(S_ISREG(unix_st.st_mode));
}

static void
test_write_operations_fail_if_read_only(const std::string & /* api_path */,
                                        const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  auto fd =
      open(file_path.c_str(), O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP);
  EXPECT_LE(1, fd);

  std::string data = "TestData";
  EXPECT_EQ(-1, write(fd, data.data(), data.size()));

  EXPECT_EQ(-1, ftruncate(fd, 9u));

#ifndef __APPLE__
  EXPECT_EQ(-1, fallocate(fd, 0, 0, 16));
#endif

  EXPECT_EQ(0, close(fd));

  std::this_thread::sleep_for(SLEEP_SECONDS);

  std::uint64_t file_size{};
  EXPECT_TRUE(utils::file::get_file_size(file_path, file_size));
  EXPECT_EQ(std::size_t(0u), file_size);

  // filesystem_item fsi{};
  // EXPECT_EQ(api_error::success,
  //           provider.get_filesystem_item(api_path, false, fsi));
  // EXPECT_TRUE(utils::file::get_file_size(fsi.source_path, file_size));
  // EXPECT_EQ(std::size_t(0u), file_size);
}

#if !__APPLE__ && HAS_SETXATTR
static void test_xattr_invalid_parameters(const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  std::string attr = "moose";
  EXPECT_EQ(-1, setxattr(nullptr, "user.test_attr", attr.data(), attr.size(),
                         XATTR_CREATE));
  EXPECT_EQ(errno, EFAULT);
  EXPECT_EQ(-1, setxattr(file_path.c_str(), nullptr, attr.data(), attr.size(),
                         XATTR_CREATE));
  EXPECT_EQ(errno, EFAULT);
  EXPECT_EQ(-1, setxattr(file_path.c_str(), "user.test_attr", nullptr,
                         attr.size(), XATTR_CREATE));
  EXPECT_EQ(errno, EFAULT);
  EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", nullptr, 0,
                        XATTR_CREATE));
}

static void test_xattr_create_and_get(const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  std::string attr = "moose";
  EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
                        attr.size(), XATTR_CREATE));

  std::string val;
  val.resize(attr.size());
  EXPECT_EQ(attr.size(),
            static_cast<std::size_t>(getxattr(
                file_path.c_str(), "user.test_attr", val.data(), val.size())));
  EXPECT_STREQ(attr.c_str(), val.c_str());
}

static void test_xattr_listxattr(const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  std::string attr = "moose";
  EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
                        attr.size(), XATTR_CREATE));
  EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr2", attr.data(),
                        attr.size(), XATTR_CREATE));

  std::string val;
  val.resize(attr.size());
  EXPECT_EQ(attr.size(),
            static_cast<std::size_t>(getxattr(
                file_path.c_str(), "user.test_attr", val.data(), val.size())));
  EXPECT_STREQ(attr.c_str(), val.c_str());

  std::string data;
  auto size = listxattr(file_path.c_str(), data.data(), 0U);
  EXPECT_EQ(31, size);

  data.resize(static_cast<std::size_t>(size));
  EXPECT_EQ(size, listxattr(file_path.c_str(), data.data(),
                            static_cast<std::size_t>(size)));

  auto *ptr = data.data();
  EXPECT_STREQ("user.test_attr", ptr);

  ptr += strlen(ptr) + 1;
  EXPECT_STREQ("user.test_attr2", ptr);
}

static void test_xattr_replace(const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  std::string attr = "moose";
  EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
                        attr.size(), XATTR_CREATE));

  attr = "cow";
  EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
                        attr.size(), XATTR_REPLACE));

  std::string val;
  val.resize(attr.size());
  EXPECT_EQ(attr.size(),
            static_cast<std::size_t>(getxattr(
                file_path.c_str(), "user.test_attr", val.data(), val.size())));
  EXPECT_STREQ(attr.c_str(), val.c_str());
}

static void test_xattr_default_create(const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  std::string attr = "moose";
  EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
                        attr.size(), 0));

  std::string val;
  val.resize(attr.size());
  EXPECT_EQ(attr.size(),
            static_cast<std::size_t>(getxattr(
                file_path.c_str(), "user.test_attr", val.data(), val.size())));
  EXPECT_STREQ(attr.c_str(), val.c_str());
}

static void test_xattr_default_replace(const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  std::string attr = "moose";
  EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
                        attr.size(), 0));

  attr = "cow";
  EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
                        attr.size(), 0));

  std::string val;
  val.resize(attr.size());
  EXPECT_EQ(attr.size(),
            static_cast<std::size_t>(getxattr(
                file_path.c_str(), "user.test_attr", val.data(), val.size())));
  EXPECT_STREQ(attr.c_str(), val.c_str());
}

static void test_xattr_create_fails_if_exists(const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  std::string attr = "moose";
  EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
                        attr.size(), 0));
  EXPECT_EQ(-1, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
                         attr.size(), XATTR_CREATE));
  EXPECT_EQ(EEXIST, errno);
}

static void
test_xattr_create_fails_if_not_exists(const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  std::string attr = "moose";
  EXPECT_EQ(-1, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
                         attr.size(), XATTR_REPLACE));
  EXPECT_EQ(ENODATA, errno);
}

static void test_xattr_removexattr(const std::string &file_path) {
  std::cout << __FUNCTION__ << std::endl;
  std::string attr = "moose";
  EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
                        attr.size(), XATTR_CREATE));

  EXPECT_EQ(0, removexattr(file_path.c_str(), "user.test_attr"));

  std::string val;
  val.resize(attr.size());
  EXPECT_EQ(-1, getxattr(file_path.c_str(), "user.test_attr", val.data(),
                         val.size()));
  EXPECT_EQ(ENODATA, errno);
}
#endif

TEST(fuse_drive, all_tests) {
  auto current_directory = std::filesystem::current_path();

  for (std::size_t idx = 0U; idx < 2U; idx++) {
    std::filesystem::current_path(current_directory);

    const auto test_directory =
        utils::path::absolute("./fuse_drive" + std::to_string(idx));
    EXPECT_TRUE(utils::file::delete_directory_recursively(test_directory));

    const auto mount_location = utils::path::absolute(
        utils::path::combine(test_directory, {"mount", std::to_string(idx)}));

    EXPECT_TRUE(utils::file::create_full_directory_path(mount_location));
    {
      std::unique_ptr<app_config> config_ptr{};
      std::unique_ptr<curl_comm> comm_ptr{};
      std::unique_ptr<i_provider> provider_ptr{};

      const auto cfg_directory = utils::path::absolute(
          utils::path::combine(test_directory, {"cfg", std::to_string(idx)}));
      EXPECT_TRUE(utils::file::create_full_directory_path(cfg_directory));

      std::vector<std::string> drive_args{};

      switch (idx) {
      case 0U: {
#ifdef REPERTORY_ENABLE_S3
        config_ptr =
            std::make_unique<app_config>(provider_type::s3, cfg_directory);
        {
          app_config src_cfg(provider_type::s3,
                             utils::path::combine(get_test_dir(), {"storj"}));
          config_ptr->set_enable_drive_events(true);
          config_ptr->set_event_level(event_level::verbose);
          config_ptr->set_s3_config(src_cfg.get_s3_config());
        }

        comm_ptr = std::make_unique<curl_comm>(config_ptr->get_s3_config());
        provider_ptr = std::make_unique<s3_provider>(*config_ptr, *comm_ptr);
        drive_args = std::vector<std::string>({"-s3", "-na", "storj"});
#else
        continue;
#endif
      } break;

      case 1U: {
        config_ptr =
            std::make_unique<app_config>(provider_type::sia, cfg_directory);
        {
          app_config src_cfg(provider_type::sia,
                             utils::path::combine(get_test_dir(), {"sia"}));
          config_ptr->set_enable_drive_events(true);
          config_ptr->set_event_level(event_level::debug);
          config_ptr->set_host_config(src_cfg.get_host_config());
        }

        comm_ptr = std::make_unique<curl_comm>(config_ptr->get_host_config());
        provider_ptr = std::make_unique<sia_provider>(*config_ptr, *comm_ptr);
      } break;

      default:
        std::cerr << "provider at index " << idx << " is not implemented"
                  << std::endl;
        continue;
      }

      std::thread th([&] {
        std::this_thread::sleep_for(5s);
        EXPECT_EQ(0, system(("mount|grep \"" + mount_location + "\"").c_str()));

        auto file_path = create_file_and_test(mount_location, "chmod_test");
        test_chmod(utils::path::create_api_path("chmod_test"), file_path);
        unlink_file_and_test(file_path);

        file_path = create_file_and_test(mount_location, "chown_test");
        test_chown(utils::path::create_api_path("chown_test"), file_path);
        unlink_file_and_test(file_path);

        file_path = utils::path::combine(mount_location, {"mkdir_test"});
        test_mkdir(utils::path::create_api_path("mkdir_test"), file_path);
        rmdir_and_test(file_path);

        file_path = create_file_and_test(mount_location, "write_read_test");
        test_write_and_read(utils::path::create_api_path("write_read_test"),
                            file_path);
        unlink_file_and_test(file_path);

        file_path =
            create_file_and_test(mount_location, "from_rename_file_test");
        auto to_file_path = utils::path::absolute(
            utils::path::combine(mount_location, {"to_rename_file_test"}));
        test_rename_file(file_path, to_file_path,
                         provider_ptr->is_rename_supported());
        EXPECT_TRUE(utils::file::retry_delete_file(file_path));
        EXPECT_TRUE(utils::file::retry_delete_file(to_file_path));

        file_path = utils::path::absolute(
            utils::path::combine(mount_location, {"from_rename_dir_test"}));
        to_file_path = utils::path::absolute(
            utils::path::combine(mount_location, {"to_rename_dir_test"}));
        test_rename_directory(file_path, to_file_path,
                              provider_ptr->is_rename_supported());
        EXPECT_TRUE(utils::file::retry_delete_directory(file_path.c_str()));
        EXPECT_TRUE(utils::file::retry_delete_directory(to_file_path.c_str()));

        file_path = create_file_and_test(mount_location, "truncate_file_test");
        test_truncate(file_path);
        unlink_file_and_test(file_path);

        file_path = create_file_and_test(mount_location, "ftruncate_file_test");
        test_ftruncate(file_path);
        unlink_file_and_test(file_path);

#ifndef __APPLE__
        file_path = create_file_and_test(mount_location, "fallocate_file_test");
        test_fallocate(utils::path::create_api_path("fallocate_file_test"),
                       file_path, *provider_ptr);
        unlink_file_and_test(file_path);
#endif

        file_path = create_file_and_test(mount_location, "write_fails_ro_test");
        test_write_operations_fail_if_read_only(
            utils::path::create_api_path("write_fails_ro_test"), file_path);
        unlink_file_and_test(file_path);

        file_path = create_file_and_test(mount_location, "getattr.txt");
        test_file_getattr(utils::path::create_api_path("getattr.txt"),
                          file_path);
        unlink_file_and_test(file_path);

        file_path = utils::path::combine(mount_location, {"getattr_dir"});
        test_directory_getattr(utils::path::create_api_path("getattr_dir"),
                               file_path);
        rmdir_and_test(file_path);

#if !__APPLE__ && HAS_SETXATTR
        file_path =
            create_file_and_test(mount_location, "xattr_invalid_names_test");
        test_xattr_invalid_parameters(file_path);
        unlink_file_and_test(file_path);

        file_path =
            create_file_and_test(mount_location, "xattr_create_get_test");
        test_xattr_create_and_get(file_path);
        unlink_file_and_test(file_path);

        file_path =
            create_file_and_test(mount_location, "xattr_listxattr_test");
        test_xattr_listxattr(file_path);
        unlink_file_and_test(file_path);

        file_path = create_file_and_test(mount_location, "xattr_replace_test");
        test_xattr_replace(file_path);
        unlink_file_and_test(file_path);

        file_path =
            create_file_and_test(mount_location, "xattr_default_create_test");
        test_xattr_default_create(file_path);
        unlink_file_and_test(file_path);

        file_path =
            create_file_and_test(mount_location, "xattr_default_replace_test");
        test_xattr_default_replace(file_path);
        unlink_file_and_test(file_path);

        file_path = create_file_and_test(mount_location,
                                         "xattr_create_fails_exists_test");
        test_xattr_create_fails_if_exists(file_path);
        unlink_file_and_test(file_path);

        file_path = create_file_and_test(mount_location,
                                         "xattr_create_fails_not_exists_test");
        test_xattr_create_fails_if_not_exists(file_path);
        unlink_file_and_test(file_path);

        file_path =
            create_file_and_test(mount_location, "xattr_removexattr_test");
        test_xattr_removexattr(file_path);
        unlink_file_and_test(file_path);
#endif

        execute_unmount(mount_location);
      });

      drive_args.push_back(mount_location);
      execute_mount(config_ptr->get_data_directory(), drive_args, th);
    }
  }

  std::filesystem::current_path(current_directory);
}
} // namespace repertory

#endif
