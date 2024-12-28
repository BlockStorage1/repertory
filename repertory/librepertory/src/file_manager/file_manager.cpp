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
#include "file_manager/file_manager.hpp"

#include "app_config.hpp"
#include "db/file_mgr_db.hpp"
#include "file_manager/cache_size_mgr.hpp"
#include "file_manager/direct_open_file.hpp"
#include "file_manager/events.hpp"
#include "file_manager/open_file.hpp"
#include "file_manager/open_file_base.hpp"
#include "file_manager/ring_buffer_open_file.hpp"
#include "file_manager/upload.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "utils/common.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/polling.hpp"

namespace repertory {
file_manager::file_manager(app_config &config, i_provider &provider)
    : config_(config), provider_(provider) {
  mgr_db_ = create_file_mgr_db(config);

  if (provider_.is_read_only()) {
    return;
  }

  E_SUBSCRIBE_EXACT(file_upload_completed,
                    [this](const file_upload_completed &completed) {
                      this->upload_completed(completed);
                    });
}

file_manager::~file_manager() {
  stop();
  mgr_db_.reset();

  E_CONSUMER_RELEASE();
}

void file_manager::close(std::uint64_t handle) {
  unique_recur_mutex_lock file_lock(open_file_mtx_);
  auto closeable_file = get_open_file_by_handle(handle);
  if (not closeable_file) {
    return;
  }
  file_lock.unlock();

  closeable_file->remove(handle);
}

auto file_manager::close_all(const std::string &api_path) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  unique_recur_mutex_lock file_lock(open_file_mtx_);
  auto file_iter = open_file_lookup_.find(api_path);
  if (file_iter == open_file_lookup_.end()) {
    return false;
  }

  auto closeable_file = file_iter->second;
  open_file_lookup_.erase(api_path);
  file_lock.unlock();

  closeable_file->remove_all();
  closeable_file->close();

  return closeable_file->get_allocated();
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
  if (res != api_error::success) {
#if !defined(_WIN32)
    if (res != api_error::item_exists) {
#endif //! defined (_WIN32)
      return res;
#if !defined(_WIN32)
    }
#endif //! defined (_WIN32)
  }

  return open(api_path, false, ofd, handle, file);
}

auto file_manager::evict_file(const std::string &api_path) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  if (provider_.is_read_only()) {
    return false;
  }

  unique_recur_mutex_lock open_lock(open_file_mtx_);
  if (is_processing(api_path)) {
    return false;
  }

  if (get_open_file_count(api_path) != 0U) {
    return false;
  }

  filesystem_item fsi{};
  auto res = provider_.get_filesystem_item(api_path, false, fsi);
  if (res != api_error::success) {
    return false;
  }

  if (fsi.source_path.empty()) {
    return false;
  }

  std::string pinned;
  res = provider_.get_item_meta(api_path, META_PINNED, pinned);
  if (res != api_error::success && res != api_error::item_not_found) {
    utils::error::raise_api_path_error(std::string{function_name}, api_path,
                                       res, "failed to get pinned status");
    return false;
  }

  if (not pinned.empty() && utils::string::to_bool(pinned)) {
    return false;
  }

  std::shared_ptr<i_closeable_open_file> closeable_file;
  if (open_file_lookup_.contains(api_path)) {
    closeable_file = open_file_lookup_.at(api_path);
  }

  open_file_lookup_.erase(api_path);
  open_lock.unlock();

  auto allocated = closeable_file ? closeable_file->get_allocated() : true;
  closeable_file.reset();

  auto removed = remove_source_and_shrink_cache(api_path, fsi.source_path,
                                                fsi.size, allocated);
  if (removed) {
    event_system::instance().raise<filesystem_item_evicted>(api_path,
                                                            fsi.source_path);
  }

  return removed;
}

