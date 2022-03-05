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
#include "drives/winfsp/i_winfsp_drive.hpp"
#include "drives/winfsp/remotewinfsp/remote_client.hpp"
#include "events/consumers/console_consumer.hpp"
#include "test_common.hpp"
#ifdef _WIN32
#include "drives/winfsp/remotewinfsp/remote_server.hpp"
#include "mocks/mock_winfsp_drive.hpp"
#else
#include "drives/fuse/remotefuse/remote_server.hpp"
#include "mocks/mock_fuse_drive.hpp"
#endif
#include "utils/utils.hpp"

using namespace repertory;
using namespace repertory::remote_winfsp;

namespace winfsp_test {
static std::string mount_location_;

static void can_delete_test(remote_client &client) {
  const auto test_file = utils::path::absolute("./win_remote/candelete.txt");
  utils::file::delete_file(test_file);
  auto api_path = utils::string::from_utf8(test_file).substr(mount_location_.size());

  native_file::native_file_ptr nf;
  native_file::create_or_open(test_file, nf);
  EXPECT_TRUE(nf);
  if (nf) {
    EXPECT_EQ(STATUS_INVALID_HANDLE,
              client.winfsp_can_delete(reinterpret_cast<PVOID>(nf->get_handle()), &api_path[0]));

    nf->close();
    utils::file::delete_file(test_file);
  }
}

static void create_and_close_test(remote_client &client) {
  const auto test_file = utils::path::absolute("./win_remote/create.txt");
  utils::file::delete_file(test_file);
  auto api_path = utils::string::from_utf8(test_file).substr(mount_location_.size());

  PVOID file_desc = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  remote::file_info fi{};
  std::string normalized_name;
  BOOLEAN exists = 0;
  const auto ret =
      client.winfsp_create(&api_path[0], 0, GENERIC_READ | GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, 0,
                           &file_desc, &fi, normalized_name, exists);
  EXPECT_EQ(STATUS_SUCCESS, ret);
  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc));

  utils::file::delete_file(test_file);
}

static void cleanup_test(remote_client &client) {
  const auto test_file = utils::path::absolute("./win_remote/cleanup.txt");
  utils::file::delete_file(test_file);
  auto api_path = utils::string::from_utf8(test_file).substr(mount_location_.size());

  PVOID file_desc = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  remote::file_info fi{};
  std::string normalized_name;
  BOOLEAN exists = 0;
  auto ret =
      client.winfsp_create(&api_path[0], 0, GENERIC_READ | GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, 0,
                           &file_desc, &fi, normalized_name, exists);
  EXPECT_EQ(STATUS_SUCCESS, ret);

  BOOLEAN was_closed = 0;
  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_cleanup(file_desc, &api_path[0], 0, was_closed));
  EXPECT_FALSE(was_closed);
  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc));

  utils::file::delete_file(test_file);
}

static void flush_test(remote_client &client) {
  const auto test_file = utils::path::absolute("./win_remote/flush.txt");
  utils::file::delete_file(test_file);
  auto api_path = utils::string::from_utf8(test_file).substr(mount_location_.size());

  PVOID file_desc = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  remote::file_info fi{};
  std::string normalized_name;
  BOOLEAN exists = 0;
  auto ret =
      client.winfsp_create(&api_path[0], 0, GENERIC_READ | GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, 0,
                           &file_desc, &fi, normalized_name, exists);
  EXPECT_EQ(STATUS_SUCCESS, ret);

  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_flush(file_desc, &fi));

  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc));

  utils::file::delete_file(test_file);
}

static void get_file_info_test(remote_client &client) {
  const auto test_file = utils::path::absolute("./win_remote/getfileinfo.txt");
  utils::file::delete_file(test_file);
  auto api_path = utils::string::from_utf8(test_file).substr(mount_location_.size());

  PVOID file_desc = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  remote::file_info fi{};
  std::string normalized_name;
  BOOLEAN exists = 0;
  auto ret =
      client.winfsp_create(&api_path[0], 0, GENERIC_READ | GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, 0,
                           &file_desc, &fi, normalized_name, exists);
  EXPECT_EQ(STATUS_SUCCESS, ret);

  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_get_file_info(file_desc, &fi));

  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc));

  utils::file::delete_file(test_file);
}

