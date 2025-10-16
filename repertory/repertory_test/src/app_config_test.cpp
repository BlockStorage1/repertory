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

#include "app_config.hpp"
#include "utils/path.hpp"

namespace repertory {
class app_config_test : public ::testing::Test {
public:
  static std::atomic<std::uint64_t> idx;

  std::string encrypt_directory;
  std::string remote_directory;
  std::string s3_directory;
  std::string sia_directory;

  void SetUp() override {
    encrypt_directory = utils::path::combine(test::get_test_output_dir(),
                                             {
                                                 "app_config_test",
                                                 "encrypt",
                                                 std::to_string(++idx),
                                             });

    remote_directory = utils::path::combine(test::get_test_output_dir(),
                                            {
                                                "app_config_test",
                                                "remote",
                                                std::to_string(++idx),
                                            });

    s3_directory = utils::path::combine(test::get_test_output_dir(),
                                        {
                                            "app_config_test",
                                            "s3",
                                            std::to_string(++idx),
                                        });

    sia_directory = utils::path::combine(test::get_test_output_dir(),
                                         {
                                             "app_config_test",
                                             "sia",
                                             std::to_string(++idx),
                                         });
  }
};

static void remove_unused_types(auto &data, provider_type prov) {
  switch (prov) {
  case provider_type::encrypt:
    data.erase(JSON_DOWNLOAD_TIMEOUT_SECS);
    data.erase(JSON_ENABLE_DOWNLOAD_TIMEOUT);
    data.erase(JSON_EVICTION_DELAY_MINS);
    data.erase(JSON_EVICTION_USE_ACCESS_TIME);
    data.erase(JSON_HOST_CONFIG);
    data.erase(JSON_MAX_CACHE_SIZE_BYTES);
    data.erase(JSON_MAX_UPLOAD_COUNT);
    data.erase(JSON_ONLINE_CHECK_RETRY_SECS);
    data.erase(JSON_PREFERRED_DOWNLOAD_TYPE);
    data.erase(JSON_REMOTE_CONFIG);
    data.erase(JSON_RETRY_READ_COUNT);
    data.erase(JSON_RING_BUFFER_FILE_SIZE);
    data.erase(JSON_S3_CONFIG);
    data.erase(JSON_SIA_CONFIG);
    break;

  case provider_type::remote:
    data.erase(JSON_DATABASE_TYPE);
    data.erase(JSON_DOWNLOAD_TIMEOUT_SECS);
    data.erase(JSON_ENABLE_DOWNLOAD_TIMEOUT);
    data.erase(JSON_ENCRYPT_CONFIG);
    data.erase(JSON_EVICTION_DELAY_MINS);
    data.erase(JSON_EVICTION_USE_ACCESS_TIME);
    data.erase(JSON_HIGH_FREQ_INTERVAL_SECS);
    data.erase(JSON_HOST_CONFIG);
    data.erase(JSON_LOW_FREQ_INTERVAL_SECS);
    data.erase(JSON_MAX_CACHE_SIZE_BYTES);
    data.erase(JSON_MAX_UPLOAD_COUNT);
    data.erase(JSON_MED_FREQ_INTERVAL_SECS);
    data.erase(JSON_ONLINE_CHECK_RETRY_SECS);
    data.erase(JSON_PREFERRED_DOWNLOAD_TYPE);
    data.erase(JSON_REMOTE_MOUNT);
    data.erase(JSON_RETRY_READ_COUNT);
    data.erase(JSON_RING_BUFFER_FILE_SIZE);
    data.erase(JSON_S3_CONFIG);
    data.erase(JSON_SIA_CONFIG);
    break;

  case provider_type::s3:
    data.erase(JSON_ENCRYPT_CONFIG);
    data.erase(JSON_HOST_CONFIG);
    data.erase(JSON_REMOTE_CONFIG);
    data.erase(JSON_SIA_CONFIG);
    break;

  case provider_type::sia:
    data.erase(JSON_ENCRYPT_CONFIG);
    data.erase(JSON_REMOTE_CONFIG);
    data.erase(JSON_S3_CONFIG);
    break;

  default:
    return;
  }
}

std::atomic<std::uint64_t> app_config_test::idx{0U};

static void defaults_tests(const json &json_data, provider_type prov) {
  json json_defaults = {
      {JSON_API_PORT, default_rpc_port},
      {JSON_API_USER, std::string{REPERTORY}},
      {JSON_DOWNLOAD_TIMEOUT_SECS, default_download_timeout_secs},
      {JSON_DATABASE_TYPE, database_type::rocksdb},
      {JSON_ENABLE_DOWNLOAD_TIMEOUT, true},
      {JSON_ENABLE_DRIVE_EVENTS, false},
#if defined(_WIN32)
      {JSON_ENABLE_MOUNT_MANAGER, false},
#endif // defined(_WIN32)
      {JSON_ENCRYPT_CONFIG, encrypt_config{}},
      {JSON_EVENT_LEVEL, event_level::info},
      {JSON_EVICTION_DELAY_MINS, default_eviction_delay_mins},
      {JSON_EVICTION_USE_ACCESS_TIME, false},
      {JSON_HIGH_FREQ_INTERVAL_SECS, default_high_freq_interval_secs},
      {JSON_HOST_CONFIG, host_config{}},
      {JSON_LOW_FREQ_INTERVAL_SECS, default_low_freq_interval_secs},
      {JSON_MAX_CACHE_SIZE_BYTES, default_max_cache_size_bytes},
      {JSON_MAX_UPLOAD_COUNT, default_max_upload_count},
      {JSON_MED_FREQ_INTERVAL_SECS, default_med_freq_interval_secs},
      {JSON_ONLINE_CHECK_RETRY_SECS, default_online_check_retry_secs},
      {JSON_PREFERRED_DOWNLOAD_TYPE, download_type::default_},
      {JSON_REMOTE_CONFIG, remote::remote_config{}},
      {JSON_REMOTE_MOUNT, remote::remote_mount{}},
      {JSON_RETRY_READ_COUNT, default_retry_read_count},
      {JSON_RING_BUFFER_FILE_SIZE, default_ring_buffer_file_size},
      {JSON_S3_CONFIG, s3_config{}},
      {JSON_SIA_CONFIG, sia_config{}},
      {JSON_TASK_WAIT_MS, default_task_wait_ms},
      {JSON_VERSION, REPERTORY_CONFIG_VERSION},
  };

  remove_unused_types(json_defaults, prov);

  switch (prov) {
  case provider_type::encrypt:
    json_defaults[JSON_REMOTE_MOUNT][JSON_API_PORT] =
        app_config::default_remote_api_port(prov);
    break;

    json_defaults[JSON_REMOTE_MOUNT][JSON_API_PORT] =
        app_config::default_remote_api_port(prov);
    break;

  case provider_type::sia:
    json_defaults[JSON_HOST_CONFIG][JSON_API_PORT] =
        app_config::default_api_port(prov);
    json_defaults[JSON_HOST_CONFIG][JSON_AGENT_STRING] =
        app_config::default_agent_name(prov);
    json_defaults[JSON_REMOTE_MOUNT][JSON_API_PORT] =
        app_config::default_remote_api_port(prov);
    break;

  default:
    return;
  }

  fmt::println("testing default|{}-{}", app_config::get_provider_name(prov),
               JSON_API_PASSWORD);
  ASSERT_EQ(std::size_t(default_api_password_size),
            json_data.at(JSON_API_PASSWORD).get<std::string>().size());
  for (const auto &[key, value] : json_defaults.items()) {
    fmt::println("testing default|{}-{}", app_config::get_provider_name(prov),
                 key);
    EXPECT_EQ(value, json_data.at(key));
  }
}

template <typename get_t, typename set_t, typename val_t>
static void test_getter_setter(app_config &cfg, get_t getter, set_t setter,
                               val_t val1, val_t val2, std::string_view key,
                               std::string_view val_str) {
  (cfg.*setter)(val1);
  ASSERT_TRUE((cfg.*getter)() == val1);

  (cfg.*setter)(val2);
  ASSERT_TRUE((cfg.*getter)() == val2);

  if (key.empty()) {
    return;
  }

  EXPECT_STREQ(std::string{val_str}.c_str(),
               cfg.set_value_by_name(key, val_str).c_str());
}

static void common_tests(app_config &config, provider_type prov) {
  ASSERT_EQ(config.get_provider_type(), prov);

  std::map<std::string_view, std::function<void(app_config &)>> methods{
      {JSON_API_PASSWORD,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_api_password,
                            &app_config::set_api_password, "", "auth",
                            JSON_API_PASSWORD, "auth2");
       }},
      {JSON_API_PORT,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_api_port,
                            &app_config::set_api_port, std::uint16_t{0U},
                            std::uint16_t{1024U}, JSON_API_PORT, "1025");
       }},
      {JSON_API_USER,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_api_user,
                            &app_config::set_api_user, "", "user",
                            JSON_API_USER, "user2");
       }},
      {JSON_DOWNLOAD_TIMEOUT_SECS,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_download_timeout_secs,
                            &app_config::set_download_timeout_secs,
                            std::uint8_t{min_download_timeout_secs + 1U},
                            std::uint8_t{min_download_timeout_secs + 2U},
                            JSON_DOWNLOAD_TIMEOUT_SECS,
                            std::to_string(min_download_timeout_secs + 2U));

         cfg.set_download_timeout_secs(min_download_timeout_secs - 1U);
         EXPECT_EQ(min_download_timeout_secs, cfg.get_download_timeout_secs());
       }},
      {JSON_DATABASE_TYPE,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_database_type,
                            &app_config::set_database_type,
                            database_type::rocksdb, database_type::sqlite,
                            JSON_DATABASE_TYPE, "rocksdb");
       }},
      {JSON_ENABLE_DOWNLOAD_TIMEOUT,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_enable_download_timeout,
                            &app_config::set_enable_download_timeout, true,
                            false, JSON_ENABLE_DOWNLOAD_TIMEOUT, "1");
       }},
      {JSON_ENABLE_DRIVE_EVENTS,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_enable_drive_events,
                            &app_config::set_enable_drive_events, true, false,
                            JSON_ENABLE_DRIVE_EVENTS, "1");
       }},