auto file_manager::get_directory_items(const std::string &api_path) const
    -> directory_item_list {
  REPERTORY_USES_FUNCTION_NAME();

  directory_item_list list{};
  auto res = provider_.get_directory_items(api_path, list);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(function_name, api_path, res,
                                       "failed to get directory list");
  }
  return list;
}

auto file_manager::get_next_handle() -> std::uint64_t {
  if (++next_handle_ == 0U) {
    ++next_handle_;
  }

  return next_handle_;
}

auto file_manager::get_open_file_by_handle(std::uint64_t handle) const
    -> std::shared_ptr<i_closeable_open_file> {
  auto file_iter =
      std::find_if(open_file_lookup_.begin(), open_file_lookup_.end(),
                   [&handle](auto &&item) -> bool {
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
  unique_recur_mutex_lock open_lock(open_file_mtx_);
  auto file_ptr = get_open_file_by_handle(handle);
  if (not file_ptr) {
    return false;
  }

  if (write_supported && not file_ptr->is_write_supported()) {
    auto writeable_file = std::make_shared<open_file>(
        utils::encryption::encrypting_reader::get_data_chunk_size(),
        config_.get_enable_download_timeout()
            ? config_.get_download_timeout_secs()
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

auto file_manager::get_stored_downloads() const
    -> std::vector<i_file_mgr_db::resume_entry> {
  REPERTORY_USES_FUNCTION_NAME();

  if (provider_.is_read_only()) {
    return {};
  }

  return mgr_db_->get_resume_list();
}

auto file_manager::handle_file_rename(const std::string &from_api_path,
                                      const std::string &to_api_path)
    -> api_error {
  std::string source_path{};
  auto file_iter = open_file_lookup_.find(from_api_path);
  if (file_iter != open_file_lookup_.end()) {
    source_path = file_iter->second->get_source_path();
  }

  auto should_upload{upload_lookup_.contains(from_api_path)};
  if (should_upload) {
    if (source_path.empty()) {
      source_path = upload_lookup_.at(from_api_path)->get_source_path();
    }
  } else {
    auto upload = mgr_db_->get_upload(from_api_path);
    should_upload = upload.has_value();
    if (should_upload && source_path.empty()) {
      source_path = upload->source_path;
    }
  }

  remove_upload(from_api_path, true);

  auto ret = provider_.rename_file(from_api_path, to_api_path);
  if (ret != api_error::success) {
    queue_upload(from_api_path, source_path, false);
    return ret;
  }

  swap_renamed_items(from_api_path, to_api_path, false);

  ret = source_path.empty()
            ? api_error::success
            : provider_.set_item_meta(to_api_path, META_SOURCE, source_path);

  if (should_upload) {
    queue_upload(to_api_path, source_path, false);
  }

  return ret;
}

auto file_manager::has_no_open_file_handles() const -> bool {
  return get_open_handle_count() == 0U;
}

auto file_manager::is_processing(const std::string &api_path) const -> bool {
  if (provider_.is_read_only()) {
    return false;
  }

  unique_mutex_lock upload_lock(upload_mtx_);
  if (upload_lookup_.find(api_path) != upload_lookup_.end()) {
    return true;
  }
  upload_lock.unlock();

  auto upload = mgr_db_->get_upload(api_path);
  if (upload.has_value()) {
    return true;
  };

  unique_recur_mutex_lock open_lock(open_file_mtx_);
  auto file_iter = open_file_lookup_.find(api_path);
  if (file_iter == open_file_lookup_.end()) {
    return false;
  }

  auto closeable_file = file_iter->second;
  open_lock.unlock();

  return closeable_file->is_write_supported()
             ? closeable_file->is_modified() ||
                   not closeable_file->is_complete()
             : false;
}

auto file_manager::open(const std::string &api_path, bool directory,
                        const open_file_data &ofd, std::uint64_t &handle,
                        std::shared_ptr<i_open_file> &file) -> api_error {
  recur_mutex_lock open_lock(open_file_mtx_);
  return open(api_path, directory, ofd, handle, file, nullptr);
}

auto file_manager::open(
    const std::string &api_path, bool directory, const open_file_data &ofd,
    std::uint64_t &handle, std::shared_ptr<i_open_file> &file,
    std::shared_ptr<i_closeable_open_file> closeable_file) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

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
    auto buffer_directory{
        utils::path::combine(config_.get_data_directory(), {"buffer"}),
    };

    auto chunk_size{
        utils::encryption::encrypting_reader::get_data_chunk_size(),
    };

    auto chunk_timeout = config_.get_enable_download_timeout()
                             ? config_.get_download_timeout_secs()
                             : 0U;

    auto ring_buffer_file_size{
        static_cast<std::uint64_t>(config_.get_ring_buffer_file_size()) *
            1024UL * 1024UL,
    };

    auto ring_size{ring_buffer_file_size / chunk_size};

    const auto get_download_type = [&](download_type type) -> download_type {
      if (directory || fsi.size == 0U || is_processing(api_path)) {
        return download_type::default_;
      }

      if (type == download_type::direct) {
        return type;
      }

      if (type == download_type::default_) {
        auto free_space =
            utils::file::get_free_drive_space(config_.get_cache_directory());
        if (fsi.size < free_space) {
          return download_type::default_;
        }
      }

      if (not ring_buffer_open_file::can_handle_file(fsi.size, chunk_size,
                                                     ring_size)) {
        return download_type::direct;
      }

      if (not utils::file::directory{buffer_directory}.create_directory()) {
        utils::error::raise_error(
            function_name, utils::get_last_error_code(),
            fmt::format("failed to create buffer directory|sp|{}",
                        buffer_directory));
        return download_type::direct;
      }

      auto free_space = utils::file::get_free_drive_space(buffer_directory);
      if (ring_buffer_file_size < free_space) {
        return download_type::ring_buffer;
      }

      return download_type::direct;
    };

    auto preferred_type = config_.get_preferred_download_type();
    auto type = get_download_type(directory ? download_type::default_
                                  : preferred_type == download_type::default_
                                      ? download_type::ring_buffer
                                      : preferred_type);
    if (not directory) {
      event_system::instance().raise<download_type_selected>(
          fsi.api_path, fsi.source_path, type);
    }

    switch (type) {
    case repertory::download_type::direct: {
      closeable_file = std::make_shared<direct_open_file>(
          chunk_size, chunk_timeout, fsi, provider_);
    } break;

    case repertory::download_type::ring_buffer: {
      closeable_file = std::make_shared<ring_buffer_open_file>(
          buffer_directory, chunk_size, chunk_timeout, fsi, provider_,
          ring_size);
    } break;

    default: {
      closeable_file = std::make_shared<open_file>(chunk_size, chunk_timeout,
                                                   fsi, provider_, *this);
    } break;
    }
  }

  open_file_lookup_[api_path] = closeable_file;
  create_and_add_handle(closeable_file);
  return api_error::success;
}

void file_manager::queue_upload(const i_open_file &file) {
  queue_upload(file.get_api_path(), file.get_source_path(), false);
}

void file_manager::queue_upload(const std::string &api_path,
                                const std::string &source_path, bool no_lock) {
  if (provider_.is_read_only()) {
    return;
  }

  std::unique_ptr<mutex_lock> upload_lock;
  if (not no_lock) {
    upload_lock = std::make_unique<mutex_lock>(upload_mtx_);
  }

  remove_upload(api_path, true);

  if (mgr_db_->add_upload(i_file_mgr_db::upload_entry{
          api_path,
          source_path,
      })) {
    remove_resume(api_path, source_path, true);
    event_system::instance().raise<file_upload_queued>(api_path, source_path);
  } else {
    event_system::instance().raise<file_upload_failed>(
        api_path, source_path, "failed to queue upload");
  }

  if (not no_lock) {
    upload_notify_.notify_all();
  }
}

auto file_manager::remove_file(const std::string &api_path) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  filesystem_item fsi{};
  auto res = provider_.get_filesystem_item(api_path, false, fsi);
  if (res != api_error::success) {
    return res;
  }

  auto allocated = close_all(api_path);

  unique_mutex_lock upload_lock(upload_mtx_);
  remove_upload(api_path, true);
  remove_resume(api_path, fsi.source_path, true);
  upload_notify_.notify_all();
  upload_lock.unlock();

  recur_mutex_lock open_lock(open_file_mtx_);

  res = provider_.remove_file(api_path);
  if (res != api_error::success) {
    return res;
  }

  remove_source_and_shrink_cache(api_path, fsi.source_path, fsi.size,
                                 allocated);
  return api_error::success;
}

void file_manager::remove_resume(const std::string &api_path,
                                 const std::string &source_path) {
  remove_resume(api_path, source_path, false);
}

void file_manager::remove_resume(const std::string &api_path,
                                 const std::string &source_path, bool no_lock) {
  if (provider_.is_read_only()) {
    return;
  }

  std::unique_ptr<mutex_lock> upload_lock;
  if (not no_lock) {
    upload_lock = std::make_unique<mutex_lock>(upload_mtx_);
  }

  if (mgr_db_->remove_resume(api_path)) {
    event_system::instance().raise<download_resume_removed>(api_path,
                                                            source_path);
  }

  if (not no_lock) {
    upload_notify_.notify_all();
  }
}

auto file_manager::remove_source_and_shrink_cache(
    const std::string &api_path, const std::string &source_path,
    std::uint64_t file_size, bool allocated) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  auto file = utils::file::file{source_path};
  auto source_size = file.exists() ? file.size().value_or(0U) : 0U;

  if (not file.remove()) {
    utils::error::raise_api_path_error(function_name, api_path, source_path,
                                       utils::get_last_error_code(),
                                       "failed to delete source");
    return false;
  }

  if (not allocated || source_size == 0U) {
    auto res = cache_size_mgr::instance().shrink(0U);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(function_name, api_path, source_path,
                                         res, "failed to shrink cache");
    }

    return true;
  }

  auto res = cache_size_mgr::instance().shrink(file_size);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(function_name, api_path, source_path,
                                       res, "failed to shrink cache");
  }

  return true;
}

