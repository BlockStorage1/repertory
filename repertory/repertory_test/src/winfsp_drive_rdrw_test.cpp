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

static void test_file(auto &&mount_location, auto &&file_path, auto &&flags) {
  SYSTEM_INFO sys_info{};
  ::GetSystemInfo(&sys_info);

  DWORD bytes_per_sector{};
  DWORD free_clusters{};
  DWORD sectors_per_cluster{};
  DWORD total_clusters{};
  EXPECT_TRUE(::GetDiskFreeSpaceA(mount_location.c_str(), &sectors_per_cluster,
                                  &bytes_per_sector, &free_clusters,
                                  &total_clusters));
  const auto buffer_size = 16U * sys_info.dwPageSize;
  auto write_buffer = utils::generate_secure_random<data_buffer>(buffer_size);

  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | flags, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  auto pointer = ::SetFilePointer(handle, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);

  DWORD bytes_written{};
  EXPECT_TRUE(::WriteFile(handle, write_buffer.data(), bytes_per_sector,
                          &bytes_written, nullptr));
  EXPECT_EQ(bytes_per_sector, bytes_written);
  EXPECT_EQ(pointer + bytes_written,
            ::SetFilePointer(handle, 0, 0, FILE_CURRENT));

  pointer = ::SetFilePointer(handle, 2U * bytes_per_sector, 0, FILE_BEGIN);
  EXPECT_EQ(2U * bytes_per_sector, pointer);
  EXPECT_TRUE(::WriteFile(handle, write_buffer.data(), bytes_per_sector,
                          &bytes_written, 0));
  EXPECT_EQ(bytes_per_sector, bytes_written);
  EXPECT_EQ(pointer + bytes_written,
            ::SetFilePointer(handle, 0, 0, FILE_CURRENT));

  pointer = ::SetFilePointer(handle, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);
  data_buffer read_buffer{};
  read_buffer.resize(buffer_size);
  DWORD bytes_read{};
  EXPECT_TRUE(::ReadFile(handle, read_buffer.data(), bytes_per_sector,
                         &bytes_read, nullptr));
  EXPECT_EQ(bytes_per_sector, bytes_read);
  EXPECT_EQ(pointer + bytes_read, ::SetFilePointer(handle, 0, 0, FILE_CURRENT));
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  for (auto idx = 0; idx < 2; ++idx) {
    read_buffer.clear();
    read_buffer.resize(buffer_size);
    pointer = ::SetFilePointer(handle, 2U * bytes_per_sector, 0, FILE_BEGIN);
    EXPECT_EQ(2U * bytes_per_sector, pointer);
    EXPECT_TRUE(::ReadFile(handle, read_buffer.data(), bytes_per_sector,
                           &bytes_read, 0));
    EXPECT_EQ(bytes_per_sector, bytes_read);
    EXPECT_EQ(pointer + bytes_read,
              ::SetFilePointer(handle, 0, 0, FILE_CURRENT));
    EXPECT_EQ(0,
              std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));
  }

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  pointer = ::SetFilePointer(handle, 3U * bytes_per_sector, 0, FILE_BEGIN);
  EXPECT_EQ(3U * bytes_per_sector, pointer);
  EXPECT_TRUE(::ReadFile(handle, read_buffer.data(), bytes_per_sector,
                         &bytes_read, nullptr));
  EXPECT_EQ(0U, bytes_read);
  EXPECT_EQ(pointer + bytes_read, ::SetFilePointer(handle, 0, 0, FILE_CURRENT));
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  pointer = ::SetFilePointer(handle, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);
  EXPECT_TRUE(::WriteFile(handle, write_buffer.data(), 2U * sys_info.dwPageSize,
                          &bytes_written, nullptr));
  EXPECT_EQ(2U * sys_info.dwPageSize, bytes_written);
  EXPECT_EQ(pointer + bytes_written,
            ::SetFilePointer(handle, 0, 0, FILE_CURRENT));

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  pointer = ::SetFilePointer(handle, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);
  EXPECT_TRUE(::ReadFile(handle, read_buffer.data(), 2U * sys_info.dwPageSize,
                         &bytes_read, 0U));
  EXPECT_EQ(2U * sys_info.dwPageSize, bytes_read);
  EXPECT_EQ(pointer + bytes_read, ::SetFilePointer(handle, 0, 0, FILE_CURRENT));
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  write_buffer = utils::generate_secure_random<data_buffer>(buffer_size);

  pointer = ::SetFilePointer(handle, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);
  EXPECT_TRUE(::WriteFile(handle, write_buffer.data(), 2U * sys_info.dwPageSize,
                          &bytes_written, nullptr));
  EXPECT_EQ(2U * sys_info.dwPageSize, bytes_written);
  EXPECT_EQ(pointer + bytes_written,
            ::SetFilePointer(handle, 0, 0, FILE_CURRENT));

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  pointer = ::SetFilePointer(handle, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);
  EXPECT_TRUE(::ReadFile(handle, read_buffer.data(), 2U * sys_info.dwPageSize,
                         &bytes_read, nullptr));
  EXPECT_EQ(2U * sys_info.dwPageSize, bytes_read);
  EXPECT_EQ(pointer + bytes_read, ::SetFilePointer(handle, 0, 0, FILE_CURRENT));
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  pointer = ::SetFilePointer(handle, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);
  EXPECT_TRUE(::WriteFile(handle, write_buffer.data(),
                          2U * sys_info.dwPageSize + bytes_per_sector,
                          &bytes_written, nullptr));
  EXPECT_EQ(2U * sys_info.dwPageSize + bytes_per_sector, bytes_written);
  EXPECT_EQ(pointer + bytes_written,
            ::SetFilePointer(handle, 0, 0, FILE_CURRENT));

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  pointer = ::SetFilePointer(handle, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);
  EXPECT_TRUE(::ReadFile(handle, read_buffer.data(),
                         2U * sys_info.dwPageSize + bytes_per_sector,
                         &bytes_read, nullptr));
  EXPECT_EQ(2U * sys_info.dwPageSize + bytes_per_sector, bytes_read);
  EXPECT_EQ(pointer + bytes_read, ::SetFilePointer(handle, 0, 0, FILE_CURRENT));
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  EXPECT_TRUE(::CloseHandle(handle));

  handle = ::CreateFileA(
      file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL | flags | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  pointer = ::SetFilePointer(handle, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);
  EXPECT_TRUE(::ReadFile(handle, read_buffer.data(),
                         2U * sys_info.dwPageSize + bytes_per_sector,
                         &bytes_read, nullptr));
  EXPECT_EQ(2U * sys_info.dwPageSize + bytes_per_sector, bytes_read);
  EXPECT_EQ(pointer + bytes_read, ::SetFilePointer(handle, 0, 0, FILE_CURRENT));
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  EXPECT_TRUE(::CloseHandle(handle));

  handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_EXISTING, 0, nullptr);
  EXPECT_EQ(INVALID_HANDLE_VALUE, handle);
  EXPECT_EQ(ERROR_FILE_NOT_FOUND, ::GetLastError());
}

