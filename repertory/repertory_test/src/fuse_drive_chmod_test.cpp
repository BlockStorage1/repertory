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

#include "fixtures/fuse_fixture.hpp"

namespace repertory {
TYPED_TEST_CASE(fuse_test, fuse_provider_types);

TYPED_TEST(fuse_test, chmod_can_not_chmod_set_sticky_if_not_root) {
  std::string file_name{"chmod_test"};
  auto file_path = this->create_file_and_test(file_name);

  EXPECT_EQ(-1, chmod(file_path.c_str(), S_IRUSR | S_IWUSR | S_ISVTX));
  EXPECT_EQ(EPERM, errno);

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test, chmod_can_chmod_if_owner) {
  std::string file_name{"chmod_test"};
  auto file_path = this->create_file_and_test(file_name);

  EXPECT_EQ(0, chmod(file_path.c_str(), S_IRUSR | S_IWUSR));
  std::this_thread::sleep_for(SLEEP_SECONDS);

  struct stat64 unix_st {};
  stat64(file_path.c_str(), &unix_st);
  EXPECT_EQ(static_cast<std::uint32_t>(S_IRUSR | S_IWUSR),
            ACCESSPERMS & unix_st.st_mode);

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test, chmod_can_not_chmod_if_not_owner) {
  std::string file_name{"chmod_test"};
  auto file_path = this->create_root_file(file_name);

  EXPECT_EQ(-1, chmod(file_path.c_str(), S_IRUSR | S_IWUSR));
  EXPECT_EQ(EPERM, errno);

  this->unlink_root_file(file_path);
}

TYPED_TEST(fuse_test, chmod_can_not_chmod_setgid_if_not_root) {
  std::string file_name{"chmod_test"};
  auto file_path = this->create_file_and_test(file_name);

  EXPECT_EQ(-1, chmod(file_path.c_str(), S_IRUSR | S_IWUSR | S_ISGID));
  EXPECT_EQ(EPERM, errno);

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test, chmod_can_not_chmod_setuid_if_not_root) {
  std::string file_name{"chmod_test"};
  auto file_path = this->create_file_and_test(file_name);

  EXPECT_EQ(-1, chmod(file_path.c_str(), S_IRUSR | S_IWUSR | S_ISUID));
  EXPECT_EQ(EPERM, errno);

  this->unlink_file_and_test(file_path);
}
} // namespace repertory

#endif // !defined(_WIN32)
