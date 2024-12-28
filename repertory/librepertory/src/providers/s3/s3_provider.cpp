/*
  Copyright <2018-2024> <scott.e.graves@protonmail.com>

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
#include "providers/s3/s3_provider.hpp"

#include "app_config.hpp"
#include "comm/i_http_comm.hpp"
#include "events/event_system.hpp"
#include "events/events.hpp"
#include "file_manager/i_file_manager.hpp"
#include "types/repertory.hpp"
#include "types/s3.hpp"
#include "utils/collection.hpp"
#include "utils/config.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/encryption.hpp"
#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/polling.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"

namespace repertory {
s3_provider::s3_provider(app_config &config, i_http_comm &comm)
    : base_provider(config, comm) {}

auto s3_provider::add_if_not_found(
    api_file &file, const std::string &object_name) const -> api_error {
  api_meta_map meta{};
  if (get_item_meta(file.api_path, meta) == api_error::item_not_found) {
    auto err = create_path_directories(
        file.api_parent, utils::path::get_parent_api_path(object_name));
    if (err != api_error::success) {
      return err;
    }

    get_api_item_added()(false, file);
  }

  return api_error::success;
}

auto s3_provider::convert_api_date(std::string_view date) -> std::uint64_t {
  // 2009-10-12T17:50:30.000Z
  auto date_parts = utils::string::split(date, '.', true);
  auto date_time = date_parts.at(0U);
  auto nanos =
      (date_parts.size() <= 1U)
          ? 0U
          : utils::string::to_uint64(
                utils::string::split(date_parts.at(1U), 'Z', true).at(0U)) *
                1000000UL;

  struct tm tm1 {};
#if defined(_WIN32)
  utils::time::strptime(date_time.c_str(), "%Y-%m-%dT%T", &tm1);
  return nanos + utils::time::windows_time_t_to_unix_time(_mkgmtime(&tm1));
#else  // !defined(_WIN32)
  strptime(date_time.c_str(), "%Y-%m-%dT%T", &tm1);
  return nanos + (static_cast<std::uint64_t>(timegm(&tm1)) *
                  utils::time::NANOS_PER_SECOND);
#endif // defined(_WIN32)
}

auto s3_provider::create_directory_impl(const std::string &api_path,
                                        api_meta_map &meta) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  const auto &cfg = get_s3_config();
  auto is_encrypted = not cfg.encryption_token.empty();
  stop_type stop_requested{false};

  if (is_encrypted) {
    std::string encrypted_file_path;
    auto res = get_item_meta(utils::path::get_parent_api_path(api_path),
                             META_KEY, encrypted_file_path);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(function_name, api_path, res,
                                         "failed to create file");
      return res;
    }

    data_buffer result;
    utils::encryption::encrypt_data(
        cfg.encryption_token,
        *(utils::string::split(api_path, '/', false).end() - 1U), result);

    meta[META_KEY] = utils::path::create_api_path(
        utils::path::combine(utils::path::create_api_path(encrypted_file_path),
                             {utils::collection::to_hex_string(result)}));
  }

  auto object_name =
      utils::path::create_api_path(is_encrypted ? meta[META_KEY] : api_path);

  curl::requests::http_put_file put_file{};
  put_file.allow_timeout = true;
  put_file.aws_service = "aws:amz:" + cfg.region + ":s3";
  put_file.path = object_name + '/';

  long response_code{};
  if (not get_comm().make_request(put_file, response_code, stop_requested)) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::comm_error,
                                       "failed to create directory");
    return api_error::comm_error;
  }

  if (response_code != http_error_codes::ok) {
    utils::error::raise_api_path_error(function_name, api_path, response_code,
                                       "failed to create directory");
    return api_error::comm_error;
  }

  return api_error::success;
}

auto s3_provider::create_file_extra(const std::string &api_path,
                                    api_meta_map &meta) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  const auto &cfg = get_s3_config();
  if (not cfg.encryption_token.empty()) {
    std::string encrypted_file_path;
    auto res = get_item_meta(utils::path::get_parent_api_path(api_path),
                             META_KEY, encrypted_file_path);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(function_name, api_path, res,
                                         "failed to create file");
      return res;
    }

    data_buffer result;
    utils::encryption::encrypt_data(
        cfg.encryption_token,
        *(utils::string::split(api_path, '/', false).end() - 1U), result);

    meta[META_KEY] = utils::path::create_api_path(
        utils::path::combine(utils::path::create_api_path(encrypted_file_path),
                             {utils::collection::to_hex_string(result)}));
  }

  return api_error::success;
}

auto s3_provider::create_path_directories(
    const std::string &api_path, const std::string &key) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  if (api_path == "/") {
    return api_error::success;
  }

  const auto &cfg = get_s3_config();
  auto encryption_token = cfg.encryption_token;
  auto is_encrypted = not encryption_token.empty();

  auto path_parts = utils::string::split(api_path, '/', false);
  auto key_parts = utils::string::split(key, '/', false);

  if (is_encrypted && key_parts.size() != path_parts.size()) {
    return api_error::error;
  }

  std::string cur_key{'/'};
  std::string cur_path{'/'};
  for (std::size_t idx = 0U; idx < path_parts.size(); ++idx) {
    if (is_encrypted) {
      cur_key = utils::path::create_api_path(
          utils::path::combine(cur_key, {key_parts.at(idx)}));
    }
    cur_path = utils::path::create_api_path(
        utils::path::combine(cur_path, {path_parts.at(idx)}));

    auto exists{false};
    auto res = is_directory(cur_path, exists);
    if (res != api_error::success) {
      return res;
    }

    if (not exists) {
      curl::requests::http_put_file put_file{};
      put_file.allow_timeout = true;
      put_file.aws_service = "aws:amz:" + cfg.region + ":s3";
      put_file.path = (is_encrypted ? cur_key : cur_path) + '/';

      stop_type stop_requested{false};
      long response_code{};
      if (not get_comm().make_request(put_file, response_code,
                                      stop_requested)) {
        utils::error::raise_api_path_error(function_name, cur_path,
                                           api_error::comm_error,
                                           "failed to create directory object");
        return api_error::comm_error;
      }

      if (response_code != http_error_codes::ok) {
        utils::error::raise_api_path_error(function_name, cur_path,
                                           response_code,
                                           "failed to create directory object");
        return api_error::comm_error;
      }
    }

    api_meta_map meta{};
    res = get_item_meta(cur_path, meta);
    if (res == api_error::item_not_found) {
      auto dir = create_api_file(cur_path, cur_key, 0U,
                                 get_last_modified(true, cur_path));
      get_api_item_added()(true, dir);

      continue;
    }

    if (res != api_error::success) {
      return res;
    }
  }

  return api_error::success;
}

auto s3_provider::decrypt_object_name(std::string &object_name) const
    -> api_error {
  auto parts = utils::string::split(object_name, '/', false);
  for (auto &part : parts) {
    if (not utils::encryption::decrypt_file_name(
            get_s3_config().encryption_token, part)) {
      return api_error::decryption_error;
    }
  }

  object_name = utils::string::join(parts, '/');
  return api_error::success;
}

auto s3_provider::get_directory_item_count(const std::string &api_path) const
    -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    const auto &cfg = get_s3_config();
    auto is_encrypted = not cfg.encryption_token.empty();
    std::string key;
    if (is_encrypted) {
      auto res = get_item_meta(api_path, META_KEY, key);
      if (res != api_error::success) {
        return 0U;
      }
    }

    auto object_name =
        api_path == "/"
            ? ""
            : utils::path::create_api_path(is_encrypted ? key : api_path);

    std::string response_data{};
    long response_code{};
    auto prefix = object_name.empty() ? object_name : object_name + "/";

    auto grab_more{true};
    std::string token{};
    std::uint64_t total_count{};
    while (grab_more) {
      if (not get_object_list(response_data, response_code, "/", prefix,
                              token)) {
        return total_count;
      }

      if (response_code == http_error_codes::not_found) {
        return total_count;
      }

      if (response_code != http_error_codes::ok) {
        return total_count;
      }

      pugi::xml_document doc;
      auto res = doc.load_string(response_data.c_str());
      if (res.status != pugi::xml_parse_status::status_ok) {
        return total_count;
      }

      grab_more = doc.select_node("/ListBucketResult/IsTruncated")
                      .node()
                      .text()
                      .as_bool();
      if (grab_more) {
        token = doc.select_node("/ListBucketResult/NextContinuationToken")
                    .node()
                    .text()
                    .as_string();
      }

      auto node_list =
          doc.select_nodes("/ListBucketResult/CommonPrefixes/Prefix");
      total_count += node_list.size();

      node_list = doc.select_nodes("/ListBucketResult/Contents");
      total_count += node_list.size();
      if (not prefix.empty()) {
        --total_count;
      }
    }

    return total_count;
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "exception occurred");
  }

  return 0U;
}

auto s3_provider::get_directory_items_impl(
    const std::string &api_path, directory_item_list &list) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  const auto &cfg = get_s3_config();
  auto is_encrypted = not cfg.encryption_token.empty();

  auto ret = api_error::success;
  std::string key;
  if (is_encrypted) {
    ret = get_item_meta(api_path, META_KEY, key);
    if (ret != api_error::success) {
      return ret;
    }
  }

  auto object_name =
      api_path == "/"
          ? ""
          : utils::path::create_api_path(is_encrypted ? key : api_path);

  std::string response_data{};
  long response_code{};
  auto prefix = object_name.empty() ? object_name : object_name + "/";

  auto grab_more{true};
  std::string token{};
  while (grab_more) {
    if (not get_object_list(response_data, response_code, "/", prefix, token)) {
      return api_error::comm_error;
    }

    if (response_code == http_error_codes::not_found) {
      return api_error::directory_not_found;
    }

    if (response_code != http_error_codes::ok) {
      utils::error::raise_api_path_error(function_name, api_path, response_code,
                                         "failed to get directory items");
      return api_error::comm_error;
    }

    pugi::xml_document doc;
    auto parse_res = doc.load_string(response_data.c_str());
    if (parse_res.status != pugi::xml_parse_status::status_ok) {
      return api_error::error;
    }

    grab_more = doc.select_node("/ListBucketResult/IsTruncated")
                    .node()
                    .text()
                    .as_bool();
    if (grab_more) {
      token = doc.select_node("/ListBucketResult/NextContinuationToken")
                  .node()
                  .text()
                  .as_string();
    }

    const auto add_directory_item =
        [&](bool directory, const std::string &name,
            std::uint64_t last_modified,
            std::function<std::uint64_t(const directory_item &)> get_size)
        -> api_error {
      auto child_api_path =
          utils::path::create_api_path(utils::path::combine("/", {name}));
      std::string child_object_name;
      if (is_encrypted) {
        child_object_name = child_api_path;
        if (not utils::encryption::decrypt_file_path(cfg.encryption_token,
                                                     child_api_path)) {
          return api_error::decryption_error;
        }
      }

      directory_item dir_item{};
      dir_item.api_path = child_api_path;
      dir_item.api_parent = utils::path::get_parent_api_path(dir_item.api_path);
      dir_item.directory = directory;
      dir_item.size = get_size(dir_item);
      ret = get_item_meta(child_api_path, dir_item.meta);
      if (ret == api_error::item_not_found) {
        if (directory) {
          ret = create_path_directories(child_api_path, child_object_name);
          if (ret != api_error::success) {
            return ret;
          }
        } else {
          auto file = create_api_file(child_api_path, child_object_name,
                                      dir_item.size, last_modified);
          ret = add_if_not_found(file, child_object_name);
          if (ret != api_error::success) {
            return ret;
          }
        }

        ret = get_item_meta(child_api_path, dir_item.meta);
      }

      if (ret != api_error::success) {
        return ret;
      }

      list.push_back(std::move(dir_item));
      return api_error::success;
    };

    auto node_list =
        doc.select_nodes("/ListBucketResult/CommonPrefixes/Prefix");
    for (const auto &node : node_list) {
      add_directory_item(
          true, node.node().text().as_string(), 0U,
          [](const directory_item &) -> std::uint64_t { return 0U; });
    }

    node_list = doc.select_nodes("/ListBucketResult/Contents");
    for (const auto &node : node_list) {
      auto child_object_name = utils::path::create_api_path(
          node.node().select_node("Key").node().text().as_string());
      if (child_object_name == utils::path::create_api_path(prefix)) {
        continue;
      }

      auto size = node.node().select_node("Size").node().text().as_ullong();
      auto last_modified = convert_api_date(
          node.node().select_node("LastModified").node().text().as_string());
      add_directory_item(false, child_object_name, last_modified,
                         [this, &is_encrypted, &size](
                             const directory_item &dir_item) -> std::uint64_t {
                           std::string size_str;
                           if (get_item_meta(dir_item.api_path, META_SIZE,
                                             size_str) == api_error::success) {
                             return utils::string::to_uint64(size_str);
                           }

                           return is_encrypted
                                      ? utils::encryption::encrypting_reader::
                                            calculate_decrypted_size(size)
                                      : size;
                         });
    }
  }

  return ret;
}

auto s3_provider::get_file(const std::string &api_path,
                           api_file &file) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    bool is_encrypted{};
    std::string object_name;
    head_object_result result{};
    auto res =
        get_object_info(false, api_path, is_encrypted, object_name, result);
    if (res != api_error::success) {
      return res;
    }

    file.api_path = api_path;
    file.api_parent = utils::path::get_parent_api_path(api_path);
    file.accessed_date = file.changed_date = file.creation_date =
        file.modified_date = result.last_modified;
    file.key = is_encrypted ? utils::path::create_api_path(object_name) : "";

    std::string size_str;
    if (get_item_meta(file.api_path, META_SIZE, size_str) ==
        api_error::success) {
      file.file_size = utils::string::to_uint64(size_str);
    } else {
      file.file_size =
          is_encrypted
              ? utils::encryption::encrypting_reader::calculate_decrypted_size(
                    result.content_length)
              : result.content_length;
    }

    return add_if_not_found(file, object_name);
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "exception occurred");
  }

  return api_error::error;
}

auto s3_provider::get_file_list(api_file_list &list,
                                std::string &marker) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  std::string response_data;
  long response_code{};
  if (not get_object_list(response_data, response_code, std::nullopt,
                          std::nullopt, marker)) {
    return api_error::comm_error;
  }

  if (response_code != http_error_codes::ok) {
    utils::error::raise_error(function_name, response_code,
                              "failed to get file list");
    return api_error::comm_error;
  }

  pugi::xml_document doc;
  auto res = doc.load_string(response_data.c_str());
  if (res.status != pugi::xml_parse_status::status_ok) {
    utils::error::raise_error(function_name, res.status,
                              "failed to parse xml document");
    return api_error::comm_error;
  }

  auto grab_more =
      doc.select_node("/ListBucketResult/IsTruncated").node().text().as_bool();
  if (grab_more) {
    marker = doc.select_node("/ListBucketResult/NextContinuationToken")
                 .node()
                 .text()
                 .as_string();
  }

  auto node_list = doc.select_nodes("/ListBucketResult/Contents");
  for (const auto &node : node_list) {
    auto object_name =
        std::string{node.node().select_node("Key").node().text().as_string()};
    auto api_path{object_name};
    if (utils::string::ends_with(api_path, "/")) {
      continue;
    }

    auto is_encrypted = not get_s3_config().encryption_token.empty();
    if (is_encrypted) {
      auto err = decrypt_object_name(api_path);
      if (err != api_error::success) {
        return err;
      }
    }

    auto size = node.node().select_node("Size").node().text().as_ullong();

    api_file file{};
    file.api_path = utils::path::create_api_path(api_path);
    file.api_parent = utils::path::get_parent_api_path(file.api_path);
    file.accessed_date = file.changed_date = file.creation_date =
        file.modified_date = convert_api_date(
            node.node().select_node("LastModified").node().text().as_string());
    file.file_size =
        is_encrypted
            ? utils::encryption::encrypting_reader::calculate_decrypted_size(
                  size)
            : size;
    file.key = is_encrypted ? utils::path::create_api_path(object_name) : "";
    auto err = add_if_not_found(file, file.key);
    if (err != api_error::success) {
      return err;
    }

    list.push_back(std::move(file));
  }

  return grab_more ? api_error::more_data : api_error::success;
}

auto s3_provider::get_last_modified(
    bool directory, const std::string &api_path) const -> std::uint64_t {
  bool is_encrypted{};
  std::string object_name;
  head_object_result result{};
  return (get_object_info(directory, api_path, is_encrypted, object_name,
                          result) == api_error::success)
             ? result.last_modified
             : utils::time::get_time_now();
}

auto s3_provider::get_object_info(
    bool directory, const std::string &api_path, bool &is_encrypted,
    std::string &object_name, head_object_result &result) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    const auto &cfg = get_s3_config();
    is_encrypted = not cfg.encryption_token.empty();

    std::string key;
    if (is_encrypted) {
      auto res = get_item_meta(api_path, META_KEY, key);
      if (res != api_error::success) {
        return res;
      }
    }

    object_name = utils::path::create_api_path(is_encrypted ? key : api_path);

    curl::requests::http_head head{};
    head.allow_timeout = true;
    head.aws_service = "aws:amz:" + cfg.region + ":s3";
    head.path = directory ? object_name + '/' : object_name;
    head.response_headers = http_headers{};

    stop_type stop_requested{false};
    long response_code{};
    if (not get_comm().make_request(head, response_code, stop_requested)) {
      return api_error::comm_error;
    }

    if (response_code == http_error_codes::not_found) {
      return api_error::item_not_found;
    }

    if (response_code != http_error_codes::ok) {
      utils::error::raise_api_path_error(function_name, api_path, response_code,
                                         "failed to get object info");
      return api_error::comm_error;
    }

    result.from_headers(head.response_headers.value());

    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "exception occurred");
  }

  return api_error::error;
}

auto s3_provider::get_object_list(
    std::string &response_data, long &response_code,
    std::optional<std::string> delimiter, std::optional<std::string> prefix,
    std::optional<std::string> token) const -> bool {
  curl::requests::http_get get{};
  get.allow_timeout = true;
  get.aws_service = "aws:amz:" + get_s3_config().region + ":s3";
  get.path = '/';
  get.query["list-type"] = "2";
  if (delimiter.has_value() && not delimiter.value().empty()) {
    get.query["delimiter"] = delimiter.value();
  }
  if (prefix.has_value() && not prefix.value().empty()) {
    get.query["prefix"] = prefix.value();
    utils::string::left_trim(get.query["prefix"], '/');
  }
  if (token.has_value() && not token.value().empty()) {
    get.query["continuation-token"] = token.value();
  }
  get.response_handler = [&response_data](const data_buffer &data,
                                          long /*response_code*/) {
    response_data = std::string(data.begin(), data.end());
  };

  stop_type stop_requested{};
  return get_comm().make_request(get, response_code, stop_requested);
}

