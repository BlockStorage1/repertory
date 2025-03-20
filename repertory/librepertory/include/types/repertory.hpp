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
#ifndef REPERTORY_INCLUDE_TYPES_REPERTORY_HPP_
#define REPERTORY_INCLUDE_TYPES_REPERTORY_HPP_

namespace repertory {
constexpr const auto default_api_password_size{48U};
constexpr const auto default_download_timeout_secs{30U};
constexpr const auto default_eviction_delay_mins{1U};
constexpr const auto default_high_freq_interval_secs{std::uint16_t{30U}};
constexpr const auto default_low_freq_interval_secs{std::uint16_t(60U * 60U)};
constexpr const auto default_max_cache_size_bytes{
    std::uint64_t(20ULL * 1024ULL * 1024ULL * 1024ULL),
};
constexpr const auto default_max_upload_count{5U};
constexpr const auto default_med_freq_interval_secs{std::uint16_t{2U * 60U}};
constexpr const auto default_online_check_retry_secs{60U};
constexpr const auto default_retry_read_count{6U};
constexpr const auto default_ring_buffer_file_size{512U};
constexpr const auto default_task_wait_ms{100U};
constexpr const auto default_timeout_ms{60000U};
constexpr const auto default_ui_mgmt_port{std::uint16_t{30000U}};
constexpr const auto max_ring_buffer_file_size{std::uint16_t(1024U)};
constexpr const auto max_s3_object_name_length{1024U};
constexpr const auto min_cache_size_bytes{
    std::uint64_t(100ULL * 1024ULL * 1024ULL),
};
constexpr const auto min_download_timeout_secs{std::uint8_t(5U)};
constexpr const auto min_online_check_retry_secs{std::uint16_t(15U)};
constexpr const auto min_retry_read_count{std::uint16_t(2U)};
constexpr const auto min_ring_buffer_file_size{std::uint16_t(64U)};
constexpr const auto min_task_wait_ms{std::uint16_t(50U)};

template <typename data_t> class atomic final {
public:
  atomic() : mtx_(std::make_shared<std::mutex>()) {}

  atomic(const atomic &at_data)
      : data_(at_data.load()), mtx_(std::make_shared<std::mutex>()) {}

  atomic(data_t data)
      : data_(std::move(data)), mtx_(std::make_shared<std::mutex>()) {}

  atomic(atomic &&) = default;

  ~atomic() = default;

private:
  data_t data_;
  std::shared_ptr<std::mutex> mtx_;

public:
  [[nodiscard]] auto load() const -> data_t {
    mutex_lock lock(*mtx_);
    return data_;
  }

  auto store(data_t data) -> data_t {
    mutex_lock lock(*mtx_);
    data_ = std::move(data);
    return data_;
  }

  auto operator=(const atomic &at_data) -> atomic & {
    if (&at_data == this) {
      return *this;
    }

    store(at_data.load());
    return *this;
  }

  auto operator=(atomic &&) -> atomic & = default;

  auto operator=(data_t data) -> atomic & {
    if (&data == &data_) {
      return *this;
    }

    store(std::move(data));
    return *this;
  }

  [[nodiscard]] auto operator==(const atomic &at_data) const -> bool {
    if (&at_data == this) {
      return true;
    }

    mutex_lock lock(*mtx_);
    return at_data.load() == data_;
  }

  [[nodiscard]] auto operator==(const data_t &data) const -> bool {
    if (&data == &data_) {
      return true;
    }

    mutex_lock lock(*mtx_);
    return data == data_;
  }

  [[nodiscard]] auto operator!=(const atomic &at_data) const -> bool {
    if (&at_data == this) {
      return false;
    }

    mutex_lock lock(*mtx_);
    return at_data.load() != data_;
  }

  [[nodiscard]] auto operator!=(const data_t &data) const -> bool {
    if (&data == &data_) {
      return false;
    }

    mutex_lock lock(*mtx_);
    return data != data_;
  }

  [[nodiscard]] operator data_t() const { return load(); }
};

inline constexpr const auto max_time{
    std::numeric_limits<std::uint64_t>::max(),
};

inline constexpr const std::string META_ACCESSED{"accessed"};
inline constexpr const std::string META_ATTRIBUTES{"attributes"};
inline constexpr const std::string META_BACKUP{"backup"};
inline constexpr const std::string META_CHANGED{"changed"};
inline constexpr const std::string META_CREATION{"creation"};
inline constexpr const std::string META_DIRECTORY{"directory"};
inline constexpr const std::string META_GID{"gid"};
inline constexpr const std::string META_KEY{"key"};
inline constexpr const std::string META_MODE{"mode"};
inline constexpr const std::string META_MODIFIED{"modified"};
inline constexpr const std::string META_OSXFLAGS{"flags"};
inline constexpr const std::string META_PINNED{"pinned"};
inline constexpr const std::string META_SIZE{"size"};
inline constexpr const std::string META_SOURCE{"source"};
inline constexpr const std::string META_UID{"uid"};
inline constexpr const std::string META_WRITTEN{"written"};

inline constexpr const std::array<std::string, 16U> META_USED_NAMES = {
    META_ACCESSED, META_ATTRIBUTES, META_BACKUP,   META_CHANGED,
    META_CREATION, META_DIRECTORY,  META_GID,      META_KEY,
    META_MODE,     META_MODIFIED,   META_OSXFLAGS, META_PINNED,
    META_SIZE,     META_SOURCE,     META_UID,      META_WRITTEN,
};

using api_meta_map = std::map<std::string, std::string>;

enum class api_error {
  success = 0,
  access_denied,
  bad_address,
  buffer_overflow,
  buffer_too_small,
  cache_not_initialized,
  comm_error,
  decryption_error,
  directory_end_of_files,
  directory_exists,
  directory_not_empty,
  directory_not_found,
  download_failed,
  download_incomplete,
  download_stopped,
  empty_ring_buffer_chunk_size,
  empty_ring_buffer_size,
  error,
  file_in_use,
  file_size_mismatch,
  incompatible_version,
  invalid_handle,
  invalid_operation,
  invalid_ring_buffer_multiple,
  invalid_ring_buffer_position,
  invalid_ring_buffer_size,
  invalid_version,
  item_exists,
  item_not_found,
  more_data,
  name_too_long,
  no_disk_space,
  not_implemented,
  not_supported,
  os_error,
  out_of_memory,
  permission_denied,
  upload_failed,
  xattr_buffer_small,
  xattr_exists,
  xattr_not_found,
  xattr_too_big,
  ERROR_COUNT
};

[[nodiscard]] auto api_error_from_string(std::string_view str) -> api_error;

[[nodiscard]] auto api_error_to_string(const api_error &error)
    -> const std::string &;

enum class database_type {
  rocksdb,
  sqlite,
};
[[nodiscard]] auto
database_type_from_string(std::string type,
                          database_type default_type = database_type::rocksdb)
    -> database_type;

[[nodiscard]] auto database_type_to_string(const database_type &type)
    -> std::string;

enum class download_type {
  default_,
  direct,
  ring_buffer,
};
[[nodiscard]] auto
download_type_from_string(std::string type,
                          download_type default_type = download_type::default_)
    -> download_type;

[[nodiscard]] auto download_type_to_string(const download_type &type)
    -> std::string;

enum class event_level {
  critical,
  error,
  warn,
  info,
  debug,
  trace,
};

[[nodiscard]] auto
event_level_from_string(std::string level,
                        event_level default_level = event_level::info)
    -> event_level;

[[nodiscard]] auto event_level_to_string(event_level level) -> std::string;

enum class exit_code : std::int32_t {
  success = 0,
  communication_error = -1,
  file_creation_failed = -2,
  incompatible_version = -3,
  invalid_syntax = -4,
  lock_failed = -5,
  mount_active = -6,
  mount_result = -7,
  not_mounted = -8,
  startup_exception = -9,
  failed_to_get_mount_state = -10,
  export_failed = -11,
  import_failed = -12,
  option_not_found = -13,
  invalid_provider_type = -14,
  set_option_not_found = -15,
  pin_failed = -16,
  unpin_failed = -17,
  init_failed = -18,
  ui_mount_failed = -19,
  exception = -20,
};

enum http_error_codes : std::int32_t {
  ok = 200,
  multiple_choices = 300,
  unauthorized = 401,
  not_found = 404,
  internal_error = 500,
};

enum class lock_result {
  success,
  locked,
  failure,
};

enum class provider_type : std::size_t {
  sia,
  remote,
  s3,
  encrypt,
  unknown,
};

[[nodiscard]] auto
provider_type_from_string(std::string_view type,
                          provider_type default_type = provider_type::unknown)
    -> provider_type;

[[nodiscard]] auto provider_type_to_string(provider_type type) -> std::string;

void clean_json_config(provider_type prov, nlohmann::json &data);

[[nodiscard]] auto clean_json_value(std::string_view name,
                                    std::string_view data) -> std::string;

#if defined(_WIN32)
struct open_file_data final {
  PVOID directory_buffer{nullptr};
};
#else
using open_file_data = int;
#endif

struct api_file final {
  std::string api_path;
  std::string api_parent;
  std::uint64_t accessed_date{};
  std::uint64_t changed_date{};
  std::uint64_t creation_date{};
  std::uint64_t file_size{};
  std::string key;
  std::uint64_t modified_date{};
  std::string source_path;
};

struct directory_item final {
  std::string api_path;
  std::string api_parent;
  bool directory{false};
  std::uint64_t size{};
  api_meta_map meta;
};

struct encrypt_config final {
  std::string encryption_token;
  std::string path;

