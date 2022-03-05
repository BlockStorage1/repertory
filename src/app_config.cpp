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
#include "app_config.hpp"
#include "types/startup_exception.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
app_config::app_config(const provider_type &pt, const std::string &data_directory)
    : pt_(pt),
      api_auth_(utils::generate_random_string(48u)),
      api_port_(default_rpc_port(pt)),
      api_user_("repertory"),
      config_changed_(false),
      data_directory_(data_directory.empty() ? default_data_directory(pt)
                      : ((pt == provider_type::remote) || (pt == provider_type::s3))
                          ? utils::path::absolute(data_directory)
                          : utils::path::absolute(
                                utils::path::combine(data_directory, {get_provider_name(pt)}))),
      download_timeout_secs_(30),
      enable_chunk_downloader_timeout_(true),
      enable_comm_duration_events_(false),
      enable_drive_events_(false),
      enable_max_cache_size_(true),
#ifdef _WIN32
      enable_mount_manager_(false),
#endif
      enable_remote_mount_(false),
      event_level_(event_level::normal),
      eviction_delay_mins_(30),
      eviction_uses_accessed_time_(false),
      high_freq_interval_secs_(30),
      is_remote_mount_(false),
      low_freq_interval_secs_(60 * 60),
      max_cache_size_bytes_(20 * 1024 * 1024 * 1024ULL),
      max_upload_count_(5u),
      min_download_timeout_secs_(5),
      minimum_redundancy_(2.5),
      online_check_retry_secs_(60),
      orphaned_file_retention_days_(15),
      preferred_download_type_(utils::download_type_to_string(download_type::fallback)),
      read_ahead_count_(4),
      remote_client_pool_size_(10),
      remote_host_name_or_ip_(""),
      remote_max_connections_(20),
      remote_port_((pt == provider_type::sia)      ? 20000
                   : (pt == provider_type::s3)     ? 20002
                   : (pt == provider_type::skynet) ? 20003
                                                   : 20001),
      remote_receive_timeout_secs_(120),
      remote_send_timeout_secs_(30),
      remote_token_(""),
      retry_read_count_(6),
      ring_buffer_file_size_(512),
      storage_byte_month_("0") {
  cache_directory_ = utils::path::combine(data_directory_, {"cache"});
  log_directory_ = utils::path::combine(data_directory_, {"logs"});

  hc_.agent_string = default_agent_name(pt_);
  hc_.api_password = get_provider_api_password(pt_);
  hc_.api_port = default_api_port(pt_);

  if (not utils::file::create_full_directory_path(data_directory_))
    throw startup_exception("unable to create: " + data_directory_);

  if (not utils::file::create_full_directory_path(cache_directory_))
    throw startup_exception("unable to create: " + cache_directory_);

  if (not utils::file::create_full_directory_path(log_directory_))
    throw startup_exception("unable to create: " + log_directory_);

  if (not load()) {
    save();
  }
}

std::string app_config::get_config_file_path() const {
  const auto configFilePath = utils::path::combine(data_directory_, {"config.json"});
  return configFilePath;
}

std::string app_config::default_agent_name(const provider_type &pt) {
  static const std::vector<std::string> PROVIDER_AGENT_NAMES = {"Sia-Agent", "", "", "", ""};

  return PROVIDER_AGENT_NAMES[static_cast<int>(pt)];
}

std::uint16_t app_config::default_api_port(const provider_type &pt) {
  static const std::vector<std::uint16_t> PROVIDER_API_PORTS = {9980u, 0u, 0u, 0u, 0u};
  return PROVIDER_API_PORTS[static_cast<int>(pt)];
}

std::string app_config::default_data_directory(const provider_type &pt) {
#ifdef _WIN32
  auto data_directory =
      utils::path::combine(utils::get_local_app_data_directory(),
                           {REPERTORY_DATA_NAME, app_config::get_provider_name(pt)});
#else
#ifdef __APPLE__
  auto data_directory =
      utils::path::resolve(std::string("~/Library/Application Support/") + REPERTORY_DATA_NAME +
                           '/' + app_config::get_provider_name(pt));
#else
  auto data_directory = utils::path::resolve(std::string("~/.local/") + REPERTORY_DATA_NAME + '/' +
                                             app_config::get_provider_name(pt));
#endif
#endif
  return data_directory;
}

