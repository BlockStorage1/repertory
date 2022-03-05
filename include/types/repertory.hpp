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
#ifndef INCLUDE_TYPES_REPERTORY_HPP_
#define INCLUDE_TYPES_REPERTORY_HPP_

#include "common.hpp"

namespace repertory {
#define META_ACCESSED "accessed"
#define META_ATTRIBUTES "attributes"
#define META_BACKUP "backup"
#define META_CHANGED "changed"
#define META_CREATION "creation"
#define META_ENCRYPTION_TOKEN "token2"
#define META_GID "gid"
#define META_ID "id"
#define META_KEY "key"
#define META_MODE "mode"
#define META_MODIFIED "modified"
#define META_OSXFLAGS "flags"
#define META_PINNED "pinned"
#define META_SIZE "size"
#define META_SOURCE "source"
#define META_UID "uid"
#define META_WRITTEN "written"

const std::vector<std::string> META_USED_NAMES = {
    META_ACCESSED, META_ATTRIBUTES, META_BACKUP, META_CHANGED, META_CREATION, META_ENCRYPTION_TOKEN,
    META_GID,      META_ID,         META_KEY,    META_MODE,    META_MODIFIED, META_OSXFLAGS,
    META_PINNED,   META_SIZE,       META_SOURCE, META_UID,     META_WRITTEN,
};

typedef std::unordered_map<std::string, std::string> api_meta_map;

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
  download_timeout,
  empty_ring_buffer_chunk_size,
  empty_ring_buffer_size,
  error,
  file_exists,
  file_in_use,
  incompatible_version,
  invalid_handle,
  invalid_operation,
  invalid_ring_buffer_multiple,
  invalid_ring_buffer_size,
  invalid_version,
  item_is_file,
  item_not_found,
  not_implemented,
  not_supported,
  os_error,
  permission_denied,
  upload_failed,
  upload_stopped,
  xattr_buffer_small,
  xattr_exists,
  xattr_invalid_namespace,
  xattr_not_found,
  xattr_osx_invalid,
  xattr_too_big,
  ERROR_COUNT
};

const std::string &api_error_to_string(const api_error &error);

enum class download_type { direct, fallback, ring_buffer };

enum class exit_code {
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
};

enum class lock_result {
  success,
  locked,
  failure,
};

enum class provider_type {
  sia,
  remote,
  s3,
  skynet,
  passthrough,
  unknown,
};

#ifdef _WIN32
struct open_file_data {
  void *directory_buffer = nullptr;
};
#else
typedef int open_file_data;
#endif

struct api_file {
  std::string api_path{};
  std::string api_parent{};
  std::uint64_t accessed_date = 0u;
  std::uint64_t changed_date = 0u;
  std::uint64_t created_date = 0u;
  std::string encryption_token{};
  std::uint64_t file_size = 0u;
  std::uint64_t modified_date = 0u;
  bool recoverable = false;
  double redundancy = 0.0;
  std::string source_path{};
};

struct directory_item {
  std::string api_path{};
  std::string api_parent{};
  bool directory = false;
  std::uint64_t size = 0u;
  api_meta_map meta{};
  bool resolved = false;

  static directory_item from_json(const json &item) {
    directory_item ret{};
    ret.api_path = item["path"].get<std::string>();
    ret.api_parent = item["parent"].get<std::string>();
    ret.directory = item["directory"].get<bool>();
    ret.size = item["size"].get<std::uint64_t>();
    ret.meta = item["meta"].get<std::unordered_map<std::string, std::string>>();
    return ret;
  }

  json to_json() const {
    return {{"path", api_path},
            {"parent", api_parent},
            {"size", size},
            {"directory", directory},
            {"meta", meta}};
  }
};

struct filesystem_item {
  std::string api_path{};
  std::string api_parent{};
  bool changed = false;
  bool directory = false;
  std::string encryption_token{};
  OSHandle handle = REPERTORY_INVALID_HANDLE;
  std::shared_ptr<std::recursive_mutex> lock;
  bool meta_changed = false;
  std::unordered_map<std::uint64_t, open_file_data> open_data;
  std::uint64_t size = 0u;
  std::string source_path{};
  bool source_path_changed = false;

