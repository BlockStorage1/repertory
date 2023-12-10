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
#ifndef INCLUDE_APP_CONFIG_HPP_
#define INCLUDE_APP_CONFIG_HPP_

#include "events/event_system.hpp"
#include "events/events.hpp"
#include "types/repertory.hpp"
#include "utils/error_utils.hpp"

namespace repertory {
class app_config final {
public:
  [[nodiscard]] static auto default_agent_name(const provider_type &prov)
      -> std::string;

  [[nodiscard]] static auto default_api_port(const provider_type &prov)
      -> std::uint16_t;

  [[nodiscard]] static auto default_data_directory(const provider_type &prov)
      -> std::string;

  [[nodiscard]] static auto default_remote_port(const provider_type &prov)
      -> std::uint16_t;

  [[nodiscard]] static auto default_rpc_port(const provider_type &prov)
      -> std::uint16_t;

  [[nodiscard]] static auto get_provider_api_password(const provider_type &prov)
      -> std::string;

  [[nodiscard]] static auto get_provider_display_name(const provider_type &prov)
      -> std::string;

  [[nodiscard]] static auto get_provider_name(const provider_type &prov)
      -> std::string;

public:
  app_config(const provider_type &prov, const std::string &data_directory = "");

  ~app_config() { save(); }

private:
  provider_type prov_;
  std::string api_auth_;
  std::uint16_t api_port_;
  std::string api_user_;
  bool config_changed_;
  std::string data_directory_;
  std::uint8_t download_timeout_secs_;
  bool enable_chunk_downloader_timeout_;
  bool enable_comm_duration_events_;
  bool enable_drive_events_;
  bool enable_max_cache_size_;
#ifdef _WIN32
  bool enable_mount_manager_;
#endif
  bool enable_remote_mount_;
  encrypt_config encrypt_config_;
  event_level event_level_;
  std::uint32_t eviction_delay_mins_;
  bool eviction_uses_accessed_time_;
  std::uint8_t high_freq_interval_secs_;
  bool is_remote_mount_;
  std::uint32_t low_freq_interval_secs_;
  std::uint64_t max_cache_size_bytes_;
  std::uint8_t max_upload_count_;
  std::uint8_t min_download_timeout_secs_;
  std::uint16_t online_check_retry_secs_;
  std::uint16_t orphaned_file_retention_days_;
  std::string preferred_download_type_;
  std::uint8_t read_ahead_count_;
  std::uint8_t remote_client_pool_size_;
  std::string remote_host_name_or_ip_;
  std::uint8_t remote_max_connections_;
  std::uint16_t remote_port_;
  std::uint16_t remote_receive_timeout_secs_;
  std::uint16_t remote_send_timeout_secs_;
  std::string remote_token_;
  std::uint16_t retry_read_count_;
  std::uint16_t ring_buffer_file_size_;
  std::string cache_directory_;
  host_config hc_;
  s3_config s3_config_;
  std::uint64_t version_ = REPERTORY_CONFIG_VERSION;
  std::string log_directory_;
  mutable std::recursive_mutex read_write_mutex_;
  mutable std::recursive_mutex remote_mount_mutex_;

private:
  [[nodiscard]] auto load() -> bool;

  template <typename dest>
  auto get_value(const json &json_document, const std::string &name, dest &dst,
                 bool &success_flag) -> bool {
    auto ret = false;
    try {
      if (json_document.find(name) != json_document.end()) {
        dst = json_document[name].get<dest>();
        ret = true;
      } else {
        success_flag = false;
      }
    } catch (const json::exception &ex) {
      utils::error::raise_error(__FUNCTION__, ex, "exception occurred");
      success_flag = false;
      ret = false;
    }

    return ret;
  }

  template <typename dest, typename source>
  auto set_value(dest &dst, const source &src) -> bool {
    auto ret = false;
    recur_mutex_lock lock(read_write_mutex_);
    if (dst != src) {
      dst = src;
      config_changed_ = true;
      save();
      ret = true;
    }

    return ret;
  }

public:
  [[nodiscard]] auto get_api_auth() const -> std::string { return api_auth_; }

  [[nodiscard]] auto get_api_port() const -> std::uint16_t { return api_port_; }

  [[nodiscard]] auto get_api_user() const -> std::string { return api_user_; }

  [[nodiscard]] auto get_cache_directory() const -> std::string {
    return cache_directory_;
  }

  [[nodiscard]] auto get_chunk_downloader_timeout_secs() const -> std::uint8_t {
    return std::max(min_download_timeout_secs_, download_timeout_secs_);
  }

  [[nodiscard]] auto get_config_file_path() const -> std::string;

  [[nodiscard]] auto get_data_directory() const -> std::string {
    return data_directory_;
  }

  [[nodiscard]] auto get_enable_chunk_download_timeout() const -> bool {
    return enable_chunk_downloader_timeout_;
  }

