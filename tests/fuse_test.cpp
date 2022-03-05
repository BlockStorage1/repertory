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
#if !_WIN32

#include "fixtures/fuse_fixture.hpp"
#include "test_common.hpp"
#include "utils/event_capture.hpp"
#include "utils/file_utils.hpp"

#ifndef ACCESSPERMS
#define ACCESSPERMS (S_IRWXU | S_IRWXG | S_IRWXO) /* 0777 */
#endif

namespace repertory {
static void retry_unlink(const std::string &file) {
  int ret = 0;
  std::this_thread::sleep_for(100ms);
  for (auto i = 0; ((ret = unlink(file.c_str())) != 0) && (i < 20); i++) {
    std::this_thread::sleep_for(100ms);
  }
  EXPECT_EQ(0, ret);
}

static auto mount_setup(const std::size_t &idx, fuse_test *test, std::string &mount_point) {
  // Mock communicator setup
  static long i = 0;
  if (idx == 0) {
    auto uid = std::to_string(getuid());
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.push_return("get", "/wallet", {}, {}, api_error::success, true);
    json files;
    files["files"] = std::vector<json>();
    mock_comm.push_return("get", "/renter/files", files, {}, api_error::success, true);
    mock_comm.push_return("get", "/renter/file/", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/.localized", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/.localized", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/.hidden", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/.hidden", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/.DS_Store", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/.DS_Store", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/BDMV", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/BDMV", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/.xdg-volume-info", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/.xdg-volume-info", {},
                          {{"message", "no file known"}}, api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/autorun.inf", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/autorun.inf", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/.Trash", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/.Trash", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/.Trash-" + uid, {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/.Trash-" + uid, {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("post", "/renter/downloads/clear", files, {}, api_error::success, true);
    mock_comm.push_return("get", "/renter/prices", {}, {{"message", "offline"}},
                          api_error::comm_error, true);
    mock_comm.push_return(
        "get", "/daemon/version",
        {{"version", app_config::get_provider_minimum_version(provider_type::sia)}}, {},
        api_error::success, true);
  } else if (idx == 2) {
#ifdef REPERTORY_ENABLE_S3_TESTING
    /* auto &mock_comm = test->mockS3Comm_; */
    /* ON_CALL(mock_comm, Exists(_)).WillByDefault(Return(false)); */
    /* ON_CALL(mock_comm, get_file_list(_)).WillByDefault(Return(api_error::success)); */
#endif
  }

  // Mount setup
  mount_point = utils::path::absolute("./fuse_mount" + std::to_string(i++));
  utils::file::create_full_directory_path(mount_point);
  return std::vector<std::string>({"unittests", "-f", mount_point});
}

static void execute_mount(const std::size_t &idx, fuse_test *test,
                          const std::vector<std::string> &drive_args, std::thread &th) {
  ASSERT_EQ(0, std::get<2>(test->provider_tests_[idx])->mount(drive_args));
  th.join();
}

static void unmount(const std::string &mount_point) {
  auto unmounted = false;
  for (int i = 0; not unmounted && (i < 10); i++) {
#if __APPLE__
    unmounted = (system(("umount \"" + mount_point + "\"").c_str()) == 0);
#else
    unmounted = (system(("fusermount -u \"" + mount_point + "\"").c_str()) == 0);
#endif
    if (not unmounted) {
      std::this_thread::sleep_for(100ms);
    }
  }

  EXPECT_TRUE(unmounted);
  rmdir(mount_point.c_str());
}

TEST_F(fuse_test, mount_and_unmount) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
    }

    event_capture ec({
        "drive_mounted",
        "drive_unmounted",
        "drive_unmount_pending",
        "drive_mount_result",
    });

    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        EXPECT_EQ(0, system(("mount|grep \"" + mount_point + "\"").c_str()));
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);

    event_system::instance().stop();
    event_system::instance().start();
  }
}

