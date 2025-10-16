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
#include "providers/sia/sia_provider.hpp"

#include "app_config.hpp"
#include "comm/i_http_comm.hpp"
#include "events/event_system.hpp"
#include "events/types/service_start_begin.hpp"
#include "events/types/service_start_end.hpp"
#include "events/types/service_stop_begin.hpp"
#include "events/types/service_stop_end.hpp"
#include "file_manager/i_file_manager.hpp"
#include "providers/base_provider.hpp"
#include "providers/s3/s3_provider.hpp"
#include "types/repertory.hpp"
#include "utils/common.hpp"
#include "utils/error_utils.hpp"
#include "utils/path.hpp"
#include "utils/polling.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"
#include "utils/utils.hpp"

namespace {
[[nodiscard]] auto get_last_modified(const nlohmann::json &obj)
    -> std::uint64_t {
  try {
    return repertory::s3_provider::convert_api_date(
        obj["modTime"].get<std::string>());
  } catch (...) {
    return repertory::utils::time::get_time_now();
  }
}
} // namespace

namespace repertory {
sia_provider::sia_provider(app_config &config, i_http_comm &comm)
    : base_provider(config, comm) {}

auto sia_provider::check_version(std::string &required_version,
                                 std::string &returned_version) const -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  required_version = RENTERD_MIN_VERSION;

  try {
    curl::requests::http_get get{};
    get.allow_timeout = true;
    get.path = "/api/bus/state";

    nlohmann::json state_data;
    std::string error_data;
    get.response_handler = [&error_data, &state_data](auto &&data,
                                                      long response_code) {
      if (response_code == http_error_codes::ok) {
        state_data = nlohmann::json::parse(data.begin(), data.end());
        return;
      }

      error_data = std::string(data.begin(), data.end());
    };

    long response_code{};
    stop_type stop_requested{};
    if (not get_comm().make_request(get, response_code, stop_requested)) {
      utils::error::raise_error(function_name, response_code,
                                "failed to check state");
      return false;
    }

    if (response_code != http_error_codes::ok) {
      utils::error::raise_error(
          function_name, response_code,
          fmt::format("failed to check state|response|{}", error_data));
      return false;
    }

    returned_version = state_data.at("version").get<std::string>().substr(1U);
    return utils::compare_version_strings(returned_version, required_version) >=
           0;
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "failed to check version");
  }

  return false;
}

auto sia_provider::create_directory_impl(std::string_view api_path,
                                         api_meta_map & /* meta */)
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  curl::requests::http_put_file put_file{};
  put_file.allow_timeout = true;
  put_file.path = fmt::format("/api/worker/object{}/", api_path);
  put_file.query["bucket"] = get_sia_config().bucket;

  std::string error_data;
  put_file.response_handler = [&error_data](auto &&data, long response_code) {
    if (response_code == http_error_codes::ok) {
      return;
    }

    error_data = std::string(data.begin(), data.end());
  };

  long response_code{};
  stop_type stop_requested{};
  if (not get_comm().make_request(put_file, response_code, stop_requested)) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::comm_error,
                                       "failed to create directory");
    return api_error::comm_error;
  }

  if (response_code != http_error_codes::ok) {
    utils::error::raise_api_path_error(
        function_name, api_path, response_code,
        fmt::format("failed to create directory|response|{}", error_data));
    return api_error::comm_error;
  }

  return api_error::success;
}

auto sia_provider::create_directory_key(std::string_view api_path) const
    -> api_error {
  auto parent_api_path = utils::path::get_parent_api_path(api_path);

  json object_list;
  if (not get_object_list(parent_api_path, object_list)) {
    return api_error::comm_error;
  }

  if (not object_list.contains("objects")) {
    return api_error::directory_not_found;
  }

  const auto &list = object_list.at("objects");
  if (std::ranges::find_if(list, [&api_path](auto &&entry) -> bool {
        return entry.at("key").template get<std::string>() ==
               std::string{api_path} + '/';
      }) == list.end()) {
    return api_error::directory_not_found;
  }

  api_meta_map meta;
  return const_cast<sia_provider *>(this)->create_directory_impl(api_path,
                                                                 meta);
}

auto sia_provider::ensure_directory_exists(std::string_view api_path) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  bool exists{};
  auto res{is_directory(api_path, exists)};
  if (res != api_error::success) {
    utils::error::raise_api_path_error(function_name, api_path, res,
                                       "failed detect existing directory");
    return res;
  }

  if (not exists) {
    utils::error::raise_api_path_error(function_name, api_path, res,
                                       "directory not found");
    return api_error::directory_not_found;
  }

  return api_error::success;
}

