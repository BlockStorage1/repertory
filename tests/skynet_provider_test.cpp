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
#if defined(REPERTORY_ENABLE_SKYNET)

#include "fixtures/skynet_provider_fixture.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/encryption.hpp"

#define EXTERNAL_SKYLINK "AACoqIuN00YdDhS21dUMpMYFYGDeGmPnGoNWOkItkmzLfw"

namespace repertory {
static void populate_file_meta(const std::string &, api_meta_map &meta) {
  meta[META_ACCESSED] = std::to_string(utils::get_file_time_now());
  meta[META_MODIFIED] = std::to_string(utils::get_file_time_now());
  meta[META_CREATION] = std::to_string(utils::get_file_time_now());
}

TEST_F(skynet_provider_test, create_directory_and_create_file) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));
  EXPECT_TRUE(provider_->is_directory("/"));

  api_meta_map file_meta{};
  populate_file_meta("/test.txt", file_meta);
  EXPECT_EQ(api_error::success, provider_->create_file("/test.txt", file_meta));
  EXPECT_TRUE(provider_->is_file("/test.txt"));
  EXPECT_TRUE(provider_->is_file_writeable("/test.txt"));
}

TEST_F(skynet_provider_test, get_file) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));
  EXPECT_TRUE(provider_->is_directory("/"));

  api_meta_map file_meta{};
  populate_file_meta("/test.txt", file_meta);
  EXPECT_EQ(api_error::success, provider_->create_file("/test.txt", file_meta));
  EXPECT_TRUE(provider_->is_file("/test.txt"));

  api_file apiFile{};
  EXPECT_EQ(api_error::success, provider_->get_file("/test.txt", apiFile));

  EXPECT_STREQ("/test.txt", apiFile.api_path.c_str());
  EXPECT_STREQ("/", apiFile.api_parent.c_str());
  EXPECT_EQ(utils::string::to_uint64(file_meta[META_ACCESSED]), apiFile.accessed_date);
  EXPECT_EQ(utils::string::to_uint64(file_meta[META_MODIFIED]), apiFile.changed_date);
  EXPECT_EQ(utils::string::to_uint64(file_meta[META_CREATION]), apiFile.created_date);
  EXPECT_EQ(0, apiFile.file_size);
  EXPECT_EQ(utils::string::to_uint64(file_meta[META_MODIFIED]), apiFile.modified_date);
  EXPECT_TRUE(apiFile.recoverable);
  EXPECT_EQ(3.0, apiFile.redundancy);
  EXPECT_TRUE(utils::file::is_file(apiFile.source_path));
}

TEST_F(skynet_provider_test, get_directory_item_count) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));
  EXPECT_EQ(api_error::success, provider_->create_directory("/sub", directory_meta));

  api_meta_map file_meta{};
  populate_file_meta("/test.txt", file_meta);
  EXPECT_EQ(api_error::success, provider_->create_file("/test.txt", file_meta));

  EXPECT_EQ(2, provider_->get_directory_item_count("/"));
}

TEST_F(skynet_provider_test, get_directory_items) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));
  EXPECT_EQ(api_error::success, provider_->create_directory("/sub", directory_meta));

  api_meta_map file_meta{};
  populate_file_meta("/test.txt", file_meta);
  EXPECT_EQ(api_error::success, provider_->create_file("/test.txt", file_meta));

  directory_item_list itemList;
  EXPECT_EQ(api_error::success, provider_->get_directory_items("/", itemList));
  EXPECT_EQ(std::size_t(4), itemList.size());

  EXPECT_STREQ("/sub", itemList[2u].api_path.c_str());
  EXPECT_STREQ("/", itemList[2u].api_parent.c_str());
  EXPECT_TRUE(itemList[2u].directory);
  EXPECT_EQ(0u, itemList[2u].size);
  // itemList[0].MetaMap;

  EXPECT_STREQ("/test.txt", itemList[3u].api_path.c_str());
  EXPECT_STREQ("/", itemList[3u].api_parent.c_str());
  EXPECT_FALSE(itemList[3u].directory);
  EXPECT_EQ(0u, itemList[3u].size);
  // itemList[1].MetaMap;
}