static void get_security_by_name_test(remote_client &client) {
  const auto test_file = utils::path::absolute("./win_remote/getsecuritybyname.txt");
  utils::file::delete_file(test_file);
  auto api_path = utils::string::from_utf8(test_file).substr(mount_location_.size());

  PVOID file_desc = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  remote::file_info fi{};
  std::string normalized_name;
  BOOLEAN exists = 0;
  auto ret =
      client.winfsp_create(&api_path[0], 0, GENERIC_READ | GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, 0,
                           &file_desc, &fi, normalized_name, exists);
  EXPECT_EQ(STATUS_SUCCESS, ret);
  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc));

  UINT32 attributes = 0;
  std::uint64_t securityDescriptorSize = 1024;
  std::wstring strDescriptor;
  ret = client.winfsp_get_security_by_name(&api_path[0], &attributes, &securityDescriptorSize,
                                           strDescriptor);
  EXPECT_EQ(STATUS_SUCCESS, ret);
  EXPECT_EQ(FILE_ATTRIBUTE_NORMAL, attributes);
  EXPECT_FALSE(strDescriptor.empty());

  utils::file::delete_file(test_file);
}

static void get_volume_info_test(remote_client &client) {
  UINT64 total_size = 0u;
  UINT64 free_size = 0u;
  std::string volume_label;
  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_get_volume_info(total_size, free_size, volume_label));
  EXPECT_EQ(100, free_size);
  EXPECT_EQ(200, total_size);
  EXPECT_STREQ(&volume_label[0], "TestVolumeLabel");
}

static void mounted_test(remote_client &client) {
  const auto location = utils::string::from_utf8(mount_location_);
  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_mounted(location));
}

static void open_test(remote_client &client) {
  const auto test_file = utils::path::absolute("./win_remote/open.txt");
  utils::file::delete_file(test_file);
  auto api_path = utils::string::from_utf8(test_file).substr(mount_location_.size());

  PVOID file_desc = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  PVOID file_desc2 = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  {
    remote::file_info fi{};
    std::string normalized_name;
    BOOLEAN exists = 0;
    auto ret =
        client.winfsp_create(&api_path[0], 0, GENERIC_READ | GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL,
                             0, &file_desc, &fi, normalized_name, exists);
    EXPECT_EQ(STATUS_SUCCESS, ret);
  }
  {
    remote::file_info fi{};
    std::string normalized_name;
    const auto ret = client.winfsp_open(&api_path[0], 0, GENERIC_READ | GENERIC_WRITE, &file_desc2,
                                        &fi, normalized_name);
    EXPECT_EQ(STATUS_SUCCESS, ret);
  }

  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc));
  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc2));

  utils::file::delete_file(test_file);
}

static void overwrite_test(remote_client &client) {
  const auto test_file = utils::path::absolute("./win_remote/overwrite.txt");
  utils::file::delete_file(test_file);
  auto api_path = utils::string::from_utf8(test_file).substr(mount_location_.size());

  PVOID file_desc = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  remote::file_info fi{};
  std::string normalized_name;
  BOOLEAN exists = 0;
  auto ret =
      client.winfsp_create(&api_path[0], 0, GENERIC_READ | GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, 0,
                           &file_desc, &fi, normalized_name, exists);
  EXPECT_EQ(STATUS_SUCCESS, ret);

  const UINT32 attributes = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_ARCHIVE;
  const BOOLEAN replace_attributes = 0;
  const UINT64 allocation_size = 0;
  ret = client.winfsp_overwrite(file_desc, attributes, replace_attributes, allocation_size, &fi);
  EXPECT_EQ(STATUS_SUCCESS, ret);
  EXPECT_EQ(0, fi.FileSize);
  EXPECT_EQ(attributes, fi.FileAttributes);

  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc));

  utils::file::delete_file(test_file);
}

static void create_and_read_directory_test(remote_client &client) {
  const auto test_directory = utils::path::absolute("./win_remote/readdirectory");
  utils::file::delete_directory(test_directory);
  auto api_path = utils::string::from_utf8(test_directory).substr(mount_location_.size());

  PVOID file_desc = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  remote::file_info fi{};
  std::string normalized_name;
  BOOLEAN exists = 0;
  auto ret =
      client.winfsp_create(&api_path[0], FILE_DIRECTORY_FILE, GENERIC_READ | GENERIC_WRITE,
                           FILE_ATTRIBUTE_DIRECTORY, 0, &file_desc, &fi, normalized_name, exists);
  EXPECT_EQ(STATUS_SUCCESS, ret);

  EXPECT_TRUE(utils::file::is_directory(test_directory));

  json list;
  ret = client.winfsp_read_directory(file_desc, nullptr, nullptr, list);
  EXPECT_EQ(STATUS_SUCCESS, ret);
  EXPECT_EQ(2, list.size());

  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc));

  utils::file::delete_directory(test_directory);
}