static void test_append_file(auto &&mount_location, auto &&file_path,
                             auto &&flags, bool should_fail = false) {
  SYSTEM_INFO sys_info{};
  ::GetSystemInfo(&sys_info);

  DWORD bytes_per_sector{};
  DWORD free_clusters{};
  DWORD sectors_per_cluster{};
  DWORD total_clusters{};
  EXPECT_TRUE(::GetDiskFreeSpaceA(mount_location.c_str(), &sectors_per_cluster,
                                  &bytes_per_sector, &free_clusters,
                                  &total_clusters));
  const auto buffer_size = 16U * sys_info.dwPageSize;
  auto write_buffer = utils::generate_secure_random<data_buffer>(buffer_size);

  auto handle =
      CreateFileA(file_path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                  CREATE_NEW, FILE_ATTRIBUTE_NORMAL | flags, nullptr);
  if (should_fail) {
    EXPECT_EQ(INVALID_HANDLE_VALUE, handle);
    return;
  }

  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  DWORD bytes_written{};
  EXPECT_TRUE(::WriteFile(handle, write_buffer.data(), bytes_per_sector,
                          &bytes_written, nullptr));
  EXPECT_EQ(bytes_per_sector, bytes_written);

  EXPECT_TRUE(::WriteFile(handle, write_buffer.data() + bytes_per_sector,
                          bytes_per_sector, &bytes_written, nullptr));
  EXPECT_EQ(bytes_per_sector, bytes_written);

  EXPECT_TRUE(::CloseHandle(handle));

  handle = CreateFileA(
      file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL | flags | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  auto pointer = ::SetFilePointer(handle, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);

  data_buffer read_buffer{};
  read_buffer.resize(buffer_size);
  DWORD bytes_read{};
  EXPECT_TRUE(::ReadFile(handle, read_buffer.data(), 2U * bytes_per_sector,
                         &bytes_read, nullptr));
  EXPECT_EQ(2U * bytes_per_sector, bytes_read);
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  EXPECT_TRUE(::CloseHandle(handle));

  handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_EXISTING, 0, nullptr);
  EXPECT_EQ(INVALID_HANDLE_VALUE, handle);
  EXPECT_EQ(ERROR_FILE_NOT_FOUND, ::GetLastError());
}

