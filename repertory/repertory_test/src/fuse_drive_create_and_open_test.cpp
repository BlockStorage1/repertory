/*
  Copyright <2018-2024> <scott.e.graves@protonmail.com>

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

TYPED_TEST(fuse_test, create_can_create_and_remove_directory) {
  std::string dir_name{"create_test"};
  auto dir_path = this->create_directory_and_test(dir_name);
  this->rmdir_and_test(dir_path);
}

TYPED_TEST(fuse_test, create_can_create_and_remove_file) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);
  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test, create_can_create_directory_with_specific_perms) {
  std::string dir_name{"create_test"};
  auto dir_path = this->create_directory_and_test(dir_name, S_IRUSR);

  struct stat64 unix_st {};
  EXPECT_EQ(0, stat64(dir_path.c_str(), &unix_st));
  EXPECT_EQ(S_IRUSR, unix_st.st_mode & ACCESSPERMS);

  this->rmdir_and_test(dir_path);
}

TYPED_TEST(fuse_test, create_can_create_file_with_specific_perms) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name, S_IRUSR);

  struct stat64 unix_st {};
  EXPECT_EQ(0, stat64(file_path.c_str(), &unix_st));
  EXPECT_EQ(S_IRUSR, unix_st.st_mode & ACCESSPERMS);

  this->unlink_file_and_test(file_path);
}

// 1. Create File - O_CREAT
TYPED_TEST(fuse_test, create_can_create_file) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_path(file_name);

  auto handle = open(file_path.c_str(), O_CREAT, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 2. Create File - O_CREAT | O_WRONLY
TYPED_TEST(fuse_test, create_can_create_file_wo) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_path(file_name);

  auto handle = open(file_path.c_str(), O_CREAT | O_WRONLY, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 3. Create File - O_CREAT | O_RDWR
TYPED_TEST(fuse_test, create_can_create_file_rw) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_path(file_name);

  auto handle = open(file_path.c_str(), O_CREAT | O_RDWR, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 4. Create File - O_CREAT | O_TRUNC
TYPED_TEST(fuse_test, create_can_create_with_truncate_file) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_path(file_name);

  auto handle = open(file_path.c_str(), O_CREAT | O_TRUNC, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  auto size = utils::file::file{file_path}.size();
  EXPECT_TRUE(size.has_value());
  EXPECT_EQ(0U, *size);

  this->unlink_file_and_test(file_path);
}

// 5. Create File - O_CREAT | O_TRUNC | O_WRONLY
TYPED_TEST(fuse_test, create_can_create_with_truncate_file_wo) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_path(file_name);

  auto handle =
      open(file_path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  auto size = utils::file::file{file_path}.size();
  EXPECT_TRUE(size.has_value());
  EXPECT_EQ(0U, *size);

  this->unlink_file_and_test(file_path);
}

// 6. Create File - O_CREAT | O_TRUNC | O_RDWR
TYPED_TEST(fuse_test, create_can_create_with_truncate_file_rw) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_path(file_name);

  auto handle =
      open(file_path.c_str(), O_CREAT | O_TRUNC | O_RDWR, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  auto size = utils::file::file{file_path}.size();
  EXPECT_TRUE(size.has_value());
  EXPECT_EQ(0U, *size);

  this->unlink_file_and_test(file_path);
}

// 7. Create File - O_CREAT | O_APPEND
TYPED_TEST(fuse_test, create_can_create_file_for_append) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_path(file_name);

  auto handle = open(file_path.c_str(), O_CREAT | O_APPEND, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 8. Create File - O_CREAT | O_APPEND | O_WRONLY
TYPED_TEST(fuse_test, create_can_create_file_for_append_wo) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_path(file_name);

  auto handle =
      open(file_path.c_str(), O_CREAT | O_APPEND | O_WRONLY, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 9. Create File - O_CREAT | O_EXCL | O_WRONLY (file does not exist)
TYPED_TEST(fuse_test, create_can_create_file_excl_wo) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_path(file_name);

  auto handle =
      open(file_path.c_str(), O_CREAT | O_EXCL | O_WRONLY, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 10. Create File - O_CREAT | O_EXCL | O_RDWR (file does not exist)
TYPED_TEST(fuse_test, create_can_create_file_excl_rw) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_path(file_name);

  auto handle = open(file_path.c_str(), O_CREAT | O_EXCL | O_RDWR, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 11. Create File - O_CREAT | O_EXCL (file does not exist)
TYPED_TEST(fuse_test, create_can_create_file_excl) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_path(file_name);

  auto handle = open(file_path.c_str(), O_CREAT | O_EXCL, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 1. Open Existing File - O_RDONLY
TYPED_TEST(fuse_test, create_can_open_existing_file_ro) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);

  auto handle = open(file_path.c_str(), O_RDONLY, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 2. Open Existing File - O_WRONLY
TYPED_TEST(fuse_test, create_can_open_existing_file_wo) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);

  auto handle = open(file_path.c_str(), O_WRONLY, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 3. Open Existing File - O_RDWR
TYPED_TEST(fuse_test, create_can_open_existing_file_rw) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);

  auto handle = open(file_path.c_str(), O_RDWR, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 4. Open Existing File - O_APPEND
TYPED_TEST(fuse_test, create_can_open_existing_file_for_append) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);

  auto handle = open(file_path.c_str(), O_APPEND, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 5. Open Existing File - O_APPEND | O_WRONLY
TYPED_TEST(fuse_test, create_can_open_existing_file_for_append_wo) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);

  auto handle = open(file_path.c_str(), O_APPEND | O_WRONLY, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 6. Open Existing File - O_APPEND | O_RDWR
TYPED_TEST(fuse_test, create_can_open_existing_file_for_append_rw) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);

  auto handle = open(file_path.c_str(), O_APPEND | O_RDWR, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 7. Open Existing File - O_TRUNC | O_WRONLY
TYPED_TEST(fuse_test, create_can_open_and_truncate_existing_file_wo) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);
  EXPECT_EQ(0, truncate(file_path.c_str(), 24U));

  auto size = utils::file::file{file_path}.size();
  EXPECT_TRUE(size.has_value());
  EXPECT_EQ(24U, *size);

  auto handle = open(file_path.c_str(), O_TRUNC | O_WRONLY, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  size = utils::file::file{file_path}.size();
  EXPECT_TRUE(size.has_value());
  EXPECT_EQ(0U, *size);

  this->unlink_file_and_test(file_path);
}

// 8. Open Existing File - O_TRUNC | O_RDWR
TYPED_TEST(fuse_test, create_can_open_and_truncate_existing_file_rw) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);
  EXPECT_EQ(0, truncate(file_path.c_str(), 24U));

  auto size = utils::file::file{file_path}.size();
  EXPECT_TRUE(size.has_value());
  EXPECT_EQ(24U, *size);

  auto handle = open(file_path.c_str(), O_TRUNC | O_RDWR, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  size = utils::file::file{file_path}.size();
  EXPECT_TRUE(size.has_value());
  EXPECT_EQ(0U, *size);

  this->unlink_file_and_test(file_path);
}

// 9. Open Existing File - O_TRUNC
TYPED_TEST(fuse_test, create_can_open_and_truncate_existing_file) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);
  EXPECT_EQ(0, truncate(file_path.c_str(), 24U));

  auto size = utils::file::file{file_path}.size();
  EXPECT_TRUE(size.has_value());
  EXPECT_EQ(24U, *size);

  auto handle = open(file_path.c_str(), O_TRUNC, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  size = utils::file::file{file_path}.size();
  EXPECT_TRUE(size.has_value());
  EXPECT_EQ(0U, *size);

  this->unlink_file_and_test(file_path);
}

// 10. Open Existing File - O_EXCL | O_WRONLY (file does not exist)
TYPED_TEST(fuse_test, create_can_open_existing_file_with_excl_wr) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);

  auto handle = open(file_path.c_str(), O_EXCL | O_WRONLY, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

// 11. Open Existing File - O_EXCL | O_RDWR (file does not exist)
TYPED_TEST(fuse_test, create_can_open_existing_file_with_excl_rw) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);

  auto handle = open(file_path.c_str(), O_EXCL | O_RDWR, ACCESSPERMS);
  EXPECT_LE(1, handle);
  close(handle);

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test, create_fails_with_excl_if_path_is_directory) {
  std::array<int, 3U> ops{
      O_CREAT | O_EXCL,
      O_CREAT | O_EXCL | O_RDWR,
      O_CREAT | O_EXCL | O_WRONLY,
  };

  std::string dir_name{"create_test"};
  auto dir_path = this->create_directory_and_test(dir_name);

  for (auto &&flags : ops) {
    auto handle = open(dir_path.c_str(), flags, ACCESSPERMS);
    EXPECT_EQ(-1, handle);

    EXPECT_EQ(EEXIST, errno);
  }

  this->rmdir_and_test(dir_path);
}

TYPED_TEST(fuse_test, create_fails_with_excl_if_file_exists) {
  std::array<int, 3U> ops{
      O_CREAT | O_EXCL,
      O_CREAT | O_EXCL | O_RDWR,
      O_CREAT | O_EXCL | O_WRONLY,
  };

  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);

  for (auto &&flags : ops) {
    auto handle = open(file_path.c_str(), flags, ACCESSPERMS);
    EXPECT_EQ(-1, handle);

    EXPECT_EQ(EEXIST, errno);
  }

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test, create_fails_if_path_is_directory) {
  std::array<int, 7U> ops{
      O_CREAT | O_APPEND,
      O_CREAT | O_RDWR,
      O_CREAT | O_TRUNC | O_RDWR,
      O_CREAT | O_TRUNC | O_WRONLY,
      O_CREAT | O_TRUNC,
      O_CREAT | O_WRONLY,
      O_CREAT,
  };

  std::string dir_name{"create_test"};
  auto dir_path = this->create_directory_and_test(dir_name);

  for (auto &&flags : ops) {
    auto handle = open(dir_path.c_str(), flags, ACCESSPERMS);
    EXPECT_EQ(-1, handle);

    EXPECT_EQ(EISDIR, errno);
  }

  this->rmdir_and_test(dir_path);
}

TYPED_TEST(fuse_test, create_fails_if_parent_path_does_not_exist) {
  std::array<int, 10U> ops{
      O_CREAT | O_APPEND,
      O_CREAT | O_EXCL,
      O_CREAT | O_EXCL | O_RDWR,
      O_CREAT | O_EXCL | O_WRONLY,
      O_CREAT | O_RDWR,
      O_CREAT | O_TRUNC | O_RDWR,
      O_CREAT | O_TRUNC | O_WRONLY,
      O_CREAT | O_TRUNC,
      O_CREAT | O_WRONLY,
      O_CREAT,
  };

  std::string file_name{"no_dir/create_test"};
  auto file_path = this->create_file_path(file_name);

  for (auto &&flags : ops) {
    auto handle = open(file_path.c_str(), flags, ACCESSPERMS);
    EXPECT_EQ(-1, handle);

    EXPECT_EQ(ENOENT, errno);
  }
}

TYPED_TEST(fuse_test, create_fails_if_invalid) {
  std::array<int, 1U> ops{
      O_CREAT | O_TRUNC | O_APPEND,
  };

  std::string file_name{"create_test"};
  auto file_path = this->create_file_path(file_name);

  for (auto &&flags : ops) {
    auto handle = open(file_path.c_str(), flags, ACCESSPERMS);
    EXPECT_EQ(-1, handle);

    EXPECT_EQ(EINVAL, errno);
  }
}

TYPED_TEST(fuse_test, create_open_fails_if_path_is_directory) {
  std::array<int, 9U> ops{
      O_APPEND,        O_EXCL | O_WRONLY,   O_RDWR | O_APPEND,
      O_RDWR | O_EXCL, O_RDWR | O_TRUNC,    O_RDWR,
      O_TRUNC,         O_WRONLY | O_APPEND, O_WRONLY,
  };

  std::string dir_name{"create_test"};
  auto dir_path = this->create_directory_and_test(dir_name);

  for (auto &&flags : ops) {
    auto handle = open(dir_path.c_str(), flags);
    EXPECT_EQ(-1, handle);
    if (handle != -1) {
      std::cout << std::oct << flags << std::endl;
    }
    EXPECT_EQ(EISDIR, errno);
  }

  this->rmdir_and_test(dir_path);
}

TYPED_TEST(fuse_test, create_open_fails_if_path_does_not_exist) {
  std::array<int, 10U> ops{
      O_APPEND,
      O_EXCL | O_WRONLY,
      O_EXCL,
      O_RDWR | O_APPEND,
      O_RDWR | O_EXCL,
      O_RDWR | O_TRUNC,
      O_RDWR,
      O_TRUNC,
      O_WRONLY | O_APPEND,
      O_WRONLY,
  };

  std::string file_name{"create_test"};
  auto file_path = this->create_file_path(file_name);

  for (auto &&flags : ops) {
    auto handle = open(file_path.c_str(), flags);
    EXPECT_EQ(-1, handle);
    EXPECT_EQ(ENOENT, errno);
  }
}
} // namespace repertory

#endif // !defined(_WIN32)