TEST_F(skynet_provider_test, get_file_list) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));
  EXPECT_EQ(api_error::success, provider_->create_directory("/sub", directory_meta));

  api_meta_map file_meta{};
  populate_file_meta("/test.txt", file_meta);
  EXPECT_EQ(api_error::success, provider_->create_file("/test.txt", file_meta));
  populate_file_meta("/test2.txt", file_meta);
  EXPECT_EQ(api_error::success, provider_->create_file("/test2.txt", file_meta));

  api_file_list fileList;
  EXPECT_EQ(api_error::success, provider_->get_file_list(fileList));
  EXPECT_EQ(std::size_t(2), fileList.size());
}

TEST_F(skynet_provider_test, get_fileSize) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));

  api_meta_map file_meta{};
  populate_file_meta("/test.txt", file_meta);
  EXPECT_EQ(api_error::success, provider_->create_file("/test.txt", file_meta));

  std::uint64_t file_size = 100ull;
  EXPECT_EQ(api_error::success, provider_->get_file_size("/test.txt", file_size));
  EXPECT_EQ(std::uint64_t(0), file_size);
}

TEST_F(skynet_provider_test, get_host_config) {
  auto hc = provider_->get_host_config(false);
  EXPECT_TRUE(hc.agent_string.empty());
  EXPECT_TRUE(hc.api_password.empty());
  EXPECT_EQ(hc.api_port, 443);
  EXPECT_STREQ(DEFAULT_SKYNET_URLS[0u].c_str(), hc.host_name_or_ip.c_str());
  EXPECT_STREQ("/", hc.path.c_str());
  EXPECT_STREQ("https", hc.protocol.c_str());
  EXPECT_STREQ(DEFAULT_SKYNET_URLS[1u].c_str(), hc.auth_url.c_str());

  hc = provider_->get_host_config(true);
  EXPECT_TRUE(hc.agent_string.empty());
  EXPECT_TRUE(hc.api_password.empty());
  EXPECT_EQ(hc.api_port, 443);
  EXPECT_STREQ(DEFAULT_SKYNET_URLS[0u].c_str(), hc.host_name_or_ip.c_str());
  EXPECT_STREQ("/skynet/skyfile", hc.path.c_str());
  EXPECT_STREQ(DEFAULT_SKYNET_URLS[1u].c_str(), hc.auth_url.c_str());
  EXPECT_STREQ("https", hc.protocol.c_str());

  const auto string_list = config_->get_value_by_name("SkynetConfig.PortalList");
  json list = json::parse(string_list);
  list[0u]["AuthUser"] = "test_user";
  list[0u]["AuthURL"] = "test_url";
  list[0u]["AuthPassword"] = "test_pwd";

  config_->set_value_by_name("SkynetConfig.PortalList", list.dump());
  provider_->update_portal_list();

  hc = provider_->get_host_config(false);
  EXPECT_TRUE(hc.agent_string.empty());
  EXPECT_TRUE(hc.api_password.empty());
  EXPECT_EQ(hc.api_port, 443);
  EXPECT_STREQ(DEFAULT_SKYNET_URLS[0u].c_str(), hc.host_name_or_ip.c_str());
  EXPECT_STREQ("/", hc.path.c_str());
  EXPECT_STREQ("https", hc.protocol.c_str());
  EXPECT_STREQ("test_user", hc.auth_user.c_str());
  EXPECT_STREQ("test_url", hc.auth_url.c_str());
  EXPECT_STREQ("test_pwd", hc.auth_password.c_str());

  hc = provider_->get_host_config(true);
  EXPECT_TRUE(hc.agent_string.empty());
  EXPECT_TRUE(hc.api_password.empty());
  EXPECT_EQ(hc.api_port, 443);
  EXPECT_STREQ(DEFAULT_SKYNET_URLS[0u].c_str(), hc.host_name_or_ip.c_str());
  EXPECT_STREQ("/skynet/skyfile", hc.path.c_str());
  EXPECT_STREQ("https", hc.protocol.c_str());
  EXPECT_STREQ("test_user", hc.auth_user.c_str());
  EXPECT_STREQ("test_url", hc.auth_url.c_str());
  EXPECT_STREQ("test_pwd", hc.auth_password.c_str());
}

TEST_F(skynet_provider_test, remove_directory) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));
  EXPECT_EQ(api_error::success, provider_->create_directory("/sub", directory_meta));
  EXPECT_TRUE(provider_->is_directory("/sub"));

  EXPECT_EQ(api_error::success, provider_->remove_directory("/sub"));
  EXPECT_FALSE(provider_->is_directory("/sub"));
}