auto s3_provider::get_total_drive_space() const -> std::uint64_t {
  return std::numeric_limits<std::int64_t>::max() / std::int64_t(2);
}

auto s3_provider::is_directory(const std::string &api_path,
                               bool &exists) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  exists = false;
  if (api_path == "/") {
    exists = true;
    return api_error::success;
  }

  try {
    bool is_encrypted{};
    std::string object_name;
    head_object_result result{};
    auto res =
        get_object_info(true, api_path, is_encrypted, object_name, result);
    if (res != api_error::item_not_found && res != api_error::success) {
      return res;
    }
    exists = res != api_error::item_not_found;
    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "exception occurred");
  }

  return api_error::error;
}

auto s3_provider::is_file(const std::string &api_path,
                          bool &exists) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  exists = false;
  if (api_path == "/") {
    return api_error::success;
  }

  try {
    bool is_encrypted{};
    std::string object_name;
    head_object_result result{};
    auto res =
        get_object_info(false, api_path, is_encrypted, object_name, result);
    if (res != api_error::item_not_found && res != api_error::success) {
      return res;
    }
    exists = res != api_error::item_not_found;
    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "exception occurred");
  }

  return api_error::error;
}

auto s3_provider::is_online() const -> bool {
  // TODO implement this
  return true;
}