  bool is_encrypted() const { return not encryption_token.empty(); }
};

struct host_config {
  std::string agent_string{};
  std::string api_password{};
  std::uint16_t api_port = 0u;
  std::string host_name_or_ip = "localhost";
  std::string path{};
  std::string protocol = "http";
  std::uint32_t timeout_ms = 60000u;
  std::string auth_url{};
  std::string auth_user{};
  std::string auth_password{};

  bool operator==(const host_config &hc) const noexcept {
    if (&hc != this) {
      return agent_string == hc.agent_string && api_password == hc.api_password &&
             api_port == hc.api_port && host_name_or_ip == hc.host_name_or_ip && path == hc.path &&
             protocol == hc.protocol && timeout_ms == hc.timeout_ms && auth_url == hc.auth_url &&
             auth_user == hc.auth_user && auth_password == hc.auth_password;
    }
    return true;
  }

  bool operator!=(const host_config &hc) const noexcept {
    if (&hc != this) {
      return not(hc == *this);
    }
    return false;
  }
};

static void to_json(json &j, const host_config &hc) {
  j = json{{"AgentString", hc.agent_string},
           {"ApiPassword", hc.api_password},
           {"ApiPort", hc.api_port},
           {"AuthPassword", hc.auth_password},
           {"AuthURL", hc.auth_url},
           {"AuthUser", hc.auth_user},
           {"HostNameOrIp", hc.host_name_or_ip},
           {"Path", hc.path},
           {"Protocol", hc.protocol},
           {"TimeoutMs", hc.timeout_ms}};
}

static void from_json(const json &j, host_config &hc) {
  j.at("AgentString").get_to(hc.agent_string);
  j.at("ApiPassword").get_to(hc.api_password);
  j.at("ApiPort").get_to(hc.api_port);
  j.at("AuthPassword").get_to(hc.auth_password);
  j.at("AuthURL").get_to(hc.auth_url);
  j.at("AuthUser").get_to(hc.auth_user);
  j.at("HostNameOrIp").get_to(hc.host_name_or_ip);
  j.at("Path").get_to(hc.path);
  j.at("Protocol").get_to(hc.protocol);
  j.at("TimeoutMs").get_to(hc.timeout_ms);
}

struct http_range {
  std::uint64_t begin = 0u;
  std::uint64_t end = 0u;
};

#if 0
struct PassthroughConfig {
  std::string Location{};
  std::string Name{};
};
#endif

struct s3_config {
  std::string access_key{};
  std::string bucket{};
  std::uint16_t cache_timeout_secs = 60u;
  std::string encryption_token{};
  std::string region = "any";
  std::string secret_key{};
  std::uint32_t timeout_ms = 60000u;
  std::string url;
};

typedef ttmath::Big<1, 30> api_currency;

typedef std::vector<api_file> api_file_list;
typedef std::vector<directory_item> directory_item_list;
typedef ttmath::UInt<64> hastings;
typedef std::unordered_map<std::string, std::string> http_headers;
typedef std::unordered_map<std::string, std::string> http_parameters;
typedef std::vector<http_range> http_ranges;

typedef std::lock_guard<std::mutex> mutex_lock;
typedef std::lock_guard<std::recursive_mutex> recur_mutex_lock;
typedef std::unique_lock<std::mutex> unique_mutex_lock;
typedef std::unique_lock<std::recursive_mutex> unique_recur_mutex_lock;

typedef std::function<void(api_file &file)> api_file_provider_callback;

typedef std::function<void(const std::string &api_path, const std::string &api_parent,
                           const std::string &source, const bool &directory,
                           const std::uint64_t &create_date, const std::uint64_t &access_date,
                           const std::uint64_t &modified_date, const std::uint64_t &changed_date)>
    api_item_added_callback;

typedef std::function<api_error(const std::string &path, const std::size_t &size,
                                const std::uint64_t &offset, std::vector<char> &data,
                                const bool &stop_requested)>
    api_reader_callback;

typedef std::function<void(directory_item &di, const bool &format_path)> meta_provider_callback;
} // namespace repertory

#endif // INCLUDE_TYPES_REPERTORY_HPP_
