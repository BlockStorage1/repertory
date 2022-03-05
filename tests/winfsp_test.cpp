/*
  Copyright <2018-2022> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute,
  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
  OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#if _WIN32

#include "fixtures/winfsp_fixture.hpp"
#include "utils/event_capture.hpp"
#include "test_common.hpp"

namespace repertory {
void launch_app(std::string cmd) {
  PROCESS_INFORMATION pi{};
  STARTUPINFO si{};
  si.cb = sizeof(si);

  if (!::CreateProcessA(nullptr, (LPSTR)cmd.c_str(), nullptr, nullptr, FALSE,
                        CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &si,
                        &pi)) {
    throw std::runtime_error("CreateProcess failed (" + std::to_string(::GetLastError()) + ")");
  }

  ::WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD code{};
  ::GetExitCodeProcess(pi.hProcess, &code);

  ::CloseHandle(pi.hProcess);
  ::CloseHandle(pi.hThread);
  EXPECT_EQ(0, code);
}

E_SIMPLE1(test_begin, normal, false, std::string, test_name, TN, E_STRING);
#define TEST_HEADER(func)                                                                          \
  event_system::instance().raise<test_begin>(                                                      \
      std::string(func) + "\r\n***********************\r\n***********************")

static auto mount_setup(const std::size_t &idx, winfsp_test *test, std::string &mount_point) {
  // Mock communicator setup
  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.push_return("get", "/wallet", {}, {}, api_error::success, true);
    mock_comm.push_return("get", "/renter/dir/",
                          {{"directories", {{{"numfiles", 0}, {"numsubdirs", 0}}}}}, {},
                          api_error::success, true);
    mock_comm.push_return("get", "/renter/file/", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/autorun.inf", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/.git", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/.git", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/HEAD", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/HEAD", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/autorun.inf", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/AutoRun.inf", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/AutoRun.inf", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/Desktop.ini", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/Desktop.ini", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    json files;
    files["files"] = std::vector<json>();
    mock_comm.push_return("get", "/renter/files", files, {}, api_error::success, true);
    mock_comm.push_return("get", "/renter/prices", {}, {{"message", "offline"}},
                          api_error::comm_error, true);
    mock_comm.push_return(
        "get", "/daemon/version",
        {{"version", app_config::get_provider_minimum_version(provider_type::sia)}}, {},
        api_error::success, true);

    // Mount setup
    mount_point = "U:";
    return std::vector<std::string>({"unittests", "-f", mount_point});
  }

  if (idx == 1) {
    // Mount setup
    mount_point = "V:";
    return std::vector<std::string>({"unittests", "-f", mount_point});
  }
}

static void execute_mount(const std::size_t &idx, winfsp_test *test,
                          const std::vector<std::string> &driveArgs, std::thread &th) {
  ASSERT_EQ(0, std::get<2>(test->provider_tests_[idx])->mount(driveArgs));
  th.join();
}

static void unmount(const std::size_t &idx, winfsp_test *test, const std::string &mount_point) {
  std::get<2>(test->provider_tests_[idx])->shutdown();
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
  EXPECT_TRUE(::GetFileAttributesEx(&mount_point[0], GetFileExInfoStandard, &ad));
  EXPECT_EQ(FILE_ATTRIBUTE_DIRECTORY, ad.dwFileAttributes);
  EXPECT_EQ(0, ad.nFileSizeHigh);
  EXPECT_EQ(0, ad.nFileSizeLow);
}

static auto create_test(const std::size_t &idx, winfsp_test *test, const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);
  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.push_return("get", "/renter/dir/test_create.txt", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/test_create.txt", {}, {{"message", "no file known"}},
                          api_error::comm_error);
    mock_comm.push_return("post_params", "/renter/delete/test_create.txt", {},
                          {{"message", "no file known"}}, api_error::comm_error);
    mock_comm.push_return("post_params", "/renter/upload/test_create.txt", {}, {},
                          api_error::success);
    mock_comm.push_return("get", "/renter/file/test_create.txt",
                          RENTER_FILE_DATA("test_create.txt", 0), {}, api_error::success, true);
  }

  auto file = utils::path::combine(mount_point, {{"test_create.txt"}});
  auto handle = ::CreateFileA(&file[0], GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle);
  EXPECT_TRUE(::CloseHandle(handle));

  EXPECT_TRUE(utils::file::is_file(file));

  std::uint64_t file_size;
  EXPECT_TRUE(utils::file::get_file_size(file, file_size));
  EXPECT_EQ(0, file_size);

  std::string attr;
  EXPECT_EQ(api_error::success, std::get<1>(test->provider_tests_[idx])
                                    ->get_item_meta("/test_create.txt", META_ATTRIBUTES, attr));
  EXPECT_EQ(FILE_ATTRIBUTE_NORMAL, utils::string::to_uint32(attr));

  event_system::instance().stop();
  event_system::instance().start();

  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.remove_return("get", "/renter/dir/test_create.txt");
    mock_comm.remove_return("post_params", "/renter/delete/test_create.txt");
    mock_comm.remove_return("post_params", "/renter/upload/test_create.txt");
    mock_comm.remove_return("get", "/renter/file/test_create.txt");
  }

  return file;
}

static void delete_file_test(const std::size_t &idx, winfsp_test *test, const std::string &file) {
  TEST_HEADER(__FUNCTION__);
  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.push_return("get", "/renter/dir/test_create.txt", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/test_create.txt",
                          RENTER_FILE_DATA("test_create.txt", 0), {}, api_error::success);
    mock_comm.push_return("get", "/renter/file/test_create.txt",
                          RENTER_FILE_DATA("test_create.txt", 0), {}, api_error::success);
    mock_comm.push_return("get", "/renter/file/test_create.txt",
                          RENTER_FILE_DATA("test_create.txt", 0), {}, api_error::success);
    mock_comm.push_return("get", "/renter/file/test_create.txt",
                          RENTER_FILE_DATA("test_create.txt", 0), {}, api_error::success);
    mock_comm.push_return("get", "/renter/file/test_create.txt", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("post_params", "/renter/delete/test_create.txt", {}, {},
                          api_error::success);
  }
  {
    event_capture ec({"file_removed"});
    EXPECT_TRUE(utils::file::delete_file(file));
    EXPECT_FALSE(utils::file::is_file(file));
    event_system::instance().stop();
    event_system::instance().start();
  }

  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.remove_return("post_params", "/renter/delete/test_create.txt");
  }
}

static void create_directory_test(const std::size_t &idx, winfsp_test *test,
                                  const std::string &directory) {
  TEST_HEADER(__FUNCTION__);
  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.push_return("get", "/renter/dir/" + directory.substr(3), {},
                          {{"message", "no file known"}}, api_error::comm_error);
    mock_comm.push_return("get", "/renter/dir/" + directory.substr(3), {},
                          {{"message", "no file known"}}, api_error::comm_error);
    mock_comm.push_return("get", "/renter/file/" + directory.substr(3), {},
                          {{"message", "no file known"}}, api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/" + directory.substr(3),
                          {{"directories", {{{"numfiles", 0}, {"numsubdirs", 0}}}}}, {},
                          api_error::success, true);
    mock_comm.push_return("post_params", "/renter/dir/" + directory.substr(3), {}, {},
                          api_error::success);
  }

  EXPECT_FALSE(::PathIsDirectory(&directory[0]));
  EXPECT_TRUE(::CreateDirectoryA(&directory[0], nullptr));
  EXPECT_TRUE(::PathIsDirectory(&directory[0]));

  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.remove_return("get", "/renter/dir/" + directory.substr(3));
    mock_comm.remove_return("get", "/renter/file/" + directory.substr(3));
    mock_comm.remove_return("post_params", "/renter/dir/" + directory.substr(3));
  }
}

static void remove_directory_test(const std::size_t &idx, winfsp_test *test,
                                  const std::string &directory) {
  TEST_HEADER(__FUNCTION__);
  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.push_return("get", "/renter/dir/" + directory.substr(3),
                          {{"directories", {{{"numfiles", 0}, {"numsubdirs", 0}}}}}, {},
                          api_error::success);
    mock_comm.push_return("get", "/renter/dir/" + directory.substr(3),
                          {{"directories", {{{"numfiles", 0}, {"numsubdirs", 0}}}}}, {},
                          api_error::success);
    mock_comm.push_return("get", "/renter/dir/" + directory.substr(3),
                          {{"directories", {{{"numfiles", 0}, {"numsubdirs", 0}}}}}, {},
                          api_error::success);
    mock_comm.push_return("get", "/renter/dir/" + directory.substr(3),
                          {{"directories", {{{"numfiles", 0}, {"numsubdirs", 0}}}}}, {},
                          api_error::success);
    mock_comm.push_return("post_params", "/renter/dir/" + directory.substr(3), {}, {},
                          api_error::success);
    mock_comm.push_return("get", "/renter/dir/" + directory.substr(3), {},
                          {{"message", "no file known"}}, api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/" + directory.substr(3), {},
                          {{"message", "no file known"}}, api_error::comm_error, true);
  }

  {
    event_capture ec({"directory_removed"});
    EXPECT_TRUE(::PathIsDirectory(&directory[0]));
    EXPECT_TRUE(::RemoveDirectoryA(&directory[0]));
    EXPECT_FALSE(::PathIsDirectory(&directory[0]));
    event_system::instance().stop();
    event_system::instance().start();
  }

  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.remove_return("post_params", "/renter/dir/" + directory.substr(3));
  }
}

static void write_file_test(const std::size_t &idx, winfsp_test *test,
                            const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);
  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.push_return("get", "/renter/dir/",
                          {{"directories", {{{"numfiles", 0}, {"numsubdirs", 0}}}}}, {},
                          api_error::success, true);
    mock_comm.push_return("get", "/renter/dir/test_write.txt", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/test_write.txt", {}, {{"message", "no file known"}},
                          api_error::comm_error);
    mock_comm.push_return("post_params", "/renter/upload/test_write.txt", {}, {},
                          api_error::success);
    mock_comm.push_return("post_params", "/renter/upload/test_write.txt", {}, {},
                          api_error::success);
    mock_comm.push_return("get", "/renter/file/test_write.txt",
                          RENTER_FILE_DATA("test_write.txt", 0), {}, api_error::success);
    mock_comm.push_return("get", "/renter/file/test_write.txt",
                          RENTER_FILE_DATA("test_write.txt", 10), {}, api_error::success, true);
  }

  const auto file = utils::path::combine(mount_point, {"test_write.txt"});
  auto handle = ::CreateFileA(&file[0], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle);
  const std::string data = "0123456789";
  DWORD bytes_written = 0;
  EXPECT_TRUE(
      ::WriteFile(handle, &data[0], static_cast<DWORD>(data.size()), &bytes_written, nullptr));
  EXPECT_EQ(10, bytes_written);
  EXPECT_TRUE(::CloseHandle(handle));

  EXPECT_TRUE(utils::file::is_file(file));

  std::uint64_t file_size;
  EXPECT_TRUE(utils::file::get_file_size(file, file_size));
  EXPECT_EQ(10, file_size);

  event_system::instance().stop();
  event_system::instance().start();

  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.remove_return("get", "/renter/dir/");
    mock_comm.remove_return("post_params", "/renter/upload/test_write.txt");
  }
}

static void read_file_test(const std::size_t &idx, winfsp_test *test,
                           const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);
  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.push_return("get", "/renter/dir/",
                          {{"directories", {{{"numfiles", 0}, {"numsubdirs", 0}}}}}, {},
                          api_error::success, true);
    mock_comm.push_return("get", "/renter/dir/test_read.txt", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/test_read.txt", {}, {{"message", "no file known"}},
                          api_error::comm_error);
    mock_comm.push_return("post_params", "/renter/upload/test_read.txt", {}, {}, api_error::success,
                          true);
    mock_comm.push_return("get", "/renter/file/test_read.txt", RENTER_FILE_DATA("test_read.txt", 0),
                          {}, api_error::success);
    mock_comm.push_return("get", "/renter/file/test_read.txt",
                          RENTER_FILE_DATA("test_read.txt", 10), {}, api_error::success, true);
  }

  const auto file = utils::path::combine(mount_point, {"test_read.txt"});
  auto handle = ::CreateFileA(&file[0], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle);
  const std::string data = "0123456789";
  DWORD bytes_written = 0;
  EXPECT_TRUE(
      ::WriteFile(handle, &data[0], static_cast<DWORD>(data.size()), &bytes_written, nullptr));
  EXPECT_EQ(10, bytes_written);

  std::vector<char> data2;
  data2.resize(10);
  DWORD bytes_read = 0;
  EXPECT_EQ(0, ::SetFilePointer(handle, 0, nullptr, FILE_BEGIN));
  EXPECT_TRUE(
      ::ReadFile(handle, &data2[0], static_cast<DWORD>(data2.size()), &bytes_read, nullptr));
  EXPECT_EQ(10, bytes_read);
  for (auto i = 0; i < data.size(); i++) {
    EXPECT_EQ(data[i], data2[i]);
  }
  EXPECT_TRUE(::CloseHandle(handle));

  event_system::instance().stop();
  event_system::instance().start();
}

static void rename_file_test(const std::size_t &idx, winfsp_test *test,
                             const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);
  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.push_return("get", "/renter/dir/rename_file.txt", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/rename_file.txt", {}, {{"message", "no file known"}},
                          api_error::comm_error);
    mock_comm.push_return("post_params", "/renter/upload/rename_file.txt", {}, {},
                          api_error::success);
    mock_comm.push_return("get", "/renter/file/rename_file.txt",
                          RENTER_FILE_DATA("rename_file.txt", 0), {}, api_error::success, true);

    mock_comm.push_return("get", "/renter/dir/rename_file2.txt", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/rename_file2.txt", {},
                          {{"message", "no file known"}}, api_error::comm_error);
    mock_comm.push_return("get", "/renter/file/rename_file2.txt", {},
                          {{"message", "no file known"}}, api_error::comm_error);
    mock_comm.push_return("get", "/renter/file/rename_file2.txt", {},
                          {{"message", "no file known"}}, api_error::comm_error);
    mock_comm.push_return("post_params", "/renter/rename/rename_file.txt", {}, {},
                          api_error::success);
  }

  const auto file = utils::path::combine(mount_point, {"rename_file.txt"});
  auto handle = ::CreateFileA(&file[0], GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle);
  EXPECT_TRUE(::CloseHandle(handle));

  auto &provider = *std::get<1>(test->provider_tests_[idx]);

  api_meta_map meta1{};
  EXPECT_EQ(api_error::success, provider.get_item_meta("/rename_file.txt", meta1));

  const auto file2 = utils::path::combine(mount_point, {"rename_file2.txt"});
  EXPECT_TRUE(::MoveFile(&file[0], &file2[0]));
  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.remove_return("get", "/renter/file/rename_file.txt");
    mock_comm.push_return("get", "/renter/file/rename_file.txt", {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/rename_file2.txt",
                          RENTER_FILE_DATA("rename_file2.txt", 0), {}, api_error::success, true);
  }

  EXPECT_TRUE(utils::file::is_file(file2));
  EXPECT_FALSE(utils::file::is_file(file));

  api_meta_map meta2{};
  EXPECT_EQ(api_error::success, provider.get_item_meta("/rename_file2.txt", meta2));
  EXPECT_STREQ(meta1[META_SOURCE].c_str(), meta2[META_SOURCE].c_str());

  filesystem_item fsi{};
  EXPECT_EQ(api_error::success, provider.get_filesystem_item("/rename_file2.txt", false, fsi));
  EXPECT_STREQ(meta1[META_SOURCE].c_str(), fsi.source_path.c_str());

  filesystem_item fsi2{};
  EXPECT_EQ(api_error::success,
            provider.get_filesystem_item_from_source_path(fsi.source_path, fsi2));
  EXPECT_STREQ("/rename_file2.txt", fsi2.api_path.c_str());

  EXPECT_EQ(api_error::item_not_found, provider.get_item_meta("/rename_file.txt", meta2));
  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.remove_return("post_params", "/renter/upload/rename_file.txt");
    mock_comm.remove_return("post_params", "/renter/rename/rename_file.txt");
  }
}

static void rename_directory_test(const std::size_t &idx, winfsp_test *test,
                                  const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);
  std::string directory = "rename_dir";
  const auto full_directory = utils::path::combine(mount_point, {directory});
  std::string directory2 = "rename_dir2";
  const auto full_directory2 = utils::path::combine(mount_point, {directory2});
  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.push_return("get", "/renter/dir/" + directory, {}, {{"message", "no file known"}},
                          api_error::comm_error);
    mock_comm.push_return("get", "/renter/dir/" + directory, {}, {{"message", "no file known"}},
                          api_error::comm_error);
    mock_comm.push_return("get", "/renter/file/" + directory, {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/" + directory2, {}, {{"message", "no file known"}},
                          api_error::comm_error);
    mock_comm.push_return("get", "/renter/dir/" + directory2, {}, {{"message", "no file known"}},
                          api_error::comm_error);
    mock_comm.push_return("get", "/renter/dir/" + directory2, {}, {{"message", "no file known"}},
                          api_error::comm_error);

    mock_comm.push_return("get", "/renter/file/" + directory2, {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/" + directory, RENTER_DIR_DATA(directory), {},
                          api_error::success);
    mock_comm.push_return("get", "/renter/dir/" + directory, RENTER_DIR_DATA(directory), {},
                          api_error::success);
    mock_comm.push_return("get", "/renter/dir/" + directory, RENTER_DIR_DATA(directory), {},
                          api_error::success);
    mock_comm.push_return("get", "/renter/dir/" + directory, RENTER_DIR_DATA(directory), {},
                          api_error::success);
    mock_comm.push_return("get", "/renter/dir/" + directory, RENTER_DIR_DATA(directory), {},
                          api_error::success);
    mock_comm.push_return("get", "/renter/dir/" + directory, RENTER_DIR_DATA(directory), {},
                          api_error::success);
    mock_comm.push_return("get", "/renter/dir/" + directory, RENTER_DIR_DATA(directory), {},
                          api_error::success);
    mock_comm.push_return("get", "/renter/dir/" + directory, RENTER_DIR_DATA(directory), {},
                          api_error::success);
    mock_comm.push_return("post_params", "/renter/dir/" + directory, {}, {}, api_error::success);
    mock_comm.push_return("post_params", "/renter/dir/" + directory, {}, {}, api_error::success);

    mock_comm.push_return("post_params", "/renter/dir/" + directory2, {}, {}, api_error::success);
    mock_comm.push_return("get", "/renter/dir/" + directory2, RENTER_DIR_DATA(directory2), {},
                          api_error::success, true);

    mock_comm.push_return("get", "/renter/dir/" + directory, {}, {{"message", "no file known"}},
                          api_error::comm_error, true);
  }

  EXPECT_FALSE(::PathIsDirectory(&full_directory[0]));
  EXPECT_TRUE(::CreateDirectoryA(&full_directory[0], nullptr));
  EXPECT_TRUE(::PathIsDirectory(&full_directory[0]));
  EXPECT_TRUE(::MoveFile(&full_directory[0], &full_directory2[0]));
  EXPECT_FALSE(::PathIsDirectory(&full_directory[0]));
  EXPECT_TRUE(::PathIsDirectory(&full_directory2[0]));

  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.remove_return("post_params", "/renter/dir/" + directory);
    mock_comm.remove_return("get", "/renter/dir/" + directory2);
    mock_comm.remove_return("get", "/renter/file/" + directory2);
    mock_comm.remove_return("post_params", "/renter/dir/" + directory2);
  }
}

static void get_set_basic_info_test(const std::size_t &idx, winfsp_test *test,
                                    const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);
  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.push_return("get", "/renter/dir/setbasicinfo_file.txt", {},
                          {{"message", "no file known"}}, api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/setbasicinfo_file.txt", {},
                          {{"message", "no file known"}}, api_error::comm_error);
    mock_comm.push_return("post_params", "/renter/upload/setbasicinfo_file.txt", {}, {},
                          api_error::success, true);

    mock_comm.push_return("get", "/renter/file/setbasicinfo_file.txt",
                          RENTER_FILE_DATA("setbasicinfo_file.txt", 0), {}, api_error::success,
                          true);
  }

  const auto file = utils::path::combine(mount_point, {"setbasicinfo_file.txt"});
  auto handle = ::CreateFileA(&file[0], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
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

  EXPECT_TRUE(::SetFileInformationByHandle(handle, FileBasicInfo, &fbi, sizeof(FILE_BASIC_INFO)));

  FILE_BASIC_INFO fbi2{};
  EXPECT_TRUE(
      ::GetFileInformationByHandleEx(handle, FileBasicInfo, &fbi2, sizeof(FILE_BASIC_INFO)));

  EXPECT_EQ(0, memcmp(&fbi, &fbi2, sizeof(FILE_BASIC_INFO)));

  std::cout << fbi.FileAttributes << " " << fbi.ChangeTime.QuadPart << " "
            << fbi.CreationTime.QuadPart << " " << fbi.LastAccessTime.QuadPart << " "
            << fbi.LastWriteTime.QuadPart << std::endl;
  std::cout << fbi2.FileAttributes << " " << fbi2.ChangeTime.QuadPart << " "
            << fbi2.CreationTime.QuadPart << " " << fbi2.LastAccessTime.QuadPart << " "
            << fbi2.LastWriteTime.QuadPart << std::endl;

  EXPECT_TRUE(::CloseHandle(handle));
}

static void overwrite_file_test(const std::size_t &idx, winfsp_test *test,
                                const std::string &mount_point) {
  TEST_HEADER(__FUNCTION__);
  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.push_return("get", "/renter/dir/test_overwrite.txt", {},
                          {{"message", "no file known"}}, api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/dir/test_overwrite2.txt", {},
                          {{"message", "no file known"}}, api_error::comm_error, true);
    mock_comm.push_return("get", "/renter/file/test_overwrite.txt", {},
                          {{"message", "no file known"}}, api_error::comm_error);

    mock_comm.push_return("post_params", "/renter/upload/test_overwrite.txt", {}, {},
                          api_error::success, true);
    mock_comm.push_return("post_params", "/renter/upload/test_overwrite2.txt", {}, {},
                          api_error::success, true);

    mock_comm.push_return("post_params", "/renter/delete/test_overwrite2.txt", {}, {},
                          api_error::success, true);
  }

  const auto file = utils::path::combine("./", {"test_overwrite.txt"});
  auto handle = ::CreateFileA(&file[0], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  EXPECT_NE(INVALID_HANDLE_VALUE, handle);
  if (handle != INVALID_HANDLE_VALUE) {
    const std::string data = "0123456789";
    DWORD bytes_written = 0;
    EXPECT_TRUE(
        ::WriteFile(handle, &data[0], static_cast<DWORD>(data.size()), &bytes_written, nullptr));
    EXPECT_EQ(10, bytes_written);
    EXPECT_TRUE(::CloseHandle(handle));

    if (bytes_written == 10) {
      const auto file2 = utils::path::combine(mount_point, {"test_overwrite2.txt"});
      if (idx == 0) {
        auto &mock_comm = test->mock_sia_comm_;
        mock_comm.push_return("get", "/renter/file/test_overwrite2.txt", {},
                              {{"message", "no file known"}}, api_error::comm_error);
        mock_comm.push_return("get", "/renter/file/test_overwrite.txt",
                              RENTER_FILE_DATA("test_overwrite.txt", 0), {}, api_error::success,
                              true);
        mock_comm.push_return("get", "/renter/file/test_overwrite2.txt",
                              RENTER_FILE_DATA("test_overwrite2.txt", 0), {}, api_error::success,
                              true);
      }
      EXPECT_TRUE(::CopyFile(&file[0], &file2[0], TRUE));

      EXPECT_FALSE(::CopyFile(&file[0], &file2[0], TRUE));
    }
  }

  if (idx == 0) {
    auto &mock_comm = test->mock_sia_comm_;
    mock_comm.remove_return("post_params", "/renter/upload/test_overwrite.txt");
    mock_comm.remove_return("post_params", "/renter/upload/test_overwrite2.txt");
  }
}

TEST_F(winfsp_test, all_tests) {
  if (PROVIDER_INDEX == 0) {
    for (std::size_t idx = 0; idx < provider_tests_.size(); idx++) {
      launch_app(("cmd.exe /c unittests.exe --gtest_filter=winfsp_test.all_tests "
                  "--provider_index " +
                  std::to_string(idx) + " > unittests" + std::to_string(idx) + ".log 2>&1"));
    }
  } else {
    const auto idx = PROVIDER_INDEX - 1;

    std::string mount_point;
    const auto drive_args = mount_setup(idx, this, mount_point);

    event_capture ec({"drive_mounted"});
    std::thread th([&] {
      const auto mounted = ec.wait_for_event("drive_mounted");
      EXPECT_TRUE(mounted);
      if (mounted) {
        root_creation_test(mount_point);
        {
          const auto file = create_test(idx, this, mount_point);
          delete_file_test(idx, this, file);
        }
        {
          const auto testDir = utils::path::combine(mount_point, {"TestDir"});
          create_directory_test(idx, this, testDir);
          remove_directory_test(idx, this, testDir);
        }
        write_file_test(idx, this, mount_point);
        read_file_test(idx, this, mount_point);
        rename_file_test(idx, this, mount_point);
        rename_directory_test(idx, this, mount_point);
        overwrite_file_test(idx, this, mount_point);
        get_set_basic_info_test(idx, this, mount_point);
      }

      if (mounted) {
        unmount(idx, this, mount_point);
      }
    });

    execute_mount(idx, this, drive_args, th);
  }
}
} // namespace repertory

#endif
