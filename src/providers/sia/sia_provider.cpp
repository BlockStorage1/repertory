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
#include "providers/sia/sia_provider.hpp"
#include "comm/i_comm.hpp"
#include "app_config.hpp"
#include "db/meta_db.hpp"
#include "drives/i_open_file_table.hpp"
#include "events/events.hpp"
#include "types/repertory.hpp"
#include "types/startup_exception.hpp"
#include "utils/file_utils.hpp"
#include "utils/global_data.hpp"
#include "utils/path_utils.hpp"
#include "utils/polling.hpp"

namespace repertory {
sia_provider::sia_provider(app_config &config, i_comm &comm) : base_provider(config), comm_(comm) {}

void sia_provider::calculate_total_drive_space() {
  std::uint64_t ret = 0u;
  json result, error;
  auto storage_cost = get_config().get_storage_byte_month();
  const auto success = (get_comm().get("/renter/prices", result, error) == api_error::success);
  if (success || (storage_cost > 0)) {
    if (success) {
      storage_cost = result["storageterabytemonth"].get<std::string>();
      get_config().set_storage_byte_month(storage_cost);
    }
    if (get_comm().get("/renter", result, error) == api_error::success) {
      const auto funds = utils::hastings_string_to_api_currency(
          result["settings"]["allowance"]["funds"].get<std::string>());
      if ((storage_cost > 0) && (funds > 0)) {
        const auto funds_in_hastings = utils::api_currency_to_hastings(funds);
        ttmath::Parser<api_currency> parser;
        parser.Parse(funds_in_hastings.ToString() + " / " + storage_cost.ToString() + " * 1e12");
        ret = not parser.stack.empty() ? parser.stack[0u].value.ToUInt() : 0u;
      }
    }
  }

  total_drive_space_ = ret;
}

api_error sia_provider::calculate_used_drive_space(std::uint64_t &used_space) {
  api_file_list list;
  const auto ret = get_file_list(list);
  used_space = std::accumulate(
      list.begin(), list.end(), std::uint64_t(0), [this](std::uint64_t a, const auto &v) {
        if (not meta_db_.get_item_meta_exists(v.api_path)) {
          this->notify_file_added(v.api_path, utils::path::get_parent_api_path(v.api_path), 0);
        }
        return a + v.file_size;
      });
  return ret;
}

api_error sia_provider::check_file_exists(const std::string &api_path) const {
  json data, error;
  auto ret = get_comm().get("/renter/file" + api_path, data, error);
  if ((ret != api_error::success) && check_not_found(error)) {
    ret = api_error::item_not_found;
  }

  return ret;
}

bool sia_provider::check_directory_found(const json &error) const {
  return ((error.find("message") != error.end()) &&
          (utils::string::contains(error["message"].get<std::string>(),
                                   "a siadir already exists at that location")));
}

bool sia_provider::check_not_found(const json &error) const {
  return (
      (error.find("message") != error.end()) &&
      (utils::string::contains(error["message"].get<std::string>(), "no file known") ||
       utils::string::contains(error["message"].get<std::string>(), "path does not exist") ||
       utils::string::contains(error["message"].get<std::string>(), "no such file or directory") ||
       utils::string::contains(error["message"].get<std::string>(), "cannot find the file") ||
       utils::string::contains(error["message"].get<std::string>(),
                               "no siadir known with that path") ||
       utils::string::contains(error["message"].get<std::string>(), "cannot find the path")));
}

void sia_provider::cleanup() {
  remove_deleted_files();
  remove_unknown_source_files();
  remove_expired_orphaned_files();
}

void sia_provider::create_api_file(const json &json_file, const std::string &path_name,
                                   api_file &file) const {
  file.api_path = utils::path::create_api_path(json_file[path_name].get<std::string>());
  file.api_parent = utils::path::get_parent_api_path(file.api_path);
  file.file_size = json_file["filesize"].get<std::uint64_t>();
  file.recoverable = json_file["recoverable"].get<bool>();
  file.redundancy = json_file["redundancy"].get<double>();
  file.source_path = json_file["localpath"].get<std::string>();

  set_api_file_dates(json_file, file);
}

api_error sia_provider::create_directory(const std::string &api_path, const api_meta_map &meta) {
#ifdef _WIN32
  auto ret = is_directory(api_path) ? api_error::directory_exists
             : is_file(api_path)    ? api_error::file_exists
                                    : api_error::success;
#else
  auto ret = api_error::success;
#endif
  if (ret == api_error::success) {
    json result, error;
    ret = get_comm().post("/renter/dir" + api_path, {{"action", "create"}}, result, error);
    if (ret == api_error::success) {
      ret = set_item_meta(api_path, meta);
    } else if (check_directory_found(error)) {
      ret = api_error::directory_exists;
    } else {
      event_system::instance().raise<repertory_exception>(__FUNCTION__, error.dump(2));
    }
  }

  return ret;
}

void sia_provider::drive_space_thread() {
  while (not stop_requested_) {
    unique_mutex_lock l(start_stop_mutex_);
    if (not stop_requested_) {
      start_stop_notify_.wait_for(l, 5s);
    }
    l.unlock();

    if (not stop_requested_) {
      calculate_total_drive_space();
    }
  }
}

api_error sia_provider::get_directory(const std::string &api_path, json &result,
                                      json &error) const {
  return get_comm().get("/renter/dir" + api_path, result, error);
}

std::uint64_t sia_provider::get_directory_item_count(const std::string &api_path) const {
  std::uint64_t ret = 0u;

  json result, error;
  const auto res = get_directory(api_path, result, error);
  if (res == api_error::success) {
    const auto directory_count = result["directories"].size() - 1;
    const auto file_count = result["files"].size();
    ret = file_count + directory_count;
  }

  return ret;
}

api_error sia_provider::get_directory_items(const std::string &api_path,
                                            directory_item_list &list) const {
  list.clear();

  json result, error;
  auto ret = get_directory(api_path, result, error);
  if (ret == api_error::success) {
    const auto create_directory_item = [this](const std::string &item_path, const bool &directory,
                                              const json &item, directory_item &di) {
      di.api_path = item_path;
      di.api_parent = utils::path::get_parent_api_path(di.api_path);
      di.directory = directory;
      if (directory) {
        const auto directory_count = item["numsubdirs"].get<std::uint64_t>();
        const auto file_count = item["numfiles"].get<std::uint64_t>();
        di.size = (directory_count + file_count);
      } else {
        di.size = item["filesize"].get<std::uint64_t>();
      }
      this->get_item_meta(di.api_path, di.meta);
      this->oft_->update_directory_item(di);
    };

    const auto process_item = [&](const bool &directory, const json &item) {
      const auto item_path = utils::path::create_api_path(
          item[app_config::get_provider_path_name(get_config().get_provider_type())]
              .get<std::string>());
      const auto is_root_path = (item_path == api_path);

      directory_item di{};
      create_directory_item(item_path, directory, item, di);
      if (is_root_path) {
        di.api_path = ".";
      }
      list.emplace_back(di);
      if (is_root_path) {
        if (item_path == "/") {
          di.api_path = "..";
          list.emplace_back(di);
        } else {
          directory_item dot_dot_di{};
          json result, error;
          const auto res = get_directory(di.api_parent, result, error);
          if (res == api_error::success) {
            create_directory_item(di.api_parent, true, result["directories"][0u], dot_dot_di);
          }
          dot_dot_di.api_path = "..";
          list.emplace_back(dot_dot_di);
        }
      }
    };

    for (const auto &dir : result["directories"]) {
      process_item(true, dir);
    }

    for (const auto &file : result["files"]) {
      process_item(false, file);
    }

    ret = api_error::success;
  }

  return ret;
}

api_error sia_provider::get_file(const std::string &api_path, api_file &file) const {
  json data, error;
  const auto ret = get_comm().get("/renter/file" + api_path, data, error);
  if (ret == api_error::success) {
    const std::string path_name =
        app_config::get_provider_path_name(get_config().get_provider_type());
    create_api_file(data["file"], path_name, file);
  } else {
    event_system::instance().raise<file_get_failed>(api_path, error.dump(2));
  }

  return ret;
}

api_error sia_provider::get_file_list(api_file_list &list) const {
  auto ret = api_error::success;
  json data;
  json error;
  if ((ret = get_comm().get("/renter/files", data, error)) == api_error::success) {
    for (const auto &file_data : data["files"]) {
      const std::string path_name =
          app_config::get_provider_path_name(get_config().get_provider_type());
      api_file file{};
      create_api_file(file_data, path_name, file);

      list.emplace_back(file);
    }
  } else {
    event_system::instance().raise<file_get_api_list_failed>(error.dump(2));
  }

  return ret;
}

api_error sia_provider::get_file_size(const std::string &api_path, std::uint64_t &file_size) const {
  file_size = 0u;

  json data, error;
  const auto ret = get_comm().get("/renter/file" + api_path, data, error);
  if (ret == api_error::success) {
    file_size = data["file"]["filesize"].get<std::uint64_t>();
  } else {
    event_system::instance().raise<file_get_size_failed>(api_path, error.dump(2));
  }

  return ret;
}

api_error sia_provider::get_filesystem_item(const std::string &api_path, const bool &directory,
                                            filesystem_item &fsi) const {
  auto ret = api_error::error;
  if (directory) {
    json result, error;
    ret = get_directory(api_path, result, error);
    if (ret != api_error::success) {
      if (check_not_found(error)) {
        ret = api_error::item_not_found;
      } else {
        event_system::instance().raise<filesystem_item_get_failed>(api_path, error.dump(2));
      }
    }

    update_filesystem_item(true, ret, api_path, fsi);
  } else {
    api_file file{};
    ret = get_filesystem_item_and_file(api_path, file, fsi);
  }

  return ret;
}

api_error sia_provider::get_filesystem_item_and_file(const std::string &api_path, api_file &file,
                                                     filesystem_item &fsi) const {
  auto ret = get_item_meta(api_path, META_SOURCE, fsi.source_path);
  if (ret == api_error::success) {
    json data, error;
    ret = get_comm().get("/renter/file" + api_path, data, error);
    if (ret == api_error::success) {
      create_api_file(data["file"],
                      app_config::get_provider_path_name(get_config().get_provider_type()), file);
      fsi.size = file.file_size;
    } else if (check_not_found(error)) {
      ret = api_error::item_not_found;
    } else {
      event_system::instance().raise<filesystem_item_get_failed>(api_path, error.dump(2));
    }
  }

  update_filesystem_item(false, ret, api_path, fsi);
  return ret;
}

std::uint64_t sia_provider::get_total_item_count() const {
  std::uint64_t ret = 0u;

  json result, error;
  const auto res = get_directory("/", result, error);
  if (res == api_error::success) {
    ret = result["directories"][0u]["aggregatenumfiles"].get<std::uint64_t>();
  }

  return ret;
}

bool sia_provider::is_directory(const std::string &api_path) const {
  auto ret = false;

  json result, error;
  const auto res = (api_path == "/") ? api_error::success : get_directory(api_path, result, error);
  if (res == api_error::success) {
    ret = true;
  } else if (not check_not_found(error)) {
    event_system::instance().raise<repertory_exception>(__FUNCTION__, error.dump(2));
  }

  return ret;
}

bool sia_provider::is_file(const std::string &api_path) const {
  return (api_path != "/") && (check_file_exists(api_path) == api_error::success);
}

bool sia_provider::is_online() const {
  // TODO Expand here for true online detection (i.e. wallet unlocked)
  // TODO Return error string for display
  json data, error;
  return (get_comm().get("/wallet", data, error) == api_error::success);
}

api_error sia_provider::notify_file_added(const std::string &api_path,
                                          const std::string &api_parent,
                                          const std::uint64_t &size) {
  recur_mutex_lock l(notify_added_mutex_);

  json data;
  json error;
  auto ret = get_comm().get("/renter/file" + api_path, data, error);
  if (ret == api_error::success) {
    api_file file{};
    create_api_file(data["file"],
                    app_config::get_provider_path_name(get_config().get_provider_type()), file);

    get_api_item_added()(api_path, api_parent, file.source_path, false, file.created_date,
                         file.accessed_date, file.modified_date, file.changed_date);

    if (size) {
      global_data::instance().increment_used_drive_space(size);
    }
  } else {
    event_system::instance().raise<repertory_exception>(__FUNCTION__, error.dump(2));
  }

  return ret;
}

bool sia_provider::processed_orphaned_file(const std::string &source_path,
                                           const std::string &api_path) const {
  auto ret = false;
  if (not oft_->contains_restore(api_path)) {
    const auto orphaned_directory =
        utils::path::combine(get_config().get_data_directory(), {"orphaned"});

    event_system::instance().raise<orphaned_file_detected>(source_path);
    const auto orphaned_file = utils::path::combine(
        orphaned_directory,
        {utils::path::strip_to_file_name(api_path.empty() ? source_path : api_path)});

    if (utils::file::reset_modified_time(source_path) &&
        utils::file::move_file(source_path, orphaned_file)) {
      event_system::instance().raise<orphaned_file_processed>(source_path, orphaned_file);
      ret = true;
    } else {
      event_system::instance().raise<orphaned_file_processing_failed>(
          source_path, orphaned_file, std::to_string(utils::get_last_error_code()));
    }
  }

  return ret;
}

api_error sia_provider::read_file_bytes(const std::string &api_path, const std::size_t &size,
                                        const std::uint64_t &offset, std::vector<char> &buffer,
                                        const bool &stop_requested) {
  auto ret = api_error::download_failed;
  const auto retry_count = get_config().get_retry_read_count();
  for (std::uint16_t i = 0u; not stop_requested && (ret != api_error::success) && (i < retry_count);
       i++) {
    json error;
    ret = get_comm().get_raw("/renter/download" + api_path,
                             {{"httpresp", "true"},
                              {"async", "false"},
                              {"offset", utils::string::from_int64(offset)},
                              {"length", utils::string::from_int64(size)}},
                             buffer, error, stop_requested);
    if (ret != api_error::success) {
      event_system::instance().raise<file_read_bytes_failed>(api_path, error.dump(2), i + 1u);
      if (not stop_requested && ((i + 1) < retry_count)) {
        std::this_thread::sleep_for(1s);
      }
    }
  }

  return ret;
}

void sia_provider::remove_deleted_files() {
  std::vector<std::string> removed_files{};

  api_file_list list{};
  if (get_file_list(list) == api_error::success) {
    if (not list.empty()) {
      auto iterator = meta_db_.create_iterator(false);
      for (iterator->SeekToFirst(); not stop_requested_ && iterator->Valid(); iterator->Next()) {
        const auto api_path = iterator->key().ToString();
        if (api_path.empty()) {
          meta_db_.remove_item_meta(api_path);
        } else {
          auto it = std::find_if(list.begin(), list.end(), [&api_path](const auto &file) -> bool {
            return file.api_path == api_path;
          });
          if (it == list.end()) {
            removed_files.emplace_back(api_path);
          }
        }
      }
    }
  }

  while (not stop_requested_ && not removed_files.empty()) {
    const auto api_path = removed_files.back();
    removed_files.pop_back();

    std::string source_path;
    if (not is_directory(api_path) && (check_file_exists(api_path) == api_error::item_not_found) &&
        (meta_db_.get_item_meta(api_path, META_SOURCE, source_path) == api_error::success)) {
      if (not source_path.empty()) {
        oft_->perform_locked_operation(
            [this, &api_path, &source_path](i_open_file_table &, i_provider &) -> bool {
              if (oft_->has_no_open_file_handles()) {
                std::uint64_t used_space = 0u;
                if (calculate_used_drive_space(used_space) == api_error::success) {
                  meta_db_.remove_item_meta(api_path);
                  event_system::instance().raise<file_removed_externally>(api_path, source_path);

                  std::uint64_t fileSize = 0u;
                  if (utils::file::get_file_size(source_path, fileSize) &&
                      processed_orphaned_file(source_path, api_path)) {
                    global_data::instance().update_used_space(fileSize, 0u, true);
                  }

                  global_data::instance().initialize_used_drive_space(used_space);
                }
              }

              return true;
            });
      }
    }
  }
}

api_error sia_provider::remove_directory(const std::string &api_path) {
  auto ret = api_error::directory_not_empty;
  if (get_directory_item_count(api_path) == 0u) {
    json result, error;
    ret = get_comm().post("/renter/dir" + api_path, {{"action", "delete"}}, result, error);
    if (ret == api_error::success) {
      meta_db_.remove_item_meta(api_path);
      event_system::instance().raise<directory_removed>(api_path);
      ret = api_error::success;
    } else if (check_not_found(error)) {
      meta_db_.remove_item_meta(api_path);
      ret = api_error::directory_not_found;
    } else {
      event_system::instance().raise<directory_remove_failed>(api_path, error.dump(2));
    }
  }

  return ret;
}

void sia_provider::remove_expired_orphaned_files() {
  const auto orphaned_directory =
      utils::path::combine(get_config().get_data_directory(), {"orphaned"});
  const auto files = utils::file::get_directory_files(orphaned_directory, true);
  for (const auto &file : files) {
    if (utils::file::is_modified_date_older_than(
            file, std::chrono::hours(get_config().get_orphaned_file_retention_days() * 24))) {
      if (utils::file::delete_file(file)) {
        event_system::instance().raise<orphaned_file_deleted>(file);
      }
    }
    if (stop_requested_) {
      break;
    }
  }
}

api_error sia_provider::remove_file(const std::string &api_path) {
  json result, error;
  auto ret = get_comm().post("/renter/delete" + api_path, {{"root", "false"}}, result, error);
  auto not_found = false;
  if ((ret == api_error::success) || (not_found = check_not_found(error))) {
    if (not not_found) {
      event_system::instance().raise<file_removed>(api_path);
    }
    ret = not_found ? api_error::item_not_found : api_error::success;

    meta_db_.remove_item_meta(api_path);
  } else {
    event_system::instance().raise<file_remove_failed>(api_path, error.dump(2));
  }

  return ret;
}

void sia_provider::remove_unknown_source_files() {
  auto files = utils::file::get_directory_files(get_config().get_cache_directory(), true);
  while (not stop_requested_ && not files.empty()) {
    const auto file = files.front();
    files.pop_front();

    std::string api_path;
    if (not meta_db_.get_source_path_exists(file)) {
      processed_orphaned_file(file);
    }
  }
}

api_error sia_provider::rename_file(const std::string &from_api_path,
                                    const std::string &to_api_path) {
  std::string current_source;
  auto ret = get_item_meta(from_api_path, META_SOURCE, current_source);
  if (ret == api_error::success) {
    json result, error;
    const auto propertyName =
        "new" + app_config::get_provider_path_name(get_config().get_provider_type());
    const auto dest_api_path = to_api_path.substr(1);
    ret = get_comm().post("/renter/rename" + from_api_path, {{propertyName, dest_api_path}}, result,
                          error);
    if (ret == api_error::success) {
      meta_db_.rename_item_meta(current_source, from_api_path, to_api_path);
    } else if (check_not_found(error)) {
      ret = api_error::item_not_found;
    } else {
      event_system::instance().raise<file_rename_failed>(from_api_path, to_api_path, error.dump(2));
    }
  }

  return ret;
}

void sia_provider::set_api_file_dates(const json &file_data, api_file &file) const {
  file.accessed_date = utils::convert_api_date(file_data["accesstime"].get<std::string>());
  file.changed_date = utils::convert_api_date(file_data["changetime"].get<std::string>());
  file.created_date = utils::convert_api_date(file_data["createtime"].get<std::string>());
  file.modified_date = utils::convert_api_date(file_data["modtime"].get<std::string>());
}

bool sia_provider::start(api_item_added_callback api_item_added, i_open_file_table *oft) {
  const auto unmount_requested = base_provider::start(api_item_added, oft);
  if (not unmount_requested && is_online()) {
    {
      json data, error;
      const auto res = comm_.get("/daemon/version", data, error);
      if (res == api_error::success) {
        if (utils::compare_version_strings(
                data["version"].get<std::string>(),
                app_config::get_provider_minimum_version(get_config().get_provider_type())) < 0) {
          throw startup_exception("incompatible daemon version: " +
                                  data["version"].get<std::string>());
        }
      } else {
        throw startup_exception("failed to get version from daemon");
      }
    }

    mutex_lock l(start_stop_mutex_);
    if (not drive_space_thread_) {
      stop_requested_ = false;

      calculate_total_drive_space();

      std::uint64_t used_space = 0u;
      if (calculate_used_drive_space(used_space) != api_error::success) {
        throw startup_exception("failed to determine used space");
      }
      global_data::instance().initialize_used_drive_space(used_space);

      drive_space_thread_ = std::make_unique<std::thread>([this] {
        cleanup();
        if (not stop_requested_) {
          polling::instance().set_callback({"check_deleted", true, [this]() {
                                              remove_deleted_files();
                                              remove_expired_orphaned_files();
                                            }});
          drive_space_thread();
          polling::instance().remove_callback("check_deleted");
        }
      });
    }
  } else {
    throw startup_exception("failed to connect to api");
  }
  return unmount_requested;
}

void sia_provider::stop() {
  unique_mutex_lock l(start_stop_mutex_);
  if (drive_space_thread_) {
    event_system::instance().raise<service_shutdown>("sia_provider");
    stop_requested_ = true;

    start_stop_notify_.notify_all();
    l.unlock();

    drive_space_thread_->join();
    drive_space_thread_.reset();
  }
}

api_error sia_provider::upload_file(const std::string &api_path, const std::string &source_path,
                                    const std::string &) {
  event_system::instance().raise<file_upload_begin>(api_path, source_path);

  auto ret = set_source_path(api_path, source_path);
  if (ret == api_error::success) {
    std::uint64_t fileSize;
    if (utils::file::get_file_size(source_path, fileSize)) {
      json result, error;
      ret = (get_comm().post("/renter/upload" + api_path,
                             {{"source", &source_path[0]}, {"force", "true"}}, result,
                             error) == api_error::success)
                ? api_error::success
                : api_error::upload_failed;
      if (ret != api_error::success) {
        if (check_not_found(error)) {
          ret = (get_comm().post("/renter/upload" + api_path, {{"source", &source_path[0]}}, result,
                                 error) == api_error::success)
                    ? api_error::success
                    : api_error::upload_failed;
        }
        if (ret != api_error::success) {
          event_system::instance().raise<file_upload_failed>(api_path, source_path, error.dump(2));
        }
      }
    } else {
      ret = api_error::os_error;
      event_system::instance().raise<file_upload_failed>(api_path, source_path,
                                                         "Failed to get source file size");
    }
  } else {
    event_system::instance().raise<file_upload_failed>(api_path, source_path,
                                                       "Failed to set source path");
  }

  event_system::instance().raise<file_upload_end>(api_path, source_path, ret);
  return ret;
}
} // namespace repertory