  auto operator==(const encrypt_config &cfg) const noexcept -> bool {
    if (&cfg != this) {
      return encryption_token == cfg.encryption_token && path == cfg.path;
    }

    return true;
  }

  auto operator!=(const encrypt_config &cfg) const noexcept -> bool {
    if (&cfg != this) {
      return not(cfg == *this);
    }

    return false;
  }
};

struct filesystem_item final {
  std::string api_path;
  std::string api_parent;
  bool directory{false};
  std::uint64_t size{};
  std::string source_path;
};

struct host_config final {
  std::string agent_string;
  std::string api_password;
  std::string api_user;
  std::uint16_t api_port;
  std::string host_name_or_ip{"localhost"};
  std::string path;
  std::string protocol{"http"};
  std::uint32_t timeout_ms{default_timeout_ms};

  auto operator==(const host_config &cfg) const noexcept -> bool {
    if (&cfg != this) {
      return agent_string == cfg.agent_string &&
             api_password == cfg.api_password && api_user == cfg.api_user &&
             api_port == cfg.api_port &&
             host_name_or_ip == cfg.host_name_or_ip && path == cfg.path &&
             protocol == cfg.protocol && timeout_ms == cfg.timeout_ms;
    }

    return true;
  }

  auto operator!=(const host_config &cfg) const noexcept -> bool {
    if (&cfg != this) {
      return not(cfg == *this);
    }

    return false;
  }
};

struct s3_config final {
  std::string access_key;
  std::string bucket;
  std::string encryption_token;
  std::string region{"any"};
  std::string secret_key;
  std::uint32_t timeout_ms{default_timeout_ms};
  std::string url;
  bool use_path_style{false};
  bool use_region_in_url{false};

