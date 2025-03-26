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
#include "types/repertory.hpp"

#include "app_config.hpp"
#include "types/startup_exception.hpp"
#include "utils/string.hpp"

namespace repertory {
void clean_json_config(provider_type prov, nlohmann::json &data) {
  data[JSON_API_PASSWORD] = "";

  switch (prov) {
  case provider_type::encrypt:
    data[JSON_ENCRYPT_CONFIG][JSON_ENCRYPTION_TOKEN] = "";
    data[JSON_REMOTE_MOUNT][JSON_ENCRYPTION_TOKEN] = "";
    break;

  case provider_type::remote:
    data[JSON_REMOTE_CONFIG][JSON_ENCRYPTION_TOKEN] = "";
    break;

  case provider_type::s3:
    data[JSON_REMOTE_MOUNT][JSON_ENCRYPTION_TOKEN] = "";
    data[JSON_S3_CONFIG][JSON_ENCRYPTION_TOKEN] = "";
    data[JSON_S3_CONFIG][JSON_SECRET_KEY] = "";
    break;

  case provider_type::sia:
    data[JSON_REMOTE_MOUNT][JSON_ENCRYPTION_TOKEN] = "";
    data[JSON_HOST_CONFIG][JSON_API_PASSWORD] = "";

    break;
  default:
    return;
  }
}

auto clean_json_value(std::string_view name, std::string_view data)
    -> std::string {
  if (name ==
          fmt::format("{}.{}", JSON_ENCRYPT_CONFIG, JSON_ENCRYPTION_TOKEN) ||
      name == fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_API_PASSWORD) ||
      name == fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_ENCRYPTION_TOKEN) ||
      name == fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_ENCRYPTION_TOKEN) ||
      name == fmt::format("{}.{}", JSON_S3_CONFIG, JSON_ENCRYPTION_TOKEN) ||
      name == fmt::format("{}.{}", JSON_S3_CONFIG, JSON_SECRET_KEY) ||
      name == JSON_API_PASSWORD) {
    return "";
  }

  return std::string{data};
}

auto database_type_from_string(std::string type, database_type default_type)
    -> database_type {
  type = utils::string::to_lower(utils::string::trim(type));
  if (type == "rocksdb") {
    return database_type::rocksdb;
  }

  if (type == "sqlite") {
    return database_type::sqlite;
  }

  return default_type;
}

auto database_type_to_string(const database_type &type) -> std::string {
  switch (type) {
  case database_type::rocksdb:
    return "rocksdb";
  case database_type::sqlite:
    return "sqlite";
  default:
    return "rocksdb";
  }
}

auto download_type_from_string(std::string type, download_type default_type)
    -> download_type {
  type = utils::string::to_lower(utils::string::trim(type));
  if (type == "default") {
    return download_type::default_;
  }

  if (type == "direct") {
    return download_type::direct;
  }

  if (type == "ring_buffer") {
    return download_type::ring_buffer;
  }

  return default_type;
}

auto download_type_to_string(const download_type &type) -> std::string {
  switch (type) {
  case download_type::default_:
    return "default";
  case download_type::direct:
    return "direct";
  case download_type::ring_buffer:
    return "ring_buffer";
  default:
    return "default";
  }
}

auto event_level_from_string(std::string level, event_level default_level)
    -> event_level {
  level = utils::string::to_lower(level);
  if (level == "critical" || level == "event_level::critical") {
    return event_level::critical;
  }

  if (level == "debug" || level == "event_level::debug") {
    return event_level::debug;
  }

  if (level == "warn" || level == "event_level::warn") {
    return event_level::warn;
  }

  if (level == "info" || level == "event_level::info") {
    return event_level::info;
  }

  if (level == "error" || level == "event_level::error") {
    return event_level::error;
  }

  if (level == "trace" || level == "event_level::trace") {
    return event_level::trace;
  }

  return default_level;
}

