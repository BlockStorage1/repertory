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
#include "app_config.hpp"

#include "types/startup_exception.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/unix/unix_utils.hpp"
#include "utils/utils.hpp"

namespace {
constexpr const auto default_api_auth_size = 48U;
constexpr const auto default_download_timeout_ces = 30U;
constexpr const auto default_eviction_delay_mins = 10U;
constexpr const auto default_high_freq_interval_secs = 30U;
constexpr const auto default_low_freq_interval_secs = 60U * 60U;
constexpr const auto default_max_cache_size_bytes =
    20ULL * 1024ULL * 1024ULL * 1024ULL;
constexpr const auto default_max_upload_count = 5U;
constexpr const auto default_min_download_timeout_secs = 5U;
constexpr const auto default_online_check_retry_secs = 60U;
constexpr const auto default_orphaned_file_retention_days = 15U;
constexpr const auto default_read_ahead_count = 4U;
constexpr const auto default_remote_client_pool_size = 10U;
constexpr const auto default_remote_host_name_or_ip = "";
constexpr const auto default_remote_max_connections = 20U;
constexpr const auto default_remote_receive_timeout_secs = 120U;
constexpr const auto default_remote_send_timeout_secs = 30U;
constexpr const auto default_remote_token = "";
constexpr const auto default_retry_read_count = 6U;
constexpr const auto default_ring_buffer_file_size = 512U;
constexpr const auto retry_save_count = 5U;
} // namespace

