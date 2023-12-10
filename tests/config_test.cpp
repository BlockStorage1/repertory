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
#include "test_common.hpp"

#include "app_config.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/string_utils.hpp"

namespace repertory {
class config_test : public ::testing::Test {
public:
  void SetUp() override {
    ASSERT_TRUE(utils::file::delete_directory_recursively(
        utils::path::absolute("./data")));
  }

  void TearDown() override {
    EXPECT_TRUE(utils::file::delete_directory_recursively(
        utils::path::absolute("./data")));
  }
};

const auto DEFAULT_SIA_CONFIG = "{\n"
                                "  \"ApiAuth\": \"\",\n"
                                "  \"ApiPort\": 10000,\n"
                                "  \"ApiUser\": \"repertory\",\n"
                                "  \"ChunkDownloaderTimeoutSeconds\": 30,\n"
                                "  \"EnableChunkDownloaderTimeout\": true,\n"
                                "  \"EnableCommDurationEvents\": false,\n"
                                "  \"EnableDriveEvents\": false,\n"
                                "  \"EnableMaxCacheSize\": false,\n"
#ifdef _WIN32
                                "  \"EnableMountManager\": false,\n"
#endif
                                "  \"EventLevel\": \"normal\",\n"
                                "  \"EvictionDelayMinutes\": 10,\n"
                                "  \"EvictionUsesAccessedTime\": false,\n"
                                "  \"HighFreqIntervalSeconds\": 30,\n"
                                "  \"HostConfig\": {\n"
                                "    \"AgentString\": \"Sia-Agent\",\n"
                                "    \"ApiPassword\": \"\",\n"
                                "    \"ApiPort\": 9980,\n"
                                "    \"HostNameOrIp\": \"localhost\",\n"
                                "    \"TimeoutMs\": 60000\n"
                                "  },\n"
                                "  \"LowFreqIntervalSeconds\": 3600,\n"
                                "  \"MaxCacheSizeBytes\": 21474836480,\n"
                                "  \"MaxUploadCount\": 5,\n"
                                "  \"OnlineCheckRetrySeconds\": 60,\n"
                                "  \"OrphanedFileRetentionDays\": 15,\n"
                                "  \"PreferredDownloadType\": \"fallback\",\n"
                                "  \"ReadAheadCount\": 4,\n"
                                "  \"RemoteMount\": {\n"
                                "    \"EnableRemoteMount\": false,\n"
                                "    \"IsRemoteMount\": false,\n"
                                "    \"RemoteClientPoolSize\": 10,\n"
                                "    \"RemoteHostNameOrIp\": \"\",\n"
                                "    \"RemoteMaxConnections\": 20,\n"
                                "    \"RemotePort\": 20000,\n"
                                "    \"RemoteReceiveTimeoutSeconds\": 120,\n"
                                "    \"RemoteSendTimeoutSeconds\": 30,\n"
                                "    \"RemoteToken\": \"\"\n"
                                "  },\n"
                                "  \"RetryReadCount\": 6,\n"
                                "  \"RingBufferFileSize\": 512,\n"
                                "  \"Version\": " +
                                std::to_string(REPERTORY_CONFIG_VERSION) +
                                "\n"
                                "}";

const auto DEFAULT_S3_CONFIG = "{\n"
                               "  \"ApiAuth\": \"\",\n"
                               "  \"ApiPort\": 10100,\n"
                               "  \"ApiUser\": \"repertory\",\n"
                               "  \"ChunkDownloaderTimeoutSeconds\": 30,\n"
                               "  \"EnableChunkDownloaderTimeout\": true,\n"
                               "  \"EnableCommDurationEvents\": false,\n"
                               "  \"EnableDriveEvents\": false,\n"
                               "  \"EnableMaxCacheSize\": false,\n"
#ifdef _WIN32
                               "  \"EnableMountManager\": false,\n"
#endif
                               "  \"EventLevel\": \"normal\",\n"
                               "  \"EvictionDelayMinutes\": 10,\n"
                               "  \"EvictionUsesAccessedTime\": false,\n"
                               "  \"HighFreqIntervalSeconds\": 30,\n"
                               "  \"LowFreqIntervalSeconds\": 3600,\n"
                               "  \"MaxCacheSizeBytes\": 21474836480,\n"
                               "  \"MaxUploadCount\": 5,\n"
                               "  \"OnlineCheckRetrySeconds\": 60,\n"
                               "  \"OrphanedFileRetentionDays\": 15,\n"
                               "  \"PreferredDownloadType\": \"fallback\",\n"
                               "  \"ReadAheadCount\": 4,\n"
                               "  \"RemoteMount\": {\n"
                               "    \"EnableRemoteMount\": false,\n"
                               "    \"IsRemoteMount\": false,\n"
                               "    \"RemoteClientPoolSize\": 10,\n"
                               "    \"RemoteHostNameOrIp\": \"\",\n"
                               "    \"RemoteMaxConnections\": 20,\n"
                               "    \"RemotePort\": 20100,\n"
                               "    \"RemoteReceiveTimeoutSeconds\": 120,\n"
                               "    \"RemoteSendTimeoutSeconds\": 30,\n"
                               "    \"RemoteToken\": \"\"\n"
                               "  },\n"
                               "  \"RetryReadCount\": 6,\n"
                               "  \"RingBufferFileSize\": 512,\n"
                               "  \"S3Config\": {\n"
                               "    \"AccessKey\": \"\",\n"
                               "    \"Bucket\": \"\",\n"
                               "    \"CacheTimeoutSeconds\": 60,\n"
                               "    \"EncryptionToken\": \"\",\n"
                               "    \"Region\": \"any\",\n"
                               "    \"SecretKey\": \"\",\n"
                               "    \"TimeoutMs\": 60000,\n"
                               "    \"URL\": \"\",\n"
                               "    \"UsePathStyle\": false,\n"
                               "    \"UseRegionInURL\": false\n"
                               "  },\n"
                               "  \"Version\": " +
                               std::to_string(REPERTORY_CONFIG_VERSION) +
                               "\n"
                               "}";

TEST_F(config_test, sia_default_settings) {
  const auto config_file = utils::path::absolute(
      utils::path::combine("./data/sia", {"config.json"}));

  for (int i = 0; i < 2; i++) {
    app_config config(provider_type::sia, "./data/sia");
    config.set_remote_token("");
    config.set_api_auth("");
    EXPECT_TRUE(config.set_value_by_name("HostConfig.ApiPassword", "").empty());
    json data;
    EXPECT_TRUE(utils::file::read_json_file(config_file, data));
    EXPECT_STREQ(DEFAULT_SIA_CONFIG.c_str(), data.dump(2).c_str());
    EXPECT_TRUE(utils::file::is_directory("./data/sia/cache"));
    EXPECT_TRUE(utils::file::is_directory("./data/sia/logs"));
  }
}

TEST_F(config_test, s3_default_settings) {
  const auto config_file =
      utils::path::absolute(utils::path::combine("./data/s3", {"config.json"}));

  for (int i = 0; i < 2; i++) {
    app_config config(provider_type::s3, "./data/s3");
    config.set_remote_token("");
    config.set_api_auth("");
    json data;
    EXPECT_TRUE(utils::file::read_json_file(config_file, data));
    EXPECT_STREQ(DEFAULT_S3_CONFIG.c_str(), data.dump(2).c_str());
    EXPECT_TRUE(utils::file::is_directory("./data/s3/cache"));
    EXPECT_TRUE(utils::file::is_directory("./data/s3/logs"));
  }
}

TEST_F(config_test, api_path) {
  std::string original_value;
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_api_auth();
    EXPECT_EQ(48U, original_value.size());
  }
}