auto sia_provider::get_directory_item_count(std::string_view api_path) const
    -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    json object_list;
    if (not get_object_list(api_path, object_list)) {
      return 0U;
    }

    std::uint64_t item_count{};
    if (object_list.contains("objects")) {
      for (const auto &entry : object_list.at("objects")) {
        try {
          auto name{entry.at("key").get<std::string>()};
          auto entry_api_path{utils::path::create_api_path(name)};
          if (utils::string::ends_with(name, "/") &&
              (entry_api_path == api_path)) {
            continue;
          }
          ++item_count;
        } catch (const std::exception &e) {
          utils::error::raise_api_path_error(function_name, api_path, e,
                                             "failed to process entry|" +
                                                 entry.dump());
        }
      }
    }

    return item_count;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to get directory item count");
  }

  return 0U;
}

auto sia_provider::get_directory_items_impl(std::string_view api_path,
                                            directory_item_list &list) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  json object_list;
  if (not get_object_list(api_path, object_list)) {
    return api_error::comm_error;
  }

  iterate_objects(
      api_path, object_list,
      [&](auto &&entry_api_path, auto &&directory, auto &&entry) {
        api_meta_map meta;
        auto res{get_item_meta(entry_api_path, meta)};
        if (res != api_error::success) {
          utils::error::raise_api_path_error(function_name, entry_api_path, res,
                                             "failed to get item meta");
          return;
        }

        auto file = create_api_file(
            entry_api_path,
            directory ? 0U : entry["size"].template get<std::uint64_t>(), meta);

        directory_item dir_item{};
        dir_item.api_parent = file.api_parent;
        dir_item.api_path = file.api_path;
        dir_item.directory = directory;
        dir_item.meta = meta;
        dir_item.size = file.file_size;
        list.emplace_back(std::move(dir_item));
      });

  return api_error::success;
}

auto sia_provider::get_file(std::string_view api_path, api_file &file) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    json file_data{};
    auto res{get_object_info(api_path, file_data)};
    if (res != api_error::success) {
      if (res != api_error::item_not_found) {
        return res;
      }

      bool exists{};
      res = is_directory(api_path, exists);
      if (res != api_error::success) {
        utils::error::raise_api_path_error(
            function_name, api_path, res,
            "failed to determine if directory exists");
      }

      return exists ? api_error::directory_exists : api_error::item_not_found;
    }

    auto size{
        file_data.at("size").get<std::uint64_t>(),
    };

    api_meta_map meta{};
    if (get_item_meta(api_path, meta) == api_error::item_not_found) {
      file = create_api_file(api_path, "", size, get_last_modified(file_data));
      get_api_item_added()(false, file);
    } else {
      file = create_api_file(api_path, size, meta);
    }

    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to get file");
  }

  return api_error::error;
}

auto sia_provider::get_file_list(api_file_list &list, std::string &marker) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    json object_list;
    if (not get_object_list("", object_list, marker)) {
      return api_error::comm_error;
    }

    iterate_objects("/", object_list,
                    [&](auto &&entry_api_path, auto &&directory, auto &&entry) {
                      if (directory) {
                        return;
                      }

                      api_meta_map meta;
                      auto res{get_item_meta(entry_api_path, meta)};
                      if (res != api_error::success) {
                        utils::error::raise_api_path_error(
                            function_name, entry_api_path, res,
                            "failed to get item meta");
                        return;
                      }

                      list.emplace_back(create_api_file(
                          entry_api_path,
                          entry["size"].template get<std::uint64_t>(), meta));
                    });

    marker = object_list.at("nextMarker").get<std::string>();
    return object_list.at("hasMore").get<bool>() ? api_error::more_data
                                                 : api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, "/", e,
                                       "failed to process directory");
  }

  return api_error::error;
}

auto sia_provider::get_object_info(std::string_view api_path,
                                   json &object_info) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    curl::requests::http_get get{};
    get.allow_timeout = true;
    get.path = fmt::format("/api/bus/object{}", api_path);
    get.query["bucket"] = get_sia_config().bucket;
    get.query["onlymetadata"] = "true";

    std::string error_data;
    get.response_handler = [&error_data, &object_info](auto &&data,
                                                       long response_code) {
      if (response_code == http_error_codes::ok) {
        object_info = nlohmann::json::parse(data.begin(), data.end());
        return;
      }

      error_data = std::string(data.begin(), data.end());
    };

    long response_code{};
    stop_type stop_requested{};
    if (not get_comm().make_request(get, response_code, stop_requested)) {
      return api_error::comm_error;
    }

    if (response_code == http_error_codes::not_found) {
      return api_error::item_not_found;
    }

    if (response_code != http_error_codes::ok) {
      utils::error::raise_api_path_error(
          function_name, api_path, response_code,
          fmt::format("failed to get object info|response|{}", error_data));
      return api_error::comm_error;
    }

    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to get object info");
  }

  return api_error::error;
}

