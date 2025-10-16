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

TYPED_TEST(fuse_test, fsync_basic_succeeds_on_dirty_desc) {
  std::string name{"fsync_dirty"};
  auto path = this->create_file_and_test(name, 0644);

  auto desc = ::open(path.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  this->write_all(desc, "ABC");
  errno = 0;
  ASSERT_EQ(0, ::fsync(desc));
  ::close(desc);

  EXPECT_EQ("ABC", this->slurp(path));
  this->unlink_file_and_test(path);
}

TYPED_TEST(fuse_test, fsync_noop_on_clean_desc) {
  std::string name{"fsync_clean"};
  auto path = this->create_file_and_test(name, 0644);

  auto desc = ::open(path.c_str(), O_RDONLY);
  ASSERT_NE(desc, -1);

  errno = 0;
  EXPECT_EQ(0, ::fsync(desc));
  ::close(desc);

  this->unlink_file_and_test(path);
}

TYPED_TEST(fuse_test, fsync_on_unlinked_file) {
#if defined(__APPLE__)
  GTEST_SKIP();
  return;
#endif // defined(__APPLE__)

  std::string name{"fsync_unlinked"};
  auto path = this->create_file_and_test(name, 0644);

  auto desc = ::open(path.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  ASSERT_EQ(0, ::unlink(path.c_str()));

  this->write_all(desc, "XYZ");
  errno = 0;
  EXPECT_EQ(0, ::fsync(desc));
  ::close(desc);
}

TYPED_TEST(fuse_test, fsync_after_rename) {
  if (this->current_provider != provider_type::sia) {
    // TODO finish test
    GTEST_SKIP();
    return;
  }

  std::string src_name{"fsync_ren_src"};
  auto src = this->create_file_and_test(src_name, 0644);

  auto desc = ::open(src.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  this->write_all(desc, "AAA");

  std::string dst_name{"fsync_ren_dst"};
  auto dst =
      utils::path::combine(utils::path::get_parent_path(src), {dst_name});
  errno = 0;
  ASSERT_EQ(0, ::rename(src.c_str(), dst.c_str()));

  this->write_all(desc, "_BBB");
  errno = 0;
  ASSERT_EQ(0, ::fsync(desc));
  ::close(desc);

  EXPECT_EQ("AAA_BBB", this->slurp(dst));
  this->unlink_file_and_test(dst);
}

#if defined(__linux__)
TYPED_TEST(fuse_test, fsync_fdatasync_behaves_like_fsync_on_linux) {
  std::string name{"descatasync_linux"};
  auto path = this->create_file_and_test(name, 0644);

  auto desc = ::open(path.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  this->write_all(desc, "DATA");
  errno = 0;
  ASSERT_EQ(0, ::fdatasync(desc));
  ::close(desc);

  EXPECT_EQ("DATA", this->slurp(path));
  this->unlink_file_and_test(path);
}
#endif // defined(__linux__)
} // namespace repertory

#endif // !defined(_WIN32)