auto s3_provider::read_file_bytes(const std::string &api_path, std::size_t size,
                                  std::uint64_t offset, data_buffer &data,
                                  stop_type &stop_requested) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    const auto &cfg = get_s3_config();
    auto is_encrypted = not cfg.encryption_token.empty();
    std::string key;
    if (is_encrypted) {
      auto res = get_item_meta(api_path, META_KEY, key);
      if (res != api_error::success) {
        return res;
      }
    }

    auto object_name =
        utils::path::create_api_path(is_encrypted ? key : api_path);

    const auto read_bytes =
        [this, &api_path, &cfg, &object_name,
         &stop_requested](std::size_t read_size, std::size_t read_offset,
                          data_buffer &read_buffer) -> api_error {
      auto res = api_error::error;
      for (std::uint32_t idx = 0U;
           not stop_requested && res != api_error::success &&
           idx < get_config().get_retry_read_count() + 1U;
           ++idx) {
        curl::requests::http_get get{};
        get.aws_service = "aws:amz:" + cfg.region + ":s3";
        get.headers["response-content-type"] = "binary/octet-stream";
        get.path = object_name;
        get.range = {{
            read_offset,
            read_offset + read_size - 1U,
        }};
        get.response_handler = [&read_buffer](const data_buffer &response_data,
                                              long /*response_code*/) {
          read_buffer = response_data;
        };

        long response_code{};
        const auto notify_retry = [&]() {
          if (response_code == 0) {
            utils::error::raise_api_path_error(
                function_name, api_path, api_error::comm_error,
                "read file bytes failed|offset|" + std::to_string(read_offset) +
                    "|size|" + std::to_string(read_size) + "|retry|" +
                    std::to_string(idx + 1U));
          } else {
            utils::error::raise_api_path_error(
                function_name, api_path, response_code,
                "read file bytes failed|offset|" + std::to_string(read_offset) +
                    "|size|" + std::to_string(read_size) + "|retry|" +
                    std::to_string(idx + 1U));
          }
          std::this_thread::sleep_for(1s);
        };

        if (not get_comm().make_request(get, response_code, stop_requested)) {
          notify_retry();
          continue;
        }

        if (response_code < http_error_codes::ok ||
            response_code >= http_error_codes::multiple_choices) {
          notify_retry();
          continue;
        }

        res = api_error::success;
      }

      return res;
    };

    if (not is_encrypted) {
      return read_bytes(size, offset, data);
    }

    std::string temp;
    auto res = get_item_meta(api_path, META_SIZE, temp);
    if (res != api_error::success) {
      return res;
    }

    auto total_size = utils::string::to_uint64(temp);
    return utils::encryption::read_encrypted_range(
               {offset, offset + size - 1U},
               utils::encryption::generate_key<utils::encryption::hash_256_t>(
                   cfg.encryption_token),
               [&](data_buffer &ct_buffer, std::uint64_t start_offset,
                   std::uint64_t end_offset) -> bool {
                 return read_bytes((end_offset - start_offset + 1U),
                                   start_offset,
                                   ct_buffer) == api_error::success;
               },
               total_size, data)
               ? api_error::success
               : api_error::decryption_error;

  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "exception occurred");
  }

  return api_error::error;
}

