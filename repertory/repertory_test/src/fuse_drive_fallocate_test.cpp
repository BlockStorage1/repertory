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
#include <gtest/gtest.h>
#if !defined(_WIN32)

#include "fixtures/drive_fixture.hpp"

namespace repertory {
TYPED_TEST_SUITE(fuse_test, platform_provider_types);

TYPED_TEST(fuse_test, fallocate_can_handle_preallocate) {
  std::string name{"fallocate"};
  auto src = this->create_file_and_test(name);

  auto desc = ::open(src.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  constexpr off_t len = 64 * 1024;

#if defined(__APPLE__)
  fstore_t store{};
  store.fst_flags = F_ALLOCATECONTIG;
  store.fst_posmode = F_PEOFPOSMODE;
  store.fst_offset = 0;
  store.fst_length = len;
  store.fst_bytesalloc = 0;

  auto res = ::fcntl(desc, F_PREALLOCATE, &store);
  EXPECT_EQ(res, -1);

  struct stat st_unix{};
  EXPECT_EQ(0, ::fstat(desc, &st_unix));
  EXPECT_TRUE(S_ISREG(st_unix.st_mode));
  EXPECT_EQ(0, st_unix.st_size);
#else  // !defined(__APPLE__)
  constexpr off_t off = 0;

  auto res = ::posix_fallocate(desc, off, len);
  if (res == EOPNOTSUPP) {
    ::close(desc);
    this->unlink_file_and_test(src);
    return;
  }
  EXPECT_EQ(0, res);

  struct stat st_unix{};
  EXPECT_EQ(0, ::fstat(desc, &st_unix));
  EXPECT_TRUE(S_ISREG(st_unix.st_mode));
  EXPECT_EQ(off + len, st_unix.st_size);
#endif // defined(__APPLE__)

  ::close(desc);
  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, fallocate_then_ftruncate_makes_size_visible) {
  std::string name{"fallocate"};
  auto src = this->create_file_and_test(name);

  auto desc = ::open(src.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  constexpr off_t len = 128 * 1024;

#if defined(__APPLE__)
  fstore_t store{};
  store.fst_flags = F_ALLOCATECONTIG;
  store.fst_posmode = F_PEOFPOSMODE;
  store.fst_offset = 0;
  store.fst_length = len;
  store.fst_bytesalloc = 0;
  auto res = ::fcntl(desc, F_PREALLOCATE, &store);
  EXPECT_EQ(res, -1);

  EXPECT_EQ(0, ::ftruncate(desc, len));
#else  // !defined(__APPLE__)
  auto res = ::posix_fallocate(desc, 0, len);
  if (res == EOPNOTSUPP) {
    ::close(desc);
    this->unlink_file_and_test(src);
    return;
  }
  EXPECT_EQ(0, res);
  EXPECT_EQ(0, ::ftruncate(desc, len / 2));
  EXPECT_EQ(0, ::ftruncate(desc, len));
#endif // defined(__APPLE__)

  struct stat st_unix{};
  EXPECT_EQ(0, ::fstat(desc, &st_unix));
  EXPECT_TRUE(S_ISREG(st_unix.st_mode));
  EXPECT_EQ(len, st_unix.st_size);

  ::close(desc);
  this->unlink_file_and_test(src);
}

#if !defined(__APPLE__)
TYPED_TEST(fuse_test,
           fallocate_does_not_change_size_when_keep_size_is_specified) {
  std::string name{"fallocate"};
  auto src = this->create_file_and_test(name);

  auto desc = ::open(src.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  EXPECT_EQ(0, ::ftruncate(desc, 4096));

  constexpr off_t len = 64 * 1024;
  errno = 0;
  auto res = ::fallocate(desc, FALLOC_FL_KEEP_SIZE, 0, len);
  if (res == -1 &&
      (errno == ENOSYS || errno == EOPNOTSUPP || errno == EINVAL)) {
    ::close(desc);
    this->unlink_file_and_test(src);
    return;
  }
  EXPECT_EQ(0, res);

  struct stat st_unix{};
  EXPECT_EQ(0, ::fstat(desc, &st_unix));
  EXPECT_TRUE(S_ISREG(st_unix.st_mode));
  EXPECT_EQ(4096, st_unix.st_size);

  ::close(desc);
  this->unlink_file_and_test(src);
}

TYPED_TEST(
    fuse_test,
    fallocate_does_not_change_size_when_keep_size_and_punch_hole_are_specified) {
  std::string name{"fallocate"};
  auto src = this->create_file_and_test(name);

  auto desc = ::open(src.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  constexpr off_t size = 64 * 1024;
  EXPECT_EQ(0, ::ftruncate(desc, size));

  constexpr off_t hole_off = 24 * 1024;
  constexpr off_t hole_len = 8 * 1024;

  errno = 0;
  auto res = ::fallocate(desc, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE,
                         hole_off, hole_len);
  if (res == -1 &&
      (errno == ENOSYS || errno == EOPNOTSUPP || errno == EINVAL)) {
    ::close(desc);
    this->unlink_file_and_test(src);
    return;
  }
  EXPECT_EQ(0, res) << "errno: " << errno;

  struct stat st_unix{};
  EXPECT_EQ(0, ::fstat(desc, &st_unix));
  EXPECT_EQ(size, st_unix.st_size);

  ::close(desc);
  this->unlink_file_and_test(src);
}
#endif // !defined(__APPLE__)

TYPED_TEST(fuse_test, fallocate_can_handle_invalid_arguments) {
  std::string name{"fallocate"};
  auto src = this->create_file_and_test(name);

  auto desc = ::open(src.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

#if defined(__APPLE__)
  fstore_t store{};
  store.fst_flags = F_ALLOCATEALL;
  store.fst_posmode = F_PEOFPOSMODE;
  store.fst_offset = 0;
  store.fst_length = 0;
  errno = 0;
  auto res = ::fcntl(desc, F_PREALLOCATE, &store);
  EXPECT_EQ(res, -1);
#else  // !defined(__APPLE__)
  auto ret1 = ::posix_fallocate(desc, -1, 4096);
  EXPECT_EQ(EINVAL, ret1);

  auto ret2 = ::posix_fallocate(desc, 0, -4096);
  EXPECT_EQ(EINVAL, ret2);
#endif // defined(__APPLE__)

  ::close(desc);
  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, fallocate_fails_on_directory) {
  std::string dir_name{"dir"};
  auto dir = this->create_directory_and_test(dir_name);

#if defined(__APPLE__)
  auto desc = ::open(dir.c_str(), O_RDONLY);
  EXPECT_NE(desc, -1);

  fstore_t store{};
  store.fst_flags = F_ALLOCATEALL;
  store.fst_posmode = F_PEOFPOSMODE;
  store.fst_offset = 0;
  store.fst_length = 4096;

  errno = 0;
  auto res = ::fcntl(desc, F_PREALLOCATE, &store);
  EXPECT_EQ(res, -1);
#else  // !defined(__APPLE__)
  auto desc = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY);
  EXPECT_NE(desc, -1);

  auto ret = ::posix_fallocate(desc, 0, 4096);
  EXPECT_NE(0, ret);
#endif // defined(__APPLE__)

  ::close(desc);
  this->rmdir_and_test(dir);
}
} // namespace repertory

#endif // !defined(_WIN32)
