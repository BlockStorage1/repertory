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
#if defined(REPERTORY_ENABLE_S3)

#include "providers/s3/s3_provider.hpp"

#include "app_config.hpp"
#include "comm/curl/curl_comm.hpp"
#include "comm/i_http_comm.hpp"
#include "file_manager/i_file_manager.hpp"
#include "types/repertory.hpp"
#include "types/s3.hpp"
#include "types/startup_exception.hpp"
#include "utils/encryption.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/polling.hpp"
#include "utils/string_utils.hpp"

namespace repertory {
s3_provider::s3_provider(app_config &config, i_http_comm &comm)
    : base_provider(config, comm) {
  get_comm().enable_s3_path_style(config.get_s3_config().use_path_style);
}

auto s3_provider::add_if_not_found(api_file &file,
                                   const std::string &object_name) const
    -> api_error {
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

auto s3_provider::create_directory_impl(const std::string &api_path,
                                        api_meta_map &meta) -> api_error {
  const auto cfg = get_config().get_s3_config();
  const auto is_encrypted = not cfg.encryption_token.empty();
  stop_type stop_requested{false};

  if (is_encrypted) {
    std::string encrypted_file_path;
    auto res = get_item_meta(utils::path::get_parent_api_path(api_path),
                             META_KEY, encrypted_file_path);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                         "failed to create file");
      return res;
    }

    data_buffer result;
    utils::encryption::encrypt_data(
        cfg.encryption_token,
        *(utils::string::split(api_path, '/', false).end() - 1U), result);

    meta[META_KEY] = utils::path::create_api_path(
        utils::path::combine(utils::path::create_api_path(encrypted_file_path),
                             {utils::to_hex_string(result)}));
  }

  const auto object_name =
      utils::path::create_api_path(is_encrypted ? meta[META_KEY] : api_path);

  curl::requests::http_put_file put_file{};
  put_file.allow_timeout = true;
  put_file.aws_service = "aws:amz:" + cfg.region + ":s3";
  put_file.path = object_name + '/';

  long response_code{};
  if (not get_comm().make_request(put_file, response_code, stop_requested)) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       api_error::comm_error,
                                       "failed to create directory");
    return api_error::comm_error;
  }

  if (response_code != http_error_codes::ok) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, response_code,
                                       "failed to create directory");
    return api_error::comm_error;
  }

  return api_error::success;
}

auto s3_provider::create_file_extra(const std::string &api_path,
                                    api_meta_map &meta) -> api_error {
  if (not get_config().get_s3_config().encryption_token.empty()) {
    std::string encrypted_file_path;
    auto res = get_item_meta(utils::path::get_parent_api_path(api_path),
                             META_KEY, encrypted_file_path);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                         "failed to create file");
      return res;
    }

    data_buffer result;
    utils::encryption::encrypt_data(
        get_config().get_s3_config().encryption_token,
        *(utils::string::split(api_path, '/', false).end() - 1U), result);

    meta[META_KEY] = utils::path::create_api_path(
        utils::path::combine(utils::path::create_api_path(encrypted_file_path),
                             {utils::to_hex_string(result)}));
  }

  return api_error::success;
}

