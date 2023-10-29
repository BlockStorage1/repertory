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
#include "providers/sia/sia_provider.hpp"

#include "app_config.hpp"
#include "comm/i_http_comm.hpp"
#include "db/meta_db.hpp"
#include "events/events.hpp"
#include "file_manager/i_file_manager.hpp"
#include "types/repertory.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/polling.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
sia_provider::sia_provider(app_config &config, i_http_comm &comm)
    : config_(config), comm_(comm) {}

auto sia_provider::get_object_info(const std::string &api_path,
                                   json &object_info) const -> api_error {
  try {
    curl::requests::http_get get{};
    get.allow_timeout = true;
    get.path = "/api/bus/objects" + api_path;

    get.response_handler = [&object_info](const data_buffer &data,
                                          long response_code) {
      if (response_code == 200) {
        object_info = nlohmann::json::parse(data.begin(), data.end());
      }
    };

    long response_code{};
    stop_type stop_requested{};
    if (not comm_.make_request(get, response_code, stop_requested)) {
      return api_error::comm_error;
    }

    if (response_code == 404) {
      return api_error::item_not_found;
    }

    if (response_code != 200) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, response_code,
                                         "failed to get object info");
      return api_error::comm_error;
    }

    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                       "failed to get object info");
  }

  return api_error::error;
}

auto sia_provider::get_object_list(const std::string api_path,
                                   nlohmann::json &object_list) const -> bool {
  curl::requests::http_get get{};
  get.allow_timeout = true;
  get.path = "/api/bus/objects" + api_path + "/";

  get.response_handler = [&object_list](const data_buffer &data,
                                        long response_code) {
    if (response_code == 200) {
      object_list = nlohmann::json::parse(data.begin(), data.end());
    }
  };

  long response_code{};
  stop_type stop_requested{};
  if (not comm_.make_request(get, response_code, stop_requested)) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       api_error::comm_error,
                                       "failed to get object list");
    return false;
  }

  if (response_code != 200) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, response_code,
                                       "failed to get object list");
    return false;
  }

  return true;
}

auto sia_provider::create_api_file(std::string path, std::uint64_t size)
    -> api_file {
  api_file file{};
  file.api_path = utils::path::create_api_path(path);
  file.api_parent = utils::path::get_parent_api_path(file.api_path);
  file.accessed_date = utils::get_file_time_now();
  file.changed_date = utils::get_file_time_now();
  file.creation_date = utils::get_file_time_now();
  file.modified_date = utils::get_file_time_now();
  file.file_size = size;
  return file;
}

auto sia_provider::create_api_file(std::string path, std::uint64_t size,
                                   api_meta_map &meta) -> api_file {
  auto current_size = utils::string::to_uint64(meta[META_SIZE]);
  if (current_size == 0U) {
    current_size = size;
  }

  api_file file{};
  file.api_path = utils::path::create_api_path(path);
  file.api_parent = utils::path::get_parent_api_path(file.api_path);
  file.accessed_date = utils::string::to_uint64(meta[META_ACCESSED]);
  file.changed_date = utils::string::to_uint64(meta[META_CHANGED]);
  file.creation_date = utils::string::to_uint64(meta[META_CREATION]);
  file.file_size = current_size;
  file.modified_date = utils::string::to_uint64(meta[META_MODIFIED]);
  return file;
}

auto sia_provider::create_directory(const std::string &api_path,
                                    api_meta_map &meta) -> api_error {
  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       api_error::directory_exists,
                                       "failed to create directory");
    return api_error::directory_exists;
  }

  res = is_file(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       api_error::item_exists,
                                       "failed to create directory");
    return api_error::item_exists;
  }

  try {
    curl::requests::http_put_file put_file{};
    put_file.file_name =
        *(utils::string::split(api_path, '/', false).end() - 1u);
    put_file.path = "/api/worker/objects" + api_path + "/";

    long response_code{};
    stop_type stop_requested{};
    if (not comm_.make_request(put_file, response_code, stop_requested)) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                         api_error::comm_error,
                                         "failed to create directory");
      return api_error::comm_error;
    }

    if (response_code != 200) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, response_code,
                                         "failed to create directory");
      return api_error::comm_error;
    }
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                       "failed to create directory");
    return api_error::error;
  }

  meta[META_DIRECTORY] = utils::string::from_bool(true);
  return set_item_meta(api_path, meta);
}

