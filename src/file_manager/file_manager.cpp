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
#include "file_manager/file_manager.hpp"

#include "app_config.hpp"
#include "file_manager/events.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/polling.hpp"
#include "utils/rocksdb_utils.hpp"
#include "utils/unix/unix_utils.hpp"

namespace repertory {
static auto create_resume_entry(const i_open_file &o) -> json {
  return {
      {"chunk_size", o.get_chunk_size()},
      {"path", o.get_api_path()},
      {"read_state", utils::string::from_dynamic_bitset(o.get_read_state())},
      {"source", o.get_source_path()},
  };
}

static void restore_resume_entry(const json &resume_entry,
                                 std::string &api_path, std::size_t &chunk_size,
                                 boost::dynamic_bitset<> &read_state,
                                 std::string &source_path) {
  api_path = resume_entry["path"].get<std::string>();
  chunk_size = resume_entry["chunk_size"].get<std::size_t>();
  read_state = utils::string::to_dynamic_bitset(
      resume_entry["read_state"].get<std::string>());
  source_path = resume_entry["source"].get<std::string>();
}

file_manager::file_manager(app_config &config, i_provider &provider)
    : config_(config), provider_(provider) {
  if (not provider_.is_direct_only()) {
    auto families = std::vector<rocksdb::ColumnFamilyDescriptor>();
    families.emplace_back(rocksdb::kDefaultColumnFamilyName,
                          rocksdb::ColumnFamilyOptions());
    families.emplace_back("upload", rocksdb::ColumnFamilyOptions());
    families.emplace_back("upload_active", rocksdb::ColumnFamilyOptions());

    auto handles = std::vector<rocksdb::ColumnFamilyHandle *>();
    utils::db::create_rocksdb(config, "upload_db", families, handles, db_);
    std::size_t idx{};
    default_family_ = handles[idx++];
    upload_family_ = handles[idx++];
    upload_active_family_ = handles[idx++];

    E_SUBSCRIBE_EXACT(file_upload_completed,
                      [this](const file_upload_completed &evt) {
                        this->upload_completed(evt);
                      });
  }
}

file_manager::~file_manager() {
  stop();

  E_CONSUMER_RELEASE();

  db_.reset();
}

void file_manager::close(std::uint64_t handle) {
  unique_recur_mutex_lock file_lock(open_file_mtx_);
  auto it = open_handle_lookup_.find(handle);
  if (it != open_handle_lookup_.end()) {
    auto *cur_file = it->second;
    open_handle_lookup_.erase(handle);
    cur_file->remove(handle);

    if (cur_file->can_close()) {
      const auto api_path = cur_file->get_api_path();
      cur_file = nullptr;

      auto closeable_file = open_file_lookup_.at(api_path);
      open_file_lookup_.erase(api_path);
      file_lock.unlock();

      closeable_file->close();
    }
  }
}

void file_manager::close_all(const std::string &api_path) {
  recur_mutex_lock file_lock(open_file_mtx_);
  std::vector<std::uint64_t> handles;
  auto it = open_file_lookup_.find(api_path);
  if (it != open_file_lookup_.end()) {
    handles = it->second->get_handles();
  }

  for (auto &handle : handles) {
    open_file_lookup_[api_path]->remove(handle);
    open_handle_lookup_.erase(handle);
  }

  open_file_lookup_.erase(api_path);
}

void file_manager::close_timed_out_files() {
  unique_recur_mutex_lock file_lock(open_file_mtx_);
  auto closeable_list = std::accumulate(
      open_file_lookup_.begin(), open_file_lookup_.end(),
      std::vector<std::string>{}, [](auto items, const auto &kv) -> auto {
        if (kv.second->get_open_file_count() == 0U && kv.second->can_close()) {
          items.emplace_back(kv.first);
        }
        return items;
      });

  std::vector<std::shared_ptr<i_closeable_open_file>> open_files{};
  for (const auto &api_path : closeable_list) {
    auto closeable_file = open_file_lookup_.at(api_path);
    open_file_lookup_.erase(api_path);
    open_files.emplace_back(closeable_file);
  }
  closeable_list.clear();
  file_lock.unlock();

  for (auto &closeable_file : open_files) {
    closeable_file->close();
    event_system::instance().raise<download_timeout>(
        closeable_file->get_api_path());
  }
}

auto file_manager::create(const std::string &api_path, api_meta_map &meta,
                          open_file_data ofd, std::uint64_t &handle,
                          std::shared_ptr<i_open_file> &f) -> api_error {
  recur_mutex_lock file_lock(open_file_mtx_);
  auto res = provider_.create_file(api_path, meta);
  if (res != api_error::success && res != api_error::item_exists) {
    return res;
  }

  return open(api_path, false, ofd, handle, f);
}

auto file_manager::evict_file(const std::string &api_path) -> bool {
  if (provider_.is_direct_only()) {
    return false;
  }

  recur_mutex_lock open_lock(open_file_mtx_);
  if (is_processing(api_path)) {
    return false;
  }

  if (get_open_file_count(api_path) != 0U) {
    return false;
  }

  std::string pinned;
  auto res = provider_.get_item_meta(api_path, META_PINNED, pinned);
  if (res != api_error::success && res != api_error::item_not_found) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                       "failed to get pinned status");
    return false;
  }

  if (not pinned.empty() && utils::string::to_bool(pinned)) {
    return false;
  }

  std::string source_path{};
  res = provider_.get_item_meta(api_path, META_SOURCE, source_path);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                       "failed to get source path");
    return false;
  }
  if (source_path.empty()) {
    return false;
  }

  auto removed = utils::file::retry_delete_file(source_path);
  if (removed) {
    event_system::instance().raise<filesystem_item_evicted>(api_path,
                                                            source_path);
  }

  return removed;
}

