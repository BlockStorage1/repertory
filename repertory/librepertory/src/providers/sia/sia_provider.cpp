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
#include "providers/sia/sia_provider.hpp"

#include "app_config.hpp"
#include "comm/i_http_comm.hpp"
#include "events/events.hpp"
#include "file_manager/i_file_manager.hpp"
#include "providers/base_provider.hpp"
#include "providers/s3/s3_provider.hpp"
#include "types/repertory.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"
#include "utils/polling.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"
#include "utils/utils.hpp"

namespace {
[[nodiscard]] auto get_bucket(const repertory::sia_config &cfg) -> std::string {
  if (cfg.bucket.empty()) {
    return "default";
  }
  return cfg.bucket;
}

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

auto sia_provider::create_directory_impl(const std::string &api_path,
                                         api_meta_map & /* meta */)
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  curl::requests::http_put_file put_file{};
  put_file.allow_timeout = true;
  put_file.path = "/api/worker/objects" + api_path + "/";
  put_file.query["bucket"] = get_bucket(get_sia_config());

  long response_code{};
  stop_type stop_requested{};
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

auto sia_provider::get_directory_item_count(const std::string &api_path) const
    -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    json object_list{};
    if (not get_object_list(api_path, object_list)) {
      return 0U;
    }

    std::uint64_t item_count{};
    if (object_list.contains("entries")) {
      for (const auto &entry : object_list.at("entries")) {
        try {
          auto name = entry.at("name").get<std::string>();
          auto entry_api_path = utils::path::create_api_path(name);
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

auto sia_provider::get_directory_items_impl(const std::string &api_path,
                                            directory_item_list &list) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  json object_list{};
  if (not get_object_list(api_path, object_list)) {
    return api_error::comm_error;
  }

  if (object_list.contains("entries")) {
    for (const auto &entry : object_list.at("entries")) {
      try {
        auto name = entry.at("name").get<std::string>();
        auto entry_api_path = utils::path::create_api_path(name);

        auto directory = utils::string::ends_with(name, "/");
        if (directory && (entry_api_path == api_path)) {
          continue;
        }

        api_file file{};
        api_meta_map meta{};
        if (get_item_meta(entry_api_path, meta) == api_error::item_not_found) {
          file = create_api_file(entry_api_path, "",
                                 directory ? 0U
                                           : entry["size"].get<std::uint64_t>(),
                                 get_last_modified(entry));
          get_api_item_added()(directory, file);
          auto res = get_item_meta(entry_api_path, meta);
          if (res != api_error::success) {
            utils::error::raise_error(function_name, res,
                                      "failed to get item meta");
            continue;
          }
        } else {
          file = create_api_file(
              entry_api_path,
              directory ? 0U : entry["size"].get<std::uint64_t>(), meta);
        }

        directory_item dir_item{};
        dir_item.api_parent = file.api_parent;
        dir_item.api_path = file.api_path;
        dir_item.directory = directory;
        dir_item.meta = meta;
        dir_item.resolved = true;
        dir_item.size = file.file_size;
        list.emplace_back(std::move(dir_item));
      } catch (const std::exception &e) {
        utils::error::raise_api_path_error(function_name, api_path, e,
                                           "failed to process entry|" +
                                               entry.dump());
      }
    }
  }

  return api_error::success;
}

auto sia_provider::get_file(const std::string &api_path, api_file &file) const
    -> api_error {
  json file_data{};
  auto res = get_object_info(api_path, file_data);
  if (res != api_error::success) {
    return res;
  }

  auto slabs = file_data["object"]["Slabs"];
  auto size =
      std::accumulate(slabs.begin(), slabs.end(), std::uint64_t(0U),
                      [](std::uint64_t total_size,
                         const nlohmann::json &slab) -> std::uint64_t {
                        return total_size + slab["Length"].get<std::uint64_t>();
                      });

  api_meta_map meta{};
  if (get_item_meta(api_path, meta) == api_error::item_not_found) {
    file = create_api_file(api_path, "", size,
                           get_last_modified(file_data["object"]));
    get_api_item_added()(false, file);
  } else {
    file = create_api_file(api_path, size, meta);
  }

  return api_error::success;
}

auto sia_provider::get_file_list(api_file_list &list,
                                 std::string & /* marker */) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  using dir_func = std::function<api_error(std::string api_path)>;
  const dir_func get_files_in_dir = [&](std::string api_path) -> api_error {
    try {
      nlohmann::json object_list{};
      if (not get_object_list(api_path, object_list)) {
        return api_error::comm_error;
      }

      if (object_list.contains("entries")) {
        for (const auto &entry : object_list.at("entries")) {
          auto name = entry.at("name").get<std::string>();
          auto entry_api_path = utils::path::create_api_path(name);

          if (utils::string::ends_with(name, "/")) {
            if (entry_api_path == utils::path::create_api_path(api_path)) {
              continue;
            }

            api_meta_map meta{};
            if (get_item_meta(entry_api_path, meta) ==
                api_error::item_not_found) {
              auto dir = create_api_file(entry_api_path, "", 0U,
                                         get_last_modified(entry));
              get_api_item_added()(true, dir);
            }

            auto res = get_files_in_dir(entry_api_path);
            if (res != api_error::success) {
              return res;
            }
            continue;
          }

          api_file file{};
          api_meta_map meta{};
          if (get_item_meta(entry_api_path, meta) ==
              api_error::item_not_found) {
            file = create_api_file(entry_api_path, "",
                                   entry["size"].get<std::uint64_t>(),
                                   get_last_modified(entry));
            get_api_item_added()(false, file);
          } else {
            file = create_api_file(entry_api_path,
                                   entry["size"].get<std::uint64_t>(), meta);
          }

          list.emplace_back(std::move(file));
        }
      }

      return api_error::success;
    } catch (const std::exception &e) {
      utils::error::raise_error(function_name, e,
                                "failed to process directory|" + api_path);
    }

    return api_error::error;
  };

  return get_files_in_dir("");
}

auto sia_provider::get_object_info(const std::string &api_path,
                                   json &object_info) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    curl::requests::http_get get{};
    get.allow_timeout = true;
    get.path = "/api/bus/objects" + api_path;
    get.query["bucket"] = get_bucket(get_sia_config());

    get.response_handler = [&object_info](const data_buffer &data,
                                          long response_code) {
      if (response_code == http_error_codes::ok) {
        object_info = nlohmann::json::parse(data.begin(), data.end());
      }
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
      utils::error::raise_api_path_error(function_name, api_path, response_code,
                                         "failed to get object info");
      return api_error::comm_error;
    }

    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to get object info");
  }