auto sia_provider::create_directory_clone_source_meta(
    const std::string &source_api_path, const std::string &api_path)
    -> api_error {
  bool exists{};
  auto res = is_file(source_api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       api_error::item_exists,
                                       "failed to create directory");
    return api_error::item_exists;
  }

  res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       api_error::directory_exists,
                                       "failed to create directory");
    return api_error::directory_exists;
  }

  res = is_file(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       api_error::item_exists,
                                       "failed to create directory");
    return api_error::item_exists;
  }

  api_meta_map meta{};
  res = get_item_meta(source_api_path, meta);
  if (res != api_error::success) {
    if (res == api_error::item_not_found) {
      res = api_error::directory_not_found;
    }
    utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                       "failed to create directory");
    return res;
  }

  return create_directory(api_path, meta);
}

auto sia_provider::create_file(const std::string &api_path, api_meta_map &meta)
    -> api_error {
  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success && res != api_error::item_not_found) {
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       api_error::directory_exists,
                                       "failed to create file");
    return api_error::directory_exists;
  }

  res = is_file(api_path, exists);
  if (res != api_error::success && res != api_error::item_not_found) {
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       api_error::item_exists,
                                       "failed to create file");
    return api_error::item_exists;
  }

  try {
    curl::requests::http_put_file put_file{};
    put_file.file_name =
        *(utils::string::split(api_path, '/', false).end() - 1u);
    put_file.path = "/api/worker/objects" + api_path;

    long response_code{};
    stop_type stop_requested{};
    if (not comm_.make_request(put_file, response_code, stop_requested)) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                         api_error::comm_error,
                                         "failed to create file");
      return api_error::comm_error;
    }

    if (response_code != 200) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, response_code,
                                         "failed to create file");
      return api_error::comm_error;
    }
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                       "failed to create file");
    return api_error::error;
  }

  meta[META_DIRECTORY] = utils::string::from_bool(false);
  return set_item_meta(api_path, meta);
}

auto sia_provider::get_api_path_from_source(const std::string &source_path,
                                            std::string &api_path) const
    -> api_error {
  if (source_path.empty()) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       api_error::item_not_found,
                                       "failed to source path from api path");
    return api_error::item_not_found;
  }

  auto iterator = std::unique_ptr<rocksdb::Iterator>(
      db_->NewIterator(rocksdb::ReadOptions()));
  for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
    std::string current_source_path{};
    if (get_item_meta(iterator->key().ToString(), META_SOURCE,
                      current_source_path) != api_error::success) {
      continue;
    }

    if (current_source_path.empty()) {
      continue;
    }

    if (current_source_path == source_path) {
      api_path = iterator->key().ToString();
      return api_error::success;
    }
  }

  return api_error::item_not_found;
}

auto sia_provider::get_directory_item_count(const std::string &api_path) const
    -> std::uint64_t {
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
          utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                             "failed to process entry|" +
                                                 entry.dump());
        }
      }
    }

    return item_count;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                       "failed to get directory item count");
  }

  return 0U;
}

auto sia_provider::get_directory_items(const std::string &api_path,
                                       directory_item_list &list) const
    -> api_error {
  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (not exists) {
    return api_error::directory_not_found;
  }

  try {
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
          if (get_item_meta(entry_api_path, meta) ==
              api_error::item_not_found) {
            file = create_api_file(
                entry_api_path,
                directory ? 0U : entry["size"].get<std::uint64_t>());
            api_item_added_(directory, file);
            res = get_item_meta(entry_api_path, meta);
            if (res != api_error::success) {
              utils::error::raise_error(__FUNCTION__, res,
                                        "failed to get item meta");
              continue;
            }
          } else {
            file = create_api_file(
                entry_api_path,
                directory ? 0U : entry["size"].get<std::uint64_t>(), meta);
          }

          directory_item di{};
          di.api_parent = file.api_parent;
          di.api_path = file.api_path;
          di.directory = directory;
          di.meta = meta;
          di.resolved = true;
          di.size = file.file_size;
          list.emplace_back(std::move(di));
        } catch (const std::exception &e) {
          utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                             "failed to process entry|" +
                                                 entry.dump());
        }
      }
    }
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                       "failed to get directory items");
    return api_error::error;
  }

  std::sort(list.begin(), list.end(), [](const auto &a, const auto &b) -> bool {
    return (a.directory && not b.directory) ||
           (not(b.directory && not a.directory) &&
            (a.api_path.compare(b.api_path) < 0));
  });

  list.insert(list.begin(), directory_item{
                                "..",
                                "",
                                true,
                            });
  list.insert(list.begin(), directory_item{
                                ".",
                                "",
                                true,
                            });
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
    file = create_api_file(api_path, size);
    api_item_added_(false, file);
  } else {
    file = create_api_file(api_path, size, meta);
  }

  return api_error::success;
}

