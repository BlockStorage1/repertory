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
#include "file_manager/i_file_manager.hpp"
#include "types/repertory.hpp"
#include "types/startup_exception.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/native_file.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
base_provider::base_provider(app_config &config) : config_(config) {}

void base_provider::calculate_used_drive_space(bool add_missing) {
  api_file_list list{};
  if (get_file_list(list) != api_error::success) {
    return;
  }

  used_space_ = std::accumulate(
      list.begin(), list.end(), std::uint64_t(0U),
      [this, &add_missing](std::uint64_t total_size, const auto &file) {
        if (add_missing && not meta_db_->get_item_meta_exists(file.api_path)) {
          [[maybe_unused]] auto res = this->notify_file_added(
              file.api_path, utils::path::get_parent_api_path(file.api_path),
              0);
        }

        return total_size + file.file_size;
      });
}

void base_provider::cleanup() {
  remove_deleted_files();
  remove_unknown_source_files();
  remove_expired_orphaned_files();
}

auto base_provider::create_directory_clone_source_meta(
    const std::string &source_api_path, const std::string &api_path)
    -> api_error {
  api_meta_map meta{};
  auto ret = get_item_meta(source_api_path, meta);
  if (ret == api_error::success) {
    ret = create_directory(api_path, meta);
  }
  return ret == api_error::item_not_found ? api_error::directory_not_found
                                          : ret;
}

auto base_provider::create_file(const std::string &api_path, api_meta_map &meta)
    -> api_error {
  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    return api_error::directory_exists;
  }

  res = is_file(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    return api_error::item_exists;
  }

  if ((res = meta_db_->set_item_meta(api_path, meta)) != api_error::success) {
    return res;
  }

  {
    native_file_ptr nf;
    res = native_file::create_or_open(meta[META_SOURCE], nf);
    if (res != api_error::success) {
      return res;
    }
    nf->close();
  }

  stop_type stop_requested = false;
  return upload_file(api_path, meta[META_SOURCE], meta[META_ENCRYPTION_TOKEN],
                     stop_requested);
}

auto base_provider::get_api_path_from_source(const std::string &source_path,
                                             std::string &api_path) const
    -> api_error {
  return meta_db_->get_api_path_from_source(source_path, api_path);
}