  auto operator==(const s3_config &cfg) const noexcept -> bool {
    if (&cfg != this) {
      return access_key == cfg.access_key && bucket == cfg.bucket &&
             encryption_token == cfg.encryption_token && region == cfg.region &&
             secret_key == cfg.secret_key && timeout_ms == cfg.timeout_ms &&
             url == cfg.url && use_path_style == cfg.use_path_style &&
             use_region_in_url == cfg.use_region_in_url;
    }

    return true;
  }

  auto operator!=(const s3_config &cfg) const noexcept -> bool {
    if (&cfg != this) {
      return not(cfg == *this);
    }

    return false;
  }
};

struct sia_config final {
  std::string bucket;

  auto operator==(const sia_config &cfg) const noexcept -> bool {
    if (&cfg != this) {
      return bucket == cfg.bucket;
    }

    return true;
  }

  auto operator!=(const sia_config &cfg) const noexcept -> bool {
    if (&cfg != this) {
      return not(cfg == *this);
    }

    return false;
  }
};

using api_file_list = std::vector<api_file>;
using api_file_provider_callback = std::function<void(api_file &)>;
using api_item_added_callback = std::function<api_error(bool, api_file &)>;
using directory_item_list = std::vector<directory_item>;
using meta_provider_callback = std::function<void(directory_item &)>;

inline constexpr const auto JSON_ACCESS_KEY{"AccessKey"};
inline constexpr const auto JSON_AGENT_STRING{"AgentString"};
inline constexpr const auto JSON_API_PARENT{"ApiParent"};
inline constexpr const auto JSON_API_PASSWORD{"ApiPassword"};
inline constexpr const auto JSON_API_PATH{"ApiPath"};
inline constexpr const auto JSON_API_PORT{"ApiPort"};
inline constexpr const auto JSON_API_USER{"ApiUser"};
inline constexpr const auto JSON_BUCKET{"Bucket"};
inline constexpr const auto JSON_CLIENT_POOL_SIZE{"ClientPoolSize"};
inline constexpr const auto JSON_DATABASE_TYPE{"DatabaseType"};
inline constexpr const auto JSON_DIRECTORY{"Directory"};
inline constexpr const auto JSON_DOWNLOAD_TIMEOUT_SECS{
    "DownloadTimeoutSeconds"};
inline constexpr const auto JSON_ENABLE_DRIVE_EVENTS{"EnableDriveEvents"};
inline constexpr const auto JSON_ENABLE_DOWNLOAD_TIMEOUT{
    "EnableDownloadTimeout"};
inline constexpr const auto JSON_ENABLE_MOUNT_MANAGER{"EnableMountManager"};
inline constexpr const auto JSON_ENABLE_REMOTE_MOUNT{"Enable"};
inline constexpr const auto JSON_ENCRYPTION_TOKEN{"EncryptionToken"};
inline constexpr const auto JSON_ENCRYPT_CONFIG{"EncryptConfig"};
inline constexpr const auto JSON_EVENT_LEVEL{"EventLevel"};
inline constexpr const auto JSON_EVICTION_DELAY_MINS{"EvictionDelayMinutes"};
inline constexpr const auto JSON_EVICTION_USE_ACCESS_TIME{
    "EvictionUseAccessedTime"};
inline constexpr const auto JSON_HIGH_FREQ_INTERVAL_SECS{
    "HighFreqIntervalSeconds"};
inline constexpr const auto JSON_HOST_CONFIG{"HostConfig"};
inline constexpr const auto JSON_HOST_NAME_OR_IP{"HostNameOrIp"};
inline constexpr const auto JSON_LOW_FREQ_INTERVAL_SECS{
    "LowFreqIntervalSeconds"};
inline constexpr const auto JSON_MAX_CACHE_SIZE_BYTES{"MaxCacheSizeBytes"};
inline constexpr const auto JSON_MAX_CONNECTIONS{"MaxConnections"};
inline constexpr const auto JSON_MAX_UPLOAD_COUNT{"MaxUploadCount"};
inline constexpr const auto JSON_MED_FREQ_INTERVAL_SECS{
    "MedFreqIntervalSeconds"};
inline constexpr const auto JSON_META{"Meta"};
inline constexpr const auto JSON_MOUNT_LOCATIONS{"MountLocations"};
inline constexpr const auto JSON_ONLINE_CHECK_RETRY_SECS{
    "OnlineCheckRetrySeconds"};
inline constexpr const auto JSON_PATH{"Path"};
inline constexpr const auto JSON_PREFERRED_DOWNLOAD_TYPE{
    "PreferredDownloadType"};
inline constexpr const auto JSON_PROTOCOL{"Protocol"};
inline constexpr const auto JSON_RECV_TIMEOUT_MS{"ReceiveTimeoutMs"};
inline constexpr const auto JSON_REGION{"Region"};
inline constexpr const auto JSON_REMOTE_CONFIG{"RemoteConfig"};
inline constexpr const auto JSON_REMOTE_MOUNT{"RemoteMount"};
inline constexpr const auto JSON_RETRY_READ_COUNT{"RetryReadCount"};
inline constexpr const auto JSON_RING_BUFFER_FILE_SIZE{"RingBufferFileSize"};
inline constexpr const auto JSON_S3_CONFIG{"S3Config"};
inline constexpr const auto JSON_SECRET_KEY{"SecretKey"};
inline constexpr const auto JSON_SEND_TIMEOUT_MS{"SendTimeoutMs"};
inline constexpr const auto JSON_SIA_CONFIG{"SiaConfig"};
inline constexpr const auto JSON_SIZE{"Size"};
inline constexpr const auto JSON_TASK_WAIT_MS{"TaskWaitMs"};
inline constexpr const auto JSON_TIMEOUT_MS{"TimeoutMs"};
inline constexpr const auto JSON_URL{"URL"};
inline constexpr const auto JSON_USE_PATH_STYLE{"UsePathStyle"};
inline constexpr const auto JSON_USE_REGION_IN_URL{"UseRegionInURL"};
inline constexpr const auto JSON_VERSION{"Version"};
} // namespace repertory