auto sia_provider::get_file_list(api_file_list &list) const -> api_error {
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
              auto dir = create_api_file(entry_api_path, 0U);
              api_item_added_(true, dir);
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
            file = create_api_file(entry_api_path,
                                   entry["size"].get<std::uint64_t>());
            api_item_added_(false, file);
          } else {
            file = create_api_file(entry_api_path,
                                   entry["size"].get<std::uint64_t>(), meta);
          }

          list.emplace_back(std::move(file));
        }
      }

      return api_error::success;
    } catch (const std::exception &e) {
      utils::error::raise_error(__FUNCTION__, e,
                                "failed to process directory|" + api_path);
    }

    return api_error::error;
  };

  return get_files_in_dir("");
}

auto sia_provider::get_file_size(const std::string &api_path,
                                 std::uint64_t &file_size) const -> api_error {
  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    return api_error::directory_exists;
  }

  api_file file{};
  res = get_file(api_path, file);
  if (res != api_error::success) {
    return res;
  }

  file_size = file.file_size;
  return api_error::success;
}

auto sia_provider::get_filesystem_item(const std::string &api_path,
                                       bool directory,
                                       filesystem_item &fsi) const
    -> api_error {
  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (directory && not exists) {
    return api_error::directory_not_found;
  }

  res = is_file(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (not directory && not exists) {
    return api_error::item_not_found;
  }

  api_meta_map meta{};
  res = get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  fsi.api_parent = utils::path::get_parent_api_path(api_path);
  fsi.api_path = api_path;
  fsi.directory = directory;
  fsi.size = fsi.directory ? 0U : utils::string::to_uint64(meta[META_SIZE]);
  fsi.source_path = meta[META_SOURCE];

  return api_error::success;
}

auto sia_provider::get_filesystem_item_and_file(const std::string &api_path,
                                                api_file &file,
                                                filesystem_item &fsi) const
    -> api_error {
  auto res = get_file(api_path, file);
  if (res != api_error::success) {
    return res;
  }

  api_meta_map meta{};
  res = get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  fsi.api_parent = utils::path::get_parent_api_path(api_path);
  fsi.api_path = api_path;
  fsi.directory = false;
  fsi.size = utils::string::to_uint64(meta[META_SIZE]);
  fsi.source_path = meta[META_SOURCE];

  return api_error::success;
}

auto sia_provider::get_filesystem_item_from_source_path(
    const std::string &source_path, filesystem_item &fsi) const -> api_error {
  std::string api_path{};
  auto res = get_api_path_from_source(source_path, api_path);
  if (res != api_error::success) {
    return res;
  }

  bool exists{};
  res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    return api_error::directory_exists;
  }

  return get_filesystem_item(api_path, false, fsi);
}

auto sia_provider::get_item_meta(const std::string &api_path,
                                 api_meta_map &meta) const -> api_error {
  std::string meta_value{};
  db_->Get(rocksdb::ReadOptions(), api_path, &meta_value);
  if (meta_value.empty()) {
    return api_error::item_not_found;
  }

  try {
    meta = json::parse(meta_value).get<api_meta_map>();

    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                       "failed to get item meta");
  }

  return api_error::error;
}

auto sia_provider::get_item_meta(const std::string &api_path,
                                 const std::string &key,
                                 std::string &value) const -> api_error {
  std::string meta_value{};
  db_->Get(rocksdb::ReadOptions(), api_path, &meta_value);
  if (meta_value.empty()) {
    return api_error::item_not_found;
  }

  try {
    value = json::parse(meta_value)[key];
    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                       "failed to get item meta");
  }

  return api_error::error;
}