static void test_overlapped_file(auto &&mount_location, auto &&file_path,
                                 auto &&flags) {
  SYSTEM_INFO sys_info{};
  ::GetSystemInfo(&sys_info);

  DWORD bytes_per_sector{};
  DWORD free_clusters{};
  DWORD sectors_per_cluster{};
  DWORD total_clusters{};
  EXPECT_TRUE(::GetDiskFreeSpaceA(mount_location.c_str(), &sectors_per_cluster,
                                  &bytes_per_sector, &free_clusters,
                                  &total_clusters));
  const auto buffer_size = 16U * sys_info.dwPageSize;
  auto write_buffer = utils::generate_secure_random<data_buffer>(buffer_size);

  OVERLAPPED overlapped{};
  overlapped.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
  ASSERT_NE(nullptr, overlapped.hEvent);

  auto handle = ::CreateFileA(
      file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
      FILE_ATTRIBUTE_NORMAL | flags | FILE_FLAG_OVERLAPPED, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  overlapped.Offset = 0U;
  DWORD bytes_written{};
  auto ret = ::WriteFile(handle, write_buffer.data(), bytes_per_sector,
                         &bytes_written, &overlapped);
  EXPECT_TRUE(ret || ERROR_IO_PENDING == ::GetLastError());
  EXPECT_TRUE(::GetOverlappedResult(handle, &overlapped, &bytes_written, TRUE));
  EXPECT_EQ(bytes_per_sector, bytes_written);

  overlapped.Offset = 2U * bytes_per_sector;
  ret = ::WriteFile(handle, write_buffer.data(), bytes_per_sector,
                    &bytes_written, &overlapped);
  EXPECT_TRUE(ret || ERROR_IO_PENDING == ::GetLastError());
  EXPECT_TRUE(::GetOverlappedResult(handle, &overlapped, &bytes_written, TRUE));
  EXPECT_EQ(bytes_per_sector, bytes_written);

  data_buffer read_buffer{};
  read_buffer.resize(buffer_size);
  overlapped.Offset = 0U;
  DWORD bytes_read{};
  ret = ::ReadFile(handle, read_buffer.data(), bytes_per_sector, &bytes_read,
                   &overlapped);

  EXPECT_TRUE(ret || ERROR_IO_PENDING == ::GetLastError());
  EXPECT_TRUE(::GetOverlappedResult(handle, &overlapped, &bytes_read, TRUE));

  EXPECT_EQ(bytes_per_sector, bytes_read);
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  overlapped.Offset = 2U * bytes_per_sector;
  ret = ::ReadFile(handle, read_buffer.data(), bytes_per_sector, &bytes_read,
                   &overlapped);
  EXPECT_TRUE(ret || ERROR_IO_PENDING == ::GetLastError());
  EXPECT_TRUE(::GetOverlappedResult(handle, &overlapped, &bytes_read, TRUE));
  EXPECT_EQ(bytes_per_sector, bytes_read);
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  overlapped.Offset = 2U * bytes_per_sector;
  ret = ::ReadFile(handle, read_buffer.data(), 2U * bytes_per_sector,
                   &bytes_read, &overlapped);
  EXPECT_TRUE(ret || ERROR_IO_PENDING == ::GetLastError());
  EXPECT_TRUE(::GetOverlappedResult(handle, &overlapped, &bytes_read, TRUE));
  EXPECT_EQ(bytes_per_sector, bytes_read);
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  LARGE_INTEGER size{};
  ::GetFileSizeEx(handle, &size);

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  overlapped.Offset = 3U * bytes_per_sector;
  ret = ::ReadFile(handle, read_buffer.data(), bytes_per_sector, &bytes_read,
                   &overlapped);
  EXPECT_TRUE(ret || ERROR_IO_PENDING == ::GetLastError() ||
              ERROR_HANDLE_EOF == ::GetLastError());
  if (ERROR_HANDLE_EOF != ::GetLastError()) {
    ret = ::GetOverlappedResult(handle, &overlapped, &bytes_read, TRUE);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ERROR_HANDLE_EOF, ::GetLastError());
  }
  EXPECT_EQ(0U, bytes_read);
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  overlapped.Offset = 0U;
  ret = ::WriteFile(handle, write_buffer.data(), 2U * sys_info.dwPageSize,
                    &bytes_written, &overlapped);
  EXPECT_TRUE(ret || ERROR_IO_PENDING == ::GetLastError());
  EXPECT_TRUE(::GetOverlappedResult(handle, &overlapped, &bytes_written, TRUE));
  EXPECT_EQ(2U * sys_info.dwPageSize, bytes_written);

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  overlapped.Offset = 0U;
  ret = ::ReadFile(handle, read_buffer.data(), 2U * sys_info.dwPageSize,
                   &bytes_read, &overlapped);
  EXPECT_TRUE(ret || ERROR_IO_PENDING == ::GetLastError());
  EXPECT_TRUE(::GetOverlappedResult(handle, &overlapped, &bytes_read, TRUE));
  EXPECT_EQ(2U * sys_info.dwPageSize, bytes_read);
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  write_buffer = utils::generate_secure_random<data_buffer>(buffer_size);

  overlapped.Offset = 0U;
  ret = ::WriteFile(handle, write_buffer.data(), 2U * sys_info.dwPageSize,
                    &bytes_written, &overlapped);
  EXPECT_TRUE(ret || ERROR_IO_PENDING == ::GetLastError());
  EXPECT_TRUE(::GetOverlappedResult(handle, &overlapped, &bytes_written, TRUE));
  EXPECT_EQ(2U * sys_info.dwPageSize, bytes_written);

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  overlapped.Offset = 0U;
  ret = ::ReadFile(handle, read_buffer.data(), 2U * sys_info.dwPageSize,
                   &bytes_read, &overlapped);
  EXPECT_TRUE(ret || ERROR_IO_PENDING == ::GetLastError());
  EXPECT_TRUE(::GetOverlappedResult(handle, &overlapped, &bytes_read, TRUE));
  EXPECT_EQ(2U * sys_info.dwPageSize, bytes_read);
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  overlapped.Offset = 0U;
  ret = ::WriteFile(handle, write_buffer.data(),
                    2U * sys_info.dwPageSize + bytes_per_sector, &bytes_written,
                    &overlapped);
  EXPECT_TRUE(ret || ERROR_IO_PENDING == ::GetLastError());
  EXPECT_TRUE(::GetOverlappedResult(handle, &overlapped, &bytes_written, TRUE));
  EXPECT_EQ(2U * sys_info.dwPageSize + bytes_per_sector, bytes_written);

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  overlapped.Offset = 0U;
  ret = ::ReadFile(handle, read_buffer.data(),
                   2U * sys_info.dwPageSize + bytes_per_sector, &bytes_read,
                   &overlapped);
  EXPECT_TRUE(ret || ERROR_IO_PENDING == ::GetLastError());
  EXPECT_TRUE(::GetOverlappedResult(handle, &overlapped, &bytes_read, TRUE));
  EXPECT_EQ(2U * sys_info.dwPageSize + bytes_per_sector, bytes_read);
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  EXPECT_TRUE(::CloseHandle(handle));

  handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL | flags | FILE_FLAG_OVERLAPPED |
                             FILE_FLAG_DELETE_ON_CLOSE,
                         nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  overlapped.Offset = 0U;
  ret = ::ReadFile(handle, read_buffer.data(),
                   2U * sys_info.dwPageSize + bytes_per_sector, &bytes_read,
                   &overlapped);
  EXPECT_TRUE(ret || ERROR_IO_PENDING == ::GetLastError());
  EXPECT_TRUE(::GetOverlappedResult(handle, &overlapped, &bytes_read, TRUE));
  EXPECT_EQ(2U * sys_info.dwPageSize + bytes_per_sector, bytes_read);
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  EXPECT_TRUE(::CloseHandle(handle));

  EXPECT_TRUE(::CloseHandle(overlapped.hEvent));

  handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_EXISTING, 0, nullptr);
  EXPECT_EQ(INVALID_HANDLE_VALUE, handle);
  EXPECT_EQ(ERROR_FILE_NOT_FOUND, ::GetLastError());
}