std::uint16_t app_config::default_rpc_port(const provider_type &pt) {
  static const std::vector<std::uint16_t> PROVIDER_RPC_PORTS = {
      11101u, 11105u, 11103u, 11104u, 11105u,
  };
  return PROVIDER_RPC_PORTS[static_cast<int>(pt)];
}

json app_config::get_json() const {
  json ret = {{"ApiAuth", api_auth_},
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
              {"EventLevel", event_level_to_string(event_level_)},
              {"EvictionDelayMinutes", eviction_delay_mins_},
              {"EvictionUsesAccessedTime", eviction_uses_accessed_time_},
              {"HighFreqIntervalSeconds", high_freq_interval_secs_},
              {"HostConfig",
               {{"AgentString", hc_.agent_string},
                {"ApiPassword", hc_.api_password},
                {"ApiPort", hc_.api_port},
                {"HostNameOrIp", hc_.host_name_or_ip},
                {"TimeoutMs", hc_.timeout_ms}}},
              {"LowFreqIntervalSeconds", low_freq_interval_secs_},
              {"MaxCacheSizeBytes", max_cache_size_bytes_},
              {"MaxUploadCount", max_upload_count_},
              {"MinimumRedundancy", minimum_redundancy_},
              {"OnlineCheckRetrySeconds", online_check_retry_secs_},
              {"OrphanedFileRetentionDays", orphaned_file_retention_days_},
              {"PreferredDownloadType", preferred_download_type_},
              {"ReadAheadCount", read_ahead_count_},
              {
                  "RemoteMount",
                  {{"EnableRemoteMount", enable_remote_mount_},
                   {"IsRemoteMount", is_remote_mount_},
                   {"RemoteClientPoolSize", remote_client_pool_size_},
                   {"RemoteMaxConnections", remote_max_connections_},
                   {"RemoteHostNameOrIp", remote_host_name_or_ip_},
                   {"RemotePort", remote_port_},
                   {"RemoteReceiveTimeoutSeconds", remote_receive_timeout_secs_},
                   {"RemoteSendTimeoutSeconds", remote_send_timeout_secs_},
                   {"RemoteToken", remote_token_}},
              },
              {"RetryReadCount", retry_read_count_},
              {"RingBufferFileSize", ring_buffer_file_size_},
              {"S3Config",
               {{"AccessKey", s3_config_.access_key},
                {"Bucket", s3_config_.bucket},
                {"CacheTimeoutSeconds", s3_config_.cache_timeout_secs},
                {"EncryptionToken", s3_config_.encryption_token},
                {"Region", s3_config_.region},
                {"SecretKey", s3_config_.secret_key},
                {"TimeoutMs", s3_config_.timeout_ms},
                {"URL", s3_config_.url}}},
              {"SkynetConfig",
               {{"EncryptionToken", skynet_config_.encryption_token},
                {"PortalList", skynet_config_.portal_list}}},
              {"StorageByteMonth", storage_byte_month_.ToString()},
              {"Version", version_}};

  if (pt_ == provider_type::s3) {
    ret.erase("HostConfig");
    ret.erase("SkynetConfig");
    ret.erase("StorageByteMonth");
  } else if ((pt_ == provider_type::sia)) {
    ret.erase("S3Config");
    ret.erase("SkynetConfig");
    ret.erase("MaxUploadCount");
  } else if (pt_ == provider_type::skynet) {
    ret.erase("S3Config");
    ret.erase("HostConfig");
    ret.erase("StorageByteMonth");
    ret.erase("OrphanedFileRetentionDays");
  } else if (pt_ == provider_type::remote) {
    ret.erase("MaxUploadCount");
    ret.erase("S3Config");
    ret.erase("HostConfig");
    ret.erase("SkynetConfig");
    ret.erase("ChunkDownloaderTimeoutSeconds");
    ret.erase("EnableChunkDownloaderTimeout");
    ret.erase("EnableChunkDownloaderTimeout");
    ret.erase("EnableMountManager");
    ret.erase("EnableMaxCacheSize");
    ret.erase("EvictionDelayMinutes");
    ret.erase("HighFreqIntervalSeconds");
    ret.erase("LowFreqIntervalSeconds");
    ret.erase("MaxCacheSizeBytes");
    ret.erase("MinimumRedundancy");
    ret.erase("OnlineCheckRetrySeconds");
    ret.erase("OrphanedFileRetentionDays");
    ret.erase("PreferredDownloadType");
    ret.erase("ReadAheadCount");
    ret.erase("RetryReadCount");
    ret.erase("RingBufferFileSize");
    ret.erase("StorageByteMonth");
  }

  if ((pt_ == provider_type::s3) || (pt_ == provider_type::skynet)) {
    ret.erase("MinimumRedundancy");
  }

  return ret;
}

