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
#include "providers/base_provider.hpp"

#include "app_config.hpp"
#include "database/db_common.hpp"
#include "database/db_insert.hpp"
#include "database/db_select.hpp"
#include "events/event_system.hpp"
#include "events/events.hpp"
#include "file_manager/i_file_manager.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/polling.hpp"

namespace repertory {
auto base_provider::create_api_file(std::string path, std::string key,
                                    std::uint64_t size) -> api_file {
  api_file file{};
  file.api_path = utils::path::create_api_path(path);
  file.api_parent = utils::path::get_parent_api_path(file.api_path);
  file.accessed_date = utils::get_file_time_now();
  file.changed_date = utils::get_file_time_now();
  file.creation_date = utils::get_file_time_now();
  file.modified_date = utils::get_file_time_now();
  file.key = key;
  file.file_size = size;
  return file;
}

auto base_provider::create_api_file(std::string path, std::uint64_t size,
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

auto base_provider::create_directory_clone_source_meta(
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

auto base_provider::create_directory(const std::string &api_path,
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
    utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                       "failed to create directory");
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       api_error::item_exists,
                                       "failed to create directory");
    return api_error::item_exists;
  }

  try {
    res = create_directory_impl(api_path, meta);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                         "failed to create directory");
      return res;
    }
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                       "failed to create directory");
    return api_error::error;
  }

  meta[META_DIRECTORY] = utils::string::from_bool(true);
  return set_item_meta(api_path, meta);
}

auto base_provider::create_file(const std::string &api_path, api_meta_map &meta)
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
    res = create_file_extra(api_path, meta);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                         "failed to create file");
      return res;
    }

    meta[META_DIRECTORY] = utils::string::from_bool(false);
    res = set_item_meta(api_path, meta);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                         "failed to create file");
      return res;
    }

    stop_type stop_requested{false};
    res = upload_file(api_path, meta[META_SOURCE], stop_requested);
    if (res != api_error::success) {
      db3_->remove_api_path(api_path);
    }

    return res;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                       "failed to create file");
  }

  return api_error::error;
}

auto base_provider::get_api_path_from_source(const std::string &source_path,
                                             std::string &api_path) const
    -> api_error {
  if (source_path.empty()) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       api_error::item_not_found,
                                       "failed to source path from api path");
    return api_error::item_not_found;
  }

  return db3_->get_api_path(source_path, api_path);
}