#if defined(_WIN32)
      {JSON_ENABLE_MOUNT_MANAGER,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_enable_mount_manager,
                            &app_config::set_enable_mount_manager, true, false,
                            JSON_ENABLE_MOUNT_MANAGER, "1");
       }},
#endif // defined(_WIN32)
      {JSON_ENCRYPT_CONFIG,
       [](app_config &cfg) {
         encrypt_config cfg1{};
         cfg1.encryption_token = "1";
         cfg1.path = "2";

         encrypt_config cfg2{};
         cfg2.encryption_token = "2";
         cfg2.path = "1";

         ASSERT_NE(cfg1, cfg2);
         test_getter_setter(cfg, &app_config::get_encrypt_config,
                            &app_config::set_encrypt_config, cfg1, cfg2, "",
                            "");

         encrypt_config cfg3{};
         cfg3.encryption_token = "3";
         cfg3.path = "4";

         auto value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_ENCRYPT_CONFIG, JSON_ENCRYPTION_TOKEN),
             cfg3.encryption_token);
         EXPECT_STREQ(cfg3.encryption_token.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_ENCRYPT_CONFIG, JSON_PATH), cfg3.path);
         EXPECT_STREQ(cfg3.path.c_str(), value.c_str());
       }},
      {JSON_EVENT_LEVEL,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_event_level,
                            &app_config::set_event_level, event_level::critical,
                            event_level::debug, JSON_EVENT_LEVEL, "info");
       }},
      {JSON_EVICTION_DELAY_MINS,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_eviction_delay_mins,
                            &app_config::set_eviction_delay_mins,
                            std::uint32_t{0U}, std::uint32_t{1U},
                            JSON_EVICTION_DELAY_MINS, "2");
       }},
      {JSON_EVICTION_USE_ACCESS_TIME,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_eviction_uses_accessed_time,
                            &app_config::set_eviction_uses_accessed_time, true,
                            false, JSON_EVICTION_USE_ACCESS_TIME, "1");
       }},
      {JSON_HIGH_FREQ_INTERVAL_SECS,
       [](app_config &cfg) {
         test_getter_setter(
             cfg, &app_config::get_high_frequency_interval_secs,
             &app_config::set_high_frequency_interval_secs,
             std::uint16_t{default_high_freq_interval_secs + 1U},
             std::uint16_t{default_high_freq_interval_secs + 2U},
             JSON_HIGH_FREQ_INTERVAL_SECS,
             std::to_string(default_high_freq_interval_secs + 3U));

         cfg.set_high_frequency_interval_secs(0U);
         EXPECT_EQ(1U, cfg.get_high_frequency_interval_secs());
       }},
      {JSON_HOST_CONFIG,
       [](app_config &cfg) {
         host_config cfg1{};
         cfg1.agent_string = "1";
         cfg1.api_password = "2";
         cfg1.api_user = "3";
         cfg1.api_port = 4U;
         cfg1.host_name_or_ip = "5";
         cfg1.path = "6";
         cfg1.protocol = "http";
         cfg1.timeout_ms = 8U;

         host_config cfg2{};
         cfg2.agent_string = "9";
         cfg2.api_password = "10";
         cfg2.api_user = "11";
         cfg2.api_port = 12U;
         cfg2.host_name_or_ip = "13";
         cfg2.path = "14";
         cfg2.protocol = "https";
         cfg2.timeout_ms = 16U;

         ASSERT_NE(cfg1, cfg2);

         test_getter_setter(cfg, &app_config::get_host_config,
                            &app_config::set_host_config, cfg1, cfg2, "", "");

         host_config cfg3{};
         cfg3.agent_string = "17";
         cfg3.api_password = "18";
         cfg3.api_user = "19";
         cfg3.api_port = 20U;
         cfg3.host_name_or_ip = "21";
         cfg3.path = "22";
         cfg3.protocol = "http";
         cfg3.timeout_ms = 24;

         auto value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_AGENT_STRING),
             cfg3.agent_string);
         EXPECT_STREQ(cfg3.agent_string.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_API_PASSWORD),
             cfg3.api_password);
         EXPECT_STREQ(cfg3.api_password.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_API_USER),
             cfg3.api_user);
         EXPECT_STREQ(cfg3.api_user.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_API_PORT),
             std::to_string(cfg3.api_port));
         EXPECT_STREQ(std::to_string(cfg3.api_port).c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_HOST_NAME_OR_IP),
             cfg3.host_name_or_ip);
         EXPECT_STREQ(cfg3.host_name_or_ip.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_PATH), cfg3.path);
         EXPECT_STREQ(cfg3.path.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_PROTOCOL),
             cfg3.protocol);
         EXPECT_STREQ(cfg3.protocol.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_TIMEOUT_MS),
             std::to_string(cfg3.timeout_ms));
         EXPECT_STREQ(std::to_string(cfg3.timeout_ms).c_str(), value.c_str());
       }},
      {JSON_LOW_FREQ_INTERVAL_SECS,
       [](app_config &cfg) {
         test_getter_setter(
             cfg, &app_config::get_low_frequency_interval_secs,
             &app_config::set_low_frequency_interval_secs,
             std::uint16_t{default_low_freq_interval_secs + 1U},
             std::uint16_t{default_low_freq_interval_secs + 2U},
             JSON_LOW_FREQ_INTERVAL_SECS,
             std::to_string(default_low_freq_interval_secs + 3U));

         cfg.set_low_frequency_interval_secs(0U);
         EXPECT_EQ(1U, cfg.get_low_frequency_interval_secs());
       }},
      {JSON_MAX_CACHE_SIZE_BYTES,
       [](app_config &cfg) {
         test_getter_setter(
             cfg, &app_config::get_max_cache_size_bytes,
             &app_config::set_max_cache_size_bytes, min_cache_size_bytes + 1U,
             min_cache_size_bytes + 2U, JSON_MAX_CACHE_SIZE_BYTES,
             std::to_string(min_cache_size_bytes + 3U));

         cfg.set_max_cache_size_bytes(min_cache_size_bytes - 1U);
         EXPECT_EQ(min_cache_size_bytes, cfg.get_max_cache_size_bytes());
       }},
      {JSON_MAX_UPLOAD_COUNT,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_max_upload_count,
                            &app_config::set_max_upload_count, std::uint8_t{1U},
                            std::uint8_t{2U}, JSON_MAX_UPLOAD_COUNT, "3");

         cfg.set_max_upload_count(0U);
         EXPECT_EQ(1U, cfg.get_max_upload_count());
       }},
      {JSON_MED_FREQ_INTERVAL_SECS,
       [](app_config &cfg) {
         test_getter_setter(
             cfg, &app_config::get_med_frequency_interval_secs,
             &app_config::set_med_frequency_interval_secs,
             std::uint16_t{default_med_freq_interval_secs + 1U},
             std::uint16_t{default_med_freq_interval_secs + 2U},
             JSON_MED_FREQ_INTERVAL_SECS,
             std::to_string(default_med_freq_interval_secs + 3U));

         cfg.set_med_frequency_interval_secs(0U);
         EXPECT_EQ(1U, cfg.get_med_frequency_interval_secs());
       }},
      {JSON_ONLINE_CHECK_RETRY_SECS,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_online_check_retry_secs,
                            &app_config::set_online_check_retry_secs,
                            std::uint16_t{min_online_check_retry_secs + 1U},
                            std::uint16_t{min_online_check_retry_secs + 2U},
                            JSON_ONLINE_CHECK_RETRY_SECS,
                            std::to_string(min_online_check_retry_secs + 3U));

         cfg.set_online_check_retry_secs(min_online_check_retry_secs - 1U);
         EXPECT_EQ(min_online_check_retry_secs,
                   cfg.get_online_check_retry_secs());
       }},
      {JSON_PREFERRED_DOWNLOAD_TYPE,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_preferred_download_type,
                            &app_config::set_preferred_download_type,
                            download_type::direct, download_type::default_,
                            JSON_PREFERRED_DOWNLOAD_TYPE, "ring_buffer");
       }},
      {JSON_REMOTE_CONFIG,
       [](app_config &cfg) {
         remote::remote_config remote_cfg1{};
         remote_cfg1.api_port = 1U;
         remote_cfg1.encryption_token = "2";
         remote_cfg1.host_name_or_ip = "3";
         remote_cfg1.max_connections = 4U;
         remote_cfg1.recv_timeout_ms = 5U;
         remote_cfg1.send_timeout_ms = 6U;
         remote_cfg1.conn_timeout_ms = 7U;

         remote::remote_config remote_cfg2{};
         remote_cfg1.api_port = 7U;
         remote_cfg1.encryption_token = "6";
         remote_cfg1.host_name_or_ip = "6";
         remote_cfg1.max_connections = 4U;
         remote_cfg1.recv_timeout_ms = 3U;
         remote_cfg1.send_timeout_ms = 2U;
         remote_cfg1.conn_timeout_ms = 1U;

         ASSERT_NE(remote_cfg1, remote_cfg2);

         test_getter_setter(cfg, &app_config::get_remote_config,
                            &app_config::set_remote_config, remote_cfg1,
                            remote_cfg2, "", "");

         remote::remote_config remote_cfg3{};
         remote_cfg1.api_port = 7U;
         remote_cfg1.encryption_token = "8";
         remote_cfg1.host_name_or_ip = "9";
         remote_cfg1.max_connections = 10U;
         remote_cfg1.recv_timeout_ms = 11U;
         remote_cfg1.send_timeout_ms = 12U;
         remote_cfg1.conn_timeout_ms = 13U;

         auto value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_API_PORT),
             std::to_string(remote_cfg3.api_port));
         EXPECT_STREQ(std::to_string(remote_cfg3.api_port).c_str(),
                      value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_CONNECT_TIMEOUT_MS),
             std::to_string(remote_cfg3.conn_timeout_ms));
         EXPECT_STREQ(std::to_string(remote_cfg3.conn_timeout_ms).c_str(),
                      value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_ENCRYPTION_TOKEN),
             remote_cfg3.encryption_token);
         EXPECT_STREQ(remote_cfg3.encryption_token.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_HOST_NAME_OR_IP),
             remote_cfg3.host_name_or_ip);
         EXPECT_STREQ(remote_cfg3.host_name_or_ip.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_MAX_CONNECTIONS),
             std::to_string(remote_cfg3.max_connections));
         EXPECT_STREQ(std::to_string(remote_cfg3.max_connections).c_str(),
                      value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_RECV_TIMEOUT_MS),
             std::to_string(remote_cfg3.recv_timeout_ms));
         EXPECT_STREQ(std::to_string(remote_cfg3.recv_timeout_ms).c_str(),
                      value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_SEND_TIMEOUT_MS),
             std::to_string(remote_cfg3.send_timeout_ms));
         EXPECT_STREQ(std::to_string(remote_cfg3.send_timeout_ms).c_str(),
                      value.c_str());
       }},
      {JSON_REMOTE_MOUNT,
       [](app_config &cfg) {
         remote::remote_mount mnt_cfg1{};
         mnt_cfg1.api_port = 1U;
         mnt_cfg1.client_pool_size = 2U;
         mnt_cfg1.enable = false;
         mnt_cfg1.encryption_token = "3";

         remote::remote_mount mnt_cfg2{};
         mnt_cfg2.api_port = 3U;
         mnt_cfg2.client_pool_size = 4U;
         mnt_cfg2.enable = true;
         mnt_cfg2.encryption_token = "5";

         ASSERT_NE(mnt_cfg1, mnt_cfg2);

         test_getter_setter(cfg, &app_config::get_remote_mount,
                            &app_config::set_remote_mount, mnt_cfg1, mnt_cfg2,
                            "", "");

         remote::remote_mount mnt_cfg3{};
         mnt_cfg3.api_port = 9U;
         mnt_cfg3.client_pool_size = 10U;
         mnt_cfg3.enable = false;
         mnt_cfg3.encryption_token = "11";

         auto value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_API_PORT),
             std::to_string(mnt_cfg3.api_port));
         EXPECT_STREQ(std::to_string(mnt_cfg3.api_port).c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_CLIENT_POOL_SIZE),
             std::to_string(mnt_cfg3.client_pool_size));
         EXPECT_STREQ(std::to_string(mnt_cfg3.client_pool_size).c_str(),
                      value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_ENABLE_REMOTE_MOUNT),
             utils::string::from_bool(mnt_cfg3.enable));
         EXPECT_STREQ(utils::string::from_bool(mnt_cfg3.enable).c_str(),
                      value.c_str());
       }},
      {JSON_RETRY_READ_COUNT,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_retry_read_count,
                            &app_config::set_retry_read_count,
                            std::uint16_t{min_retry_read_count + 1U},
                            std::uint16_t{min_retry_read_count + 2U},
                            JSON_RETRY_READ_COUNT,
                            std::to_string(min_retry_read_count + 3U));

         cfg.set_retry_read_count(min_retry_read_count - 1U);
         EXPECT_EQ(min_retry_read_count, cfg.get_retry_read_count());
       }},
      {JSON_RING_BUFFER_FILE_SIZE,
       [](app_config &cfg) {
         test_getter_setter(cfg, &app_config::get_ring_buffer_file_size,
                            &app_config::set_ring_buffer_file_size,
                            std::uint16_t{min_ring_buffer_file_size + 1U},
                            std::uint16_t{min_ring_buffer_file_size + 2U},
                            JSON_RING_BUFFER_FILE_SIZE,
                            std::to_string(min_ring_buffer_file_size + 3U));

         cfg.set_ring_buffer_file_size(min_ring_buffer_file_size - 1U);
         EXPECT_EQ(min_ring_buffer_file_size, cfg.get_ring_buffer_file_size());

         cfg.set_ring_buffer_file_size(max_ring_buffer_file_size + 1U);
         EXPECT_EQ(max_ring_buffer_file_size, cfg.get_ring_buffer_file_size());
       }},
      {JSON_S3_CONFIG,
       [](auto &&cfg) {
         s3_config cfg1{};
         cfg1.access_key = "1";
         cfg1.bucket = "2";
         cfg1.encryption_token = "3";
         cfg1.region = "4";
         cfg1.secret_key = "5";
         cfg1.timeout_ms = 6U;
         cfg1.url = "7";
         cfg1.use_path_style = false;
         cfg1.use_region_in_url = false;
         cfg1.force_legacy_encryption = false;

         s3_config cfg2{};
         cfg2.access_key = "8";
         cfg2.bucket = "9";
         cfg2.encryption_token = "10";
         cfg2.region = "11";
         cfg2.secret_key = "12";
         cfg2.timeout_ms = 13U;
         cfg2.url = "14";
         cfg2.use_path_style = true;
         cfg2.use_region_in_url = true;
         cfg2.force_legacy_encryption = true;

         ASSERT_NE(cfg1, cfg2);

         test_getter_setter(cfg, &app_config::get_s3_config,
                            &app_config::set_s3_config, cfg1, cfg2, "", "");

         s3_config cfg3{};
         cfg3.access_key = "8";
         cfg3.bucket = "9";
         cfg3.encryption_token = "10";
         cfg3.region = "11";
         cfg3.secret_key = "12";
         cfg3.timeout_ms = 13U;
         cfg3.url = "14";
         cfg3.use_path_style = true;
         cfg3.use_region_in_url = true;
         cfg3.force_legacy_encryption = true;

         auto value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_S3_CONFIG, JSON_ACCESS_KEY),
             cfg3.access_key);
         EXPECT_STREQ(cfg3.access_key.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_S3_CONFIG, JSON_BUCKET), cfg3.bucket);
         EXPECT_STREQ(cfg3.bucket.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_S3_CONFIG, JSON_ENCRYPTION_TOKEN),
             cfg3.encryption_token);
         EXPECT_STREQ(cfg3.encryption_token.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_S3_CONFIG, JSON_REGION), cfg3.region);
         EXPECT_STREQ(cfg3.region.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_S3_CONFIG, JSON_SECRET_KEY),
             cfg3.secret_key);
         EXPECT_STREQ(cfg3.secret_key.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_S3_CONFIG, JSON_TIMEOUT_MS),
             std::to_string(cfg3.timeout_ms));
         EXPECT_STREQ(std::to_string(cfg3.timeout_ms).c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_S3_CONFIG, JSON_URL), cfg3.url);
         EXPECT_STREQ(cfg3.url.c_str(), value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_S3_CONFIG, JSON_USE_PATH_STYLE),
             utils::string::from_bool(cfg3.use_path_style));
         EXPECT_STREQ(utils::string::from_bool(cfg3.use_path_style).c_str(),
                      value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_S3_CONFIG, JSON_USE_REGION_IN_URL),
             utils::string::from_bool(cfg3.use_region_in_url));
         EXPECT_STREQ(utils::string::from_bool(cfg3.use_region_in_url).c_str(),
                      value.c_str());

         value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_S3_CONFIG, JSON_FORCE_LEGACY_ENCRYPTION),
             utils::string::from_bool(cfg3.force_legacy_encryption));
         EXPECT_STREQ(
             utils::string::from_bool(cfg3.force_legacy_encryption).c_str(),
             value.c_str());
       }},
      {JSON_SIA_CONFIG,
       [](app_config &cfg) {
         sia_config cfg1{};
         cfg1.bucket = "1";

         sia_config cfg2{};
         cfg2.bucket = "2";

         ASSERT_NE(cfg1, cfg2);

         test_getter_setter(cfg, &app_config::get_sia_config,
                            &app_config::set_sia_config, cfg1, cfg2, "", "");

         sia_config cfg3{};
         cfg3.bucket = "3";

         auto value = cfg.set_value_by_name(
             fmt::format("{}.{}", JSON_SIA_CONFIG, JSON_BUCKET), cfg3.bucket);
         EXPECT_STREQ(cfg3.bucket.c_str(), value.c_str());
       }},
      {JSON_TASK_WAIT_MS,
       [](app_config &cfg) {
         test_getter_setter(
             cfg, &app_config::get_task_wait_ms, &app_config::set_task_wait_ms,
             std::uint16_t{min_task_wait_ms + 1U},
             std::uint16_t{min_task_wait_ms + 2U}, JSON_TASK_WAIT_MS,
             std::to_string(min_task_wait_ms + 3U));

         cfg.set_task_wait_ms(min_task_wait_ms - 1U);
         EXPECT_EQ(min_task_wait_ms, cfg.get_task_wait_ms());
       }},
  };

  remove_unused_types(methods, prov);

  for (const auto &[key, test_function] : methods) {
    fmt::println("testing setting|{}-{}", app_config::get_provider_name(prov),
                 key);
    test_function(config);
  }
}

TEST_F(app_config_test, encrypt_config) {
  app_config config(provider_type::encrypt, encrypt_directory);
  defaults_tests(config.get_json(), provider_type::encrypt);
  common_tests(config, provider_type::encrypt);
}

TEST_F(app_config_test, remote_config) {
  app_config config(provider_type::remote, remote_directory);
  defaults_tests(config.get_json(), provider_type::remote);
  common_tests(config, provider_type::remote);
}

TEST_F(app_config_test, s3_config) {
  app_config config(provider_type::s3, s3_directory);
  defaults_tests(config.get_json(), provider_type::s3);
  common_tests(config, provider_type::s3);
}

TEST_F(app_config_test, sia_config) {
  app_config config(provider_type::sia, sia_directory);
  defaults_tests(config.get_json(), provider_type::sia);
  common_tests(config, provider_type::sia);
}
} // namespace repertory