auto file_manager::get_directory_items(const std::string &api_path) const
    -> directory_item_list {
  directory_item_list list{};
  auto res = provider_.get_directory_items(api_path, list);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                       "failed to get directory list");
  }
  return list;
}

auto file_manager::get_next_handle() -> std::uint64_t {
  if (++next_handle_ == 0u) {
    next_handle_++;
  }

  return next_handle_;
}

auto file_manager::get_open_file_count(const std::string &api_path) const
    -> std::size_t {
  recur_mutex_lock open_lock(open_file_mtx_);
  auto it = open_file_lookup_.find(api_path);
  if (it != open_file_lookup_.end()) {
    return it->second->get_open_file_count();
  }

  return 0u;
}

auto file_manager::get_open_file(std::uint64_t handle, bool write_supported,
                                 std::shared_ptr<i_open_file> &f) -> bool {
  recur_mutex_lock open_lock(open_file_mtx_);
  auto it = open_handle_lookup_.find(handle);
  if (it == open_handle_lookup_.end()) {
    return false;
  }

  auto of = open_file_lookup_.at(it->second->get_api_path());
  if (write_supported && not of->is_write_supported()) {
    auto new_f = std::make_shared<open_file>(
        utils::encryption::encrypting_reader::get_data_chunk_size(),
        config_.get_enable_chunk_download_timeout()
            ? config_.get_chunk_downloader_timeout_secs()
            : 0U,
        of->get_filesystem_item(), of->get_open_data(), provider_, *this);
    open_file_lookup_[of->get_api_path()] = new_f;
    f = new_f;
    return true;
  }

  f = of;
  return true;
}

auto file_manager::get_open_file_count() const -> std::size_t {
  recur_mutex_lock open_lock(open_file_mtx_);
  return open_file_lookup_.size();
}

auto file_manager::get_open_files() const
    -> std::unordered_map<std::string, std::size_t> {
  std::unordered_map<std::string, std::size_t> ret;

  recur_mutex_lock open_lock(open_file_mtx_);
  for (const auto &kv : open_file_lookup_) {
    ret[kv.first] = kv.second->get_open_file_count();
  }

  return ret;
}

auto file_manager::get_open_handle_count() const -> std::size_t {
  recur_mutex_lock open_lock(open_file_mtx_);
  return open_handle_lookup_.size();
}

auto file_manager::get_stored_downloads() const -> std::vector<json> {
  std::vector<json> ret;
  if (not provider_.is_direct_only()) {
    auto iterator = std::unique_ptr<rocksdb::Iterator>(
        db_->NewIterator(rocksdb::ReadOptions(), default_family_));
    for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
      ret.emplace_back(json::parse(iterator->value().ToString()));
    }
  }
  return ret;
}

