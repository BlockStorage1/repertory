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

#include "fixtures/drive_fixture.hpp"

namespace repertory {
TYPED_TEST_SUITE(fuse_test, platform_provider_types);

TYPED_TEST(fuse_test, rename_can_rename_a_file) {
  if (this->current_provider != provider_type::sia) {
    // TODO finish test
    GTEST_SKIP();
    return;
  }

  std::string src_file_name{"rename_test"};
  auto src = this->create_file_and_test(src_file_name);

  std::string dest_file_name{"rename_test_2"};
  auto dst = this->create_file_and_test(dest_file_name);

  errno = 0;
  ASSERT_EQ(0, ::rename(src.c_str(), dst.c_str()));

  errno = 0;
  EXPECT_EQ(-1, ::access(src.c_str(), F_OK));
  EXPECT_EQ(ENOENT, errno);

  struct stat st_unix{};
  ASSERT_EQ(0, ::stat(dst.c_str(), &st_unix));
  EXPECT_TRUE(S_ISREG(st_unix.st_mode));

  this->unlink_file_and_test(dst);
}

TYPED_TEST(fuse_test, rename_can_rename_a_directory) {
  if (this->current_provider != provider_type::sia) {
    // TODO finish test
    GTEST_SKIP();
    return;
  }

  std::string src_dir_name{"rename_test"};
  auto src_dir = this->create_directory_and_test(src_dir_name);

  auto dst_dir = utils::path::combine(utils::path::get_parent_path(src_dir),
                                      {"rename_test_2"});

  errno = 0;
  ASSERT_EQ(0, ::rename(src_dir.c_str(), dst_dir.c_str()));

  errno = 0;
  EXPECT_EQ(-1, ::access(src_dir.c_str(), F_OK));
  EXPECT_EQ(ENOENT, errno);

  struct stat st_unix{};
  ASSERT_EQ(0, ::stat(dst_dir.c_str(), &st_unix));
  EXPECT_TRUE(S_ISDIR(st_unix.st_mode));

  this->rmdir_and_test(dst_dir);
}

TYPED_TEST(fuse_test, rename_can_overwrite_existing_file) {
  if (this->current_provider != provider_type::sia) {
    // TODO finish test
    GTEST_SKIP();
    return;
  }

  std::string src_name{"rename.txt"};
  auto src = this->create_file_and_test(src_name);

  std::string dst_name{"rename2.txt"};
  auto dst = this->create_file_and_test(dst_name);

  this->overwrite_text(src, "SRC");
  this->overwrite_text(dst, "DST");

  errno = 0;
  ASSERT_EQ(0, ::rename(src.c_str(), dst.c_str()));

  errno = 0;
  EXPECT_EQ(-1, ::access(src.c_str(), F_OK));
  EXPECT_EQ(ENOENT, errno);

  EXPECT_EQ("SRC", this->slurp(dst));

  this->unlink_file_and_test(dst);
}

TYPED_TEST(fuse_test, rename_can_rename_file_into_different_directory) {
  if (this->current_provider != provider_type::sia) {
    // TODO finish test
    GTEST_SKIP();
    return;
  }

  std::string dir_name_1{"dir_1"};
  auto dir1 = this->create_directory_and_test(dir_name_1);

  std::string dir_name_2{"dir_2"};
  auto dir2 = this->create_directory_and_test(dir_name_2);

  std::string file_name{dir_name_1 + "/rename"};
  auto src = this->create_file_and_test(file_name);
  std::string dst = utils::path::combine(dir2, {"moved.txt"});

  this->overwrite_text(src, "CMDC");

  errno = 0;
  ASSERT_EQ(0, ::rename(src.c_str(), dst.c_str()));

  errno = 0;
  EXPECT_EQ(-1, ::access(src.c_str(), F_OK));
  EXPECT_EQ(ENOENT, errno);
  EXPECT_EQ("CMDC", this->slurp(dst));

  this->unlink_file_and_test(dst);
  this->rmdir_and_test(dir1);
  this->rmdir_and_test(dir2);
}

TYPED_TEST(fuse_test, rename_can_rename_file_to_same_path) {
  if (this->current_provider != provider_type::sia) {
    // TODO finish test
    GTEST_SKIP();
    return;
  }

  std::string file_name{"rename"};
  auto src = this->create_file_and_test(file_name);
  this->overwrite_text(src, "CMDC");

  errno = 0;
  EXPECT_EQ(0, ::rename(src.c_str(), src.c_str()));
  EXPECT_EQ("CMDC", this->slurp(src));

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, rename_file_fails_if_source_file_does_not_exist) {
  if (this->current_provider != provider_type::sia) {
    // TODO finish test
    GTEST_SKIP();
    return;
  }

  std::string src_file_name{"rename"};
  auto src = this->create_file_path(src_file_name);

  std::string dst_file_name{"rename_2"};
  auto dst = this->create_file_path(dst_file_name);

  errno = 0;
  EXPECT_EQ(-1, ::rename(src.c_str(), dst.c_str()));
  EXPECT_EQ(ENOENT, errno);

  EXPECT_FALSE(utils::file::file{src}.exists());
  EXPECT_FALSE(utils::file::file{dst}.exists());
}

TYPED_TEST(fuse_test,
           rename_file_fails_if_destination_directory_does_not_exist) {
  if (this->current_provider != provider_type::sia) {
    // TODO finish test
    GTEST_SKIP();
    return;
  }

  std::string file_name{"rename"};
  auto src = this->create_file_and_test(file_name);

  std::string dst_file_name{"cow_moose_doge_chicken/rename_2"};
  auto dst = this->create_file_path(dst_file_name);

  errno = 0;
  EXPECT_EQ(-1, ::rename(src.c_str(), dst.c_str()));
  EXPECT_EQ(ENOENT, errno);

  EXPECT_FALSE(utils::file::file{dst}.exists());

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, rename_file_fails_if_destination_is_directory) {
  if (this->current_provider != provider_type::sia) {
    // TODO finish test
    GTEST_SKIP();
    return;
  }

  std::string file_name{"rename"};
  auto src = this->create_file_and_test(file_name);

  std::string dir_name{"dir"};
  auto dest_dir = this->create_directory_and_test(dir_name);

  errno = 0;
  EXPECT_EQ(-1, ::rename(src.c_str(), dest_dir.c_str()));
  EXPECT_EQ(EISDIR, errno);

  EXPECT_TRUE(utils::file::directory{dest_dir}.exists());

  this->unlink_file_and_test(src);
  this->rmdir_and_test(dest_dir);
}

TYPED_TEST(fuse_test, rename_file_fails_if_source_directory_is_read_only) {
  if (this->current_provider != provider_type::sia) {
    // TODO finish test
    GTEST_SKIP();
    return;
  }

  std::string src_dir_name("dir_1");
  auto src_dir = this->create_directory_and_test(src_dir_name);

  std::string dest_dir_name("dir_2");
  auto dest_dir = this->create_directory_and_test(dest_dir_name);

  std::string file_name{src_dir_name + "/rename"};
  auto src = this->create_file_and_test(file_name);
  auto dst = dest_dir + "/dest";

  ASSERT_EQ(0, ::chmod(src_dir.c_str(), 0555));

  errno = 0;
  EXPECT_EQ(-1, ::rename(src.c_str(), dst.c_str()));
  EXPECT_EQ(EACCES, errno);

  EXPECT_FALSE(utils::file::file{dst}.exists());

  ASSERT_EQ(0, ::chmod(src_dir.c_str(), 0755));

  this->unlink_file_and_test(src);
  this->rmdir_and_test(src_dir);
  this->rmdir_and_test(dest_dir);
}

TYPED_TEST(fuse_test, rename_file_fails_if_destination_directory_is_read_only) {
  if (this->current_provider != provider_type::sia) {
    // TODO finish test
    GTEST_SKIP();
    return;
  }

  std::string src_dir_name("dir_1");
  auto src_dir = this->create_directory_and_test(src_dir_name);

  std::string dest_dir_name("dir_2");
  auto dest_dir = this->create_directory_and_test(dest_dir_name);

  std::string file_name{src_dir_name + "/rename"};
  auto src = this->create_file_and_test(file_name);
  auto dst = dest_dir + "/dest";

  ASSERT_EQ(0, ::chmod(dest_dir.c_str(), 0555));

  errno = 0;
  EXPECT_EQ(-1, ::rename(src.c_str(), dst.c_str()));
  EXPECT_EQ(EACCES, errno);

  EXPECT_FALSE(utils::file::file{dst}.exists());

  ASSERT_EQ(0, ::chmod(dest_dir.c_str(), 0755));

  this->unlink_file_and_test(src);
  this->rmdir_and_test(src_dir);
  this->rmdir_and_test(dest_dir);
}

TYPED_TEST(fuse_test, rename_file_succeeds_if_destination_file_is_read_only) {
  if (this->current_provider != provider_type::sia) {
    // TODO finish test
    GTEST_SKIP();
    return;
  }

  std::string src_file_name{"rename_test"};
  auto src = this->create_file_and_test(src_file_name);

  std::string dest_file_name{"rename_test_2"};
  auto dst = this->create_file_and_test(dest_file_name);

  this->overwrite_text(src, "NEW");
  this->overwrite_text(dst, "OLD");

  ASSERT_EQ(0, ::chmod(dst.c_str(), 0444));

  errno = 0;
  auto res = ::rename(src.c_str(), dst.c_str());
  if (res == -1 && errno == EROFS) {
    this->unlink_file_and_test(src);
    ASSERT_EQ(0, ::chmod(dst.c_str(), 0644));
    this->unlink_file_and_test(dst);
    GTEST_SKIP();
  }
  ASSERT_EQ(0, res);

  EXPECT_EQ("NEW", this->slurp(dst));

  ASSERT_EQ(0, ::chmod(dst.c_str(), 0644));
  this->unlink_file_and_test(dst);
}

TYPED_TEST(fuse_test, rename_file_retains_open_file_descriptor) {
  if (this->current_provider != provider_type::sia) {
    // TODO finish test
    GTEST_SKIP();
    return;
  }

  std::string src_file_name{"rename_test"};
  auto src = this->create_file_and_test(src_file_name);

  std::string dest_file_name{"rename_test_2"};
  auto dst = this->create_file_and_test(dest_file_name);

  this->overwrite_text(src, "HELLO");

  int desc = ::open(src.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  errno = 0;
  ASSERT_EQ(0, ::rename(src.c_str(), dst.c_str()));

  errno = 0;
  EXPECT_EQ(-1, ::access(src.c_str(), F_OK));
  EXPECT_EQ(ENOENT, errno);

  ASSERT_NE(-1, ::lseek(desc, 0, SEEK_END));
  this->write_all(desc, " WORLD");
  ::close(desc);

  EXPECT_EQ("HELLO WORLD", this->slurp(dst));
  this->unlink_file_and_test(dst);
}

// TODO revisit tests
/*
TYPED_TEST(fuse_test, rename_file_on_readonly_mount_sets_erofs_or_skip) {
  auto src = this->create_file_and_test("rn_ro_src.txt");
  auto dst = this->mount_location() + "/rn_ro_dst.txt";

  if (::rename(src.c_str(), src.c_str()) == 0) {
    this->unlink_file_and_test(src);
    GTEST_SKIP();
  }

  errno = 0;
  EXPECT_EQ(-1, ::rename(src.c_str(), dst.c_str()));
  EXPECT_EQ(EROFS, errno);

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, rename_file_destination_component_name_too_long) {
  auto src = this->create_file_and_test("rn_ntl_src.txt");

  std::string longname(300, 'a');
  std::string dst = this->mount_location() + "/" + longname;

  errno = 0;
  EXPECT_EQ(-1, ::rename(src.c_str(), dst.c_str()));
  EXPECT_TRUE(errno == ENAMETOOLONG || errno == EINVAL);

  this->unlink_file_and_test(src);
} */
} // namespace repertory

#endif // !defined(_WIN32)