  return api_error::error;
}

auto sia_provider::get_object_list(const std::string &api_path,
                                   nlohmann::json &object_list) const -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  curl::requests::http_get get{};
  get.allow_timeout = true;
  get.path = "/api/bus/objects" + api_path + "/";
  get.query["bucket"] = get_bucket(get_sia_config());

  get.response_handler = [&object_list](const data_buffer &data,
                                        long response_code) {
    if (response_code == http_error_codes::ok) {
      object_list = nlohmann::json::parse(data.begin(), data.end());
    }
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
    utils::error::raise_api_path_error(function_name, api_path, response_code,
                                       "failed to get object list");
    return false;
  }

  return true;
}

auto sia_provider::get_total_drive_space() const -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    curl::requests::http_get get{};
    get.allow_timeout = true;
    get.path = "/api/autopilot/config";
    get.query["bucket"] = get_bucket(get_sia_config());

    json config_data{};
    get.response_handler = [&config_data](const data_buffer &data,
                                          long response_code) {
      if (response_code == http_error_codes::ok) {
        config_data = nlohmann::json::parse(data.begin(), data.end());
      }
    };

    long response_code{};
    stop_type stop_requested{};
    if (not get_comm().make_request(get, response_code, stop_requested)) {
      return 0U;
    }

    if (response_code != http_error_codes::ok) {
      utils::error::raise_error(function_name, response_code,
                                "failed to get total drive space");
      return 0U;
    }

    return config_data["contracts"]["storage"].get<std::uint64_t>();
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e,
                              "failed to get total drive space");
  }

  return 0U;
}

auto sia_provider::is_directory(const std::string &api_path, bool &exists) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  if (api_path == "/") {
    exists = true;
    return api_error::success;
  }

  exists = false;

  try {
    json object_list{};
    if (not get_object_list(utils::path::get_parent_api_path(api_path),
                            object_list)) {
      return api_error::comm_error;
    }

    exists = object_list.contains("entries") &&
             std::find_if(object_list.at("entries").begin(),
                          object_list.at("entries").end(),
                          [&api_path](const auto &entry) -> bool {
                            return entry.at("name") == (api_path + "/");
                          }) != object_list.at("entries").end();
    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to determine path is directory");
  }

  return api_error::error;
}

auto sia_provider::is_file(const std::string &api_path, bool &exists) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  exists = false;
  if (api_path == "/") {
    return api_error::success;
  }

  try {
    json file_data{};
    auto res = get_object_info(api_path, file_data);
    if (res == api_error::item_not_found) {
      return api_error::success;
    }

    if (res != api_error::success) {
      return res;
    }

    exists = not file_data.contains("entries");
    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to determine path is directory");
  }

  return api_error::error;
}

auto sia_provider::is_online() const -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    curl::requests::http_get get{};
    get.allow_timeout = true;
    get.path = "/api/bus/consensus/state";
    get.query["bucket"] = get_bucket(get_sia_config());

    json state_data{};
    get.response_handler = [&state_data](const data_buffer &data,
                                         long response_code) {
      if (response_code == http_error_codes::ok) {
        state_data = nlohmann::json::parse(data.begin(), data.end());
      }
    };

    long response_code{};
    stop_type stop_requested{};
    if (not get_comm().make_request(get, response_code, stop_requested)) {
      utils::error::raise_error(function_name, api_error::comm_error,
                                "failed to determine if provider is online");
      return false;
    }

    if (response_code != http_error_codes::ok) {
      utils::error::raise_error(function_name, response_code,
                                "failed to determine if provider is online");
      return false;
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e,
                              "failed to determine if provider is online");
  }

  return false;
}