TEST_F(fuse_test, root_creation) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        struct stat st {};
        EXPECT_EQ(0, stat(mount_point.c_str(), &st));
        EXPECT_EQ(getuid(), st.st_uid);
        EXPECT_EQ(getgid(), st.st_gid);
        EXPECT_TRUE(S_ISDIR(st.st_mode));
        EXPECT_EQ(S_IRUSR | S_IWUSR | S_IXUSR, ACCESSPERMS & st.st_mode);
        EXPECT_EQ(0, st.st_size);
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, chmod) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        EXPECT_EQ(0, chmod(mount_point.c_str(), S_IRUSR | S_IWUSR));
        std::string mode;
        EXPECT_EQ(api_error::success,
                  std::get<1>(provider_tests_[idx])->get_item_meta("/", META_MODE, mode));
        EXPECT_EQ(S_IRUSR | S_IWUSR, ACCESSPERMS & utils::string::to_uint32(mode));
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, chown_uid) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        EXPECT_EQ(0, chown(mount_point.c_str(), 0, -1));

        std::string uid;
        EXPECT_EQ(api_error::success,
                  std::get<1>(provider_tests_[idx])->get_item_meta("/", META_UID, uid));
        EXPECT_STREQ("0", uid.c_str());

        std::string gid;
        EXPECT_EQ(api_error::success,
                  std::get<1>(provider_tests_[idx])->get_item_meta("/", META_GID, gid));
        if (not gid.empty()) {
          EXPECT_EQ(getgid(), utils::string::to_uint32(gid));
        }
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, chown_gid) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        EXPECT_EQ(0, chown(mount_point.c_str(), -1, 0));

        std::string uid;
        EXPECT_EQ(api_error::success,
                  std::get<1>(provider_tests_[idx])->get_item_meta("/", META_UID, uid));
        EXPECT_EQ(getuid(), utils::string::to_uint32(uid));

        std::string gid;
        EXPECT_EQ(api_error::success,
                  std::get<1>(provider_tests_[idx])->get_item_meta("/", META_GID, gid));
        EXPECT_STREQ("0", gid.c_str());
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, mkdir) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return("post_params", "/renter/dir/new_dir", {}, {}, api_error::success);
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/new_dir", {}, {{"message", "no file known"}},
                            api_error::comm_error);
      mock_comm.push_return("get", "/renter/dir/new_dir", {}, {{"message", "no file known"}},
                            api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/new_dir", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/._new_dir", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/dir/._new_dir", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return(
          "get", "/renter/dir/new_dir",
          {{"directories", {{{"siapath", "new_dir"}, {"numfiles", 0}, {"numsubdirs", 0}}}},
           {"files", {}}},
          {}, api_error::success, true);
    } else if (idx == 2) {
#ifdef REPERTORY_ENABLE_S3_TESTING
      /* EXPECT_CALL(mockS3Comm_, Exists("/new_dir")).WillRepeatedly(Return(false)); */
      /* EXPECT_CALL(mockS3Comm_, Exists(_)).WillRepeatedly(Return(false)); */
      // EXPECT_CALL(mockS3Comm_, get_directory_item_count("/new_dir")).WillRepeatedly(Return(2u));
#endif
    }

    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto newDir = utils::path::combine(mount_point, {"new_dir"});
        EXPECT_EQ(0, mkdir(newDir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP));

        EXPECT_TRUE(utils::file::is_directory(newDir));
        EXPECT_FALSE(utils::file::is_file(newDir));

        std::string uid;
        EXPECT_EQ(api_error::success,
                  std::get<1>(provider_tests_[idx])->get_item_meta("/new_dir", META_UID, uid));
        EXPECT_EQ(getuid(), utils::string::to_uint32(uid));

        std::string gid;
        EXPECT_EQ(api_error::success,
                  std::get<1>(provider_tests_[idx])->get_item_meta("/new_dir", META_GID, gid));
        EXPECT_EQ(getgid(), utils::string::to_uint32(gid));

        std::string mode;
        EXPECT_EQ(api_error::success,
                  std::get<1>(provider_tests_[idx])->get_item_meta("/new_dir", META_MODE, mode));
        EXPECT_EQ(S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP,
                  ACCESSPERMS & (utils::string::to_uint32(mode)));
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, rmdir) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;

      mock_comm.push_return("post_params", "/renter/dir/rm_dir", {}, {}, api_error::success);
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/rm_dir", {}, {{"message", "no file known"}},
                            api_error::comm_error);
      mock_comm.push_return("get", "/renter/dir/rm_dir", {}, {{"message", "no file known"}},
                            api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/rm_dir", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/._rm_dir", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/dir/._rm_dir", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);

      mock_comm.push_return(
          "get", "/renter/dir/rm_dir",
          {{"directories", {{{"siapath", "rm_dir"}, {"numfiles", 0}, {"numsubdirs", 0}}}},
           {"files", {}}},
          {}, api_error::success);

      mock_comm.push_return("post_params", "/renter/dir/rm_dir", {}, {}, api_error::success);
      mock_comm.push_return("get", "/renter/dir/rm_dir", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
    } else if (idx == 2) {
#ifdef REPERTORY_ENABLE_S3_TESTING
      /* EXPECT_CALL(mockS3Comm_, Exists("/rm_dir")).WillRepeatedly(Return(false)); */
      /* EXPECT_CALL(mockS3Comm_, Exists(_)).WillRepeatedly(Return(false)); */
      // EXPECT_CALL(mockS3Comm_, get_directory_item_count("/rm_dir")).WillRepeatedly(Return(2u));
#endif
    }

    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        event_capture ec2({"directory_removed"});

        const auto new_directory = utils::path::combine(mount_point, {"rm_dir"});
        EXPECT_EQ(0, mkdir(new_directory.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP));

        EXPECT_TRUE(utils::file::is_directory(new_directory));
        EXPECT_EQ(0, rmdir(new_directory.c_str()));
        EXPECT_FALSE(utils::file::is_directory(new_directory));

        api_meta_map meta;
        EXPECT_EQ(api_error::item_not_found,
                  std::get<1>(provider_tests_[idx])->get_item_meta("/rm_dir", meta));
        EXPECT_EQ(0, meta.size());
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, create_new_file) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/create_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/create_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/create_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/create_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/dir/._create_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/._create_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("post_params", "/renter/upload/create_file.txt", {}, {},
                            api_error::success);
      mock_comm.push_return("get", "/renter/file/create_file.txt",
                            RENTER_FILE_DATA("create_file.txt", 0), {}, api_error::success, true);
    } else if (idx == 2) {
#ifdef REPERTORY_ENABLE_S3_TESTING
      /* EXPECT_CALL(mockS3Comm_, upload_file("/create_file.txt", _, _)) */
      /*     .WillOnce(Return(api_error::success)); */
      api_file file{};
      file.accessed_date = utils::get_file_time_now();
      file.api_path =
          utils::path::create_api_path(utils::path::combine("repertory", {"create_file.txt"}));
      file.api_parent = utils::path::get_parent_api_path(file.api_path);
      file.changed_date = utils::get_file_time_now();
      file.created_date = utils::get_file_time_now();
      file.file_size = 0u;
      file.modified_date = utils::get_file_time_now();
      file.recoverable = true;
      file.redundancy = 3.0;
      /* EXPECT_CALL(mockS3Comm_, GetFile("/create_file.txt", _)) */
      /*     .WillRepeatedly(DoAll(SetArgReferee<1>(apiFile), Return(api_error::success))); */
      /* EXPECT_CALL(mockS3Comm_, Exists(_)).WillRepeatedly(Return(false)); */
#endif
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        event_capture ec2((idx == 0) ? std::vector<std::string>({
                                           "file_upload_begin",
                                           "file_upload_end",
                                           "filesystem_item_handle_closed",
                                           "filesystem_item_closed",
                                       })
                                     : std::vector<std::string>({
                                           "filesystem_item_handle_closed",
                                           "filesystem_item_closed",
                                       }));

        const auto file = utils::path::combine(mount_point, {"create_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);
        EXPECT_TRUE(utils::file::is_file(file));
        EXPECT_FALSE(utils::file::is_directory(file));

        std::uint64_t file_size;
        EXPECT_TRUE(utils::file::get_file_size(file, file_size));
        EXPECT_EQ(0, file_size);

        std::string uid;
        EXPECT_EQ(
            api_error::success,
            std::get<1>(provider_tests_[idx])->get_item_meta("/create_file.txt", META_UID, uid));
        EXPECT_EQ(getuid(), utils::string::to_uint32(uid));

        std::string gid;
        EXPECT_EQ(
            api_error::success,
            std::get<1>(provider_tests_[idx])->get_item_meta("/create_file.txt", META_GID, gid));
        EXPECT_EQ(getgid(), utils::string::to_uint32(gid));

        std::string mode;
        EXPECT_EQ(
            api_error::success,
            std::get<1>(provider_tests_[idx])->get_item_meta("/create_file.txt", META_MODE, mode));
        EXPECT_EQ(S_IRUSR | S_IWUSR | S_IRGRP, ACCESSPERMS & (utils::string::to_uint32(mode)));

        EXPECT_EQ(0, close(fd));
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, unlink) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/unlink_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/unlink_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/unlink_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/unlink_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/dir/._unlink_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/._unlink_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("post_params", "/renter/upload/unlink_file.txt", {}, {},
                            api_error::success);
      mock_comm.push_return("get", "/renter/file/unlink_file.txt",
                            RENTER_FILE_DATA("unlink_file.txt", 0), {}, api_error::success);
      mock_comm.push_return("get", "/renter/file/unlink_file.txt",
                            RENTER_FILE_DATA("unlink_file.txt", 0), {}, api_error::success);
      mock_comm.push_return("get", "/renter/file/unlink_file.txt",
                            RENTER_FILE_DATA("unlink_file.txt", 0), {}, api_error::success);
#if __APPLE__
      mock_comm.push_return("get", "/renter/file/unlink_file.txt",
                            RENTER_FILE_DATA("unlink_file.txt", 0), {}, api_error::success);
#endif
      mock_comm.push_return("post_params", "/renter/delete/unlink_file.txt", {}, {},
                            api_error::success, true);
      mock_comm.push_return("get", "/renter/file/unlink_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
    } else if (idx == 2) {
#ifdef REPERTORY_ENABLE_S3_TESTING
      /* EXPECT_CALL(mockS3Comm_, upload_file("/unlink_file.txt", _, _)) */
      /* .WillOnce(Return(api_error::success)); */
      api_file apiFile{};
      apiFile.accessed_date = utils::get_file_time_now();
      apiFile.api_path =
          utils::path::create_api_path(utils::path::combine("repertory", {"unlink_file.txt"}));
      apiFile.api_parent = utils::path::get_parent_api_path(apiFile.api_path);
      apiFile.changed_date = utils::get_file_time_now();
      apiFile.created_date = utils::get_file_time_now();
      apiFile.file_size = 0u;
      apiFile.modified_date = utils::get_file_time_now();
      apiFile.recoverable = true;
      apiFile.redundancy = 3.0;
      /* EXPECT_CALL(mockS3Comm_, GetFile("/unlink_file.txt", _)) */
      /*     .WillRepeatedly(DoAll(SetArgReferee<1>(apiFile), Return(api_error::success))); */
      /* EXPECT_CALL(mockS3Comm_, Exists(_)).WillRepeatedly(Return(false)); */
      /* EXPECT_CALL(mockS3Comm_, RemoveFile("/unlink_file.txt")) */
      /*     .WillOnce(Return(api_error::success)); */
#endif
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        event_capture ec2({"file_removed"});

        const auto file = utils::path::combine(mount_point, {"unlink_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);
        EXPECT_EQ(0, close(fd));

        filesystem_item fsi{};
        EXPECT_EQ(
            api_error::success,
            std::get<1>(provider_tests_[idx])->get_filesystem_item("/unlink_file.txt", false, fsi));
        EXPECT_TRUE(utils::file::is_file(file));
        EXPECT_TRUE(utils::file::is_file(fsi.source_path));

        retry_unlink(file);

        EXPECT_FALSE(utils::file::is_file(file));
        EXPECT_FALSE(utils::file::is_file(fsi.source_path));

        api_meta_map meta;
        EXPECT_EQ(api_error::item_not_found,
                  std::get<1>(provider_tests_[idx])->get_item_meta("/unlink_file.txt", meta));
        EXPECT_EQ(0, meta.size());
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, write) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/write_file.txt", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/write_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/write_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/write_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/dir/._write_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/._write_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("post_params", "/renter/upload/write_file.txt", {}, {},
                            api_error::success, true);
      mock_comm.push_return("get", "/renter/file/write_file.txt",
                            RENTER_FILE_DATA("write_file.txt", 0), {}, api_error::success);
      mock_comm.push_return("get", "/renter/file/write_file.txt",
                            RENTER_FILE_DATA("write_file.txt", 8), {}, api_error::success, true);
    }

    event_capture ec((idx == 0)
                         ? std::vector<std::string>({"drive_mounted"})
                         : std::vector<std::string>({"drive_mounted", "file_upload_queued"}));
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"write_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);

        std::string data = "TestData";
        EXPECT_EQ(data.size(), write(fd, &data[0], data.size()));
        EXPECT_EQ(0, close(fd));

        std::this_thread::sleep_for(1s);

        std::uint64_t file_size;
        EXPECT_TRUE(utils::file::get_file_size(file, file_size));
        EXPECT_EQ(data.size(), file_size);

        filesystem_item fsi{};
        EXPECT_EQ(
            api_error::success,
            std::get<1>(provider_tests_[idx])->get_filesystem_item("/write_file.txt", false, fsi));
        EXPECT_TRUE(utils::file::get_file_size(fsi.source_path, file_size));
        EXPECT_EQ(data.size(), file_size);
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, read) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/read_file.txt", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/read_file.txt", {}, {{"message", "no file known"}},
                            api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/read_file.txt", {}, {{"message", "no file known"}},
                            api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/read_file.txt", {}, {{"message", "no file known"}},
                            api_error::comm_error);
      mock_comm.push_return("get", "/renter/dir/._read_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/._read_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("post_params", "/renter/upload/read_file.txt", {}, {},
                            api_error::success, true);
      mock_comm.push_return("get", "/renter/file/read_file.txt",
                            RENTER_FILE_DATA("read_file.txt", 0), {}, api_error::success);
      mock_comm.push_return("get", "/renter/file/read_file.txt",
                            RENTER_FILE_DATA("read_file.txt", 8), {}, api_error::success, true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"read_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);

        std::string data = "TestData";
        EXPECT_EQ(data.size(), write(fd, &data[0], data.size()));

        EXPECT_EQ(0, lseek(fd, 0, SEEK_SET));

        std::vector<char> read_data;
        read_data.resize(data.size());
        EXPECT_EQ(data.size(), read(fd, &read_data[0], read_data.size()));

        EXPECT_EQ(0, memcmp(&data[0], &read_data[0], data.size()));

        EXPECT_EQ(0, close(fd));
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, rename) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/rename_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/dir/rename_file2.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/rename_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/rename_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/rename_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/dir/._rename_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/._rename_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/rename_file.txt",
                            RENTER_FILE_DATA("rename_file.txt", 0), {}, api_error::success);
      mock_comm.push_return("get", "/renter/file/rename_file.txt",
                            RENTER_FILE_DATA("rename_file.txt", 0), {}, api_error::success);
      mock_comm.push_return("get", "/renter/file/rename_file.txt",
                            RENTER_FILE_DATA("rename_file.txt", 0), {}, api_error::success);
