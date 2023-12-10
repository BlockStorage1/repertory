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
#include "database/db_common.hpp"
#include "database/db_insert.hpp"
#include "database/db_select.hpp"
#include "file_manager/events.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "types/startup_exception.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/polling.hpp"
#include "utils/utils.hpp"

namespace {
[[nodiscard]] auto create_resume_entry(const repertory::i_open_file &file)
    -> json {
  return {
      {"chunk_size", file.get_chunk_size()},
      {"path", file.get_api_path()},
      {"read_state",
       repertory::utils::string::from_dynamic_bitset(file.get_read_state())},
      {"source", file.get_source_path()},
  };
}

void restore_resume_entry(const json &resume_entry, std::string &api_path,
                          std::size_t &chunk_size,
                          boost::dynamic_bitset<> &read_state,
                          std::string &source_path) {
  api_path = resume_entry["path"].get<std::string>();
  chunk_size = resume_entry["chunk_size"].get<std::size_t>();
  read_state = repertory::utils::string::to_dynamic_bitset(
      resume_entry["read_state"].get<std::string>());
  source_path = resume_entry["source"].get<std::string>();
}

const std::string resume_table = "resume";
const std::string upload_table = "upload";
const std::string upload_active_table = "upload_active";
const std::map<std::string, std::string> sql_create_tables{
    {
        {resume_table},
        {
            "CREATE TABLE IF NOT EXISTS " + resume_table +
                "("
                "api_path TEXT PRIMARY KEY ASC, "
                "data TEXT"
                ");",
        },
    },
    {
        {upload_table},
        {
            "CREATE TABLE IF NOT EXISTS " + upload_table +
                "("
                "api_path TEXT PRIMARY KEY ASC, "
                "date_time INTEGER, "
                "source_path TEXT"
                ");",
        },
    },
    {
        {upload_active_table},
        {
            "CREATE TABLE IF NOT EXISTS " + upload_active_table +
                "("
                "api_path TEXT PRIMARY KEY ASC, "
                "source_path TEXT"
                ");",
        },
    },
};
} // namespace