auto sia_provider::get_pinned_files() const -> std::vector<std::string> {
  std::vector<std::string> ret{};

  auto iterator = std::unique_ptr<rocksdb::Iterator>(
      db_->NewIterator(rocksdb::ReadOptions()));
  for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
    std::string pinned{};
    if (get_item_meta(iterator->key().ToString(), META_PINNED, pinned) !=
        api_error::success) {
      continue;
    }
    if (pinned.empty() || not utils::string::to_bool(pinned)) {
      continue;
    }
    ret.emplace_back(iterator->key().ToString());
  }

  return ret;
}

auto sia_provider::get_total_drive_space() const -> std::uint64_t {
  try {
    curl::requests::http_get get{};
    get.allow_timeout = true;
    get.path = "/api/autopilot/config";

    json config_data{};
    get.response_handler = [&config_data](const data_buffer &data,
                                          long response_code) {
      if (response_code == 200) {
        config_data = nlohmann::json::parse(data.begin(), data.end());
      }
    };

    long response_code{};
    stop_type stop_requested{};
    if (not comm_.make_request(get, response_code, stop_requested)) {
      return 0U;
    }

    if (response_code != 200) {
      utils::error::raise_error(__FUNCTION__, response_code,
                                "failed to get total drive space");
      return 0U;
    }

    return config_data["contracts"]["storage"].get<std::uint64_t>();
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e,
                              "failed to get total drive space");
  }

  return 0U;
}

auto sia_provider::get_total_item_count() const -> std::uint64_t {
  std::uint64_t ret{};
  auto iterator = std::unique_ptr<rocksdb::Iterator>(
      db_->NewIterator(rocksdb::ReadOptions()));
  for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
    ret++;
  }
  return ret;
}

auto sia_provider::get_used_drive_space() const -> std::uint64_t {
  // TODO adjust size based on open files
  try {
    curl::requests::http_get get{};
    get.allow_timeout = true;
    get.path = "/api/bus/stats/objects";

    json object_data{};
    get.response_handler = [&object_data](const data_buffer &data,
                                          long response_code) {
      if (response_code == 200) {
        object_data = nlohmann::json::parse(data.begin(), data.end());
      }
    };

    long response_code{};
    stop_type stop_requested{};
    if (not comm_.make_request(get, response_code, stop_requested)) {
      return 0U;
    }

    if (response_code != 200) {
      utils::error::raise_error(__FUNCTION__, response_code,
                                "failed to get used drive space");
      return 0U;
    }

    auto used_space = object_data["totalObjectsSize"].get<std::uint64_t>();
    fm_->update_used_space(used_space);
    return used_space;
  } catch (const std::exception &ex) {
    utils::error::raise_error(__FUNCTION__, ex,
                              "failed to get used drive space");
  }

  return 0U;
}

auto sia_provider::is_directory(const std::string &api_path, bool &exists) const
    -> api_error {
  if (api_path == "/") {
    exists = true;
    return api_error::success;
  }

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
    utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                       "failed to determine path is directory");
  }

  return api_error::error;
}

auto sia_provider::is_file(const std::string &api_path, bool &exists) const
    -> api_error {
  exists = false;

  if (api_path == "/") {
    return api_error::success;
  }

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
}

auto sia_provider::is_file_writeable(const std::string &api_path) const
    -> bool {
  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return false;
  }

  return not exists;
}

auto sia_provider::is_online() const -> bool {
  try {
    curl::requests::http_get get{};
    get.allow_timeout = true;
    get.path = "/api/bus/consensus/state";

    json state_data{};
    get.response_handler = [&state_data](const data_buffer &data,
                                         long response_code) {
      if (response_code == 200) {
        state_data = nlohmann::json::parse(data.begin(), data.end());
      }
    };

    long response_code{};
    stop_type stop_requested{};
    if (not comm_.make_request(get, response_code, stop_requested)) {
      utils::error::raise_error(__FUNCTION__, api_error::comm_error,
                                "failed to determine if provider is online");
      return false;
    }

    if (response_code != 200) {
      utils::error::raise_error(__FUNCTION__, response_code,
                                "failed to determine if provider is online");
      return false;
    }

    event_system::instance().raise<debug_log>(__FUNCTION__, "",
                                              state_data.dump());
    return true;
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e,
                              "failed to determine if provider is online");
  }

  return false;
}

