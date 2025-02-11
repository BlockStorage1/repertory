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
#include "test_common.hpp"

#include "types/remote.hpp"
#include "types/repertory.hpp"

namespace repertory {
TEST(json_serialize, can_handle_directory_item) {
  directory_item cfg{
      "api", "parent", true, 2U, {{META_DIRECTORY, "true"}},
  };

  json data(cfg);
  EXPECT_STREQ("api", data.at(JSON_API_PATH).get<std::string>().c_str());
  EXPECT_STREQ("parent", data.at(JSON_API_PARENT).get<std::string>().c_str());
  EXPECT_TRUE(data.at(JSON_DIRECTORY).get<bool>());
  EXPECT_STREQ(
      "true", data.at(JSON_META).at(META_DIRECTORY).get<std::string>().c_str());

  {
    auto cfg2 = data.get<directory_item>();
    EXPECT_STREQ(cfg2.api_path.c_str(), cfg.api_path.c_str());
    EXPECT_STREQ(cfg2.api_parent.c_str(), cfg.api_parent.c_str());
    EXPECT_EQ(cfg2.directory, cfg.directory);
    EXPECT_STREQ(cfg2.meta.at(META_DIRECTORY).c_str(),
                 cfg.meta.at(META_DIRECTORY).c_str());
  }
}

TEST(json_serialize, can_handle_encrypt_config) {
  encrypt_config cfg{
      "token",
      "path",
  };

  json data(cfg);
  EXPECT_STREQ("token",
               data.at(JSON_ENCRYPTION_TOKEN).get<std::string>().c_str());
  EXPECT_STREQ("path", data.at(JSON_PATH).get<std::string>().c_str());

  {
    auto cfg2 = data.get<encrypt_config>();
    EXPECT_STREQ(cfg2.encryption_token.c_str(), cfg.encryption_token.c_str());
    EXPECT_STREQ(cfg2.path.c_str(), cfg.path.c_str());
  }
}

TEST(json_serialize, can_handle_host_config) {
  host_config cfg{
      "agent", "pwd", "user", 1024U, "host", "path", "http", 11U,
  };

  json data(cfg);
  EXPECT_STREQ("agent", data.at(JSON_AGENT_STRING).get<std::string>().c_str());
  EXPECT_STREQ("pwd", data.at(JSON_API_PASSWORD).get<std::string>().c_str());
  EXPECT_STREQ("user", data.at(JSON_API_USER).get<std::string>().c_str());
  EXPECT_EQ(1024U, data.at(JSON_API_PORT).get<std::uint16_t>());
  EXPECT_STREQ("host",
               data.at(JSON_HOST_NAME_OR_IP).get<std::string>().c_str());
  EXPECT_STREQ("path", data.at(JSON_PATH).get<std::string>().c_str());
  EXPECT_STREQ("http", data.at(JSON_PROTOCOL).get<std::string>().c_str());
  EXPECT_EQ(11U, data.at(JSON_TIMEOUT_MS).get<std::uint16_t>());

  {
    auto cfg2 = data.get<host_config>();
    EXPECT_STREQ(cfg2.agent_string.c_str(), cfg.agent_string.c_str());
    EXPECT_STREQ(cfg2.api_password.c_str(), cfg.api_password.c_str());
    EXPECT_STREQ(cfg2.api_user.c_str(), cfg.api_user.c_str());
    EXPECT_EQ(cfg2.api_port, cfg.api_port);
    EXPECT_STREQ(cfg2.host_name_or_ip.c_str(), cfg.host_name_or_ip.c_str());
    EXPECT_STREQ(cfg2.path.c_str(), cfg.path.c_str());
    EXPECT_STREQ(cfg2.protocol.c_str(), cfg.protocol.c_str());
    EXPECT_EQ(cfg2.timeout_ms, cfg.timeout_ms);
  }
}

TEST(json_serialize, can_handle_remote_config) {
  remote::remote_config cfg{
      1024U, "token", "host", 11U, 20U, 21U,
  };

  json data(cfg);
  EXPECT_EQ(1024U, data.at(JSON_API_PORT).get<std::uint16_t>());
  EXPECT_STREQ("token",
               data.at(JSON_ENCRYPTION_TOKEN).get<std::string>().c_str());
  EXPECT_STREQ("host",
               data.at(JSON_HOST_NAME_OR_IP).get<std::string>().c_str());
  EXPECT_EQ(11U, data.at(JSON_MAX_CONNECTIONS).get<std::uint16_t>());
  EXPECT_EQ(20U, data.at(JSON_RECV_TIMEOUT_MS).get<std::uint32_t>());
  EXPECT_EQ(21U, data.at(JSON_SEND_TIMEOUT_MS).get<std::uint32_t>());

  {
    auto cfg2 = data.get<remote::remote_config>();
    EXPECT_EQ(cfg2.api_port, cfg.api_port);
    EXPECT_STREQ(cfg2.encryption_token.c_str(), cfg.encryption_token.c_str());
    EXPECT_STREQ(cfg2.host_name_or_ip.c_str(), cfg.host_name_or_ip.c_str());
    EXPECT_EQ(cfg2.max_connections, cfg.max_connections);
    EXPECT_EQ(cfg2.recv_timeout_ms, cfg.recv_timeout_ms);
    EXPECT_EQ(cfg2.send_timeout_ms, cfg.send_timeout_ms);
  }
}

TEST(json_serialize, can_handle_remote_mount) {
  remote::remote_mount cfg{1024U, 21U, true, "token"};

  json data(cfg);
  EXPECT_EQ(1024U, data.at(JSON_API_PORT).get<std::uint16_t>());
  EXPECT_EQ(21U, data.at(JSON_CLIENT_POOL_SIZE).get<std::uint16_t>());
  EXPECT_TRUE(data.at(JSON_ENABLE_REMOTE_MOUNT).get<bool>());
  EXPECT_STREQ("token",
               data.at(JSON_ENCRYPTION_TOKEN).get<std::string>().c_str());

  {
    auto cfg2 = data.get<remote::remote_mount>();
    EXPECT_EQ(cfg2.api_port, cfg.api_port);
    EXPECT_EQ(cfg2.client_pool_size, cfg.client_pool_size);
    EXPECT_EQ(cfg2.enable, cfg.enable);
    EXPECT_STREQ(cfg2.encryption_token.c_str(), cfg.encryption_token.c_str());
  }
}

TEST(json_serialize, can_handle_s3_config) {
  s3_config cfg{
      "access", "bucket", "token", "region", "secret", 31U, "url", true, false,
  };

  json data(cfg);
  EXPECT_STREQ("access", data.at(JSON_ACCESS_KEY).get<std::string>().c_str());
  EXPECT_STREQ("bucket", data.at(JSON_BUCKET).get<std::string>().c_str());
  EXPECT_STREQ("token",
               data.at(JSON_ENCRYPTION_TOKEN).get<std::string>().c_str());
  EXPECT_STREQ("region", data.at(JSON_REGION).get<std::string>().c_str());
  EXPECT_STREQ("secret", data.at(JSON_SECRET_KEY).get<std::string>().c_str());
  EXPECT_EQ(31U, data.at(JSON_TIMEOUT_MS).get<std::uint32_t>());
  EXPECT_STREQ("url", data.at(JSON_URL).get<std::string>().c_str());
  EXPECT_TRUE(data.at(JSON_USE_PATH_STYLE).get<bool>());
  EXPECT_FALSE(data.at(JSON_USE_REGION_IN_URL).get<bool>());

  {
    auto cfg2 = data.get<s3_config>();
    EXPECT_STREQ(cfg2.access_key.c_str(), cfg.access_key.c_str());
    EXPECT_STREQ(cfg2.bucket.c_str(), cfg.bucket.c_str());
    EXPECT_STREQ(cfg2.encryption_token.c_str(), cfg.encryption_token.c_str());
    EXPECT_STREQ(cfg2.region.c_str(), cfg.region.c_str());
    EXPECT_STREQ(cfg2.secret_key.c_str(), cfg.secret_key.c_str());
    EXPECT_EQ(cfg2.timeout_ms, cfg.timeout_ms);
    EXPECT_STREQ(cfg2.url.c_str(), cfg.url.c_str());
    EXPECT_EQ(cfg2.use_path_style, cfg.use_path_style);
    EXPECT_EQ(cfg2.use_region_in_url, cfg.use_region_in_url);
  }
}

TEST(json_serialize, can_handle_sia_config) {
  sia_config cfg{
      "bucket",
  };

  json data(cfg);
  EXPECT_STREQ("bucket", data.at(JSON_BUCKET).get<std::string>().c_str());

  {
    auto cfg2 = data.get<sia_config>();
    EXPECT_STREQ(cfg2.bucket.c_str(), cfg.bucket.c_str());
  }
}

TEST(json_serialize, can_handle_atomic) {
  atomic<sia_config> cfg({
      "bucket",
  });

  json data(cfg);
  EXPECT_STREQ("bucket", data.at(JSON_BUCKET).get<std::string>().c_str());

  {
    auto cfg2 = data.get<atomic<sia_config>>();
    EXPECT_STREQ(cfg2.load().bucket.c_str(), cfg.load().bucket.c_str());
  }
}

TEST(json_serialize, can_handle_database_type) {
  json data(database_type::rocksdb);
  EXPECT_EQ(database_type::rocksdb, data.get<database_type>());
  EXPECT_STREQ("rocksdb", data.get<std::string>().c_str());

  data = database_type::sqlite;
  EXPECT_EQ(database_type::sqlite, data.get<database_type>());
  EXPECT_STREQ("sqlite", data.get<std::string>().c_str());
}

TEST(json_serialize, can_handle_download_type) {
  json data(download_type::direct);
  EXPECT_EQ(download_type::direct, data.get<download_type>());
  EXPECT_STREQ("direct", data.get<std::string>().c_str());

  data = download_type::default_;
  EXPECT_EQ(download_type::default_, data.get<download_type>());
  EXPECT_STREQ("default", data.get<std::string>().c_str());

  data = download_type::ring_buffer;
  EXPECT_EQ(download_type::ring_buffer, data.get<download_type>());
  EXPECT_STREQ("ring_buffer", data.get<std::string>().c_str());
}

TEST(json_serialize, can_handle_atomic_database_type) {
  json data(atomic<database_type>{database_type::rocksdb});
  EXPECT_EQ(database_type::rocksdb, data.get<atomic<database_type>>());
  EXPECT_STREQ("rocksdb", data.get<std::string>().c_str());

  data = atomic<database_type>(database_type::sqlite);
  EXPECT_EQ(database_type::sqlite, data.get<atomic<database_type>>());
  EXPECT_STREQ("sqlite", data.get<std::string>().c_str());
}

TEST(json_serialize, can_handle_atomic_download_type) {
  json data(atomic<download_type>{download_type::direct});
  EXPECT_EQ(download_type::direct, data.get<atomic<download_type>>());
  EXPECT_STREQ("direct", data.get<std::string>().c_str());

  data = atomic<download_type>{download_type::default_};
  EXPECT_EQ(download_type::default_, data.get<download_type>());
  EXPECT_STREQ("default", data.get<std::string>().c_str());

  data = atomic<download_type>{download_type::ring_buffer};
  EXPECT_EQ(download_type::ring_buffer, data.get<atomic<download_type>>());
  EXPECT_STREQ("ring_buffer", data.get<std::string>().c_str());
}
} // namespace repertory
