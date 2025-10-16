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

TYPED_TEST(fuse_test, getattr_regular_file_reports_type_and_size) {
  std::string name{"getattr"};
  auto src = this->create_file_and_test(name);
  this->overwrite_text(src, "HELLO");

  struct stat st_unix{};
  errno = 0;
  ASSERT_EQ(0, ::stat(src.c_str(), &st_unix));

  EXPECT_TRUE(S_ISREG(st_unix.st_mode));
  EXPECT_EQ(5, st_unix.st_size);

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, getattr_directory_reports_type) {
  std::string dir_name{"getattr_dir"};
  auto dir = this->create_directory_and_test(dir_name);

  struct stat st_unix{};
  errno = 0;
  ASSERT_EQ(0, ::stat(dir.c_str(), &st_unix));
  EXPECT_TRUE(S_ISDIR(st_unix.st_mode));

  this->rmdir_and_test(dir);
}

TYPED_TEST(fuse_test, getattr_missing_path_sets_enoent) {
  std::string file_name{"getattr"};
  auto src = this->create_file_path(file_name);
  struct stat st_unix{};
  errno = 0;
  EXPECT_EQ(-1, ::stat(src.c_str(), &st_unix));
  EXPECT_EQ(ENOENT, errno);
}

TYPED_TEST(fuse_test, fgetattr_on_open_file_reflects_size_growth) {
  std::string name{"fgetattr"};
  auto src = this->create_file_and_test(name);
  this->overwrite_text(src, "ABC");

  auto desc = ::open(src.c_str(), O_RDWR | O_APPEND);
  ASSERT_NE(desc, -1);

  std::string_view more{"DEF"};
  ASSERT_EQ(3, ::write(desc, more.data(), more.size()));

  struct stat st_unix{};
  errno = 0;
  ASSERT_EQ(0, ::fstat(desc, &st_unix));
  EXPECT_TRUE(S_ISREG(st_unix.st_mode));
  EXPECT_EQ(6, st_unix.st_size);

  ::close(desc);
  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, fgetattr_directory_reports_type) {
  std::string dir_name{"dir"};
  auto dir = this->create_directory_and_test(dir_name);

#if defined(O_DIRECTORY)
  auto desc = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY);
#else  // !defined(O_DIRECTORY)
  auto desc = ::open(d.c_str(), O_RDONLY);
#endif // defined(O_DIRECTORY)
  ASSERT_NE(desc, -1);

  struct stat st_unix{};
  errno = 0;
  ASSERT_EQ(0, ::fstat(desc, &st_unix));
  EXPECT_TRUE(S_ISDIR(st_unix.st_mode));

  ::close(desc);
  this->rmdir_and_test(dir);
}

TYPED_TEST(fuse_test, fgetattr_on_closed_fd_sets_ebadf) {
  std::string name{"fgetattr"};
  auto src = this->create_file_and_test(name);
  this->overwrite_text(src, "X");

  auto desc = ::open(src.c_str(), O_RDONLY);
  ASSERT_NE(desc, -1);
  ASSERT_EQ(0, ::close(desc));

  struct stat st_unix{};
  errno = 0;
  EXPECT_EQ(-1, ::fstat(desc, &st_unix));
  EXPECT_EQ(EBADF, errno);

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, getattr_reflects_changes_after_write_and_chmod) {
  std::string name{"getattr"};
  auto src = this->create_file_and_test(name);
  this->overwrite_text(src, "HI"); // 2 bytes

  auto desc = ::open(src.c_str(), O_RDWR | O_APPEND);
  ASSERT_NE(desc, -1);
  std::string_view more{"CMDC"};
  ASSERT_EQ(4, ::write(desc, more.data(), more.size()));
  ASSERT_EQ(0, ::fsync(desc));
  ASSERT_EQ(0, ::close(desc));

  ASSERT_EQ(0, ::chmod(src.c_str(), 0644));

  struct stat st_unix{};
  errno = 0;
  ASSERT_EQ(0, ::stat(src.c_str(), &st_unix));
  EXPECT_TRUE(S_ISREG(st_unix.st_mode));
  EXPECT_EQ(6, st_unix.st_size);

  this->unlink_file_and_test(src);
}
} // namespace repertory

#endif // !defined(_WIN32)