auto sia_provider::read_file_bytes(const std::string &api_path,
                                   std::size_t size, std::uint64_t offset,
                                   data_buffer &buffer,
                                   stop_type &stop_requested) -> api_error {
  curl::requests::http_get get{};
  get.path = "/api/worker/objects" + api_path;
  get.range = {{offset, offset + size - 1U}};
  get.response_handler = [&buffer](const data_buffer &data,
                                   long /*response_code*/) { buffer = data; };

  auto res = api_error::comm_error;
  for (std::uint32_t i = 0U; not stop_requested && res != api_error::success &&
                             i < config_.get_retry_read_count() + 1U;
       i++) {
    long response_code{};
    const auto notify_retry = [&]() {
      if (response_code) {
        utils::error::raise_api_path_error(
            __FUNCTION__, api_path, response_code,
            "read file bytes failed|offset|" + std::to_string(offset) +
                "|size|" + std::to_string(size) + "|retry|" +
                std::to_string(i + 1U));
      } else {
        utils::error::raise_api_path_error(
            __FUNCTION__, api_path, api_error::comm_error,
            "read file bytes failed|offset|" + std::to_string(offset) +
                "|size|" + std::to_string(size) + "|retry|" +
                std::to_string(i + 1U));
      }
      std::this_thread::sleep_for(1s);
    };

    if (not comm_.make_request(get, response_code, stop_requested)) {
      notify_retry();
      continue;
    }

    if (response_code < 200 || response_code >= 300) {
      notify_retry();
      continue;
    }

    res = api_error::success;
  }

  return res;
}

void sia_provider::remove_deleted_files() {
  struct removed_item {
    std::string api_path{};
    bool directory{};
    std::string source_path{};
  };

  api_file_list list{};
  auto res = get_file_list(list);
  if (res != api_error::success) {
    utils::error::raise_error(__FUNCTION__, res, "failed to get file list");
    return;
  }

  std::vector<removed_item> removed_list{};
  auto iterator = std::unique_ptr<rocksdb::Iterator>(
      db_->NewIterator(rocksdb::ReadOptions()));
  for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
    api_meta_map meta{};
    if (get_item_meta(iterator->key().ToString(), meta) == api_error::success) {
      if (utils::string::to_bool(meta[META_DIRECTORY])) {
        bool exists{};
        auto res = is_directory(iterator->key().ToString(), exists);
        if (res != api_error::success) {
          continue;
        }
        if (not exists) {
          removed_list.emplace_back(
              removed_item{iterator->key().ToString(), true, ""});
        }
        continue;
      }

      bool exists{};
      auto res = is_file(iterator->key().ToString(), exists);
      if (res != api_error::success) {
        continue;
      }
      if (not exists) {
        removed_list.emplace_back(
            removed_item{iterator->key().ToString(), false, meta[META_SOURCE]});
      }
    }
  }

  for (const auto &item : removed_list) {
    if (not item.directory) {
      if (utils::file::is_file(item.source_path)) {
        const auto orphaned_directory =
            utils::path::combine(config_.get_data_directory(), {"orphaned"});
        if (utils::file::create_full_directory_path(orphaned_directory)) {
          const auto parts = utils::string::split(item.api_path, '/', false);
          const auto orphaned_file = utils::path::combine(
              orphaned_directory,
              {utils::path::strip_to_file_name(item.source_path) + '_' +
               parts[parts.size() - 1U]});

          event_system::instance().raise<orphaned_file_detected>(
              item.source_path);
          if (utils::file::reset_modified_time(item.source_path) &&
              utils::file::copy_file(item.source_path, orphaned_file)) {
            event_system::instance().raise<orphaned_file_processed>(
                item.source_path, orphaned_file);
          } else {
            event_system::instance().raise<orphaned_file_processing_failed>(
                item.source_path, orphaned_file,
                std::to_string(utils::get_last_error_code()));
          }
        } else {
          utils::error::raise_error(
              __FUNCTION__, std::to_string(utils::get_last_error_code()),
              "failed to create orphaned director|sp|" + orphaned_directory);
          continue;
        }
      }

      if (fm_->evict_file(item.api_path)) {
        db_->Delete(rocksdb::WriteOptions(), item.api_path);
        event_system::instance().raise<file_removed_externally>(
            item.api_path, item.source_path);
      }
    }
  }

  for (const auto &item : removed_list) {
    if (item.directory) {
      db_->Delete(rocksdb::WriteOptions(), item.api_path);
      event_system::instance().raise<directory_removed_externally>(
          item.api_path, item.source_path);
    }
  }
}

