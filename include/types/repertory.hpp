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
#ifndef INCLUDE_TYPES_REPERTORY_HPP_
#define INCLUDE_TYPES_REPERTORY_HPP_

namespace repertory {
const std::string META_ACCESSED = "accessed";
const std::string META_ATTRIBUTES = "attributes";
const std::string META_BACKUP = "backup";
const std::string META_CHANGED = "changed";
const std::string META_CREATION = "creation";
const std::string META_DIRECTORY = "directory";
const std::string META_GID = "gid";
const std::string META_KEY = "key";
const std::string META_MODE = "mode";
const std::string META_MODIFIED = "modified";
const std::string META_OSXFLAGS = "flags";
const std::string META_PINNED = "pinned";
const std::string META_SIZE = "size";
const std::string META_SOURCE = "source";
const std::string META_UID = "uid";
const std::string META_WRITTEN = "written";

const std::vector<std::string> META_USED_NAMES = {
    META_ACCESSED, META_ATTRIBUTES, META_BACKUP,   META_CHANGED,
    META_CREATION, META_DIRECTORY,  META_GID,      META_KEY,
    META_MODE,     META_MODIFIED,   META_OSXFLAGS, META_PINNED,
    META_SIZE,     META_SOURCE,     META_UID,      META_WRITTEN,
};

using api_meta_map = std::map<std::string, std::string>;

using stop_type = std::atomic<bool>;

enum class api_error {
  success = 0,
  access_denied,
  bad_address,
  buffer_overflow,
  buffer_too_small,
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
  invalid_ring_buffer_size,
  invalid_version,
  item_exists,
  item_not_found,
  no_disk_space,
  not_implemented,
  not_supported,
  os_error,
  out_of_memory,
  permission_denied,
  upload_failed,
  upload_stopped,
  xattr_buffer_small,
  xattr_exists,
  xattr_not_found,
  xattr_too_big,
  ERROR_COUNT
};

[[nodiscard]] auto api_error_from_string(std::string_view s) -> api_error;

[[nodiscard]] auto api_error_to_string(const api_error &error)
    -> const std::string &;

enum class download_type { direct, fallback, ring_buffer };

enum class exit_code : std::int32_t {
  success,
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
};

enum http_error_codes : std::int32_t {
  ok = 200,
  multiple_choices = 300,
  not_found = 404,
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

#ifdef _WIN32
struct open_file_data {
  void *directory_buffer{};
};
#else
using open_file_data = int;
#endif

struct api_file {
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

struct directory_item {
  std::string api_path;
  std::string api_parent;
  bool directory{false};
  std::uint64_t size{};
  api_meta_map meta{};
  bool resolved{false};

  [[nodiscard]] static auto from_json(const json &item) -> directory_item {
    directory_item ret{};
    ret.api_path = item["path"].get<std::string>();
    ret.api_parent = item["parent"].get<std::string>();
    ret.directory = item["directory"].get<bool>();
    ret.size = item["size"].get<std::uint64_t>();
    ret.meta = item["meta"].get<api_meta_map>();
    return ret;
  }

  [[nodiscard]] auto to_json() const -> json {
    return {{"path", api_path},
            {"parent", api_parent},
            {"size", size},
            {"directory", directory},
            {"meta", meta}};
  }
};

struct filesystem_item {
  std::string api_path;
  std::string api_parent;
  bool directory{false};
  std::uint64_t size{};
  std::string source_path;
};

struct host_config {
  std::string agent_string;
  std::string api_password;
  std::string api_user;
  std::uint16_t api_port{};
  std::string host_name_or_ip{"localhost"};
  std::string path{};
  std::string protocol{"http"};
  std::uint32_t timeout_ms{60000U};

  auto operator==(const host_config &hc) const noexcept -> bool {
    if (&hc != this) {
      return agent_string == hc.agent_string &&
             api_password == hc.api_password && api_user == hc.api_user &&
             api_port == hc.api_port && host_name_or_ip == hc.host_name_or_ip &&
             path == hc.path && protocol == hc.protocol &&
             timeout_ms == hc.timeout_ms;
    }
    return true;
  }

  auto operator!=(const host_config &hc) const noexcept -> bool {
    if (&hc != this) {
      return not(hc == *this);
    }
    return false;
  }
};

#ifdef __GNUG__
__attribute__((unused))
#endif
static void
to_json(json &j, const host_config &hc) {
  j = json{{"AgentString", hc.agent_string},
           {"ApiPassword", hc.api_password},
           {"ApiPort", hc.api_port},
           {"ApiUser", hc.api_user},
           {"HostNameOrIp", hc.host_name_or_ip},
           {"Path", hc.path},
           {"Protocol", hc.protocol},
           {"TimeoutMs", hc.timeout_ms}};
}

#ifdef __GNUG__
__attribute__((unused))
#endif
static void
from_json(const json &j, host_config &hc) {
  j.at("AgentString").get_to(hc.agent_string);
  j.at("ApiPassword").get_to(hc.api_password);
  j.at("ApiPort").get_to(hc.api_port);
  j.at("AuthUser").get_to(hc.api_user);
  j.at("HostNameOrIp").get_to(hc.host_name_or_ip);
  j.at("Path").get_to(hc.path);
  j.at("Protocol").get_to(hc.protocol);
  j.at("TimeoutMs").get_to(hc.timeout_ms);
}

struct http_range {
  std::uint64_t begin;
  std::uint64_t end;
};

struct encrypt_config {
  std::string encryption_token;
  std::string path;
};

struct s3_config {
  std::string access_key;
  std::string bucket;
  std::uint16_t cache_timeout_secs{60U};
  std::string encryption_token;
  std::string region = "any";
  std::string secret_key;
  std::uint32_t timeout_ms{60000U};
  std::string url;
  bool use_path_style{false};
  bool use_region_in_url{false};
};

using data_buffer = std::vector<char>;

using api_file_list = std::vector<api_file>;
using api_file_provider_callback = std::function<void(api_file &)>;
using api_item_added_callback = std::function<api_error(bool, api_file &)>;
using directory_item_list = std::vector<directory_item>;
using http_headers = std::unordered_map<std::string, std::string>;
using http_parameters = std::unordered_map<std::string, std::string>;
using http_ranges = std::vector<http_range>;
using meta_provider_callback = std::function<void(directory_item &)>;
using mutex_lock = std::lock_guard<std::mutex>;
using query_parameters = std::map<std::string, std::string>;
using recur_mutex_lock = std::lock_guard<std::recursive_mutex>;
using unique_mutex_lock = std::unique_lock<std::mutex>;
using unique_recur_mutex_lock = std::unique_lock<std::recursive_mutex>;
} // namespace repertory

#endif // INCLUDE_TYPES_REPERTORY_HPP_