#if __APPLE__
      mock_comm.push_return("get", "/renter/file/rename_file.txt",
                            RENTER_FILE_DATA("rename_file.txt", 0), {}, api_error::success);
#endif
      mock_comm.push_return("post_params", "/renter/upload/rename_file.txt", {}, {},
                            api_error::success);
      mock_comm.push_return("get", "/renter/file/rename_file2.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/rename_file2.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/dir/._rename_file2.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/._rename_file2.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("post_params", "/renter/rename/rename_file.txt", {}, {},
                            api_error::success);
      mock_comm.push_return("get", "/renter/file/rename_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/rename_file2.txt",
                            RENTER_FILE_DATA("rename_file2.txt", 0), {}, api_error::success, true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"rename_file.txt"});
        const auto new_file = utils::path::combine(mount_point, {"rename_file2.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);

        EXPECT_EQ(0, close(fd));

        api_meta_map meta1{};
        EXPECT_EQ(api_error::success,
                  std::get<1>(provider_tests_[idx])->get_item_meta("/rename_file.txt", meta1));

        EXPECT_EQ(0, rename(file.c_str(), new_file.c_str()));

        api_meta_map meta2{};
        EXPECT_EQ(api_error::success,
                  std::get<1>(provider_tests_[idx])->get_item_meta("/rename_file2.txt", meta2));
        EXPECT_STREQ(meta1[META_SOURCE].c_str(), meta2[META_SOURCE].c_str());

        filesystem_item fsi{};
        EXPECT_EQ(api_error::success, std::get<1>(provider_tests_[idx])
                                          ->get_filesystem_item("/rename_file2.txt", false, fsi));
        EXPECT_STREQ(meta1[META_SOURCE].c_str(), fsi.source_path.c_str());

        filesystem_item fsi2{};
        EXPECT_EQ(api_error::success,
                  std::get<1>(provider_tests_[idx])
                      ->get_filesystem_item_from_source_path(fsi.source_path, fsi2));
        EXPECT_STREQ("/rename_file2.txt", fsi2.api_path.c_str());

        EXPECT_EQ(-1, unlink(file.c_str()));
        EXPECT_EQ(ENOENT, errno);
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, truncate) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/truncate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/truncate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/truncate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/truncate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/dir/._truncate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/._truncate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("post_params", "/renter/upload/truncate_file.txt", {}, {},
                            api_error::success, true);
      mock_comm.push_return("get", "/renter/file/truncate_file.txt",
                            RENTER_FILE_DATA("truncate_file.txt", 0), {}, api_error::success);
      mock_comm.push_return("get", "/renter/file/truncate_file.txt",
                            RENTER_FILE_DATA("truncate_file.txt", 0), {}, api_error::success);
#if __APPLE__
      mock_comm.push_return("get", "/renter/file/truncate_file.txt",
                            RENTER_FILE_DATA("truncate_file.txt", 0), {}, api_error::success);
      mock_comm.push_return("get", "/renter/file/truncate_file.txt",
                            RENTER_FILE_DATA("truncate_file.txt", 0), {}, api_error::success);
#endif
      mock_comm.push_return("get", "/renter/file/truncate_file.txt",
                            RENTER_FILE_DATA("truncate_file.txt", 16), {}, api_error::success,
                            true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"truncate_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);
        EXPECT_EQ(0, close(fd));

        EXPECT_EQ(0, truncate(file.c_str(), 16));

        std::this_thread::sleep_for(1s);

        std::uint64_t file_size;
        EXPECT_TRUE(utils::file::get_file_size(file, file_size));
        EXPECT_EQ(16, file_size);
        filesystem_item fsi{};
        EXPECT_EQ(api_error::success, std::get<1>(provider_tests_[idx])
                                          ->get_filesystem_item("/truncate_file.txt", false, fsi));

        file_size = 0;
        EXPECT_TRUE(utils::file::get_file_size(fsi.source_path, file_size));
        EXPECT_EQ(16, file_size);
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, ftruncate) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/ftruncate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/ftruncate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/ftruncate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/ftruncate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/dir/._ftruncate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/._ftruncate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("post_params", "/renter/upload/ftruncate_file.txt", {}, {},
                            api_error::success, true);
      mock_comm.push_return("get", "/renter/file/ftruncate_file.txt",
                            RENTER_FILE_DATA("ftruncate_file.txt", 0), {}, api_error::success);
      mock_comm.push_return("get", "/renter/file/ftruncate_file.txt",
                            RENTER_FILE_DATA("ftruncate_file.txt", 16), {}, api_error::success,
                            true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"ftruncate_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);

        EXPECT_EQ(0, ftruncate(fd, 16));

        std::uint64_t file_size;
        EXPECT_TRUE(utils::file::get_file_size(file, file_size));
        EXPECT_EQ(16, file_size);

        EXPECT_EQ(0, close(fd));

        filesystem_item fsi{};
        EXPECT_EQ(api_error::success, std::get<1>(provider_tests_[idx])
                                          ->get_filesystem_item("/ftruncate_file.txt", false, fsi));

        file_size = 0;
        EXPECT_TRUE(utils::file::get_file_size(fsi.source_path, file_size));
        EXPECT_EQ(16, file_size);
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}
#ifdef __APPLE__
TEST_F(FUSETest, fallocate) {}
#else
TEST_F(fuse_test, fallocate) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/fallocate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/fallocate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/fallocate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/fallocate_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("post_params", "/renter/upload/fallocate_file.txt", {}, {},
                            api_error::success, true);
      mock_comm.push_return("get", "/renter/file/fallocate_file.txt",
                            RENTER_FILE_DATA("fallocate_file.txt", 0), {}, api_error::success);
      mock_comm.push_return("get", "/renter/file/fallocate_file.txt",
                            RENTER_FILE_DATA("fallocate_file.txt", 16), {}, api_error::success,
                            true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"fallocate_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);
        EXPECT_EQ(0, fallocate(fd, 0, 0, 16));

        std::uint64_t file_size;
        EXPECT_TRUE(utils::file::get_file_size(file, file_size));
        EXPECT_EQ(16, file_size);

        EXPECT_EQ(0, close(fd));

        filesystem_item fsi{};
        EXPECT_EQ(api_error::success, std::get<1>(provider_tests_[idx])
                                          ->get_filesystem_item("/fallocate_file.txt", false, fsi));

        file_size = 0;
        EXPECT_TRUE(utils::file::get_file_size(fsi.source_path, file_size));
        EXPECT_EQ(16, file_size);
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}
#endif
#if !__APPLE__ && HAS_SETXATTR
TEST_F(fuse_test, invalid_setxattr) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/xattr_file.txt", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("post_params", "/renter/upload/xattr_file.txt", {}, {},
                            api_error::success);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt",
                            RENTER_FILE_DATA("xattr_file.txt", 0), {}, api_error::success, true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"xattr_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);
        EXPECT_EQ(0, close(fd));

        std::string attr = "moose";
        EXPECT_EQ(-1, setxattr(nullptr, "user.test_attr", &attr[0], attr.size(), XATTR_CREATE));
        EXPECT_EQ(errno, EFAULT);
        EXPECT_EQ(-1, setxattr(file.c_str(), nullptr, &attr[0], attr.size(), XATTR_CREATE));
        EXPECT_EQ(errno, EFAULT);
        EXPECT_EQ(-1, setxattr(file.c_str(), "user.test_attr", nullptr, attr.size(), XATTR_CREATE));
        EXPECT_EQ(errno, EFAULT);
        EXPECT_EQ(0, setxattr(file.c_str(), "user.test_attr", nullptr, 0, XATTR_CREATE));
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, create_and_get_extended_attribute) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/xattr_file.txt", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("post_params", "/renter/upload/xattr_file.txt", {}, {},
                            api_error::success);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt",
                            RENTER_FILE_DATA("xattr_file.txt", 0), {}, api_error::success, true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"xattr_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);
        EXPECT_EQ(0, close(fd));

        std::string attr = "moose";
        EXPECT_EQ(0, setxattr(file.c_str(), "user.test_attr", &attr[0], attr.size(), XATTR_CREATE));

        std::string val;
        val.resize(attr.size());
        EXPECT_EQ(attr.size(), getxattr(file.c_str(), "user.test_attr", &val[0], val.size()));
        EXPECT_STREQ(attr.c_str(), val.c_str());
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, replace_extended_attribute) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/xattr_file.txt", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("post_params", "/renter/upload/xattr_file.txt", {}, {},
                            api_error::success);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt",
                            RENTER_FILE_DATA("xattr_file.txt", 0), {}, api_error::success, true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"xattr_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);
        EXPECT_EQ(0, close(fd));

        std::this_thread::sleep_for(1s);

        std::string attr = "moose";
        EXPECT_EQ(0, setxattr(file.c_str(), "user.test_attr", &attr[0], attr.size(), XATTR_CREATE));

        attr = "cow";
        EXPECT_EQ(0,
                  setxattr(file.c_str(), "user.test_attr", &attr[0], attr.size(), XATTR_REPLACE));

        std::string val;
        val.resize(attr.size());
        EXPECT_EQ(attr.size(), getxattr(file.c_str(), "user.test_attr", &val[0], val.size()));
        EXPECT_STREQ(attr.c_str(), val.c_str());
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, default_create_extended_attribute) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/xattr_file.txt", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("post_params", "/renter/upload/xattr_file.txt", {}, {},
                            api_error::success);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt",
                            RENTER_FILE_DATA("xattr_file.txt", 0), {}, api_error::success, true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"xattr_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);
        EXPECT_EQ(0, close(fd));

        std::this_thread::sleep_for(1s);

        std::string attr = "moose";
        EXPECT_EQ(0, setxattr(file.c_str(), "user.test_attr", &attr[0], attr.size(), 0));

        std::string val;
        val.resize(attr.size());
        EXPECT_EQ(attr.size(), getxattr(file.c_str(), "user.test_attr", &val[0], val.size()));
        EXPECT_STREQ(attr.c_str(), val.c_str());
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, default_replace_extended_attribute) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/xattr_file.txt", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("post_params", "/renter/upload/xattr_file.txt", {}, {},
                            api_error::success);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt",
                            RENTER_FILE_DATA("xattr_file.txt", 0), {}, api_error::success, true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"xattr_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);
        EXPECT_EQ(0, close(fd));

        std::this_thread::sleep_for(1s);

        std::string attr = "moose";
        EXPECT_EQ(0, setxattr(file.c_str(), "user.test_attr", &attr[0], attr.size(), 0));

        attr = "cow";
        EXPECT_EQ(0, setxattr(file.c_str(), "user.test_attr", &attr[0], attr.size(), 0));

        std::string val;
        val.resize(attr.size());
        EXPECT_EQ(attr.size(), getxattr(file.c_str(), "user.test_attr", &val[0], val.size()));
        EXPECT_STREQ(attr.c_str(), val.c_str());
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, create_extended_attribute_fails_if_exists) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/xattr_file.txt", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("post_params", "/renter/upload/xattr_file.txt", {}, {},
                            api_error::success);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt",
                            RENTER_FILE_DATA("xattr_file.txt", 0), {}, api_error::success, true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"xattr_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);
        EXPECT_EQ(0, close(fd));

        std::string attr = "moose";
        EXPECT_EQ(0, setxattr(file.c_str(), "user.test_attr", &attr[0], attr.size(), 0));
        EXPECT_EQ(-1,
                  setxattr(file.c_str(), "user.test_attr", &attr[0], attr.size(), XATTR_CREATE));
        EXPECT_EQ(EEXIST, errno);
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, replace_extended_attribute_fails_if_not_exists) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/xattr_file.txt", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("post_params", "/renter/upload/xattr_file.txt", {}, {},
                            api_error::success);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt",
                            RENTER_FILE_DATA("xattr_file.txt", 0), {}, api_error::success, true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"xattr_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);
        EXPECT_EQ(0, close(fd));

        std::string attr = "moose";
        EXPECT_EQ(-1,
                  setxattr(file.c_str(), "user.test_attr", &attr[0], attr.size(), XATTR_REPLACE));
        EXPECT_EQ(ENODATA, errno);
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}