std::uint64_t app_config::get_max_cache_size_bytes() const {
  const auto maxSpace = std::max((std::uint64_t)100 * 1024 * 1024, max_cache_size_bytes_);
  return std::min(utils::file::get_available_drive_space(get_cache_directory()), maxSpace);
}

std::string app_config::get_provider_api_password(const provider_type &pt) {
#ifdef _WIN32
  auto api_file = utils::path::combine(utils::get_local_app_data_directory(),
                                       {get_provider_display_name(pt), "apipassword"});
#else
#ifdef __APPLE__
  auto api_file = utils::path::combine(
      utils::path::resolve("~"),
      {"/Library/Application Support", get_provider_display_name(pt), "apipassword"});
#else
  auto api_file =
      utils::path::combine(utils::path::resolve("~/."), {get_provider_name(pt), "apipassword"});
#endif
#endif
  auto lines = utils::file::read_file_lines(api_file);
  return lines.empty() ? "" : utils::string::trim(lines[0]);
}

std::string app_config::get_provider_display_name(const provider_type &pt) {
  static const std::vector<std::string> PROVIDER_DISPLAY_NAMES = {
      "Sia", "Remote", "S3", "Skynet", "Passthrough",
  };
  return PROVIDER_DISPLAY_NAMES[static_cast<int>(pt)];
}

std::string app_config::get_provider_minimum_version(const provider_type &pt) {
  static const std::vector<std::string> PROVIDER_MIN_VERSIONS = {
      MIN_SIA_VERSION, MIN_SP_VERSION, MIN_REMOTE_VERSION, "", "", ""};
  return PROVIDER_MIN_VERSIONS[static_cast<int>(pt)];
}

std::string app_config::get_provider_name(const provider_type &pt) {
  static const std::vector<std::string> PROVIDER_NAMES = {
      "sia", "remote", "s3", "skynet", "passthrough",
  };
  return PROVIDER_NAMES[static_cast<int>(pt)];
}

std::string app_config::get_provider_path_name(const provider_type &pt) {
  static const std::vector<std::string> PROVIDER_PATH_NAMES = {
      "siapath", "", "", "", "",
  };
  return PROVIDER_PATH_NAMES[static_cast<int>(pt)];
}

