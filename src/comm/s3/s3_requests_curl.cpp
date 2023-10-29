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

#include "comm/s3/s3_requests.hpp"

#include "comm/curl/curl_comm.hpp"
#include "comm/curl/requests/http_get.hpp"
#include "utils/encryption.hpp"
#include "utils/error_utils.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
namespace {
[[nodiscard]] auto
get_object_list(i_http_comm &client, const s3_config &config,
                std::string &response_data, long &response_code,
                std::optional<std::string> delimiter = std::nullopt,
                std::optional<std::string> prefix = std::nullopt) -> bool {
  curl::requests::http_get get{};
  get.aws_service = "aws:amz:" + config.region + ":s3";
  get.path = '/';
  get.query["list-type"] = "2";
  if (delimiter.has_value() && not delimiter.value().empty()) {
    get.query["delimiter"] = delimiter.value();
  }
  if (prefix.has_value() && not prefix.value().empty()) {
    get.query["prefix"] = prefix.value();
  }
  get.response_handler = [&response_data](const data_buffer &data,
                                          long /*response_code*/) {
    response_data = std::string(data.begin(), data.end());
  };

  stop_type stop_requested{};
  return client.make_request(get, response_code, stop_requested);
}
} // namespace

auto create_directory_object_request_impl(i_http_comm &client,
                                          const s3_config &config,
                                          const std::string &object_name,
                                          long &response_code) -> bool {
  try {
    curl::requests::http_put_file put_file{};
    put_file.aws_service = "aws:amz:" + config.region + ":s3";
    put_file.file_name =
        *(utils::string::split(object_name, '/', false).end() - 1U);
    put_file.path = '/' + object_name;

    stop_type stop_requested{false};
    return client.make_request(put_file, response_code, stop_requested);
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return false;
}

auto delete_object_request_impl(i_http_comm &client, const s3_config &config,
                                const std::string &object_name,
                                long &response_code) -> bool {
  try {
    head_object_result result{};
    if (not head_object_request_impl(client, config, object_name, result,
                                     response_code)) {
      return false;
    }

    if (response_code == 404) {
      return true;
    }

    curl::requests::http_delete del{};
    del.aws_service = "aws:amz:" + config.region + ":s3";
    del.path = '/' + object_name;

    stop_type stop_requested{false};
    return client.make_request(del, response_code, stop_requested);
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return false;
}

auto head_object_request_impl(i_http_comm &client, const s3_config &config,
                              const std::string &object_name,
                              head_object_result &result, long &response_code)
    -> bool {
  try {
    curl::requests::http_head head{};
    head.aws_service = "aws:amz:" + config.region + ":s3";
    head.path = '/' + object_name;
    head.response_headers = http_headers{};

    stop_type stop_requested{false};
    if (not client.make_request(head, response_code, stop_requested)) {
      return false;
    }

    if (response_code == 200) {
      result.from_headers(head.response_headers.value());
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return false;
}

auto list_directories_request_impl(i_http_comm &client, const s3_config &config,
                                   list_directories_result &result,
                                   long &response_code) -> bool {
  try {
    std::string response_data{};
    if (not get_object_list(client, config, response_data, response_code)) {
      return false;
    }

    if (response_code != 200) {
      return false;
    }

    pugi::xml_document doc;
    auto res = doc.load_string(response_data.c_str());
    if (res.status != pugi::xml_parse_status::status_ok) {
      return false;
    }

    auto node_list = doc.select_nodes("/ListBucketResult/Contents");
    for (const auto &node : node_list) {
      auto object_name =
          node.node().select_node("Key").node().text().as_string();
      if (utils::string::ends_with(object_name, "/")) {
        api_file directory{};
        directory.api_path = utils::path::create_api_path(object_name);
        directory.api_parent =
            utils::path::get_parent_api_path(directory.api_path);
        directory.accessed_date = utils::get_file_time_now();
        directory.changed_date = utils::convert_api_date(
            node.node().select_node("LastModified").node().text().as_string());
        directory.creation_date = directory.changed_date;
        directory.modified_date = directory.changed_date;
        result.emplace_back(std::move(directory));
      }
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return false;
}

auto list_files_request_impl(
    i_http_comm &client, const s3_config &config,
    const get_api_file_token_callback &get_api_file_token,
    const get_name_callback &get_name, list_files_result &result,
    long &response_code) -> bool {
  try {
    std::string response_data{};
    if (not get_object_list(client, config, response_data, response_code)) {
      return false;
    }

    if (response_code != 200) {
      return false;
    }

    pugi::xml_document doc;
    auto res = doc.load_string(response_data.c_str());
    if (res.status != pugi::xml_parse_status::status_ok) {
      return false;
    }

    auto node_list = doc.select_nodes("/ListBucketResult/Contents");
    for (const auto &node : node_list) {
      std::string object_name =
          node.node().select_node("Key").node().text().as_string();
      if (not utils::string::ends_with(object_name, "/")) {
        api_file file{};
        object_name = get_name(
            *(utils::string::split(object_name, '/', false).end() - 1u),
            object_name);
        file.api_path = utils::path::create_api_path(object_name);
        file.api_parent = utils::path::get_parent_api_path(file.api_path);
        file.accessed_date = utils::get_file_time_now();
        file.encryption_token = get_api_file_token(file.api_path);
        auto size = node.node().select_node("Size").node().text().as_ullong();
        file.file_size = file.encryption_token.empty()
                             ? size
                             : utils::encryption::encrypting_reader::
                                   calculate_decrypted_size(size);
        file.changed_date = utils::convert_api_date(
            node.node().select_node("LastModified").node().text().as_string());
        file.creation_date = file.changed_date;
        file.modified_date = file.changed_date;
        result.emplace_back(std::move(file));
      }
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return false;
}

auto list_objects_in_directory_request_impl(
    i_http_comm &client, const s3_config &config,
    const std::string &object_name, meta_provider_callback meta_provider,
    list_objects_result &result, long &response_code) -> bool {
  try {
    std::string response_data{};
    auto prefix = object_name.empty() ? object_name : object_name + "/";
    if (not get_object_list(client, config, response_data, response_code, "/",
                            prefix)) {
      return false;
    }

    if (response_code != 200) {
      return false;
    }

    pugi::xml_document doc;
    auto res = doc.load_string(response_data.c_str());
    if (res.status != pugi::xml_parse_status::status_ok) {
      return false;
    }

    const auto add_directory_item =
        [&](bool directory, const std::string &name,
            std::function<std::uint64_t(const directory_item &)> get_size) {
          directory_item di{};
          di.api_path =
              utils::path::create_api_path(utils::path::combine("/", {name}));
          di.api_parent = utils::path::get_parent_api_path(di.api_path);
          di.directory = directory;
          di.size = get_size(di);
          meta_provider(di);
          result.emplace_back(std::move(di));
        };

    auto node_list =
        doc.select_nodes("/ListBucketResult/CommonPrefixes/Prefix");
    for (const auto &node : node_list) {
      add_directory_item(
          true, node.node().text().as_string(),
          [](const directory_item &) -> std::uint64_t { return 0U; });
    }

    node_list = doc.select_nodes("/ListBucketResult/Contents");
    for (const auto &node : node_list) {
      auto child_object_name =
          node.node().select_node("Key").node().text().as_string();
      if (child_object_name != prefix) {
        auto size = node.node().select_node("Size").node().text().as_ullong();
        add_directory_item(
            false, child_object_name,
            [&size](const directory_item &) -> std::uint64_t { return size; });
      }
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return false;
}

auto list_objects_request_impl(i_http_comm &client, const s3_config &config,
                               list_objects_result &result, long &response_code)
    -> bool {
  try {
    std::string response_data{};
    if (not get_object_list(client, config, response_data, response_code)) {
      return false;
    }

    if (response_code != 200) {
      return false;
    }

    pugi::xml_document doc;
    auto res = doc.load_string(response_data.c_str());
    if (res.status != pugi::xml_parse_status::status_ok) {
      return false;
    }

    auto node_list = doc.select_nodes("/ListBucketResult/Contents");
    for (const auto &node : node_list) {
      auto object_name =
          node.node().select_node("Key").node().text().as_string();
      auto size = node.node().select_node("Size").node().text().as_ullong();
      directory_item di{};
      di.api_path = utils::path::create_api_path(object_name);
      di.api_parent = utils::path::get_parent_api_path(di.api_path);
      di.directory = utils::string::ends_with(object_name, "/");
      di.size = di.directory ? 0U : size;
      di.resolved = false;
      result.emplace_back(std::move(di));
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return false;
}

auto put_object_request_impl(i_http_comm &client, const s3_config &config,
                             std::string object_name,
                             const std::string &source_path,
                             const std::string &encryption_token,
                             get_key_callback get_key, set_key_callback set_key,
                             long &response_code, stop_type &stop_requested)
    -> bool {
  try {
    curl::requests::http_put_file put_file{};
    put_file.aws_service = "aws:amz:" + config.region + ":s3";
    put_file.encryption_token = encryption_token;
    put_file.file_name =
        *(utils::string::split(object_name, '/', false).end() - 1U);
    put_file.path = '/' + object_name;
    put_file.source_path = source_path;

    if (not encryption_token.empty()) {
      static stop_type no_stop{false};

      put_file.reader = std::make_shared<utils::encryption::encrypting_reader>(
          put_file.file_name, source_path, no_stop, encryption_token,
          std::nullopt, -1);
      auto key = get_key();
      if (key.empty()) {
        key = put_file.reader->get_encrypted_file_name();
        set_key(key);
      }
    }

    return client.make_request(put_file, response_code, stop_requested);
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return false;
}

auto read_object_request_impl(i_http_comm &client, const s3_config &config,
                              const std::string &object_name, std::size_t size,
                              std::uint64_t offset, data_buffer &data,
                              long &response_code, stop_type &stop_requested)
    -> bool {
  try {
    curl::requests::http_get get{};
    get.aws_service = "aws:amz:" + config.region + ":s3";
    get.headers["response-content-type"] = "binary/octet-stream";
    get.path = '/' + object_name;
    get.range = {{offset, offset + size - 1}};
    get.response_handler = [&data](const data_buffer &response_data,
                                   long /*response_code*/) {
      data = response_data;
    };

    return client.make_request(get, response_code, stop_requested);
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return false;
}
} // namespace repertory

#endif
