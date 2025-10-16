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

TYPED_TEST(winfsp_test, volume_can_get_volume_info) {
  std::string volume_label;
  volume_label.resize(MAX_PATH + 1U);

  std::string fs_name;
  fs_name.resize(MAX_PATH + 1U);

  DWORD flags{};
  DWORD max_component_length{};
  DWORD serial_num{};
  EXPECT_TRUE(::GetVolumeInformationA(
      this->mount_location.c_str(), volume_label.data(),
      static_cast<DWORD>(volume_label.size()), &serial_num,
      &max_component_length, &flags, fs_name.data(),
      static_cast<DWORD>(fs_name.size())));
  EXPECT_EQ(FILE_CASE_SENSITIVE_SEARCH | FILE_CASE_PRESERVED_NAMES |
                FILE_UNICODE_ON_DISK,
            flags);
  EXPECT_EQ(255U, max_component_length);
  EXPECT_EQ(0U, serial_num);
  if (this->current_provider == provider_type::unknown) {
    EXPECT_STREQ(
        ("repertory_" + app_config::get_provider_name(provider_type::sia))
            .c_str(),
        volume_label.c_str());
  } else {
    EXPECT_STREQ(
        ("repertory_" + app_config::get_provider_name(this->current_provider))
            .c_str(),
        volume_label.c_str());
  }
  EXPECT_STREQ(this->mount_location.c_str(), fs_name.c_str());
}

TYPED_TEST(winfsp_test, volume_can_get_size_info) {
  {
    DWORD bytes_per_sector{};
    DWORD free_clusters{};
    DWORD sectors_per_cluster{};
    DWORD total_clusters{};
    EXPECT_TRUE(::GetDiskFreeSpaceA(this->mount_location.c_str(),
                                    &sectors_per_cluster, &bytes_per_sector,
                                    &free_clusters, &total_clusters));
    EXPECT_NE(0U, bytes_per_sector);
    EXPECT_NE(0U, free_clusters);
    EXPECT_NE(0U, sectors_per_cluster);
    EXPECT_NE(0U, total_clusters);
  }

  {
    ULARGE_INTEGER caller_free_bytes{};
    ULARGE_INTEGER free_bytes{};
    ULARGE_INTEGER total_bytes{};
    EXPECT_TRUE(::GetDiskFreeSpaceExA(this->mount_location.c_str(),
                                      &caller_free_bytes, &total_bytes,
                                      &free_bytes));
    EXPECT_NE(0U, caller_free_bytes.QuadPart);
    EXPECT_NE(0U, total_bytes.QuadPart);
    EXPECT_NE(0U, free_bytes.QuadPart);
  }
}

TYPED_TEST(winfsp_test, volume_can_get_file_type) {
  auto handle = ::CreateFileA(
      this->mount_location.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
      nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);

  EXPECT_EQ(FILE_TYPE_DISK, ::GetFileType(handle));

  ::CloseHandle(handle);
}
} // namespace repertory

#endif // defined(_WIN32)
