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
#include "app_config.hpp"

#include "events/event_system.hpp"
#include "events/types/event_level_changed.hpp"
#include "file_manager/cache_size_mgr.hpp"
#include "platform/platform.hpp"
#include "types/startup_exception.hpp"
#include "utils/common.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"
#include "utils/utils.hpp"

namespace {
template <typename dest>
auto get_value(const json &data, const std::string &name, dest &dst,
               bool &found) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (data.find(name) == data.end()) {
      found = false;
      return false;
    }

    data.at(name).get_to(dst);
    return true;
  } catch (const std::exception &ex) {
    repertory::utils::error::raise_error(
        function_name, ex, fmt::format("failed to get value|name|{}", name));
  }

  found = false;
  return false;
}
} // namespace

namespace repertory {
stop_type app_config::stop_requested{false};

auto app_config::get_stop_requested() -> bool { return stop_requested.load(); }

void app_config::set_stop_requested() { stop_requested.store(true); }

app_config::app_config(const provider_type &prov,
                       std::string_view data_directory)
    : prov_(prov),
      api_auth_(utils::generate_random_string(default_api_auth_size)),
      api_port_(default_rpc_port(prov)),
      api_user_(std::string{REPERTORY}),
      config_changed_(false),
      download_timeout_secs_(default_download_timeout_secs),
      enable_download_timeout_(true),
      enable_drive_events_(false),
#if defined(_WIN32)
      enable_mount_manager_(false),
#endif // defined(_WIN32)
      event_level_(event_level::info),
      eviction_delay_mins_(default_eviction_delay_mins),
      eviction_uses_accessed_time_(false),
      high_freq_interval_secs_(default_high_freq_interval_secs),
      low_freq_interval_secs_(default_low_freq_interval_secs),
      max_cache_size_bytes_(default_max_cache_size_bytes),
      max_upload_count_(default_max_upload_count),
      med_freq_interval_secs_(default_med_freq_interval_secs),
      online_check_retry_secs_(default_online_check_retry_secs),
      preferred_download_type_(download_type::default_),
      retry_read_count_(default_retry_read_count),
      ring_buffer_file_size_(default_ring_buffer_file_size),
      task_wait_ms_(default_task_wait_ms) {
  data_directory_ = data_directory.empty()
                        ? default_data_directory(prov)
                        : utils::path::absolute(data_directory);
  cache_directory_ = utils::path::combine(data_directory_, {"cache"});
  log_directory_ = utils::path::combine(data_directory_, {"logs"});

  auto host_cfg = get_host_config();
  host_cfg.agent_string = default_agent_name(prov_);
  host_cfg.api_port = default_api_port(prov_);
  host_config_ = host_cfg;

  auto rm_cfg = get_remote_mount();
  rm_cfg.api_port = default_remote_api_port(prov);
  remote_mount_ = rm_cfg;

  if (not utils::file::directory(data_directory_).create_directory()) {
    throw startup_exception(
        fmt::format("unable to create data directory|sp|{}", data_directory_));
  }

  if (not utils::file::directory(cache_directory_).create_directory()) {
    throw startup_exception(fmt::format(
        "unable to create cache directory|sp|{}", cache_directory_));
  }

  if (not utils::file::directory(log_directory_).create_directory()) {
    throw startup_exception(
        fmt::format("unable to create log directory|sp|{}", log_directory_));
  }

  if (not load()) {
    save();
  }

  value_get_lookup_ = {
      {JSON_API_AUTH, [this]() { return get_api_auth(); }},
      {JSON_API_PORT, [this]() { return std::to_string(get_api_port()); }},
      {JSON_API_USER, [this]() { return get_api_user(); }},
      {JSON_DATABASE_TYPE,
       [this]() { return database_type_to_string(get_database_type()); }},
      {JSON_DOWNLOAD_TIMEOUT_SECS,
       [this]() { return std::to_string(get_download_timeout_secs()); }},
      {JSON_ENABLE_DOWNLOAD_TIMEOUT,
       [this]() {
         return utils::string::from_bool(get_enable_download_timeout());
       }},
      {JSON_ENABLE_DRIVE_EVENTS,
       [this]() {
         return utils::string::from_bool(get_enable_drive_events());
       }},
#if defined(_WIN32)
      {JSON_ENABLE_MOUNT_MANAGER,
       [this]() {
         return utils::string::from_bool(get_enable_mount_manager());
       }},
#endif // defined(_WIN32)
      {fmt::format("{}.{}", JSON_ENCRYPT_CONFIG, JSON_ENCRYPTION_TOKEN),
       [this]() { return get_encrypt_config().encryption_token; }},
      {fmt::format("{}.{}", JSON_ENCRYPT_CONFIG, JSON_PATH),
       [this]() { return utils::path::absolute(get_encrypt_config().path); }},
      {JSON_EVENT_LEVEL,
       [this]() { return event_level_to_string(get_event_level()); }},
      {JSON_EVICTION_DELAY_MINS,
       [this]() { return std::to_string(get_eviction_delay_mins()); }},
      {JSON_EVICTION_USE_ACCESS_TIME,
       [this]() {
         return utils::string::from_bool(get_eviction_uses_accessed_time());
       }},
      {JSON_HIGH_FREQ_INTERVAL_SECS,
       [this]() { return std::to_string(get_high_frequency_interval_secs()); }},
      {fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_AGENT_STRING),
       [this]() { return get_host_config().agent_string; }},
      {fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_API_PASSWORD),
       [this]() { return get_host_config().api_password; }},
      {fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_API_PORT),
       [this]() { return std::to_string(get_host_config().api_port); }},
      {fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_API_USER),
       [this]() { return get_host_config().api_user; }},
      {fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_HOST_NAME_OR_IP),
       [this]() { return get_host_config().host_name_or_ip; }},
      {fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_PATH),
       [this]() { return get_host_config().path; }},
      {fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_PROTOCOL),
       [this]() { return get_host_config().protocol; }},
      {fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_TIMEOUT_MS),
       [this]() { return std::to_string(get_host_config().timeout_ms); }},
      {JSON_LOW_FREQ_INTERVAL_SECS,
       [this]() { return std::to_string(get_low_frequency_interval_secs()); }},
      {JSON_MAX_CACHE_SIZE_BYTES,
       [this]() { return std::to_string(get_max_cache_size_bytes()); }},
      {JSON_MAX_UPLOAD_COUNT,
       [this]() { return std::to_string(get_max_upload_count()); }},
      {JSON_MED_FREQ_INTERVAL_SECS,
       [this]() { return std::to_string(get_med_frequency_interval_secs()); }},
      {JSON_ONLINE_CHECK_RETRY_SECS,
       [this]() { return std::to_string(get_online_check_retry_secs()); }},
      {JSON_PREFERRED_DOWNLOAD_TYPE,
       [this]() {
         return download_type_to_string(get_preferred_download_type());
       }},
      {fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_API_PORT),
       [this]() { return std::to_string(get_remote_config().api_port); }},
      {fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_ENCRYPTION_TOKEN),
       [this]() { return get_remote_config().encryption_token; }},
      {fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_HOST_NAME_OR_IP),
       [this]() { return get_remote_config().host_name_or_ip; }},
      {fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_MAX_CONNECTIONS),
       [this]() {
         return std::to_string(get_remote_config().max_connections);
       }},
      {fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_RECV_TIMEOUT_MS),
       [this]() {
         return std::to_string(get_remote_config().recv_timeout_ms);
       }},
      {fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_SEND_TIMEOUT_MS),
       [this]() {
         return std::to_string(get_remote_config().send_timeout_ms);
       }},
      {fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_API_PORT),
       [this]() { return std::to_string(get_remote_mount().api_port); }},
      {fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_CLIENT_POOL_SIZE),
       [this]() {
         return std::to_string(get_remote_mount().client_pool_size);
       }},
      {fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_ENABLE_REMOTE_MOUNT),
       [this]() {
         return utils::string::from_bool(get_remote_mount().enable);
       }},
      {fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_ENCRYPTION_TOKEN),
       [this]() { return get_remote_mount().encryption_token; }},
      {JSON_RETRY_READ_COUNT,
       [this]() { return std::to_string(get_retry_read_count()); }},
      {JSON_RING_BUFFER_FILE_SIZE,
       [this]() { return std::to_string(get_ring_buffer_file_size()); }},
      {fmt::format("{}.{}", JSON_S3_CONFIG, JSON_ACCESS_KEY),
       [this]() { return get_s3_config().access_key; }},
      {fmt::format("{}.{}", JSON_S3_CONFIG, JSON_BUCKET),
       [this]() { return get_s3_config().bucket; }},
      {fmt::format("{}.{}", JSON_S3_CONFIG, JSON_ENCRYPTION_TOKEN),
       [this]() { return get_s3_config().encryption_token; }},
      {fmt::format("{}.{}", JSON_S3_CONFIG, JSON_REGION),
       [this]() { return get_s3_config().region; }},
      {fmt::format("{}.{}", JSON_S3_CONFIG, JSON_SECRET_KEY),
       [this]() { return get_s3_config().secret_key; }},
      {fmt::format("{}.{}", JSON_S3_CONFIG, JSON_TIMEOUT_MS),
       [this]() { return std::to_string(get_s3_config().timeout_ms); }},
      {fmt::format("{}.{}", JSON_S3_CONFIG, JSON_URL),
       [this]() { return get_s3_config().url; }},
      {fmt::format("{}.{}", JSON_S3_CONFIG, JSON_USE_PATH_STYLE),
       [this]() {
         return utils::string::from_bool(get_s3_config().use_path_style);
       }},
      {fmt::format("{}.{}", JSON_S3_CONFIG, JSON_USE_REGION_IN_URL),
       [this]() {
         return utils::string::from_bool(get_s3_config().use_region_in_url);
       }},
      {fmt::format("{}.{}", JSON_SIA_CONFIG, JSON_BUCKET),
       [this]() { return get_sia_config().bucket; }},
      {JSON_TASK_WAIT_MS,
       [this]() { return std::to_string(get_task_wait_ms()); }},
  };

  value_set_lookup_ = {
      {
          JSON_API_AUTH,
          [this](const std::string &value) {
            set_api_auth(value);
            return get_api_auth();
          },
      },
      {
          JSON_API_PORT,
          [this](const std::string &value) {
            set_api_port(utils::string::to_uint16(value));
            return std::to_string(get_api_port());
          },
      },
      {
          JSON_API_USER,
          [this](const std::string &value) {
            set_api_user(value);
            return get_api_user();
          },
      },
      {
          JSON_DATABASE_TYPE,
          [this](const std::string &value) {
            set_database_type(database_type_from_string(value));
            return database_type_to_string(db_type_);
          },
      },
      {
          JSON_DOWNLOAD_TIMEOUT_SECS,
          [this](const std::string &value) {
            set_download_timeout_secs(utils::string::to_uint8(value));
            return std::to_string(get_download_timeout_secs());
          },
      },
      {
          JSON_ENABLE_DOWNLOAD_TIMEOUT,
          [this](const std::string &value) {
            set_enable_download_timeout(utils::string::to_bool(value));
            return utils::string::from_bool(get_enable_download_timeout());
          },
      },
      {
          JSON_ENABLE_DRIVE_EVENTS,
          [this](const std::string &value) {
            set_enable_drive_events(utils::string::to_bool(value));
            return utils::string::from_bool(get_enable_drive_events());
          },
      },
#if defined(_WIN32)
      {
          JSON_ENABLE_MOUNT_MANAGER,
          [this](const std::string &value) {
            set_enable_mount_manager(utils::string::to_bool(value));
            return utils::string::from_bool(get_enable_mount_manager());
          },
      },
#endif // defined(_WIN32)
      {
          fmt::format("{}.{}", JSON_ENCRYPT_CONFIG, JSON_ENCRYPTION_TOKEN),
          [this](const std::string &value) {
            auto cfg = get_encrypt_config();
            cfg.encryption_token = value;
            set_encrypt_config(cfg);
            return get_encrypt_config().encryption_token;
          },
      },
      {
          fmt::format("{}.{}", JSON_ENCRYPT_CONFIG, JSON_PATH),
          [this](const std::string &value) {
            auto cfg = get_encrypt_config();
            cfg.path = value;
            set_encrypt_config(cfg);
            return get_encrypt_config().path;
          },
      },
      {
          JSON_EVENT_LEVEL,
          [this](const std::string &value) {
            set_event_level(event_level_from_string(value));
            return event_level_to_string(get_event_level());
          },
      },
      {
          JSON_EVICTION_DELAY_MINS,
          [this](const std::string &value) {
            set_eviction_delay_mins(utils::string::to_uint32(value));
            return std::to_string(get_eviction_delay_mins());
          },
      },
      {
          JSON_EVICTION_USE_ACCESS_TIME,
          [this](const std::string &value) {
            set_eviction_uses_accessed_time(utils::string::to_bool(value));
            return utils::string::from_bool(get_eviction_uses_accessed_time());
          },
      },
      {
          JSON_HIGH_FREQ_INTERVAL_SECS,
          [this](const std::string &value) {
            set_high_frequency_interval_secs(utils::string::to_uint16(value));
            return std::to_string(get_high_frequency_interval_secs());
          },
      },
      {
          fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_AGENT_STRING),
          [this](const std::string &value) {
            auto cfg = get_host_config();
            cfg.agent_string = value;
            set_host_config(cfg);
            return get_host_config().agent_string;
          },
      },
      {
          fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_API_PASSWORD),
          [this](const std::string &value) {
            auto cfg = get_host_config();
            cfg.api_password = value;
            set_host_config(cfg);
            return get_host_config().api_password;
          },
      },
      {
          fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_API_PORT),
          [this](const std::string &value) {
            auto cfg = get_host_config();
            cfg.api_port = utils::string::to_uint16(value);
            set_host_config(cfg);
            return std::to_string(get_host_config().api_port);
          },
      },
      {
          fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_API_USER),
          [this](const std::string &value) {
            auto cfg = get_host_config();
            cfg.api_user = value;
            set_host_config(cfg);
            return get_host_config().api_user;
          },
      },
      {
          fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_HOST_NAME_OR_IP),
          [this](const std::string &value) {
            auto cfg = get_host_config();
            cfg.host_name_or_ip = value;
            set_host_config(cfg);
            return get_host_config().host_name_or_ip;
          },
      },
      {
          fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_PATH),
          [this](const std::string &value) {
            auto cfg = get_host_config();
            cfg.path = value;
            set_host_config(cfg);
            return get_host_config().path;
          },
      },
      {
          fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_PROTOCOL),
          [this](const std::string &value) {
            auto cfg = get_host_config();
            cfg.protocol = value;
            set_host_config(cfg);
            return get_host_config().protocol;
          },
      },
      {
          fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_TIMEOUT_MS),
          [this](const std::string &value) {
            auto cfg = get_host_config();
            cfg.timeout_ms = utils::string::to_uint32(value);
            set_host_config(cfg);
            return std::to_string(get_host_config().timeout_ms);
          },
      },
      {
          JSON_LOW_FREQ_INTERVAL_SECS,
          [this](const std::string &value) {
            set_low_frequency_interval_secs(utils::string::to_uint16(value));
            return std::to_string(get_low_frequency_interval_secs());
          },
      },
      {
          JSON_MED_FREQ_INTERVAL_SECS,
          [this](const std::string &value) {
            set_med_frequency_interval_secs(utils::string::to_uint16(value));
            return std::to_string(get_med_frequency_interval_secs());
          },
      },
      {
          JSON_MAX_CACHE_SIZE_BYTES,
          [this](const std::string &value) {
            set_max_cache_size_bytes(utils::string::to_uint64(value));
            return std::to_string(get_max_cache_size_bytes());
          },
      },
      {
          JSON_MAX_UPLOAD_COUNT,
          [this](const std::string &value) {
            set_max_upload_count(utils::string::to_uint8(value));
            return std::to_string(get_max_upload_count());
          },
      },
      {
          JSON_ONLINE_CHECK_RETRY_SECS,
          [this](const std::string &value) {
            set_online_check_retry_secs(utils::string::to_uint16(value));
            return std::to_string(get_online_check_retry_secs());
          },
      },
      {
          JSON_PREFERRED_DOWNLOAD_TYPE,
          [this](const std::string &value) {
            set_preferred_download_type(download_type_from_string(value));
            return download_type_to_string(get_preferred_download_type());
          },
      },
      {
          fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_API_PORT),
          [this](const std::string &value) {
            auto cfg = get_remote_config();
            cfg.api_port = utils::string::to_uint16(value);
            set_remote_config(cfg);
            return std::to_string(get_remote_config().api_port);
          },
      },
      {
          fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_ENCRYPTION_TOKEN),
          [this](const std::string &value) {
            auto cfg = get_remote_config();
            cfg.encryption_token = value;
            set_remote_config(cfg);
            return get_remote_config().encryption_token;
          },
      },
      {
          fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_HOST_NAME_OR_IP),
          [this](const std::string &value) {
            auto cfg = get_remote_config();
            cfg.host_name_or_ip = value;
            set_remote_config(cfg);
            return get_remote_config().host_name_or_ip;
          },
      },
      {
          fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_MAX_CONNECTIONS),
          [this](const std::string &value) {
            auto cfg = get_remote_config();
            cfg.max_connections = utils::string::to_uint8(value);
            set_remote_config(cfg);
            return std::to_string(get_remote_config().max_connections);
          },
      },
      {
          fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_RECV_TIMEOUT_MS),
          [this](const std::string &value) {
            auto cfg = get_remote_config();
            cfg.recv_timeout_ms = utils::string::to_uint32(value);
            set_remote_config(cfg);
            return std::to_string(get_remote_config().recv_timeout_ms);
          },
      },
      {
          fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_SEND_TIMEOUT_MS),
          [this](const std::string &value) {
            auto cfg = get_remote_config();
            cfg.send_timeout_ms = utils::string::to_uint32(value);
            set_remote_config(cfg);
            return std::to_string(get_remote_config().send_timeout_ms);
          },
      },
      {
          fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_API_PORT),
          [this](const std::string &value) {
            auto cfg = get_remote_mount();
            cfg.api_port = utils::string::to_uint16(value);
            set_remote_mount(cfg);
            return std::to_string(get_remote_mount().api_port);
          },
      },
      {
          fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_CLIENT_POOL_SIZE),
          [this](const std::string &value) {
            auto cfg = get_remote_mount();
            cfg.client_pool_size = utils::string::to_uint8(value);
            set_remote_mount(cfg);
            return std::to_string(get_remote_mount().client_pool_size);
          },
      },
      {
          fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_ENABLE_REMOTE_MOUNT),
          [this](const std::string &value) {
            auto cfg = get_remote_mount();
            cfg.enable = utils::string::to_bool(value);
            set_remote_mount(cfg);
            return utils::string::from_bool(get_remote_mount().enable);
          },
      },
      {
          fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_ENCRYPTION_TOKEN),
          [this](const std::string &value) {
            auto cfg = get_remote_mount();
            cfg.encryption_token = value;
            set_remote_mount(cfg);
            return get_remote_mount().encryption_token;
          },
      },
      {
          JSON_RETRY_READ_COUNT,
          [this](const std::string &value) {
            set_retry_read_count(utils::string::to_uint16(value));
            return std::to_string(get_retry_read_count());
          },
      },
      {
          JSON_RING_BUFFER_FILE_SIZE,
          [this](const std::string &value) {
            set_ring_buffer_file_size(utils::string::to_uint16(value));
            return std::to_string(get_ring_buffer_file_size());
          },
      },
      {
          fmt::format("{}.{}", JSON_S3_CONFIG, JSON_ACCESS_KEY),
          [this](const std::string &value) {
            auto cfg = get_s3_config();
            cfg.access_key = value;
            set_s3_config(cfg);
            return get_s3_config().access_key;
          },
      },
      {
          fmt::format("{}.{}", JSON_S3_CONFIG, JSON_BUCKET),
          [this](const std::string &value) {
            auto cfg = get_s3_config();
            cfg.bucket = value;
            set_s3_config(cfg);
            return get_s3_config().bucket;
          },
      },
      {
          fmt::format("{}.{}", JSON_S3_CONFIG, JSON_ENCRYPTION_TOKEN),
          [this](const std::string &value) {
            auto cfg = get_s3_config();
            cfg.encryption_token = value;
            set_s3_config(cfg);
            return get_s3_config().encryption_token;
          },
      },
      {
          fmt::format("{}.{}", JSON_S3_CONFIG, JSON_REGION),
          [this](const std::string &value) {
            auto cfg = get_s3_config();
            cfg.region = value;
            set_s3_config(cfg);
            return get_s3_config().region;
          },
      },
      {
          fmt::format("{}.{}", JSON_S3_CONFIG, JSON_SECRET_KEY),
          [this](const std::string &value) {
            auto cfg = get_s3_config();
            cfg.secret_key = value;
            set_s3_config(cfg);
            return get_s3_config().secret_key;
          },
      },
      {
          fmt::format("{}.{}", JSON_S3_CONFIG, JSON_TIMEOUT_MS),
          [this](const std::string &value) {
            auto cfg = get_s3_config();
            cfg.timeout_ms = utils::string::to_uint32(value);
            set_s3_config(cfg);
            return std::to_string(get_s3_config().timeout_ms);
          },
      },
      {
          fmt::format("{}.{}", JSON_S3_CONFIG, JSON_URL),
          [this](const std::string &value) {
            auto cfg = get_s3_config();
            cfg.url = value;
            set_s3_config(cfg);
            return get_s3_config().url;
          },
      },
      {
          fmt::format("{}.{}", JSON_S3_CONFIG, JSON_USE_PATH_STYLE),
          [this](const std::string &value) {
            auto cfg = get_s3_config();
            cfg.use_path_style = utils::string::to_bool(value);
            set_s3_config(cfg);
            return utils::string::from_bool(get_s3_config().use_path_style);
          },
      },
      {
          fmt::format("{}.{}", JSON_S3_CONFIG, JSON_USE_REGION_IN_URL),
          [this](const std::string &value) {
            auto cfg = get_s3_config();
            cfg.use_region_in_url = utils::string::to_bool(value);
            set_s3_config(cfg);
            return utils::string::from_bool(get_s3_config().use_region_in_url);
          },
      },
      {
          fmt::format("{}.{}", JSON_SIA_CONFIG, JSON_BUCKET),
          [this](const std::string &value) {
            auto cfg = get_sia_config();
            cfg.bucket = value;
            set_sia_config(cfg);
            return get_sia_config().bucket;
          },
      },
      {
          JSON_TASK_WAIT_MS,
          [this](const std::string &value) {
            set_task_wait_ms(utils::string::to_uint16(value));
            return std::to_string(get_task_wait_ms());
          },
      },
  };
}