auto file_manager::handle_file_rename(const std::string &from_api_path,
                                      const std::string &to_api_path)
    -> api_error {
  auto ret = api_error::file_in_use;
  if ((ret = provider_.rename_file(from_api_path, to_api_path)) ==
      api_error::success) {
    swap_renamed_items(from_api_path, to_api_path);
  }

  return ret;
}

auto file_manager::has_no_open_file_handles() const -> bool {
  recur_mutex_lock open_lock(open_file_mtx_);
  return open_handle_lookup_.empty();
}

auto file_manager::is_processing(const std::string &api_path) const -> bool {
  if (provider_.is_direct_only()) {
    return false;
  }

  recur_mutex_lock open_lock(open_file_mtx_);

  mutex_lock upload_lock(upload_mtx_);
  if (upload_lookup_.find(api_path) != upload_lookup_.end()) {
    return true;
  }

  {
    auto iterator = std::unique_ptr<rocksdb::Iterator>(
        db_->NewIterator(rocksdb::ReadOptions(), upload_family_));
    for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
      const auto parts = utils::string::split(iterator->key().ToString(), ':');
      if (parts[1u] == api_path) {
        return true;
      }
    }
  }

  auto it = open_file_lookup_.find(api_path);
  if (it != open_file_lookup_.end()) {
    return it->second->is_modified() || not it->second->is_complete();
  }

  return false;
}

auto file_manager::open(const std::string &api_path, bool directory,
                        const open_file_data &ofd, std::uint64_t &handle,
                        std::shared_ptr<i_open_file> &f) -> api_error {
  return open(api_path, directory, ofd, handle, f, nullptr);
}

auto file_manager::open(const std::string &api_path, bool directory,
                        const open_file_data &ofd, std::uint64_t &handle,
                        std::shared_ptr<i_open_file> &f,
                        std::shared_ptr<i_closeable_open_file> of)
    -> api_error {
  const auto create_and_add_handle =
      [&](std::shared_ptr<i_closeable_open_file> cur_file) {
        handle = get_next_handle();
        cur_file->add(handle, ofd);
        open_handle_lookup_[handle] = cur_file.get();
        f = cur_file;
      };

  recur_mutex_lock open_lock(open_file_mtx_);
  auto it = open_file_lookup_.find(api_path);
  if (it != open_file_lookup_.end()) {
    create_and_add_handle(it->second);
    return api_error::success;
  }

  filesystem_item fsi{};
  auto res = provider_.get_filesystem_item(api_path, directory, fsi);
  if (res != api_error::success) {
    return res;
  }

  if (fsi.source_path.empty()) {
    fsi.source_path = utils::path::combine(config_.get_cache_directory(),
                                           {utils::create_uuid_string()});
    if ((res = provider_.set_item_meta(fsi.api_path, META_SOURCE,
                                       fsi.source_path)) !=
        api_error::success) {
      return res;
    }
  }

  if (not of) {
    of = std::make_shared<open_file>(
        utils::encryption::encrypting_reader::get_data_chunk_size(),
        config_.get_enable_chunk_download_timeout()
            ? config_.get_chunk_downloader_timeout_secs()
            : 0U,
        fsi, provider_, *this);
  }
  open_file_lookup_[api_path] = of;
  create_and_add_handle(of);

  return api_error::success;
}

auto file_manager::perform_locked_operation(
    locked_operation_callback locked_operation) -> bool {
  recur_mutex_lock open_lock(open_file_mtx_);
  return locked_operation(provider_);
}

void file_manager::queue_upload(const i_open_file &o) {
  return queue_upload(o.get_api_path(), o.get_source_path(), false);
}

void file_manager::queue_upload(const std::string &api_path,
                                const std::string &source_path, bool no_lock) {
  if (provider_.is_direct_only()) {
    return;
  }

  std::unique_ptr<mutex_lock> l;
  if (not no_lock) {
    l = std::make_unique<mutex_lock>(upload_mtx_);
  }
  remove_upload(api_path, true);

  auto res = db_->Put(
      rocksdb::WriteOptions(), upload_family_,
      std::to_string(utils::get_file_time_now()) + ":" + api_path, source_path);
  if (res.ok()) {
    remove_resume(api_path, source_path);
    event_system::instance().raise<file_upload_queued>(api_path, source_path);
  } else {
    event_system::instance().raise<file_upload_failed>(api_path, source_path,
                                                       res.ToString());
  }

  if (not no_lock) {
    upload_notify_.notify_all();
  }
}