TEST_F(skynet_provider_test, recreate_directory_after_remove_directory) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));
  EXPECT_EQ(api_error::success, provider_->create_directory("/sub", directory_meta));
  EXPECT_TRUE(provider_->is_directory("/sub"));

  EXPECT_EQ(api_error::success, provider_->remove_directory("/sub"));
  EXPECT_FALSE(provider_->is_directory("/sub"));

  EXPECT_EQ(api_error::success, provider_->create_directory("/sub", directory_meta));
  EXPECT_TRUE(provider_->is_directory("/sub"));
}

TEST_F(skynet_provider_test, remove_directory_fails_if_sub_directories_exist) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));
  EXPECT_EQ(api_error::success, provider_->create_directory("/sub", directory_meta));
  EXPECT_EQ(api_error::success, provider_->create_directory("/sub/sub2", directory_meta));
  EXPECT_TRUE(provider_->is_directory("/sub/sub2"));

  EXPECT_EQ(api_error::directory_not_empty, provider_->remove_directory("/sub"));
  EXPECT_TRUE(provider_->is_directory("/sub"));
  EXPECT_TRUE(provider_->is_directory("/sub/sub2"));
}

TEST_F(skynet_provider_test, remove_directory_fails_if_files_exist) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));
  EXPECT_EQ(api_error::success, provider_->create_directory("/sub", directory_meta));
  EXPECT_TRUE(provider_->is_directory("/sub"));

  api_meta_map file_meta{};
  populate_file_meta("/sub/test.txt", file_meta);
  EXPECT_EQ(api_error::success, provider_->create_file("/sub/test.txt", file_meta));

  EXPECT_EQ(api_error::directory_not_empty, provider_->remove_directory("/sub"));
  EXPECT_TRUE(provider_->is_directory("/sub"));
  EXPECT_TRUE(provider_->is_file("/sub/test.txt"));
}

TEST_F(skynet_provider_test, remove_directory_fails_for_root_directory) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));

  EXPECT_EQ(api_error::access_denied, provider_->remove_directory("/"));
  EXPECT_TRUE(provider_->is_directory("/"));
}

TEST_F(skynet_provider_test, remove_file) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));
  EXPECT_TRUE(provider_->is_directory("/"));

  api_meta_map file_meta{};
  populate_file_meta("/test.txt", file_meta);
  EXPECT_EQ(api_error::success, provider_->create_file("/test.txt", file_meta));
  EXPECT_TRUE(provider_->is_file("/test.txt"));

  EXPECT_EQ(api_error::success, provider_->remove_file("/test.txt"));
  EXPECT_FALSE(provider_->is_file("/test.txt"));
}

TEST_F(skynet_provider_test, recreate_file_after_remove_file) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));
  EXPECT_TRUE(provider_->is_directory("/"));

  api_meta_map file_meta{};
  populate_file_meta("/test.txt", file_meta);
  EXPECT_EQ(api_error::success, provider_->create_file("/test.txt", file_meta));
  EXPECT_TRUE(provider_->is_file("/test.txt"));

  EXPECT_EQ(api_error::success, provider_->remove_file("/test.txt"));
  EXPECT_FALSE(provider_->is_file("/test.txt"));

  EXPECT_EQ(api_error::success, provider_->create_file("/test.txt", file_meta));
  EXPECT_TRUE(provider_->is_file("/test.txt"));
}

TEST_F(skynet_provider_test, rename_file) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));

  api_meta_map file_meta{};
  populate_file_meta("/test.txt", file_meta);
  EXPECT_EQ(api_error::success, provider_->create_file("/test.txt", file_meta));
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/test.txt", file_meta));

  EXPECT_EQ(api_error::success, provider_->rename_file("/test.txt", "/test2.txt"));
  EXPECT_TRUE(provider_->is_file("/test2.txt"));
  EXPECT_FALSE(provider_->is_file("/test.txt"));

  api_meta_map fileMeta2{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/test2.txt", fileMeta2));
  EXPECT_EQ(file_meta.size(), fileMeta2.size());
  for (const auto &kv : file_meta) {
    EXPECT_STREQ(file_meta[kv.first].c_str(), fileMeta2[kv.first].c_str());
  }
}