auto app_config::default_agent_name(const provider_type &prov) -> std::string {
  static const std::array<std::string,
                          static_cast<std::size_t>(provider_type::unknown)>
      PROVIDER_AGENT_NAMES = {
          "Sia-Agent",
          "",
          "",
          "",
      };

  return PROVIDER_AGENT_NAMES.at(static_cast<std::size_t>(prov));
}

auto app_config::default_api_port(const provider_type &prov) -> std::uint16_t {
  static const std::array<std::uint16_t,
                          static_cast<std::size_t>(provider_type::unknown)>
      PROVIDER_API_PORTS = {
          9980U,
          0U,
          0U,
          0U,
      };
  return PROVIDER_API_PORTS.at(static_cast<std::size_t>(prov));
}

auto app_config::default_data_directory(const provider_type &prov)
    -> std::string {
#if defined(_WIN32)
  auto data_directory =
      utils::path::combine(utils::get_local_app_data_directory(),
                           {
                               REPERTORY_DATA_NAME,
                               app_config::get_provider_name(prov),
                           });
#else // !defined(_WIN32)
#if defined(__APPLE__)
  auto data_directory =
      utils::path::combine("~", {
                                    "Library",
                                    "Application Support",
                                    REPERTORY_DATA_NAME,
                                    app_config::get_provider_name(prov),
                                });
#else  // !defined(__APPLE__)
  auto data_directory =
      utils::path::combine("~", {
                                    ".local",
                                    REPERTORY_DATA_NAME,
                                    app_config::get_provider_name(prov),
                                });
#endif // defined(__APPLE__)
#endif // defined(_WIN32)
  return data_directory;
}

