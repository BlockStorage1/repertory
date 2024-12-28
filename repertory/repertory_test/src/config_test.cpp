/*
 Copyright <2018-2024> <scott.e.graves@protonmail.com>

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
#include "events/event_system.hpp"
#include "utils/common.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"

namespace repertory {
class config_test : public ::testing::Test {
public:
  console_consumer cs;

  static std::atomic<std::uint64_t> idx;

  std::string s3_directory;
  std::string sia_directory;

  void SetUp() override {
    s3_directory = utils::path::combine(test::get_test_output_dir(),
                                        {
                                            "config_test",
                                            "s3",
                                            std::to_string(++idx),
                                        });

    sia_directory = utils::path::combine(test::get_test_output_dir(),
                                         {
                                             "config_test",
                                             "sia",
                                             std::to_string(++idx),
                                         });
    event_system::instance().start();
  }

  void TearDown() override { event_system::instance().stop(); }
};

std::atomic<std::uint64_t> config_test::idx{0U};

TEST_F(config_test, api_path) {
  std::string original_value;
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_api_auth();
    EXPECT_EQ(48U, original_value.size());
  }
}

TEST_F(config_test, api_auth) {
  std::string original_value;
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_api_auth();
    config.set_api_auth(original_value.substr(0, 20));
    EXPECT_EQ(original_value.substr(0, 20), config.get_api_auth());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(original_value.substr(0, 20), config.get_api_auth());
  }
}

TEST_F(config_test, api_port) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_api_port();
    config.set_api_port(original_value + 5);
    EXPECT_EQ(original_value + 5, config.get_api_port());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(original_value + 5, config.get_api_port());
  }
}

TEST_F(config_test, api_user) {
  std::string original_value;
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_api_user();
    config.set_api_user(original_value.substr(0, 2));
    EXPECT_EQ(original_value.substr(0, 2), config.get_api_user());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(original_value.substr(0, 2), config.get_api_user());
  }
}

TEST_F(config_test, download_timeout_secs) {
  std::uint8_t original_value{};
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_download_timeout_secs();
    config.set_download_timeout_secs(original_value + 5);
    EXPECT_EQ(original_value + 5, config.get_download_timeout_secs());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(original_value + 5, config.get_download_timeout_secs());
  }
}

TEST_F(config_test, enable_download_timeout) {
  bool original_value{};
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_enable_download_timeout();
    config.set_enable_download_timeout(not original_value);
    EXPECT_EQ(not original_value, config.get_enable_download_timeout());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(not original_value, config.get_enable_download_timeout());
  }
}

TEST_F(config_test, enable_drive_events) {
  bool original_value{};
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_enable_drive_events();
    config.set_enable_drive_events(not original_value);
    EXPECT_EQ(not original_value, config.get_enable_drive_events());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(not original_value, config.get_enable_drive_events());
  }
}

#if defined(_WIN32)
TEST_F(config_test, enable_mount_manager) {
  bool original_value;
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_enable_mount_manager();
    config.set_enable_mount_manager(not original_value);
    EXPECT_EQ(not original_value, config.get_enable_mount_manager());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(not original_value, config.get_enable_mount_manager());
  }
}
#endif
TEST_F(config_test, event_level) {
  {
    app_config config(provider_type::sia, sia_directory);
    config.set_event_level(event_level::debug);
    EXPECT_EQ(event_level::debug, config.get_event_level());
    config.set_event_level(event_level::warn);
    EXPECT_EQ(event_level::warn, config.get_event_level());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(event_level::warn, config.get_event_level());
  }
}

TEST_F(config_test, eviction_delay_mins) {
  std::uint32_t original_value{};
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_eviction_delay_mins();
    config.set_eviction_delay_mins(original_value + 5);
    EXPECT_EQ(original_value + 5, config.get_eviction_delay_mins());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(original_value + 5, config.get_eviction_delay_mins());
  }
}

TEST_F(config_test, eviction_uses_accessed_time) {
  bool original_value{};
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_eviction_uses_accessed_time();
    config.set_eviction_uses_accessed_time(not original_value);
    EXPECT_EQ(not original_value, config.get_eviction_uses_accessed_time());
  }

  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(not original_value, config.get_eviction_uses_accessed_time());
  }
}

TEST_F(config_test, high_frequency_interval_secs) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_high_frequency_interval_secs();
    config.set_high_frequency_interval_secs(original_value + 5U);
    EXPECT_EQ(original_value + 5U, config.get_high_frequency_interval_secs());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(original_value + 5U, config.get_high_frequency_interval_secs());
  }
}

TEST_F(config_test, low_frequency_interval_secs) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_low_frequency_interval_secs();
    config.set_low_frequency_interval_secs(original_value + 5U);
    EXPECT_EQ(original_value + 5U, config.get_low_frequency_interval_secs());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(original_value + 5U, config.get_low_frequency_interval_secs());
  }
}

TEST_F(config_test, med_frequency_interval_secs) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_med_frequency_interval_secs();
    config.set_med_frequency_interval_secs(original_value + 5U);
    EXPECT_EQ(original_value + 5U, config.get_med_frequency_interval_secs());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(original_value + 5U, config.get_med_frequency_interval_secs());
  }
}

TEST_F(config_test, max_cache_size_bytes) {
  {
    app_config config(provider_type::sia, sia_directory);
    config.set_max_cache_size_bytes(100 * 1024 * 1024);
    EXPECT_EQ(100U * 1024 * 1024, config.get_max_cache_size_bytes());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(100U * 1024 * 1024, config.get_max_cache_size_bytes());
  }
}

TEST_F(config_test, max_upload_count) {
  {
    app_config config(provider_type::sia, sia_directory);
    config.set_max_upload_count(8U);
    EXPECT_EQ(std::uint8_t(8U), config.get_max_upload_count());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(std::uint8_t(8U), config.get_max_upload_count());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    config.set_max_upload_count(0U);
    EXPECT_EQ(std::uint8_t(1U), config.get_max_upload_count());
  }
}

TEST_F(config_test, online_check_retry_secs) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_online_check_retry_secs();
    config.set_online_check_retry_secs(original_value + 1);
    EXPECT_EQ(original_value + 1, config.get_online_check_retry_secs());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(original_value + 1, config.get_online_check_retry_secs());
  }
}

TEST_F(config_test, online_check_retry_secs_minimum_value) {
  {
    app_config config(provider_type::sia, sia_directory);
    config.set_online_check_retry_secs(14);
    EXPECT_EQ(15, config.get_online_check_retry_secs());
  }
}

TEST_F(config_test, orphaned_file_retention_days) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_orphaned_file_retention_days();
    config.set_orphaned_file_retention_days(original_value + 1);
    EXPECT_EQ(original_value + 1, config.get_orphaned_file_retention_days());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(original_value + 1, config.get_orphaned_file_retention_days());
  }
}

TEST_F(config_test, orphaned_file_retention_days_minimum_value) {
  {
    app_config config(provider_type::sia, sia_directory);
    config.set_orphaned_file_retention_days(0);
    EXPECT_EQ(1, config.get_orphaned_file_retention_days());
  }
}

TEST_F(config_test, orphaned_file_retention_days_maximum_value) {
  {
    app_config config(provider_type::sia, sia_directory);
    config.set_orphaned_file_retention_days(32);
    EXPECT_EQ(31, config.get_orphaned_file_retention_days());
  }
}

TEST_F(config_test, get_cache_directory) {
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_STREQ(utils::path::combine(sia_directory, {"cache"}).c_str(),
                 config.get_cache_directory().c_str());
  }
}

TEST_F(config_test, get_config_file_path) {
  {
    const auto config_file = utils::path::absolute(
        utils::path::combine(sia_directory, {"config.json"}));

    app_config config(provider_type::sia, sia_directory);
    EXPECT_STREQ(config_file.c_str(), config.get_config_file_path().c_str());
  }
}

TEST_F(config_test, get_data_directory) {
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_STREQ(sia_directory.c_str(), config.get_data_directory().c_str());
  }
}

TEST_F(config_test, get_log_directory) {
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_STREQ(utils::path::combine(sia_directory, {"logs"}).c_str(),
                 config.get_log_directory().c_str());
  }
}

TEST_F(config_test, ring_buffer_file_size) {
  std::uint16_t original_value;
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_ring_buffer_file_size();
    config.set_ring_buffer_file_size(original_value + 5u);
    EXPECT_EQ(original_value + 5u, config.get_ring_buffer_file_size());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(original_value + 5u, config.get_ring_buffer_file_size());
  }
}

TEST_F(config_test, ring_buffer_file_size_minimum_size) {
  {
    app_config config(provider_type::sia, sia_directory);
    config.set_ring_buffer_file_size(63u);
    EXPECT_EQ(64u, config.get_ring_buffer_file_size());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(64u, config.get_ring_buffer_file_size());
  }
}

TEST_F(config_test, ring_buffer_file_size_maximum_size) {
  {
    app_config config(provider_type::sia, sia_directory);
    config.set_ring_buffer_file_size(1025u);
    EXPECT_EQ(1024u, config.get_ring_buffer_file_size());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(1024u, config.get_ring_buffer_file_size());
  }
}

TEST_F(config_test, preferred_download_type) {
  download_type original_value;
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_preferred_download_type();
    config.set_preferred_download_type(download_type::ring_buffer);
    EXPECT_NE(original_value, config.get_preferred_download_type());
  }
  {
    app_config config(provider_type::sia, sia_directory);
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

#if defined(_WIN32)
  const auto local_app_data = utils::get_environment_variable("localappdata");
#endif
#if defined(__linux__)
  const auto local_app_data =
      utils::path::combine(utils::get_environment_variable("HOME"), {".local"});
#endif
#if defined(__APPLE__)
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
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(REPERTORY_CONFIG_VERSION, config.get_version());
  }
}

// TEST_F(config_test, enable_remote_mount) {
//   bool original_value{};
//   {
//     app_config config(provider_type::sia, sia_directory);
//     original_value = config.get_enable_remote_mount();
//     config.set_enable_remote_mount(not original_value);
//     EXPECT_EQ(not original_value, config.get_enable_remote_mount());
//   }
//   {
//     app_config config(provider_type::sia, sia_directory);
//     EXPECT_EQ(not original_value, config.get_enable_remote_mount());
//   }
// }

// TEST_F(config_test, is_remote_mount) {
//   bool original_value{};
//   {
//     app_config config(provider_type::sia, sia_directory);
//     original_value = config.get_is_remote_mount();
//     config.set_is_remote_mount(not original_value);
//     EXPECT_EQ(not original_value, config.get_is_remote_mount());
//   }
//   {
//     app_config config(provider_type::sia, sia_directory);
//     EXPECT_EQ(not original_value, config.get_is_remote_mount());
//   }
// }

// TEST_F(config_test, enable_remote_mount_fails_if_remote_mount_is_true) {
//   app_config config(provider_type::sia, sia_directory);
//   config.set_is_remote_mount(true);
//   config.set_enable_remote_mount(true);
//   EXPECT_FALSE(config.get_enable_remote_mount());
//   EXPECT_TRUE(config.get_is_remote_mount());
// }

// TEST_F(config_test, set_is_remote_mount_fails_if_enable_remote_mount_is_true)
// {
//   app_config config(provider_type::sia, sia_directory);
//   config.set_enable_remote_mount(true);
//   config.set_is_remote_mount(true);
//   EXPECT_FALSE(config.get_is_remote_mount());
//   EXPECT_TRUE(config.get_enable_remote_mount());
// }

// TEST_F(config_test, remote_host_name_or_ip) {
//   {
//     app_config config(provider_type::sia, sia_directory);
//     config.set_remote_host_name_or_ip("my.host.name");
//     EXPECT_STREQ("my.host.name",
//     config.get_remote_host_name_or_ip().c_str());
//   }
//   {
//     app_config config(provider_type::sia, sia_directory);
//     EXPECT_STREQ("my.host.name",
//     config.get_remote_host_name_or_ip().c_str());
//   }
// }

// TEST_F(config_test, remote_api_port) {
//   std::uint16_t original_value{};
//   {
//     app_config config(provider_type::sia, sia_directory);
//     original_value = config.get_remote_api_port();
//     config.set_remote_api_port(original_value + 5);
//     EXPECT_EQ(original_value + 5, config.get_remote_api_port());
//   }
//   {
//     app_config config(provider_type::sia, sia_directory);
//     EXPECT_EQ(original_value + 5, config.get_remote_api_port());
//   }
// }

// TEST_F(config_test, remote_receive_timeout_secs) {
//   std::uint16_t original_value{};
//   {
//     app_config config(provider_type::sia, sia_directory);
//     original_value = config.get_remote_receive_timeout_secs();
//     config.set_remote_receive_timeout_secs(original_value + 5);
//     EXPECT_EQ(original_value + 5, config.get_remote_receive_timeout_secs());
//   }
//   {
//     app_config config(provider_type::sia, sia_directory);
//     EXPECT_EQ(original_value + 5, config.get_remote_receive_timeout_secs());
//   }
// }

// TEST_F(config_test, remote_send_timeout_secs) {
//   std::uint16_t original_value{};
//   {
//     app_config config(provider_type::sia, sia_directory);
//     original_value = config.get_remote_send_timeout_secs();
//     config.set_remote_send_timeout_secs(original_value + 5);
//     EXPECT_EQ(original_value + 5, config.get_remote_send_timeout_secs());
//   }
//   {
//     app_config config(provider_type::sia, sia_directory);
//     EXPECT_EQ(original_value + 5, config.get_remote_send_timeout_secs());
//   }
// }

// TEST_F(config_test, remote_encryption_token) {
//   {
//     app_config config(provider_type::sia, sia_directory);
//     config.set_remote_encryption_token("myToken");
//     EXPECT_STREQ("myToken", config.get_remote_encryption_token().c_str());
//   }
//   {
//     app_config config(provider_type::sia, sia_directory);
//     EXPECT_STREQ("myToken", config.get_remote_encryption_token().c_str());
//   }
// }
//
// TEST_F(config_test, remote_client_pool_size) {
//   std::uint8_t original_value{};
//   {
//     app_config config(provider_type::sia, sia_directory);
//     original_value = config.get_remote_client_pool_size();
//     config.set_remote_client_pool_size(original_value + 5);
//     EXPECT_EQ(original_value + 5, config.get_remote_client_pool_size());
//   }
//   {
//     app_config config(provider_type::sia, sia_directory);
//     EXPECT_EQ(original_value + 5, config.get_remote_client_pool_size());
//   }
// }
//
// TEST_F(config_test, remote_client_pool_size_minimum_value) {
//   {
//     app_config config(provider_type::sia, sia_directory);
//     config.set_remote_client_pool_size(0);
//     EXPECT_EQ(5, config.get_remote_client_pool_size());
//   }
//   {
//     app_config config(provider_type::sia, sia_directory);
//     EXPECT_EQ(5, config.get_remote_client_pool_size());
//   }
// }

// TEST_F(config_test, remote_max_connections) {
//   std::uint8_t original_value{};
//   {
//     app_config config(provider_type::sia, sia_directory);
//     original_value = config.get_remote_max_connections();
//     config.set_remote_max_connections(original_value + 5);
//     EXPECT_EQ(original_value + 5, config.get_remote_max_connections());
//   }
//   {
//     app_config config(provider_type::sia, sia_directory);
//     EXPECT_EQ(original_value + 5, config.get_remote_max_connections());
//   }
// }

// TEST_F(config_test, remote_max_connections_minimum_value) {
//   {
//     app_config config(provider_type::sia, sia_directory);
//     config.set_remote_max_connections(0);
//     EXPECT_EQ(1, config.get_remote_max_connections());
//   }
//   {
//     app_config config(provider_type::sia, sia_directory);
//     EXPECT_EQ(1, config.get_remote_max_connections());
//   }
// }

TEST_F(config_test, retry_read_count) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_retry_read_count();
    config.set_retry_read_count(original_value + 1);
    EXPECT_EQ(original_value + 1, config.get_retry_read_count());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(original_value + 1, config.get_retry_read_count());
  }
}

TEST_F(config_test, retry_read_count_minimum_value) {
  {
    app_config config(provider_type::sia, sia_directory);
    config.set_retry_read_count(1);
    EXPECT_EQ(2, config.get_retry_read_count());
  }
}

TEST_F(config_test, task_wait_ms) {
  std::uint16_t original_value{};
  {
    app_config config(provider_type::sia, sia_directory);
    original_value = config.get_task_wait_ms();
    config.set_task_wait_ms(original_value + 1U);
    EXPECT_EQ(original_value + 1U, config.get_task_wait_ms());
  }
  {
    app_config config(provider_type::sia, sia_directory);
    EXPECT_EQ(original_value + 1U, config.get_task_wait_ms());
  }
}

TEST_F(config_test, task_wait_ms_minimum_value) {
  {
    app_config config(provider_type::sia, sia_directory);
    config.set_task_wait_ms(1U);
    EXPECT_EQ(50U, config.get_task_wait_ms());
  }
}

TEST_F(config_test, can_set_database_type) {
  {
    app_config config(provider_type::sia, sia_directory);
    config.set_database_type(database_type::rocksdb);
    EXPECT_EQ(database_type::rocksdb, config.get_database_type());

    config.set_database_type(database_type::sqlite);
    EXPECT_EQ(database_type::sqlite, config.get_database_type());

    config.set_database_type(database_type::rocksdb);
    EXPECT_EQ(database_type::rocksdb, config.get_database_type());
  }
}
} // namespace repertory