auto sia_provider::remove_directory(const std::string &api_path) -> api_error {
  const auto notify_end = [&api_path](api_error error) -> api_error {
    if (error == api_error::success) {
      event_system::instance().raise<directory_removed>(api_path);
    } else {
      event_system::instance().raise<directory_remove_failed>(
          api_path, api_error_to_string(error));
    }
    return error;
  };

  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (not exists) {
    return notify_end(api_error::item_not_found);
  }

  curl::requests::http_delete del{};
  del.allow_timeout = true;
  del.path = "/api/bus/objects" + api_path + "/";

  long response_code{};
  stop_type stop_requested{};
  if (not comm_.make_request(del, response_code, stop_requested)) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       api_error::comm_error,
                                       "failed to remove directory");
    return notify_end(api_error::comm_error);
  }

  if (response_code != 200) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, response_code,
                                       "failed to remove directory");
    return notify_end(api_error::comm_error);
  }

  auto res2 = db_->Delete(rocksdb::WriteOptions(), api_path);
  if (not res2.ok()) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, res2.code(),
                                       "failed to remove directory");
    return notify_end(api_error::error);
  }

  return notify_end(api_error::success);
}

auto sia_provider::remove_file(const std::string &api_path) -> api_error {
  const auto notify_end = [&api_path](api_error error) -> api_error {
    if (error == api_error::success) {
      event_system::instance().raise<file_removed>(api_path);
    } else {
      event_system::instance().raise<file_remove_failed>(
          api_path, api_error_to_string(error));
    }

    return error;
  };

  const auto remove_file_meta = [this, &api_path, &notify_end]() -> api_error {
    api_meta_map meta{};
    auto res = get_item_meta(api_path, meta);

    auto res2 = db_->Delete(rocksdb::WriteOptions(), api_path);
    if (not res2.ok()) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, res2.code(),
                                         "failed to remove file");
      return notify_end(api_error::error);
    }

    return notify_end(res);
  };

  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return notify_end(res);
  }
  if (exists) {
    exists = false;
    return notify_end(api_error::directory_exists);
  }

  res = is_file(api_path, exists);
  if (res != api_error::success) {
    return notify_end(res);
  }
  if (not exists) {
    event_system::instance().raise<file_remove_failed>(
        api_path, api_error_to_string(api_error::item_not_found));
    return remove_file_meta();
  }

  curl::requests::http_delete del{};
  del.allow_timeout = true;
  del.path = "/api/bus/objects" + api_path;

  long response_code{};
  stop_type stop_requested{};
  if (not comm_.make_request(del, response_code, stop_requested)) {
    utils::error::raise_api_path_error(
        __FUNCTION__, api_path, api_error::comm_error, "failed to remove file");
    return notify_end(api_error::comm_error);
  }

  if (response_code != 200 && response_code != 404) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, response_code,
                                       "failed to remove file");
    return notify_end(api_error::comm_error);
  }

  return remove_file_meta();
}

auto sia_provider::remove_item_meta(const std::string &api_path,
                                    const std::string &key) -> api_error {
  api_meta_map meta{};
  auto res = get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  meta.erase(key);

  auto res2 = db_->Put(rocksdb::WriteOptions(), api_path, json(meta).dump());
  if (not res2.ok()) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, res2.code(),
                                       "failed to remove item meta");
    return api_error::error;
  }

  return api_error::success;
}

auto sia_provider::rename_file(const std::string & /*from_api_path*/,
                               const std::string & /*to_api_path*/)
    -> api_error {
  return api_error::not_implemented;
}