TEST_F(fuse_test, removexattr) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/xattr_file.txt", {}, {{"message", "no file known"}},
                            api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("post_params", "/renter/upload/xattr_file.txt", {}, {},
                            api_error::success);
      mock_comm.push_return("get", "/renter/file/xattr_file.txt",
                            RENTER_FILE_DATA("xattr_file.txt", 0), {}, api_error::success, true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"xattr_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);
        EXPECT_EQ(0, close(fd));

        std::this_thread::sleep_for(1s);

        std::string attr = "moose";
        EXPECT_EQ(0, setxattr(file.c_str(), "user.test_attr", &attr[0], attr.size(), XATTR_CREATE));

        EXPECT_EQ(0, removexattr(file.c_str(), "user.test_attr"));

        std::string val;
        val.resize(attr.size());
        EXPECT_EQ(-1, getxattr(file.c_str(), "user.test_attr", &val[0], val.size()));
        EXPECT_EQ(ENODATA, errno);
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}
#endif

TEST_F(fuse_test, write_fails_if_file_is_read_only) {
  for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);
    if (idx == 0) {
      auto &mock_comm = mock_sia_comm_;
      mock_comm.push_return(
          "get", "/renter/dir/",
          {{"directories",
            {{{"siapath", ""}, {"numfiles", 0}, {"numsubdirs", 0}, {"aggregatenumfiles", 1}}}},
           {"files", {}}},
          {}, api_error::success, true);
      mock_comm.push_return("get", "/renter/dir/write_fails_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/write_fails_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/write_fails_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/file/write_fails_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error);
      mock_comm.push_return("get", "/renter/dir/._write_fails_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("get", "/renter/file/._write_fails_file.txt", {},
                            {{"message", "no file known"}}, api_error::comm_error, true);
      mock_comm.push_return("post_params", "/renter/upload/write_fails_file.txt", {}, {},
                            api_error::success, true);
      mock_comm.push_return("get", "/renter/file/write_fails_file.txt",
                            RENTER_FILE_DATA("write_fails_file.txt", 0), {}, api_error::success);
      mock_comm.push_return("get", "/renter/file/write_fails_file.txt",
                            RENTER_FILE_DATA("write_fails_file.txt", 8), {}, api_error::success,
                            true);
    }

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        const auto file = utils::path::combine(mount_point, {"write_fails_file.txt"});
        auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
        EXPECT_LE(1, fd);

        std::string data = "TestData";
        EXPECT_EQ(data.size(), write(fd, &data[0], data.size()));

        EXPECT_EQ(0, close(fd));

        fd = open(file.c_str(), O_RDONLY);
        EXPECT_LE(1, fd);

        EXPECT_EQ(-1, write(fd, &data[0], data.size()));
#ifndef __APPLE__
        EXPECT_EQ(-1, fallocate(fd, 0, 0, 16));
#endif
        EXPECT_EQ(-1, ftruncate(fd, 100));

        EXPECT_EQ(0, close(fd));
      }

      unmount(mount_point);
    });

    execute_mount(idx, this, drive_args, th);
  }
}
} // namespace repertory

#endif
