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
#include "providers/base_provider.hpp"

#include "app_config.hpp"
#include "db/meta_db.hpp"
#include "events/event_system.hpp"
#include "events/events.hpp"
#include "file_manager/cache_size_mgr.hpp"
#include "file_manager/i_file_manager.hpp"
#include "platform/platform.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"
#include "utils/polling.hpp"
#include "utils/tasks.hpp"
#include "utils/time.hpp"

namespace repertory {
void base_provider::add_all_items(const stop_type &stop_requested) {
  REPERTORY_USES_FUNCTION_NAME();

  api_file_list list{};
  std::string marker;
  auto res{api_error::more_data};
  while (not stop_requested && res == api_error::more_data) {
    res = get_file_list(list, marker);
    if (res != api_error::success && res != api_error::more_data) {
      utils::error::raise_error(function_name, res, "failed to get file list");
    }
  }
}

auto base_provider::create_api_file(std::string path, std::string key,
                                    std::uint64_t size, std::uint64_t file_time)
    -> api_file {
  api_file file{};
  file.api_path = utils::path::create_api_path(path);
  file.api_parent = utils::path::get_parent_api_path(file.api_path);
  file.accessed_date = file_time;
  file.changed_date = file_time;
  file.creation_date = file_time;
  file.modified_date = file_time;
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
  REPERTORY_USES_FUNCTION_NAME();

  bool exists{};
  auto res = is_file(source_api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::item_exists,
                                       "failed to create directory");
    return api_error::item_exists;
  }

  res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::directory_exists,
                                       "failed to create directory");
    return api_error::directory_exists;
  }

  res = is_file(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(function_name, api_path,
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
    utils::error::raise_api_path_error(function_name, api_path, res,
                                       "failed to create directory");
    return res;
  }

  return create_directory(api_path, meta);
}

auto base_provider::create_directory(const std::string &api_path,
                                     api_meta_map &meta) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::directory_exists,
                                       "failed to create directory");
    return api_error::directory_exists;
  }

  res = is_file(api_path, exists);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(function_name, api_path, res,
                                       "failed to create directory");
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::item_exists,
                                       "failed to create directory");
    return api_error::item_exists;
  }

  try {
    res = create_directory_impl(api_path, meta);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(function_name, api_path, res,
                                         "failed to create directory");
      return res;
    }
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to create directory");
    return api_error::error;
  }

  meta[META_DIRECTORY] = utils::string::from_bool(true);
  return set_item_meta(api_path, meta);
}

auto base_provider::create_file(const std::string &api_path, api_meta_map &meta)
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success && res != api_error::item_not_found) {
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::directory_exists,
                                       "failed to create file");
    return api_error::directory_exists;
  }

  res = is_file(api_path, exists);
  if (res != api_error::success && res != api_error::item_not_found) {
    return res;
  }
  if (exists) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::item_exists,
                                       "failed to create file");
    return api_error::item_exists;
  }

  try {
    res = create_file_extra(api_path, meta);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(function_name, api_path, res,
                                         "failed to create file");
      return res;
    }

    meta[META_DIRECTORY] = utils::string::from_bool(false);
    res = set_item_meta(api_path, meta);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(function_name, api_path, res,
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
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to create file");
  }

  return api_error::error;
}

auto base_provider::get_api_path_from_source(const std::string &source_path,
                                             std::string &api_path) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  if (source_path.empty()) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::item_not_found,
                                       "failed to source path from api path");
    return api_error::item_not_found;
  }

  return db3_->get_api_path(source_path, api_path);
}