TEST_F(config_test, api_auth) {
  std::string original_value;
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_api_auth();
    config.set_api_auth(original_value.substr(0, 20));
    EXPECT_EQ(original_value.substr(0, 20), config.get_api_auth());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value.substr(0, 20), config.get_api_auth());
  }
}

TEST_F(config_test, api_port) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_api_port();
    config.set_api_port(original_value + 5);
    EXPECT_EQ(original_value + 5, config.get_api_port());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 5, config.get_api_port());
  }
}

TEST_F(config_test, api_user) {
  std::string original_value;
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_api_user();
    config.set_api_user(original_value.substr(0, 2));
    EXPECT_EQ(original_value.substr(0, 2), config.get_api_user());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value.substr(0, 2), config.get_api_user());
  }
}

TEST_F(config_test, chunk_downloader_timeout_secs) {
  std::uint8_t original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_chunk_downloader_timeout_secs();
    config.set_chunk_downloader_timeout_secs(original_value + 5);
    EXPECT_EQ(original_value + 5, config.get_chunk_downloader_timeout_secs());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 5, config.get_chunk_downloader_timeout_secs());
  }
}

TEST_F(config_test, enable_chunk_download_timeout) {
  bool original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_enable_chunk_download_timeout();
    config.set_enable_chunk_downloader_timeout(not original_value);
    EXPECT_EQ(not original_value, config.get_enable_chunk_download_timeout());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(not original_value, config.get_enable_chunk_download_timeout());
  }
}