auto app_config::default_remote_api_port(const provider_type &prov)
    -> std::uint16_t {
  static const std::array<std::uint16_t,
                          static_cast<std::size_t>(provider_type::unknown)>
      PROVIDER_REMOTE_PORTS = {
          20000U,
          20010U,
          20100U,
          20001U,
      };
  return PROVIDER_REMOTE_PORTS.at(static_cast<std::size_t>(prov));
}
auto app_config::default_rpc_port(const provider_type &prov) -> std::uint16_t {
  static const std::array<std::uint16_t,
                          static_cast<std::size_t>(provider_type::unknown)>
      PROVIDER_RPC_PORTS = {
          10000U,
          10010U,
          10100U,
          10002U,
      };
  return PROVIDER_RPC_PORTS.at(static_cast<std::size_t>(prov));
}

auto app_config::get_api_auth() const -> std::string { return api_auth_; }

auto app_config::get_api_port() const -> std::uint16_t { return api_port_; }

auto app_config::get_api_user() const -> std::string { return api_user_; }

auto app_config::get_cache_directory() const -> std::string {
  return cache_directory_;
}

auto app_config::get_config_file_path() const -> std::string {
  return utils::path::combine(data_directory_, {"config.json"});
}

auto app_config::get_database_type() const -> database_type { return db_type_; }

