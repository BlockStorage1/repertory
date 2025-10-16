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

TYPED_TEST(winfsp_test, delete_directory_fails_if_directory_not_empty) {
  auto dir_path{
      utils::path::combine(this->mount_location, {"test_dir_3"}),
  };

  auto file_path{
      utils::path::combine(dir_path, {"test_file_3"}),
  };

  ASSERT_TRUE(::CreateDirectoryA(dir_path.c_str(), nullptr));

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  EXPECT_FALSE(::RemoveDirectoryA(dir_path.c_str()));
  EXPECT_EQ(ERROR_DIR_NOT_EMPTY, ::GetLastError());

  EXPECT_TRUE(::DeleteFileA(file_path.c_str()));
  EXPECT_FALSE(::DeleteFileA(file_path.c_str()));
  EXPECT_EQ(ERROR_FILE_NOT_FOUND, ::GetLastError());

  EXPECT_TRUE(::RemoveDirectoryA(dir_path.c_str()));
  EXPECT_FALSE(::RemoveDirectoryA(dir_path.c_str()));
  EXPECT_EQ(ERROR_FILE_NOT_FOUND, ::GetLastError());
}

TYPED_TEST(winfsp_test,
           delete_read_file_attributes_fails_if_delete_is_pending) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_3"}),
  };

  auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  ::CloseHandle(handle);

  handle = ::CreateFileA(file_path.c_str(), DELETE, FILE_SHARE_DELETE, nullptr,
                         OPEN_EXISTING, 0, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  typedef struct {
    BOOLEAN Disposition;
  } MY_FILE_DISPOSITION_INFO;

  MY_FILE_DISPOSITION_INFO disp_info{TRUE};
  EXPECT_TRUE(::SetFileInformationByHandle(handle, FileDispositionInfo,
                                           &disp_info, sizeof disp_info));
  auto handle2 = ::CreateFileA(file_path.c_str(), FILE_READ_ATTRIBUTES, 0,
                               nullptr, OPEN_EXISTING, 0, nullptr);
  EXPECT_EQ(INVALID_HANDLE_VALUE, handle2);
  EXPECT_EQ(ERROR_ACCESS_DENIED, ::GetLastError());

  ::CloseHandle(handle);

  EXPECT_FALSE(::DeleteFileA(file_path.c_str()));
  EXPECT_EQ(ERROR_FILE_NOT_FOUND, ::GetLastError());
}

TYPED_TEST(winfsp_test, delete_can_handle_mmap_after_file_deletion) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_3"}),
  };

  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  SYSTEM_INFO sys_info{};
  ::GetSystemInfo(&sys_info);

  auto *mapping =
      ::CreateFileMappingA(handle, nullptr, PAGE_READWRITE, 0,
                           sys_info.dwAllocationGranularity, nullptr);
  EXPECT_TRUE(mapping != nullptr);
  EXPECT_TRUE(::CloseHandle(handle));

  handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_EXISTING, 0, nullptr);
  EXPECT_EQ(INVALID_HANDLE_VALUE, handle);
  EXPECT_EQ(ERROR_FILE_NOT_FOUND, ::GetLastError());

  EXPECT_TRUE(::CloseHandle(mapping));
}