NLOHMANN_JSON_NAMESPACE_BEGIN
template <> struct adl_serializer<repertory::directory_item> {
  static void to_json(json &data, const repertory::directory_item &value) {
    data[repertory::JSON_API_PARENT] = value.api_parent;
    data[repertory::JSON_API_PATH] = value.api_path;
    data[repertory::JSON_DIRECTORY] = value.directory;
    data[repertory::JSON_META] = value.meta;
    data[repertory::JSON_SIZE] = value.size;
  }

  static void from_json(const json &data, repertory::directory_item &value) {
    data.at(repertory::JSON_API_PARENT).get_to<std::string>(value.api_parent);
    data.at(repertory::JSON_API_PATH).get_to<std::string>(value.api_path);
    data.at(repertory::JSON_DIRECTORY).get_to<bool>(value.directory);
    data.at(repertory::JSON_META).get_to<repertory::api_meta_map>(value.meta);
    data.at(repertory::JSON_SIZE).get_to<std::uint64_t>(value.size);
  }
};

template <> struct adl_serializer<repertory::encrypt_config> {
  static void to_json(json &data, const repertory::encrypt_config &value) {
    data[repertory::JSON_ENCRYPTION_TOKEN] = value.encryption_token;
    data[repertory::JSON_PATH] = value.path;
  }

  static void from_json(const json &data, repertory::encrypt_config &value) {
    data.at(repertory::JSON_ENCRYPTION_TOKEN).get_to(value.encryption_token);
    data.at(repertory::JSON_PATH).get_to(value.path);
  }
};

