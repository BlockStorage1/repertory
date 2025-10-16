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

TYPED_TEST(fuse_test, unlink_can_remove_file) {
  std::string name{"unlink"};
  auto path = this->create_file_and_test(name);

  this->unlink_file_and_test(path);

  struct stat st{};
  EXPECT_EQ(-1, ::stat(path.c_str(), &st));
  EXPECT_EQ(ENOENT, errno);
}

TYPED_TEST(fuse_test, unlink_open_file_leaves_handle_intact) {
#if defined(__APPLE__)
  // TODO fgetattr() not supported
  GTEST_SKIP();
  return;
#endif // defined(__APPLE__)

  std::string name{"unlink"};
  auto path = this->create_file_and_test(name);

  {
    auto desc = ::open(path.c_str(), O_RDWR);
    ASSERT_NE(desc, -1);
    this->write_all(desc, "HELLO");
    ::close(desc);
  }

  auto desc = ::open(path.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  ASSERT_EQ(0, ::unlink(path.c_str()));

  auto res = ::lseek(desc, 0, SEEK_END);
  fmt::println("lseek|{}|{}", res, errno);

  ASSERT_NE(-1, res);
  this->write_all(desc, " WORLD");

  ASSERT_NE(-1, ::lseek(desc, 0, SEEK_SET));
  std::string out;
  char buf[4096];
  for (;;) {
    res = ::read(desc, buf, sizeof(buf));
    ASSERT_NE(res, -1);
    if (res == 0) {
      break;
    }
    out.append(buf, buf + res);
  }
  ::close(desc);

  EXPECT_EQ("HELLO WORLD", out);
}

TYPED_TEST(fuse_test, unlink_fails_if_file_is_not_found) {
  std::string name{"unlink"};
  auto missing = this->create_file_path(name);

  errno = 0;
  EXPECT_EQ(-1, ::unlink(missing.c_str()));
  EXPECT_EQ(ENOENT, errno);
}

TYPED_TEST(fuse_test, unlink_directory_fails) {
  std::string name{"unlink"};
  auto dir = this->create_directory_and_test(name);

  errno = 0;
  EXPECT_EQ(-1, ::unlink(dir.c_str()));
#if defined(__APPLE__)
  EXPECT_EQ(EPERM, errno);
#else  // !defined(__APPLE__)
  EXPECT_EQ(EISDIR, errno);
#endif // defined(__APPLE__)

  this->rmdir_and_test(dir);
}

// TODO revisit
// TYPED_TEST(fuse_test, unlink_on_readonly_mount_erofs_or_skip) {
//   std::string name{"unlink"};
//   auto path = this->create_file_path(name);
//
//   auto desc = ::open(path.c_str(), O_CREAT | O_WRONLY, 0644);
//   if (desc == -1) {
//     if (errno == EROFS) {
//       GTEST_SKIP(); unlink would return EROFS";
//     }
//     FAIL();
//   }
//   ::close(desc);
//
//   ASSERT_EQ(0, ::unlink(path.c_str()));
//   GTEST_SKIP(); RO-specific unlink test skipped";
// }
} // namespace repertory

#endif // !defined(_WIN32)