auto app_config::get_data_directory() const -> std::string {
  return data_directory_;
}

auto app_config::get_download_timeout_secs() const -> std::uint8_t {
  return std::max(min_download_timeout_secs, download_timeout_secs_.load());
}

auto app_config::get_enable_download_timeout() const -> bool {
  return enable_download_timeout_;
}

auto app_config::get_enable_drive_events() const -> bool {
  return enable_drive_events_;
}

auto app_config::get_encrypt_config() const -> encrypt_config {
  return encrypt_config_;
}

#if defined(_WIN32)
auto app_config::get_enable_mount_manager() const -> bool {
  return enable_mount_manager_;
}
#endif // defined(_WIN32)

auto app_config::get_event_level() const -> event_level { return event_level_; }

auto app_config::get_eviction_delay_mins() const -> std::uint32_t {
  return eviction_delay_mins_;
}

auto app_config::get_eviction_uses_accessed_time() const -> bool {
  return eviction_uses_accessed_time_;
}

auto app_config::get_high_frequency_interval_secs() const -> std::uint16_t {
  return std::max(static_cast<std::uint16_t>(1U),
                  high_freq_interval_secs_.load());
}

auto app_config::get_host_config() const -> host_config { return host_config_; }

auto app_config::get_json() const -> json {
  json ret = {
      {JSON_API_AUTH, api_auth_},
      {JSON_API_PORT, api_port_},
      {JSON_API_USER, api_user_},
      {JSON_DOWNLOAD_TIMEOUT_SECS, download_timeout_secs_},
      {JSON_DATABASE_TYPE, db_type_},
      {JSON_ENABLE_DOWNLOAD_TIMEOUT, enable_download_timeout_},
      {JSON_ENABLE_DRIVE_EVENTS, enable_drive_events_},
#if defined(_WIN32)
      {JSON_ENABLE_MOUNT_MANAGER, enable_mount_manager_},
#endif // defined(_WIN32)
      {JSON_ENCRYPT_CONFIG, encrypt_config_},
      {JSON_EVENT_LEVEL, event_level_},
      {JSON_EVICTION_DELAY_MINS, eviction_delay_mins_},
      {JSON_EVICTION_USE_ACCESS_TIME, eviction_uses_accessed_time_},
      {JSON_HIGH_FREQ_INTERVAL_SECS, high_freq_interval_secs_},
      {JSON_HOST_CONFIG, host_config_},
      {JSON_LOW_FREQ_INTERVAL_SECS, low_freq_interval_secs_},
      {JSON_MAX_CACHE_SIZE_BYTES, max_cache_size_bytes_},
      {JSON_MAX_UPLOAD_COUNT, max_upload_count_},
      {JSON_MED_FREQ_INTERVAL_SECS, med_freq_interval_secs_},
      {JSON_ONLINE_CHECK_RETRY_SECS, online_check_retry_secs_},
      {JSON_PREFERRED_DOWNLOAD_TYPE, preferred_download_type_},
      {JSON_REMOTE_CONFIG, remote_config_},
      {JSON_REMOTE_MOUNT, remote_mount_},
      {JSON_RETRY_READ_COUNT, retry_read_count_},
      {JSON_RING_BUFFER_FILE_SIZE, ring_buffer_file_size_},
      {JSON_S3_CONFIG, s3_config_},
      {JSON_SIA_CONFIG, sia_config_},
      {JSON_TASK_WAIT_MS, task_wait_ms_},
      {JSON_VERSION, version_},
  };

  switch (prov_) {
  case provider_type::encrypt: {
    ret.erase(JSON_DOWNLOAD_TIMEOUT_SECS);
    ret.erase(JSON_ENABLE_DOWNLOAD_TIMEOUT);
    ret.erase(JSON_EVICTION_DELAY_MINS);
    ret.erase(JSON_EVICTION_USE_ACCESS_TIME);
    ret.erase(JSON_HOST_CONFIG);
    ret.erase(JSON_MAX_CACHE_SIZE_BYTES);
    ret.erase(JSON_MAX_UPLOAD_COUNT);
    ret.erase(JSON_ONLINE_CHECK_RETRY_SECS);
    ret.erase(JSON_PREFERRED_DOWNLOAD_TYPE);
    ret.erase(JSON_REMOTE_CONFIG);
    ret.erase(JSON_RETRY_READ_COUNT);
    ret.erase(JSON_RING_BUFFER_FILE_SIZE);
    ret.erase(JSON_S3_CONFIG);
    ret.erase(JSON_SIA_CONFIG);
  } break;
  case provider_type::remote: {
    ret.erase(JSON_DATABASE_TYPE);
    ret.erase(JSON_DOWNLOAD_TIMEOUT_SECS);
    ret.erase(JSON_ENABLE_DOWNLOAD_TIMEOUT);
    ret.erase(JSON_ENCRYPT_CONFIG);
    ret.erase(JSON_EVICTION_DELAY_MINS);
    ret.erase(JSON_EVICTION_USE_ACCESS_TIME);
    ret.erase(JSON_HIGH_FREQ_INTERVAL_SECS);
    ret.erase(JSON_HOST_CONFIG);
    ret.erase(JSON_LOW_FREQ_INTERVAL_SECS);
    ret.erase(JSON_MAX_CACHE_SIZE_BYTES);
    ret.erase(JSON_MAX_UPLOAD_COUNT);
    ret.erase(JSON_MED_FREQ_INTERVAL_SECS);
    ret.erase(JSON_ONLINE_CHECK_RETRY_SECS);
    ret.erase(JSON_PREFERRED_DOWNLOAD_TYPE);
    ret.erase(JSON_REMOTE_MOUNT);
    ret.erase(JSON_RETRY_READ_COUNT);
    ret.erase(JSON_RING_BUFFER_FILE_SIZE);
    ret.erase(JSON_S3_CONFIG);
    ret.erase(JSON_SIA_CONFIG);
  } break;
  case provider_type::s3: {
    ret.erase(JSON_ENCRYPT_CONFIG);
    ret.erase(JSON_HOST_CONFIG);
    ret.erase(JSON_REMOTE_CONFIG);
    ret.erase(JSON_SIA_CONFIG);
  } break;
  case provider_type::sia: {
    ret.erase(JSON_ENCRYPT_CONFIG);
    ret.erase(JSON_REMOTE_CONFIG);
    ret.erase(JSON_S3_CONFIG);
  } break;
  default:
    throw std::runtime_error(
        fmt::format("unsupported provider type|{}", get_provider_name(prov_)));
  }

  return ret;
}