TYPED_TEST(winfsp_test, delete_can_delete_after_mapping) {
  auto dir_path{
      utils::path::combine(this->mount_location, {"test_dir_3"}),
  };
  auto file_path{
      utils::path::combine(dir_path, {"test_file_3"}),
  };
  auto file_path2{
      utils::path::combine(dir_path, {"test_file2_3"}),
  };

  ASSERT_TRUE(::CreateDirectoryA(dir_path.c_str(), nullptr));

  auto seed = static_cast<unsigned>(std::time(nullptr));
  srand(seed);

  SYSTEM_INFO sys_info{};
  ::GetSystemInfo(&sys_info);

  {
    auto handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    ASSERT_NE(INVALID_HANDLE_VALUE, handle);

    auto *mapping =
        ::CreateFileMappingA(handle, nullptr, PAGE_READWRITE, 0,
                             16U * sys_info.dwAllocationGranularity, nullptr);
    EXPECT_TRUE(::CloseHandle(handle));
    ASSERT_TRUE(mapping != nullptr);

    auto *view = ::MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    ASSERT_TRUE(view != nullptr);

    for (PUINT8 ptr = reinterpret_cast<PUINT8>(view),
                end = ptr + 16U * sys_info.dwAllocationGranularity;
         end > ptr; ++ptr) {
      *ptr = rand() & 0xFF;
    }

    EXPECT_TRUE(::UnmapViewOfFile(view));
    EXPECT_TRUE(::CloseHandle(mapping));
  }

  {
    auto handle =
        ::CreateFileA(file_path2.c_str(), GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                      FILE_ATTRIBUTE_NORMAL, nullptr);
    ASSERT_NE(INVALID_HANDLE_VALUE, handle);

    auto mapping =
        ::CreateFileMappingA(handle, nullptr, PAGE_READWRITE, 0,
                             16U * sys_info.dwAllocationGranularity, nullptr);
    EXPECT_TRUE(::CloseHandle(handle));
    ASSERT_TRUE(mapping != nullptr);

    auto view = ::MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    EXPECT_TRUE(view != nullptr);

    for (PUINT8 ptr = reinterpret_cast<PUINT8>(view),
                end = ptr + 16U * sys_info.dwAllocationGranularity;
         end > ptr; ++ptr) {
      *ptr = rand() & 0xFF;
    }

    EXPECT_TRUE(::UnmapViewOfFile(view));
    EXPECT_TRUE(::CloseHandle(mapping));
  }

  ASSERT_TRUE(::DeleteFileA(file_path.c_str()));
  ASSERT_TRUE(::DeleteFileA(file_path2.c_str()));

  ASSERT_TRUE(::RemoveDirectoryA(dir_path.c_str()));
  ASSERT_FALSE(::RemoveDirectoryA(dir_path.c_str()));
  ASSERT_EQ(ERROR_FILE_NOT_FOUND, ::GetLastError());
}

TYPED_TEST(winfsp_test, delete_can_delete_on_close_after_mapping) {
  auto dir_path{
      utils::path::combine(this->mount_location, {"test_dir_3"}),
  };
  auto file_path{
      utils::path::combine(dir_path, {"test_file_3"}),
  };
  auto file_path2{
      utils::path::combine(dir_path, {"test_file2_3"}),
  };

  ASSERT_TRUE(::CreateDirectoryA(dir_path.c_str(), nullptr));

  auto seed = static_cast<unsigned>(std::time(nullptr));
  srand(seed);

  SYSTEM_INFO sys_info{};
  ::GetSystemInfo(&sys_info);

  {
    auto handle = ::CreateFileA(
        file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    ASSERT_NE(INVALID_HANDLE_VALUE, handle);

    auto *mapping =
        ::CreateFileMappingA(handle, nullptr, PAGE_READWRITE, 0,
                             16U * sys_info.dwAllocationGranularity, nullptr);
    EXPECT_TRUE(mapping != nullptr);

    auto *view = ::MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    EXPECT_TRUE(view != nullptr);
    for (PUINT8 ptr = reinterpret_cast<PUINT8>(view),
                end = ptr + 16U * sys_info.dwAllocationGranularity;
         end > ptr; ++ptr) {
      *ptr = rand() & 0xFF;
    }

    EXPECT_TRUE(::UnmapViewOfFile(view));
    EXPECT_TRUE(::CloseHandle(mapping));
    EXPECT_TRUE(::CloseHandle(handle));
  }

  {
    auto handle = ::CreateFileA(
        file_path2.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    ASSERT_NE(INVALID_HANDLE_VALUE, handle);

    auto *mapping =
        ::CreateFileMappingA(handle, nullptr, PAGE_READWRITE, 0,
                             16U * sys_info.dwAllocationGranularity, nullptr);
    EXPECT_TRUE(mapping != nullptr);

    auto *view = ::MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    EXPECT_TRUE(view != nullptr);

    for (PUINT8 ptr = reinterpret_cast<PUINT8>(view),
                end = ptr + 16U * sys_info.dwAllocationGranularity;
         end > ptr; ++ptr) {
      *ptr = rand() & 0xFF;
    }

    EXPECT_TRUE(::UnmapViewOfFile(view));
    EXPECT_TRUE(::CloseHandle(mapping));
    EXPECT_TRUE(::CloseHandle(handle));
  }

  ASSERT_TRUE(::RemoveDirectoryA(dir_path.c_str()));
  ASSERT_FALSE(::RemoveDirectoryA(dir_path.c_str()));
  ASSERT_EQ(ERROR_FILE_NOT_FOUND, ::GetLastError());
}
} // namespace repertory

#endif // defined(_WIN32)