TEST_F(skynet_provider_test, upload_file_and_read_file_bytes) {
  config_->set_value_by_name("SkynetConfig.EncryptionToken", "");

  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));

  api_meta_map file_meta{};
  populate_file_meta("/test.txt", file_meta);
  EXPECT_EQ(api_error::success, provider_->create_file("/test.txt", file_meta));

  filesystem_item fsi{};
  EXPECT_EQ(api_error::success, provider_->get_filesystem_item("/test.txt", false, fsi));
  EXPECT_TRUE(fsi.encryption_token.empty());
  EXPECT_FALSE(fsi.is_encrypted());

  json j = {{"test", "test"}, {"test2", "test"}};
  EXPECT_TRUE(utils::file::write_json_file(fsi.source_path, j));

  EXPECT_EQ(api_error::success,
            provider_->upload_file(fsi.api_path, fsi.source_path, fsi.encryption_token));
  while (provider_->is_processing(fsi.api_path)) {
    std::this_thread::sleep_for(1ms);
  }

  std::string id;
  EXPECT_EQ(api_error::success, provider_->get_item_meta(fsi.api_path, META_ID, id));

  json skynet_info = json::parse(id);
  std::cout << skynet_info.dump(2) << std::endl;
  EXPECT_NE(0u, skynet_info["skylink"].size());

  std::uint64_t file_size = 0;
  utils::file::get_file_size(fsi.source_path, file_size);

  std::vector<char> data;
  auto stop_requested = false;
  EXPECT_EQ(api_error::success,
            provider_->read_file_bytes("/test.txt", static_cast<std::size_t>(file_size), 0, data,
                                       stop_requested));
  EXPECT_EQ(file_size, std::uint64_t(data.size()));
  EXPECT_STREQ(j.dump().c_str(), json::parse(std::string(&data[0], data.size())).dump().c_str());

  http_ranges ranges = {{0, 0}};
  http_headers headers;
  json error;
  EXPECT_EQ(api_error::success,
            curl_comm_->get_range_and_headers(
                provider_->get_host_config(false), "/" + skynet_info["skylink"].get<std::string>(),
                0u, {{"format", "concat"}}, "", data, ranges, error, headers, stop_requested));
  for (const auto &header : headers) {
    std::cout << header.first << ":" << header.second << std::endl;
  }
  json meta_data;
  EXPECT_EQ(api_error::success,
            provider_->get_skynet_metadata(skynet_info["skylink"].get<std::string>(), meta_data));
  EXPECT_STREQ(meta_data["filename"].get<std::string>().c_str(), "test.txt");
}

TEST_F(skynet_provider_test, upload_encrypted_file_and_read_file_bytes) {
  config_->set_value_by_name("SkynetConfig.EncryptionToken", "TestToken");
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));

  api_meta_map file_meta{};
  populate_file_meta("/test.txt", file_meta);
  EXPECT_EQ(api_error::success, provider_->create_file("/test.txt", file_meta));

  filesystem_item fsi{};
  EXPECT_EQ(api_error::success, provider_->get_filesystem_item("/test.txt", false, fsi));
  EXPECT_STREQ("TestToken", fsi.encryption_token.c_str());
  EXPECT_TRUE(fsi.is_encrypted());

  const auto file_size = 2u * utils::encryption::encrypting_reader::get_data_chunk_size() + 3u;
  auto source_file = create_random_file(fsi.source_path, file_size);

  EXPECT_EQ(api_error::success,
            provider_->upload_file(fsi.api_path, fsi.source_path, fsi.encryption_token));
  while (provider_->is_processing(fsi.api_path)) {
    std::this_thread::sleep_for(1ms);
  }

  std::string id;
  EXPECT_EQ(api_error::success, provider_->get_item_meta(fsi.api_path, META_ID, id));

  json skynet_info = json::parse(id);
  std::cout << skynet_info.dump(2) << std::endl;
  EXPECT_NE(0u, skynet_info["skylink"].size());

  auto stop_requested = false;
  const auto size = file_size / 3u;
  const auto size_remain = file_size % 3u;
  for (std::uint8_t i = 0u; i < 3u; i++) {
    const auto read_size = size + (i == 2u ? size_remain : 0u);
    std::vector<char> data;
    EXPECT_EQ(api_error::success,
              provider_->read_file_bytes("/test.txt", read_size, i * size, data, stop_requested));

    std::size_t bytes_read{};
    std::vector<char> b(read_size);
    EXPECT_TRUE(source_file->read_bytes(&b[0u], b.size(), i * size, bytes_read));
    EXPECT_EQ(b.size(), read_size);
    EXPECT_EQ(0, std::memcmp(&data[0u], &b[0u], data.size()));
  }
  source_file->close();

  http_ranges ranges = {{0, 0}};
  http_headers headers;
  json error;
  auto err = api_error::error;
  for (std::size_t j = 0u; (err != api_error::success) && (j < config_->get_retry_read_count());
       j++) {
    std::vector<char> data;
    err = curl_comm_->get_range_and_headers(
        provider_->get_host_config(false), "/" + skynet_info["skylink"].get<std::string>(), 0u,
        {{"format", "concat"}}, "", data, ranges, error, headers, stop_requested);
  }

  EXPECT_EQ(api_error::success, err);
  for (const auto &header : headers) {
    std::cout << header.first << ":" << header.second << std::endl;
  }

  json meta_data;
  EXPECT_EQ(api_error::success,
            provider_->get_skynet_metadata(skynet_info["skylink"].get<std::string>(), meta_data));

  const auto encrypted_file_name = meta_data["filename"].get<std::string>();
  std::vector<char> buffer;
  EXPECT_TRUE(utils::from_hex_string(encrypted_file_name, buffer));

  std::string file_name;
  decrypt_and_verify(buffer, fsi.encryption_token, file_name);
  EXPECT_STREQ("test.txt", file_name.c_str());

  config_->set_value_by_name("SkynetConfig.EncryptionToken", "");
}

