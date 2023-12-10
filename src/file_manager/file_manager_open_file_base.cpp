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

#include "utils/path_utils.hpp"

namespace repertory {
file_manager::open_file_base::open_file_base(std::uint64_t chunk_size,
                                             std::uint8_t chunk_timeout,
                                             filesystem_item fsi,
                                             i_provider &provider)
    : open_file_base(chunk_size, chunk_timeout, fsi, {}, provider) {}

file_manager::open_file_base::open_file_base(
    std::uint64_t chunk_size, std::uint8_t chunk_timeout, filesystem_item fsi,
    std::map<std::uint64_t, open_file_data> open_data, i_provider &provider)
    : chunk_size_(chunk_size),
      chunk_timeout_(chunk_timeout),
      fsi_(std::move(fsi)),
      last_chunk_size_(static_cast<std::size_t>(
          fsi.size <= chunk_size          ? fsi.size
          : (fsi.size % chunk_size) == 0U ? chunk_size
                                          : fsi.size % chunk_size)),
      open_data_(std::move(open_data)),
      provider_(provider) {
  if (not fsi.directory) {
    io_thread_ = std::make_unique<std::thread>([this] { file_io_thread(); });
  }
}

void file_manager::open_file_base::add(std::uint64_t handle,
                                       open_file_data ofd) {
  recur_mutex_lock file_lock(file_mtx_);
  open_data_[handle] = ofd;
  if (open_data_.size() == 1U) {
    event_system::instance().raise<filesystem_item_opened>(
        fsi_.api_path, fsi_.source_path, fsi_.directory);
  }

  event_system::instance().raise<filesystem_item_handle_opened>(
      fsi_.api_path, handle, fsi_.source_path, fsi_.directory);
}

auto file_manager::open_file_base::can_close() const -> bool {
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

  if (is_download_complete()) {
    return true;
  }

  const std::chrono::system_clock::time_point last_access = last_access_;
  const auto duration = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now() - last_access);
  return (duration.count() >= chunk_timeout_);
}

auto file_manager::open_file_base::do_io(std::function<api_error()> action)
    -> api_error {
  unique_mutex_lock io_lock(io_thread_mtx_);
  auto item = std::make_shared<io_item>(action);
  io_thread_queue_.emplace_back(item);
  io_thread_notify_.notify_all();
  io_lock.unlock();

  return item->get_result();
}

void file_manager::open_file_base::file_io_thread() {
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

auto file_manager::open_file_base::get_api_error() const -> api_error {
  mutex_lock error_lock(error_mtx_);
  return error_;
}

auto file_manager::open_file_base::get_api_path() const -> std::string {
  recur_mutex_lock file_lock(file_mtx_);
  return fsi_.api_path;
}

auto file_manager::open_file_base::get_file_size() const -> std::uint64_t {
  recur_mutex_lock file_lock(file_mtx_);
  return fsi_.size;
}

auto file_manager::open_file_base::get_filesystem_item() const
    -> filesystem_item {
  recur_mutex_lock file_lock(file_mtx_);
  return fsi_;
}

auto file_manager::open_file_base::get_handles() const
    -> std::vector<std::uint64_t> {
  recur_mutex_lock file_lock(file_mtx_);
  std::vector<std::uint64_t> ret;
  for (const auto &item : open_data_) {
    ret.emplace_back(item.first);
  }

  return ret;
}

auto file_manager::open_file_base::get_open_data() const
    -> std::map<std::uint64_t, open_file_data> {
  recur_mutex_lock file_lock(file_mtx_);
  return open_data_;
}

auto file_manager::open_file_base::get_open_data(std::uint64_t handle) const
    -> open_file_data {
  recur_mutex_lock file_lock(file_mtx_);
  return open_data_.at(handle);
}

auto file_manager::open_file_base::get_open_file_count() const -> std::size_t {
  recur_mutex_lock file_lock(file_mtx_);
  return open_data_.size();
}

auto file_manager::open_file_base::is_modified() const -> bool {
  recur_mutex_lock file_lock(file_mtx_);
  return modified_;
}

void file_manager::open_file_base::remove(std::uint64_t handle) {
  recur_mutex_lock file_lock(file_mtx_);
  open_data_.erase(handle);
  event_system::instance().raise<filesystem_item_handle_closed>(
      fsi_.api_path, handle, fsi_.source_path, fsi_.directory, modified_);
  if (open_data_.empty()) {
    event_system::instance().raise<filesystem_item_closed>(
        fsi_.api_path, fsi_.source_path, fsi_.directory, modified_);
  }
}

void file_manager::open_file_base::reset_timeout() {
  last_access_ = std::chrono::system_clock::now();
}

auto file_manager::open_file_base::set_api_error(const api_error &err)
    -> api_error {
  mutex_lock error_lock(error_mtx_);
  if (error_ != err) {
    return ((error_ = (error_ == api_error::success ||
                               error_ == api_error::download_incomplete ||
                               error_ == api_error::download_stopped
                           ? err
                           : error_)));
  }

  return error_;
}

void file_manager::open_file_base::set_api_path(const std::string &api_path) {
  recur_mutex_lock file_lock(file_mtx_);
  fsi_.api_path = api_path;
  fsi_.api_parent = utils::path::get_parent_api_path(api_path);
}

auto file_manager::open_file_base::close() -> bool {
  unique_mutex_lock io_lock(io_thread_mtx_);
  if (not fsi_.directory && not io_stop_requested_) {
    io_stop_requested_ = true;
    io_thread_notify_.notify_all();
    io_lock.unlock();

    if (io_thread_) {
      io_thread_->join();
      io_thread_.reset();
      return true;
    }

    return false;
  }

  io_thread_notify_.notify_all();
  io_lock.unlock();
  return false;
}
} // namespace repertory
