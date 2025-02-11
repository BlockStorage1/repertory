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

TYPED_TEST(winfsp_test, info_can_get_tag_info) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_2"}),
  };
  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  FILE_ATTRIBUTE_TAG_INFO tag_info{};
  EXPECT_TRUE(::GetFileInformationByHandleEx(handle, FileAttributeTagInfo,
                                             &tag_info, sizeof tag_info));
  EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE, tag_info.FileAttributes);
  EXPECT_EQ(0, tag_info.ReparseTag);

  ::CloseHandle(handle);
}

TYPED_TEST(winfsp_test, info_can_get_basic_info) {
  FILETIME file_time{};
  ::GetSystemTimeAsFileTime(&file_time);

  auto time_low = reinterpret_cast<PLARGE_INTEGER>(&file_time)->QuadPart;
  auto time_high = time_low + 10000 * 10000 /* 10 seconds */;

  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_2"}),
  };
  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  FILE_BASIC_INFO basic_info{};
  EXPECT_TRUE(::GetFileInformationByHandleEx(handle, FileBasicInfo, &basic_info,
                                             sizeof basic_info));
  EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE, basic_info.FileAttributes);

  EXPECT_LE(time_low, basic_info.CreationTime.QuadPart);
  EXPECT_GT(time_high, basic_info.CreationTime.QuadPart);

  EXPECT_LE(time_low, basic_info.LastAccessTime.QuadPart);
  EXPECT_GT(time_high, basic_info.LastAccessTime.QuadPart);

  EXPECT_LE(time_low, basic_info.LastWriteTime.QuadPart);
  EXPECT_GT(time_high, basic_info.LastWriteTime.QuadPart);

  EXPECT_LE(time_low, basic_info.ChangeTime.QuadPart);
  EXPECT_GT(time_high, basic_info.ChangeTime.QuadPart);

  ::CloseHandle(handle);
}

TYPED_TEST(winfsp_test, info_can_get_standard_info) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_2"}),
  };
  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  FILE_STANDARD_INFO std_info{};
  EXPECT_TRUE(::GetFileInformationByHandleEx(handle, FileStandardInfo,
                                             &std_info, sizeof std_info));
  EXPECT_EQ(0, std_info.AllocationSize.QuadPart);
  EXPECT_EQ(0, std_info.EndOfFile.QuadPart);
  EXPECT_EQ(1, std_info.NumberOfLinks);
  EXPECT_FALSE(std_info.DeletePending);
  EXPECT_FALSE(std_info.Directory);

  ::CloseHandle(handle);
}

TYPED_TEST(winfsp_test, info_can_get_file_name_info) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_2"}),
  };
  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  std::array<std::uint8_t, sizeof(FILE_NAME_INFO) + MAX_PATH> name_info{};
  EXPECT_TRUE(
      ::GetFileInformationByHandleEx(handle, FileNameInfo, name_info.data(),
                                     static_cast<DWORD>(name_info.size())));

  auto *info = reinterpret_cast<FILE_NAME_INFO *>(name_info.data());
  auto expected_name{
      std::string{"\\repertory\\"} + this->mount_location.at(0U) +
          "\\test_file_2",
  };

  EXPECT_EQ(info->FileNameLength,
            static_cast<DWORD>(expected_name.size() * 2U));
  EXPECT_STREQ(expected_name.c_str(),
               utils::string::to_utf8(info->FileName).c_str());

  ::CloseHandle(handle);
}

TYPED_TEST(winfsp_test, info_get_file_name_info_buffer_too_small) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_2"}),
  };
  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  std::array<std::uint8_t, sizeof(FILE_NAME_INFO)> name_info{};
  EXPECT_FALSE(
      ::GetFileInformationByHandleEx(handle, FileNameInfo, name_info.data(),
                                     static_cast<DWORD>(name_info.size())));
  EXPECT_EQ(ERROR_MORE_DATA, ::GetLastError());

  auto *info = reinterpret_cast<FILE_NAME_INFO *>(name_info.data());
  EXPECT_EQ('\\', info->FileName[0U]);

  ::CloseHandle(handle);
}