TEST_F(skynet_provider_test, upload_tiny_encrypted_file_and_read_file_bytes) {
  config_->set_value_by_name("SkynetConfig.EncryptionToken", "TestToken");
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));

  api_meta_map file_meta{};
  populate_file_meta("/test.txt", file_meta);
  EXPECT_EQ(api_error::success, provider_->create_file("/test.txt", file_meta));

  filesystem_item fsi{};
  EXPECT_EQ(api_error::success, provider_->get_filesystem_item("/test.txt", false, fsi));
  EXPECT_STREQ("TestToken", fsi.encryption_token.c_str());
  EXPECT_TRUE(fsi.is_encrypted());

  json j = {{"test", "test"}, {"test2", "test"}};
  EXPECT_TRUE(utils::file::write_json_file(fsi.source_path, j));

  EXPECT_EQ(api_error::success,
            provider_->upload_file(fsi.api_path, fsi.source_path, fsi.encryption_token));
  while (provider_->is_processing(fsi.api_path)) {
    std::this_thread::sleep_for(1ms);
  }

  std::string id;
  EXPECT_EQ(api_error::success, provider_->get_item_meta(fsi.api_path, META_ID, id));

  json skynet_info = json::parse(id);
  std::cout << skynet_info.dump(2) << std::endl;
  EXPECT_NE(0u, skynet_info["skylink"].size());

  std::uint64_t file_size = 0;
  utils::file::get_file_size(fsi.source_path, file_size);

  std::vector<char> data;
  auto stop_requested = false;
  EXPECT_EQ(api_error::success,
            provider_->read_file_bytes("/test.txt", static_cast<std::size_t>(file_size), 0, data,
                                       stop_requested));
  EXPECT_EQ(file_size, std::uint64_t(data.size()));

  const auto str = std::string(&data[0u], data.size());
  EXPECT_STREQ(j.dump().c_str(), json::parse(str).dump().c_str());

  http_ranges ranges = {{0, 0}};
  http_headers headers;
  json error;
  auto apiError = api_error::error;
  for (std::size_t i = 0; (apiError != api_error::success) && (i < config_->get_retry_read_count());
       i++) {
    apiError = curl_comm_->get_range_and_headers(
        provider_->get_host_config(false), "/" + skynet_info["skylink"].get<std::string>(), 0u,
        {{"format", "concat"}}, "", data, ranges, error, headers, stop_requested);
  }

  EXPECT_EQ(api_error::success, apiError);
  for (const auto &header : headers) {
    std::cout << header.first << ":" << header.second << std::endl;
  }

  json meta_data;
  EXPECT_EQ(api_error::success,
            provider_->get_skynet_metadata(skynet_info["skylink"].get<std::string>(), meta_data));

  const auto encrypted_file_name = meta_data["filename"].get<std::string>();
  std::vector<char> buffer;
  EXPECT_TRUE(utils::from_hex_string(encrypted_file_name, buffer));

  std::string file_name;
  decrypt_and_verify(buffer, fsi.encryption_token, file_name);
  EXPECT_STREQ("test.txt", file_name.c_str());

  config_->set_value_by_name("SkynetConfig.EncryptionToken", "");
}