void file_manager::remove_upload(const std::string &api_path) {
  remove_upload(api_path, false);
}

void file_manager::remove_upload(const std::string &api_path, bool no_lock) {
  REPERTORY_USES_FUNCTION_NAME();

  if (provider_.is_read_only()) {
    return;
  }

  std::unique_ptr<mutex_lock> upload_lock;
  if (not no_lock) {
    upload_lock = std::make_unique<mutex_lock>(upload_mtx_);
  }

  if (not mgr_db_->remove_upload(api_path)) {
    utils::error::raise_api_path_error(
        function_name, api_path, api_error::error, "failed to remove upload");
  }

  auto removed = mgr_db_->remove_upload_active(api_path);
  if (not removed) {
    utils::error::raise_api_path_error(function_name, api_path,
                                       api_error::error,
                                       "failed to remove active upload");
  }

  if (upload_lookup_.find(api_path) != upload_lookup_.end()) {
    upload_lookup_.at(api_path)->cancel();
    upload_lookup_.erase(api_path);
  }

  if (removed) {
    event_system::instance().raise<file_upload_removed>(api_path);
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

  swap_renamed_items(from_api_path, to_api_path, true);
  return api_error::success;
}

auto file_manager::rename_file(const std::string &from_api_path,
                               const std::string &to_api_path,
                               bool overwrite) -> api_error {
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

  // Check destination parent directory exists
  res = provider_.is_directory(utils::path::get_parent_api_path(to_api_path),
                               exists);
  if (res != api_error::success) {
    return res;
  }
  if (not exists) {
    return api_error::directory_not_found;
  }

  // Handle destination file exists (should overwrite)
  if (dest_exists) {
    res = remove_file(to_api_path);
    if ((res != api_error::success) && (res != api_error::item_not_found)) {
      return res;
    }
  }

  return handle_file_rename(from_api_path, to_api_path);
}

void file_manager::start() {
  REPERTORY_USES_FUNCTION_NAME();

  if (upload_thread_) {
    return;
  }

  stop_requested_ = false;

  polling::instance().set_callback({
      "timed_out_close",
      polling::frequency::second,
      [this](auto && /* stop_requested */) { this->close_timed_out_files(); },
  });

  if (provider_.is_read_only()) {
    stop_requested_ = false;
    return;
  }

  for (const auto &entry : mgr_db_->get_upload_active_list()) {
    queue_upload(entry.api_path, entry.source_path, false);
  }

  for (const auto &entry : mgr_db_->get_resume_list()) {
    try {
      filesystem_item fsi{};
      auto res = provider_.get_filesystem_item(entry.api_path, false, fsi);
      if (res != api_error::success) {
        event_system::instance().raise<download_restore_failed>(
            entry.api_path, entry.source_path,
            "failed to get filesystem item|" + api_error_to_string(res));
        continue;
      }

      if (entry.source_path != fsi.source_path) {
        event_system::instance().raise<download_restore_failed>(
            fsi.api_path, fsi.source_path,
            "source path mismatch|expected|" + entry.source_path + "|actual|" +
                fsi.source_path);
        continue;
      }

      auto opt_size = utils::file::file{fsi.source_path}.size();
      if (not opt_size.has_value()) {
        event_system::instance().raise<download_restore_failed>(
            fsi.api_path, fsi.source_path,
            "failed to get file size: " +
                std::to_string(utils::get_last_error_code()));
        continue;
      }

      auto file_size{opt_size.value()};
      if (file_size != fsi.size) {
        event_system::instance().raise<download_restore_failed>(
            fsi.api_path, fsi.source_path,
            "file size mismatch|expected|" + std::to_string(fsi.size) +
                "|actual|" + std::to_string(file_size));
        continue;
      }

      auto closeable_file =
          std::make_shared<open_file>(entry.chunk_size,
                                      config_.get_enable_download_timeout()
                                          ? config_.get_download_timeout_secs()
                                          : 0U,
                                      fsi, provider_, entry.read_state, *this);
      open_file_lookup_[entry.api_path] = closeable_file;
      event_system::instance().raise<download_restored>(fsi.api_path,
                                                        fsi.source_path);
    } catch (const std::exception &ex) {
      utils::error::raise_error(function_name, ex, "query error");
    }
  }

  upload_thread_ = std::make_unique<std::thread>([this] { upload_handler(); });
  event_system::instance().raise<service_started>("file_manager");
}

void file_manager::stop() {
  if (stop_requested_) {
    return;
  }

  event_system::instance().raise<service_shutdown_begin>("file_manager");

  stop_requested_ = true;

  polling::instance().remove_callback("timed_out_close");

  unique_mutex_lock upload_lock(upload_mtx_);
  upload_notify_.notify_all();
  upload_lock.unlock();

  if (upload_thread_) {
    upload_thread_->join();
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

  upload_thread_.reset();

  event_system::instance().raise<service_shutdown_end>("file_manager");
}

void file_manager::store_resume(const i_open_file &file) {
  if (provider_.is_read_only()) {
    return;
  }

  if (mgr_db_->add_resume(i_file_mgr_db::resume_entry{
          file.get_api_path(),
          file.get_chunk_size(),
          file.get_read_state(),
          file.get_source_path(),
      })) {
    event_system::instance().raise<download_resume_added>(
        file.get_api_path(), file.get_source_path());
    return;
  }

  event_system::instance().raise<download_resume_add_failed>(
      file.get_api_path(), file.get_source_path(), "failed to store resume");
}

void file_manager::swap_renamed_items(std::string from_api_path,
                                      std::string to_api_path, bool directory) {
  REPERTORY_USES_FUNCTION_NAME();

  auto file_iter = open_file_lookup_.find(from_api_path);
  if (file_iter != open_file_lookup_.end()) {
    auto closeable_file = std::move(open_file_lookup_[from_api_path]);
    open_file_lookup_.erase(from_api_path);
    closeable_file->set_api_path(to_api_path);
    open_file_lookup_[to_api_path] = std::move(closeable_file);
  }

  if (directory) {
    return;
  }

  if (mgr_db_->rename_resume(from_api_path, to_api_path)) {
    return;
  }

  utils::error::raise_api_path_error(function_name, to_api_path,
                                     api_error::error,
                                     "failed to update resume table");
}

void file_manager::upload_completed(const file_upload_completed &evt) {
  REPERTORY_USES_FUNCTION_NAME();

  unique_mutex_lock upload_lock(upload_mtx_);

  if (not utils::string::to_bool(evt.get_cancelled().get<std::string>())) {
    auto err = api_error_from_string(evt.get_result().get<std::string>());
    if (err == api_error::success) {
      if (not mgr_db_->remove_upload_active(
              evt.get_api_path().get<std::string>())) {
        utils::error::raise_api_path_error(
            function_name, evt.get_api_path().get<std::string>(),
            evt.get_source().get<std::string>(),
            "failed to remove from upload_active table");
      }

      upload_lookup_.erase(evt.get_api_path());
    } else {
      bool exists{};
      auto res = provider_.is_file(evt.get_api_path(), exists);
      if ((res == api_error::success && not exists) ||
          not utils::file::file(evt.get_source().get<std::string>()).exists()) {
        event_system::instance().raise<file_upload_not_found>(
            evt.get_api_path(), evt.get_source());
        remove_upload(evt.get_api_path(), true);
      } else {
        event_system::instance().raise<file_upload_retry>(
            evt.get_api_path(), evt.get_source(), err);

        queue_upload(evt.get_api_path(), evt.get_source(), true);
        upload_notify_.wait_for(upload_lock, 5s);
      }
    }
  }

  upload_notify_.notify_all();
}

void file_manager::upload_handler() {
  REPERTORY_USES_FUNCTION_NAME();

  while (not stop_requested_) {
    auto should_wait{true};
    unique_mutex_lock upload_lock(upload_mtx_);
    if (stop_requested_) {
      upload_notify_.notify_all();
      continue;
    }

    if (upload_lookup_.size() < config_.get_max_upload_count()) {
      try {
        auto entry = mgr_db_->get_next_upload();
        if (entry.has_value()) {
          filesystem_item fsi{};
          auto res = provider_.get_filesystem_item(entry->api_path, false, fsi);
          switch (res) {
          case api_error::item_not_found: {
            should_wait = false;
            event_system::instance().raise<file_upload_not_found>(
                entry->api_path, entry->source_path);
            remove_upload(entry->api_path, true);
          } break;

          case api_error::success: {
            should_wait = false;

            upload_lookup_[fsi.api_path] =
                std::make_unique<upload>(fsi, provider_);
            if (mgr_db_->remove_upload(entry->api_path)) {
              if (not mgr_db_->add_upload_active(
                      i_file_mgr_db::upload_active_entry{
                          entry->api_path,
                          entry->source_path,
                      })) {
                utils::error::raise_api_path_error(
                    function_name, entry->api_path, entry->source_path,
                    "failed to add to upload_active table");
              }
            }
          } break;

          default: {
            event_system::instance().raise<file_upload_retry>(
                entry->api_path, entry->source_path, res);
            queue_upload(entry->api_path, entry->source_path, true);
          } break;
          }
        }
      } catch (const std::exception &ex) {
        utils::error::raise_error(function_name, ex, "query error");
      }
    }

    if (should_wait) {
      upload_notify_.wait_for(upload_lock, 5s);
    }

    upload_notify_.notify_all();
  }
}
} // namespace repertory