auto base_provider::get_directory_items(const std::string &api_path,
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
    res = get_directory_items_impl(api_path, list);
    if (res != api_error::success) {
      return res;
    }
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                       "failed to get directory items");
    return api_error::error;
  }

  std::sort(list.begin(), list.end(),
            [](const auto &item1, const auto &item2) -> bool {
              return (item1.directory && not item2.directory) ||
                     (not(item2.directory && not item1.directory) &&
                      (item1.api_path.compare(item2.api_path) < 0));
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

auto base_provider::get_file_size(const std::string &api_path,
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

auto base_provider::get_filesystem_item(const std::string &api_path,
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

auto base_provider::get_filesystem_item_and_file(const std::string &api_path,
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

auto base_provider::get_filesystem_item_from_source_path(
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

auto base_provider::get_item_meta(const std::string &api_path,
                                  api_meta_map &meta) const -> api_error {
  return db3_->get_item_meta(api_path, meta);
}

auto base_provider::get_item_meta(const std::string &api_path,
                                  const std::string &key,
                                  std::string &value) const -> api_error {
  return db3_->get_item_meta(api_path, key, value);
}

auto base_provider::get_pinned_files() const -> std::vector<std::string> {
  return db3_->get_pinned_files();
}

auto base_provider::get_total_item_count() const -> std::uint64_t {
  return db3_->get_total_item_count();
}

auto base_provider::get_used_drive_space() const -> std::uint64_t {
  try {
    auto used_space = get_used_drive_space_impl();
    get_file_mgr()->update_used_space(used_space);
    return used_space;
  } catch (const std::exception &ex) {
    utils::error::raise_error(__FUNCTION__, ex,
                              "failed to get used drive space");
  }

  return 0U;
}

auto base_provider::is_file_writeable(const std::string &api_path) const
    -> bool {
  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return false;
  }

  return not exists;
}

void base_provider::remove_deleted_files() {
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

  for (const auto &api_path : db3_->get_api_path_list()) {
    api_meta_map meta{};
    if (get_item_meta(api_path, meta) == api_error::success) {
      if (utils::string::to_bool(meta[META_DIRECTORY])) {
        bool exists{};
        if (is_directory(api_path, exists) != api_error::success) {
          continue;
        }
        if (not exists) {
          removed_list.emplace_back(removed_item{api_path, true, ""});
        }
        continue;
      }

      bool exists{};
      if (is_file(api_path, exists) != api_error::success) {
        continue;
      }
      if (not exists) {
        removed_list.emplace_back(
            removed_item{api_path, false, meta[META_SOURCE]});
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
        db3_->remove_api_path(item.api_path);
        event_system::instance().raise<file_removed_externally>(
            item.api_path, item.source_path);
      }
    }
  }

  for (const auto &item : removed_list) {
    if (item.directory) {
      db3_->remove_api_path(item.api_path);
      event_system::instance().raise<directory_removed_externally>(
          item.api_path, item.source_path);
    }
  }
}

auto base_provider::remove_file(const std::string &api_path) -> api_error {
  const auto notify_end = [&api_path](api_error error) -> api_error {
    if (error == api_error::success) {
      event_system::instance().raise<file_removed>(api_path);
    } else {
      event_system::instance().raise<file_remove_failed>(
          api_path, api_error_to_string(error));
    }

    return error;
  };

  const auto *const function_name = __FUNCTION__;

  const auto remove_file_meta = [this, &api_path, &function_name,
                                 &notify_end]() -> api_error {
    api_meta_map meta{};
    auto res = get_item_meta(api_path, meta);
    db3_->remove_api_path(api_path);
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

  res = remove_file_impl(api_path);
  if (res != api_error::success) {
    return notify_end(res);
  }

  return remove_file_meta();
}

auto base_provider::remove_directory(const std::string &api_path) -> api_error {
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
    return notify_end(res);
  }
  if (not exists) {
    return notify_end(api_error::item_not_found);
  }

  res = remove_directory_impl(api_path);
  if (res != api_error::success) {
    return notify_end(res);
  }

  db3_->remove_api_path(api_path);

  return notify_end(api_error::success);
}

auto base_provider::remove_item_meta(const std::string &api_path,
                                     const std::string &key) -> api_error {
  return db3_->remove_item_meta(api_path, key);
}

auto base_provider::set_item_meta(const std::string &api_path,
                                  const std::string &key,
                                  const std::string &value) -> api_error {
  return db3_->set_item_meta(api_path, key, value);
}

auto base_provider::set_item_meta(const std::string &api_path,
                                  const api_meta_map &meta) -> api_error {
  return db3_->set_item_meta(api_path, meta);
}

auto base_provider::start(api_item_added_callback api_item_added,
                          i_file_manager *mgr) -> bool {
  api_item_added_ = api_item_added;
  fm_ = mgr;

  db3_ = std::make_unique<meta_db>(config_);

  api_meta_map meta{};
  if (get_item_meta("/", meta) == api_error::item_not_found) {
    auto dir = create_api_file("/", "", 0U);
    api_item_added_(true, dir);
  }

  auto online = false;
  auto unmount_requested = false;
  {
    repertory::event_consumer consumer(
        "unmount_requested",
        [&unmount_requested](const event &) { unmount_requested = true; });
    for (std::uint16_t i = 0U; not online && not unmount_requested &&
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

void base_provider::stop() {
  polling::instance().remove_callback("check_deleted");
  db3_.reset();
}

auto base_provider::upload_file(const std::string &api_path,
                                const std::string &source_path,
                                stop_type &stop_requested) -> api_error {
  event_system::instance().raise<provider_upload_begin>(api_path, source_path);

  const auto notify_end = [&api_path,
                           &source_path](api_error error) -> api_error {
    event_system::instance().raise<provider_upload_end>(api_path, source_path,
                                                        error);
    return error;
  };
  try {
    return notify_end(upload_file_impl(api_path, source_path, stop_requested));
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }

  return notify_end(api_error::error);
}
} // namespace repertory