auto base_provider::get_directory_items(const std::string &api_path,
                                        directory_item_list &list) const
    -> api_error {
  auto res = populate_directory_items(api_path, list);
  if (res != api_error::success) {
    return res;
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

auto base_provider::get_file(const std::string &api_path, api_file &file) const
    -> api_error {
  auto ret = api_error::success;
  try {
    if ((ret = populate_file(api_path, file)) != api_error::success) {
      event_system::instance().raise<file_get_failed>(api_path,
                                                      api_error_to_string(ret));
    }

    std::string sz;
    if ((ret = get_item_meta(api_path, META_SIZE, sz)) != api_error::success) {
      return ret;
    }

    file.file_size = utils::string::to_uint64(sz);
    return ret;
  } catch (const std::exception &e) {
    event_system::instance().raise<file_get_failed>(
        api_path, e.what() ? e.what() : "failed to get file");
  }

  return api_error::error;
}

auto base_provider::get_file_size(const std::string &api_path,
                                  std::uint64_t &file_size) const -> api_error {
  api_file file{};
  const auto ret = get_file(api_path, file);
  if (ret == api_error::success) {
    file_size = file.file_size;
  } else {
    event_system::instance().raise<file_get_size_failed>(
        api_path, api_error_to_string(ret));
  }

  return ret;
}

auto base_provider::get_filesystem_item(const std::string &api_path,
                                        bool directory,
                                        filesystem_item &fsi) const
    -> api_error {
  auto ret = api_error::error;
  if (directory) {
    bool exists{};
    ret = is_directory(api_path, exists);
    if (ret != api_error::success) {
      return ret;
    }
    ret = exists ? api_error::success : api_error::item_not_found;
    update_filesystem_item(true, ret, api_path, fsi);
  } else {
    api_file file{};
    ret = get_filesystem_item_and_file(api_path, file, fsi);
  }

  return ret;
}

auto base_provider::get_filesystem_item_and_file(const std::string &api_path,
                                                 api_file &file,
                                                 filesystem_item &fsi) const
    -> api_error {
  auto ret = get_item_meta(api_path, META_SOURCE, fsi.source_path);
  if (ret == api_error::success) {
    ret = get_file(api_path, file);
    if (ret == api_error::success) {
      fsi.encryption_token = file.encryption_token;
      fsi.size = file.file_size;
    } else {
      bool exists{};
      ret = is_file(api_path, exists);
      if (ret != api_error::success) {
        return ret;
      }
      if (not exists) {
        ret = api_error::item_not_found;
      }
    }
  }

  update_filesystem_item(false, ret, api_path, fsi);
  return ret;
}

auto base_provider::get_filesystem_item_from_source_path(
    const std::string &source_path, filesystem_item &fsi) const -> api_error {
  auto ret = api_error::item_not_found;
  if (not source_path.empty()) {
    std::string api_path;
    if ((ret = get_api_path_from_source(source_path, api_path)) ==
        api_error::success) {
      ret = get_filesystem_item(api_path, false, fsi);
    }
  }

  return ret;
}

auto base_provider::get_item_meta(const std::string &api_path,
                                  api_meta_map &meta) const -> api_error {
  auto ret = meta_db_->get_item_meta(api_path, meta);
  if (ret == api_error::item_not_found) {
    auto get_meta = false;
    bool exists{};
    ret = is_directory(api_path, exists);
    if (ret != api_error::success) {
      return ret;
    }

    if (exists) {
      ret = notify_directory_added(api_path,
                                   utils::path::get_parent_api_path(api_path));
      if (ret == api_error::success) {
        get_meta = true;
      }
    } else {
      ret = is_file(api_path, exists);
      if (ret != api_error::success) {
        return ret;
      }
      if (exists) {
        std::uint64_t file_size{};
        if ((ret = get_file_size(api_path, file_size)) == api_error::success) {
          get_meta = ((ret = notify_file_added(
                           api_path, utils::path::get_parent_api_path(api_path),
                           file_size)) == api_error::success);
        }
      }
    }

    ret = get_meta ? meta_db_->get_item_meta(api_path, meta)
                   : api_error::item_not_found;
  }

  return ret;
}

auto base_provider::get_item_meta(const std::string &api_path,
                                  const std::string &key,
                                  std::string &value) const -> api_error {
  auto ret = meta_db_->get_item_meta(api_path, key, value);
  if (ret == api_error::item_not_found) {
    auto get_meta = false;
    bool exists{};
    ret = is_directory(api_path, exists);
    if (ret != api_error::success) {
      return ret;
    }
    if (exists) {
      ret = notify_directory_added(api_path,
                                   utils::path::get_parent_api_path(api_path));
      if (ret == api_error::success) {
        get_meta = true;
      }
    } else {
      ret = is_file(api_path, exists);
      if (ret != api_error::success) {
        return ret;
      }
      if (exists) {
        std::uint64_t file_size{};
        if ((ret = get_file_size(api_path, file_size)) == api_error::success) {
          get_meta = ((ret = notify_file_added(
                           api_path, utils::path::get_parent_api_path(api_path),
                           file_size)) == api_error::success);
        }
      }
    }

    ret = get_meta ? meta_db_->get_item_meta(api_path, key, value)
                   : api_error::item_not_found;
  }

  return ret;
}

auto base_provider::get_used_drive_space() const -> std::uint64_t {
  std::uint64_t used_space = used_space_;
  fm_->update_used_space(used_space);
  return used_space;
}

auto base_provider::notify_directory_added(const std::string &api_path,
                                           const std::string &api_parent)
    -> api_error {
  recur_mutex_lock l(notify_added_mutex_);

  const auto now = utils::get_file_time_now();
  api_file file{};
  file.api_path = api_path;
  file.api_parent = api_parent;
  file.accessed_date = now;
  file.changed_date = now;
  file.creation_date = now;
  file.file_size = 0U;
  file.modified_date = now;
  return api_item_added_(true, file);
}

auto base_provider::processed_orphaned_file(const std::string &source_path,
                                            const std::string &api_path) const
    -> bool {
  const auto orphaned_directory =
      utils::path::combine(get_config().get_data_directory(), {"orphaned"});
  if (utils::file::create_full_directory_path(orphaned_directory)) {
    event_system::instance().raise<orphaned_file_detected>(source_path);
    const auto parts = utils::string::split(api_path, '/', false);
    const auto orphaned_file = utils::path::combine(
        orphaned_directory, {utils::path::strip_to_file_name(source_path) +
                             '_' + parts[parts.size() - 1U]});

    if (utils::file::reset_modified_time(source_path) &&
        utils::file::move_file(source_path, orphaned_file)) {
      event_system::instance().raise<orphaned_file_processed>(source_path,
                                                              orphaned_file);
      return true;
    }

    event_system::instance().raise<orphaned_file_processing_failed>(
        source_path, orphaned_file,
        std::to_string(utils::get_last_error_code()));
    return false;
  }

  utils::error::raise_error(
      __FUNCTION__, std::to_string(utils::get_last_error_code()),
      "failed to create orphaned director|sp|" + orphaned_directory);
  return false;
}

void base_provider::remove_deleted_files() {
  std::vector<std::string> removed_files{};

  api_file_list list{};
  if (get_file_list(list) == api_error::success) {
    if (not list.empty()) {
      auto iterator = meta_db_->create_iterator(false);
      for (iterator->SeekToFirst(); not stop_requested_ && iterator->Valid();
           iterator->Next()) {
        const auto meta_api_path = iterator->key().ToString();
        if (meta_api_path.empty()) {
          const auto res = meta_db_->remove_item_meta(meta_api_path);
          if (res != api_error::success) {
            utils::error::raise_api_path_error(__FUNCTION__, meta_api_path, res,
                                               "failed to remove item meta");
          }
        } else {
          auto api_path = meta_api_path;
          const auto it = std::find_if(list.begin(), list.end(),
                                       [&api_path](const auto &file) -> bool {
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

    bool exists{};
    auto res = is_directory(api_path, exists);
    if (res != api_error::success) {
      continue;
    }

    std::string source_path;
    if (not exists &&
        (check_file_exists(api_path) == api_error::item_not_found) &&
        (meta_db_->get_item_meta(api_path, META_SOURCE, source_path) ==
         api_error::success)) {
      if (not source_path.empty()) {
        fm_->perform_locked_operation(
            [this, &api_path, &source_path](i_provider &) -> bool {
              if (fm_->has_no_open_file_handles()) {
                const auto res = meta_db_->remove_item_meta(api_path);
                if (res == api_error::success) {
                  event_system::instance().raise<file_removed_externally>(
                      api_path, source_path);
                  processed_orphaned_file(source_path, api_path);
                } else {
                  utils::error::raise_api_path_error(
                      __FUNCTION__, api_path, source_path, res,
                      "failed to remove item meta for externally removed file");
                }
              }

              return true;
            });
      }
    }
  }
}

void base_provider::remove_expired_orphaned_files() {
  const auto orphaned_directory =
      utils::path::combine(get_config().get_data_directory(), {"orphaned"});
  const auto files = utils::file::get_directory_files(orphaned_directory, true);
  for (const auto &file : files) {
    if (utils::file::is_modified_date_older_than(
            file, std::chrono::hours(
                      get_config().get_orphaned_file_retention_days() * 24))) {
      if (utils::file::retry_delete_file(file)) {
        event_system::instance().raise<orphaned_file_deleted>(file);
      }
    }
    if (stop_requested_) {
      break;
    }
  }
}

void base_provider::remove_unknown_source_files() {
  auto files = utils::file::get_directory_files(
      get_config().get_cache_directory(), true);
  while (not stop_requested_ && not files.empty()) {
    const auto file = files.front();
    files.pop_front();

    std::string api_path;
    if (not meta_db_->get_source_path_exists(file)) {
      processed_orphaned_file(file);
    }
  }
}

auto base_provider::rename_file(const std::string &from_api_path,
                                const std::string &to_api_path) -> api_error {
  std::string source_path;
  auto ret = get_item_meta(from_api_path, META_SOURCE, source_path);
  if (ret != api_error::success) {
    return ret;
  }

  std::string encryption_token;
  ret = get_item_meta(from_api_path, META_ENCRYPTION_TOKEN, encryption_token);
  if (ret != api_error::success) {
    return ret;
  }

  ret = handle_rename_file(from_api_path, to_api_path, source_path);

  return ret;
}

auto base_provider::start(api_item_added_callback api_item_added,
                          i_file_manager *fm) -> bool {
  meta_db_ = std::make_unique<meta_db>(config_);

  api_item_added_ = api_item_added;
  fm_ = fm;

  auto unmount_requested = false;
  {
    repertory::event_consumer ec(
        "unmount_requested",
        [&unmount_requested](const event &) { unmount_requested = true; });
    for (std::uint16_t i = 0U; not unmount_requested && not is_online() &&
                               (i < get_config().get_online_check_retry_secs());
         i++) {
      event_system::instance().raise<provider_offline>(
          get_config().get_host_config().host_name_or_ip,
          get_config().get_host_config().api_port);
      std::this_thread::sleep_for(1s);
    }
  }

  auto ret = not unmount_requested && is_online();
  if (ret) {
    // Force root creation
    api_meta_map meta{};
    auto res = get_item_meta("/", meta);
    if (res != api_error::success) {
      throw startup_exception("failed to create root|err|" +
                              api_error_to_string(res));
    }

    calculate_used_drive_space(false);
  }

  return ret;
}

void base_provider::stop() { meta_db_.reset(); }

void base_provider::update_filesystem_item(bool directory,
                                           const api_error &error,
                                           const std::string &api_path,
                                           filesystem_item &fsi) const {
  if (error == api_error::success) {
    fsi.directory = directory;
    fsi.api_path = api_path;
    fsi.api_parent = utils::path::get_parent_api_path(api_path);
  } else {
    event_system::instance().raise<filesystem_item_get_failed>(
        api_path, std::to_string(static_cast<int>(error)));
  }
}
} // namespace repertory