TEST_F(config_test, enable_comm_duration_events) {
  bool original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_enable_comm_duration_events();
    config.set_enable_comm_duration_events(not original_value);
    EXPECT_EQ(not original_value, config.get_enable_comm_duration_events());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(not original_value, config.get_enable_comm_duration_events());
  }
}

TEST_F(config_test, enable_drive_events) {
  bool original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_enable_drive_events();
    config.set_enable_drive_events(not original_value);
    EXPECT_EQ(not original_value, config.get_enable_drive_events());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(not original_value, config.get_enable_drive_events());
  }
}

TEST_F(config_test, enable_max_cache_size) {
  bool original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_enable_max_cache_size();
    config.set_enable_max_cache_size(not original_value);
    EXPECT_EQ(not original_value, config.get_enable_max_cache_size());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(not original_value, config.get_enable_max_cache_size());
  }
}
#ifdef _WIN32
TEST_F(config_test, enable_mount_manager) {
  bool original_value;
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_enable_mount_manager();
    config.set_enable_mount_manager(not original_value);
    EXPECT_EQ(not original_value, config.get_enable_mount_manager());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(not original_value, config.get_enable_mount_manager());
  }
}
#endif
TEST_F(config_test, event_level) {
  {
    app_config config(provider_type::sia, "./data");
    config.set_event_level(event_level::debug);
    EXPECT_EQ(event_level::debug, config.get_event_level());
    config.set_event_level(event_level::warn);
    EXPECT_EQ(event_level::warn, config.get_event_level());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(event_level::warn, config.get_event_level());
  }
}

TEST_F(config_test, eviction_delay_mins) {
  std::uint32_t original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_eviction_delay_mins();
    config.set_eviction_delay_mins(original_value + 5);
    EXPECT_EQ(original_value + 5, config.get_eviction_delay_mins());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 5, config.get_eviction_delay_mins());
  }
}

TEST_F(config_test, eviction_uses_accessed_time) {
  bool original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_eviction_uses_accessed_time();
    config.set_eviction_uses_accessed_time(not original_value);
    EXPECT_EQ(not original_value, config.get_eviction_uses_accessed_time());
  }

  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(not original_value, config.get_eviction_uses_accessed_time());
  }
}

TEST_F(config_test, high_frequency_interval_secs) {
  std::uint8_t original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_high_frequency_interval_secs();
    config.set_high_frequency_interval_secs(original_value + 5);
    EXPECT_EQ(original_value + 5, config.get_high_frequency_interval_secs());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 5, config.get_high_frequency_interval_secs());
  }
}

