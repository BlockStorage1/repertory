/*
  Copyright <2018-2025> <scott.e.graves@protonmail.com>

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
#include "file_manager/open_file_base.hpp"

#include "events/event_system.hpp"
#include "events/types/filesystem_item_closed.hpp"
#include "events/types/filesystem_item_handle_closed.hpp"
#include "events/types/filesystem_item_handle_opened.hpp"
#include "events/types/filesystem_item_opened.hpp"
#include "providers/i_provider.hpp"
#include "utils/path.hpp"

namespace repertory {
void open_file_base::download::notify(api_error err) {
  complete_ = true;
  error_ = err;
  unique_mutex_lock lock(mtx_);
  notify_.notify_all();
}

auto open_file_base::download::wait() -> api_error {
  if (complete_) {
    return error_;
  }

  unique_mutex_lock lock(mtx_);
  if (not complete_) {
    notify_.wait(lock);
  }
  notify_.notify_all();

  return error_;
}

void open_file_base::io_item::action() {
  result_ = action_();

  mutex_lock lock(mtx_);
  notify_.notify_all();
}

auto open_file_base::io_item::get_result() -> api_error {
  unique_mutex_lock lock(mtx_);
  if (result_.has_value()) {
    return result_.value();
  }

  notify_.wait(lock);
  return result_.value_or(api_error::error);
}

open_file_base::open_file_base(std::uint64_t chunk_size,
                               std::uint8_t chunk_timeout, filesystem_item fsi,
                               i_provider &provider, bool disable_io)
    : open_file_base(chunk_size, chunk_timeout, fsi, {}, provider, disable_io) {
}

open_file_base::open_file_base(
    std::uint64_t chunk_size, std::uint8_t chunk_timeout, filesystem_item fsi,
    std::map<std::uint64_t, open_file_data> open_data, i_provider &provider,
    bool disable_io)
    : chunk_size_(chunk_size),
      chunk_timeout_(chunk_timeout),
      fsi_(std::move(fsi)),
      last_chunk_size_(static_cast<std::size_t>(
          fsi.size <= chunk_size          ? fsi.size
          : (fsi.size % chunk_size) == 0U ? chunk_size
                                          : fsi.size % chunk_size)),
      open_data_(std::move(open_data)),
      provider_(provider) {
  if (not fsi.directory && not disable_io) {
    io_thread_ = std::make_unique<std::thread>([this] { file_io_thread(); });
  }
}

void open_file_base::add(std::uint64_t handle, open_file_data ofd) {
  REPERTORY_USES_FUNCTION_NAME();

  recur_mutex_lock file_lock(file_mtx_);
  open_data_[handle] = ofd;
  if (open_data_.size() == 1U) {
    event_system::instance().raise<filesystem_item_opened>(
        fsi_.api_path, fsi_.directory, function_name, fsi_.source_path);
  }

  event_system::instance().raise<filesystem_item_handle_opened>(
      fsi_.api_path, fsi_.directory, function_name, handle, fsi_.source_path);
}

auto open_file_base::can_close() const -> bool {
  recur_mutex_lock file_lock(file_mtx_);
  if (fsi_.directory) {
    return true;
  }

  if (not open_data_.empty()) {
    return false;
  }

  if (modified_) {
    return false;
  }

  if (get_api_error() != api_error::success) {
    return true;
  }

  if (is_complete()) {
    return true;
  }

  if (provider_.is_read_only()) {
    return true;
  }

  std::chrono::system_clock::time_point last_access{last_access_};
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now() - last_access);
  return (duration.count() >= chunk_timeout_);
}

auto open_file_base::close() -> bool {
  unique_mutex_lock io_lock(io_thread_mtx_);
  if (io_stop_requested_ || not io_thread_) {
    io_thread_notify_.notify_all();
    io_lock.unlock();
    return false;
  }

  io_stop_requested_ = true;
  io_thread_notify_.notify_all();
  io_lock.unlock();

  io_thread_->join();
  io_thread_.reset();

  return true;
}

auto open_file_base::do_io(std::function<api_error()> action) -> api_error {
  unique_mutex_lock io_lock(io_thread_mtx_);
  auto item = std::make_shared<io_item>(action);
  io_thread_queue_.emplace_back(item);
  io_thread_notify_.notify_all();
  io_lock.unlock();

  return item->get_result();
}

void open_file_base::file_io_thread() {
  unique_mutex_lock io_lock(io_thread_mtx_);
  io_thread_notify_.notify_all();
  io_lock.unlock();

  const auto process_queue = [&]() {
    io_lock.lock();
    if (not io_stop_requested_ && io_thread_queue_.empty()) {
      io_thread_notify_.wait(io_lock);
    }

    while (not io_thread_queue_.empty()) {
      auto *item = io_thread_queue_.front().get();
      io_thread_notify_.notify_all();
      io_lock.unlock();

      item->action();

      io_lock.lock();
      io_thread_queue_.pop_front();
    }

    io_thread_notify_.notify_all();
    io_lock.unlock();
  };

  while (not io_stop_requested_) {
    process_queue();
  }

  process_queue();
}

auto open_file_base::get_api_error() const -> api_error {
  mutex_lock error_lock(error_mtx_);
  return error_;
}

auto open_file_base::get_api_path() const -> std::string {
  recur_mutex_lock file_lock(file_mtx_);
  return fsi_.api_path;
}

auto open_file_base::get_file_size() const -> std::uint64_t {
  recur_mutex_lock file_lock(file_mtx_);
  return fsi_.size;
}

[[nodiscard]] auto open_file_base::get_last_chunk_size() const -> std::size_t {
  recur_mutex_lock file_lock(file_mtx_);
  return last_chunk_size_;
}

void open_file_base::set_file_size(std::uint64_t size) {
  recur_mutex_lock file_lock(file_mtx_);
  fsi_.size = size;
}

void open_file_base::set_last_chunk_size(std::size_t size) {
  recur_mutex_lock file_lock(file_mtx_);
  last_chunk_size_ = size;
}

void open_file_base::set_modified(bool modified) {
  recur_mutex_lock file_lock(file_mtx_);
  modified_ = modified;
}

void open_file_base::set_removed(bool removed) {
  recur_mutex_lock file_lock(file_mtx_);
  removed_ = removed;
}

void open_file_base::set_source_path(std::string source_path) {
  recur_mutex_lock file_lock(file_mtx_);
  fsi_.source_path = std::move(source_path);
}

auto open_file_base::get_filesystem_item() const -> filesystem_item {
  recur_mutex_lock file_lock(file_mtx_);
  return fsi_;
}

auto open_file_base::get_handles() const -> std::vector<std::uint64_t> {
  recur_mutex_lock file_lock(file_mtx_);

  std::vector<std::uint64_t> ret;
  for (const auto &item : open_data_) {
    ret.emplace_back(item.first);
  }

  return ret;
}

auto open_file_base::get_open_data()
    -> std::map<std::uint64_t, open_file_data> & {
  recur_mutex_lock file_lock(file_mtx_);
  return open_data_;
}

auto open_file_base::get_open_data() const
    -> const std::map<std::uint64_t, open_file_data> & {
  recur_mutex_lock file_lock(file_mtx_);
  return open_data_;
}

auto open_file_base::get_open_data(std::uint64_t handle) -> open_file_data & {
  recur_mutex_lock file_lock(file_mtx_);
  return open_data_.at(handle);
}

auto open_file_base::get_open_data(std::uint64_t handle) const
    -> const open_file_data & {
  recur_mutex_lock file_lock(file_mtx_);
  return open_data_.at(handle);
}

auto open_file_base::get_open_file_count() const -> std::size_t {
  recur_mutex_lock file_lock(file_mtx_);
  return open_data_.size();
}

auto open_file_base::get_source_path() const -> std::string {
  recur_mutex_lock file_lock(file_mtx_);
  return fsi_.source_path;
}

auto open_file_base::has_handle(std::uint64_t handle) const -> bool {
  recur_mutex_lock file_lock(file_mtx_);
  return open_data_.find(handle) != open_data_.end();
}

auto open_file_base::is_modified() const -> bool {
  recur_mutex_lock file_lock(file_mtx_);
  return modified_;
}

auto open_file_base::is_removed() const -> bool {
  recur_mutex_lock file_lock(file_mtx_);
  return removed_;
}

void open_file_base::notify_io() {
  mutex_lock io_lock(io_thread_mtx_);
  io_thread_notify_.notify_all();
}

void open_file_base::remove(std::uint64_t handle) {
  REPERTORY_USES_FUNCTION_NAME();

  recur_mutex_lock file_lock(file_mtx_);
  if (open_data_.find(handle) == open_data_.end()) {
    return;
  }

  open_data_.erase(handle);
  event_system::instance().raise<filesystem_item_handle_closed>(
      fsi_.api_path, modified_, fsi_.directory, function_name, handle,
      fsi_.source_path);
  if (not open_data_.empty()) {
    return;
  }

  event_system::instance().raise<filesystem_item_closed>(
      fsi_.api_path, modified_, fsi_.directory, function_name,
      fsi_.source_path);
}

void open_file_base::remove_all() {
  REPERTORY_USES_FUNCTION_NAME();

  recur_mutex_lock file_lock(file_mtx_);
  if (open_data_.empty()) {
    return;
  }

  auto open_data = open_data_;
  open_data_.clear();

  for (const auto &data : open_data) {
    event_system::instance().raise<filesystem_item_handle_closed>(
        fsi_.api_path, modified_, fsi_.directory, function_name, data.first,
        fsi_.source_path);
  }

  event_system::instance().raise<filesystem_item_closed>(
      fsi_.api_path, modified_, fsi_.directory, function_name,
      fsi_.source_path);
}

void open_file_base::reset_timeout() {
  last_access_ = std::chrono::system_clock::now();
}

auto open_file_base::set_api_error(api_error err) -> api_error {
  mutex_lock error_lock(error_mtx_);
  if (error_ == err) {
    return error_;
  }

  return ((error_ = (error_ == api_error::success ||
                             error_ == api_error::download_incomplete ||
                             error_ == api_error::download_stopped
                         ? err
                         : error_)));
}

void open_file_base::set_api_path(const std::string &api_path) {
  recur_mutex_lock file_lock(file_mtx_);
  fsi_.api_path = api_path;
  fsi_.api_parent = utils::path::get_parent_api_path(api_path);
}

void open_file_base::wait_for_io(stop_type_callback stop_requested_cb) {
  unique_mutex_lock io_lock(io_thread_mtx_);
  if (not stop_requested_cb() && io_thread_queue_.empty()) {
    io_thread_notify_.wait(io_lock);
  }
  io_thread_notify_.notify_all();
  io_lock.unlock();
}
} // namespace repertory