template <> struct adl_serializer<repertory::host_config> {
  static void to_json(json &data, const repertory::host_config &value) {
    data[repertory::JSON_AGENT_STRING] = value.agent_string;
    data[repertory::JSON_API_PASSWORD] = value.api_password;
    data[repertory::JSON_API_PORT] = value.api_port;
    data[repertory::JSON_API_USER] = value.api_user;
    data[repertory::JSON_HOST_NAME_OR_IP] = value.host_name_or_ip;
    data[repertory::JSON_PATH] = value.path;
    data[repertory::JSON_PROTOCOL] = value.protocol;
    data[repertory::JSON_TIMEOUT_MS] = value.timeout_ms;
  }

  static void from_json(const json &data, repertory::host_config &value) {
    data.at(repertory::JSON_AGENT_STRING).get_to(value.agent_string);
    data.at(repertory::JSON_API_PASSWORD).get_to(value.api_password);
    data.at(repertory::JSON_API_PORT).get_to(value.api_port);
    data.at(repertory::JSON_API_USER).get_to(value.api_user);
    data.at(repertory::JSON_HOST_NAME_OR_IP).get_to(value.host_name_or_ip);
    data.at(repertory::JSON_PATH).get_to(value.path);
    data.at(repertory::JSON_PROTOCOL).get_to(value.protocol);
    data.at(repertory::JSON_TIMEOUT_MS).get_to(value.timeout_ms);
  }
};

template <> struct adl_serializer<repertory::s3_config> {
  static void to_json(json &data, const repertory::s3_config &value) {
    data[repertory::JSON_ACCESS_KEY] = value.access_key;
    data[repertory::JSON_BUCKET] = value.bucket;
    data[repertory::JSON_ENCRYPTION_TOKEN] = value.encryption_token;
    data[repertory::JSON_REGION] = value.region;
    data[repertory::JSON_SECRET_KEY] = value.secret_key;
    data[repertory::JSON_TIMEOUT_MS] = value.timeout_ms;
    data[repertory::JSON_URL] = value.url;
    data[repertory::JSON_USE_PATH_STYLE] = value.use_path_style;
    data[repertory::JSON_USE_REGION_IN_URL] = value.use_region_in_url;
  }

  static void from_json(const json &data, repertory::s3_config &value) {
    data.at(repertory::JSON_ACCESS_KEY).get_to(value.access_key);
    data.at(repertory::JSON_BUCKET).get_to(value.bucket);
    data.at(repertory::JSON_ENCRYPTION_TOKEN).get_to(value.encryption_token);
    data.at(repertory::JSON_REGION).get_to(value.region);
    data.at(repertory::JSON_SECRET_KEY).get_to(value.secret_key);
    data.at(repertory::JSON_TIMEOUT_MS).get_to(value.timeout_ms);
    data.at(repertory::JSON_URL).get_to(value.url);
    data.at(repertory::JSON_USE_PATH_STYLE).get_to(value.use_path_style);
    data.at(repertory::JSON_USE_REGION_IN_URL).get_to(value.use_region_in_url);
  }
};