std::string app_config::get_value_by_name(const std::string &name) {
  try {
    if (name == "ApiAuth") {
      return api_auth_;
    } else if (name == "ApiPort") {
      return std::to_string(get_api_port());
    } else if (name == "ApiUser") {
      return api_user_;
    } else if (name == "ChunkDownloaderTimeoutSeconds") {
      return std::to_string(get_chunk_downloader_timeout_secs());
    } else if (name == "EnableChunkDownloaderTimeout") {
      return std::to_string(get_enable_chunk_download_timeout());
    } else if (name == "GetEnableCommDurationEvents") {
      return std::to_string(get_enable_comm_duration_events());
    } else if (name == "EnableDriveEvents") {
      return std::to_string(get_enable_drive_events());
    } else if (name == "EnableMaxCacheSize") {
      return std::to_string(get_enable_max_cache_size());
#ifdef _WIN32
    } else if (name == "EnableMountManager") {
      return std::to_string(get_enable_mount_manager());
#endif
    } else if (name == "EventLevel") {
      return event_level_to_string(get_event_level());
    } else if (name == "EvictionDelayMinutes") {
      return std::to_string(get_eviction_delay_mins());
    } else if (name == "EvictionUsesAccessedTime") {
      return std::to_string(get_eviction_uses_accessed_time());
    } else if (name == "HighFreqIntervalSeconds") {
      return std::to_string(get_high_frequency_interval_secs());
    } else if (name == "HostConfig.AgentString") {
      return hc_.agent_string;
    } else if (name == "HostConfig.ApiPassword") {
      return hc_.api_password;
    } else if (name == "HostConfig.ApiPort") {
      return std::to_string(hc_.api_port);
    } else if (name == "HostConfig.HostNameOrIp") {
      return hc_.host_name_or_ip;
    } else if (name == "HostConfig.TimeoutMs") {
      return std::to_string(hc_.timeout_ms);
    } else if (name == "LowFreqIntervalSeconds") {
      return std::to_string(get_low_frequency_interval_secs());
    } else if (name == "MaxCacheSizeBytes") {
      return std::to_string(get_max_cache_size_bytes());
    } else if (name == "MaxUploadCount") {
      return std::to_string(get_max_upload_count());
    } else if (name == "MinimumRedundancy") {
      return std::to_string(get_minimum_redundancy());
    } else if (name == "OnlineCheckRetrySeconds") {
      return std::to_string(get_online_check_retry_secs());
    } else if (name == "OrphanedFileRetentionDays") {
      return std::to_string(get_orphaned_file_retention_days());
    } else if (name == "PreferredDownloadType") {
      return utils::download_type_to_string(
          utils::download_type_from_string(preferred_download_type_, download_type::fallback));
    } else if (name == "ReadAheadCount") {
      return std::to_string(get_read_ahead_count());
    } else if (name == "RemoteMount.EnableRemoteMount") {
      return std::to_string(get_enable_remote_mount());
    } else if (name == "RemoteMount.IsRemoteMount") {
      return std::to_string(get_is_remote_mount());
    } else if (name == "RemoteMount.RemoteClientPoolSize") {
      return std::to_string(get_remote_client_pool_size());
    } else if (name == "RemoteMount.RemoteHostNameOrIp") {
      return get_remote_host_name_or_ip();
    } else if (name == "RemoteMount.RemoteMaxConnections") {
      return std::to_string(get_remote_max_connections());
    } else if (name == "RemoteMount.RemotePort") {
      return std::to_string(get_remote_port());
    } else if (name == "RemoteMount.RemoteReceiveTimeoutSeconds") {
      return std::to_string(get_remote_receive_timeout_secs());
    } else if (name == "RemoteMount.RemoteSendTimeoutSeconds") {
      return std::to_string(get_remote_send_timeout_secs());
    } else if (name == "RemoteMount.RemoteToken") {
      return get_remote_token();
    } else if (name == "RetryReadCount") {
      return std::to_string(get_retry_read_count());
    } else if (name == "RingBufferFileSize") {
      return std::to_string(get_ring_buffer_file_size());
    } else if (name == "S3Config.AccessKey") {
      return s3_config_.access_key;
    } else if (name == "S3Config.Bucket") {
      return s3_config_.bucket;
    } else if (name == "S3Config.EncryptionToken") {
      return s3_config_.encryption_token;
    } else if (name == "S3Config.CacheTimeoutSeconds") {
      return std::to_string(s3_config_.cache_timeout_secs);
    } else if (name == "S3Config.Region") {
      return s3_config_.region;
    } else if (name == "S3Config.SecretKey") {
      return s3_config_.secret_key;
    } else if (name == "S3Config.URL") {
      return s3_config_.url;
    } else if (name == "S3Config.TimeoutMs") {
      return std::to_string(s3_config_.timeout_ms);
    } else if (name == "S3Config.EncryptionToken") {
      return s3_config_.encryption_token;
    } else if (name == "SkynetConfig.EncryptionToken") {
      return skynet_config_.encryption_token;
    } else if (name == "SkynetConfig.PortalList") {
      return skynet_config_.to_string();
    }
  } catch (const std::exception &e) {
    event_system::instance().raise<repertory_exception>(__FUNCTION__, e.what());
  }
  return "";
}

