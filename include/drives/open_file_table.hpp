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
#ifndef INCLUDE_DRIVES_OPEN_FILE_TABLE_HPP_
#define INCLUDE_DRIVES_OPEN_FILE_TABLE_HPP_

#include "common.hpp"
#include "app_config.hpp"
#include "db/retry_db.hpp"
#include "download/i_download_manager.hpp"
#include "drives/i_open_file_table.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "utils/file_utils.hpp"
#include "utils/global_data.hpp"
#include "utils/native_file.hpp"
#include "utils/path_utils.hpp"
#include "utils/polling.hpp"

namespace repertory {
template <typename flags> class open_file_table final : public virtual i_open_file_table {
public:
  open_file_table(i_provider &provider, const app_config &config, i_download_manager &dm)
      : provider_(provider), config_(config), dm_(dm), retry_db_(config) {
    // Set initial value for used cache space
    global_data::instance().initialize_used_cache_space(
        utils::file::calculate_used_space(config_.get_cache_directory(), false));
    polling::instance().set_callback(
        {"last_close_clear", false, [this] {
           std::vector<std::string> keys;
           unique_mutex_lock l(last_close_mutex_);
           std::transform(last_close_lookup_.begin(), last_close_lookup_.end(),
                          std::back_inserter(keys), [](const auto &kv) { return kv.first; });
           l.unlock();
           for (const auto &key : keys) {
             l.lock();
             remove_if_expired(key, last_close_lookup_[key]);
             l.unlock();
           }
         }});
  }

  ~open_file_table() override { polling::instance().remove_callback("last_close_clear"); }

private:
  struct open_file_info {
    filesystem_item item;
    api_meta_map meta;
  };

private:
  i_provider &provider_;
  const app_config &config_;
  i_download_manager &dm_;
  retry_db retry_db_;
  std::unordered_map<std::string, std::shared_ptr<open_file_info>> open_file_lookup_;
  mutable std::recursive_mutex open_file_mutex_;
  std::unordered_map<std::uint64_t, open_file_info *> open_handle_lookup_;
  std::uint64_t next_handle_ = 1u;
  bool stop_requested_ = false;
  std::mutex retry_mutex_;
  std::unique_ptr<std::thread> retry_thread_;
  std::condition_variable retry_notify_;
  std::mutex start_stop_mutex_;
  std::mutex last_close_mutex_;
  std::unordered_map<std::string, std::uint64_t> last_close_lookup_;

private:
  api_error get_filesystem_item(const std::string &api_path, const bool &directory,
                                filesystem_item &fsi) {
    auto ret = api_error::item_not_found;

    const auto it = open_file_lookup_.find(api_path);
    if (it != open_file_lookup_.end()) {
      fsi = it->second->item;
      ret = api_error::success;
    } else {
      ret = provider_.get_filesystem_item(api_path, directory, fsi);
    }

    return ret;
  }

  std::uint64_t get_next_handle() {
    std::uint64_t ret = 0u;
    while ((ret = next_handle_++) == 0u) {
    }
    return ret;
  }

  api_error handle_file_rename(const std::string &from_api_path, const std::string &to_api_path) {
    auto ret = api_error::file_in_use;
    if (dm_.pause_download(from_api_path)) {
      if ((ret = provider_.rename_file(from_api_path, to_api_path)) == api_error::success) {
        swap_renamed_items(from_api_path, to_api_path);
        dm_.rename_download(from_api_path, to_api_path);
        dm_.resume_download(to_api_path);

        retry_db_.rename(from_api_path, to_api_path);
      } else {
        dm_.resume_download(from_api_path);
      }
    }

    return ret;
  }

  void handle_file_upload(filesystem_item &fsi) {
    fsi.changed = false;
    handle_file_upload(&fsi);
  }

  void handle_file_upload(const filesystem_item *fsi) {
    // Remove from retry queue, if present
    retry_db_.remove(fsi->api_path);

    // Upload file and add to retry queue on failure
    auto nf = native_file::attach(fsi->handle);
    nf->flush();
    if (provider_.upload_file(fsi->api_path, fsi->source_path, fsi->encryption_token) !=
        api_error::success) {
      retry_db_.set(fsi->api_path);
      event_system::instance().raise<failed_upload_queued>(fsi->api_path);
    }
  }

  bool remove_if_expired(const std::string &api_path, const std::uint64_t &time) {
    auto ret = false;
#ifdef _WIN32
    const auto delay = std::chrono::minutes(config_.get_eviction_delay_mins());
    const auto last_check = std::chrono::system_clock::from_time_t(time);
    if ((ret = ((last_check + delay) <= std::chrono::system_clock::now())))
#else
    if ((ret = ((time + ((config_.get_eviction_delay_mins() * 60L) * NANOS_PER_SECOND)) <=
                utils::get_time_now())))
#endif
    {
      last_close_lookup_.erase(api_path);
    }
    return ret;
  }

  bool retry_delete_file(const std::string &file) {
    auto deleted = false;
    for (std::uint8_t i = 0u; not(deleted = utils::file::delete_file(file)) && (i < 100u); i++) {
      std::this_thread::sleep_for(10ms);
    }
    return deleted;
  }

  void swap_renamed_items(std::string from_api_path, std::string to_api_path) {
    const auto it = open_file_lookup_.find(from_api_path);
    if (it != open_file_lookup_.end()) {
      open_file_lookup_[to_api_path] = open_file_lookup_[from_api_path];
      open_file_lookup_.erase(from_api_path);
      auto &fsi = open_file_lookup_[to_api_path]->item;
      fsi.api_path = to_api_path;
      fsi.api_parent = utils::path::get_parent_api_path(to_api_path);
    }
  }

public:
  bool has_no_open_file_handles() const override {
    recur_mutex_lock l(open_file_mutex_);
    return std::find_if(open_file_lookup_.cbegin(), open_file_lookup_.cend(), [](const auto &kv) {
             return not kv.second->item.directory;
           }) == open_file_lookup_.cend();
  }

  void close(const std::uint64_t &handle) override {
    recur_mutex_lock l(open_file_mutex_);
    const auto it = open_handle_lookup_.find(handle);
    if (it != open_handle_lookup_.end()) {
      auto *oi = it->second;
      open_handle_lookup_.erase(handle);

      auto &fsi = oi->item;
      const auto was_changed = fsi.changed;

      // Handle meta change
      if (fsi.meta_changed) {
        if (provider_.set_item_meta(fsi.api_path, oi->meta) == api_error::success) {
          fsi.meta_changed = false;
        } else {
          event_system::instance().raise<repertory_exception>(
              __FUNCTION__, "failed to set file meta: " + fsi.api_path);
        }
      }

      // Handle source path change
      if (not fsi.directory && fsi.source_path_changed) {
        if (provider_.set_source_path(fsi.api_path, fsi.source_path) == api_error::success) {
          fsi.source_path_changed = false;
        } else {
          event_system::instance().raise<repertory_exception>(
              __FUNCTION__, "failed to set source path: " + fsi.api_path + "|" + fsi.source_path);
        }
      }

      // Update last close time in lookup table
      if (not fsi.directory) {
        mutex_lock l2(last_close_mutex_);
        last_close_lookup_[fsi.api_path] = utils::get_time_now();
      }

      // Handle file change
#ifdef __APPLE__
      // Special handling for OS X - only upload if handle being closed is writable
      if (not fsi.directory && was_changed && (fsi.open_data[handle] & O_ACCMODE))
#else
      if (not fsi.directory && was_changed)
#endif
      {
        handle_file_upload(fsi);
      }

      // Close internal handle if no more open files
      auto &od = fsi.open_data;
      od.erase(handle);
      event_system::instance().raise<filesystem_item_handle_closed>(
          fsi.api_path, handle, fsi.source_path, fsi.directory, was_changed);
      if (od.empty()) {
        native_file::attach(fsi.handle)->close();

        event_system::instance().raise<filesystem_item_closed>(fsi.api_path, fsi.source_path,
                                                               fsi.directory, was_changed);
        open_file_lookup_.erase(fsi.api_path);
      }
    }
  }

#ifdef _WIN32
  void close_all(const std::string &api_path) {
    recur_mutex_lock l(open_file_mutex_);
    const auto it = open_file_lookup_.find(api_path);
    if (it != open_file_lookup_.end()) {
      auto *oi = it->second.get();
      std::vector<std::uint64_t> handles;
      for (const auto &kv : open_handle_lookup_) {
        if (kv.second == oi) {
          handles.emplace_back(kv.first);
        }
      }

      while (!handles.empty()) {
        close(handles.back());
        handles.pop_back();
      }
    }
  }
#endif // _WIN32

  bool contains_restore(const std::string &api_path) const override {
    return dm_.contains_restore(api_path);
  }

  api_error derive_file_size(const std::string &api_path, std::uint64_t &file_size) {
    auto ret = api_error::success;
    file_size = 0u;

    if (provider_.is_file(api_path)) {
      unique_recur_mutex_lock l(open_file_mutex_);
      const auto it = open_file_lookup_.find(api_path);
      if (it == open_file_lookup_.end()) {
        l.unlock();
        ret = provider_.get_file_size(api_path, file_size);
      } else {
        file_size = open_file_lookup_[api_path]->item.size;
      }
    }

    return ret;
  }

  api_error derive_item_data(const std::string &api_path, api_meta_map &meta) {
    auto ret = api_error::success;
    meta.clear();

    unique_recur_mutex_lock l(open_file_mutex_);
    const auto it = open_file_lookup_.find(api_path);
    if (it == open_file_lookup_.end()) {
      l.unlock();
      ret = provider_.get_item_meta(api_path, meta);
    } else {
      meta = open_file_lookup_[api_path]->meta;
    }

    return ret;
  }

  api_error derive_item_data(const directory_item &di, std::uint64_t &file_size,
                             api_meta_map &meta) {
    return derive_item_data(di.api_path, di.directory, file_size, meta);
  }

  api_error derive_item_data(const std::string &api_path, const bool &directory,
                             std::uint64_t &file_size, api_meta_map &meta) {
    auto ret = api_error::success;
    meta.clear();
    file_size = 0;

    unique_recur_mutex_lock l(open_file_mutex_);
    const auto it = open_file_lookup_.find(api_path);
    if (it == open_file_lookup_.end()) {
      l.unlock();
      ret = provider_.get_item_meta(api_path, meta);
      if ((ret == api_error::success) && not directory) {
        ret = provider_.get_file_size(api_path, file_size);
      }
    } else {
      meta = open_file_lookup_[api_path]->meta;
      if (not directory) {
        file_size = open_file_lookup_[api_path]->item.size;
      }
    }

    return ret;
  }

  bool evict_file(const std::string &api_path) override {
    auto ret = false;
    auto allow_eviction = true;
    // Ensure enough time has passed since file was closed
    {
      mutex_lock l(last_close_mutex_);
      const auto it = last_close_lookup_.find(api_path);
      if (it != last_close_lookup_.end()) {
        allow_eviction = remove_if_expired(api_path, it->second);
      }
    }

    if (allow_eviction) {
      recur_mutex_lock l(open_file_mutex_);
      // Ensure item is not in upload retry queue
      if (not retry_db_.exists(api_path) && (get_open_count(api_path) == 0u)) {
        // Ensure item is not currently downloading
        if (not dm_.is_processing(api_path)) {
          filesystem_item fsi{};
          if (provider_.get_filesystem_item(api_path, false, fsi) == api_error::success) {
            std::uint64_t file_size = 0u;
            if ((ret = (utils::file::get_file_size(fsi.source_path, file_size) &&
                        retry_delete_file(fsi.source_path)))) {
              global_data::instance().update_used_space(file_size, 0, true);
              event_system::instance().raise<filesystem_item_evicted>(fsi.api_path,
                                                                      fsi.source_path);
            }
          }
        }
      }
    }

    return ret;
  }

  void force_schedule_upload(const filesystem_item &fsi) override {
    recur_mutex_lock l(open_file_mutex_);
    filesystem_item *fsi_ptr = nullptr;
    if (get_open_file(fsi.api_path, fsi_ptr)) {
      handle_file_upload(*fsi_ptr);
    } else {
      handle_file_upload(&fsi);
    }
  }

  directory_item_list get_directory_items(const std::string &api_path) const override {
    directory_item_list list;
    provider_.get_directory_items(api_path, list);
    return list;
  }

  std::uint64_t get_open_count(const std::string &api_path) const override {
    std::uint64_t ret = 0u;
    recur_mutex_lock l(open_file_mutex_);
    const auto it = open_file_lookup_.find(api_path);
    if (it != open_file_lookup_.end()) {
      ret = it->second->item.open_data.size();
    }
    return ret;
  }

  bool get_open_file(const std::string &api_path, filesystem_item *&fsi) override {
    auto ret = false;

    recur_mutex_lock l(open_file_mutex_);
    const auto it = open_file_lookup_.find(api_path);
    if (it != open_file_lookup_.end()) {
      fsi = &it->second->item;
      ret = true;
    }
    return ret;
  }

  bool get_open_file(const std::uint64_t &handle, filesystem_item *&fsi) {
    auto ret = false;
    recur_mutex_lock l(open_file_mutex_);
    const auto it = open_handle_lookup_.find(handle);
    if (it != open_handle_lookup_.end()) {
      fsi = &it->second->item;
      ret = true;
    }
    return ret;
  }

  std::unordered_map<std::string, std::size_t> get_open_files() const override {
    std::unordered_map<std::string, std::size_t> ret;
    unique_recur_mutex_lock l(open_file_mutex_);
    for (const auto &kv : open_file_lookup_) {
      ret.insert({kv.first, kv.second->item.open_data.size()});
    }
    l.unlock();
    return ret;
  }

  api_error open(const filesystem_item &fsi, std::uint64_t &handle) override {
    auto ret = api_error::success;

    recur_mutex_lock l(open_file_mutex_);
    if (open_file_lookup_.find(fsi.api_path) == open_file_lookup_.end()) {
      api_meta_map meta;
      if ((ret = provider_.get_item_meta(fsi.api_path, meta)) == api_error::success) {
        auto oi = std::make_shared<open_file_info>();
        oi->meta = meta;
        oi->item = fsi;
        oi->item.lock = std::make_shared<std::recursive_mutex>();
        open_file_lookup_.insert({fsi.api_path, oi});

        event_system::instance().raise<filesystem_item_opened>(
            oi->item.api_path, oi->item.source_path, oi->item.directory);
      }
    }

    if (ret == api_error::success) {
      ret = Open(fsi.api_path, fsi.directory, utils::file::get_read_write_open_flags(), handle);
    }

    return ret;
  }

  api_error Open(const std::string &api_path, const bool &directory, const flags &f,
                 std::uint64_t &handle) {
    auto ret = api_error::success;

    recur_mutex_lock l(open_file_mutex_);
    if (open_file_lookup_.find(api_path) == open_file_lookup_.end()) {
      api_meta_map meta;
      if ((ret = provider_.get_item_meta(api_path, meta)) == api_error::success) {
        auto oi = std::make_shared<open_file_info>();
        oi->meta = meta;
        if ((ret = provider_.get_filesystem_item(api_path, directory, oi->item)) ==
            api_error::success) {
          open_file_lookup_.insert({api_path, oi});
          event_system::instance().raise<filesystem_item_opened>(
              oi->item.api_path, oi->item.source_path, oi->item.directory);
        }
      }
    }

    if (ret == api_error::success) {
      auto *oi = open_file_lookup_[api_path].get();
      auto &fsi = oi->item;
      if (fsi.directory == directory) {
        handle = get_next_handle();
        fsi.open_data.insert({handle, f});
        open_handle_lookup_.insert({handle, oi});
      } else {
        ret = directory ? api_error::file_exists : api_error::directory_exists;
      }
    }

    return ret;
  }

  bool perform_locked_operation(locked_operation_callback locked_operation) override {
    recur_mutex_lock l(open_file_mutex_);
    return locked_operation(*this, provider_);
  }

  api_error remove_file(const std::string &api_path) {
    recur_mutex_lock l(open_file_mutex_);

    filesystem_item fsi{};
    auto ret = api_error::file_in_use;
    if ((get_open_count(api_path) == 0u) &&
        ((ret = provider_.get_filesystem_item(api_path, false, fsi)) == api_error::success) &&
        ((ret = provider_.remove_file(api_path)) == api_error::success)) {
      std::uint64_t file_size = 0u;
      utils::file::get_file_size(fsi.source_path, file_size);
      if (retry_delete_file(fsi.source_path) && file_size) {
        global_data::instance().update_used_space(file_size, 0, false);
      }
    }

    return ret;
  }

#ifdef HAS_SETXATTR
  api_error remove_xattr_meta(const std::string &api_path, const std::string &name) {
    auto ret = api_error::xattr_not_found;
    if (utils::collection_excludes(META_USED_NAMES, name)) {
      unique_recur_mutex_lock l(open_file_mutex_);
      if (open_file_lookup_.find(api_path) == open_file_lookup_.end()) {
        l.unlock();
        ret = provider_.remove_item_meta(api_path, name);
      } else if (open_file_lookup_[api_path]->meta.find(name) !=
                 open_file_lookup_[api_path]->meta.end()) {
        open_file_lookup_[api_path]->item.meta_changed = true;
        open_file_lookup_[api_path]->meta.erase(name);
        ret = api_error::success;
      }
    }

    return ret;
  }
#endif

  api_error rename_directory(const std::string &from_api_path, const std::string &to_api_path) {
    unique_recur_mutex_lock l(open_file_mutex_);
    auto ret = api_error::not_implemented;
    if (provider_.is_rename_supported()) {
      ret = api_error::directory_not_found;
      // Ensure source directory exists
      if (provider_.is_directory(from_api_path)) {
        ret = api_error::directory_exists;
        // Ensure destination directory does not exist
        if (not provider_.is_directory(to_api_path)) {
          ret = api_error::file_exists;
          // Ensure destination is not a file
          if (not provider_.is_file(from_api_path)) {
            ret = api_error::directory_not_found;
            // Ensure parent destination directory exists
            directory_item_list list;
            if (provider_.is_directory(utils::path::get_parent_api_path(to_api_path)) &&
                ((ret = provider_.create_directory_clone_source_meta(from_api_path, to_api_path)) ==
                 api_error::success) &&
                ((ret = provider_.get_directory_items(from_api_path, list)) ==
                 api_error::success)) {
              // Rename all items - directories MUST BE returned first
              for (std::size_t i = 0u; (ret == api_error::success) && (i < list.size()); i++) {
                const auto &api_path = list[i].api_path;
                if ((api_path != ".") && (api_path != "..")) {
                  const auto old_api_path = api_path;
                  const auto new_api_path = utils::path::create_api_path(utils::path::combine(
                      to_api_path, {old_api_path.substr(from_api_path.size())}));
                  if (list[i].directory) {
                    ret = rename_directory(old_api_path, new_api_path);
                  } else {
                    ret = rename_file(old_api_path, new_api_path);
                  }
                }
              }

              if (ret == api_error::success) {
                swap_renamed_items(from_api_path, to_api_path);
                ret = provider_.remove_directory(from_api_path);
              }
            }
          }
        }
      }
    }

    return ret;
  }

  api_error rename_file(const std::string &from_api_path, const std::string &to_api_path,
                        const bool &overwrite = true) {
    auto ret = api_error::not_implemented;
    if (provider_.is_rename_supported()) {
      // Don't rename if paths are the same
      if ((ret = (from_api_path == to_api_path) ? api_error::file_exists : api_error::success) ==
          api_error::success) {
        retry_db_.pause();

        unique_recur_mutex_lock l(open_file_mutex_);
        // Check allow overwrite if file exists
        if (not overwrite && provider_.is_file(to_api_path)) {
          l.unlock();
          ret = api_error::file_exists;
        } else {
          // Don't rename if source does not exist
          if ((ret = provider_.is_file(from_api_path)
                         ? api_error::success
                         : api_error::item_not_found) == api_error::success) {
            // Don't rename if destination file is downloading
            if ((ret = dm_.is_processing(to_api_path) ? api_error::file_in_use
                                                      : api_error::success) == api_error::success) {
              // Don't rename if destination file has open handles
              ret = api_error::file_in_use;
              if (get_open_count(to_api_path) == 0u) {
                if (provider_.is_file(
                        to_api_path)) { // Handle destination file exists (should overwrite)
                  filesystem_item fsi{};
                  if ((ret = get_filesystem_item(to_api_path, false, fsi)) == api_error::success) {
                    ret = api_error::os_error;
                    std::uint64_t file_size = 0u;
                    if (utils::file::get_file_size(fsi.source_path, file_size)) {
                      ret = provider_.remove_file(to_api_path);
                      if ((ret == api_error::success) || (ret == api_error::item_not_found)) {
                        if (retry_delete_file(fsi.source_path) && file_size) {
                          global_data::instance().update_used_space(file_size, 0, false);
                        }
                        ret = handle_file_rename(from_api_path, to_api_path);
                      }
                    }
                  }
                  l.unlock();
                } else if (provider_.is_directory(to_api_path)) { // Handle destination is directory
                  l.unlock();
                  ret = api_error::directory_exists;
                } else if (provider_.is_directory(utils::path::get_parent_api_path(
                               to_api_path))) { // Handle rename if destination directory exists
                  ret = handle_file_rename(from_api_path, to_api_path);
                  l.unlock();
                } else { // Destination directory not found
                  l.unlock();
                  ret = api_error::directory_not_found;
                }
              }
            } else if (provider_.is_directory(from_api_path)) {
              l.unlock();
              ret = api_error::directory_exists;
            }
          }
        }

        retry_db_.resume();
      }
    }

    return ret;
  }

  api_error set_item_meta(const std::string &api_path, const std::string &key,
                          const std::string &value) override {
    unique_recur_mutex_lock l(open_file_mutex_);
    if (open_file_lookup_.find(api_path) == open_file_lookup_.end()) {
      l.unlock();
      return provider_.set_item_meta(api_path, key, value);
    }

    if (open_file_lookup_[api_path]->meta[key] != value) {
      open_file_lookup_[api_path]->item.meta_changed = true;
      open_file_lookup_[api_path]->meta[key] = value;
    }

    return api_error::success;
  }

  api_error set_item_meta(const std::string &api_path, const api_meta_map &meta) {
    auto ret = api_error::success;
    auto it = meta.begin();
    for (std::size_t i = 0u; (ret == api_error::success) && (i < meta.size()); i++) {
      ret = set_item_meta(api_path, it->first, it->second);
      it++;
    }

    return ret;
  }

  void start() {
    mutex_lock start_stop_lock(start_stop_mutex_);
    if (not retry_thread_) {
      stop_requested_ = false;
      retry_thread_ = std::make_unique<std::thread>([this] {
        while (not stop_requested_) {
          const auto processed = retry_db_.process_all([this](const std::string &api_path) -> bool {
            auto success = false;
            event_system::instance().raise<failed_upload_retry>(api_path);
            unique_recur_mutex_lock open_file_lock(open_file_mutex_);
            if (open_file_lookup_.find(api_path) == open_file_lookup_.end()) {
              open_file_lock.unlock();

              filesystem_item fsi{};
              const auto res = provider_.get_filesystem_item(api_path, false, fsi);
              if ((res == api_error::success) ||
                  ((res == api_error::item_not_found) && provider_.is_file(api_path))) {
                if (provider_.upload_file(api_path, fsi.source_path, fsi.encryption_token) ==
                    api_error::success) {
                  success = true;
                }
              }

              // Remove deleted files
              if (not success && not provider_.is_file(api_path)) {
                success = true;
              }
            } else {
              // File is open, so force re-upload on close
              open_file_lookup_[api_path]->item.changed = true;
              open_file_lock.unlock();
              success = true;
            }

            return success;
          });
          if (not processed && not stop_requested_) {
            unique_mutex_lock retryLock(retry_mutex_);
            if (not stop_requested_) {
              retry_notify_.wait_for(retryLock, 5s);
            }
          }
        }
      });
    }
  }

  void stop() {
    mutex_lock start_stop_lock(start_stop_mutex_);
    if (retry_thread_) {
      event_system::instance().raise<service_shutdown>("open_file_table");
      stop_requested_ = true;

      unique_mutex_lock retry_lock(retry_mutex_);
      retry_notify_.notify_all();
      retry_lock.unlock();

      retry_thread_->join();
      retry_thread_.reset();
    }
  }

  void update_directory_item(directory_item &di) const override {
    recur_mutex_lock l(open_file_mutex_);
    const auto it = open_file_lookup_.find(di.api_path);
    if (it != open_file_lookup_.end()) {
      const auto &ofi = open_file_lookup_.at(di.api_path);
      di.meta = ofi->meta;
      if (not di.directory) {
        di.size = ofi->item.size;
      }
    }
  }
};
} // namespace repertory

#endif // INCLUDE_DRIVES_OPEN_FILE_TABLE_HPP_