auto app_config::get_log_directory() const -> std::string {
  return log_directory_;
}

auto app_config::get_low_frequency_interval_secs() const -> std::uint16_t {
  return std::max(static_cast<std::uint16_t>(1U),
                  low_freq_interval_secs_.load());
}

auto app_config::get_max_cache_size_bytes() const -> std::uint64_t {
  auto max_space = std::max(min_cache_size_bytes, max_cache_size_bytes_.load());
  auto free_space = utils::file::get_free_drive_space(get_cache_directory());
  return free_space.has_value() ? std::min(free_space.value(), max_space)
                                : max_space;
}

auto app_config::get_max_upload_count() const -> std::uint8_t {
  return std::max(std::uint8_t(1U), max_upload_count_.load());
}

auto app_config::get_med_frequency_interval_secs() const -> std::uint16_t {
  return std::max(static_cast<std::uint16_t>(1U),
                  med_freq_interval_secs_.load());
}

auto app_config::get_online_check_retry_secs() const -> std::uint16_t {
  return std::max(min_online_check_retry_secs, online_check_retry_secs_.load());
}

auto app_config::get_preferred_download_type() const -> download_type {
  return preferred_download_type_;
}

auto app_config::get_provider_display_name(const provider_type &prov)
    -> std::string {
  static const std::array<std::string,
                          static_cast<std::size_t>(provider_type::unknown)>
      PROVIDER_DISPLAY_NAMES = {
          "Sia",
          "Remote",
          "S3",
          "Encrypt",
      };
  return PROVIDER_DISPLAY_NAMES.at(static_cast<std::size_t>(prov));
}