TEST_F(skynet_provider_test, import_and_export) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));

  skylink_import sl{"", "", "AACeCiD6WQG6DzDcCdIu3cFPSxMUMoQPx46NYSyijNMKUA", ""};
  EXPECT_EQ(api_error::success, provider_->import_skylink(sl));
  EXPECT_TRUE(provider_->is_file("/repertory_test_import.txt"));

  std::uint64_t file_size = 0u;
  EXPECT_EQ(api_error::success, provider_->get_file_size("/repertory_test_import.txt", file_size));
  EXPECT_EQ(std::uint64_t(3u), file_size);

  api_meta_map meta;
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/repertory_test_import.txt", meta));

  const auto id = json::parse(meta[META_ID]);
  EXPECT_STREQ("AACeCiD6WQG6DzDcCdIu3cFPSxMUMoQPx46NYSyijNMKUA",
               id["skylink"].get<std::string>().c_str());

  const auto test_success = [](json &result) {
    EXPECT_EQ(std::size_t(1u), result["success"].size());

    auto i = skylink_import::from_json(result["success"][0u]);
    EXPECT_STREQ("/", i.directory.c_str());
    EXPECT_STREQ("repertory_test_import.txt", i.file_name.c_str());
    EXPECT_STREQ("AACeCiD6WQG6DzDcCdIu3cFPSxMUMoQPx46NYSyijNMKUA", i.skylink.c_str());
    EXPECT_STREQ("", i.token.c_str());
  };

  {
    auto export_with_failure =
        provider_->export_list({"/repertory_test_import.txt", "/repertory_test_import2.txt"});
    std::cout << export_with_failure.dump(2) << std::endl;
    EXPECT_EQ(std::size_t(1), export_with_failure["failed"].size());
    EXPECT_STREQ("/repertory_test_import2.txt",
                 export_with_failure["failed"][0u].get<std::string>().c_str());

    test_success(export_with_failure);
  }

  {
    auto export_all = provider_->export_all();
    std::cout << export_all.dump(2) << std::endl;
    EXPECT_TRUE(export_all["failed"].empty());

    test_success(export_all);
  }
  provider_->remove_file("/repertory_test_import.txt");
}

TEST_F(skynet_provider_test, import_and_export_with_different_directory) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));

  skylink_import sl{"/test/sub", "", "AACeCiD6WQG6DzDcCdIu3cFPSxMUMoQPx46NYSyijNMKUA", ""};
  EXPECT_EQ(api_error::success, provider_->import_skylink(sl));
  EXPECT_TRUE(provider_->is_directory("/test"));
  EXPECT_TRUE(provider_->is_directory("/test/sub"));
  EXPECT_TRUE(provider_->is_file("/test/sub/repertory_test_import.txt"));

  std::uint64_t file_size = 0;
  EXPECT_EQ(api_error::success,
            provider_->get_file_size("/test/sub/repertory_test_import.txt", file_size));
  EXPECT_EQ(std::uint64_t(3), file_size);

  api_meta_map meta;
  EXPECT_EQ(api_error::success,
            provider_->get_item_meta("/test/sub/repertory_test_import.txt", meta));

  const auto id = json::parse(meta[META_ID]);
  EXPECT_STREQ("AACeCiD6WQG6DzDcCdIu3cFPSxMUMoQPx46NYSyijNMKUA",
               id["skylink"].get<std::string>().c_str());

  const auto test_success = [](json &result) {
    EXPECT_EQ(std::size_t(1), result["success"].size());

    auto i = skylink_import::from_json(result["success"][0u]);
    EXPECT_STREQ("/test/sub", i.directory.c_str());
    EXPECT_STREQ("repertory_test_import.txt", i.file_name.c_str());
    EXPECT_STREQ("AACeCiD6WQG6DzDcCdIu3cFPSxMUMoQPx46NYSyijNMKUA", i.skylink.c_str());
    EXPECT_STREQ("", i.token.c_str());
  };

  {
    auto export_with_failure = provider_->export_list(
        {"/test/sub/repertory_test_import.txt", "/test/sub/repertory_test_import2.txt"});
    std::cout << export_with_failure.dump(2) << std::endl;
    EXPECT_EQ(std::size_t(1), export_with_failure["failed"].size());
    EXPECT_STREQ("/test/sub/repertory_test_import2.txt",
                 export_with_failure["failed"][0u].get<std::string>().c_str());

    test_success(export_with_failure);
  }

  {
    auto export_all = provider_->export_all();
    std::cout << export_all.dump(2) << std::endl;
    EXPECT_TRUE(export_all["failed"].empty());

    test_success(export_all);
  }

  provider_->remove_file("/test/sub/repertory_test_import.txt");
}