bool app_config::load() {
  auto ret = false;

  const auto config_file_path = get_config_file_path();
  recur_mutex_lock l(read_write_mutex_);
  if (utils::file::is_file(config_file_path)) {
    try {
      std::ifstream config_file(&config_file_path[0]);
      if (config_file.is_open()) {
        std::stringstream ss;
        ss << config_file.rdbuf();
        const auto json_text = ss.str();
        config_file.close();
        if ((ret = not json_text.empty())) {
          const auto json_document = json::parse(json_text);

          get_value(json_document, "ApiAuth", api_auth_, ret);
          get_value(json_document, "ApiPort", api_port_, ret);
          get_value(json_document, "ApiUser", api_user_, ret);
          get_value(json_document, "ChunkDownloaderTimeoutSeconds", download_timeout_secs_, ret);
          get_value(json_document, "EvictionDelayMinutes", eviction_delay_mins_, ret);
          get_value(json_document, "EvictionUsesAccessedTime", eviction_uses_accessed_time_, ret);
          get_value(json_document, "EnableChunkDownloaderTimeout", enable_chunk_downloader_timeout_,
                    ret);
          get_value(json_document, "EnableCommDurationEvents", enable_comm_duration_events_, ret);
          get_value(json_document, "EnableDriveEvents", enable_drive_events_, ret);

          std::string level;
          if (get_value(json_document, "EventLevel", level, ret)) {
            event_level_ = event_level_from_string(level);
          }

          if (json_document.find("HostConfig") != json_document.end()) {
            auto host_config_json = json_document["HostConfig"];
            auto hc = hc_;
            get_value(host_config_json, "AgentString", hc.agent_string, ret);
            get_value(host_config_json, "ApiPassword", hc.api_password, ret);
            get_value(host_config_json, "ApiPort", hc.api_port, ret);
            get_value(host_config_json, "HostNameOrIp", hc.host_name_or_ip, ret);
            get_value(host_config_json, "TimeoutMs", hc.timeout_ms, ret);
            hc_ = hc;
          } else {
            ret = false;
          }

          if (hc_.api_password.empty()) {
            hc_.api_password = get_provider_api_password(pt_);
            if (hc_.api_password.empty()) {
              ret = false;
            }
          }

          if (json_document.find("S3Config") != json_document.end()) {
            auto s3_config_json = json_document["S3Config"];
            auto s3 = s3_config_;
            get_value(s3_config_json, "AccessKey", s3.access_key, ret);
            get_value(s3_config_json, "Bucket", s3.bucket, ret);
            get_value(s3_config_json, "CacheTimeoutSeconds", s3.cache_timeout_secs, ret);
            get_value(s3_config_json, "EncryptionToken", s3.encryption_token, ret);
            get_value(s3_config_json, "Region", s3.region, ret);
            get_value(s3_config_json, "SecretKey", s3.secret_key, ret);
            get_value(s3_config_json, "TimeoutMs", s3.timeout_ms, ret);
            get_value(s3_config_json, "URL", s3.url, ret);
            s3_config_ = s3;
          } else {
            ret = false;
          }

          if (json_document.find("SkynetConfig") != json_document.end()) {
            auto skynet_config_json = json_document["SkynetConfig"];
            auto skynet = skynet_config_;
            get_value(skynet_config_json, "EncryptionToken", skynet.encryption_token, ret);
            get_value(skynet_config_json, "PortalList", skynet.portal_list, ret);
            skynet_config_ = skynet;
          } else {
            ret = false;
          }

          get_value(json_document, "ReadAheadCount", read_ahead_count_, ret);
          get_value(json_document, "RingBufferFileSize", ring_buffer_file_size_, ret);
          get_value(json_document, "MinimumRedundancy", minimum_redundancy_, ret);
          get_value(json_document, "EnableMaxCacheSize", enable_max_cache_size_, ret);
#ifdef _WIN32
          get_value(json_document, "EnableMountManager", enable_mount_manager_, ret);
#endif
          get_value(json_document, "MaxCacheSizeBytes", max_cache_size_bytes_, ret);
          get_value(json_document, "MaxUploadCount", max_upload_count_, ret);
          get_value(json_document, "OnlineCheckRetrySeconds", online_check_retry_secs_, ret);
          get_value(json_document, "HighFreqIntervalSeconds", high_freq_interval_secs_, ret);
          get_value(json_document, "LowFreqIntervalSeconds", low_freq_interval_secs_, ret);
          get_value(json_document, "OrphanedFileRetentionDays", orphaned_file_retention_days_, ret);
          get_value(json_document, "PreferredDownloadType", preferred_download_type_, ret);
          get_value(json_document, "RetryReadCount", retry_read_count_, ret);
          if (json_document.find("RemoteMount") != json_document.end()) {
            auto remoteMount = json_document["RemoteMount"];
            get_value(remoteMount, "EnableRemoteMount", enable_remote_mount_, ret);
            get_value(remoteMount, "IsRemoteMount", is_remote_mount_, ret);
            get_value(remoteMount, "RemoteClientPoolSize", remote_client_pool_size_, ret);
            get_value(remoteMount, "RemoteHostNameOrIp", remote_host_name_or_ip_, ret);
            get_value(remoteMount, "RemoteMaxConnections", remote_max_connections_, ret);
            get_value(remoteMount, "RemotePort", remote_port_, ret);
            get_value(remoteMount, "RemoteReceiveTimeoutSeconds", remote_receive_timeout_secs_,
                      ret);
            get_value(remoteMount, "RemoteSendTimeoutSeconds", remote_send_timeout_secs_, ret);
            get_value(remoteMount, "RemoteToken", remote_token_, ret);
          } else {
            ret = false;
          }
          std::string storage_byte_month;
          if (get_value(json_document, "StorageByteMonth", storage_byte_month, ret)) {
            storage_byte_month_ = storage_byte_month;
          }

          std::uint64_t version = 0u;
          get_value(json_document, "Version", version, ret);

          // Handle configuration defaults for new config versions
          if (version != REPERTORY_CONFIG_VERSION) {
            if (version > REPERTORY_CONFIG_VERSION) {
              version = 0u;
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
      event_system::instance().raise<repertory_exception>(__FUNCTION__, ex.what());
      ret = false;
    }
  }

  return ret;
}

void app_config::save() {
  const auto configFilePath = get_config_file_path();
  recur_mutex_lock l(read_write_mutex_);
  if (config_changed_ || not utils::file::is_file(configFilePath)) {
    if (not utils::file::is_directory(data_directory_)) {
      utils::file::create_full_directory_path(data_directory_);
    }
    config_changed_ = false;
    json data = get_json();
    auto success = false;
    for (auto i = 0; not success && (i < 5); i++) {
      if (not(success = utils::file::write_json_file(configFilePath, data))) {
        std::this_thread::sleep_for(1s);
      }
    }
  }
}

void app_config::set_enable_remote_mount(const bool &enable_remote_mount) {
  recur_mutex_lock remote_lock(remote_mount_mutex_);
  if (get_is_remote_mount()) {
    set_value(enable_remote_mount_, false);
  } else {
    set_value(enable_remote_mount_, enable_remote_mount);
  }
}

void app_config::set_is_remote_mount(const bool &is_remote_mount) {
  recur_mutex_lock remote_lock(remote_mount_mutex_);

  if (get_enable_remote_mount()) {
    set_value(is_remote_mount_, false);
    return;
  }

  set_value(is_remote_mount_, is_remote_mount);
}

void app_config::set_storage_byte_month(const api_currency &storage_byte_month) {
  if ((storage_byte_month > 0) && (storage_byte_month_ != storage_byte_month)) {
    storage_byte_month_ = storage_byte_month;
    config_changed_ = true;
  }
}

std::string app_config::set_value_by_name(const std::string &name, const std::string &value) {
  try {
    if (name == "ApiAuth") {
      set_api_auth(value);
      return get_api_auth();
    } else if (name == "ApiPort") {
      set_api_port(utils::string::to_uint16(value));
      return std::to_string(get_api_port());
    } else if (name == "ApiUser") {
      set_api_user(value);
      return get_api_user();
    } else if (name == "ChunkDownloaderTimeoutSeconds") {
      set_chunk_downloader_timeout_secs(utils::string::to_uint8(value));
      return std::to_string(get_chunk_downloader_timeout_secs());
    } else if (name == "EnableChunkDownloaderTimeout") {
      set_enable_chunk_downloader_timeout(utils::string::to_bool(value));
      return std::to_string(get_enable_chunk_download_timeout());
    } else if (name == "EnableCommDurationEvents") {
      set_enable_comm_duration_events(utils::string::to_bool(value));
      return std::to_string(get_enable_comm_duration_events());
    } else if (name == "EnableDriveEvents") {
      set_enable_drive_events(utils::string::to_bool(value));
      return std::to_string(get_enable_drive_events());
    } else if (name == "EnableMaxCacheSize") {
      set_enable_max_cache_size(utils::string::to_bool(value));
      return std::to_string(get_enable_max_cache_size());
#ifdef _WIN32
    } else if (name == "EnableMountManager") {
      set_enable_mount_manager(utils::string::to_bool(value));
      return std::to_string(get_enable_mount_manager());
#endif
    } else if (name == "EventLevel") {
      set_event_level(event_level_from_string(value));
      return event_level_to_string(get_event_level());
    } else if (name == "EvictionDelayMinutes") {
      set_eviction_delay_mins(utils::string::to_uint32(value));
      return std::to_string(get_eviction_delay_mins());
    } else if (name == "EvictionUsesAccessedTime") {
      set_eviction_uses_accessed_time(utils::string::to_bool(value));
      return std::to_string(get_eviction_uses_accessed_time());
    } else if (name == "HighFreqIntervalSeconds") {
      set_high_frequency_interval_secs(utils::string::to_uint8(value));
      return std::to_string(get_high_frequency_interval_secs());
    } else if (name == "HostConfig.AgentString") {
      set_value(hc_.agent_string, value);
      return hc_.agent_string;
    } else if (name == "HostConfig.ApiPassword") {
      set_value(hc_.api_password, value);
      return hc_.api_password;
    } else if (name == "HostConfig.ApiPort") {
      set_value(hc_.api_port, utils::string::to_uint16(value));
      return std::to_string(hc_.api_port);
    } else if (name == "HostConfig.HostNameOrIp") {
      set_value(hc_.host_name_or_ip, value);
      return hc_.host_name_or_ip;
    } else if (name == "HostConfig.TimeoutMs") {
      set_value(hc_.timeout_ms, utils::string::to_uint32(value));
      return std::to_string(hc_.timeout_ms);
    } else if (name == "LowFreqIntervalSeconds") {
      set_low_frequency_interval_secs(utils::string::to_uint32(value));
      return std::to_string(get_low_frequency_interval_secs());
    } else if (name == "MaxCacheSizeBytes") {
      set_max_cache_size_bytes(utils::string::to_uint64(value));
      return std::to_string(get_max_cache_size_bytes());
    } else if (name == "MaxUploadCount") {
      set_max_upload_count(utils::string::to_uint8(value));
      return std::to_string(get_max_upload_count());
    } else if (name == "MinimumRedundancy") {
      set_minimum_redundancy(utils::string::to_double(value));
      return std::to_string(get_minimum_redundancy());
    } else if (name == "OnlineCheckRetrySeconds") {
      set_online_check_retry_secs(utils::string::to_uint16(value));
      return std::to_string(get_online_check_retry_secs());
    } else if (name == "OrphanedFileRetentionDays") {
      set_orphaned_file_retention_days(utils::string::to_uint16(value));
      return std::to_string(get_orphaned_file_retention_days());
    } else if (name == "PreferredDownloadType") {
      set_preferred_download_type(utils::download_type_from_string(value, download_type::fallback));
      return utils::download_type_to_string(get_preferred_download_type());
    } else if (name == "ReadAheadCount") {
      set_read_ahead_count(utils::string::to_uint8(value));
      return std::to_string(get_read_ahead_count());
    } else if (name == "RemoteMount.EnableRemoteMount") {
      set_enable_remote_mount(utils::string::to_bool(value));
      return std::to_string(get_enable_remote_mount());
    } else if (name == "RemoteMount.IsRemoteMount") {
      set_is_remote_mount(utils::string::to_bool(value));
      return std::to_string(get_is_remote_mount());
    } else if (name == "RemoteMount.RemoteClientPoolSize") {
      set_remote_client_pool_size(utils::string::to_uint8(value));
      return std::to_string(get_remote_client_pool_size());
    } else if (name == "RemoteMount.RemoteHostNameOrIp") {
      set_remote_host_name_or_ip(value);
      return get_remote_host_name_or_ip();
    } else if (name == "RemoteMount.RemoteMaxConnections") {
      set_remote_max_connections(utils::string::to_uint8(value));
      return std::to_string(get_remote_max_connections());
    } else if (name == "RemoteMount.RemotePort") {
      set_remote_port(utils::string::to_uint16(value));
      return std::to_string(get_remote_port());
    } else if (name == "RemoteMount.RemoteReceiveTimeoutSeconds") {
      set_remote_receive_timeout_secs(utils::string::to_uint16(value));
      return std::to_string(get_remote_receive_timeout_secs());
    } else if (name == "RemoteMount.RemoteSendTimeoutSeconds") {
      set_remote_send_timeout_secs(utils::string::to_uint16(value));
      return std::to_string(get_remote_send_timeout_secs());
    } else if (name == "RemoteMount.RemoteToken") {
      set_remote_token(value);
      return get_remote_token();
    } else if (name == "RetryReadCount") {
      set_retry_read_count(utils::string::to_uint16(value));
      return std::to_string(get_retry_read_count());
    } else if (name == "RingBufferFileSize") {
      set_ring_buffer_file_size(utils::string::to_uint16(value));
      return std::to_string(get_ring_buffer_file_size());
    } else if (name == "S3Config.AccessKey") {
      set_value(s3_config_.access_key, value);
      return s3_config_.access_key;
    } else if (name == "S3Config.Bucket") {
      set_value(s3_config_.bucket, value);
      return s3_config_.bucket;
    } else if (name == "S3Config.CacheTimeoutSeconds") {
      const auto timeout = std::max(std::uint16_t(5u), utils::string::to_uint16(value));
      set_value(s3_config_.cache_timeout_secs, timeout);
      return std::to_string(s3_config_.cache_timeout_secs);
    } else if (name == "S3Config.Region") {
      set_value(s3_config_.region, value);
      return s3_config_.region;
    } else if (name == "S3Config.SecretKey") {
      set_value(s3_config_.secret_key, value);
      return s3_config_.secret_key;
    } else if (name == "S3Config.URL") {
      set_value(s3_config_.url, value);
      return s3_config_.url;
    } else if (name == "S3Config.TimeoutMs") {
      set_value(s3_config_.timeout_ms, utils::string::to_uint32(value));
      return std::to_string(s3_config_.timeout_ms);
    } else if (name == "S3Config.EncryptionToken") {
      set_value(s3_config_.encryption_token, value);
      return s3_config_.encryption_token;
    } else if (name == "SkynetConfig.EncryptionToken") {
      set_value(skynet_config_.encryption_token, value);
      return skynet_config_.encryption_token;
    } else if (name == "SkynetConfig.PortalList") {
      set_value(skynet_config_.portal_list, skynet_config::from_string(value));
      return skynet_config_.to_string();
    }
  } catch (const std::exception &e) {
    event_system::instance().raise<repertory_exception>(__FUNCTION__, e.what());
  }
  return "";
}
} // namespace repertory
