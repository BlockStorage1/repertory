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

TYPED_TEST(fuse_test,
           chown_can_chown_group_if_owner_and_a_member_of_the_group) {
  std::string file_name{"chown_test"};
  auto file_path = this->create_file_and_test(file_name);

  struct stat64 unix_st {};
  EXPECT_EQ(0, stat64(file_path.c_str(), &unix_st));

  EXPECT_EQ(0, chown(file_path.c_str(), static_cast<uid_t>(-1), getgid()));
  std::this_thread::sleep_for(SLEEP_SECONDS);

  struct stat64 unix_st2 {};
  stat64(file_path.c_str(), &unix_st2);
  EXPECT_EQ(getgid(), unix_st2.st_gid);
  EXPECT_EQ(unix_st.st_uid, unix_st2.st_uid);

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(
    fuse_test,
    chown_can_chown_group_when_specifying_owner_and_a_member_of_the_group) {
  std::string file_name{"chown_test"};
  auto file_path = this->create_file_and_test(file_name);

  struct stat64 unix_st {};
  EXPECT_EQ(0, stat64(file_path.c_str(), &unix_st));

  EXPECT_EQ(0, chown(file_path.c_str(), getuid(), getgid()));
  std::this_thread::sleep_for(SLEEP_SECONDS);

  struct stat64 unix_st2 {};
  stat64(file_path.c_str(), &unix_st2);
  EXPECT_EQ(getgid(), unix_st2.st_gid);
  EXPECT_EQ(unix_st.st_uid, unix_st2.st_uid);

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test,
           chown_can_not_chown_group_if_owner_but_not_a_member_of_the_group) {
  std::string file_name{"chown_test"};
  auto file_path = this->create_file_and_test(file_name);

  struct stat64 unix_st {};
  EXPECT_EQ(0, stat64(file_path.c_str(), &unix_st));

  EXPECT_EQ(-1, chown(file_path.c_str(), static_cast<uid_t>(-1), 0));
  EXPECT_EQ(EPERM, errno);

  struct stat64 unix_st2 {};
  stat64(file_path.c_str(), &unix_st2);
  EXPECT_EQ(unix_st.st_gid, unix_st2.st_gid);
  EXPECT_EQ(unix_st.st_uid, unix_st2.st_uid);

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test, chown_can_not_chown_group_if_not_the_owner) {
  std::string file_name{"chown_test"};
  auto file_path = this->create_root_file(file_name);

  struct stat64 unix_st {};
  EXPECT_EQ(0, stat64(file_path.c_str(), &unix_st));

  EXPECT_EQ(-1, chown(file_path.c_str(), static_cast<uid_t>(-1), getgid()));
  EXPECT_EQ(EPERM, errno);

  struct stat64 unix_st2 {};
  stat64(file_path.c_str(), &unix_st2);
  EXPECT_EQ(unix_st.st_gid, unix_st2.st_gid);
  EXPECT_EQ(unix_st.st_uid, unix_st2.st_uid);

  this->unlink_root_file(file_path);
}

TYPED_TEST(fuse_test, chown_can_not_chown_user_if_not_root) {
  std::string file_name{"chown_test"};
  auto file_path = this->create_file_and_test(file_name);

  struct stat64 unix_st {};
  EXPECT_EQ(0, stat64(file_path.c_str(), &unix_st));

  EXPECT_EQ(-1, chown(file_path.c_str(), 0, static_cast<gid_t>(-1)));
  EXPECT_EQ(EPERM, errno);

  struct stat64 unix_st2 {};
  stat64(file_path.c_str(), &unix_st2);
  EXPECT_EQ(unix_st.st_gid, unix_st2.st_gid);
  EXPECT_EQ(unix_st.st_uid, unix_st2.st_uid);

  this->unlink_file_and_test(file_path);
}
} // namespace repertory

#endif // !defined(_WIN32)