namespace repertory {
file_manager::file_manager(app_config &config, i_provider &provider)
    : config_(config), provider_(provider) {
  if (not provider_.is_direct_only()) {
    auto db_path =
        utils::path::combine(config.get_data_directory(), {"file_manager.db3"});

    sqlite3 *db3{nullptr};
    auto res =
        sqlite3_open_v2(db_path.c_str(), &db3,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (res != SQLITE_OK) {
      throw startup_exception("failed to open db|" + db_path + '|' +
                              std::to_string(res) + '|' + sqlite3_errstr(res));
    }
    db_.reset(db3);

    for (const auto &create_item : sql_create_tables) {
      std::string err;
      if (not db::execute_sql(*db_, create_item.second, err)) {
        db_.reset();
        throw startup_exception(err);
      }
    }

    db::set_journal_mode(*db_);

    E_SUBSCRIBE_EXACT(file_upload_completed,
                      [this](const file_upload_completed &completed) {
                        this->upload_completed(completed);
                      });
  }
}

file_manager::~file_manager() {
  stop();
  db_.reset();

  E_CONSUMER_RELEASE();
}

void file_manager::close(std::uint64_t handle) {
  recur_mutex_lock file_lock(open_file_mtx_);
  auto closeable_file = get_open_file_by_handle(handle);
  if (closeable_file) {
    closeable_file->remove(handle);
  }
}

void file_manager::close_all(const std::string &api_path) {
  recur_mutex_lock file_lock(open_file_mtx_);
  std::vector<std::uint64_t> handles;
  auto file_iter = open_file_lookup_.find(api_path);
  if (file_iter == open_file_lookup_.end()) {
    return;
  }

  auto closeable_file = file_iter->second;

  handles = closeable_file->get_handles();
  for (auto handle : handles) {
    closeable_file->remove(handle);
  }
}

void file_manager::close_timed_out_files() {
  unique_recur_mutex_lock file_lock(open_file_mtx_);
  auto closeable_list =
      std::accumulate(open_file_lookup_.begin(), open_file_lookup_.end(),
                      std::vector<std::shared_ptr<i_closeable_open_file>>{},
                      [](auto items, const auto &item) -> auto {
                        if (item.second->get_open_file_count() == 0U &&
                            item.second->can_close()) {
                          items.push_back(item.second);
                        }
                        return items;
                      });
  for (const auto &closeable_file : closeable_list) {
    open_file_lookup_.erase(closeable_file->get_api_path());
  }
  file_lock.unlock();

  for (auto &closeable_file : closeable_list) {
    closeable_file->close();
    event_system::instance().raise<item_timeout>(
        closeable_file->get_api_path());
  }
  closeable_list.clear();
}

auto file_manager::create(const std::string &api_path, api_meta_map &meta,
                          open_file_data ofd, std::uint64_t &handle,
                          std::shared_ptr<i_open_file> &file) -> api_error {
  recur_mutex_lock file_lock(open_file_mtx_);
  auto res = provider_.create_file(api_path, meta);
  if (res != api_error::success && res != api_error::item_exists) {
    return res;
  }

  return open(api_path, false, ofd, handle, file);
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

  open_file_lookup_.erase(api_path);

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
  if (++next_handle_ == 0U) {
    next_handle_++;
  }

  return next_handle_;
}

auto file_manager::get_open_file_by_handle(std::uint64_t handle) const
    -> std::shared_ptr<i_closeable_open_file> {
  auto file_iter =
      std::find_if(open_file_lookup_.begin(), open_file_lookup_.end(),
                   [&handle](const auto &item) -> bool {
                     return item.second->has_handle(handle);
                   });
  return (file_iter == open_file_lookup_.end()) ? nullptr : file_iter->second;
}

auto file_manager::get_open_file_count(const std::string &api_path) const
    -> std::size_t {
  auto file_iter = open_file_lookup_.find(api_path);
  return (file_iter == open_file_lookup_.end())
             ? 0U
             : file_iter->second->get_open_file_count();
}

auto file_manager::get_open_file(std::uint64_t handle, bool write_supported,
                                 std::shared_ptr<i_open_file> &file) -> bool {
  recur_mutex_lock open_lock(open_file_mtx_);
  auto file_ptr = get_open_file_by_handle(handle);
  if (not file_ptr) {
    return false;
  }

  if (write_supported && not file_ptr->is_write_supported()) {
    auto writeable_file = std::make_shared<open_file>(
        utils::encryption::encrypting_reader::get_data_chunk_size(),
        config_.get_enable_chunk_download_timeout()
            ? config_.get_chunk_downloader_timeout_secs()
            : 0U,
        file_ptr->get_filesystem_item(), file_ptr->get_open_data(), provider_,
        *this);
    open_file_lookup_[file_ptr->get_api_path()] = writeable_file;
    file = writeable_file;
    return true;
  }

  file = file_ptr;
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
  for (const auto &item : open_file_lookup_) {
    ret[item.first] = item.second->get_open_file_count();
  }

  return ret;
}

auto file_manager::get_open_handle_count() const -> std::size_t {
  recur_mutex_lock open_lock(open_file_mtx_);
  return std::accumulate(
      open_file_lookup_.begin(), open_file_lookup_.end(), std::size_t(0U),
      [](std::size_t count, const auto &item) -> std::size_t {
        return count + item.second->get_open_file_count();
      });
}

auto file_manager::get_stored_downloads() const -> std::vector<json> {
  std::vector<json> ret;
  if (not provider_.is_direct_only()) {
    auto result = db::db_select{*db_.get(), resume_table}.go();
    if (not result.ok()) {
      return ret;
    }

    while (result.has_row()) {
      try {
        std::optional<db::db_select::row> row;
        if (not result.get_row(row)) {
          continue;
        }
        if (not row.has_value()) {
          continue;
        }

        ret.push_back(row.value().get_column("data").get_value_as_json());
      } catch (const std::exception &ex) {
        utils::error::raise_error(__FUNCTION__, ex, "query error");
      }
    }
  }

  return ret;
}

auto file_manager::handle_file_rename(const std::string &from_api_path,
                                      const std::string &to_api_path)
    -> api_error {
  std::string source_path{};
  bool should_upload{upload_lookup_.contains(from_api_path)};
  if (should_upload) {
    source_path = upload_lookup_.at(from_api_path)->get_source_path();
    remove_upload(from_api_path);
  } else {
    auto result = db::db_select{*db_.get(), upload_table}
                      .column("source_path")
                      .where("api_path")
                      .equals(from_api_path)
                      .go();
    std::optional<db::db_select::row> row;
    should_upload = result.get_row(row) && row.has_value();
    if (should_upload) {
      source_path = row->get_column("source_path").get_value<std::string>();
      remove_upload(from_api_path);
    }
  }

  auto ret = provider_.rename_file(from_api_path, to_api_path);
  if (ret == api_error::success) {
    swap_renamed_items(from_api_path, to_api_path);
    if (should_upload) {
      queue_upload(to_api_path, source_path, false);
    }
  } else if (should_upload) {
    queue_upload(from_api_path, source_path, false);
  }

  return ret;
}

auto file_manager::has_no_open_file_handles() const -> bool {
  return get_open_handle_count() == 0U;
}

auto file_manager::is_processing(const std::string &api_path) const -> bool {
  if (provider_.is_direct_only()) {
    return false;
  }

  unique_mutex_lock upload_lock(upload_mtx_);
  if (upload_lookup_.find(api_path) != upload_lookup_.end()) {
    return true;
  }
  upload_lock.unlock();

  db::db_select query{*db_.get(), upload_table};
  if (query.where("api_path").equals(api_path).go().has_row()) {
    return true;
  };

  recur_mutex_lock open_lock(open_file_mtx_);
  auto file_iter = open_file_lookup_.find(api_path);
  return (file_iter == open_file_lookup_.end())
             ? false
             : file_iter->second->is_modified() ||
                   not file_iter->second->is_complete();
}

auto file_manager::open(const std::string &api_path, bool directory,
                        const open_file_data &ofd, std::uint64_t &handle,
                        std::shared_ptr<i_open_file> &file) -> api_error {
  recur_mutex_lock open_lock(open_file_mtx_);
  return open(api_path, directory, ofd, handle, file, nullptr);
}

auto file_manager::open(const std::string &api_path, bool directory,
                        const open_file_data &ofd, std::uint64_t &handle,
                        std::shared_ptr<i_open_file> &file,
                        std::shared_ptr<i_closeable_open_file> closeable_file)
    -> api_error {
  const auto create_and_add_handle =
      [&](std::shared_ptr<i_closeable_open_file> cur_file) {
        handle = get_next_handle();
        cur_file->add(handle, ofd);
        file = cur_file;
      };

  auto file_iter = open_file_lookup_.find(api_path);
  if (file_iter != open_file_lookup_.end()) {
    create_and_add_handle(file_iter->second);
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
    res = provider_.set_item_meta(fsi.api_path, META_SOURCE, fsi.source_path);
    if (res != api_error::success) {
      return res;
    }
  }

  if (not closeable_file) {
    closeable_file = std::make_shared<open_file>(
        utils::encryption::encrypting_reader::get_data_chunk_size(),
        config_.get_enable_chunk_download_timeout()
            ? config_.get_chunk_downloader_timeout_secs()
            : 0U,
        fsi, provider_, *this);
  }
  open_file_lookup_[api_path] = closeable_file;
  create_and_add_handle(closeable_file);

  return api_error::success;
}

void file_manager::queue_upload(const i_open_file &file) {
  return queue_upload(file.get_api_path(), file.get_source_path(), false);
}

void file_manager::queue_upload(const std::string &api_path,
                                const std::string &source_path, bool no_lock) {
  if (provider_.is_direct_only()) {
    return;
  }

  std::unique_ptr<mutex_lock> lock;
  if (not no_lock) {
    lock = std::make_unique<mutex_lock>(upload_mtx_);
  }
  remove_upload(api_path, true);

  auto result =
      db::db_insert{*db_.get(), upload_table}
          .or_replace()
          .column_value("api_path", api_path)
          .column_value("date_time",
                        static_cast<std::int64_t>(utils::get_file_time_now()))
          .column_value("source_path", source_path)
          .go();
  if (result.ok()) {
    remove_resume(api_path, source_path);
    event_system::instance().raise<file_upload_queued>(api_path, source_path);
  } else {
    event_system::instance().raise<file_upload_failed>(
        api_path, source_path,
        std::to_string(result.get_error()) + '|' + result.get_error_str());
  }

  if (not no_lock) {
    upload_notify_.notify_all();
  }
}

auto file_manager::remove_file(const std::string &api_path) -> api_error {
  recur_mutex_lock open_lock(open_file_mtx_);
  auto file_iter = open_file_lookup_.find(api_path);
  if (file_iter != open_file_lookup_.end() &&
      file_iter->second->is_modified()) {
    return api_error::file_in_use;
  }

  filesystem_item fsi{};
  auto res = provider_.get_filesystem_item(api_path, false, fsi);
  if (res != api_error::success) {
    return res;
  }

  remove_upload(api_path);
  close_all(api_path);

  res = provider_.remove_file(api_path);
  if (res != api_error::success) {
    return res;
  }

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
  auto result = db::db_select{*db_.get(), resume_table}
                    .delete_query()
                    .where("api_path")
                    .equals(api_path)
                    .go();
  if (result.ok()) {
    event_system::instance().raise<download_stored_removed>(api_path,
                                                            source_path);
  }
}

void file_manager::remove_upload(const std::string &api_path) {
  remove_upload(api_path, false);
}

void file_manager::remove_upload(const std::string &api_path, bool no_lock) {
  if (provider_.is_direct_only()) {
    return;
  }

  std::unique_ptr<mutex_lock> lock;
  if (not no_lock) {
    lock = std::make_unique<mutex_lock>(upload_mtx_);
  }

  auto file_iter = upload_lookup_.find(api_path);
  if (file_iter == upload_lookup_.end()) {
    auto result = db::db_select{*db_.get(), upload_table}
                      .delete_query()
                      .where("api_path")
                      .equals(api_path)
                      .go();
    if (result.ok()) {
      result = db::db_select{*db_.get(), upload_active_table}
                   .delete_query()
                   .where("api_path")
                   .equals(api_path)
                   .go();
      event_system::instance().raise<file_upload_removed>(api_path);
    }
  } else {
    auto result = db::db_select{*db_.get(), upload_active_table}
                      .delete_query()
                      .where("api_path")
                      .equals(api_path)
                      .go();
    if (result.ok()) {
      event_system::instance().raise<file_upload_removed>(api_path);
    }

    file_iter->second->cancel();
    upload_lookup_.erase(api_path);
  }

  if (not no_lock) {
    upload_notify_.notify_all();
  }
}

auto file_manager::rename_directory(const std::string &from_api_path,
                                    const std::string &to_api_path)
    -> api_error {
  if (not provider_.is_rename_supported()) {
    return api_error::not_implemented;
  }

  unique_recur_mutex_lock lock(open_file_mtx_);
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

  // TODO add provider bulk rename support
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
    auto &api_path = list[i].api_path;
    if ((api_path != ".") && (api_path != "..")) {
      auto old_api_path = api_path;
      auto new_api_path = utils::path::create_api_path(utils::path::combine(
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

  unique_recur_mutex_lock lock(open_file_mtx_);

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

    res = remove_file(to_api_path);
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
  polling::instance().set_callback(
      {"timed_out_close", polling::frequency::second,
       [this]() { this->close_timed_out_files(); }});

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

    auto result = db::db_select{*db_.get(), upload_active_table}.go();
    while (result.has_row()) {
      try {
        std::optional<db::db_select::row> row;
        if (result.get_row(row) && row.has_value()) {
          active_items.emplace_back(active_item{
              row->get_column("api_path").get_value<std::string>(),
              row->get_column("source_path").get_value<std::string>(),
          });
        }
      } catch (const std::exception &ex) {
        utils::error::raise_error(__FUNCTION__, ex, "query error");
      }
    }

    for (const auto &active_item : active_items) {
      queue_upload(active_item.api_path, active_item.source_path, false);
    }
    active_items.clear();

    result = db::db_select{*db_.get(), resume_table}.go();
    if (not result.ok()) {
      return;
    }

    while (result.has_row()) {
      try {
        std::optional<db::db_select::row> row;
        if (not(result.get_row(row) && row.has_value())) {
          return;
        }

        auto resume_entry = row.value().get_column("data").get_value_as_json();

        std::string api_path;
        std::string source_path;
        std::size_t chunk_size{};
        boost::dynamic_bitset<> read_state;
        restore_resume_entry(resume_entry, api_path, chunk_size, read_state,
                             source_path);

        filesystem_item fsi{};
        auto res = provider_.get_filesystem_item(api_path, false, fsi);
        if (res == api_error::success) {
          if (source_path == fsi.source_path) {
            std::uint64_t file_size{};
            if (utils::file::get_file_size(fsi.source_path, file_size)) {
              if (file_size == fsi.size) {
                auto closeable_file = std::make_shared<open_file>(
                    chunk_size,
                    config_.get_enable_chunk_download_timeout()
                        ? config_.get_chunk_downloader_timeout_secs()
                        : 0U,
                    fsi, provider_, read_state, *this);
                open_file_lookup_[api_path] = closeable_file;
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
      } catch (const std::exception &ex) {
        utils::error::raise_error(__FUNCTION__, ex, "query error");
      }
    }
  }

  upload_thread_ = std::make_unique<std::thread>([this] { upload_handler(); });
  event_system::instance().raise<service_started>("file_manager");
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

    upload_lock.lock();
    for (auto &item : upload_lookup_) {
      item.second->stop();
    }
    upload_notify_.notify_all();
    upload_lock.unlock();

    while (not upload_lookup_.empty()) {
      upload_lock.lock();
      if (not upload_lookup_.empty()) {
        upload_notify_.wait_for(upload_lock, 1ms);
      }
      upload_notify_.notify_all();
      upload_lock.unlock();
    }

    event_system::instance().raise<service_shutdown_end>("file_manager");
  }
}

void file_manager::store_resume(const i_open_file &file) {
  if (provider_.is_direct_only()) {
    return;
  }

  auto result = db::db_insert{*db_.get(), resume_table}
                    .or_replace()
                    .column_value("api_path", file.get_api_path())
                    .column_value("data", create_resume_entry(file).dump())
                    .go();
  if (result.ok()) {
    event_system::instance().raise<download_stored>(file.get_api_path(),
                                                    file.get_source_path());
    return;
  }

  event_system::instance().raise<download_stored_failed>(
      file.get_api_path(), file.get_source_path(),
      "failed to insert|" + std::to_string(result.get_error()) + '|' +
          result.get_error_str());
}

void file_manager::swap_renamed_items(std::string from_api_path,
                                      std::string to_api_path) {
  auto file_iter = open_file_lookup_.find(from_api_path);
  if (file_iter != open_file_lookup_.end()) {
    open_file_lookup_[to_api_path] = open_file_lookup_[from_api_path];
    open_file_lookup_.erase(from_api_path);
    open_file_lookup_[to_api_path]->set_api_path(to_api_path);
  }
}

void file_manager::upload_completed(const file_upload_completed &evt) {
  unique_mutex_lock upload_lock(upload_mtx_);

  if (not utils::string::to_bool(evt.get_cancelled().get<std::string>())) {
    auto err = api_error_from_string(evt.get_result().get<std::string>());
    switch (err) {
    case api_error::success: {
      auto result = db::db_select{*db_.get(), upload_active_table}
                        .delete_query()
                        .where("api_path")
                        .equals(evt.get_api_path().get<std::string>())
                        .go();
      if (not result.ok()) {
        utils::error::raise_api_path_error(
            __FUNCTION__, evt.get_api_path().get<std::string>(),
            evt.get_source().get<std::string>(),
            "failed to remove from to upload_active table");
      }
    } break;

    case api_error::upload_stopped: {
      event_system::instance().raise<file_upload_retry>(evt.get_api_path(),
                                                        evt.get_source(), err);
      queue_upload(evt.get_api_path(), evt.get_source(), true);
      upload_notify_.wait_for(upload_lock, 5s);
    } break;

    default: {
      bool exists{};
      auto res = provider_.is_file(evt.get_api_path(), exists);
      if ((res == api_error::success && not exists) ||
          not utils::file::is_file(evt.get_source())) {
        event_system::instance().raise<file_upload_not_found>(
            evt.get_api_path(), evt.get_source());
        remove_upload(evt.get_api_path(), true);
        upload_notify_.notify_all();
        return;
      }

      event_system::instance().raise<file_upload_retry>(evt.get_api_path(),
                                                        evt.get_source(), err);
      queue_upload(evt.get_api_path(), evt.get_source(), true);
      upload_notify_.wait_for(upload_lock, 5s);
    } break;
    }

    upload_lookup_.erase(evt.get_api_path());
  }

  upload_notify_.notify_all();
}

void file_manager::upload_handler() {
  while (not stop_requested_) {
    unique_mutex_lock upload_lock(upload_mtx_);
    if (not stop_requested_) {
      if (upload_lookup_.size() < config_.get_max_upload_count()) {
        auto result = db::db_select{*db_.get(), upload_table}
                          .order_by("api_path", true)
                          .limit(1)
                          .go();
        try {
          std::optional<db::db_select::row> row;
          if (result.get_row(row) && row.has_value()) {
            auto api_path =
                row->get_column("api_path").get_value<std::string>();
            auto source_path =
                row->get_column("source_path").get_value<std::string>();

            filesystem_item fsi{};
            auto res = provider_.get_filesystem_item(api_path, false, fsi);
            switch (res) {
            case api_error::item_not_found: {
              event_system::instance().raise<file_upload_not_found>(
                  api_path, source_path);
              remove_upload(api_path, true);
            } break;

            case api_error::success: {
              upload_lookup_[fsi.api_path] =
                  std::make_unique<upload>(fsi, provider_);
              auto del_res = db::db_select{*db_.get(), upload_table}
                                 .delete_query()
                                 .where("api_path")
                                 .equals(api_path)
                                 .go();
              if (del_res.ok()) {
                auto ins_res = db::db_insert{*db_.get(), upload_active_table}
                                   .column_value("api_path", api_path)
                                   .column_value("source_path", source_path)
                                   .go();
                if (not ins_res.ok()) {
                  utils::error::raise_api_path_error(
                      __FUNCTION__, api_path, source_path,
                      "failed to add to upload_active table");
                }
              }
            } break;

            default: {
              event_system::instance().raise<file_upload_retry>(
                  api_path, source_path, res);
              queue_upload(api_path, source_path, true);
              upload_notify_.wait_for(upload_lock, 5s);
            } break;
            }
          }
        } catch (const std::exception &ex) {
          utils::error::raise_error(__FUNCTION__, ex, "query error");
        }
      }
    }

    upload_notify_.notify_all();
  }
}

void file_manager::update_used_space(std::uint64_t &used_space) const {
  recur_mutex_lock open_lock(open_file_mtx_);
  for (const auto &item : open_file_lookup_) {
    std::uint64_t file_size{};
    auto res = provider_.get_file_size(item.second->get_api_path(), file_size);
    if ((res == api_error::success) &&
        (file_size != item.second->get_file_size()) &&
        (used_space >= file_size)) {
      used_space -= file_size;
      used_space += item.second->get_file_size();
    }
  }
}
} // namespace repertory