auto app_config::get_provider_name(const provider_type &prov) -> std::string {
  static const std::array<std::string,
                          static_cast<std::size_t>(provider_type::unknown)>
      PROVIDER_NAMES = {
          "sia",
          "remote",
          "s3",
          "encrypt",
      };
  return PROVIDER_NAMES.at(static_cast<std::size_t>(prov));
}

auto app_config::get_provider_type() const -> provider_type { return prov_; }

auto app_config::get_remote_config() const -> remote::remote_config {
  return remote_config_;
}

auto app_config::get_remote_mount() const -> remote::remote_mount {
  return remote_mount_;
}

auto app_config::get_retry_read_count() const -> std::uint16_t {
  return std::max(min_retry_read_count, retry_read_count_.load());
}

auto app_config::get_ring_buffer_file_size() const -> std::uint16_t {
  return std::max(
      min_ring_buffer_file_size,
      std::min(max_ring_buffer_file_size, ring_buffer_file_size_.load()));
}

auto app_config::get_s3_config() const -> s3_config { return s3_config_; }

auto app_config::get_sia_config() const -> sia_config { return sia_config_; }

auto app_config::get_task_wait_ms() const -> std::uint16_t {
  return std::max(min_task_wait_ms, task_wait_ms_.load());
}

auto app_config::get_value_by_name(const std::string &name) const
    -> std::string {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    return value_get_lookup_.at(name)();
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e,
                              fmt::format("value not found|name|{}", name));
  }

  return "";
}