auto file_manager::remove_file(const std::string &api_path) -> api_error {
  recur_mutex_lock open_lock(open_file_mtx_);
  auto it = open_file_lookup_.find(api_path);
  if (it != open_file_lookup_.end() && it->second->is_modified()) {
    return api_error::file_in_use;
  }

  filesystem_item fsi{};
  auto res = provider_.get_filesystem_item(api_path, false, fsi);
  if (res != api_error::success) {
    return res;
  }

  if ((res = provider_.remove_file(api_path)) != api_error::success) {
    return res;
  }

  remove_upload(api_path);

  close_all(api_path);

  if (not utils::file::retry_delete_file(fsi.source_path)) {
    utils::error::raise_api_path_error(
        __FUNCTION__, fsi.api_path, fsi.source_path,
        utils::get_last_error_code(), "failed to delete source");
    return api_error::success;
  }

  return api_error::success;
}

void file_manager::remove_resume(const std::string &api_path,
                                 const std::string &source_path) {
  auto res = db_->Delete(rocksdb::WriteOptions(), default_family_, api_path);
  if (res.ok()) {
    event_system::instance().raise<download_stored_removed>(api_path,
                                                            source_path);
  } else {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, source_path,
                                       res.code(),
                                       "failed to remove resume entry");
  }
}

void file_manager::remove_upload(const std::string &api_path) {
  remove_upload(api_path, false);
}

void file_manager::remove_upload(const std::string &api_path, bool no_lock) {
  if (provider_.is_direct_only()) {
    return;
  }

  std::unique_ptr<mutex_lock> l;
  if (not no_lock) {
    l = std::make_unique<mutex_lock>(upload_mtx_);
  }

  auto it = upload_lookup_.find(api_path);
  if (it == upload_lookup_.end()) {
    auto iterator = std::unique_ptr<rocksdb::Iterator>(
        db_->NewIterator(rocksdb::ReadOptions(), upload_family_));
    for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
      const auto parts = utils::string::split(iterator->key().ToString(), ':');
      if (parts[1U] == api_path) {
        if (db_->Delete(rocksdb::WriteOptions(), upload_family_,
                        iterator->key())
                .ok()) {
          db_->Delete(rocksdb::WriteOptions(), upload_active_family_,
                      iterator->key());
        }
        event_system::instance().raise<file_upload_removed>(
            api_path, iterator->value().ToString());
      }
    }
  } else {
    auto iterator = std::unique_ptr<rocksdb::Iterator>(
        db_->NewIterator(rocksdb::ReadOptions(), upload_active_family_));
    for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
      const auto parts = utils::string::split(iterator->key().ToString(), ':');
      if (parts[1U] == api_path) {
        db_->Delete(rocksdb::WriteOptions(), upload_active_family_,
                    iterator->key());
      }
    }
    event_system::instance().raise<file_upload_removed>(
        api_path, it->second->get_source_path());

    it->second->cancel();
    upload_lookup_.erase(api_path);
  }

  if (not no_lock) {
    upload_notify_.notify_all();
  }
}

void file_manager::swap_renamed_items(std::string from_api_path,
                                      std::string to_api_path) {
  const auto it = open_file_lookup_.find(from_api_path);
  if (it != open_file_lookup_.end()) {
    open_file_lookup_[to_api_path] = open_file_lookup_[from_api_path];
    open_file_lookup_.erase(from_api_path);
    open_file_lookup_[to_api_path]->set_api_path(to_api_path);
  }
}

