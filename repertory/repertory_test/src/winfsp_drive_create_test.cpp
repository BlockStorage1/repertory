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

TYPED_TEST(winfsp_test, cr8_file_can_create_file) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_0"}),
  };
  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);
}

TYPED_TEST(winfsp_test, cr8_file_create_new_fails_when_file_exists) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_0"}),
  };
  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_EQ(INVALID_HANDLE_VALUE, handle);
  EXPECT_EQ(ERROR_FILE_EXISTS, ::GetLastError());
}

TYPED_TEST(winfsp_test, cr8_file_can_open_existing_file) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_0"}),
  };
  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);
}

TYPED_TEST(winfsp_test, cr8_file_create_always_succeeds_when_file_exists) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_0"}),
  };
  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  // EXPECT file_size is 0
  ::CloseHandle(handle);
}

TYPED_TEST(winfsp_test, cr8_file_can_delete_file_after_close) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_0"}),
  };
  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                    FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_EQ(INVALID_HANDLE_VALUE, handle);
  EXPECT_EQ(ERROR_FILE_NOT_FOUND, ::GetLastError());

  // EXPECT file not found
}

TYPED_TEST(winfsp_test,
           cr8_file_cannot_create_files_with_invalid_characters_in_path) {
  for (const auto &invalid_char : std::array<std::string, 7U>{
           {"*", ":", "<", ">", "?", "|", "\""},
       }) {
    auto handle = ::CreateFileA(
        (this->mount_location + "\\" + invalid_char + "\\test_file_0").c_str(),
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    ASSERT_EQ(INVALID_HANDLE_VALUE, handle);
    if (handle != INVALID_HANDLE_VALUE) {
      std::cout << "char: " << invalid_char << std::endl;
    }
    EXPECT_EQ(ERROR_INVALID_NAME, ::GetLastError());
  }
}

TYPED_TEST(winfsp_test,
           cr8_file_cannot_create_stream_files_with_extra_component_in_path) {
  auto file_path{
      utils::path::combine(this->mount_location,
                           {
                               "test_file_0:test",
                               "moose",
                           }),
  };
  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_EQ(INVALID_HANDLE_VALUE, handle);
  EXPECT_EQ(ERROR_INVALID_NAME, ::GetLastError());
}

TYPED_TEST(winfsp_test, cr8_file_can_create_directory) {
  auto dir_path{
      utils::path::combine(this->mount_location,
                           {
                               "test_dir_0",
                           }),
  };
  EXPECT_TRUE(::CreateDirectoryA(dir_path.c_str(), nullptr));
  EXPECT_FALSE(::CreateDirectoryA(dir_path.c_str(), nullptr));
}

TYPED_TEST(winfsp_test, cr8_file_directory_delete_fails_if_not_empty) {
  auto dir_path{
      utils::path::combine(this->mount_location,
                           {
                               "test_dir_0",
                           }),
  };
  auto file_path{
      utils::path::combine(dir_path,
                           {
                               "test_file_0",
                           }),
  };

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  handle = ::CreateFileA(
      dir_path.c_str(), GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  handle = ::CreateFileA(
      dir_path.c_str(), GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  handle = ::CreateFileA(
      dir_path.c_str(), GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_EQ(INVALID_HANDLE_VALUE, handle);
  EXPECT_EQ(ERROR_FILE_NOT_FOUND, ::GetLastError());
}
} // namespace repertory

#endif // defined(_WIN32)