auto s3_provider::remove_directory_impl(const std::string &api_path)
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  const auto &cfg = get_s3_config();
  auto is_encrypted = not cfg.encryption_token.empty();

  std::string key;
  if (is_encrypted) {
    auto res = get_item_meta(api_path, META_KEY, key);
    if (res != api_error::success) {
      return res;
    }
  }

  auto object_name =
      utils::path::create_api_path(is_encrypted ? key : api_path);

  curl::requests::http_delete del{};
  del.allow_timeout = true;
  del.aws_service = "aws:amz:" + cfg.region + ":s3";
  del.path = object_name + '/';

  long response_code{};
  stop_type stop_requested{};
  if (not get_comm().make_request(del, response_code, stop_requested)) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::comm_error,
                                       "failed to remove directory");
    return api_error::comm_error;
  }

  if ((response_code < http_error_codes::ok ||
       response_code >= http_error_codes::multiple_choices) &&
      response_code != http_error_codes::not_found) {
    utils::error::raise_api_path_error(function_name, api_path, response_code,
                                       "failed to remove directory");
    return api_error::comm_error;
  }

  return api_error::success;
}

auto s3_provider::remove_file_impl(const std::string &api_path) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  const auto &cfg = get_s3_config();
  auto is_encrypted = not cfg.encryption_token.empty();

  std::string key;
  if (is_encrypted) {
    auto res = get_item_meta(api_path, META_KEY, key);
    if (res != api_error::success) {
      return res;
    }
  }

  auto object_name =
      utils::path::create_api_path(is_encrypted ? key : api_path);

  curl::requests::http_delete del{};
  del.allow_timeout = true;
  del.aws_service = "aws:amz:" + cfg.region + ":s3";
  del.path = object_name;

  long response_code{};
  stop_type stop_requested{};
  if (not get_comm().make_request(del, response_code, stop_requested)) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::comm_error,
                                       "failed to remove file");
    return api_error::comm_error;
  }

  if ((response_code < http_error_codes::ok ||
       response_code >= http_error_codes::multiple_choices) &&
      response_code != http_error_codes::not_found) {
    utils::error::raise_api_path_error(function_name, api_path, response_code,
                                       "failed to remove file");
    return api_error::comm_error;
  }

  return api_error::success;
}