static void open_and_read_directory_test(remote_client &client) {
  const auto test_directory = utils::path::absolute("./win_remote/openreaddirectory");
  utils::file::delete_directory(test_directory);
  auto api_path = utils::string::from_utf8(test_directory).substr(mount_location_.size());

  PVOID file_desc = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  remote::file_info fi{};
  std::string normalized_name;
  BOOLEAN exists = 0;
  auto ret =
      client.winfsp_create(&api_path[0], FILE_DIRECTORY_FILE, GENERIC_READ | GENERIC_WRITE,
                           FILE_ATTRIBUTE_DIRECTORY, 0, &file_desc, &fi, normalized_name, exists);
  EXPECT_EQ(STATUS_SUCCESS, ret);

  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc));

  EXPECT_TRUE(utils::file::is_directory(test_directory));

  file_desc = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  ret = client.winfsp_open(&api_path[0], FILE_DIRECTORY_FILE, GENERIC_READ | GENERIC_WRITE,
                           &file_desc, &fi, normalized_name);
  EXPECT_EQ(STATUS_SUCCESS, ret);

  json item_list;
  ret = client.winfsp_read_directory(file_desc, nullptr, nullptr, item_list);
  EXPECT_EQ(STATUS_SUCCESS, ret);
  EXPECT_EQ(2, item_list.size());

  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc));

  utils::file::delete_directory(test_directory);
}

static void read_and_write_test(remote_client &client) {
  const auto test_file = utils::path::absolute("./win_remote/readwrite.txt");
  utils::file::delete_file(test_file);
  auto api_path = utils::string::from_utf8(test_file).substr(mount_location_.size());

  PVOID file_desc = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  remote::file_info fi{};
  std::string normalized_name;
  BOOLEAN exists = 0;
  auto ret =
      client.winfsp_create(&api_path[0], 0, GENERIC_READ | GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, 0,
                           &file_desc, &fi, normalized_name, exists);
  EXPECT_EQ(STATUS_SUCCESS, ret);

  std::vector<char> buffer;
  buffer.emplace_back('T');
  buffer.emplace_back('e');
  buffer.emplace_back('s');
  buffer.emplace_back('t');
  UINT32 bytes_transferred = 0;
  ret = client.winfsp_write(file_desc, &buffer[0], 0, static_cast<UINT32>(buffer.size()), 0, 0,
                            &bytes_transferred, &fi);
  EXPECT_EQ(STATUS_SUCCESS, ret);
  EXPECT_EQ(buffer.size(), bytes_transferred);

  std::vector<char> buffer2(buffer.size());
  UINT32 bytes_transferred2 = 0;
  ret = client.winfsp_read(file_desc, &buffer2[0], 0, static_cast<UINT32>(buffer2.size()),
                           &bytes_transferred2);
  EXPECT_EQ(STATUS_SUCCESS, ret);
  EXPECT_EQ(bytes_transferred, bytes_transferred2);
  EXPECT_EQ(0, memcmp(&buffer[0], &buffer2[0], buffer.size()));

  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc));

  utils::file::delete_file(test_file);
}

static void rename_test(remote_client &client) {
  const auto test_file = utils::path::absolute("./win_remote/rename.txt");
  const auto test_file2 = utils::path::absolute("./win_remote/rename2.txt");
  utils::file::delete_file(test_file);
  utils::file::delete_file(test_file2);
  auto api_path = utils::string::from_utf8(test_file).substr(mount_location_.size());
  auto api_path2 = utils::string::from_utf8(test_file2).substr(mount_location_.size());

  PVOID file_desc = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  remote::file_info fi{};
  std::string normalized_name;
  BOOLEAN exists = 0;
  auto ret =
      client.winfsp_create(&api_path[0], 0, GENERIC_READ | GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, 0,
                           &file_desc, &fi, normalized_name, exists);
  EXPECT_EQ(STATUS_SUCCESS, ret);

  ret = client.winfsp_rename(file_desc, &api_path[0], &api_path2[0], 0);
  EXPECT_EQ(STATUS_SUCCESS, ret);
  EXPECT_TRUE(utils::file::is_file(test_file2));
  EXPECT_FALSE(utils::file::is_file(test_file));

  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc));

  utils::file::delete_file(test_file);
  utils::file::delete_file(test_file2);
}