auto file_manager::rename_directory(const std::string &from_api_path,
                                    const std::string &to_api_path)
    -> api_error {
  if (not provider_.is_rename_supported()) {
    return api_error::not_implemented;
  }

  unique_recur_mutex_lock l(open_file_mtx_);
  // Ensure source directory exists
  bool exists{};
  auto res = provider_.is_directory(from_api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (not exists) {
    return api_error::directory_not_found;
  }

  // Ensure destination directory does not exist
  res = provider_.is_directory(to_api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    return api_error::directory_exists;
  }

  // Ensure destination is not a file
  res = provider_.is_file(from_api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    return api_error::item_exists;
  }

  // Create destination directory
  res =
      provider_.create_directory_clone_source_meta(from_api_path, to_api_path);
  if (res != api_error::success) {
    return res;
  }

  directory_item_list list{};
  res = provider_.get_directory_items(from_api_path, list);
  if (res != api_error::success) {
    return res;
  }

  // Rename all items - directories MUST BE returned first
  for (std::size_t i = 0U; (res == api_error::success) && (i < list.size());
       i++) {
    const auto &api_path = list[i].api_path;
    if ((api_path != ".") && (api_path != "..")) {
      const auto old_api_path = api_path;
      const auto new_api_path =
          utils::path::create_api_path(utils::path::combine(
              to_api_path, {old_api_path.substr(from_api_path.size())}));
      res = list[i].directory ? rename_directory(old_api_path, new_api_path)
                              : rename_file(old_api_path, new_api_path, false);
    }
  }

  if (res != api_error::success) {
    return res;
  }

  res = provider_.remove_directory(from_api_path);
  if (res != api_error::success) {
    return res;
  }

  swap_renamed_items(from_api_path, to_api_path);
  return api_error::success;
}

auto file_manager::rename_file(const std::string &from_api_path,
                               const std::string &to_api_path, bool overwrite)
    -> api_error {
  if (not provider_.is_rename_supported()) {
    return api_error::not_implemented;
  }

  // Don't rename if paths are the same
  if (from_api_path == to_api_path) {
    return api_error::item_exists;
  }

  unique_recur_mutex_lock l(open_file_mtx_);

  // Don't rename if source is directory
  bool exists{};
  auto res = provider_.is_directory(from_api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    return api_error::directory_exists;
  }

  // Don't rename if source does not exist
  res = provider_.is_file(from_api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (not exists) {
    return api_error::item_not_found;
  }

  // Don't rename if destination is directory
  res = provider_.is_directory(to_api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    res = api_error::directory_exists;
  }

  // Check allow overwrite if file exists
  bool dest_exists{};
  res = provider_.is_file(to_api_path, dest_exists);
  if (res != api_error::success) {
    return res;
  }
  if (not overwrite && dest_exists) {
    return api_error::item_exists;
  }

  // Don't rename if destination file has open handles
  if (get_open_file_count(to_api_path) != 0U) {
    return api_error::file_in_use;
  }

  // Don't rename if destination file is uploading or downloading
  if (is_processing(to_api_path)) {
    return api_error::file_in_use;
  }

  // Handle destination file exists (should overwrite)
  if (dest_exists) {
    filesystem_item fsi{};
    res = provider_.get_filesystem_item(to_api_path, false, fsi);
    if (res != api_error::success) {
      return res;
    }

    std::uint64_t file_size{};
    if (not utils::file::get_file_size(fsi.source_path, file_size)) {
      return api_error::os_error;
    }

    res = provider_.remove_file(to_api_path);
    if ((res == api_error::success) || (res == api_error::item_not_found)) {
      if (not utils::file::retry_delete_file(fsi.source_path)) {
        utils::error::raise_api_path_error(
            __FUNCTION__, fsi.api_path, fsi.source_path,
            utils::get_last_error_code(), "failed to delete source path");
      }
      return handle_file_rename(from_api_path, to_api_path);
    }

    return res;
  }

  // Check destination parent directory exists
  res = provider_.is_directory(utils::path::get_parent_api_path(to_api_path),
                               exists);
  if (res != api_error::success) {
    return res;
  }
  if (not exists) {
    return api_error::directory_not_found;
  }

  return handle_file_rename(from_api_path, to_api_path);
}

void file_manager::start() {
  if (provider_.is_direct_only()) {
    stop_requested_ = false;
    return;
  }

  if (not upload_thread_) {
    stop_requested_ = false;

    struct active_item {
      std::string api_path;
      std::string source_path;
    };

    std::vector<active_item> active_items{};

    auto iterator = std::unique_ptr<rocksdb::Iterator>(
        db_->NewIterator(rocksdb::ReadOptions(), upload_active_family_));
    for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
      const auto parts = utils::string::split(iterator->key().ToString(), ':');
      active_items.emplace_back(
          active_item{parts[1U], iterator->value().ToString()});
    }
    for (const auto &active_item : active_items) {
      queue_upload(active_item.api_path, active_item.source_path, false);
    }
    active_items.clear();

    iterator = std::unique_ptr<rocksdb::Iterator>(
        db_->NewIterator(rocksdb::ReadOptions(), default_family_));
    for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
      std::string api_path;
      std::string source_path;
      std::size_t chunk_size{};
      boost::dynamic_bitset<> read_state;
      restore_resume_entry(json::parse(iterator->value().ToString()), api_path,
                           chunk_size, read_state, source_path);

      filesystem_item fsi{};
      auto res = provider_.get_filesystem_item(api_path, false, fsi);
      if (res == api_error::success) {
        if (source_path == fsi.source_path) {
          std::uint64_t file_size{};
          if (utils::file::get_file_size(fsi.source_path, file_size)) {
            if (file_size == fsi.size) {
              auto f = std::make_shared<open_file>(
                  chunk_size,
                  config_.get_enable_chunk_download_timeout()
                      ? config_.get_chunk_downloader_timeout_secs()
                      : 0U,
                  fsi, provider_, read_state, *this);
              open_file_lookup_[api_path] = f;
              event_system::instance().raise<download_restored>(
                  fsi.api_path, fsi.source_path);
            } else {
              event_system::instance().raise<download_restore_failed>(
                  fsi.api_path, fsi.source_path,
                  "file size mismatch|expected|" + std::to_string(fsi.size) +
                      "|actual|" + std::to_string(file_size));
            }
          } else {
            event_system::instance().raise<download_restore_failed>(
                fsi.api_path, fsi.source_path,
                "failed to get file size: " +
                    std::to_string(utils::get_last_error_code()));
          }
        } else {
          event_system::instance().raise<download_restore_failed>(
              fsi.api_path, fsi.source_path,
              "source path mismatch|expected|" + source_path + "|actual|" +
                  fsi.source_path);
        }
      } else {
        event_system::instance().raise<download_restore_failed>(
            api_path, source_path,
            "failed to get filesystem item|" + api_error_to_string(res));
      }
    }

    upload_thread_ =
        std::make_unique<std::thread>([this] { upload_handler(); });
    polling::instance().set_callback(
        {"timed_out_close", polling::frequency::second,
         [this]() { this->close_timed_out_files(); }});
    event_system::instance().raise<service_started>("file_manager");
  }
}