auto s3_provider::rename_file(const std::string & /* from_api_path */,
                              const std::string & /* to_api_path */)
    -> api_error {
  return api_error::not_implemented;
}

auto s3_provider::start(api_item_added_callback api_item_added,
                        i_file_manager *mgr) -> bool {
  event_system::instance().raise<service_started>("s3_provider");
  s3_config_ = get_config().get_s3_config();
  get_comm().enable_s3_path_style(s3_config_.use_path_style);
  return base_provider::start(api_item_added, mgr);
}

void s3_provider::stop() {
  event_system::instance().raise<service_shutdown_begin>("s3_provider");
  base_provider::stop();
  event_system::instance().raise<service_shutdown_end>("s3_provider");
}

auto s3_provider::upload_file_impl(const std::string &api_path,
                                   const std::string &source_path,
                                   stop_type &stop_requested) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  std::uint64_t file_size{};
  if (utils::file::file{source_path}.exists()) {
    auto opt_size = utils::file::file{source_path}.size();
    if (not opt_size.has_value()) {
      return api_error::comm_error;
    }
    file_size = opt_size.value();
  }

  const auto &cfg = get_s3_config();
  auto is_encrypted = not cfg.encryption_token.empty();

  std::string key;
  if (is_encrypted) {
    auto res = get_item_meta(api_path, META_KEY, key);
    if (res != api_error::success) {
      return res;
    }
  }

  auto object_name =
      utils::path::create_api_path(is_encrypted ? key : api_path);

  curl::requests::http_put_file put_file{};
  put_file.aws_service = "aws:amz:" + cfg.region + ":s3";
  put_file.path = object_name;
  put_file.source_path = source_path;

  if (is_encrypted && file_size > 0U) {
    static stop_type no_stop{false};

    put_file.reader = std::make_shared<utils::encryption::encrypting_reader>(
        object_name, source_path, no_stop, cfg.encryption_token, -1);
  }

  long response_code{};
  if (not get_comm().make_request(put_file, response_code, stop_requested)) {
    return api_error::comm_error;
  }

  if (response_code != http_error_codes::ok) {
    utils::error::raise_api_path_error(function_name, api_path, response_code,
                                       "failed to get upload file");
    return api_error::comm_error;
  }

  return api_error::success;
}
} // namespace repertory
