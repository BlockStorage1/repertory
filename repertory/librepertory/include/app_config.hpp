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
#ifndef REPERTORY_INCLUDE_APP_CONFIG_HPP_
#define REPERTORY_INCLUDE_APP_CONFIG_HPP_

#include "types/remote.hpp"
#include "types/repertory.hpp"

namespace repertory {
class app_config final {
private:
  static stop_type stop_requested;

public:
  [[nodiscard]] static auto default_agent_name(provider_type prov)
      -> std::string;

  [[nodiscard]] static auto default_api_port(provider_type prov)
      -> std::uint16_t;

  [[nodiscard]] static auto default_data_directory(provider_type prov)
      -> std::string;

  [[nodiscard]] static auto default_remote_api_port(provider_type prov)
      -> std::uint16_t;

  [[nodiscard]] static auto get_provider_display_name(provider_type prov)
      -> std::string;

  [[nodiscard]] static auto get_provider_name(provider_type prov)
      -> std::string;

  [[nodiscard]] static auto get_root_data_directory() -> std::string;

public:
  [[nodiscard]] static auto get_stop_requested() -> bool;

  static void set_stop_requested();

public:
  app_config(provider_type prov, std::string_view data_directory);

  app_config() = delete;
  app_config(app_config &&) = delete;
  app_config(const app_config &) = delete;

  ~app_config() { save(); }

  auto operator=(const app_config &) -> app_config & = delete;
  auto operator=(app_config &&) -> app_config & = delete;

private:
  provider_type prov_;
  utils::atomic<std::string> api_password_;
  std::atomic<std::uint16_t> api_port_;
  utils::atomic<std::string> api_user_;
  std::string cache_directory_;
  std::atomic<bool> config_changed_;
  std::string data_directory_;
  std::atomic<database_type> db_type_{database_type::rocksdb};
  std::atomic<std::uint8_t> download_timeout_secs_;
  std::atomic<bool> enable_download_timeout_;
  std::atomic<bool> enable_drive_events_;
#if defined(_WIN32)
  std::atomic<bool> enable_mount_manager_;
#endif // defined(_WIN32)
  std::atomic<event_level> event_level_;
  std::atomic<std::uint32_t> eviction_delay_mins_;
  std::atomic<bool> eviction_uses_accessed_time_;
  std::atomic<std::uint16_t> high_freq_interval_secs_;
  std::string log_directory_;
  std::atomic<std::uint16_t> low_freq_interval_secs_;
  std::atomic<std::uint64_t> max_cache_size_bytes_;
  std::atomic<std::uint8_t> max_upload_count_;
  std::atomic<std::uint16_t> med_freq_interval_secs_;
  std::atomic<std::uint16_t> online_check_retry_secs_;
  std::atomic<download_type> preferred_download_type_;
  std::atomic<std::uint16_t> retry_read_count_;
  std::atomic<std::uint16_t> ring_buffer_file_size_;
  std::atomic<std::uint16_t> task_wait_ms_;

private:
  utils::atomic<encrypt_config> encrypt_config_;
  utils::atomic<host_config> host_config_;
  mutable std::recursive_mutex read_write_mutex_;
  utils::atomic<remote::remote_config> remote_config_;
  utils::atomic<remote::remote_mount> remote_mount_;
  utils::atomic<s3_config> s3_config_;
  utils::atomic<sia_config> sia_config_;
  std::unordered_map<std::string, std::function<std::string()>>
      value_get_lookup_;
  std::unordered_map<std::string, std::function<std::string(std::string_view)>>
      value_set_lookup_;
  std::uint64_t version_{REPERTORY_CONFIG_VERSION};

private:
  [[nodiscard]] auto load() -> bool;

  template <typename dest, typename source>
  auto set_value(dest &dst, const source &src) -> bool;

  auto set_value(utils::atomic<std::string> &dst, std::string_view src) -> bool;

public:
  [[nodiscard]] auto get_api_password() const -> std::string;

  [[nodiscard]] auto get_api_port() const -> std::uint16_t;

  [[nodiscard]] auto get_api_user() const -> std::string;

  [[nodiscard]] auto get_cache_directory() const -> std::string;