static void test_mixed_file(auto &&mount_location, auto &&file_path) {
  SYSTEM_INFO sys_info{};
  ::GetSystemInfo(&sys_info);

  DWORD bytes_per_sector{};
  DWORD free_clusters{};
  DWORD sectors_per_cluster{};
  DWORD total_clusters{};
  EXPECT_TRUE(::GetDiskFreeSpaceA(mount_location.c_str(), &sectors_per_cluster,
                                  &bytes_per_sector, &free_clusters,
                                  &total_clusters));
  const auto buffer_size = 16U * sys_info.dwPageSize;
  auto write_buffer = utils::generate_secure_random<data_buffer>(buffer_size);

  auto handle0 = CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                             CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle0);

  auto handle1 =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle0);

  auto pointer = ::SetFilePointer(handle0, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);

  DWORD bytes_written{};
  EXPECT_TRUE(::WriteFile(handle0, write_buffer.data(), bytes_per_sector,
                          &bytes_written, nullptr));

  EXPECT_EQ(bytes_per_sector, bytes_written);
  EXPECT_EQ(pointer + bytes_per_sector,
            ::SetFilePointer(handle0, 0, 0, FILE_CURRENT));

  pointer = ::SetFilePointer(handle1, 2 * bytes_per_sector, 0, FILE_BEGIN);
  EXPECT_EQ(2U * bytes_per_sector, pointer);

  EXPECT_TRUE(::WriteFile(handle1, write_buffer.data(), bytes_per_sector,
                          &bytes_written, nullptr));
  EXPECT_EQ(bytes_per_sector, bytes_written);
  EXPECT_EQ(pointer + bytes_written,
            ::SetFilePointer(handle1, 0, 0, FILE_CURRENT));

  pointer = ::SetFilePointer(handle0, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);

  data_buffer read_buffer{};
  read_buffer.resize(buffer_size);
  DWORD bytes_read{};
  EXPECT_TRUE(::ReadFile(handle0, read_buffer.data(), bytes_per_sector,
                         &bytes_read, nullptr));
  EXPECT_EQ(bytes_per_sector, bytes_read);
  EXPECT_EQ(pointer + bytes_read,
            ::SetFilePointer(handle0, 0, 0, FILE_CURRENT));
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  pointer = ::SetFilePointer(handle1, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);
  EXPECT_TRUE(::ReadFile(handle1, read_buffer.data(), bytes_per_sector,
                         &bytes_read, nullptr));
  EXPECT_EQ(bytes_per_sector, bytes_read);
  EXPECT_EQ(pointer + bytes_read,
            ::SetFilePointer(handle1, 0, 0, FILE_CURRENT));
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  EXPECT_TRUE(::CloseHandle(handle0));

  EXPECT_TRUE(::CloseHandle(handle1));

  EXPECT_TRUE(::DeleteFileA(file_path.c_str()));

  handle0 = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                          CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle0);

  pointer = ::SetFilePointer(handle0, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);
  EXPECT_TRUE(::WriteFile(handle0, write_buffer.data(), bytes_per_sector,
                          &bytes_written, nullptr));
  EXPECT_EQ(bytes_per_sector, bytes_written);
  EXPECT_EQ(pointer + bytes_written,
            ::SetFilePointer(handle0, 0, 0, FILE_CURRENT));

  pointer = ::SetFilePointer(handle0, 2 * bytes_per_sector, 0, FILE_BEGIN);
  EXPECT_EQ(2U * bytes_per_sector, pointer);
  EXPECT_TRUE(::WriteFile(handle0, write_buffer.data(), bytes_per_sector,
                          &bytes_written, nullptr));
  EXPECT_EQ(bytes_per_sector, bytes_written);
  EXPECT_EQ(pointer + bytes_written,
            ::SetFilePointer(handle0, 0, 0, FILE_CURRENT));

  pointer = ::SetFilePointer(handle0, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  EXPECT_TRUE(::ReadFile(handle0, read_buffer.data(), bytes_per_sector,
                         &bytes_read, nullptr));
  EXPECT_EQ(bytes_per_sector, bytes_read);
  EXPECT_EQ(pointer + bytes_read,
            ::SetFilePointer(handle0, 0, 0, FILE_CURRENT));
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  EXPECT_TRUE(::CloseHandle(handle0));

  handle1 =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle0);

  pointer = ::SetFilePointer(handle1, 0, 0, FILE_BEGIN);
  EXPECT_EQ(0U, pointer);

  read_buffer.clear();
  read_buffer.resize(buffer_size);
  EXPECT_TRUE(::ReadFile(handle1, read_buffer.data(), bytes_per_sector,
                         &bytes_read, nullptr));
  EXPECT_EQ(bytes_per_sector, bytes_read);
  EXPECT_EQ(pointer + bytes_read,
            ::SetFilePointer(handle1, 0, 0, FILE_CURRENT));
  EXPECT_EQ(0,
            std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));

  EXPECT_TRUE(::CloseHandle(handle1));

  EXPECT_TRUE(::DeleteFileA(file_path.c_str()));
}