auto sia_provider::set_item_meta(const std::string &api_path,
                                 const std::string &key,
                                 const std::string &value) -> api_error {
  json meta_json{};
  std::string meta_value{};
  db_->Get(rocksdb::ReadOptions(), api_path, &meta_value);
  if (not meta_value.empty()) {
    try {
      meta_json = json::parse(meta_value);
    } catch (const std::exception &e) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                         "failed to set item meta");
      return api_error::error;
    }
  }

  meta_json[key] = value;

  const auto res =
      db_->Put(rocksdb::WriteOptions(), api_path, meta_json.dump());
  if (not res.ok()) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, res.code(),
                                       "failed to set item meta");
    return api_error::error;
  }

  return api_error::success;
}

auto sia_provider::set_item_meta(const std::string &api_path,
                                 const api_meta_map &meta) -> api_error {
  json meta_json{};
  std::string meta_value{};
  db_->Get(rocksdb::ReadOptions(), api_path, &meta_value);
  if (not meta_value.empty()) {
    try {
      meta_json = json::parse(meta_value);
    } catch (const std::exception &e) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                         "failed to set item meta");
      return api_error::error;
    }
  }

  for (const auto &kv : meta) {
    meta_json[kv.first] = kv.second;
  }

  const auto res =
      db_->Put(rocksdb::WriteOptions(), api_path, meta_json.dump());
  if (not res.ok()) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, res.code(),
                                       "failed to set item meta");
    return api_error::error;
  }

  return api_error::success;
}

auto sia_provider::start(api_item_added_callback api_item_added,
                         i_file_manager *fm) -> bool {
  event_system::instance().raise<service_started>("sia_provider");
  utils::db::create_rocksdb(config_, DB_NAME, db_);

  api_item_added_ = api_item_added;
  fm_ = fm;

  api_meta_map meta{};
  if (get_item_meta("/", meta) == api_error::item_not_found) {
    auto dir = create_api_file("/", 0U);
    api_item_added_(true, dir);
  }

  auto online = false;
  auto unmount_requested = false;
  {
    repertory::event_consumer ec(
        "unmount_requested",
        [&unmount_requested](const event &) { unmount_requested = true; });
    for (std::uint16_t i = 0u; not online && not unmount_requested &&
                               (i < config_.get_online_check_retry_secs());
         i++) {
      online = is_online();
      if (not online) {
        event_system::instance().raise<provider_offline>(
            config_.get_host_config().host_name_or_ip,
            config_.get_host_config().api_port);
        std::this_thread::sleep_for(1s);
      }
    }
  }

  if (online && not unmount_requested) {
    polling::instance().set_callback({"check_deleted", polling::frequency::low,
                                      [this]() { remove_deleted_files(); }});
    return true;
  }

  return false;
}

void sia_provider::stop() {
  event_system::instance().raise<service_shutdown_begin>("sia_provider");
  polling::instance().remove_callback("check_deleted");
  db_.reset();
  event_system::instance().raise<service_shutdown_end>("sia_provider");
}

auto sia_provider::upload_file(const std::string &api_path,
                               const std::string &source_path,
                               const std::string & /* encryption_token */,
                               stop_type &stop_requested) -> api_error {
  event_system::instance().raise<provider_upload_begin>(api_path, source_path);

  const auto notify_end = [&api_path,
                           &source_path](api_error error) -> api_error {
    event_system::instance().raise<provider_upload_end>(api_path, source_path,
                                                        error);
    return error;
  };

  try {
    curl::requests::http_put_file put_file{};
    put_file.file_name =
        *(utils::string::split(api_path, '/', false).end() - 1u);
    put_file.path = "/api/worker/objects" + api_path;
    put_file.source_path = source_path;

    long response_code{};
    if (not comm_.make_request(put_file, response_code, stop_requested)) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, source_path,
                                         api_error::comm_error,
                                         "failed to upload file");
      return notify_end(api_error::comm_error);
    }

    if (response_code != 200) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, source_path,
                                         response_code,
                                         "failed to upload file");
      return notify_end(api_error::comm_error);
    }

    return notify_end(api_error::success);
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, source_path, e,
                                       "failed to upload file");
  }

  return notify_end(api_error::error);
}
} // namespace repertory