TEST_F(skynet_provider_test, import_directory) {
  const auto expected_json_data = json::parse(
      R"({ "filename": "skynet_test", "length": 36, "subfiles": { "sub_dir/test_03": { "contenttype": "application/octet-stream", "filename": "sub_dir/test_03", "len": 9, "offset": 18 }, "sub_dir/test_04": { "contenttype": "application/octet-stream", "filename": "sub_dir/test_04", "len": 9, "offset": 27 }, "test_01": { "contenttype": "application/octet-stream", "filename": "test_01", "len": 9 }, "test_02": { "contenttype": "application/octet-stream", "filename": "test_02", "len": 9, "offset": 9 } }, "tryfiles": [ "index.html" ] })");
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));

  skylink_import sl{"", "", EXTERNAL_SKYLINK, ""};
  EXPECT_EQ(api_error::success, provider_->import_skylink(sl));

  for (const auto &subFile : expected_json_data["subfiles"]) {
    const auto api_path = utils::path::create_api_path(
        utils::path::combine("/", {subFile["filename"].get<std::string>()}));
    EXPECT_TRUE(provider_->is_file(api_path));

    std::uint64_t file_size = 0;
    EXPECT_EQ(api_error::success, provider_->get_file_size(api_path, file_size));
    EXPECT_EQ(subFile["len"].get<std::uint64_t>(), file_size);

    api_meta_map meta;
    EXPECT_EQ(api_error::success, provider_->get_item_meta(api_path, meta));

    const auto id = json::parse(meta[META_ID]);
    const auto skylink = sl.skylink + api_path;
    EXPECT_STREQ(skylink.c_str(), id["skylink"].get<std::string>().c_str());
  }
}

TEST_F(skynet_provider_test, rename_file_fails_on_skylinks_with_directory_paths) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));
  EXPECT_EQ(api_error::success, provider_->create_directory("/test_rename2", {}));
  EXPECT_EQ(api_error::success, provider_->create_directory("/test_rename2/static", {}));
  EXPECT_EQ(api_error::success, provider_->create_directory("/test_rename2/static/css", {}));

  skylink_import sl{"/test_rename", "", EXTERNAL_SKYLINK, ""};
  EXPECT_EQ(api_error::success, provider_->import_skylink(sl));
  if (not ::testing::Test::HasFailure()) {
    EXPECT_EQ(api_error::access_denied,
              provider_->rename_file("/test_rename/test_01", "/test_rename/test_01_"));
    EXPECT_EQ(api_error::access_denied, provider_->rename_file("/test_rename/sub_dir/test_03",
                                                               "/test_rename/sub_dir/test_03_"));

    EXPECT_EQ(api_error::access_denied,
              provider_->rename_file("/test_rename/test_01", "/test_rename2/test_01_"));
    EXPECT_EQ(api_error::access_denied, provider_->rename_file("/test_rename/sub_dir/test_03",
                                                               "/test_rename2/sub_dir/test_03_"));

    EXPECT_EQ(api_error::success, provider_->create_directory("/test_rename/a", {}));
    EXPECT_EQ(api_error::success, provider_->create_directory("/test_rename/sub_dir/b", {}));

    EXPECT_EQ(api_error::access_denied,
              provider_->rename_file("/test_rename/test_01", "/test_rename/a/test_01_"));

    EXPECT_EQ(api_error::access_denied, provider_->rename_file("/test_rename/sub_dir/test_03",
                                                               "/test_rename/static/b/test_03_"));
  }
}

TEST_F(skynet_provider_test, rename_file_succeeds_with_logical_directory_paths) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));
  EXPECT_EQ(api_error::success, provider_->create_directory("/test_rename2", {}));
  EXPECT_EQ(api_error::success, provider_->create_directory("/test_rename2/sub_dir", {}));

  skylink_import sl{"/test_rename", "", EXTERNAL_SKYLINK, ""};
  EXPECT_EQ(api_error::success, provider_->import_skylink(sl));
  if (not ::testing::Test::HasFailure()) {
    EXPECT_EQ(api_error::success,
              provider_->rename_file("/test_rename/test_01", "/test_rename2/test_01"));
    EXPECT_EQ(api_error::success, provider_->rename_file("/test_rename/sub_dir/test_03",
                                                         "/test_rename2/sub_dir/test_03"));
  }
}