static void test_mmap_file(auto &&mount_location, auto &&file_path,
                           auto &&flags, auto &&early_close) {
  SYSTEM_INFO sys_info{};
  ::GetSystemInfo(&sys_info);

  DWORD bytes_per_sector{};
  DWORD free_clusters{};
  DWORD sectors_per_cluster{};
  DWORD total_clusters{};
  EXPECT_TRUE(::GetDiskFreeSpaceA(mount_location.c_str(), &sectors_per_cluster,
                                  &bytes_per_sector, &free_clusters,
                                  &total_clusters));
  const auto buffer_size = 16U * sys_info.dwPageSize;
  auto write_buffer = utils::generate_secure_random<data_buffer>(buffer_size);

  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL | flags, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle);

  DWORD file_size0{
      2U * sys_info.dwAllocationGranularity,
  };
  DWORD file_size1{100U};
  auto *mapping = ::CreateFileMappingA(handle, nullptr, PAGE_READWRITE, 0,
                                       file_size0 + file_size1, nullptr);
  ASSERT_NE(nullptr, mapping);

  if (early_close) {
    EXPECT_TRUE(::CloseHandle(handle));
  }

  auto *view = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  ASSERT_NE(nullptr, view);

  auto seed = static_cast<unsigned>(std::time(nullptr));
  srand(seed);
  for (PUINT8 begin = reinterpret_cast<PUINT8>(view) + file_size1 / 2U,
              end = begin + file_size0;
       end > begin; ++begin) {
    *begin = rand() & 0xFF;
  }

  EXPECT_TRUE(::UnmapViewOfFile(view));

  view = ::MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  ASSERT_NE(nullptr, view);

  srand(seed);
  for (PUINT8 begin = reinterpret_cast<PUINT8>(view) + file_size1 / 2U,
              end = begin + file_size0;
       end > begin; begin++) {
    EXPECT_EQ(*begin, (rand() & 0xFF));
  }

  EXPECT_TRUE(::UnmapViewOfFile(view));

  EXPECT_TRUE(::CloseHandle(mapping));

  if (not early_close) {
    EXPECT_TRUE(::CloseHandle(handle));
  }

  handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | flags, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle);

  mapping = ::CreateFileMappingA(handle, nullptr, PAGE_READWRITE, 0,
                                 file_size0 + file_size1, nullptr);
  ASSERT_NE(nullptr, mapping);

  if (early_close) {
    EXPECT_TRUE(::CloseHandle(handle));
  }

  view = ::MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  ASSERT_NE(nullptr, view);

  srand(seed);
  for (PUINT8 begin = reinterpret_cast<PUINT8>(view) + file_size1 / 2U,
              end = begin + file_size0;
       end > begin; ++begin) {
    EXPECT_EQ(*begin, (rand() & 0xFF));
  }

  if (not early_close) {
    auto pointer = ::SetFilePointer(handle, file_size0 / 2U, 0, FILE_BEGIN);
    EXPECT_EQ(file_size0 / 2U, pointer);

    DWORD bytes_written{};
    EXPECT_TRUE(::WriteFile(handle, write_buffer.data(), bytes_per_sector,
                            &bytes_written, nullptr));
    EXPECT_EQ(bytes_per_sector, bytes_written);
    EXPECT_EQ(pointer + bytes_written,
              ::SetFilePointer(handle, 0, 0, FILE_CURRENT));
  }

  EXPECT_TRUE(::UnmapViewOfFile(view));

  EXPECT_TRUE(::CloseHandle(mapping));

  if (not early_close) {
    EXPECT_TRUE(::CloseHandle(handle));
  }

  handle = ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | flags, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  mapping = ::CreateFileMappingA(handle, nullptr, PAGE_READWRITE, 0,
                                 file_size0 + file_size1, nullptr);
  ASSERT_NE(nullptr, mapping);

  if (early_close) {
    EXPECT_TRUE(::CloseHandle(handle));
  }

  view = ::MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  ASSERT_NE(nullptr, view);

  if (not early_close) {
    auto pointer = ::SetFilePointer(handle, file_size0 / 2U, 0, FILE_BEGIN);
    EXPECT_EQ(file_size0 / 2U, pointer);

    data_buffer read_buffer{};
    read_buffer.resize(buffer_size);
    DWORD bytes_read{};
    EXPECT_TRUE(::ReadFile(handle, read_buffer.data(), bytes_per_sector,
                           &bytes_read, nullptr));
    EXPECT_EQ(bytes_per_sector, bytes_read);
    EXPECT_EQ(pointer + bytes_read,
              ::SetFilePointer(handle, 0, 0, FILE_CURRENT));
    EXPECT_EQ(0,
              std::memcmp(write_buffer.data(), read_buffer.data(), bytes_read));
  }

  srand(seed);
  for (PUINT8 begin = reinterpret_cast<PUINT8>(view) + file_size1 / 2U,
              end = reinterpret_cast<PUINT8>(view) + file_size0 / 2U;
       end > begin; begin++) {
    EXPECT_EQ(*begin, (rand() & 0xFF));
  }

  if (not early_close) {
    EXPECT_EQ(0, std::memcmp(write_buffer.data(), view + file_size0 / 2U,
                             bytes_per_sector));
  }

  for (size_t idx = 0U; bytes_per_sector > idx; ++idx) {
    rand();
  }

  for (PUINT8
           begin = reinterpret_cast<PUINT8>(view) + file_size0 / 2U +
                   bytes_per_sector,
           end = reinterpret_cast<PUINT8>(view) + file_size1 / 2U + file_size0;
       end > begin; ++begin) {
    EXPECT_EQ(*begin, (rand() & 0xFF));
  }

  EXPECT_TRUE(::UnmapViewOfFile(view));

  EXPECT_TRUE(::CloseHandle(mapping));

  if (not early_close) {
    EXPECT_TRUE(::CloseHandle(handle));
  }

  EXPECT_TRUE(DeleteFileA(file_path.c_str()));
}

