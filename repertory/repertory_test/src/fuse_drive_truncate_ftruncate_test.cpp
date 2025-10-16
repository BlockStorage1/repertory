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

TYPED_TEST(fuse_test, truncate_can_shrink_file) {
  std::string file_name{"truncate"};
  auto src = this->create_file_and_test(file_name);
  this->overwrite_text(src, "ABCDEFGH");

  errno = 0;
  ASSERT_EQ(0, ::truncate(src.c_str(), 3));
  EXPECT_EQ(3, this->stat_size(src));
  EXPECT_EQ("ABC", this->slurp(src));

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, truncate_expand_file_is_zero_filled) {
  std::string name{"truncate"};
  auto src = this->create_file_and_test(name);
  this->overwrite_text(src, "XYZ");

  errno = 0;
  ASSERT_EQ(0, ::truncate(src.c_str(), 10));
  EXPECT_EQ(10, this->stat_size(src));

  auto data = this->slurp(src);
  ASSERT_EQ(10U, data.size());
  EXPECT_EQ('X', data.at(0U));
  EXPECT_EQ('Y', data.at(1U));
  EXPECT_EQ('Z', data.at(2U));
  for (std::size_t idx = 3; idx < data.size(); ++idx) {
    EXPECT_EQ('\0', data[idx]);
  }

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, truncate_fails_if_source_does_not_exist) {
  std::string name{"truncate"};
  auto src = this->create_file_path(name);

  errno = 0;
  EXPECT_EQ(-1, ::truncate(src.c_str(), 1));
  EXPECT_EQ(ENOENT, errno);

  EXPECT_FALSE(utils::file::file{src}.exists());
}

TYPED_TEST(fuse_test, truncate_fails_if_path_is_directory) {
  std::string name{"truncate"};
  auto src = this->create_directory_and_test(name);

  errno = 0;
  auto res = ::truncate(src.c_str(), 0);
  EXPECT_EQ(-1, res);
  EXPECT_TRUE(errno == EISDIR || errno == EPERM || errno == EACCES ||
              errno == EINVAL);

  this->rmdir_and_test(src);
}

TYPED_TEST(fuse_test, truncate_fails_if_file_is_read_only) {
  std::string name{"trunc_ro"};
  auto src = this->create_file_and_test(name);
  this->overwrite_text(src, "DATA");

  ASSERT_EQ(0, ::chmod(src.c_str(), 0444));

  errno = 0;
  int res = ::truncate(src.c_str(), 2);
  if (res == -1 && errno == EROFS) {
    ASSERT_EQ(0, ::chmod(src.c_str(), 0644));
    this->unlink_file_and_test(src);
    return;
  }

  EXPECT_EQ(-1, res);
  EXPECT_TRUE(errno == EACCES || errno == EPERM);

  ASSERT_EQ(0, ::chmod(src.c_str(), 0644));
  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, ftruncate_can_shrink_file) {
  std::string name{"ftruncate"};
  auto src = this->create_file_and_test(name);
  this->overwrite_text(src, "HELLOWORLD");

  auto desc = ::open(src.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  errno = 0;
  ASSERT_EQ(0, ::ftruncate(desc, 4));
  ::close(desc);

  EXPECT_EQ(4, this->stat_size(src));
  EXPECT_EQ("HELL", this->slurp(src));

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, ftruncate_expand_file_is_zero_filled) {
  std::string name{"ftruncate"};
  auto src = this->create_file_and_test(name);
  this->overwrite_text(src, "AA");

  auto desc = ::open(src.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  errno = 0;
  ASSERT_EQ(0, ::ftruncate(desc, 6));
  ::close(desc);

  EXPECT_EQ(6, this->stat_size(src));

  auto data = this->slurp(src);
  ASSERT_EQ(6U, data.size());
  EXPECT_EQ('A', data.at(0U));
  EXPECT_EQ('A', data.at(1U));
  for (std::size_t idx = 2; idx < data.size(); ++idx) {
    EXPECT_EQ('\0', data[idx]);
  }

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, ftruncate_fails_if_file_is_read_only) {
  std::string name{"ftruncate"};
  auto src = this->create_file_and_test(name);
  this->overwrite_text(src, "RW");

  auto desc = ::open(src.c_str(), O_RDONLY);
  ASSERT_NE(desc, -1);

  errno = 0;
  EXPECT_EQ(-1, ::ftruncate(desc, 1));
  EXPECT_TRUE(errno == EBADF || errno == EINVAL);

  ::close(desc);
  this->unlink_file_and_test(src);
}

// TODO revisit tests
/*
TYPED_TEST(fuse_test, ftruncate_on_ro_mount_erofs_or_skip) {
  if (this->current_provider != provider_type::sia)
    return;

  std::string name{"ftrunc_ro_mount"};
  std::string p = this->create_file_and_test(name);
  this->overwrite_text(p, "X");

  int desc = ::open(p.c_str(), O_RDWR);
  if (desc == -1) {
    this->unlink_file_and_test(p);
    GTEST_SKIP() << "cannot open O_RDWR; probable RO mount";
  }

  errno = 0;
  int res = ::ftruncate(desc, 0);
  if (res == -1 && errno == EROFS) {
    ::close(desc);
    this->unlink_file_and_test(p);
    GTEST_SKIP() << "read-only mount; ftruncate returns EROFS";
  }

  ASSERT_EQ(0, res) << std::strerror(errno);
  ::close(desc);

  this->unlink_file_and_test(p);
}
*/
} // namespace repertory

#endif // !defined(_WIN32)