auto event_level_to_string(event_level level) -> std::string {
  switch (level) {
  case event_level::critical:
    return "critical";
  case event_level::debug:
    return "debug";
  case event_level::error:
    return "error";
  case event_level::info:
    return "info";
  case event_level::warn:
    return "warn";
  case event_level::trace:
    return "trace";
  default:
    return "info";
  }
}
static const std::unordered_map<api_error, std::string> LOOKUP = {
    {api_error::success, "success"},
    {api_error::access_denied, "access_denied"},
    {api_error::bad_address, "bad_address"},
    {api_error::buffer_overflow, "buffer_overflow"},
    {api_error::buffer_too_small, "buffer_too_small"},
    {api_error::cache_not_initialized, "cache_not_initialized"},
    {api_error::comm_error, "comm_error"},
    {api_error::decryption_error, "decryption_error"},
    {api_error::directory_end_of_files, "directory_end_of_files"},
    {api_error::directory_exists, "directory_exists"},
    {api_error::directory_not_empty, "directory_not_empty"},
    {api_error::directory_not_found, "directory_not_found"},
    {api_error::download_failed, "download_failed"},
    {api_error::download_incomplete, "download_incomplete"},
    {api_error::download_stopped, "download_stopped"},
    {api_error::empty_ring_buffer_chunk_size, "empty_ring_buffer_chunk_size"},
    {api_error::empty_ring_buffer_size, "empty_ring_buffer_size"},
    {api_error::error, "error"},
    {api_error::file_in_use, "file_in_use"},
    {api_error::file_size_mismatch, "file_size_mismatch"},
    {api_error::incompatible_version, "incompatible_version"},
    {api_error::invalid_handle, "invalid_handle"},
    {api_error::invalid_operation, "invalid_operation"},
    {api_error::invalid_ring_buffer_multiple, "invalid_ring_buffer_multiple"},
    {api_error::invalid_ring_buffer_position, "invalid_ring_buffer_position"},
    {api_error::invalid_ring_buffer_size, "invalid_ring_buffer_size"},
    {api_error::invalid_version, "invalid_version"},
    {api_error::item_exists, "item_exists"},
    {api_error::item_not_found, "item_not_found"},
    {api_error::more_data, "more_data"},
    {api_error::name_too_long, "name_too_long"},
    {api_error::no_disk_space, "no_disk_space"},
    {api_error::not_implemented, "not_implemented"},
    {api_error::not_supported, "not_supported"},
    {api_error::os_error, "os_error"},
    {api_error::out_of_memory, "out_of_memory"},
    {api_error::permission_denied, "permission_denied"},
    {api_error::upload_failed, "upload_failed"},
    {api_error::xattr_buffer_small, "xattr_buffer_small"},
    {api_error::xattr_exists, "xattr_exists"},
    {api_error::xattr_not_found, "xattr_not_found"},
    {api_error::xattr_too_big, "xattr_too_big"},
};

auto api_error_from_string(std::string_view str) -> api_error {
  if (LOOKUP.size() != static_cast<std::size_t>(api_error::ERROR_COUNT)) {
    throw startup_exception("undefined api_error strings");
  }

  const auto iter = std::ranges::find_if(
      LOOKUP, [&str](auto &&item) -> bool { return item.second == str; });
  return iter == LOOKUP.end() ? api_error::error : iter->first;
}

auto api_error_to_string(const api_error &error) -> const std::string & {
  if (LOOKUP.size() != static_cast<std::size_t>(api_error::ERROR_COUNT)) {
    throw startup_exception("undefined api_error strings");
  }

  return LOOKUP.at(error);
}

auto provider_type_from_string(std::string_view type,
                               provider_type default_type) -> provider_type {
  auto type_lower = utils::string::to_lower(std::string{type});
  if (type_lower == "encrypt") {
    return provider_type::encrypt;
  }

  if (type_lower == "remote") {
    return provider_type::remote;
  }

  if (type_lower == "s3") {
    return provider_type::s3;
  }

  if (type_lower == "sia") {
    return provider_type::sia;
  }

  if (type_lower == "unknown") {
    return provider_type::unknown;
  }

  return default_type;
}

auto provider_type_to_string(provider_type type) -> std::string {
  return app_config::get_provider_name(type);
}
} // namespace repertory