TYPED_TEST(winfsp_test, info_can_get_file_info) {
  FILETIME file_time{};
  ::GetSystemTimeAsFileTime(&file_time);

  auto time_low = reinterpret_cast<PLARGE_INTEGER>(&file_time)->QuadPart;
  auto time_high = time_low + 10000 * 10000 /* 10 seconds */;

  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_2"}),
  };
  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  BY_HANDLE_FILE_INFORMATION file_info{};
  EXPECT_TRUE(::GetFileInformationByHandle(handle, &file_info));

  EXPECT_LE(
      time_low,
      reinterpret_cast<PLARGE_INTEGER>(&file_info.ftCreationTime)->QuadPart);
  EXPECT_GT(
      time_high,
      reinterpret_cast<PLARGE_INTEGER>(&file_info.ftCreationTime)->QuadPart);

  EXPECT_LE(
      time_low,
      reinterpret_cast<PLARGE_INTEGER>(&file_info.ftLastAccessTime)->QuadPart);
  EXPECT_GT(
      time_high,
      reinterpret_cast<PLARGE_INTEGER>(&file_info.ftLastAccessTime)->QuadPart);

  EXPECT_LE(
      time_low,
      reinterpret_cast<PLARGE_INTEGER>(&file_info.ftLastWriteTime)->QuadPart);
  EXPECT_GT(
      time_high,
      reinterpret_cast<PLARGE_INTEGER>(&file_info.ftLastWriteTime)->QuadPart);

  EXPECT_EQ(0U, file_info.nFileSizeHigh);
  EXPECT_EQ(0U, file_info.nFileSizeLow);

  EXPECT_EQ(1U, file_info.nNumberOfLinks);

  EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE, file_info.dwFileAttributes);

  EXPECT_EQ(0U, file_info.dwVolumeSerialNumber);

  ::CloseHandle(handle);
}

TYPED_TEST(winfsp_test, info_can_get_file_path) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_2"}),
  };
  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  std::string final_path;
  final_path.resize(MAX_PATH + 1U);

  auto result = ::GetFinalPathNameByHandleA(
      handle, final_path.data(), final_path.size() - 1U,
      VOLUME_NAME_NONE | FILE_NAME_OPENED);

  auto expected_name{
      std::string{"\\repertory\\"} + this->mount_location.at(0U) +
          "\\test_file_2",
  };
  EXPECT_EQ(result, static_cast<DWORD>(expected_name.size()));
  EXPECT_STREQ(expected_name.c_str(), final_path.c_str());

  ::CloseHandle(handle);
}

TYPED_TEST(winfsp_test, info_can_set_file_info_attributes_to_hidden) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_2"}),
  };
  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  EXPECT_TRUE(::SetFileAttributesA(file_path.c_str(), FILE_ATTRIBUTE_HIDDEN));

  BY_HANDLE_FILE_INFORMATION file_info{};
  EXPECT_TRUE(::GetFileInformationByHandle(handle, &file_info));
  EXPECT_EQ(FILE_ATTRIBUTE_HIDDEN, file_info.dwFileAttributes);

  ::CloseHandle(handle);
}

TYPED_TEST(
    winfsp_test,
    info_can_set_file_info_attributes_to_hidden_ignoring_directory_attribute) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_2"}),
  };
  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  EXPECT_TRUE(::SetFileAttributesA(
      file_path.c_str(), FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN));

  BY_HANDLE_FILE_INFORMATION file_info{};
  EXPECT_TRUE(::GetFileInformationByHandle(handle, &file_info));
  EXPECT_EQ(FILE_ATTRIBUTE_HIDDEN, file_info.dwFileAttributes);

  ::CloseHandle(handle);
}