  [[nodiscard]] auto get_enable_comm_duration_events() const -> bool {
    return enable_comm_duration_events_;
  }

  [[nodiscard]] auto get_enable_drive_events() const -> bool {
    return enable_drive_events_;
  }

  [[nodiscard]] auto get_encrypt_config() const -> encrypt_config {
    return encrypt_config_;
  }

#ifdef _WIN32
  [[nodiscard]] auto get_enable_mount_manager() const -> bool {
    return enable_mount_manager_;
  }
#endif

  [[nodiscard]] auto get_enable_max_cache_size() const -> bool {
    return enable_max_cache_size_;
  }

  [[nodiscard]] auto get_enable_remote_mount() const -> bool {
    return enable_remote_mount_;
  }

  [[nodiscard]] auto get_event_level() const -> event_level {
    return event_level_;
  }

  [[nodiscard]] auto get_eviction_delay_mins() const -> std::uint32_t {
    return eviction_delay_mins_;
  }

  [[nodiscard]] auto get_eviction_uses_accessed_time() const -> bool {
    return eviction_uses_accessed_time_;
  }

  [[nodiscard]] auto get_high_frequency_interval_secs() const -> std::uint8_t {
    return std::max(static_cast<std::uint8_t>(1U), high_freq_interval_secs_);
  }

  [[nodiscard]] auto get_host_config() const -> host_config { return hc_; }

  [[nodiscard]] auto get_is_remote_mount() const -> bool {
    return is_remote_mount_;
  }

  [[nodiscard]] auto get_json() const -> json;

  [[nodiscard]] auto get_log_directory() const -> std::string {
    return log_directory_;
  }

  [[nodiscard]] auto get_low_frequency_interval_secs() const -> std::uint32_t {
    return std::max(1U, low_freq_interval_secs_);
  }

  [[nodiscard]] auto get_max_cache_size_bytes() const -> std::uint64_t;

  [[nodiscard]] auto get_max_upload_count() const -> std::uint8_t {
    return std::max(std::uint8_t(1U), max_upload_count_);
  }

  [[nodiscard]] auto get_online_check_retry_secs() const -> std::uint16_t {
    return std::max(std::uint16_t(15U), online_check_retry_secs_);
  }

  [[nodiscard]] auto get_orphaned_file_retention_days() const -> std::uint16_t {
    return std::min(static_cast<std::uint16_t>(31U),
                    std::max(static_cast<std::uint16_t>(1U),
                             orphaned_file_retention_days_));
  }

  [[nodiscard]] auto get_preferred_download_type() const -> download_type {
    return utils::download_type_from_string(preferred_download_type_,
                                            download_type::fallback);
  }

  [[nodiscard]] auto get_provider_type() const -> provider_type {
    return prov_;
  }

  [[nodiscard]] auto get_read_ahead_count() const -> std::uint8_t {
    return std::max(static_cast<std::uint8_t>(1U), read_ahead_count_);
  }

  [[nodiscard]] auto get_remote_client_pool_size() const -> std::uint8_t {
    return std::max(static_cast<std::uint8_t>(5U), remote_client_pool_size_);
  }

  [[nodiscard]] auto get_remote_host_name_or_ip() const -> std::string {
    return remote_host_name_or_ip_;
  }

  [[nodiscard]] auto get_remote_max_connections() const -> std::uint8_t {
    return std::max(static_cast<std::uint8_t>(1U), remote_max_connections_);
  }

  [[nodiscard]] auto get_remote_port() const -> std::uint16_t {
    return remote_port_;
  }

  [[nodiscard]] auto get_remote_receive_timeout_secs() const -> std::uint16_t {
    return remote_receive_timeout_secs_;
  }

  [[nodiscard]] auto get_remote_send_timeout_secs() const -> std::uint16_t {
    return remote_send_timeout_secs_;
  }

  [[nodiscard]] auto get_remote_token() const -> std::string {
    return remote_token_;
  }

  [[nodiscard]] auto get_retry_read_count() const -> std::uint16_t {
    return std::max(std::uint16_t(2), retry_read_count_);
  }

  [[nodiscard]] auto get_ring_buffer_file_size() const -> std::uint16_t {
    return std::max(
        static_cast<std::uint16_t>(64U),
        std::min(static_cast<std::uint16_t>(1024U), ring_buffer_file_size_));
  }

  [[nodiscard]] auto get_s3_config() const -> s3_config { return s3_config_; }

  [[nodiscard]] auto get_value_by_name(const std::string &name) -> std::string;

  [[nodiscard]] auto get_version() const -> std::uint64_t { return version_; }

  void save();

  void set_api_auth(const std::string &api_auth) {
    set_value(api_auth_, api_auth);
  }

  void set_api_port(std::uint16_t api_port) { set_value(api_port_, api_port); }