TYPED_TEST(winfsp_test, rdrw_can_read_and_write_file_no_flags) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_5"}),
  };
  test_file(this->mount_location, file_path, 0U);
}

TYPED_TEST(winfsp_test, rdrw_can_read_and_write_file_no_buffering) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_5"}),
  };
  test_file(this->mount_location, file_path, FILE_FLAG_NO_BUFFERING);
}

TYPED_TEST(winfsp_test, rdrw_can_read_and_write_file_write_through) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_5"}),
  };
  test_file(this->mount_location, file_path, FILE_FLAG_WRITE_THROUGH);
}

TYPED_TEST(winfsp_test, rdrw_can_append_file_no_flags) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_5"}),
  };
  test_append_file(this->mount_location, file_path, 0U);
}

TYPED_TEST(winfsp_test, rdrw_can_append_file_no_buffering) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_5"}),
  };
  test_append_file(this->mount_location, file_path, FILE_FLAG_NO_BUFFERING,
                   true);
}

TYPED_TEST(winfsp_test, rdrw_can_append_file_write_through) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_5"}),
  };
  test_append_file(this->mount_location, file_path, FILE_FLAG_WRITE_THROUGH);
}

TYPED_TEST(winfsp_test, rdrw_can_read_and_write_overlapped_file_no_flags) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_5"}),
  };
  test_overlapped_file(this->mount_location, file_path, 0U);
}