static void set_basic_info_test(remote_client &client) {
  const auto test_file = utils::path::absolute("./win_remote/setbasicinfo.txt");
  utils::file::delete_file(test_file);
  utils::file::delete_file(test_file);
  auto api_path = utils::string::from_utf8(test_file).substr(mount_location_.size());

  PVOID file_desc = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  remote::file_info fi{};
  std::string normalized_name;
  BOOLEAN exists = 0;
  auto ret =
      client.winfsp_create(&api_path[0], 0, GENERIC_READ | GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, 0,
                           &file_desc, &fi, normalized_name, exists);
  EXPECT_EQ(STATUS_SUCCESS, ret);

  const auto attributes = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_ARCHIVE;
#ifdef _WIN32
  SYSTEMTIME st{};
  ::GetSystemTime(&st);

  LARGE_INTEGER ft{};
  ::SystemTimeToFileTime(&st, reinterpret_cast<FILETIME *>(&ft));

  const auto creation_time = ft.QuadPart;
  const auto last_access_time = ft.QuadPart + 1;
  const auto last_write_time = ft.QuadPart + 2;
  const auto change_time = last_write_time;
#else
  const auto creation_time = utils::unix_time_to_windows_time(utils::get_time_now());
  const auto last_access_time = creation_time + 1;
  const auto last_write_time = creation_time + 2;
  const auto change_time = last_write_time;
#endif

  EXPECT_EQ(STATUS_SUCCESS,
            client.winfsp_set_basic_info(file_desc, attributes, creation_time, last_access_time,
                                         last_write_time, change_time, &fi));
  EXPECT_EQ(attributes, fi.FileAttributes);
  EXPECT_EQ(creation_time, fi.CreationTime);
  EXPECT_EQ(last_access_time, fi.LastAccessTime);
  EXPECT_EQ(last_write_time, fi.LastWriteTime);
  EXPECT_EQ(change_time, fi.ChangeTime);

  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc));

  utils::file::delete_file(test_file);
}

static void set_file_size_test(remote_client &client) {
  const auto test_file = utils::path::absolute("./win_remote/setfilesize.txt");
  utils::file::delete_file(test_file);
  auto api_path = utils::string::from_utf8(test_file).substr(mount_location_.size());

  PVOID file_desc = reinterpret_cast<PVOID>(REPERTORY_INVALID_HANDLE);
  remote::file_info fi{};
  std::string normalized_name;
  BOOLEAN exists = 0;
  auto ret =
      client.winfsp_create(&api_path[0], 0, GENERIC_READ | GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, 0,
                           &file_desc, &fi, normalized_name, exists);
  EXPECT_EQ(STATUS_SUCCESS, ret);

  const UINT64 new_file_size = 34;
  const BOOLEAN set_allocation_size = 0;
  EXPECT_EQ(STATUS_SUCCESS,
            client.winfsp_set_file_size(file_desc, new_file_size, set_allocation_size, &fi));

  std::uint64_t file_size = 0;
  utils::file::get_file_size(test_file, file_size);
  EXPECT_EQ(34, file_size);

  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_close(file_desc));

  utils::file::delete_file(test_file);
}

static void unmounted_test(remote_client &client) {
  const auto location = utils::string::from_utf8(mount_location_);
  EXPECT_EQ(STATUS_SUCCESS, client.winfsp_unmounted(location));
}

TEST(remote_winfsp, all_tests) {
  std::uint16_t port = 0;
  const auto found_port = utils::get_next_available_port(20000u, port);
  EXPECT_TRUE(found_port);
  if (found_port) {
    console_consumer c;

    app_config config(provider_type::remote, "./win_remote");
    config.set_remote_host_name_or_ip("localhost");
    config.set_remote_port(port);
    config.set_remote_token("testtoken");
    config.set_enable_drive_events(true);
    config.set_event_level(event_level::verbose);

    event_system::instance().start();
#ifdef _WIN32
    mount_location_ = std::string(__FILE__).substr(0, 2);
    mock_winfsp_drive drive(mount_location_);
    remote_server server(config, drive, mount_location_);
#else
    mount_location_ = utils::path::absolute(".");
    mock_fuse_drive drive(mount_location_);
    remote_fuse::remote_server server(config, drive, mount_location_);
#endif
    std::thread([&]() {
      remote_client client(config);

      can_delete_test(client);
      cleanup_test(client);
      create_and_close_test(client);
      create_and_read_directory_test(client);
      flush_test(client);
      get_file_info_test(client);
      get_security_by_name_test(client);
      get_volume_info_test(client);
      mounted_test(client);
      open_and_read_directory_test(client);
      open_test(client);
      overwrite_test(client);
      read_and_write_test(client);
      rename_test(client);
      set_basic_info_test(client);
      set_file_size_test(client);
      unmounted_test(client);
    }).join();
  }

  event_system::instance().stop();
  utils::file::delete_directory_recursively("./win_remote");
}
} // namespace winfsp_test
