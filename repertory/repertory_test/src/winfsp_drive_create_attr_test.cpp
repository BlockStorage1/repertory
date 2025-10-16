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
#include "fixtures/drive_fixture.hpp"

namespace repertory {
TYPED_TEST_SUITE(winfsp_test, platform_provider_types);

TYPED_TEST(winfsp_test, cr8_attr_can_create_new_file_with_normal_attribute) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_1"}),
  };

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  auto attr = ::GetFileAttributesA(file_path.c_str());
  EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE, attr);

  EXPECT_TRUE(::DeleteFileA(file_path.c_str()));
}

TYPED_TEST(winfsp_test, cr8_attr_can_create_new_file_with_read_only_attribute) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_1"}),
  };

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_READONLY, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  auto attr = ::GetFileAttributesA(file_path.c_str());
  EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY, attr);

  EXPECT_TRUE(::SetFileAttributesA(file_path.c_str(), FILE_ATTRIBUTE_NORMAL));

  EXPECT_TRUE(::DeleteFileA(file_path.c_str()));
}

// TYPED_TEST(winfsp_test, cr8_attr_can_create_new_file_with_system_attribute) {
//   auto file_path{
//       utils::path::combine(this->mount_location, {"test_file_1"}),
//   };
//
//   auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ |
//   GENERIC_WRITE,
//                               FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
//                               CREATE_NEW, FILE_ATTRIBUTE_SYSTEM, nullptr);
//   ASSERT_NE(INVALID_HANDLE_VALUE, handle);
//   ::CloseHandle(handle);
//
//   auto attr = ::GetFileAttributesA(file_path.c_str());
//   EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_SYSTEM, attr);
//
//   EXPECT_TRUE(::SetFileAttributesA(file_path.c_str(),
//   FILE_ATTRIBUTE_NORMAL));
//
//   EXPECT_TRUE(::DeleteFileA(file_path.c_str()));
// }

TYPED_TEST(winfsp_test, cr8_attr_can_create_new_file_with_hidden_attribute) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_1"}),
  };

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_HIDDEN, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  auto attr = ::GetFileAttributesA(file_path.c_str());
  EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN, attr);

  EXPECT_TRUE(::SetFileAttributesA(file_path.c_str(), FILE_ATTRIBUTE_NORMAL));

  EXPECT_TRUE(::DeleteFileA(file_path.c_str()));
}

TYPED_TEST(winfsp_test, cr8_attr_can_create_always_file_with_normal_attribute) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_1"}),
  };

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  auto attr = ::GetFileAttributesA(file_path.c_str());
  EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE, attr);

  EXPECT_TRUE(::DeleteFileA(file_path.c_str()));
}

TYPED_TEST(winfsp_test, cr8_attr_can_create_file_with_read_only_attribute) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_1"}),
  };

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_READONLY, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  auto attr = ::GetFileAttributesA(file_path.c_str());
  EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY, attr);

  EXPECT_TRUE(::SetFileAttributesA(file_path.c_str(), FILE_ATTRIBUTE_NORMAL));

  EXPECT_TRUE(::DeleteFileA(file_path.c_str()));
}

// TYPED_TEST(winfsp_test,
// cr8_attr_can_create_always_file_with_system_attribute) {
//   auto file_path{
//       utils::path::combine(this->mount_location, {"test_file_1"}),
//   };
//
//   auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ |
//   GENERIC_WRITE,
//                               FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
//                               CREATE_ALWAYS, FILE_ATTRIBUTE_SYSTEM, nullptr);
//   ASSERT_NE(INVALID_HANDLE_VALUE, handle);
//   ::CloseHandle(handle);
//
//   auto attr = ::GetFileAttributesA(file_path.c_str());
//   EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_SYSTEM, attr);
//
//   EXPECT_TRUE(::SetFileAttributesA(file_path.c_str(),
//   FILE_ATTRIBUTE_NORMAL));
//
//   EXPECT_TRUE(::DeleteFileA(file_path.c_str()));
// }

TYPED_TEST(winfsp_test, cr8_attr_can_create_always_file_with_hidden_attribute) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_1"}),
  };

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  auto attr = ::GetFileAttributesA(file_path.c_str());
  EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN, attr);

  EXPECT_TRUE(::SetFileAttributesA(file_path.c_str(), FILE_ATTRIBUTE_NORMAL));

  EXPECT_TRUE(::DeleteFileA(file_path.c_str()));
}

TYPED_TEST(winfsp_test, cr8_attr_can_handle_read_only_directory) {
  auto dir_path{
      utils::path::combine(this->mount_location, {"test_dir_1"}),
  };
  auto file_path{
      utils::path::combine(dir_path, {"test_file_1"}),
  };
  EXPECT_TRUE(::CreateDirectoryA(dir_path.c_str(), nullptr));

  EXPECT_TRUE(::SetFileAttributesA(
      dir_path.c_str(), FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY));

  auto attr = ::GetFileAttributesA(dir_path.c_str());
  EXPECT_EQ((FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY), attr);

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  EXPECT_TRUE(::DeleteFileA(file_path.c_str()));

  EXPECT_FALSE(::RemoveDirectoryA(dir_path.c_str()));
  EXPECT_EQ(ERROR_ACCESS_DENIED, ::GetLastError());

  EXPECT_TRUE(::SetFileAttributesA(dir_path.c_str(), FILE_ATTRIBUTE_DIRECTORY));

  attr = ::GetFileAttributesA(dir_path.c_str());
  EXPECT_EQ(FILE_ATTRIBUTE_DIRECTORY, attr);

  EXPECT_TRUE(::RemoveDirectoryA(dir_path.c_str()));
}
} // namespace repertory

#endif // defined(_WIN32)