TYPED_TEST(winfsp_test, rdrw_can_read_and_write_overlapped_file_no_buffering) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_5"}),
  };
  test_overlapped_file(this->mount_location, file_path, FILE_FLAG_NO_BUFFERING);
}

TYPED_TEST(winfsp_test, rdrw_can_read_and_write_file_overlapped_write_through) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_5"}),
  };
  test_overlapped_file(this->mount_location, file_path,
                       FILE_FLAG_WRITE_THROUGH);
}

TYPED_TEST(winfsp_test, rdrw_can_read_and_write_mmap_file_no_flags) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_5"}),
  };
  test_mmap_file(this->mount_location, file_path, 0U, false);
  test_mmap_file(this->mount_location, file_path, 0U, true);
}

TYPED_TEST(winfsp_test, rdrw_can_read_and_write_mmap_file_no_buffering) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_5"}),
  };
  test_mmap_file(this->mount_location, file_path, FILE_FLAG_NO_BUFFERING,
                 false);
  test_mmap_file(this->mount_location, file_path, FILE_FLAG_NO_BUFFERING, true);
}

TYPED_TEST(winfsp_test, rdrw_can_read_and_write_file_mmap_write_through) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_5"}),
  };
  test_mmap_file(this->mount_location, file_path, FILE_FLAG_WRITE_THROUGH,
                 false);
  test_mmap_file(this->mount_location, file_path, FILE_FLAG_WRITE_THROUGH,
                 true);
}

TYPED_TEST(winfsp_test, rdrw_can_read_and_write_mixed_file) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_5"}),
  };
  test_mixed_file(this->mount_location, file_path);
}
} // namespace repertory

#endif // defined(_WIN32)