auto s3_provider::create_path_directories(const std::string &api_path,
                                          const std::string &key) const
    -> api_error {
  if (api_path == "/") {
    return api_error::success;
  }

  const auto encryption_token = get_config().get_s3_config().encryption_token;
  const auto is_encrypted = not encryption_token.empty();

  const auto path_parts = utils::string::split(api_path, '/', false);
  const auto key_parts = utils::string::split(key, '/', false);

  if (is_encrypted && key_parts.size() != path_parts.size()) {
    return api_error::error;
  }

  std::string cur_key{'/'};
  std::string cur_path{'/'};
  for (std::size_t i = 0U; i < path_parts.size(); i++) {
    if (is_encrypted) {
      cur_key = utils::path::create_api_path(
          utils::path::combine(cur_key, {key_parts.at(i)}));
    }
    cur_path = utils::path::create_api_path(
        utils::path::combine(cur_path, {path_parts.at(i)}));

    api_meta_map meta{};
    auto res = get_item_meta(cur_path, meta);
    if (res == api_error::item_not_found) {
      auto dir = create_api_file(cur_path, cur_key, 0U);
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
    auto err = utils::encryption::decrypt_file_name(
        get_config().get_s3_config().encryption_token, part);
    if (err != api_error::success) {
      return err;
    }
  }

  object_name = utils::string::join(parts, '/');
  return api_error::success;
}

auto s3_provider::get_directory_item_count(const std::string &api_path) const
    -> std::uint64_t {
  try {
    const auto cfg = get_config().get_s3_config();
    const auto is_encrypted = not cfg.encryption_token.empty();
    std::string key;
    if (is_encrypted) {
      auto res = get_item_meta(api_path, META_KEY, key);
      if (res != api_error::success) {
        return 0U;
      }
    }

    const auto object_name =
        api_path == "/"
            ? ""
            : utils::path::create_api_path(is_encrypted ? key : api_path);

    std::string response_data{};
    long response_code{};
    auto prefix = object_name.empty() ? object_name : object_name + "/";

    if (not get_object_list(response_data, response_code, "/", prefix)) {
      return 0U;
    }

    if (response_code == http_error_codes::not_found) {
      return 0U;
    }

    if (response_code != http_error_codes::ok) {
      return 0U;
    }

    pugi::xml_document doc;
    auto res = doc.load_string(response_data.c_str());
    if (res.status != pugi::xml_parse_status::status_ok) {
      return 0U;
    }

    auto node_list =
        doc.select_nodes("/ListBucketResult/CommonPrefixes/Prefix");
    std::uint64_t ret = node_list.size();

    node_list = doc.select_nodes("/ListBucketResult/Contents");
    ret += node_list.size();
    if (not prefix.empty()) {
      --ret;
    }

    return ret;
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return 0U;
}

auto s3_provider::get_directory_items_impl(const std::string &api_path,
                                           directory_item_list &list) const
    -> api_error {
  const auto cfg = get_config().get_s3_config();
  const auto is_encrypted = not cfg.encryption_token.empty();

  auto ret = api_error::success;
  std::string key;
  if (is_encrypted) {
    ret = get_item_meta(api_path, META_KEY, key);
    if (ret != api_error::success) {
      return ret;
    }
  }

  const auto object_name =
      api_path == "/"
          ? ""
          : utils::path::create_api_path(is_encrypted ? key : api_path);

  std::string response_data{};
  long response_code{};
  auto prefix = object_name.empty() ? object_name : object_name + "/";

  if (not get_object_list(response_data, response_code, "/", prefix)) {
    return api_error::comm_error;
  }

  if (response_code == http_error_codes::not_found) {
    return api_error::directory_not_found;
  }

  if (response_code != http_error_codes::ok) {
    return api_error::comm_error;
  }

  pugi::xml_document doc;
  auto parse_res = doc.load_string(response_data.c_str());
  if (parse_res.status != pugi::xml_parse_status::status_ok) {
    return api_error::error;
  }

  const auto add_directory_item =
      [&](bool directory, const std::string &name,
          std::function<std::uint64_t(const directory_item &)> get_size)
      -> api_error {
    auto child_api_path =
        utils::path::create_api_path(utils::path::combine("/", {name}));
    std::string child_object_name;
    if (is_encrypted) {
      child_object_name = child_api_path;
      ret = utils::encryption::decrypt_file_path(cfg.encryption_token,
                                                 child_api_path);
      if (ret != api_error::success) {
        return ret;
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
        auto file =
            create_api_file(child_api_path, child_object_name, dir_item.size);
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

  auto node_list = doc.select_nodes("/ListBucketResult/CommonPrefixes/Prefix");
  for (const auto &node : node_list) {
    add_directory_item(
        true, node.node().text().as_string(),
        [](const directory_item &) -> std::uint64_t { return 0U; });
  }

  node_list = doc.select_nodes("/ListBucketResult/Contents");
  for (const auto &node : node_list) {
    auto child_object_name = utils::path::create_api_path(
        node.node().select_node("Key").node().text().as_string());
    if (child_object_name != utils::path::create_api_path(prefix)) {
      auto size = node.node().select_node("Size").node().text().as_ullong();
      add_directory_item(
          false, child_object_name,
          [&is_encrypted, &size](const directory_item &) -> std::uint64_t {
            return is_encrypted ? utils::encryption::encrypting_reader::
                                      calculate_decrypted_size(size)
                                : size;
          });
    }
  }

  return ret;
}

auto s3_provider::get_file(const std::string &api_path, api_file &file) const
    -> api_error {
  try {
    bool is_encrypted{};
    std::string object_name;
    head_object_result result{};
    auto res =
        get_object_info(false, api_path, is_encrypted, object_name, result);
    if (res != api_error::success) {
      return res;
    }

    file.accessed_date = utils::get_file_time_now();
    file.api_path = api_path;
    file.api_parent = utils::path::get_parent_api_path(file.api_path);
    file.changed_date = utils::aws::format_time(result.last_modified);
    file.creation_date = utils::aws::format_time(result.last_modified);
    file.file_size =
        is_encrypted
            ? utils::encryption::encrypting_reader::calculate_decrypted_size(
                  result.content_length)
            : result.content_length;
    file.modified_date = utils::aws::format_time(result.last_modified);
    return add_if_not_found(file, object_name);
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return api_error::error;
}

auto s3_provider::get_file_list(api_file_list &list) const -> api_error {
  std::string response_data;
  long response_code{};
  if (not get_object_list(response_data, response_code)) {
    return api_error::comm_error;
  }

  if (response_code != http_error_codes::ok) {
    return api_error::comm_error;
  }

  pugi::xml_document doc;
  auto res = doc.load_string(response_data.c_str());
  if (res.status != pugi::xml_parse_status::status_ok) {
    return api_error::comm_error;
  }

  auto node_list = doc.select_nodes("/ListBucketResult/Contents");
  for (const auto &node : node_list) {
    auto api_path =
        std::string{node.node().select_node("Key").node().text().as_string()};
    if (not utils::string::ends_with(api_path, "/")) {
      auto is_encrypted =
          not get_config().get_s3_config().encryption_token.empty();
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
      file.accessed_date = utils::get_file_time_now();
      file.changed_date = utils::convert_api_date(
          node.node().select_node("LastModified").node().text().as_string());
      file.creation_date = file.changed_date;
      file.file_size =
          is_encrypted
              ? utils::encryption::encrypting_reader::calculate_decrypted_size(
                    size)
              : size;
      file.key = is_encrypted ? utils::path::create_api_path(api_path) : "";
      file.modified_date = file.changed_date;

      auto err = add_if_not_found(file, api_path);
      if (err != api_error::success) {
        return err;
      }

      list.push_back(std::move(file));
    }
  }

  return api_error::success;
}

auto s3_provider::get_object_info(bool directory, const std::string &api_path,
                                  bool &is_encrypted, std::string &object_name,
                                  head_object_result &result) const
    -> api_error {
  try {
    const auto cfg = get_config().get_s3_config();
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
      return api_error::comm_error;
    }

    result.from_headers(head.response_headers.value());

    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return api_error::error;
}

auto s3_provider::get_object_list(std::string &response_data,
                                  long &response_code,
                                  std::optional<std::string> delimiter,
                                  std::optional<std::string> prefix) const
    -> bool {
  curl::requests::http_get get{};
  get.allow_timeout = true;
  get.aws_service = "aws:amz:" + get_config().get_s3_config().region + ":s3";
  get.path = '/';
  get.query["list-type"] = "2";
  if (delimiter.has_value() && not delimiter.value().empty()) {
    get.query["delimiter"] = delimiter.value();
  }
  if (prefix.has_value() && not prefix.value().empty()) {
    get.query["prefix"] = prefix.value();
    utils::string::left_trim(get.query["prefix"], '/');
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

auto s3_provider::get_used_drive_space_impl() const -> std::uint64_t {
  std::string response_data;
  long response_code{};
  if (not get_object_list(response_data, response_code)) {
    return 0U;
  }

  if (response_code != http_error_codes::ok) {
    return 0U;
  }

  pugi::xml_document doc;
  auto res = doc.load_string(response_data.c_str());
  if (res.status != pugi::xml_parse_status::status_ok) {
    return 0U;
  }

  const auto cfg = get_config().get_s3_config();
  const auto is_encrypted = not cfg.encryption_token.empty();

  auto node_list = doc.select_nodes("/ListBucketResult/Contents");
  return std::accumulate(
      node_list.begin(), node_list.end(), std::uint64_t(0U),
      [&is_encrypted](std::uint64_t total, auto node) -> std::uint64_t {
        auto size = node.node().select_node("Size").node().text().as_ullong();
        return total + (is_encrypted ? utils::encryption::encrypting_reader::
                                           calculate_decrypted_size(size)
                                     : size);
      });
}

auto s3_provider::is_directory(const std::string &api_path, bool &exists) const
    -> api_error {
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
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return api_error::error;
}

auto s3_provider::is_file(const std::string &api_path, bool &exists) const
    -> api_error {
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
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
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
  try {
    const auto cfg = get_config().get_s3_config();
    const auto is_encrypted = not cfg.encryption_token.empty();
    std::string key;
    if (is_encrypted) {
      auto res = get_item_meta(api_path, META_KEY, key);
      if (res != api_error::success) {
        return res;
      }
    }

    const auto object_name =
        utils::path::create_api_path(is_encrypted ? key : api_path);

    const auto read_bytes =
        [this, &api_path, &cfg, &object_name,
         &stop_requested](std::size_t read_size, std::size_t read_offset,
                          data_buffer &read_buffer) -> api_error {
      auto res = api_error::error;
      for (std::uint32_t i = 0U;
           not stop_requested && res != api_error::success &&
           i < get_config().get_retry_read_count() + 1U;
           i++) {
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
                __FUNCTION__, api_path, api_error::comm_error,
                "read file bytes failed|offset|" + std::to_string(read_offset) +
                    "|size|" + std::to_string(read_size) + "|retry|" +
                    std::to_string(i + 1U));
          } else {
            utils::error::raise_api_path_error(
                __FUNCTION__, api_path, response_code,
                "read file bytes failed|offset|" + std::to_string(read_offset) +
                    "|size|" + std::to_string(read_size) + "|retry|" +
                    std::to_string(i + 1U));
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

    if (is_encrypted) {
      std::string temp;
      auto res = get_item_meta(api_path, META_SIZE, temp);
      if (res != api_error::success) {
        return res;
      }
      const auto total_size = utils::string::to_uint64(temp);

      return utils::encryption::read_encrypted_range(
          {offset, offset + size - 1U},
          utils::encryption::generate_key(cfg.encryption_token),
          [&](data_buffer &ct_buffer, std::uint64_t start_offset,
              std::uint64_t end_offset) -> api_error {
            return read_bytes((end_offset - start_offset + 1U), start_offset,
                              ct_buffer);
          },
          total_size, data);
    }

    return read_bytes(size, offset, data);
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return api_error::error;
}

auto s3_provider::remove_directory_impl(const std::string &api_path)
    -> api_error {
  const auto cfg = get_config().get_s3_config();
  const auto is_encrypted = not cfg.encryption_token.empty();

  std::string key;
  if (is_encrypted) {
    auto res = get_item_meta(api_path, META_KEY, key);
    if (res != api_error::success) {
      return res;
    }
  }

  const auto object_name =
      utils::path::create_api_path(is_encrypted ? key : api_path);

  curl::requests::http_delete del{};
  del.allow_timeout = true;
  del.aws_service = "aws:amz:" + cfg.region + ":s3";
  del.path = object_name + '/';

  long response_code{};
  stop_type stop_requested{};
  if (not get_comm().make_request(del, response_code, stop_requested)) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       api_error::comm_error,
                                       "failed to remove directory");
    return api_error::comm_error;
  }

  if ((response_code < http_error_codes::ok ||
       response_code >= http_error_codes::multiple_choices) &&
      response_code != http_error_codes::not_found) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, response_code,
                                       "failed to remove directory");
    return api_error::comm_error;
  }

  return api_error::success;
}

auto s3_provider::remove_file_impl(const std::string &api_path) -> api_error {
  const auto cfg = get_config().get_s3_config();
  const auto is_encrypted = not cfg.encryption_token.empty();

  std::string key;
  if (is_encrypted) {
    auto res = get_item_meta(api_path, META_KEY, key);
    if (res != api_error::success) {
      return res;
    }
  }

  const auto object_name =
      utils::path::create_api_path(is_encrypted ? key : api_path);

  curl::requests::http_delete del{};
  del.allow_timeout = true;
  del.aws_service = "aws:amz:" + cfg.region + ":s3";
  del.path = object_name;

  long response_code{};
  stop_type stop_requested{};
  if (not get_comm().make_request(del, response_code, stop_requested)) {
    utils::error::raise_api_path_error(
        __FUNCTION__, api_path, api_error::comm_error, "failed to remove file");
    return api_error::comm_error;
  }

  if ((response_code < http_error_codes::ok ||
       response_code >= http_error_codes::multiple_choices) &&
      response_code != http_error_codes::not_found) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, response_code,
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
  std::uint64_t file_size{};
  if (utils::file::is_file(source_path) &&
      not utils::file::get_file_size(source_path, file_size)) {
    return api_error::comm_error;
  }

  const auto cfg = get_config().get_s3_config();
  const auto is_encrypted = not cfg.encryption_token.empty();

  std::string key;
  if (is_encrypted) {
    auto res = get_item_meta(api_path, META_KEY, key);
    if (res != api_error::success) {
      return res;
    }
  }

  const auto object_name =
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
    return api_error::comm_error;
  }

  return api_error::success;
}
} // namespace repertory

#endif // REPERTORY_ENABLE_S3