TYPED_TEST(winfsp_test, info_can_set_creation_file_time) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_2"}),
  };
  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  BY_HANDLE_FILE_INFORMATION orig_file_info{};
  EXPECT_TRUE(::GetFileInformationByHandle(handle, &orig_file_info));

  static constexpr const UINT64 file_time{
      116444736000000000ULL + 0x4200000042ULL,
  };

  EXPECT_TRUE(::SetFileTime(handle,
                            reinterpret_cast<const FILETIME *>(&file_time),
                            nullptr, nullptr));

  BY_HANDLE_FILE_INFORMATION file_info{};
  EXPECT_TRUE(::GetFileInformationByHandle(handle, &file_info));

  EXPECT_EQ(file_time, *reinterpret_cast<PUINT64>(&file_info.ftCreationTime));

  EXPECT_EQ(*reinterpret_cast<PUINT64>(&orig_file_info.ftLastAccessTime),
            *reinterpret_cast<PUINT64>(&file_info.ftLastAccessTime));
  EXPECT_EQ(*reinterpret_cast<PUINT64>(&orig_file_info.ftLastWriteTime),
            *reinterpret_cast<PUINT64>(&file_info.ftLastWriteTime));

  ::CloseHandle(handle);
}

TYPED_TEST(winfsp_test, info_can_set_accessed_file_time) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_2"}),
  };
  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  BY_HANDLE_FILE_INFORMATION orig_file_info{};
  EXPECT_TRUE(::GetFileInformationByHandle(handle, &orig_file_info));

  static constexpr const UINT64 file_time{
      116444736000000000ULL + 0x4200000042ULL,
  };

  EXPECT_TRUE(::SetFileTime(handle, nullptr,
                            reinterpret_cast<const FILETIME *>(&file_time),
                            nullptr));

  BY_HANDLE_FILE_INFORMATION file_info{};
  EXPECT_TRUE(::GetFileInformationByHandle(handle, &file_info));

  EXPECT_EQ(file_time, *reinterpret_cast<PUINT64>(&file_info.ftLastAccessTime));
  EXPECT_EQ(*reinterpret_cast<PUINT64>(&orig_file_info.ftCreationTime),
            *reinterpret_cast<PUINT64>(&file_info.ftCreationTime));
  EXPECT_EQ(*reinterpret_cast<PUINT64>(&orig_file_info.ftLastWriteTime),
            *reinterpret_cast<PUINT64>(&file_info.ftLastWriteTime));

  ::CloseHandle(handle);
}

TYPED_TEST(winfsp_test, info_can_set_written_file_time) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_2"}),
  };
  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  BY_HANDLE_FILE_INFORMATION orig_file_info{};
  EXPECT_TRUE(::GetFileInformationByHandle(handle, &orig_file_info));

  static constexpr const UINT64 file_time{
      116444736000000000ULL + 0x4200000042ULL,
  };

  EXPECT_TRUE(::SetFileTime(handle, nullptr, nullptr,
                            reinterpret_cast<const FILETIME *>(&file_time)));

  BY_HANDLE_FILE_INFORMATION file_info{};
  EXPECT_TRUE(::GetFileInformationByHandle(handle, &file_info));

  EXPECT_EQ(file_time, *reinterpret_cast<PUINT64>(&file_info.ftLastWriteTime));

  EXPECT_EQ(*reinterpret_cast<PUINT64>(&orig_file_info.ftLastAccessTime),
            *reinterpret_cast<PUINT64>(&file_info.ftLastAccessTime));
  EXPECT_EQ(*reinterpret_cast<PUINT64>(&orig_file_info.ftCreationTime),
            *reinterpret_cast<PUINT64>(&file_info.ftCreationTime));

  ::CloseHandle(handle);
}

TYPED_TEST(winfsp_test, info_can_set_file_size) {
  auto file_path{
      utils::path::combine(this->mount_location, {"test_file_2"}),
  };
  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  auto offset = ::SetFilePointer(handle, 42, nullptr, FILE_BEGIN);
  EXPECT_EQ(42U, offset);

  EXPECT_TRUE(::SetEndOfFile(handle));

  BY_HANDLE_FILE_INFORMATION file_info{};
  EXPECT_TRUE(::GetFileInformationByHandle(handle, &file_info));

  EXPECT_EQ(0U, file_info.nFileSizeHigh);
  EXPECT_EQ(42U, file_info.nFileSizeLow);

  ::CloseHandle(handle);
}
} // namespace repertory

#endif // defined(_WIN32)