TEST_F(config_test, low_frequency_interval_secs) {
  std::uint32_t original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_low_frequency_interval_secs();
    config.set_low_frequency_interval_secs(original_value + 5);
    EXPECT_EQ(original_value + 5, config.get_low_frequency_interval_secs());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 5, config.get_low_frequency_interval_secs());
  }
}

TEST_F(config_test, max_cache_size_bytes) {
  {
    app_config config(provider_type::sia, "./data");
    config.set_max_cache_size_bytes(100 * 1024 * 1024);
    EXPECT_EQ(100U * 1024 * 1024, config.get_max_cache_size_bytes());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(100U * 1024 * 1024, config.get_max_cache_size_bytes());
  }
}

TEST_F(config_test, max_upload_count) {
  {
    app_config config(provider_type::sia, "./data");
    config.set_max_upload_count(8U);
    EXPECT_EQ(std::uint8_t(8U), config.get_max_upload_count());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(std::uint8_t(8U), config.get_max_upload_count());
  }
  {
    app_config config(provider_type::sia, "./data");
    config.set_max_upload_count(0U);
    EXPECT_EQ(std::uint8_t(1U), config.get_max_upload_count());
  }
}

TEST_F(config_test, online_check_retry_secs) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_online_check_retry_secs();
    config.set_online_check_retry_secs(original_value + 1);
    EXPECT_EQ(original_value + 1, config.get_online_check_retry_secs());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 1, config.get_online_check_retry_secs());
  }
}

TEST_F(config_test, online_check_retry_secs_minimum_value) {
  {
    app_config config(provider_type::sia, "./data");
    config.set_online_check_retry_secs(14);
    EXPECT_EQ(15, config.get_online_check_retry_secs());
  }
}

TEST_F(config_test, orphaned_file_retention_days) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_orphaned_file_retention_days();
    config.set_orphaned_file_retention_days(original_value + 1);
    EXPECT_EQ(original_value + 1, config.get_orphaned_file_retention_days());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 1, config.get_orphaned_file_retention_days());
  }
}

TEST_F(config_test, orphaned_file_retention_days_minimum_value) {
  {
    app_config config(provider_type::sia, "./data");
    config.set_orphaned_file_retention_days(0);
    EXPECT_EQ(1, config.get_orphaned_file_retention_days());
  }
}

TEST_F(config_test, orphaned_file_retention_days_maximum_value) {
  {
    app_config config(provider_type::sia, "./data");
    config.set_orphaned_file_retention_days(32);
    EXPECT_EQ(31, config.get_orphaned_file_retention_days());
  }
}

TEST_F(config_test, read_ahead_count) {
  std::uint8_t original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_read_ahead_count();
    config.set_read_ahead_count(original_value + 5);
    EXPECT_EQ(original_value + 5, config.get_read_ahead_count());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 5, config.get_read_ahead_count());
  }
}

TEST_F(config_test, get_cache_directory) {
  {
    app_config config(provider_type::sia, "./data/sia");
    EXPECT_STREQ(utils::path::absolute("./data/sia/cache").c_str(),
                 config.get_cache_directory().c_str());
  }
}

TEST_F(config_test, get_config_file_path) {
  {
    const auto config_file = utils::path::absolute(
        utils::path::combine("./data/sia", {"config.json"}));

    app_config config(provider_type::sia, "./data/sia");
    EXPECT_STREQ(config_file.c_str(), config.get_config_file_path().c_str());
  }
}

TEST_F(config_test, get_data_directory) {
  {
    app_config config(provider_type::sia, "./data/sia");
    EXPECT_STREQ(utils::path::absolute("./data/sia").c_str(),
                 config.get_data_directory().c_str());
  }
}

TEST_F(config_test, get_log_directory) {
  {
    app_config config(provider_type::sia, "./data/sia");
    EXPECT_STREQ(utils::path::absolute("./data/sia/logs").c_str(),
                 config.get_log_directory().c_str());
  }
}