void file_manager::stop() {
  if (not stop_requested_) {
    event_system::instance().raise<service_shutdown_begin>("file_manager");
    polling::instance().remove_callback("timed_out_close");
    stop_requested_ = true;

    unique_mutex_lock upload_lock(upload_mtx_);
    upload_notify_.notify_all();
    upload_lock.unlock();

    if (upload_thread_) {
      upload_thread_->join();
      upload_thread_.reset();
    }

    open_file_lookup_.clear();
    open_handle_lookup_.clear();

    upload_lock.lock();
    for (auto &kv : upload_lookup_) {
      kv.second->stop();
    }
    upload_notify_.notify_all();
    upload_lock.unlock();

    while (not upload_lookup_.empty()) {
      upload_lock.lock();
      if (not upload_lookup_.empty()) {
        upload_notify_.wait_for(upload_lock, 1s);
      }
      upload_notify_.notify_all();
      upload_lock.unlock();
    }

    event_system::instance().raise<service_shutdown_end>("file_manager");
  }
}

void file_manager::store_resume(const i_open_file &o) {
  if (provider_.is_direct_only()) {
    return;
  }

  recur_mutex_lock open_lock(open_file_mtx_);
  const auto res = db_->Put(rocksdb::WriteOptions(), default_family_,
                            o.get_api_path(), create_resume_entry(o).dump());
  if (res.ok()) {
    event_system::instance().raise<download_stored>(o.get_api_path(),
                                                    o.get_source_path());
  } else {
    event_system::instance().raise<download_stored_failed>(
        o.get_api_path(), o.get_source_path(), res.ToString());
  }
}