auto sia_provider::read_file_bytes(const std::string &api_path,
                                   std::size_t size, std::uint64_t offset,
                                   data_buffer &buffer,
                                   stop_type &stop_requested) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  curl::requests::http_get get{};
  get.path = "/api/worker/objects" + api_path;
  get.query["bucket"] = get_bucket(get_sia_config());
  get.range = {{
      offset,
      offset + size - 1U,
  }};
  get.response_handler = [&buffer](const data_buffer &data,
                                   long /*response_code*/) { buffer = data; };

  auto res = api_error::comm_error;
  for (std::uint32_t idx = 0U;
       not stop_requested && res != api_error::success &&
       idx < get_config().get_retry_read_count() + 1U;
       ++idx) {
    long response_code{};
    const auto notify_retry = [&]() {
      if (response_code == 0) {
        utils::error::raise_api_path_error(
            function_name, api_path, api_error::comm_error,
            "read file bytes failed|offset|" + std::to_string(offset) +
                "|size|" + std::to_string(size) + "|retry|" +
                std::to_string(idx + 1U));
      } else {
        utils::error::raise_api_path_error(
            function_name, api_path, response_code,
            "read file bytes failed|offset|" + std::to_string(offset) +
                "|size|" + std::to_string(size) + "|retry|" +
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
}

auto sia_provider::remove_directory_impl(const std::string &api_path)
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  curl::requests::http_delete del{};
  del.allow_timeout = true;
  del.path = "/api/bus/objects" + api_path + "/";
  del.query["bucket"] = get_bucket(get_sia_config());

  long response_code{};
  stop_type stop_requested{};
  if (not get_comm().make_request(del, response_code, stop_requested)) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::comm_error,
                                       "failed to remove directory");
    return api_error::comm_error;
  }

  if (response_code != http_error_codes::ok) {
    utils::error::raise_api_path_error(function_name, api_path, response_code,
                                       "failed to remove directory");
    return api_error::comm_error;
  }

  return api_error::success;
}

auto sia_provider::remove_file_impl(const std::string &api_path) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  curl::requests::http_delete del{};
  del.allow_timeout = true;
  del.path = "/api/bus/objects" + api_path;
  del.query["bucket"] = get_bucket(get_sia_config());

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
    utils::error::raise_api_path_error(function_name, api_path, response_code,
                                       "failed to remove file");
    return api_error::comm_error;
  }

  return api_error::success;
}

auto sia_provider::rename_file(const std::string &from_api_path,
                               const std::string &to_api_path) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  curl::requests::http_post post{};
  post.json = nlohmann::json({
      {"from", from_api_path},
      {"to", to_api_path},
      {"mode", "single"},
  });
  post.path = "/api/bus/objects/rename";
  post.query["bucket"] = get_bucket(get_sia_config());

  long response_code{};
  stop_type stop_requested{};
  if (not get_comm().make_request(post, response_code, stop_requested)) {
    utils::error::raise_api_path_error(
        function_name, from_api_path + '|' + to_api_path, api_error::comm_error,
        "failed to rename file");
    return api_error::comm_error;
  }

  if (response_code < http_error_codes::ok ||
      response_code >= http_error_codes::multiple_choices) {
    utils::error::raise_api_path_error(
        function_name, from_api_path + '|' + to_api_path, response_code,
        "failed to rename file file");
    return api_error::comm_error;
  }

  return get_db().rename_item_meta(from_api_path, to_api_path);
}

auto sia_provider::start(api_item_added_callback api_item_added,
                         i_file_manager *mgr) -> bool {
  event_system::instance().raise<service_started>("sia_provider");
  sia_config_ = get_config().get_sia_config();
  return base_provider::start(api_item_added, mgr);
}

void sia_provider::stop() {
  event_system::instance().raise<service_shutdown_begin>("sia_provider");
  base_provider::stop();
  event_system::instance().raise<service_shutdown_end>("sia_provider");
}

auto sia_provider::upload_file_impl(const std::string &api_path,
                                    const std::string &source_path,
                                    stop_type &stop_requested) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  curl::requests::http_put_file put_file{};
  put_file.path = "/api/worker/objects" + api_path;
  put_file.query["bucket"] = get_bucket(get_sia_config());
  put_file.source_path = source_path;

  long response_code{};
  if (not get_comm().make_request(put_file, response_code, stop_requested)) {
    utils::error::raise_api_path_error(function_name, api_path, source_path,
                                       api_error::comm_error,
                                       "failed to upload file");
    return api_error::comm_error;
  }

  if (response_code != http_error_codes::ok) {
    utils::error::raise_api_path_error(function_name, api_path, source_path,
                                       response_code, "failed to upload file");
    return api_error::comm_error;
  }

  return api_error::success;
}
} // namespace repertory