  [[nodiscard]] auto get_config_file_path() const -> std::string;

  [[nodiscard]] auto get_database_type() const -> database_type;

  [[nodiscard]] auto get_data_directory() const -> std::string;

  [[nodiscard]] auto get_download_timeout_secs() const -> std::uint8_t;

  [[nodiscard]] auto get_enable_download_timeout() const -> bool;

  [[nodiscard]] auto get_enable_drive_events() const -> bool;

  [[nodiscard]] auto get_encrypt_config() const -> encrypt_config;

#if defined(_WIN32)
  [[nodiscard]] auto get_enable_mount_manager() const -> bool;
#endif // defined(_WIN32)

  [[nodiscard]] auto get_event_level() const -> event_level;

  [[nodiscard]] auto get_eviction_delay_mins() const -> std::uint32_t;

  [[nodiscard]] auto get_eviction_uses_accessed_time() const -> bool;

  [[nodiscard]] auto get_high_frequency_interval_secs() const -> std::uint16_t;

  [[nodiscard]] auto get_host_config() const -> host_config;

  [[nodiscard]] auto get_json() const -> json;

  [[nodiscard]] auto get_log_directory() const -> std::string;

  [[nodiscard]] auto get_low_frequency_interval_secs() const -> std::uint16_t;

  [[nodiscard]] auto get_max_cache_size_bytes() const -> std::uint64_t;

  [[nodiscard]] auto get_max_upload_count() const -> std::uint8_t;

  [[nodiscard]] auto get_med_frequency_interval_secs() const -> std::uint16_t;

  [[nodiscard]] auto get_online_check_retry_secs() const -> std::uint16_t;

  [[nodiscard]] auto get_preferred_download_type() const -> download_type;

  [[nodiscard]] auto get_provider_type() const -> provider_type;

  [[nodiscard]] auto get_remote_config() const -> remote::remote_config;

  [[nodiscard]] auto get_remote_mount() const -> remote::remote_mount;

  [[nodiscard]] auto get_retry_read_count() const -> std::uint16_t;

  [[nodiscard]] auto get_ring_buffer_file_size() const -> std::uint16_t;

  [[nodiscard]] auto get_s3_config() const -> s3_config;

  [[nodiscard]] auto get_sia_config() const -> sia_config;

  [[nodiscard]] auto get_task_wait_ms() const -> std::uint16_t;

  [[nodiscard]] auto get_value_by_name(std::string_view name) const
      -> std::string;

  [[nodiscard]] auto get_raw_value_by_name(std::string_view name) const
      -> std::string;

  [[nodiscard]] auto get_version() const -> std::uint64_t;

  void save();

  void set_api_password(std::string_view value);

  void set_api_port(std::uint16_t value);

  void set_api_user(std::string_view value);

  void set_download_timeout_secs(std::uint8_t value);

  void set_database_type(const database_type &value);

  void set_enable_download_timeout(bool value);

  void set_enable_drive_events(bool value);

#if defined(_WIN32)
  void set_enable_mount_manager(bool value);
#endif // defined(_WIN32)

  void set_event_level(const event_level &value);

  void set_encrypt_config(encrypt_config value);

  void set_eviction_delay_mins(std::uint32_t value);

  void set_eviction_uses_accessed_time(bool value);

  void set_high_frequency_interval_secs(std::uint16_t value);

  void set_host_config(host_config value);

  void set_low_frequency_interval_secs(std::uint16_t value);

  void set_max_cache_size_bytes(std::uint64_t value);

  void set_max_upload_count(std::uint8_t value);

  void set_med_frequency_interval_secs(std::uint16_t value);

  void set_online_check_retry_secs(std::uint16_t value);

  void set_preferred_download_type(const download_type &value);

  void set_remote_config(remote::remote_config value);

  void set_remote_mount(remote::remote_mount value);

  void set_retry_read_count(std::uint16_t value);

  void set_ring_buffer_file_size(std::uint16_t value);

  void set_s3_config(s3_config value);

  void set_sia_config(sia_config value);

  void set_task_wait_ms(std::uint16_t value);

  [[nodiscard]] auto set_value_by_name(std::string_view name,
                                       std::string_view value) -> std::string;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_APP_CONFIG_HPP_