TEST_F(config_test, ring_buffer_file_size) {
  std::uint16_t original_value;
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_ring_buffer_file_size();
    config.set_ring_buffer_file_size(original_value + 5u);
    EXPECT_EQ(original_value + 5u, config.get_ring_buffer_file_size());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 5u, config.get_ring_buffer_file_size());
  }
}

TEST_F(config_test, ring_buffer_file_size_minimum_size) {
  {
    app_config config(provider_type::sia, "./data");
    config.set_ring_buffer_file_size(63u);
    EXPECT_EQ(64u, config.get_ring_buffer_file_size());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(64u, config.get_ring_buffer_file_size());
  }
}

TEST_F(config_test, ring_buffer_file_size_maximum_size) {
  {
    app_config config(provider_type::sia, "./data");
    config.set_ring_buffer_file_size(1025u);
    EXPECT_EQ(1024u, config.get_ring_buffer_file_size());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(1024u, config.get_ring_buffer_file_size());
  }
}

TEST_F(config_test, preferred_download_type) {
  download_type original_value;
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_preferred_download_type();
    config.set_preferred_download_type(download_type::ring_buffer);
    EXPECT_NE(original_value, config.get_preferred_download_type());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_NE(original_value, config.get_preferred_download_type());
  }
}

TEST_F(config_test, default_agent_name) {
  EXPECT_STREQ("Sia-Agent",
               app_config::default_agent_name(provider_type::sia).c_str());
}

TEST_F(config_test, default_api_port) {
  EXPECT_EQ(9980U, app_config::default_api_port(provider_type::sia));
}

TEST_F(config_test, default_data_directory) {
  const std::array<std::string, 1U> data_directory = {
      app_config::default_data_directory(provider_type::sia),
  };

#ifdef _WIN32
  const auto local_app_data = utils::get_environment_variable("localappdata");
#endif
#if __linux__
  const auto local_app_data =
      utils::path::combine(utils::get_environment_variable("HOME"), {".local"});
#endif
#ifdef __APPLE__
  const auto local_app_data = utils::path::combine(
      utils::get_environment_variable("HOME"), {"Library/Application Support"});
#endif
  auto expected_directory =
      utils::path::combine(local_app_data, {"/repertory2/sia"});
  EXPECT_STREQ(expected_directory.c_str(), data_directory[0].c_str());
}

TEST_F(config_test, default_rpc_port) {
  EXPECT_EQ(10000U, app_config::default_rpc_port(provider_type::sia));
}

TEST_F(config_test, get_provider_display_name) {
  EXPECT_STREQ(
      "Sia", app_config::get_provider_display_name(provider_type::sia).c_str());
}

TEST_F(config_test, get_provider_name) {
  EXPECT_STREQ("sia",
               app_config::get_provider_name(provider_type::sia).c_str());
}

TEST_F(config_test, get_version) {
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(REPERTORY_CONFIG_VERSION, config.get_version());
  }
}

TEST_F(config_test, enable_remote_mount) {
  bool original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_enable_remote_mount();
    config.set_enable_remote_mount(not original_value);
    EXPECT_EQ(not original_value, config.get_enable_remote_mount());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(not original_value, config.get_enable_remote_mount());
  }
}

TEST_F(config_test, is_remote_mount) {
  bool original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_is_remote_mount();
    config.set_is_remote_mount(not original_value);
    EXPECT_EQ(not original_value, config.get_is_remote_mount());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(not original_value, config.get_is_remote_mount());
  }
}

TEST_F(config_test, enable_remote_mount_fails_if_remote_mount_is_true) {
  app_config config(provider_type::sia, "./data");
  config.set_is_remote_mount(true);
  config.set_enable_remote_mount(true);
  EXPECT_FALSE(config.get_enable_remote_mount());
  EXPECT_TRUE(config.get_is_remote_mount());
}