TEST_F(skynet_provider_test, export_with_nested_directory_paths) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));

  skylink_import sl{"/test_export/nested", "", EXTERNAL_SKYLINK, ""};
  EXPECT_EQ(api_error::success, provider_->import_skylink(sl));
  if (not ::testing::Test::HasFailure()) {
    const auto json_data = provider_->export_all();
    for (const auto &e : json_data["success"]) {
      EXPECT_STREQ("/test_export/nested", e["directory"].get<std::string>().c_str());
    }
  }
}

TEST_F(skynet_provider_test, export_with_nested_directory_paths_in_root) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));

  skylink_import sl{"/", "", EXTERNAL_SKYLINK, ""};
  EXPECT_EQ(api_error::success, provider_->import_skylink(sl));
  if (not ::testing::Test::HasFailure()) {
    const auto json_data = provider_->export_all();
    for (const auto &e : json_data["success"]) {
      EXPECT_STREQ("/", e["directory"].get<std::string>().c_str());
    }
  }
}

TEST_F(skynet_provider_test, is_file_writeable_is_false_for_nested_skylinks) {
  api_meta_map directory_meta{};
  EXPECT_EQ(api_error::success, provider_->get_item_meta("/", directory_meta));

  skylink_import sl{"/", "", EXTERNAL_SKYLINK, ""};
  EXPECT_EQ(api_error::success, provider_->import_skylink(sl));
  if (not ::testing::Test::HasFailure()) {
    api_file_list list{};
    provider_->get_file_list(list);
    for (const auto &file : list) {
      EXPECT_FALSE(provider_->is_file_writeable(file.api_path));
    }
  }
}

#ifdef REPERTORY_ENABLE_SKYNET_PREMIUM_TESTS
TEST_F(skynet_provider_test, authenticated_upload) {
  app_config config(provider_type::skynet, "../..");
  auto sk = config.get_skynet_config();
  EXPECT_FALSE(sk.portal_list[0u].auth_user.empty());
  sk.portal_list[0u].path = "/skynet/skyfile";
  curl_comm comm(config);

  const auto source_file = __FILE__;
  const auto file_name = utils::path::strip_to_file_name(source_file);
  auto id = utils::create_uuid_string();
  utils::string::replace(id, "-", "");

  json data, error;
  auto sr = false;
  EXPECT_EQ(api_error::success,
            comm.post_multipart_file(sk.portal_list.at(0), "", file_name, source_file, "repertory",
                                     data, error, sr));
  std::cout << data.dump(2) << std::endl;
  std::cout << error.dump(2) << std::endl;
}

TEST_F(skynet_provider_test, authenticated_upload_access_denied_with_invalid_username) {
  app_config config(provider_type::skynet, "../..");
  auto sk = config.get_skynet_config();
  sk.portal_list[0u].auth_user = "cowaoeutnhsaoetuh@aosetuh.com";
  sk.portal_list[0u].path = "/skynet/skyfile";
  curl_comm comm(config);

  const auto source_file = __FILE__;
  const auto file_name = utils::path::strip_to_file_name(source_file);
  auto id = utils::create_uuid_string();
  utils::string::replace(id, "-", "");

  json data, error;
  auto sr = false;
  EXPECT_EQ(api_error::access_denied,
            comm.post_multipart_file(sk.portal_list.at(0), "", file_name, source_file, "repertory",
                                     data, error, sr));
  std::cout << data.dump(2) << std::endl;
  std::cout << error.dump(2) << std::endl;
}

TEST_F(skynet_provider_test, authenticated_upload_access_denied_with_invalid_password) {
  app_config config(provider_type::skynet, "../..");
  auto sk = config.get_skynet_config();
  EXPECT_FALSE(sk.portal_list[0u].auth_user.empty());

  sk.portal_list[0u].auth_password = "1";
  sk.portal_list[0u].path = "/skynet/skyfile";
  curl_comm comm(config);

  const auto source_file = __FILE__;
  const auto file_name = utils::path::strip_to_file_name(source_file);
  auto id = utils::create_uuid_string();
  utils::string::replace(id, "-", "");

  json data, error;
  auto sr = false;
  EXPECT_EQ(api_error::access_denied,
            comm.post_multipart_file(sk.portal_list.at(0), "", file_name, source_file, "repertory",
                                     data, error, sr));
  std::cout << data.dump(2) << std::endl;
  std::cout << error.dump(2) << std::endl;
}
#endif // REPERTORY_ENABLE_SKYNET_PREMIUM_TESTS
} // namespace repertory

// TODO Test import without encryption token when global encryption token is set
// TODO Test encrypted file import
// TODO Test encrypted directory import
#endif // REPERTORY_ENABLE_SKYNET