namespace repertory {
app_config::app_config(const provider_type &prov,
                       const std::string &data_directory)
    : prov_(prov),
      api_auth_(utils::generate_random_string(default_api_auth_size)),
      api_port_(default_rpc_port(prov)),
      api_user_(REPERTORY),
      config_changed_(false),
      data_directory_(data_directory.empty()
                          ? default_data_directory(prov)
                          : utils::path::absolute(data_directory)),
      download_timeout_secs_(default_download_timeout_ces),
      enable_chunk_downloader_timeout_(true),
      enable_comm_duration_events_(false),
      enable_drive_events_(false),
      enable_max_cache_size_(false),
#ifdef _WIN32
      enable_mount_manager_(false),
#endif
      enable_remote_mount_(false),
      event_level_(event_level::normal),
      eviction_delay_mins_(default_eviction_delay_mins),
      eviction_uses_accessed_time_(false),
      high_freq_interval_secs_(default_high_freq_interval_secs),
      is_remote_mount_(false),
      low_freq_interval_secs_(default_low_freq_interval_secs),
      max_cache_size_bytes_(default_max_cache_size_bytes),
      max_upload_count_(default_max_upload_count),
      min_download_timeout_secs_(default_min_download_timeout_secs),
      online_check_retry_secs_(default_online_check_retry_secs),
      orphaned_file_retention_days_(default_orphaned_file_retention_days),
      preferred_download_type_(
          utils::download_type_to_string(download_type::fallback)),
      read_ahead_count_(default_read_ahead_count),
      remote_client_pool_size_(default_remote_client_pool_size),
      remote_host_name_or_ip_(default_remote_host_name_or_ip),
      remote_max_connections_(default_remote_max_connections),
      remote_port_(default_remote_port(prov)),
      remote_receive_timeout_secs_(default_remote_receive_timeout_secs),
      remote_send_timeout_secs_(default_remote_send_timeout_secs),
      remote_token_(default_remote_token),
      retry_read_count_(default_retry_read_count),
      ring_buffer_file_size_(default_ring_buffer_file_size) {
  cache_directory_ = utils::path::combine(data_directory_, {"cache"});
  log_directory_ = utils::path::combine(data_directory_, {"logs"});

  hc_.agent_string = default_agent_name(prov_);
  hc_.api_password = get_provider_api_password(prov_);
  hc_.api_port = default_api_port(prov_);

  if (not utils::file::create_full_directory_path(data_directory_)) {
    throw startup_exception("unable to create: " + data_directory_);
  }

  if (not utils::file::create_full_directory_path(cache_directory_)) {
    throw startup_exception("unable to create: " + cache_directory_);
  }

  if (not utils::file::create_full_directory_path(log_directory_)) {
    throw startup_exception("unable to create: " + log_directory_);
  }

  if (not load()) {
    save();
  }
}

auto app_config::get_config_file_path() const -> std::string {
  return utils::path::combine(data_directory_, {"config.json"});
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
#ifdef _WIN32
  auto data_directory = utils::path::combine(
      utils::get_local_app_data_directory(),
      {REPERTORY_DATA_NAME, app_config::get_provider_name(prov)});
#else
#ifdef __APPLE__
  auto data_directory = utils::path::resolve(
      std::string("~/Library/Application Support/") + REPERTORY_DATA_NAME +
      '/' + app_config::get_provider_name(prov));
#else
  auto data_directory =
      utils::path::resolve(std::string("~/.local/") + REPERTORY_DATA_NAME +
                           '/' + app_config::get_provider_name(prov));
#endif
#endif
  return data_directory;
}

auto app_config::default_remote_port(const provider_type &prov)
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

auto app_config::get_json() const -> json {
  json ret = {
      {"ApiAuth", api_auth_},
      {"ApiPort", api_port_},
      {"ApiUser", api_user_},
      {"ChunkDownloaderTimeoutSeconds", download_timeout_secs_},
      {"EnableChunkDownloaderTimeout", enable_chunk_downloader_timeout_},
      {"EnableCommDurationEvents", enable_comm_duration_events_},
      {"EnableDriveEvents", enable_drive_events_},
#ifdef _WIN32
      {"EnableMountManager", enable_mount_manager_},
#endif
      {"EnableMaxCacheSize", enable_max_cache_size_},
      {"EncryptConfig",
       {
           {"EncryptionToken", encrypt_config_.encryption_token},
           {"Path", encrypt_config_.path},
       }},
      {"EventLevel", event_level_to_string(event_level_)},
      {"EvictionDelayMinutes", eviction_delay_mins_},
      {"EvictionUsesAccessedTime", eviction_uses_accessed_time_},
      {"HighFreqIntervalSeconds", high_freq_interval_secs_},
      {"HostConfig",
       {
           {"AgentString", hc_.agent_string},
           {"ApiPassword", hc_.api_password},
           {"ApiPort", hc_.api_port},
           {"HostNameOrIp", hc_.host_name_or_ip},
           {"TimeoutMs", hc_.timeout_ms},
       }},
      {"LowFreqIntervalSeconds", low_freq_interval_secs_},
      {"MaxCacheSizeBytes", max_cache_size_bytes_},
      {"MaxUploadCount", max_upload_count_},
      {"OnlineCheckRetrySeconds", online_check_retry_secs_},
      {"OrphanedFileRetentionDays", orphaned_file_retention_days_},
      {"PreferredDownloadType", preferred_download_type_},
      {"ReadAheadCount", read_ahead_count_},
      {
          "RemoteMount",
          {
              {"EnableRemoteMount", enable_remote_mount_},
              {"IsRemoteMount", is_remote_mount_},
              {"RemoteClientPoolSize", remote_client_pool_size_},
              {"RemoteMaxConnections", remote_max_connections_},
              {"RemoteHostNameOrIp", remote_host_name_or_ip_},
              {"RemotePort", remote_port_},
              {"RemoteReceiveTimeoutSeconds", remote_receive_timeout_secs_},
              {"RemoteSendTimeoutSeconds", remote_send_timeout_secs_},
              {"RemoteToken", remote_token_},
          },
      },
      {"RetryReadCount", retry_read_count_},
      {"RingBufferFileSize", ring_buffer_file_size_},
      {"S3Config",
       {
           {"AccessKey", s3_config_.access_key},
           {"Bucket", s3_config_.bucket},
           {"CacheTimeoutSeconds", s3_config_.cache_timeout_secs},
           {"EncryptionToken", s3_config_.encryption_token},
           {"Region", s3_config_.region},
           {"SecretKey", s3_config_.secret_key},
           {"TimeoutMs", s3_config_.timeout_ms},
           {"URL", s3_config_.url},
           {"UsePathStyle", s3_config_.use_path_style},
           {"UseRegionInURL", s3_config_.use_region_in_url},
       }},
      {"Version", version_}};

  if (prov_ == provider_type::encrypt) {
    ret.erase("ChunkDownloaderTimeoutSeconds");
    ret.erase("EnableChunkDownloaderTimeout");
    ret.erase("EnableMaxCacheSize");
    ret.erase("EvictionDelayMinutes");
    ret.erase("EvictionUsesAccessedTime");
    ret.erase("HostConfig");
    ret.erase("MaxCacheSizeBytes");
    ret.erase("MaxUploadCount");
    ret.erase("OnlineCheckRetrySeconds");
    ret.erase("OrphanedFileRetentionDays");
    ret.erase("PreferredDownloadType");
    ret.erase("ReadAheadCount");
    ret.erase("RetryReadCount");
    ret.erase("RingBufferFileSize");
    ret.erase("S3Config");
  } else if (prov_ == provider_type::s3) {
    ret.erase("EncryptConfig");
    ret.erase("HostConfig");
  } else if (prov_ == provider_type::sia) {
    ret.erase("EncryptConfig");
    ret.erase("S3Config");
  } else if (prov_ == provider_type::remote) {
    ret.erase("ChunkDownloaderTimeoutSeconds");
    ret.erase("EnableChunkDownloaderTimeout");
    ret.erase("EnableChunkDownloaderTimeout");
    ret.erase("EnableMaxCacheSize");
    ret.erase("EncryptConfig");
    ret.erase("EvictionDelayMinutes");
    ret.erase("HighFreqIntervalSeconds");
    ret.erase("HostConfig");
    ret.erase("LowFreqIntervalSeconds");
    ret.erase("MaxCacheSizeBytes");
    ret.erase("MaxUploadCount");
    ret.erase("OnlineCheckRetrySeconds");
    ret.erase("OrphanedFileRetentionDays");
    ret.erase("PreferredDownloadType");
    ret.erase("ReadAheadCount");
    ret.erase("RetryReadCount");
    ret.erase("RingBufferFileSize");
    ret.erase("S3Config");
  }

  return ret;
}

auto app_config::get_max_cache_size_bytes() const -> std::uint64_t {
  const auto max_space =
      std::max(static_cast<std::uint64_t>(100ULL * 1024ULL * 1024ULL),
               max_cache_size_bytes_);
  return std::min(utils::file::get_free_drive_space(get_cache_directory()),
                  max_space);
}

auto app_config::get_provider_api_password(const provider_type &prov)
    -> std::string {
#ifdef _WIN32
  auto api_file =
      utils::path::combine(utils::get_local_app_data_directory(),
                           {get_provider_display_name(prov), "apipassword"});
#else
#ifdef __APPLE__
  auto api_file =
      utils::path::combine(utils::path::resolve("~"),
                           {"/Library/Application Support",
                            get_provider_display_name(prov), "apipassword"});
#else
  auto api_file = utils::path::combine(
      utils::path::resolve("~/."), {get_provider_name(prov), "apipassword"});
#endif
#endif
  auto lines = utils::file::read_file_lines(api_file);
  return lines.empty() ? "" : utils::string::trim(lines[0U]);
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

auto app_config::get_value_by_name(const std::string &name) -> std::string {
  try {
    if (name == "ApiAuth") {
      return api_auth_;
    }
    if (name == "ApiPort") {
      return std::to_string(get_api_port());
    }
    if (name == "ApiUser") {
      return api_user_;
    }
    if (name == "ChunkDownloaderTimeoutSeconds") {
      return std::to_string(get_chunk_downloader_timeout_secs());
    }
    if (name == "EnableChunkDownloaderTimeout") {
      return utils::string::from_bool(get_enable_chunk_download_timeout());
    }
    if (name == "GetEnableCommDurationEvents") {
      return utils::string::from_bool(get_enable_comm_duration_events());
    }
    if (name == "EnableDriveEvents") {
      return utils::string::from_bool(get_enable_drive_events());
    }
    if (name == "EnableMaxCacheSize") {
      return utils::string::from_bool(get_enable_max_cache_size());
#ifdef _WIN32
    }
    if (name == "EnableMountManager") {
      return std::to_string(get_enable_mount_manager());
#endif
    }
    if (name == "EncryptConfig.Path") {
      return utils::path::absolute(encrypt_config_.path);
    }
    if (name == "EncryptConfig.EncryptionToken") {
      return encrypt_config_.encryption_token;
    }
    if (name == "EventLevel") {
      return event_level_to_string(get_event_level());
    }
    if (name == "EvictionDelayMinutes") {
      return std::to_string(get_eviction_delay_mins());
    }
    if (name == "EvictionUsesAccessedTime") {
      return utils::string::from_bool(get_eviction_uses_accessed_time());
    }
    if (name == "HighFreqIntervalSeconds") {
      return std::to_string(get_high_frequency_interval_secs());
    }
    if (name == "HostConfig.AgentString") {
      return hc_.agent_string;
    }
    if (name == "HostConfig.ApiPassword") {
      return hc_.api_password;
    }
    if (name == "HostConfig.ApiPort") {
      return std::to_string(hc_.api_port);
    }
    if (name == "HostConfig.HostNameOrIp") {
      return hc_.host_name_or_ip;
    }
    if (name == "HostConfig.TimeoutMs") {
      return std::to_string(hc_.timeout_ms);
    }
    if (name == "LowFreqIntervalSeconds") {
      return std::to_string(get_low_frequency_interval_secs());
    }
    if (name == "MaxCacheSizeBytes") {
      return std::to_string(get_max_cache_size_bytes());
    }
    if (name == "MaxUploadCount") {
      return std::to_string(get_max_upload_count());
    }
    if (name == "OnlineCheckRetrySeconds") {
      return std::to_string(get_online_check_retry_secs());
    }
    if (name == "OrphanedFileRetentionDays") {
      return std::to_string(get_orphaned_file_retention_days());
    }
    if (name == "PreferredDownloadType") {
      return utils::download_type_to_string(utils::download_type_from_string(
          preferred_download_type_, download_type::fallback));
    }
    if (name == "ReadAheadCount") {
      return std::to_string(get_read_ahead_count());
    }
    if (name == "RemoteMount.EnableRemoteMount") {
      return utils::string::from_bool(get_enable_remote_mount());
    }
    if (name == "RemoteMount.IsRemoteMount") {
      return utils::string::from_bool(get_is_remote_mount());
    }
    if (name == "RemoteMount.RemoteClientPoolSize") {
      return std::to_string(get_remote_client_pool_size());
    }
    if (name == "RemoteMount.RemoteHostNameOrIp") {
      return get_remote_host_name_or_ip();
    }
    if (name == "RemoteMount.RemoteMaxConnections") {
      return std::to_string(get_remote_max_connections());
    }
    if (name == "RemoteMount.RemotePort") {
      return std::to_string(get_remote_port());
    }
    if (name == "RemoteMount.RemoteReceiveTimeoutSeconds") {
      return std::to_string(get_remote_receive_timeout_secs());
    }
    if (name == "RemoteMount.RemoteSendTimeoutSeconds") {
      return std::to_string(get_remote_send_timeout_secs());
    }
    if (name == "RemoteMount.RemoteToken") {
      return get_remote_token();
    }
    if (name == "RetryReadCount") {
      return std::to_string(get_retry_read_count());
    }
    if (name == "RingBufferFileSize") {
      return std::to_string(get_ring_buffer_file_size());
    }
    if (name == "S3Config.AccessKey") {
      return s3_config_.access_key;
    }
    if (name == "S3Config.Bucket") {
      return s3_config_.bucket;
    }
    if (name == "S3Config.EncryptionToken") {
      return s3_config_.encryption_token;
    }
    if (name == "S3Config.CacheTimeoutSeconds") {
      return std::to_string(s3_config_.cache_timeout_secs);
    }
    if (name == "S3Config.Region") {
      return s3_config_.region;
    }
    if (name == "S3Config.SecretKey") {
      return s3_config_.secret_key;
    }
    if (name == "S3Config.URL") {
      return s3_config_.url;
    }
    if (name == "S3Config.UsePathStyle") {
      return utils::string::from_bool(s3_config_.use_path_style);
    }
    if (name == "S3Config.UseRegionInURL") {
      return utils::string::from_bool(s3_config_.use_region_in_url);
    }
    if (name == "S3Config.TimeoutMs") {
      return std::to_string(s3_config_.timeout_ms);
    }
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }
  return "";
}

auto app_config::load() -> bool {
  auto ret = false;

  const auto config_file_path = get_config_file_path();
  recur_mutex_lock lock(read_write_mutex_);
  if (utils::file::is_file(config_file_path)) {
    try {
      std::ifstream config_file(config_file_path.data());
      if (config_file.is_open()) {
        std::stringstream stream;
        stream << config_file.rdbuf();
        const auto json_text = stream.str();
        config_file.close();
        ret = not json_text.empty();
        if (ret) {
          const auto json_document = json::parse(json_text);

          get_value(json_document, "ApiAuth", api_auth_, ret);
          get_value(json_document, "ApiPort", api_port_, ret);
          get_value(json_document, "ApiUser", api_user_, ret);
          get_value(json_document, "ChunkDownloaderTimeoutSeconds",
                    download_timeout_secs_, ret);
          get_value(json_document, "EvictionDelayMinutes", eviction_delay_mins_,
                    ret);
          get_value(json_document, "EvictionUsesAccessedTime",
                    eviction_uses_accessed_time_, ret);
          get_value(json_document, "EnableChunkDownloaderTimeout",
                    enable_chunk_downloader_timeout_, ret);
          get_value(json_document, "EnableCommDurationEvents",
                    enable_comm_duration_events_, ret);
          get_value(json_document, "EnableDriveEvents", enable_drive_events_,
                    ret);

          if (json_document.find("EncryptConfig") != json_document.end()) {
            auto encrypt_config_json = json_document["EncryptConfig"];
            auto encrypt = encrypt_config_;
            get_value(encrypt_config_json, "Path", encrypt.path, ret);
            encrypt_config_.path = utils::path::absolute(encrypt_config_.path);
            get_value(encrypt_config_json, "EncryptionToken",
                      encrypt.encryption_token, ret);
            encrypt_config_ = encrypt;
          } else {
            ret = false;
          }

          std::string level;
          if (get_value(json_document, "EventLevel", level, ret)) {
            event_level_ = event_level_from_string(level);
          }

          if (json_document.find("HostConfig") != json_document.end()) {
            auto host_config_json = json_document["HostConfig"];
            auto cfg = hc_;
            get_value(host_config_json, "AgentString", cfg.agent_string, ret);
            get_value(host_config_json, "ApiPassword", cfg.api_password, ret);
            get_value(host_config_json, "ApiPort", cfg.api_port, ret);
            get_value(host_config_json, "HostNameOrIp", cfg.host_name_or_ip,
                      ret);
            get_value(host_config_json, "TimeoutMs", cfg.timeout_ms, ret);
            hc_ = cfg;
          } else {
            ret = false;
          }

          if (hc_.api_password.empty()) {
            hc_.api_password = get_provider_api_password(prov_);
            if (hc_.api_password.empty()) {
              ret = false;
            }
          }

          if (json_document.find("S3Config") != json_document.end()) {
            auto s3_config_json = json_document["S3Config"];
            auto s3_cfg = s3_config_;
            get_value(s3_config_json, "AccessKey", s3_cfg.access_key, ret);
            get_value(s3_config_json, "Bucket", s3_cfg.bucket, ret);
            get_value(s3_config_json, "CacheTimeoutSeconds",
                      s3_cfg.cache_timeout_secs, ret);
            get_value(s3_config_json, "EncryptionToken",
                      s3_cfg.encryption_token, ret);
            get_value(s3_config_json, "Region", s3_cfg.region, ret);
            get_value(s3_config_json, "SecretKey", s3_cfg.secret_key, ret);
            get_value(s3_config_json, "TimeoutMs", s3_cfg.timeout_ms, ret);
            get_value(s3_config_json, "URL", s3_cfg.url, ret);
            get_value(s3_config_json, "UsePathStyle", s3_cfg.use_path_style,
                      ret);
            get_value(s3_config_json, "UseRegionInURL",
                      s3_cfg.use_region_in_url, ret);
            s3_config_ = s3_cfg;
          } else {
            ret = false;
          }

          get_value(json_document, "ReadAheadCount", read_ahead_count_, ret);
          get_value(json_document, "RingBufferFileSize", ring_buffer_file_size_,
                    ret);
          get_value(json_document, "EnableMaxCacheSize", enable_max_cache_size_,
                    ret);
#ifdef _WIN32
          get_value(json_document, "EnableMountManager", enable_mount_manager_,
                    ret);
#endif
          get_value(json_document, "MaxCacheSizeBytes", max_cache_size_bytes_,
                    ret);
          get_value(json_document, "MaxUploadCount", max_upload_count_, ret);
          get_value(json_document, "OnlineCheckRetrySeconds",
                    online_check_retry_secs_, ret);
          get_value(json_document, "HighFreqIntervalSeconds",
                    high_freq_interval_secs_, ret);
          get_value(json_document, "LowFreqIntervalSeconds",
                    low_freq_interval_secs_, ret);
          get_value(json_document, "OrphanedFileRetentionDays",
                    orphaned_file_retention_days_, ret);
          get_value(json_document, "PreferredDownloadType",
                    preferred_download_type_, ret);
          get_value(json_document, "RetryReadCount", retry_read_count_, ret);
          if (json_document.find("RemoteMount") != json_document.end()) {
            auto remoteMount = json_document["RemoteMount"];
            get_value(remoteMount, "EnableRemoteMount", enable_remote_mount_,
                      ret);
            get_value(remoteMount, "IsRemoteMount", is_remote_mount_, ret);
            get_value(remoteMount, "RemoteClientPoolSize",
                      remote_client_pool_size_, ret);
            get_value(remoteMount, "RemoteHostNameOrIp",
                      remote_host_name_or_ip_, ret);
            get_value(remoteMount, "RemoteMaxConnections",
                      remote_max_connections_, ret);
            get_value(remoteMount, "RemotePort", remote_port_, ret);
            get_value(remoteMount, "RemoteReceiveTimeoutSeconds",
                      remote_receive_timeout_secs_, ret);
            get_value(remoteMount, "RemoteSendTimeoutSeconds",
                      remote_send_timeout_secs_, ret);
            get_value(remoteMount, "RemoteToken", remote_token_, ret);
          } else {
            ret = false;
          }

          std::uint64_t version{};
          get_value(json_document, "Version", version, ret);

          // Handle configuration defaults for new config versions
          if (version != REPERTORY_CONFIG_VERSION) {
            if (version > REPERTORY_CONFIG_VERSION) {
              version = 0U;
            }

            version_ = version;
            ret = false;
          }
        }
      }

      if (not ret) {
        config_changed_ = true;
      }
    } catch (const std::exception &ex) {
      utils::error::raise_error(__FUNCTION__, ex, "exception occurred");
      ret = false;
    }
  }

  return ret;
}

void app_config::save() {
  const auto file_path = get_config_file_path();
  recur_mutex_lock lock(read_write_mutex_);
  if (config_changed_ || not utils::file::is_file(file_path)) {
    if (not utils::file::is_directory(data_directory_)) {
      if (not utils::file::create_full_directory_path(data_directory_)) {
        utils::error::raise_error(
            __FUNCTION__, "failed to create directory|sp|" + data_directory_ +
                              "|err|" +
                              std::to_string(utils::get_last_error_code()));
      }
    }
    config_changed_ = false;
    json data = get_json();
    auto success = false;
    for (auto i = 0U; not success && (i < retry_save_count); i++) {
      success = utils::file::write_json_file(file_path, data);
      if (not success) {
        std::this_thread::sleep_for(1s);
      }
    }
  }
}

void app_config::set_enable_remote_mount(bool enable_remote_mount) {
  recur_mutex_lock remote_lock(remote_mount_mutex_);
  if (get_is_remote_mount()) {
    set_value(enable_remote_mount_, false);
  } else {
    set_value(enable_remote_mount_, enable_remote_mount);
  }
}

void app_config::set_is_remote_mount(bool is_remote_mount) {
  recur_mutex_lock remote_lock(remote_mount_mutex_);

  if (get_enable_remote_mount()) {
    set_value(is_remote_mount_, false);
    return;
  }

  set_value(is_remote_mount_, is_remote_mount);
}

auto app_config::set_value_by_name(const std::string &name,
                                   const std::string &value) -> std::string {
  try {
    if (name == "ApiAuth") {
      set_api_auth(value);
      return get_api_auth();
    }
    if (name == "ApiPort") {
      set_api_port(utils::string::to_uint16(value));
      return std::to_string(get_api_port());
    }
    if (name == "ApiUser") {
      set_api_user(value);
      return get_api_user();
    }
    if (name == "ChunkDownloaderTimeoutSeconds") {
      set_chunk_downloader_timeout_secs(utils::string::to_uint8(value));
      return std::to_string(get_chunk_downloader_timeout_secs());
    }
    if (name == "EnableChunkDownloaderTimeout") {
      set_enable_chunk_downloader_timeout(utils::string::to_bool(value));
      return utils::string::from_bool(get_enable_chunk_download_timeout());
    }
    if (name == "EnableCommDurationEvents") {
      set_enable_comm_duration_events(utils::string::to_bool(value));
      return utils::string::from_bool(get_enable_comm_duration_events());
    }
    if (name == "EnableDriveEvents") {
      set_enable_drive_events(utils::string::to_bool(value));
      return utils::string::from_bool(get_enable_drive_events());
    }
    if (name == "EnableMaxCacheSize") {
      set_enable_max_cache_size(utils::string::to_bool(value));
      return utils::string::from_bool(get_enable_max_cache_size());
#ifdef _WIN32
    }
    if (name == "EnableMountManager") {
      set_enable_mount_manager(utils::string::to_bool(value));
      return std::to_string(get_enable_mount_manager());
#endif
    }
    if (name == "EncryptConfig.EncryptionToken") {
      set_value(encrypt_config_.encryption_token, value);
      return encrypt_config_.encryption_token;
    }
    if (name == "EncryptConfig.Path") {
      set_value(encrypt_config_.path, utils::path::absolute(value));
      return encrypt_config_.path;
    }
    if (name == "EventLevel") {
      set_event_level(event_level_from_string(value));
      return event_level_to_string(get_event_level());
    }
    if (name == "EvictionDelayMinutes") {
      set_eviction_delay_mins(utils::string::to_uint32(value));
      return std::to_string(get_eviction_delay_mins());
    }
    if (name == "EvictionUsesAccessedTime") {
      set_eviction_uses_accessed_time(utils::string::to_bool(value));
      return utils::string::from_bool(get_eviction_uses_accessed_time());
    }
    if (name == "HighFreqIntervalSeconds") {
      set_high_frequency_interval_secs(utils::string::to_uint8(value));
      return std::to_string(get_high_frequency_interval_secs());
    }
    if (name == "HostConfig.AgentString") {
      set_value(hc_.agent_string, value);
      return hc_.agent_string;
    }
    if (name == "HostConfig.ApiPassword") {
      set_value(hc_.api_password, value);
      return hc_.api_password;
    }
    if (name == "HostConfig.ApiPort") {
      set_value(hc_.api_port, utils::string::to_uint16(value));
      return std::to_string(hc_.api_port);
    }
    if (name == "HostConfig.HostNameOrIp") {
      set_value(hc_.host_name_or_ip, value);
      return hc_.host_name_or_ip;
    }
    if (name == "HostConfig.TimeoutMs") {
      set_value(hc_.timeout_ms, utils::string::to_uint32(value));
      return std::to_string(hc_.timeout_ms);
    }
    if (name == "LowFreqIntervalSeconds") {
      set_low_frequency_interval_secs(utils::string::to_uint32(value));
      return std::to_string(get_low_frequency_interval_secs());
    }
    if (name == "MaxCacheSizeBytes") {
      set_max_cache_size_bytes(utils::string::to_uint64(value));
      return std::to_string(get_max_cache_size_bytes());
    }
    if (name == "MaxUploadCount") {
      set_max_upload_count(utils::string::to_uint8(value));
      return std::to_string(get_max_upload_count());
    }
    if (name == "OnlineCheckRetrySeconds") {
      set_online_check_retry_secs(utils::string::to_uint16(value));
      return std::to_string(get_online_check_retry_secs());
    }
    if (name == "OrphanedFileRetentionDays") {
      set_orphaned_file_retention_days(utils::string::to_uint16(value));
      return std::to_string(get_orphaned_file_retention_days());
    }
    if (name == "PreferredDownloadType") {
      set_preferred_download_type(
          utils::download_type_from_string(value, download_type::fallback));
      return utils::download_type_to_string(get_preferred_download_type());
    }
    if (name == "ReadAheadCount") {
      set_read_ahead_count(utils::string::to_uint8(value));
      return std::to_string(get_read_ahead_count());
    }
    if (name == "RemoteMount.EnableRemoteMount") {
      set_enable_remote_mount(utils::string::to_bool(value));
      return utils::string::from_bool(get_enable_remote_mount());
    }
    if (name == "RemoteMount.IsRemoteMount") {
      set_is_remote_mount(utils::string::to_bool(value));
      return utils::string::from_bool(get_is_remote_mount());
    }
    if (name == "RemoteMount.RemoteClientPoolSize") {
      set_remote_client_pool_size(utils::string::to_uint8(value));
      return std::to_string(get_remote_client_pool_size());
    }
    if (name == "RemoteMount.RemoteHostNameOrIp") {
      set_remote_host_name_or_ip(value);
      return get_remote_host_name_or_ip();
    }
    if (name == "RemoteMount.RemoteMaxConnections") {
      set_remote_max_connections(utils::string::to_uint8(value));
      return std::to_string(get_remote_max_connections());
    }
    if (name == "RemoteMount.RemotePort") {
      set_remote_port(utils::string::to_uint16(value));
      return std::to_string(get_remote_port());
    }
    if (name == "RemoteMount.RemoteReceiveTimeoutSeconds") {
      set_remote_receive_timeout_secs(utils::string::to_uint16(value));
      return std::to_string(get_remote_receive_timeout_secs());
    }
    if (name == "RemoteMount.RemoteSendTimeoutSeconds") {
      set_remote_send_timeout_secs(utils::string::to_uint16(value));
      return std::to_string(get_remote_send_timeout_secs());
    }
    if (name == "RemoteMount.RemoteToken") {
      set_remote_token(value);
      return get_remote_token();
    }
    if (name == "RetryReadCount") {
      set_retry_read_count(utils::string::to_uint16(value));
      return std::to_string(get_retry_read_count());
    }
    if (name == "RingBufferFileSize") {
      set_ring_buffer_file_size(utils::string::to_uint16(value));
      return std::to_string(get_ring_buffer_file_size());
    }
    if (name == "S3Config.AccessKey") {
      set_value(s3_config_.access_key, value);
      return s3_config_.access_key;
    }
    if (name == "S3Config.Bucket") {
      set_value(s3_config_.bucket, value);
      return s3_config_.bucket;
    }
    if (name == "S3Config.CacheTimeoutSeconds") {
      const auto timeout =
          std::max(std::uint16_t(5U), utils::string::to_uint16(value));
      set_value(s3_config_.cache_timeout_secs, timeout);
      return std::to_string(s3_config_.cache_timeout_secs);
    }
    if (name == "S3Config.Region") {
      set_value(s3_config_.region, value);
      return s3_config_.region;
    }
    if (name == "S3Config.SecretKey") {
      set_value(s3_config_.secret_key, value);
      return s3_config_.secret_key;
    }
    if (name == "S3Config.URL") {
      set_value(s3_config_.url, value);
      return s3_config_.url;
    }
    if (name == "S3Config.UsePathStyle") {
      set_value(s3_config_.use_path_style, utils::string::to_bool(value));
      return utils::string::from_bool(s3_config_.use_path_style);
    }
    if (name == "S3Config.UseRegionInURL") {
      set_value(s3_config_.use_region_in_url, utils::string::to_bool(value));
      return utils::string::from_bool(s3_config_.use_region_in_url);
    }
    if (name == "S3Config.TimeoutMs") {
      set_value(s3_config_.timeout_ms, utils::string::to_uint32(value));
      return std::to_string(s3_config_.timeout_ms);
    }
    if (name == "S3Config.EncryptionToken") {
      set_value(s3_config_.encryption_token, value);
      return s3_config_.encryption_token;
    }
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }
  return "";
}
} // namespace repertory