auto sia_provider::get_object_list(std::string_view api_path,
                                   nlohmann::json &object_list,
                                   std::optional<std::string> marker) const
    -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    curl::requests::http_get get{};
    get.allow_timeout = true;
    get.path = fmt::format("/api/bus/objects{}/", api_path);
    get.query["bucket"] = get_sia_config().bucket;
    if (marker.has_value()) {
      get.query["limit"] = "1000";
      get.query["marker"] = marker.value();
    } else {
      get.query["delimiter"] = "/";
    }

    std::string error_data;
    get.response_handler = [&error_data, &object_list](auto &&data,
                                                       long response_code) {
      if (response_code == http_error_codes::ok) {
        object_list = nlohmann::json::parse(data.begin(), data.end());
        return;
      }

      error_data = std::string(data.begin(), data.end());
    };

    long response_code{};
    stop_type stop_requested{};
    if (not get_comm().make_request(get, response_code, stop_requested)) {
      utils::error::raise_api_path_error(function_name, api_path,
                                         api_error::comm_error,
                                         "failed to get object list");
      return false;
    }

    if (response_code != http_error_codes::ok) {
      utils::error::raise_api_path_error(
          function_name, api_path, response_code,
          fmt::format("failed to get object list|response|{}", error_data));
      return false;
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to get object list");
  }

  return false;
}

auto sia_provider::get_total_drive_space() const -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    curl::requests::http_get get{};
    get.allow_timeout = true;
    get.path = "/api/bus/autopilot";
    get.query["bucket"] = get_sia_config().bucket;

    json config_data;
    std::string error_data;
    get.response_handler = [&error_data, &config_data](auto &&data,
                                                       long response_code) {
      if (response_code == http_error_codes::ok) {
        config_data = nlohmann::json::parse(data.begin(), data.end());
        return;
      }

      error_data = std::string(data.begin(), data.end());
    };

    long response_code{};
    stop_type stop_requested{};
    if (not get_comm().make_request(get, response_code, stop_requested)) {
      return 0U;
    }

    if (response_code != http_error_codes::ok) {
      utils::error::raise_error(
          function_name, response_code,
          fmt::format("failed to get total drive space|response|{}",
                      error_data));
      return 0U;
    }

    return config_data["contracts"]["storage"].get<std::uint64_t>();
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e,
                              "failed to get total drive space");
  }

  return 0U;
}

auto sia_provider::is_directory(std::string_view api_path, bool &exists) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (api_path == "/") {
      exists = true;
      return api_error::success;
    }

    exists = false;

    json file_data{};
    auto res{get_object_info(std::string{api_path} + '/', file_data)};
    if (res == api_error::item_not_found) {
      if (create_directory_key(api_path) == api_error::success) {
        exists = true;
        return api_error::success;
      }
    }

    if (res == api_error::directory_not_found ||
        res == api_error::item_not_found) {
      return api_error::success;
    }

    if (res != api_error::success) {
      return res;
    }

    exists =
        utils::string::ends_with(file_data.at("key").get<std::string>(), "/");
    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(
        function_name, api_path, e, "failed to determine if path is directory");
  }

  return api_error::error;
}

auto sia_provider::is_file(std::string_view api_path, bool &exists) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    exists = false;

    if (api_path == "/") {
      return api_error::success;
    }

    json file_data{};
    auto res{get_object_info(api_path, file_data)};
    if (res == api_error::item_not_found) {
      return api_error::success;
    }

    if (res != api_error::success) {
      return res;
    }

    exists = not utils::string::ends_with(
        file_data.at("key").get<std::string>(), "/");
    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to determine if path is file");
  }

  return api_error::error;
}

auto sia_provider::is_online() const -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    curl::requests::http_get get{};
    get.allow_timeout = true;
    get.path = "/api/bus/consensus/state";
    get.query["bucket"] = get_sia_config().bucket;

    std::string error_data;
    json state_data;
    get.response_handler = [&error_data, &state_data](auto &&data,
                                                      long response_code) {
      if (response_code == http_error_codes::ok) {
        state_data = nlohmann::json::parse(data.begin(), data.end());
        return;
      }

      error_data = std::string(data.begin(), data.end());
    };

    long response_code{};
    stop_type stop_requested{};
    if (not get_comm().make_request(get, response_code, stop_requested)) {
      utils::error::raise_error(function_name, api_error::comm_error,
                                "failed to determine if provider is online");
      return false;
    }

    if (response_code != http_error_codes::ok) {
      utils::error::raise_error(
          function_name, response_code,
          fmt::format("failed to determine if provider is online|response|{}",
                      error_data));
      return false;
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e,
                              "failed to determine if provider is online");
  }

  return false;
}