auto base_provider::get_directory_items(const std::string &api_path,
                                        directory_item_list &list) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

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
    utils::error::raise_api_path_error(function_name, api_path, e,
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
  return db3_->get_total_size();
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

void base_provider::process_removed_directories(
    std::deque<removed_item> removed_list, const stop_type &stop_requested) {
  for (const auto &item : removed_list) {
    if (stop_requested) {
      return;
    }

    if (not item.directory) {
      continue;
    }

    db3_->remove_api_path(item.api_path);
    event_system::instance().raise<directory_removed_externally>(
        item.api_path, item.source_path);
  }
}

void base_provider::process_removed_files(std::deque<removed_item> removed_list,
                                          const stop_type &stop_requested) {
  REPERTORY_USES_FUNCTION_NAME();

  auto orphaned_directory =
      utils::path::combine(get_config().get_data_directory(), {"orphaned"});
  for (const auto &item : removed_list) {
    if (stop_requested) {
      return;
    }

    if (item.directory) {
      continue;
    }

    if (not utils::file::file{item.source_path}.exists()) {
      continue;
    }

    if (not utils::file::directory{orphaned_directory}.create_directory()) {
      utils::error::raise_error(
          function_name, std::to_string(utils::get_last_error_code()),
          "failed to create orphaned directory|sp|" + orphaned_directory);
      continue;
    }

    auto parts = utils::string::split(item.api_path, '/', false);
    auto orphaned_file = utils::path::combine(
        orphaned_directory, {utils::path::strip_to_file_name(item.source_path) +
                             '_' + parts[parts.size() - 1U]});

    event_system::instance().raise<orphaned_file_detected>(item.source_path);
    if (not utils::file::reset_modified_time(item.source_path)) {
      event_system::instance().raise<orphaned_file_processing_failed>(
          item.source_path, orphaned_file,
          "reset modified failed|" +
              std::to_string(utils::get_last_error_code()));
      continue;
    }

    if (not utils::file::file(item.source_path).copy_to(orphaned_file, true)) {
      [[maybe_unused]] auto removed = utils::file::file{orphaned_file}.remove();
      event_system::instance().raise<orphaned_file_processing_failed>(
          item.source_path, orphaned_file,
          "copy failed|" + std::to_string(utils::get_last_error_code()));
      continue;
    }

    if (not fm_->evict_file(item.api_path)) {
      event_system::instance().raise<orphaned_file_processing_failed>(
          item.source_path, orphaned_file, "eviction failed");
      continue;
    }

    db3_->remove_api_path(item.api_path);
    event_system::instance().raise<file_removed_externally>(item.api_path,
                                                            item.source_path);
  }
}

void base_provider::process_removed_items(const stop_type &stop_requested) {
  auto list = db3_->get_api_path_list();
  [[maybe_unused]] auto res =
      std::all_of(list.begin(), list.end(), [&](auto &&api_path) -> bool {
        if (stop_requested) {
          return false;
        }

        tasks::instance().schedule({
            [this, api_path](auto &&task_stopped) {
              api_meta_map meta{};
              if (get_item_meta(api_path, meta) != api_error::success) {
                return;
              }

              if (utils::string::to_bool(meta[META_DIRECTORY])) {
                return;
              }
              //   bool exists{};
              //   if (is_directory(api_path, exists) != api_error::success) {
              //     return;
              //   }
              //
              //   if (exists) {
              //     return;
              //   }
              //
              //   // process_removed_directories(
              //   //     {
              //   //         removed_item{api_path, true, ""},
              //   //     },
              //   //     stop_requested2);
              //
              //   return;
              // }

              bool exists{};
              if (is_file(api_path, exists) != api_error::success) {
                return;
              }

              if (exists) {
                return;
              }

              process_removed_files(
                  {
                      removed_item{api_path, false, meta[META_SOURCE]},
                  },
                  task_stopped);
            },
        });

        return not stop_requested;
      });
}

void base_provider::remove_deleted_items(const stop_type &stop_requested) {
  add_all_items(stop_requested);
  if (stop_requested) {
    return;
  }

  remove_unmatched_source_files(stop_requested);
  if (stop_requested) {
    return;
  }

  process_removed_items(stop_requested);
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

  const auto remove_file_meta = [this, &api_path, &notify_end]() -> api_error {
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

void base_provider::remove_unmatched_source_files(
    const stop_type &stop_requested) {
  if (is_read_only()) {
    return;
  }

  const auto &cfg = get_config();

  auto source_list =
      utils::file::directory{cfg.get_cache_directory()}.get_files();
  for (const auto &source_file : source_list) {
    if (stop_requested) {
      return;
    }

    filesystem_item fsi{};
    if (get_filesystem_item_from_source_path(source_file->get_path(), fsi) !=
        api_error::item_not_found) {
      continue;
    }

    auto reference_time =
        source_file->get_time(cfg.get_eviction_uses_accessed_time()
                                  ? utils::file::time_type::accessed
                                  : utils::file::time_type::modified);
    if (not reference_time.has_value()) {
      continue;
    }

    auto delay =
        (cfg.get_eviction_delay_mins() * 60UL) * utils::time::NANOS_PER_SECOND;
    if ((reference_time.value() + static_cast<std::uint64_t>(delay)) >=
        utils::time::get_time_now()) {
      continue;
    }

    event_system::instance().raise<orphaned_source_file_detected>(
        source_file->get_path());
    if (not source_file->remove()) {
      continue;
    }

    event_system::instance().raise<orphaned_source_file_removed>(
        source_file->get_path());
  }
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

  db3_ = create_meta_db(config_);

  api_meta_map meta{};
  if (get_item_meta("/", meta) == api_error::item_not_found) {
    auto dir = create_api_file("/", "", 0U, utils::time::get_time_now());
    api_item_added_(true, dir);
  }

  auto online{false};
  auto unmount_requested{false};
  {
    const auto &cfg = get_config();

    repertory::event_consumer consumer(
        "unmount_requested",
        [&unmount_requested](const event &) { unmount_requested = true; });
    for (std::uint16_t idx = 0U; not online && not unmount_requested &&
                                 (idx < cfg.get_online_check_retry_secs());
         ++idx) {
      online = is_online();
      if (not online) {
        event_system::instance().raise<provider_offline>(
            cfg.get_host_config().host_name_or_ip,
            cfg.get_host_config().api_port);
        std::this_thread::sleep_for(1s);
      }
    }
  }

  if (not online || unmount_requested) {
    return false;
  }

  cache_size_mgr::instance().initialize(&config_);

  polling::instance().set_callback({
      "check_deleted",
      polling::frequency::low,
      [this](auto &&stop_requested) { remove_deleted_items(stop_requested); },
  });

  return true;
}

void base_provider::stop() {
  cache_size_mgr::instance().stop();
  polling::instance().remove_callback("check_deleted");
  db3_.reset();
}

auto base_provider::upload_file(const std::string &api_path,
                                const std::string &source_path,
                                stop_type &stop_requested) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

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
    utils::error::raise_error(function_name, e, "exception occurred");
  }

  return notify_end(api_error::error);
}
} // namespace repertory