auto app_config::load() -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  auto config_file_path = get_config_file_path();

  if (not utils::file::file(config_file_path).exists()) {
    config_changed_ = true;
    return false;
  }

  try {
    recur_mutex_lock lock(read_write_mutex_);

    std::ifstream config_file(config_file_path.c_str());
    if (not config_file.is_open()) {
      config_changed_ = true;
      return false;
    }

    std::stringstream stream;
    stream << config_file.rdbuf();
    auto json_text = stream.str();
    config_file.close();

    if (json_text.empty()) {
      config_changed_ = true;
      return false;
    }

    auto found{true};
    auto json_document = json::parse(json_text);

    get_value(json_document, JSON_API_AUTH, api_auth_, found);
    get_value(json_document, JSON_API_PORT, api_port_, found);
    get_value(json_document, JSON_API_USER, api_user_, found);
    get_value(json_document, JSON_DATABASE_TYPE, db_type_, found);
    get_value(json_document, JSON_DOWNLOAD_TIMEOUT_SECS, download_timeout_secs_,
              found);
    get_value(json_document, JSON_ENABLE_DOWNLOAD_TIMEOUT,
              enable_download_timeout_, found);
    get_value(json_document, JSON_ENABLE_DRIVE_EVENTS, enable_drive_events_,
              found);
#if defined(_WIN32)
    get_value(json_document, JSON_ENABLE_MOUNT_MANAGER, enable_mount_manager_,
              found);
#endif // defined(_WIN32)
    get_value(json_document, JSON_ENCRYPT_CONFIG, encrypt_config_, found);
    get_value(json_document, JSON_EVENT_LEVEL, event_level_, found);
    get_value(json_document, JSON_EVICTION_DELAY_MINS, eviction_delay_mins_,
              found);
    get_value(json_document, JSON_EVICTION_USE_ACCESS_TIME,
              eviction_uses_accessed_time_, found);
    get_value(json_document, JSON_HIGH_FREQ_INTERVAL_SECS,
              high_freq_interval_secs_, found);
    get_value(json_document, JSON_HOST_CONFIG, host_config_, found);
    get_value(json_document, JSON_LOW_FREQ_INTERVAL_SECS,
              low_freq_interval_secs_, found);
    get_value(json_document, JSON_MAX_CACHE_SIZE_BYTES, max_cache_size_bytes_,
              found);
    get_value(json_document, JSON_MAX_UPLOAD_COUNT, max_upload_count_, found);
    get_value(json_document, JSON_MED_FREQ_INTERVAL_SECS,
              med_freq_interval_secs_, found);
    get_value(json_document, JSON_ONLINE_CHECK_RETRY_SECS,
              online_check_retry_secs_, found);
    get_value(json_document, JSON_PREFERRED_DOWNLOAD_TYPE,
              preferred_download_type_, found);
    get_value(json_document, JSON_REMOTE_CONFIG, remote_config_, found);
    get_value(json_document, JSON_REMOTE_MOUNT, remote_mount_, found);
    get_value(json_document, JSON_RETRY_READ_COUNT, retry_read_count_, found);
    get_value(json_document, JSON_RING_BUFFER_FILE_SIZE, ring_buffer_file_size_,
              found);
    get_value(json_document, JSON_S3_CONFIG, s3_config_, found);
    get_value(json_document, JSON_SIA_CONFIG, sia_config_, found);
    get_value(json_document, JSON_TASK_WAIT_MS, task_wait_ms_, found);

    std::uint64_t version{};
    get_value(json_document, JSON_VERSION, version, found);

    if (version != REPERTORY_CONFIG_VERSION) {
      version_ = REPERTORY_CONFIG_VERSION;
      if (version_ == 1U) {
        if (low_freq_interval_secs_ == 0UL) {
          set_value(low_freq_interval_secs_, default_low_freq_interval_secs);
        }

        if (max_cache_size_bytes_ == 0UL) {
          set_value(max_cache_size_bytes_, default_max_cache_size_bytes);
        }
      }
      found = false;
    }

    config_changed_ = not found;
    return found;
  } catch (const std::exception &ex) {
    utils::error::raise_error(
        function_name, ex,
        fmt::format("failed to load configuration file|sp|{}",
                    config_file_path));
  }

  config_changed_ = true;
  return false;
}