void sia_provider::iterate_objects(
    std::string_view api_path, const json &object_list,
    std::function<void(std::string_view, bool, json)> handle_entry) const {
  REPERTORY_USES_FUNCTION_NAME();

  if (not object_list.contains("objects")) {
    return;
  }

  for (const auto &entry : object_list.at("objects")) {
    try {
      auto name{entry.at("key").get<std::string>()};

      auto directory{utils::string::ends_with(name, "/")};
      auto entry_api_path{utils::path::create_api_path(name)};

      {
        api_meta_map meta{};
        if (get_item_meta(entry_api_path, meta) == api_error::item_not_found) {
          auto file = create_api_file(
              entry_api_path, "",
              directory ? 0U : entry["size"].get<std::uint64_t>(),
              get_last_modified(entry));
          if (directory) {
            auto res{ensure_directory_exists(entry_api_path)};
            if (res != api_error::success) {
              utils::error::raise_api_path_error(
                  function_name, entry_api_path, res,
                  "failed detect existing directory");
              continue;
            }
          }

          get_api_item_added()(directory, file);
        }
      }

      handle_entry(entry_api_path, directory, entry);
    } catch (const std::exception &e) {
      utils::error::raise_api_path_error(
          function_name, api_path, e,
          fmt::format("failed to process entry|{}", entry.dump()));
    }
  }
}

auto sia_provider::read_file_bytes(std::string_view api_path, std::size_t size,
                                   std::uint64_t offset,
                                   data_buffer &read_buffer,
                                   stop_type &stop_requested) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    curl::requests::http_get get{};
    get.path = fmt::format("/api/worker/object{}", api_path);
    get.query["bucket"] = get_sia_config().bucket;
    get.headers["accept"] = "application/octet-stream";
    get.range = {{
        .begin = offset,
        .end = offset + size - 1U,
    }};
    get.response_handler = [&read_buffer](auto &&data,
                                          long /* response_code */) {
      read_buffer = data;
    };

    auto res{api_error::comm_error};
    for (std::uint32_t retry{0U};
         not(stop_requested || app_config::get_stop_requested()) &&
         res != api_error::success &&
         retry <
             (static_cast<std::uint32_t>(get_config().get_retry_read_count()) +
              1U);
         ++retry) {
      if (retry > 0U) {
        read_buffer.clear();
        std::this_thread::sleep_for(1s);
      }

      const auto notify_retry = [&](long response_code) {
        auto msg =
            fmt::format("read file bytes failed|offset|{}|size|{}|retry|{}",
                        std::to_string(offset), std::to_string(size),
                        std::to_string(retry + 1U));
        if (response_code == 0) {
          utils::error::raise_api_path_error(function_name, api_path,
                                             api_error::comm_error, msg);
        } else {
          utils::error::raise_api_path_error(function_name, api_path,
                                             response_code, msg);
        }
      };

      long response_code{};
      if (not get_comm().make_request(get, response_code, stop_requested)) {
        notify_retry(response_code);
        continue;
      }

      if (response_code < http_error_codes::ok ||
          response_code >= http_error_codes::multiple_choices) {
        notify_retry(response_code);
        continue;
      }

      res = api_error::success;
    }

    return res;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to read file bytes");
  }

  return api_error::error;
}

auto sia_provider::remove_directory_impl(std::string_view api_path)
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  curl::requests::http_delete del{};
  del.allow_timeout = true;
  del.path = fmt::format("/api/bus/object{}/", api_path);
  del.query["bucket"] = get_sia_config().bucket;

  std::string error_data;
  del.response_handler = [&error_data](auto &&data, long response_code) {
    if (response_code == http_error_codes::ok) {
      return;
    }

    error_data = std::string(data.begin(), data.end());
  };

  long response_code{};
  stop_type stop_requested{};
  if (not get_comm().make_request(del, response_code, stop_requested)) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::comm_error,
                                       "failed to remove directory");
    return api_error::comm_error;
  }

  if (response_code != http_error_codes::ok) {
    utils::error::raise_api_path_error(
        function_name, api_path, response_code,
        fmt::format("failed to remove directory|response|{}", error_data));
    return api_error::comm_error;
  }

  return api_error::success;
}

