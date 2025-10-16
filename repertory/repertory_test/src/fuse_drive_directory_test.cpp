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

TYPED_TEST(fuse_test, directory_can_read_empty_directory) {
  std::string dir_name{"directory"};
  auto dir = this->create_directory_and_test(dir_name);

  auto *dir_ptr = ::opendir(dir.c_str());
  ASSERT_NE(dir_ptr, nullptr);

  auto names = this->read_dirnames(dir_ptr);
  EXPECT_TRUE(names.empty());

  EXPECT_EQ(0, ::closedir(dir_ptr));
  this->rmdir_and_test(dir);
}

TYPED_TEST(fuse_test, directory_can_read_populated_directory) {
  std::string dir_name{"directory"};
  auto dir = this->create_directory_and_test(dir_name);

  auto file_name_1{dir_name + "/file_a"};
  auto src_1 = this->create_file_and_test(file_name_1);

  auto file_name_2{dir_name + "/file_b"};
  auto src_2 = this->create_file_and_test(file_name_2);

  auto sub_dir_name{dir_name + "/subdir_a"};
  auto sub_dir = this->create_directory_and_test(sub_dir_name);

  auto *dir_ptr = ::opendir(dir.c_str());
  ASSERT_NE(dir_ptr, nullptr);

  auto names = this->read_dirnames(dir_ptr);

  EXPECT_TRUE(names.contains(utils::path::strip_to_file_name(src_1)));
  EXPECT_TRUE(names.contains(utils::path::strip_to_file_name(src_2)));
  EXPECT_TRUE(names.contains(utils::path::strip_to_file_name(sub_dir)));

  ::rewinddir(dir_ptr);
  auto names2 = this->read_dirnames(dir_ptr);
  EXPECT_EQ(names, names2);

  EXPECT_EQ(0, ::closedir(dir_ptr));

  this->unlink_file_and_test(src_1);
  this->unlink_file_and_test(src_2);
  this->rmdir_and_test(sub_dir);
  this->rmdir_and_test(dir);
}

TYPED_TEST(fuse_test, directory_opendir_fails_for_file) {
  std::string file_name{"directory"};
  auto src = this->create_file_and_test(file_name);

  errno = 0;
  auto *dir_ptr = ::opendir(src.c_str());
  EXPECT_EQ(dir_ptr, nullptr);
  EXPECT_EQ(errno, ENOTDIR);

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, directory_opendir_fails_if_directory_does_not_exist) {
  std::string file_name{"directory"};
  auto dir = this->create_file_path(file_name);

  errno = 0;
  auto *dir_ptr = ::opendir(dir.c_str());
  EXPECT_EQ(dir_ptr, nullptr);
  EXPECT_EQ(errno, ENOENT);
}

TYPED_TEST(fuse_test, directory_can_opendir_after_closedir) {
  std::string dir_name{"directory"};
  auto dir = this->create_directory_and_test(dir_name);

  auto *dir_ptr = ::opendir(dir.c_str());
  ASSERT_NE(dir_ptr, nullptr);

  (void)this->read_dirnames(dir_ptr);

  EXPECT_EQ(0, ::closedir(dir_ptr));

  dir_ptr = ::opendir(dir.c_str());
  ASSERT_NE(dir_ptr, nullptr);
  EXPECT_EQ(0, ::closedir(dir_ptr));

  this->rmdir_and_test(dir);
}

TYPED_TEST(fuse_test, directory_rmdir_on_non_empty_directory_should_fail) {
  if (this->current_provider == provider_type::encrypt) {
    GTEST_SKIP();
    return;
  }

  std::string dir_name{"non_empty"};
  auto dir = this->create_directory_and_test(dir_name);

  std::string dir_name2{"non_empty_2"};
  auto dir2 = this->create_directory_and_test(dir_name2);

  std::string name{dir_name + "/child"};
  auto file = this->create_file_and_test(name, 0644);
  this->overwrite_text(file, "X");

  errno = 0;
  EXPECT_EQ(-1, ::rmdir(dir.c_str()));
  EXPECT_EQ(ENOTEMPTY, errno);

  this->unlink_file_and_test(file);
  this->rmdir_and_test(dir);
  this->rmdir_and_test(dir2);
}

TYPED_TEST(fuse_test,
           directory_rmdir_open_directory_handle_then_readdir_and_closedir) {
  std::string dname{"rm_opendir"};
  auto dir = this->create_directory_and_test(dname);

  auto *dir_ptr = ::opendir(dir.c_str());
  ASSERT_NE(dir_ptr, nullptr);

  errno = 0;
  auto res = ::rmdir(dir.c_str());
  if (res == -1 && errno == EBUSY) {
    ::closedir(dir_ptr);
    GTEST_SKIP();
  }
  ASSERT_EQ(0, res);

  struct stat u_stat{};
  EXPECT_EQ(-1, ::stat(dir.c_str(), &u_stat));
  EXPECT_EQ(ENOENT, errno);

  ::rewinddir(dir_ptr);

  errno = 0;
  auto *dir_entry = ::readdir(dir_ptr);
  EXPECT_EQ(nullptr, dir_entry);
  ::closedir(dir_ptr);
}

TYPED_TEST(fuse_test,
           directory_rmdir_open_directory_handle_non_empty_ENOTEMPTY) {
  std::string dname{"rm_opendir_ne"};
  auto dir = this->create_directory_and_test(dname);

  std::string child{dname + "/child"};
  auto child_path = this->create_file_and_test(child, 0644);
  this->overwrite_text(child_path, "x");

  auto *dir_ptr = ::opendir(dir.c_str());
  ASSERT_NE(dir_ptr, nullptr);

  errno = 0;
  EXPECT_EQ(-1, ::rmdir(dir.c_str()));
  EXPECT_EQ(ENOTEMPTY, errno);

  ::rewinddir(dir_ptr);
  [[maybe_unused]] auto *dir_ptr2 = ::readdir(dir_ptr);
  ::closedir(dir_ptr);

  this->unlink_file_and_test(child_path);
  this->rmdir_and_test(dir);
}

// TODO implement POSIX-compliant rmdir when handle is open
// TYPED_TEST(fuse_test, directory_rmdir_open_directory_handle_fstat_dirfd_ok) {
//   std::string dname{"rm_opendir_fstat"};
//   auto dir = this->create_directory_and_test(dname);
//
//   auto *dir_ptr = ::opendir(dir.c_str());
//   ASSERT_NE(dir_ptr, nullptr);
//   auto dfd = ::dirfd(dir_ptr);
//   ASSERT_NE(dfd, -1);
//
//   struct stat before{};
//   ASSERT_EQ(0, ::fstat(dfd, &before));
//
//   errno = 0;
//   auto res = ::rmdir(dir.c_str());
//   if (res == -1 && errno == EBUSY) {
//     ::closedir(dir_ptr);
//     GTEST_SKIP();
//   }
//   ASSERT_EQ(0, res);
//
//   struct stat after{};
//   ASSERT_EQ(0, ::fstat(dfd, &after));
//
//   ::closedir(dir_ptr);
//
//   struct stat u_stat{};
//   EXPECT_EQ(-1, ::stat(dir.c_str(), &u_stat));
//   EXPECT_EQ(ENOENT, errno);
// }
} // namespace repertory

#endif // !defined(_WIN32)