void file_manager::upload_completed(const file_upload_completed &e) {
  unique_mutex_lock upload_lock(upload_mtx_);

  if (not utils::string::to_bool(e.get_cancelled().get<std::string>())) {
    const auto err = api_error_from_string(e.get_result().get<std::string>());
    switch (err) {
    case api_error::success: {
      auto iterator = std::unique_ptr<rocksdb::Iterator>(
          db_->NewIterator(rocksdb::ReadOptions(), upload_active_family_));
      for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
        const auto parts =
            utils::string::split(iterator->key().ToString(), ':');
        if (parts[1U] == e.get_api_path().get<std::string>()) {
          db_->Delete(rocksdb::WriteOptions(), upload_active_family_,
                      iterator->key());
          break;
        }
      }
    } break;
    case api_error::upload_stopped: {
      event_system::instance().raise<file_upload_retry>(e.get_api_path(),
                                                        e.get_source(), err);
      queue_upload(e.get_api_path(), e.get_source(), true);
      upload_notify_.wait_for(upload_lock, 5s);
    } break;
    default: {
      bool exists{};
      auto res = provider_.is_file(e.get_api_path(), exists);
      if ((res == api_error::success && not exists) ||
          not utils::file::is_file(e.get_source())) {
        event_system::instance().raise<file_upload_not_found>(e.get_api_path(),
                                                              e.get_source());
        remove_upload(e.get_api_path(), true);
        return;
      }

      event_system::instance().raise<file_upload_retry>(e.get_api_path(),
                                                        e.get_source(), err);
      queue_upload(e.get_api_path(), e.get_source(), true);
      upload_notify_.wait_for(upload_lock, 5s);
    } break;
    }
    upload_lookup_.erase(e.get_api_path());
  }
  upload_notify_.notify_all();
}

void file_manager::upload_handler() {
  while (not stop_requested_) {
    unique_mutex_lock upload_lock(upload_mtx_);
    if (not stop_requested_) {
      if (upload_lookup_.size() < config_.get_max_upload_count()) {
        auto iterator = std::unique_ptr<rocksdb::Iterator>(
            db_->NewIterator(rocksdb::ReadOptions(), upload_family_));
        iterator->SeekToFirst();
        if (iterator->Valid()) {
          const auto parts =
              utils::string::split(iterator->key().ToString(), ':');
          const auto api_path = parts[1u];
          const auto source_path = iterator->value().ToString();

          filesystem_item fsi{};
          auto res = provider_.get_filesystem_item(api_path, false, fsi);
          switch (res) {
          case api_error::item_not_found: {
            event_system::instance().raise<file_upload_not_found>(api_path,
                                                                  source_path);
            remove_upload(parts[1u], true);
          } break;
          case api_error::success: {
            upload_lookup_[fsi.api_path] =
                std::make_unique<upload>(fsi, provider_);
            if (db_->Delete(rocksdb::WriteOptions(), upload_family_,
                            iterator->key())
                    .ok()) {
              db_->Put(rocksdb::WriteOptions(), upload_active_family_,
                       iterator->key(), iterator->value());
            }
          } break;
          default: {
            event_system::instance().raise<file_upload_retry>(api_path,
                                                              source_path, res);
            queue_upload(api_path, source_path, true);
            upload_notify_.wait_for(upload_lock, 5s);
          } break;
          }
        } else {
          iterator.release();
          upload_notify_.wait(upload_lock);
        }
      } else {
        upload_notify_.wait(upload_lock);
      }
    }
    upload_notify_.notify_all();
  }
}

void file_manager::update_used_space(std::uint64_t &used_space) const {
  recur_mutex_lock open_lock(open_file_mtx_);
  for (const auto &of : open_file_lookup_) {
    std::uint64_t file_size{};
    auto res = provider_.get_file_size(of.second->get_api_path(), file_size);
    if ((res == api_error::success) &&
        (file_size != of.second->get_file_size()) &&
        (used_space >= file_size)) {
      used_space -= file_size;
      used_space += of.second->get_file_size();
    }
  }
}
} // namespace repertory