auto sia_provider::remove_file_impl(std::string_view api_path) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  curl::requests::http_delete del{};
  del.allow_timeout = true;
  del.path = fmt::format("/api/bus/object{}", api_path);
  del.query["bucket"] = get_sia_config().bucket;

  std::string error_data;
  del.response_handler = [&error_data](auto &&data, long response_code) {
    if (response_code == http_error_codes::ok) {
      return;
    }

    error_data = std::string(data.begin(), data.end());
  };

  long response_code{};
  stop_type stop_requested{};
  if (not get_comm().make_request(del, response_code, stop_requested)) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::comm_error,
                                       "failed to remove file");
    return api_error::comm_error;
  }

  if (response_code != http_error_codes::ok &&
      response_code != http_error_codes::not_found) {
    utils::error::raise_api_path_error(
        function_name, api_path, response_code,
        fmt::format("failed to remove file|response|{}", error_data));
    return api_error::comm_error;
  }

  return api_error::success;
}

auto sia_provider::rename_file(std::string_view from_api_path,
                               std::string_view to_api_path) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    bool exists{};
    auto res = is_file(to_api_path, exists);
    if (res != api_error::success) {
      return res;
    }
    if (exists) {
      return api_error::item_exists;
    }

    res = is_directory(to_api_path, exists);
    if (res != api_error::success) {
      return res;
    }
    if (exists) {
      return api_error::directory_exists;
    }

    curl::requests::http_post post{};
    post.json = nlohmann::json({
        {"bucket", get_sia_config().bucket},
        {"from", from_api_path},
        {"to", to_api_path},
        {"mode", "single"},
    });
    post.path = "/api/bus/objects/rename";

    std::string error_data;
    post.response_handler = [&error_data](auto &&data, long response_code) {
      if (response_code == http_error_codes::ok) {
        return;
      }

      error_data = std::string(data.begin(), data.end());
    };

    long response_code{};
    stop_type stop_requested{};
    if (not get_comm().make_request(post, response_code, stop_requested)) {
      utils::error::raise_api_path_error(
          function_name, fmt::format("{}|{}", from_api_path, to_api_path),
          api_error::comm_error, "failed to rename file");
      return api_error::comm_error;
    }

    if (response_code < http_error_codes::ok ||
        response_code >= http_error_codes::multiple_choices) {
      utils::error::raise_api_path_error(
          function_name, fmt::format("{}|{}", from_api_path, to_api_path),
          response_code,
          fmt::format("failed to rename file file|response|{}", error_data));
      return api_error::item_not_found;
    }

    return get_db().rename_item_meta(from_api_path, to_api_path);
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(
        function_name, fmt::format("{}|{}", from_api_path, to_api_path), e,
        "failed to rename file file|response");
  }

  return api_error::error;
}

auto sia_provider::start(api_item_added_callback api_item_added,
                         i_file_manager *mgr) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  event_system::instance().raise<service_start_begin>(function_name,
                                                      "sia_provider");
  sia_config_ = get_config().get_sia_config();
  auto ret = base_provider::start(api_item_added, mgr);
  event_system::instance().raise<service_start_end>(function_name,
                                                    "sia_provider");
  return ret;
}

void sia_provider::stop() {
  REPERTORY_USES_FUNCTION_NAME();

  event_system::instance().raise<service_stop_begin>(function_name,
                                                     "sia_provider");
  base_provider::stop();
  event_system::instance().raise<service_stop_end>(function_name,
                                                   "sia_provider");
}

auto sia_provider::upload_file_impl(std::string_view api_path,
                                    std::string_view source_path,
                                    stop_type &stop_requested) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  curl::requests::http_put_file put_file{};
  put_file.path = fmt::format("/api/worker/object{}", api_path);
  put_file.query["bucket"] = get_sia_config().bucket;
  put_file.headers["content-type"] = "application/octet-stream";
  put_file.source_path = source_path;

  std::string error_data;
  put_file.response_handler = [&error_data](auto &&data, long response_code) {
    if (response_code == http_error_codes::ok) {
      return;
    }

    error_data = std::string(data.begin(), data.end());
  };

  long response_code{};
  if (not get_comm().make_request(put_file, response_code, stop_requested)) {
    utils::error::raise_api_path_error(function_name, api_path, source_path,
                                       api_error::comm_error,
                                       "failed to upload file");
    return api_error::comm_error;
  }

  if (response_code != http_error_codes::ok) {
    utils::error::raise_api_path_error(
        function_name, api_path, response_code,
        fmt::format("failed to upload file|response|{}", error_data));
    return api_error::comm_error;
  }

  return api_error::success;
}
} // namespace repertory
