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
#if defined(_WIN32)

//
// Implemented test cases based on WinFsp tests:
// https://github.com/winfsp/winfsp/blob/v2.0/tst/winfsp-tests
//
#include "fixtures/winfsp_fixture.hpp"

namespace repertory {
TYPED_TEST_CASE(winfsp_test, winfsp_provider_types);

TYPED_TEST(winfsp_test, rename_can_rename_file_if_dest_does_not_exist) {
  if (this->current_provider == provider_type::s3) {
    return;
  }

  auto dir_path{
      utils::path::combine(this->mount_location, {"test_dir_4"}),
  };
  auto file_path{
      utils::path::combine(dir_path, {"test_file_4"}),
  };
  auto file_path2{
      utils::path::combine(dir_path, {"test_file2_4"}),
  };

  ASSERT_TRUE(::CreateDirectoryA(dir_path.c_str(), nullptr));

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  EXPECT_TRUE(::MoveFileExA(file_path.c_str(), file_path2.c_str(), 0));

  EXPECT_TRUE(::DeleteFileA(file_path2.c_str()));
  EXPECT_TRUE(::RemoveDirectoryA(dir_path.c_str()));
}

TYPED_TEST(winfsp_test, rename_fails_if_dest_exists_and_replace_is_false) {
  if (this->current_provider == provider_type::s3) {
    return;
  }

  auto dir_path{
      utils::path::combine(this->mount_location, {"test_dir_4"}),
  };
  auto file_path{
      utils::path::combine(dir_path, {"test_file_4"}),
  };
  auto file_path2{
      utils::path::combine(dir_path, {"test_file2_4"}),
  };

  ASSERT_TRUE(::CreateDirectoryA(dir_path.c_str(), nullptr));

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  EXPECT_TRUE(::MoveFileExA(file_path.c_str(), file_path2.c_str(), 0));

  EXPECT_FALSE(::MoveFileExA(file_path.c_str(), file_path2.c_str(), 0));
  EXPECT_EQ(ERROR_ALREADY_EXISTS, ::GetLastError());

  EXPECT_TRUE(::DeleteFileA(file_path2.c_str()));
  EXPECT_TRUE(::RemoveDirectoryA(dir_path.c_str()));
}

TYPED_TEST(winfsp_test, rename_succeeds_if_dest_exists_and_replace_is_true) {
  if (this->current_provider == provider_type::s3) {
    return;
  }

  auto dir_path{
      utils::path::combine(this->mount_location, {"test_dir_4"}),
  };
  auto file_path{
      utils::path::combine(dir_path, {"test_file_4"}),
  };
  auto file_path2{
      utils::path::combine(dir_path, {"test_file2_4"}),
  };

  ASSERT_TRUE(::CreateDirectoryA(dir_path.c_str(), nullptr));

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  EXPECT_TRUE(::MoveFileExA(file_path.c_str(), file_path2.c_str(), 0));

  handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  EXPECT_TRUE(::MoveFileExA(file_path.c_str(), file_path2.c_str(),
                            MOVEFILE_REPLACE_EXISTING));

  EXPECT_TRUE(::DeleteFileA(file_path2.c_str()));
  EXPECT_TRUE(::RemoveDirectoryA(dir_path.c_str()));
}

TYPED_TEST(winfsp_test, rename_can_rename_dir_if_dest_does_not_exist) {
  if (this->current_provider == provider_type::s3) {
    return;
  }

  auto dir_path{
      utils::path::combine(this->mount_location, {"test_dir_4"}),
  };
  auto dir_path2{
      utils::path::combine(this->mount_location, {"test_dir2_4"}),
  };

  ASSERT_TRUE(::CreateDirectoryA(dir_path.c_str(), nullptr));

  EXPECT_TRUE(::MoveFileExA(dir_path.c_str(), dir_path2.c_str(), 0));

  EXPECT_TRUE(::RemoveDirectoryA(dir_path2.c_str()));
}

TYPED_TEST(winfsp_test, rename_dir_fails_if_dest_exists_and_replace_is_false) {
  if (this->current_provider == provider_type::s3) {
    return;
  }

  auto dir_path{
      utils::path::combine(this->mount_location, {"test_dir_4"}),
  };
  auto dir_path2{
      utils::path::combine(this->mount_location, {"test_dir2_4"}),
  };

  ASSERT_TRUE(::CreateDirectoryA(dir_path.c_str(), nullptr));
  ASSERT_TRUE(::CreateDirectoryA(dir_path2.c_str(), nullptr));

  EXPECT_FALSE(::MoveFileExA(dir_path.c_str(), dir_path2.c_str(), 0));
  EXPECT_EQ(ERROR_ACCESS_DENIED, ::GetLastError());

  EXPECT_TRUE(::RemoveDirectoryA(dir_path.c_str()));
  EXPECT_TRUE(::RemoveDirectoryA(dir_path2.c_str()));
}

TYPED_TEST(winfsp_test, rename_dir_fails_if_dest_exists_and_replace_is_true) {
  if (this->current_provider == provider_type::s3) {
    return;
  }

  auto dir_path{
      utils::path::combine(this->mount_location, {"test_dir_4"}),
  };
  auto dir_path2{
      utils::path::combine(this->mount_location, {"test_dir2_4"}),
  };

  ASSERT_TRUE(::CreateDirectoryA(dir_path.c_str(), nullptr));
  ASSERT_TRUE(::CreateDirectoryA(dir_path2.c_str(), nullptr));

  EXPECT_FALSE(::MoveFileExA(dir_path.c_str(), dir_path2.c_str(),
                             MOVEFILE_REPLACE_EXISTING));
  EXPECT_EQ(ERROR_ACCESS_DENIED, ::GetLastError());

  EXPECT_TRUE(::RemoveDirectoryA(dir_path.c_str()));
  EXPECT_TRUE(::RemoveDirectoryA(dir_path2.c_str()));
}

TYPED_TEST(winfsp_test,
           rename_dir_fails_directory_is_not_empty_and_replace_is_false) {
  if (this->current_provider == provider_type::s3) {
    return;
  }

  auto dir_path{
      utils::path::combine(this->mount_location, {"test_dir_4"}),
  };
  auto dir_path2{
      utils::path::combine(this->mount_location, {"test_dir2_4"}),
  };
  auto file_path{
      utils::path::combine(dir_path, {"test_file_4"}),
  };

  ASSERT_TRUE(::CreateDirectoryA(dir_path.c_str(), nullptr));

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  EXPECT_FALSE(::MoveFileExA(dir_path.c_str(), dir_path2.c_str(), 0));
  EXPECT_EQ(ERROR_ACCESS_DENIED, ::GetLastError());

  EXPECT_TRUE(::DeleteFileA(file_path.c_str()));
  EXPECT_TRUE(::RemoveDirectoryA(dir_path.c_str()));
}

TYPED_TEST(winfsp_test,
           rename_dir_fails_directory_is_not_empty_and_replace_is_true) {
  if (this->current_provider == provider_type::s3) {
    return;
  }

  auto dir_path{
      utils::path::combine(this->mount_location, {"test_dir_4"}),
  };
  auto dir_path2{
      utils::path::combine(this->mount_location, {"test_dir2_4"}),
  };
  auto file_path{
      utils::path::combine(dir_path, {"test_file_4"}),
  };

  ASSERT_TRUE(::CreateDirectoryA(dir_path.c_str(), nullptr));

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  EXPECT_FALSE(::MoveFileExA(dir_path.c_str(), dir_path2.c_str(),
                             MOVEFILE_REPLACE_EXISTING));
  EXPECT_EQ(ERROR_ACCESS_DENIED, ::GetLastError());

  EXPECT_TRUE(::DeleteFileA(file_path.c_str()));
  EXPECT_TRUE(::RemoveDirectoryA(dir_path.c_str()));
}
} // namespace repertory

#endif // defined(_WIN32)