TEST_F(config_test, set_is_remote_mount_fails_if_enable_remote_mount_is_true) {
  app_config config(provider_type::sia, "./data");
  config.set_enable_remote_mount(true);
  config.set_is_remote_mount(true);
  EXPECT_FALSE(config.get_is_remote_mount());
  EXPECT_TRUE(config.get_enable_remote_mount());
}

TEST_F(config_test, remote_host_name_or_ip) {
  {
    app_config config(provider_type::sia, "./data");
    config.set_remote_host_name_or_ip("my.host.name");
    EXPECT_STREQ("my.host.name", config.get_remote_host_name_or_ip().c_str());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_STREQ("my.host.name", config.get_remote_host_name_or_ip().c_str());
  }
}

TEST_F(config_test, remote_port) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_remote_port();
    config.set_remote_port(original_value + 5);
    EXPECT_EQ(original_value + 5, config.get_remote_port());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 5, config.get_remote_port());
  }
}

TEST_F(config_test, remote_receive_timeout_secs) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_remote_receive_timeout_secs();
    config.set_remote_receive_timeout_secs(original_value + 5);
    EXPECT_EQ(original_value + 5, config.get_remote_receive_timeout_secs());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 5, config.get_remote_receive_timeout_secs());
  }
}

TEST_F(config_test, remote_send_timeout_secs) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_remote_send_timeout_secs();
    config.set_remote_send_timeout_secs(original_value + 5);
    EXPECT_EQ(original_value + 5, config.get_remote_send_timeout_secs());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 5, config.get_remote_send_timeout_secs());
  }
}

TEST_F(config_test, remote_token) {
  {
    app_config config(provider_type::sia, "./data");
    config.set_remote_token("myToken");
    EXPECT_STREQ("myToken", config.get_remote_token().c_str());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_STREQ("myToken", config.get_remote_token().c_str());
  }
}

TEST_F(config_test, remote_client_pool_size) {
  std::uint8_t original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_remote_client_pool_size();
    config.set_remote_client_pool_size(original_value + 5);
    EXPECT_EQ(original_value + 5, config.get_remote_client_pool_size());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 5, config.get_remote_client_pool_size());
  }
}

TEST_F(config_test, remote_client_pool_size_minimum_value) {
  {
    app_config config(provider_type::sia, "./data");
    config.set_remote_client_pool_size(0);
    EXPECT_EQ(5, config.get_remote_client_pool_size());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(5, config.get_remote_client_pool_size());
  }
}

TEST_F(config_test, remote_max_connections) {
  std::uint8_t original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_remote_max_connections();
    config.set_remote_max_connections(original_value + 5);
    EXPECT_EQ(original_value + 5, config.get_remote_max_connections());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 5, config.get_remote_max_connections());
  }
}

TEST_F(config_test, remote_max_connections_minimum_value) {
  {
    app_config config(provider_type::sia, "./data");
    config.set_remote_max_connections(0);
    EXPECT_EQ(1, config.get_remote_max_connections());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(1, config.get_remote_max_connections());
  }
}

TEST_F(config_test, retry_read_count) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, "./data");
    original_value = config.get_retry_read_count();
    config.set_retry_read_count(original_value + 1);
    EXPECT_EQ(original_value + 1, config.get_retry_read_count());
  }
  {
    app_config config(provider_type::sia, "./data");
    EXPECT_EQ(original_value + 1, config.get_retry_read_count());
  }
}

TEST_F(config_test, retry_read_count_minimum_value) {
  {
    app_config config(provider_type::sia, "./data");
    config.set_retry_read_count(1);
    EXPECT_EQ(2, config.get_retry_read_count());
  }
}

TEST_F(config_test, cache_timeout_seconds_minimum_value) {
  {
    app_config config(provider_type::s3, "./data");
    EXPECT_FALSE(
        config.set_value_by_name("S3Config.CacheTimeoutSeconds", "1").empty());
    EXPECT_EQ(std::uint16_t(5U), config.get_s3_config().cache_timeout_secs);
  }
}
} // namespace repertory