template <> struct adl_serializer<repertory::sia_config> {
  static void to_json(json &data, const repertory::sia_config &value) {
    data[repertory::JSON_BUCKET] = value.bucket;
  }

  static void from_json(const json &data, repertory::sia_config &value) {
    data.at(repertory::JSON_BUCKET).get_to(value.bucket);
  }
};

template <typename data_t> struct adl_serializer<repertory::atomic<data_t>> {
  static void to_json(json &data, const repertory::atomic<data_t> &value) {
    data = value.load();
  }

  static void from_json(const json &data, repertory::atomic<data_t> &value) {
    value.store(data.get<data_t>());
  }
};

template <> struct adl_serializer<std::atomic<std::uint64_t>> {
  static void to_json(json &data, const std::atomic<std::uint64_t> &value) {
    data = value.load();
  }

  static void from_json(const json &data, std::atomic<std::uint64_t> &value) {
    value.store(data.get<std::uint64_t>());
  }
};

template <typename primitive_t>
struct adl_serializer<std::atomic<primitive_t>> {
  static void to_json(json &data, const std::atomic<primitive_t> &value) {
    data = value.load();
  }

  static void from_json(const json &data, std::atomic<primitive_t> &value) {
    value.store(data.get<primitive_t>());
  }
};

template <> struct adl_serializer<std::atomic<repertory::database_type>> {
  static void to_json(json &data,
                      const std::atomic<repertory::database_type> &value) {
    data = repertory::database_type_to_string(value.load());
  }

  static void from_json(const json &data,
                        std::atomic<repertory::database_type> &value) {
    value.store(repertory::database_type_from_string(data.get<std::string>()));
  }
};

template <> struct adl_serializer<std::atomic<repertory::event_level>> {
  static void to_json(json &data,
                      const std::atomic<repertory::event_level> &value) {
    data = repertory::event_level_to_string(value.load());
  }

  static void from_json(const json &data,
                        std::atomic<repertory::event_level> &value) {
    value.store(repertory::event_level_from_string(data.get<std::string>()));
  }
};

template <> struct adl_serializer<std::atomic<repertory::download_type>> {
  static void to_json(json &data,
                      const std::atomic<repertory::download_type> &value) {
    data = repertory::download_type_to_string(value.load());
  }

  static void from_json(const json &data,
                        std::atomic<repertory::download_type> &value) {
    value.store(repertory::download_type_from_string(data.get<std::string>()));
  }
};

template <> struct adl_serializer<repertory::database_type> {
  static void to_json(json &data, const repertory::database_type &value) {
    data = repertory::database_type_to_string(value);
  }

  static void from_json(const json &data, repertory::database_type &value) {
    value = repertory::database_type_from_string(data.get<std::string>());
  }
};

template <> struct adl_serializer<repertory::download_type> {
  static void to_json(json &data, const repertory::download_type &value) {
    data = repertory::download_type_to_string(value);
  }

  static void from_json(const json &data, repertory::download_type &value) {
    value = repertory::download_type_from_string(data.get<std::string>());
  }
};

template <> struct adl_serializer<repertory::event_level> {
  static void to_json(json &data, const repertory::event_level &value) {
    data = repertory::event_level_to_string(value);
  }

  static void from_json(const json &data, repertory::event_level &value) {
    value = repertory::event_level_from_string(data.get<std::string>());
  }
};
NLOHMANN_JSON_NAMESPACE_END

#endif // REPERTORY_INCLUDE_TYPES_REPERTORY_HPP_
