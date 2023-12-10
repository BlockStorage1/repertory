/*
  Copyright <2018-2023> <scott.e.graves@protonmail.com>

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
#if _WIN32

#include "test_common.hpp"

#include "fixtures/winfsp_fixture.hpp"
#include "types/repertory.hpp"
#include "utils/event_capture.hpp"

namespace repertory {
void launch_app(std::string cmd) {
  PROCESS_INFORMATION pi{};
  STARTUPINFO si{};
  si.cb = sizeof(si);

  if (!::CreateProcessA(nullptr, (LPSTR)cmd.c_str(), nullptr, nullptr, FALSE,
                        CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP, nullptr,
                        nullptr, &si, &pi)) {
    throw std::runtime_error("CreateProcess failed (" +
                             std::to_string(::GetLastError()) + ")");
  }

  ::WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD code{};
  ::GetExitCodeProcess(pi.hProcess, &code);

  ::CloseHandle(pi.hProcess);
  ::CloseHandle(pi.hThread);
  EXPECT_EQ(0, code);
}

E_SIMPLE1(test_begin, normal, false, std::string, test_name, TN, E_STRING);
#define TEST_HEADER(func)                                                      \
  event_system::instance().raise<test_begin>(                                  \
      std::string(func) +                                                      \
      "\r\n***********************\r\n***********************")

static auto mount_setup(std::string &mount_point) {
  mount_point = "U:";
  return std::vector<std::string>({"unittests", "-f", mount_point});
}

static void execute_mount(winfsp_test *test,
                          const std::vector<std::string> &drive_args,
                          std::thread &th) {
  ASSERT_EQ(0, test->drive->mount(drive_args));
  th.join();
}

static void unmount(winfsp_test *test, const std::string &mount_point) {
  test->drive->shutdown();
  auto mounted = utils::file::is_directory(mount_point);
  for (auto i = 0; mounted && (i < 50); i++) {
    std::this_thread::sleep_for(100ms);
    mounted = utils::file::is_directory(mount_point);
  }
  EXPECT_FALSE(utils::file::is_directory(mount_point));
}

static void root_creation_test(const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);
  WIN32_FILE_ATTRIBUTE_DATA ad{};
  EXPECT_TRUE(
      ::GetFileAttributesEx(&mount_point[0], GetFileExInfoStandard, &ad));
  EXPECT_EQ(FILE_ATTRIBUTE_DIRECTORY, ad.dwFileAttributes);
  EXPECT_EQ(0, ad.nFileSizeHigh);
  EXPECT_EQ(0, ad.nFileSizeLow);
}

static auto create_test(winfsp_test *test, const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);

  auto file = utils::path::combine(mount_point, {{"test_create.txt"}});
  auto handle = ::CreateFileA(&file[0], GENERIC_READ, FILE_SHARE_READ, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle);
  EXPECT_TRUE(::CloseHandle(handle));

  EXPECT_TRUE(utils::file::is_file(file));

  std::uint64_t file_size;
  EXPECT_TRUE(utils::file::get_file_size(file, file_size));
  EXPECT_EQ(0, file_size);

  std::string attr;
  EXPECT_EQ(api_error::success, test->provider->get_item_meta(
                                    "/test_create.txt", META_ATTRIBUTES, attr));
  EXPECT_EQ(FILE_ATTRIBUTE_NORMAL, utils::string::to_uint32(attr));

  return file;
}

static void delete_file_test(const std::string &file) {
  TEST_HEADER(__FUNCTION__);
  event_capture ec({"file_removed"});
  EXPECT_TRUE(utils::file::retry_delete_file(file));
  EXPECT_FALSE(utils::file::is_file(file));
}

static void create_directory_test(const std::string &directory) {
  TEST_HEADER(__FUNCTION__);

  EXPECT_FALSE(::PathIsDirectory(&directory[0]));
  EXPECT_TRUE(::CreateDirectoryA(&directory[0], nullptr));
  EXPECT_TRUE(::PathIsDirectory(&directory[0]));
}

static void remove_directory_test(const std::string &directory) {
  TEST_HEADER(__FUNCTION__);

  event_capture ec({"directory_removed"});
  EXPECT_TRUE(::PathIsDirectory(&directory[0]));
  EXPECT_TRUE(::RemoveDirectoryA(&directory[0]));
  EXPECT_FALSE(::PathIsDirectory(&directory[0]));
}

static void write_file_test(const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);

  const auto file = utils::path::combine(mount_point, {"test_write.txt"});
  auto handle =
      ::CreateFileA(&file[0], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                    nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle);
  const std::string data = "0123456789";
  DWORD bytes_written = 0;
  EXPECT_TRUE(::WriteFile(handle, &data[0], static_cast<DWORD>(data.size()),
                          &bytes_written, nullptr));
  EXPECT_EQ(10, bytes_written);
  EXPECT_TRUE(::CloseHandle(handle));

  EXPECT_TRUE(utils::file::is_file(file));

  std::uint64_t file_size;
  EXPECT_TRUE(utils::file::get_file_size(file, file_size));
  EXPECT_EQ(10, file_size);
}

static void read_file_test(const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);

  const auto file = utils::path::combine(mount_point, {"test_read.txt"});
  auto handle =
      ::CreateFileA(&file[0], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                    nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle);
  const std::string data = "0123456789";
  DWORD bytes_written = 0;
  EXPECT_TRUE(::WriteFile(handle, &data[0], static_cast<DWORD>(data.size()),
                          &bytes_written, nullptr));
  EXPECT_EQ(10, bytes_written);

  data_buffer data2;
  data2.resize(10);
  DWORD bytes_read = 0;
  EXPECT_EQ(0, ::SetFilePointer(handle, 0, nullptr, FILE_BEGIN));
  EXPECT_TRUE(::ReadFile(handle, &data2[0], static_cast<DWORD>(data2.size()),
                         &bytes_read, nullptr));
  EXPECT_EQ(10, bytes_read);
  for (auto i = 0; i < data.size(); i++) {
    EXPECT_EQ(data[i], data2[i]);
  }
  EXPECT_TRUE(::CloseHandle(handle));
}

static void rename_file_test(winfsp_test *test,
                             const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);
  const auto file = utils::path::combine(mount_point, {"rename_file.txt"});
  auto handle = ::CreateFileA(&file[0], GENERIC_READ, FILE_SHARE_READ, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle);
  EXPECT_TRUE(::CloseHandle(handle));

  api_meta_map meta1{};
  EXPECT_EQ(api_error::success,
            test->provider->get_item_meta("/rename_file.txt", meta1));

  const auto file2 = utils::path::combine(mount_point, {"rename_file2.txt"});
  EXPECT_TRUE(::MoveFile(&file[0], &file2[0]));

  EXPECT_TRUE(utils::file::is_file(file2));
  EXPECT_FALSE(utils::file::is_file(file));

  api_meta_map meta2{};
  EXPECT_EQ(api_error::success,
            test->provider->get_item_meta("/rename_file2.txt", meta2));
  EXPECT_STREQ(meta1[META_SOURCE].c_str(), meta2[META_SOURCE].c_str());

  filesystem_item fsi{};
  EXPECT_EQ(api_error::success, test->provider->get_filesystem_item(
                                    "/rename_file2.txt", false, fsi));
  EXPECT_STREQ(meta1[META_SOURCE].c_str(), fsi.source_path.c_str());

  filesystem_item fsi2{};
  EXPECT_EQ(api_error::success,
            test->provider->get_filesystem_item_from_source_path(
                fsi.source_path, fsi2));
  EXPECT_STREQ("/rename_file2.txt", fsi2.api_path.c_str());

  EXPECT_EQ(api_error::item_not_found,
            test->provider->get_item_meta("/rename_file.txt", meta2));
}

static void rename_directory_test(const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);
  std::string directory = "rename_dir";
  const auto full_directory = utils::path::combine(mount_point, {directory});
  std::string directory2 = "rename_dir2";
  const auto full_directory2 = utils::path::combine(mount_point, {directory2});

  EXPECT_FALSE(::PathIsDirectory(&full_directory[0]));
  EXPECT_TRUE(::CreateDirectoryA(&full_directory[0], nullptr));
  EXPECT_TRUE(::PathIsDirectory(&full_directory[0]));
  EXPECT_TRUE(::MoveFile(&full_directory[0], &full_directory2[0]));
  EXPECT_FALSE(::PathIsDirectory(&full_directory[0]));
  EXPECT_TRUE(::PathIsDirectory(&full_directory2[0]));
}

static void get_set_basic_info_test(const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);

  const auto file =
      utils::path::combine(mount_point, {"setbasicinfo_file.txt"});
  auto handle =
      ::CreateFileA(&file[0], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                    nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle);

  SYSTEMTIME st{};
  ::GetSystemTime(&st);
  st.wMinute = 0;

  FILETIME test_ch_time{};
  st.wMinute++;
  ::SystemTimeToFileTime(&st, &test_ch_time);

  FILETIME test_cr_time{};
  st.wMinute++;
  ::SystemTimeToFileTime(&st, &test_cr_time);

  FILETIME test_la_time{};
  st.wMinute++;
  ::SystemTimeToFileTime(&st, &test_la_time);

  FILETIME test_lw_time{};
  st.wMinute++;
  ::SystemTimeToFileTime(&st, &test_lw_time);

  FILE_BASIC_INFO fbi{};
  fbi.FileAttributes = FILE_ATTRIBUTE_HIDDEN;
  fbi.ChangeTime.HighPart = test_ch_time.dwHighDateTime;
  fbi.ChangeTime.LowPart = test_ch_time.dwLowDateTime;
  fbi.CreationTime = *(LARGE_INTEGER *)&test_cr_time;
  fbi.LastAccessTime = *(LARGE_INTEGER *)&test_la_time;
  fbi.LastWriteTime = *(LARGE_INTEGER *)&test_lw_time;

  EXPECT_TRUE(::SetFileInformationByHandle(handle, FileBasicInfo, &fbi,
                                           sizeof(FILE_BASIC_INFO)));

  FILE_BASIC_INFO fbi2{};
  EXPECT_TRUE(::GetFileInformationByHandleEx(handle, FileBasicInfo, &fbi2,
                                             sizeof(FILE_BASIC_INFO)));

  EXPECT_EQ(0, memcmp(&fbi, &fbi2, sizeof(FILE_BASIC_INFO)));

  std::cout << fbi.FileAttributes << " " << fbi.ChangeTime.QuadPart << " "
            << fbi.CreationTime.QuadPart << " " << fbi.LastAccessTime.QuadPart
            << " " << fbi.LastWriteTime.QuadPart << std::endl;
  std::cout << fbi2.FileAttributes << " " << fbi2.ChangeTime.QuadPart << " "
            << fbi2.CreationTime.QuadPart << " " << fbi2.LastAccessTime.QuadPart
            << " " << fbi2.LastWriteTime.QuadPart << std::endl;

  EXPECT_TRUE(::CloseHandle(handle));
}

static void overwrite_file_test(const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);

  const auto file = utils::path::combine("./", {"test_overwrite.txt"});
  auto handle =
      ::CreateFileA(&file[0], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                    nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle);
  if (handle != INVALID_HANDLE_VALUE) {
    const std::string data = "0123456789";
    DWORD bytes_written = 0;
    EXPECT_TRUE(::WriteFile(handle, &data[0], static_cast<DWORD>(data.size()),
                            &bytes_written, nullptr));
    EXPECT_EQ(10, bytes_written);
    EXPECT_TRUE(::CloseHandle(handle));

    if (bytes_written == 10) {
      const auto file2 =
          utils::path::combine(mount_point, {"test_overwrite2.txt"});
      EXPECT_TRUE(::CopyFile(&file[0], &file2[0], TRUE));

      EXPECT_FALSE(::CopyFile(&file[0], &file2[0], TRUE));
    }
  }
}

TEST_F(winfsp_test, all_tests) {
  if (PROVIDER_INDEX == 0) {
    for (std::size_t idx = 0U; idx < 2U; idx++) {
      launch_app(
          ("cmd.exe /c unittests.exe --gtest_filter=winfsp_test.all_tests "
           "--provider_index " +
           std::to_string(idx) + " > unittests" + std::to_string(idx) +
           ".log 2>&1"));
    }

    return;
  }

#ifndef REPERTORY_ENABLE_S3
  if (PROVIDER_INDEX == 1U) {
    return;
  }
#endif

  std::string mount_point;
  const auto drive_args = mount_setup(mount_point);

  event_capture ec({
      "drive_mounted",
      "drive_unmounted",
      "drive_unmount_pending",
      "drive_mount_result",
  });

  std::thread th([&] {
    const auto mounted = ec.wait_for_event("drive_mounted");
    EXPECT_TRUE(mounted);
    if (mounted) {
      root_creation_test(mount_point);
      {
        const auto file = create_test(this, mount_point);
        delete_file_test(file);
      }
      {
        const auto dir = utils::path::combine(mount_point, {"TestDir"});
        create_directory_test(dir);
        remove_directory_test(dir);
      }
      write_file_test(mount_point);
      read_file_test(mount_point);
      // TODO enable after rename support is available
      // rename_file_test(this, mount_point);
      // rename_directory_test(mount_point);
      overwrite_file_test(mount_point);
      get_set_basic_info_test(mount_point);
    }

    if (mounted) {
      unmount(this, mount_point);
      ec.wait_for_empty();
    }
  });

  execute_mount(this, drive_args, th);
}
} // namespace repertory

#endif