void app_config::save() {
  REPERTORY_USES_FUNCTION_NAME();

  recur_mutex_lock lock(read_write_mutex_);

  auto file_path = get_config_file_path();
  if (not config_changed_ && utils::file::file(file_path).exists()) {
    return;
  }

  if (not utils::file::directory{data_directory_}.create_directory()) {
    utils::error::raise_error(
        function_name,
        fmt::format("failed to create directory|sp|{}|err|{}", data_directory_,
                    utils::get_last_error_code()));
  }

  config_changed_ = not utils::retry_action([this, &file_path]() -> bool {
    return utils::file::write_json_file(file_path, get_json());
  });
}

void app_config::set_api_auth(const std::string &value) {
  set_value(api_auth_, value);
}

void app_config::set_api_port(std::uint16_t value) {
  set_value(api_port_, value);
}

void app_config::set_api_user(const std::string &value) {
  set_value(api_user_, value);
}

void app_config::set_download_timeout_secs(std::uint8_t value) {
  set_value(download_timeout_secs_, value);
}

void app_config::set_database_type(const database_type &value) {
  set_value(db_type_, value);
}

void app_config::set_enable_download_timeout(bool value) {
  set_value(enable_download_timeout_, value);
}

void app_config::set_enable_drive_events(bool value) {
  set_value(enable_drive_events_, value);
}

#if defined(_WIN32)
void app_config::set_enable_mount_manager(bool value) {
  set_value(enable_mount_manager_, value);
}
#endif // defined(_WIN32)

void app_config::set_event_level(const event_level &value) {
  REPERTORY_USES_FUNCTION_NAME();

  if (set_value(event_level_, value)) {
    event_system::instance().raise<event_level_changed>(function_name, value);
  }
}

void app_config::set_encrypt_config(encrypt_config value) {
  set_value(encrypt_config_, value);
}

void app_config::set_eviction_delay_mins(std::uint32_t value) {
  set_value(eviction_delay_mins_, value);
}

void app_config::set_eviction_uses_accessed_time(bool value) {
  set_value(eviction_uses_accessed_time_, value);
}

void app_config::set_high_frequency_interval_secs(std::uint16_t value) {
  set_value(high_freq_interval_secs_, value);
}

void app_config::set_host_config(host_config value) {
  set_value(host_config_, value);
}

void app_config::set_low_frequency_interval_secs(std::uint16_t value) {
  set_value(low_freq_interval_secs_, value);
}

void app_config::set_max_cache_size_bytes(std::uint64_t value) {
  REPERTORY_USES_FUNCTION_NAME();

  set_value(max_cache_size_bytes_, value);
  auto res = cache_size_mgr::instance().shrink(0U);
  if (res != api_error::success) {
    utils::error::raise_error(function_name, res, "failed to shrink cache");
  }
}

void app_config::set_max_upload_count(std::uint8_t value) {
  set_value(max_upload_count_, value);
}

void app_config::set_med_frequency_interval_secs(std::uint16_t value) {
  set_value(med_freq_interval_secs_, value);
}

void app_config::set_online_check_retry_secs(std::uint16_t value) {
  set_value(online_check_retry_secs_, value);
}

void app_config::set_preferred_download_type(const download_type &value) {
  set_value(preferred_download_type_, value);
}

void app_config::set_remote_config(remote::remote_config value) {
  set_value(remote_config_, value);
}

void app_config::set_remote_mount(remote::remote_mount value) {
  set_value(remote_mount_, value);
}

void app_config::set_retry_read_count(std::uint16_t value) {
  set_value(retry_read_count_, value);
}

void app_config::set_ring_buffer_file_size(std::uint16_t value) {
  set_value(ring_buffer_file_size_, value);
}

void app_config::set_s3_config(s3_config value) {
  set_value(s3_config_, value);
}

void app_config::set_sia_config(sia_config value) {
  set_value(sia_config_, value);
}

void app_config::set_task_wait_ms(std::uint16_t value) {
  set_value(task_wait_ms_, value);
}

template <typename dest, typename source>
auto app_config::set_value(dest &dst, const source &src) -> bool {
  if (dst.load() == src) {
    return false;
  }

  dst = src;
  config_changed_ = true;
  save();

  return true;
}

auto app_config::set_value_by_name(const std::string &name,
                                   const std::string &value) -> std::string {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    return value_set_lookup_.at(name)(value);
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e,
                              fmt::format("value not found|name|{}", name));
  }

  return "";
}

auto app_config::get_version() const -> std::uint64_t { return version_; }
} // namespace repertory