  void set_api_user(const std::string &api_user) {
    set_value(api_user_, api_user);
  }

  void set_chunk_downloader_timeout_secs(
      std::uint8_t chunk_downloader_timeout_secs) {
    set_value(download_timeout_secs_, chunk_downloader_timeout_secs);
  }

  void
  set_enable_chunk_downloader_timeout(bool enable_chunk_downloader_timeout) {
    set_value(enable_chunk_downloader_timeout_,
              enable_chunk_downloader_timeout);
  }

  void set_enable_comm_duration_events(bool enable_comm_duration_events) {
    set_value(enable_comm_duration_events_, enable_comm_duration_events);
  }

  void set_enable_drive_events(bool enable_drive_events) {
    set_value(enable_drive_events_, enable_drive_events);
  }

  void set_enable_max_cache_size(bool enable_max_cache_size) {
    set_value(enable_max_cache_size_, enable_max_cache_size);
  }

#ifdef _WIN32
  void set_enable_mount_manager(bool enable_mount_manager) {
    set_value(enable_mount_manager_, enable_mount_manager);
  }
#endif

  void set_enable_remote_mount(bool enable_remote_mount);

  void set_event_level(const event_level &level) {
    if (set_value(event_level_, level)) {
      event_system::instance().raise<event_level_changed>(
          event_level_to_string(level));
    }
  }

  void set_eviction_delay_mins(std::uint32_t eviction_delay_mins) {
    set_value(eviction_delay_mins_, eviction_delay_mins);
  }

  void set_eviction_uses_accessed_time(bool eviction_uses_accessed_time) {
    set_value(eviction_uses_accessed_time_, eviction_uses_accessed_time);
  }

  void
  set_high_frequency_interval_secs(std::uint8_t high_frequency_interval_secs) {
    set_value(high_freq_interval_secs_, high_frequency_interval_secs);
  }

#ifdef REPERTORY_TESTING
  void set_host_config(host_config hc) {
    config_changed_ = true;
    hc_ = std::move(hc);
    save();
  }

  void set_s3_config(s3_config s3) {
    config_changed_ = true;
    s3_config_ = std::move(s3);
    save();
  }
#endif

  void set_is_remote_mount(bool is_remote_mount);

  void
  set_low_frequency_interval_secs(std::uint32_t low_frequency_interval_secs) {
    set_value(low_freq_interval_secs_, low_frequency_interval_secs);
  }

  void set_max_cache_size_bytes(std::uint64_t max_cache_size_bytes) {
    set_value(max_cache_size_bytes_, max_cache_size_bytes);
  }

  void set_max_upload_count(std::uint8_t max_upload_count) {
    set_value(max_upload_count_, max_upload_count);
  }

  void set_online_check_retry_secs(std::uint16_t online_check_retry_secs) {
    set_value(online_check_retry_secs_, online_check_retry_secs);
  }

  void
  set_orphaned_file_retention_days(std::uint16_t orphaned_file_retention_days) {
    set_value(orphaned_file_retention_days_, orphaned_file_retention_days);
  }

  void set_preferred_download_type(const download_type &dt) {
    set_value(preferred_download_type_, utils::download_type_to_string(dt));
  }

  void set_read_ahead_count(std::uint8_t read_ahead_count) {
    set_value(read_ahead_count_, read_ahead_count);
  }

  void set_remote_client_pool_size(std::uint8_t remote_client_pool_size) {
    set_value(remote_client_pool_size_, remote_client_pool_size);
  }

  void set_ring_buffer_file_size(std::uint16_t ring_buffer_file_size) {
    set_value(ring_buffer_file_size_, ring_buffer_file_size);
  }

  void set_remote_host_name_or_ip(const std::string &remote_host_name_or_ip) {
    set_value(remote_host_name_or_ip_, remote_host_name_or_ip);
  }

  void set_remote_max_connections(std::uint8_t remote_max_connections) {
    set_value(remote_max_connections_, remote_max_connections);
  }

  void set_remote_port(std::uint16_t remote_port) {
    set_value(remote_port_, remote_port);
  }

  void
  set_remote_receive_timeout_secs(std::uint16_t remote_receive_timeout_secs) {
    set_value(remote_receive_timeout_secs_, remote_receive_timeout_secs);
  }

  void set_remote_send_timeout_secs(std::uint16_t remote_send_timeout_secs) {
    set_value(remote_send_timeout_secs_, remote_send_timeout_secs);
  }

  void set_remote_token(const std::string &remote_token) {
    set_value(remote_token_, remote_token);
  }

  void set_retry_read_count(std::uint16_t retry_read_count) {
    set_value(retry_read_count_, retry_read_count);
  }

  [[nodiscard]] auto set_value_by_name(const std::string &name,
                                       const std::string &value) -> std::string;
};
} // namespace repertory

#endif // INCLUDE_APP_CONFIG_HPP_
