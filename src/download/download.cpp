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
#include "download/download.hpp"
#include "download/events.hpp"
#include "download/utils.hpp"
#include "drives/i_open_file_table.hpp"
#include "events/event_system.hpp"
#include "providers/i_provider.hpp"
#include "utils/file_utils.hpp"
#include "utils/global_data.hpp"
#include "utils/path_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
download::download(const app_config &config, filesystem_item &fsi,
                   const api_reader_callback &api_reader, const std::size_t &chunk_size,
                   i_open_file_table &oft)
    : config_(config),
      fsi_(fsi),
      api_reader_(api_reader),
      oft_(oft),
      chunk_size_(chunk_size),
      read_chunk_state_(static_cast<std::size_t>(
          utils::divide_with_ceiling(fsi.size, static_cast<std::uint64_t>(chunk_size)))),
      last_chunk_size_(static_cast<std::size_t>(fsi.size <= chunk_size  ? fsi.size
                                                : fsi.size % chunk_size ? fsi.size % chunk_size
                                                                        : chunk_size)),
      write_chunk_state_(static_cast<std::size_t>(
          utils::divide_with_ceiling(fsi.size, static_cast<std::uint64_t>(chunk_size)))) {
  /* std::cout << "size:" << fsi.size << ":last:" << last_chunk_size_ << std::endl; */
  initialize_download(fsi, true);
}

download::download(const app_config &config, filesystem_item &fsi,
                   const api_reader_callback &api_reader, const std::size_t &chunk_size,
                   std::size_t &last_chunk_size, boost::dynamic_bitset<> &read_state,
                   boost::dynamic_bitset<> &write_state, i_open_file_table &oft)
    : config_(config),
      fsi_(fsi),
      api_reader_(api_reader),
      oft_(oft),
      chunk_size_(chunk_size),
      read_chunk_state_(read_state),
      last_chunk_size_(last_chunk_size),
      write_chunk_state_(write_state) {
  initialize_download(fsi, false);
}

download::~download() {
  if (not get_complete()) {
    notify_stop_requested();
  }

  if (io_thread_) {
    io_thread_->join();
  }

  if (auto_close_) {
    native_file::attach(fsi_.handle)->close();
    fsi_.handle = REPERTORY_INVALID_HANDLE;
  }
}

api_error download::allocate(const std::uint64_t &, const std::uint64_t &size,
                             const allocator_callback &allocator,
                             const completer_callback &completer) {
  reset_timeout(false);
  /* std::cout << "allocate called" << std::endl; */

  if (size == fsi_.size) {
    return api_error::success;
  }

  if (size > fsi_.size) {
    download_chunk(read_chunk_state_.size() - 1u, false);
  } else {
    download_chunk(static_cast<std::size_t>(
                       utils::divide_with_ceiling(size, static_cast<std::uint64_t>(chunk_size_))),
                   false);
  }

  if (error_ == api_error::success) {
    const auto original_size = fsi_.size;
    unique_mutex_lock read_write_lock(read_write_mutex_);
    const auto allocate_local_file = [&]() -> api_error {
      if ((error_ = allocator()) == api_error::success) {
        utils::file::get_file_size(fsi_.source_path, fsi_.size);
        completer(original_size, fsi_.size, true);
        oft_.force_schedule_upload(fsi_);
      }
      read_write_notify_.notify_all();
      read_write_lock.unlock();

      return error_;
    };

    if (processed_) {
      return allocate_local_file();
    }

    read_write_notify_.notify_all();
    read_write_lock.unlock();

    pause();

    read_write_lock.lock();
    if (processed_) {
      /* std::cout << "recursive allocate" << std::endl; */
      return allocate_local_file();
    }

    if ((error_ = allocator()) == api_error::success) {
      utils::file::get_file_size(fsi_.source_path, fsi_.size);

      const auto end_chunk_index = static_cast<std::size_t>(
          utils::divide_with_ceiling(fsi_.size, static_cast<std::uint64_t>(chunk_size_)));
      if (end_chunk_index >= read_chunk_state_.size()) {
        write_chunk_state_.resize(end_chunk_index + 1u);
        read_chunk_state_.resize(end_chunk_index + 1u);
      }

      last_chunk_size_ =
          static_cast<std::size_t>(fsi_.size <= chunk_size_  ? fsi_.size
                                   : fsi_.size % chunk_size_ ? fsi_.size % chunk_size_
                                                             : chunk_size_);

      read_chunk_state_[end_chunk_index] = write_chunk_state_[end_chunk_index] = true;
      completer(original_size, fsi_.size, true);
    }

    read_write_notify_.notify_all();
    read_write_lock.unlock();

    resume();
  }

  return error_;
}

void download::create_active_chunk(std::size_t chunk_index) {
  auto reader = [this, chunk_index]() {
    const auto chunk_read_size =
        (chunk_index == (read_chunk_state_.size() - 1u)) ? last_chunk_size_ : chunk_size_;
    const auto chunk_read_offset = chunk_index * chunk_size_;

    api_error ret;
    if (not get_complete()) {
      std::vector<char> buffer;
      if ((ret = api_reader_(fsi_.api_path, chunk_read_size, chunk_read_offset + read_offset_,
                             buffer, stop_requested_)) == api_error::success) {
        unique_mutex_lock read_write_lock(read_write_mutex_);
        if (not get_complete()) {
          write_queue_.emplace_back(
              std::make_shared<write_data>(chunk_index, std::move(buffer), chunk_read_offset));
        }
        read_write_notify_.notify_all();
        read_write_lock.unlock();
      }

      if (ret != api_error::success) {
        error_ = ((error_ == api_error::success) ? ret : error_);

        mutex_lock read_write_lock(read_write_mutex_);
        read_write_notify_.notify_all();
      }
    }
  };
  active_chunks_.insert({chunk_index, std::make_shared<active_chunk>(std::thread(reader))});
}

void download::download_chunk(std::size_t chunk_index, bool inactive_only) {
  unique_mutex_lock read_write_lock(read_write_mutex_);

  const auto should_download = [&]() -> bool {
    return (chunk_index < read_chunk_state_.size()) && not read_chunk_state_[chunk_index] &&
           not get_complete();
  };

  if (should_download()) {
    while (paused_ && not get_complete()) {
      read_write_notify_.notify_all();
      read_write_notify_.wait(read_write_lock);
    }

    if (should_download()) {
      auto created = false;
      if (active_chunks_.find(chunk_index) == active_chunks_.end()) {
        create_active_chunk(chunk_index);
        created = true;
      }

      if (not inactive_only || created) {
        auto chunk = active_chunks_[chunk_index];
        read_write_notify_.notify_all();
        read_write_lock.unlock();

        if (not inactive_only) {
          reset_timeout(false);
        }

        utils::spin_wait_for_mutex(
            [this, chunk_index]() -> bool {
              return read_chunk_state_[chunk_index] || get_complete();
            },
            chunk->notify, chunk->mutex, "download_chunk");
      }
    }
  }
}

api_error download::download_all() {
  const auto total_chunks = read_chunk_state_.size();
  for (std::size_t chunk = 0u; not get_complete() && (chunk < total_chunks); chunk++) {
    download_chunk(chunk, false);
  }
  utils::spin_wait_for_mutex(processed_, processed_notify_, read_write_mutex_, "download_all");
  return error_;
}

bool download::get_complete() const {
  return read_chunk_state_.all() || (error_ != api_error::success) || stop_requested_ || processed_;
}

bool download::get_timeout_enabled() const {
  return not paused_ && not write_chunk_state_.any() &&
         config_.get_enable_chunk_download_timeout() && (oft_.get_open_count(fsi_.api_path) == 0u);
}

void download::get_state_information(filesystem_item &fsi, std::size_t &chunk_size,
                                     std::size_t &last_chunk_size,
                                     boost::dynamic_bitset<> &read_state,
                                     boost::dynamic_bitset<> &write_state) {
  fsi = fsi_;
  chunk_size = chunk_size_;
  last_chunk_size = last_chunk_size_;
  read_state = read_chunk_state_;
  write_state = write_chunk_state_;
}

void download::handle_active_chunk_complete(std::size_t chunk_index, unique_mutex_lock &lock) {
  lock.lock();
  auto activeChunk = active_chunks_[chunk_index];
  active_chunks_.erase(chunk_index);
  lock.unlock();

  unique_mutex_lock completed_lock(activeChunk->mutex);
  activeChunk->notify.notify_all();
  completed_lock.unlock();

  activeChunk->worker.join();
}

void download::initialize_download(filesystem_item &fsi, const bool &delete_existing) {
  const auto current_source_path = fsi.source_path;

  if (delete_existing) {
    // Handle should always be invalid (just in case it's not - no leaking).
    unique_recur_mutex_lock item_lock(*fsi.lock);
    if (fsi.handle != REPERTORY_INVALID_HANDLE) {
      native_file::attach(fsi.handle)->close();
      fsi.handle = REPERTORY_INVALID_HANDLE;
    }
    item_lock.unlock();

    // Update current open file information to non-existing source file and free open handle.
    fsi.source_path =
        utils::path::combine(config_.get_cache_directory(), {utils::create_uuid_string()});
    fsi.source_path_changed = true;

    // Update cache space if existing file size is >0 and delete
    std::uint64_t file_size = 0u;
    utils::file::get_file_size(current_source_path, file_size);
    if (file_size) {
      global_data::instance().update_used_space(file_size, 0u, true);
    }

    utils::file::delete_file(current_source_path);

    // Update completed file information and create new lock mutex
    fsi_.source_path =
        utils::path::combine(config_.get_cache_directory(), {utils::create_uuid_string()});
    fsi_.lock = std::make_shared<std::recursive_mutex>();
  }
  // For resume-only, open file to report correct file size in fuse/winfsp operations
  else if ((error_ = oft_.open(fsi_, open_file_handle_)) != api_error::success) {
    event_system::instance().raise<download_restore_failed>(
        fsi_.api_path, fsi_.source_path, "open failed: " + api_error_to_string(error_));
  }

  if (error_ == api_error::success) {
    event_system::instance().raise<download_begin>(fsi_.api_path, fsi_.source_path);
    // Create new file for reading and writing
    if ((error_ = native_file::create_or_open(fsi_.source_path, read_write_file_)) ==
        api_error::success) {
      // Pre-allocate full file and update used cache space
      if (read_write_file_->allocate(fsi_.size)) {
        if (not delete_existing) {
          // Update used cache space
          global_data::instance().update_used_space(0u, fsi_.size, true);
        }

        // Launch read-ahead workers
        for (std::uint8_t i = 0u; i < config_.get_read_ahead_count(); i++) {
          background_workers_.emplace_back(std::thread([this]() { read_ahead_worker(); }));
        }

        // Launch read-behind worker
        background_workers_.emplace_back(std::thread([this]() { read_behind_worker(); }));

        // Launch read-end worker
        if (fsi.size > (chunk_size_ * config_.get_read_ahead_count())) {
          background_workers_.emplace_back(std::thread([this]() { read_end_worker(); }));
        }

        // Launch read/write IO worker
        io_thread_ = std::make_unique<std::thread>([this]() { io_data_worker(); });
      } else if (delete_existing) {
        const auto temp_error_code = utils::get_last_error_code();
        utils::file::delete_file(fsi_.source_path);
        utils::set_last_error_code(temp_error_code);
        error_ = api_error::os_error;
      }
    }

    if (error_ != api_error::success) {
      event_system::instance().raise<download_end>(fsi_.api_path, fsi_.source_path, 0u, error_);
    }
  }
}

void download::io_data_worker() {
  unique_mutex_lock read_write_lock(read_write_mutex_);
  read_write_lock.unlock();

  do {
    process_timeout(read_write_lock);
    process_read_queue(read_write_lock);
    process_write_queue(read_write_lock);
    notify_progress();
    process_download_complete(read_write_lock);
    wait_for_io(read_write_lock);
  } while (not processed_);

  shutdown(read_write_lock);
}

void download::notify_progress() {
  if (read_chunk_state_.any()) {
    utils::download::notify_progress<download_progress>(
        config_, fsi_.api_path, fsi_.source_path, static_cast<double>(read_chunk_state_.count()),
        static_cast<double>(read_chunk_state_.size()), progress_);
  }
}

void download::notify_stop_requested() {
  if (not stop_requested_) {
    unique_mutex_lock read_write_lock(read_write_mutex_);
    error_ =
        write_chunk_state_.any() ? api_error::download_incomplete : api_error::download_stopped;
    stop_requested_ = true;
    paused_ = false;
    read_write_notify_.notify_all();
    read_write_lock.unlock();
  }
}

bool download::pause() {
  unique_mutex_lock read_write_lock(read_write_mutex_);
  if (not paused_) {
    paused_ = true;

    while (not active_chunks_.empty() && not stop_requested_) {
      read_write_notify_.notify_all();
      read_write_notify_.wait_for(read_write_lock, 2s);
    }
    read_write_notify_.notify_all();
    read_write_lock.unlock();

    if (stop_requested_) {
      paused_ = false;
    } else {
      event_system::instance().raise<download_paused>(fsi_.api_path, fsi_.source_path);
    }

    return paused_;
  }

  return true;
}

void download::process_download_complete(unique_mutex_lock &lock) {
  if (get_complete() && not processed_) {
    lock.lock();
    if (not processed_) {
      if ((error_ == api_error::success) && read_chunk_state_.all()) {
        const auto assign_handle_and_auto_close = [this]() {
          if (fsi_.handle != REPERTORY_INVALID_HANDLE) {
            native_file::attach(fsi_.handle)->close();
            fsi_.handle = REPERTORY_INVALID_HANDLE;
          }
          fsi_.handle = read_write_file_->get_handle();
          auto_close_ = true;
        };

        if (not oft_.perform_locked_operation(
                [this, &assign_handle_and_auto_close](i_open_file_table &oft,
                                                      i_provider &provider) -> bool {
                  filesystem_item *fsi = nullptr;
                  if (oft.get_open_file(fsi_.api_path, fsi)) {
                    unique_recur_mutex_lock item_lock(*fsi->lock);
                    // Handle should always be invalid (just in case it's not - no leaking).
                    if (fsi->handle != REPERTORY_INVALID_HANDLE) {
                      native_file::attach(fsi->handle)->close();
                      fsi->handle = REPERTORY_INVALID_HANDLE;
                    }
                    fsi->handle = read_write_file_->get_handle();
                    item_lock.unlock();

                    fsi->source_path = fsi_.source_path;
                    fsi->source_path_changed = true;
                    fsi->changed = false;

                    fsi_ = *fsi;
                  } else {
                    provider.set_source_path(fsi_.api_path, fsi_.source_path);
                    assign_handle_and_auto_close();
                  }

                  return true;
                })) {
          assign_handle_and_auto_close();
        }
      } else if (error_ == api_error::success) {
        error_ = api_error::download_failed;
      }

      stop_requested_ = processed_ = true;
      read_write_notify_.notify_all();
      lock.unlock();

      process_write_queue(lock);
      process_read_queue(lock);

      lock.lock();

      if (error_ != api_error::success) {
        read_write_file_->close();
        if (error_ != api_error::download_incomplete) {
          if (utils::file::delete_file(fsi_.source_path)) {
            global_data::instance().update_used_space(fsi_.size, 0u, true);
          }
        }
      }

      processed_notify_.notify_all();
      read_write_notify_.notify_all();
      lock.unlock();

      if (write_chunk_state_.any() && (error_ == api_error::success)) {
        oft_.force_schedule_upload(fsi_);
      }

      if (open_file_handle_ != static_cast<std::uint64_t>(REPERTORY_API_INVALID_HANDLE)) {
        oft_.close(open_file_handle_);
        open_file_handle_ = REPERTORY_API_INVALID_HANDLE;
      }

      if (not disable_download_end_) {
        event_system::instance().raise<download_end>(fsi_.api_path, fsi_.source_path, 0u, error_);
      }
    } else {
      lock.unlock();
    }
  }
}

void download::process_read_queue(unique_mutex_lock &lock) {
  lock.lock();
  while (not read_queue_.empty()) {
    auto rd = read_queue_.front();
    read_queue_.pop_front();
    lock.unlock();

    if (error_ == api_error::success) {
      std::size_t bytes_read = 0u;
      if (not read_write_file_->read_bytes(&rd->data[0u], rd->data.size(), rd->offset,
                                           bytes_read)) {
        error_ = ((error_ == api_error::success) ? api_error::os_error : error_);
      }
    }

    rd->complete = true;
    unique_mutex_lock read_lock(rd->mutex);
    rd->notify.notify_all();
    read_lock.unlock();

    lock.lock();
  }
  lock.unlock();
}

void download::process_timeout(unique_mutex_lock &lock) {
  if ((std::chrono::system_clock::now() >= timeout_) && not get_complete() &&
      get_timeout_enabled()) {
    error_ = api_error::download_timeout;
    stop_requested_ = true;
    lock.lock();
    read_write_notify_.notify_all();
    lock.unlock();
    event_system::instance().raise<download_timeout>(fsi_.api_path, fsi_.source_path);
  }
}

void download::process_write_queue(unique_mutex_lock &lock) {
  lock.lock();
  do {
    if (not write_queue_.empty()) {
      auto wd = write_queue_.front();
      write_queue_.pop_front();
      lock.unlock();

      if (not get_complete() || (error_ == api_error::download_incomplete)) {
        auto &bytes_written = wd->written;
        if (read_write_file_->write_bytes(&wd->data[0u], wd->data.size(), wd->offset,
                                          bytes_written)) {
          read_write_file_->flush();
          if (wd->from_read) {
            lock.lock();
            read_chunk_state_[wd->chunk_index] = true;
            lock.unlock();
          } else {
            const auto start_chunk_index = static_cast<std::size_t>(wd->offset / chunk_size_);
            const auto end_chunk_index =
                static_cast<std::size_t>((wd->data.size() + wd->offset) / chunk_size_);
            lock.lock();
            for (std::size_t chunk_index = start_chunk_index; (chunk_index <= end_chunk_index);
                 chunk_index++) {
              write_chunk_state_[chunk_index] = true;
            }
            lock.unlock();
          }
        } else {
          error_ = ((error_ == api_error::success) ? api_error::os_error : error_);
        }
      }

      if (wd->from_read) {
        handle_active_chunk_complete(wd->chunk_index, lock);
      } else {
        wd->complete = true;
        unique_mutex_lock write_lock(wd->mutex);
        wd->notify.notify_all();
        write_lock.unlock();
      }

      lock.lock();
    }
  } while (not write_queue_.empty() && read_queue_.empty());
  lock.unlock();
}

void download::read_ahead_worker() {
  const auto total_chunks = read_chunk_state_.size();

  auto first_chunk_index = current_chunk_index_;
  const auto get_next_chunk =
      [&first_chunk_index](const std::size_t &current_chunk_index,
                           const std::size_t &last_chunk_index) -> std::size_t {
    if (current_chunk_index == first_chunk_index) {
      return last_chunk_index + 1ul;
    }

    first_chunk_index = current_chunk_index;
    return current_chunk_index + 1ul;
  };

  while (not get_complete()) {
    for (auto chunk_index = get_next_chunk(current_chunk_index_, 0u);
         (chunk_index < total_chunks) && not get_complete();) {
      download_chunk(chunk_index, true);
      chunk_index = get_next_chunk(current_chunk_index_, chunk_index);
    }

    utils::spin_wait_for_mutex(
        [this, &first_chunk_index]() -> bool {
          return get_complete() || (first_chunk_index != current_chunk_index_);
        },
        read_write_notify_, read_write_mutex_, "read_ahead_worker");
  }
}

void download::read_behind_worker() {
  auto first_chunk_index = current_chunk_index_;
  const auto get_next_chunk =
      [&first_chunk_index](const std::size_t &current_chunk_index,
                           const std::size_t &last_chunk_index) -> std::size_t {
    if (current_chunk_index == first_chunk_index) {
      return last_chunk_index ? last_chunk_index - 1ul : 0u;
    }

    first_chunk_index = current_chunk_index;
    return current_chunk_index ? current_chunk_index - 1ul : 0u;
  };

  while (not get_complete()) {
    for (auto chunk_index = get_next_chunk(current_chunk_index_, current_chunk_index_);
         not get_complete();) {
      download_chunk(chunk_index, true);
      if (not chunk_index) {
        break;
      }
      chunk_index = get_next_chunk(current_chunk_index_, chunk_index);
    }

    utils::spin_wait_for_mutex(
        [this, &first_chunk_index]() -> bool {
          return get_complete() || (first_chunk_index != current_chunk_index_);
        },
        read_write_notify_, read_write_mutex_, "read_behind_worker");
  }
}

api_error download::read_bytes(const std::uint64_t &, std::size_t read_size,
                               const std::uint64_t &read_offset, std::vector<char> &data) {
  /* std::cout << "read called:" << read_size << ':' << read_offset << std::endl; */
  data.clear();
  if ((read_size > 0u) && (error_ == api_error::success)) {
    unique_mutex_lock read_write_lock(read_write_mutex_);
    const auto read_local_file = [&]() -> api_error {
      read_write_notify_.notify_all();
      read_write_lock.unlock();

      error_ = (error_ == api_error::success)
                   ? utils::file::read_from_source(fsi_, read_size, read_offset, data)
                   : error_;

      return error_;
    };

    if (processed_) {
      return read_local_file();
    }

    read_write_notify_.notify_all();
    read_write_lock.unlock();

    const auto start_chunk_index = current_chunk_index_ =
        static_cast<std::size_t>(read_offset / chunk_size_);
    const auto end_chunk_index = static_cast<std::size_t>((read_size + read_offset) / chunk_size_);
    for (std::size_t chunk = start_chunk_index; not get_complete() && (chunk <= end_chunk_index);
         chunk++) {
      download_chunk(chunk, false);
    }

    if (error_ == api_error::success) {
      data.resize(read_size);
      auto rd = std::make_shared<read_data>(data, read_offset);

      read_write_lock.lock();
      if (processed_) {
        /* std::cout << "recursive read:" << read_size << ':' << read_offset << std::endl; */
        return read_local_file();
      }

      read_queue_.emplace_front(rd);
      read_write_notify_.notify_all();
      read_write_lock.unlock();

      reset_timeout(false);
      utils::spin_wait_for_mutex(rd->complete, rd->notify, rd->mutex, "read_bytes");

      reset_timeout(false);
    }
  }

  return error_;
}

void download::read_end_worker() {
  const auto total_chunks = read_chunk_state_.size();
  auto to_read = utils::divide_with_ceiling(std::size_t(262144u), chunk_size_);
  for (auto chunk_index = total_chunks ? total_chunks - 1ul : 0u;
       chunk_index && to_read && not get_complete(); chunk_index--, to_read--) {
    download_chunk(chunk_index, true);
  }
}

void download::reset_timeout(const bool &) {
  mutex_lock read_write_lock(read_write_mutex_);
  timeout_ = std::chrono::system_clock::now() +
             std::chrono::seconds(config_.get_chunk_downloader_timeout_secs());
  read_write_notify_.notify_all();
}

void download::resume() {
  reset_timeout(false);

  mutex_lock read_write_lock(read_write_mutex_);
  if (paused_) {
    paused_ = false;
    event_system::instance().raise<download_resumed>(fsi_.api_path, fsi_.source_path);
  }
  read_write_notify_.notify_all();
}

void download::set_api_path(const std::string &api_path) {
  mutex_lock read_write_lock(read_write_mutex_);
  fsi_.api_path = api_path;
  fsi_.api_parent = utils::path::get_parent_api_path(api_path);
  read_write_notify_.notify_all();
}

void download::shutdown(unique_mutex_lock &lock) {
  lock.lock();
  while (not active_chunks_.empty()) {
    const auto chunk_index = active_chunks_.begin()->first;
    lock.unlock();
    handle_active_chunk_complete(chunk_index, lock);
    lock.lock();
  }
  lock.unlock();

  for (auto &worker : background_workers_) {
    worker.join();
  }
  background_workers_.clear();
}

void download::wait_for_io(unique_mutex_lock &lock) {
  lock.lock();
  if (not get_complete() && read_queue_.empty() && write_queue_.empty()) {
    if (get_timeout_enabled()) {
      read_write_notify_.wait_until(lock, timeout_);
    } else {
      read_write_notify_.wait(lock);
    }
  }
  lock.unlock();
}

api_error download::write_bytes(const std::uint64_t &, const std::uint64_t &write_offset,
                                std::vector<char> data, std::size_t &bytes_written,
                                const completer_callback &completer) {
  /* std::cout << "write called:" << write_offset << std::endl; */
  bytes_written = 0u;

  if (not data.empty() && (error_ == api_error::success)) {
    const auto start_chunk_index = current_chunk_index_ =
        static_cast<std::size_t>(write_offset / chunk_size_);
    const auto end_chunk_index =
        static_cast<std::size_t>((data.size() + write_offset) / chunk_size_);
    for (std::size_t chunk_index = start_chunk_index;
         not get_complete() &&
         (chunk_index <= std::min(read_chunk_state_.size() - 1u, end_chunk_index));
         chunk_index++) {
      download_chunk(chunk_index, false);
    }

    if (error_ == api_error::success) {
      const auto original_size = fsi_.size;

      unique_mutex_lock read_write_lock(read_write_mutex_);
      const auto write_local_file = [&]() -> api_error {
        error_ = (error_ == api_error::success)
                     ? utils::file::write_to_source(fsi_, write_offset, data, bytes_written)
                     : error_;

        if (error_ == api_error::success) {
          utils::file::get_file_size(fsi_.source_path, fsi_.size);
          completer(original_size, fsi_.size, true);
          oft_.force_schedule_upload(fsi_);
        }

        read_write_notify_.notify_all();
        read_write_lock.unlock();

        return error_;
      };

      if (processed_) {
        return write_local_file();
      }

      read_write_notify_.notify_all();
      read_write_lock.unlock();

      const auto size = (write_offset + data.size());
      if (size > fsi_.size) {
        pause();
        read_write_lock.lock();

        if (end_chunk_index >= write_chunk_state_.size()) {
          const auto first_new_chunk_index = write_chunk_state_.size();
          write_chunk_state_.resize(end_chunk_index + 1u);
          read_chunk_state_.resize(end_chunk_index + 1u);
          for (std::size_t chunk_index = first_new_chunk_index; chunk_index <= end_chunk_index;
               chunk_index++) {
            read_chunk_state_[chunk_index] = true;
          }
        }

        fsi_.size = size;
        last_chunk_size_ =
            static_cast<std::size_t>(fsi_.size <= chunk_size_  ? fsi_.size
                                     : fsi_.size % chunk_size_ ? fsi_.size % chunk_size_
                                                               : chunk_size_);
        read_write_lock.unlock();
        resume();
      }

      auto wd = std::make_shared<write_data>(write_offset, std::move(data));

      read_write_lock.lock();
      if (processed_) {
        /* std::cout << "recursive write:" << write_offset << std::endl; */
        return write_local_file();
      }

      write_queue_.emplace_back(wd);
      read_write_notify_.notify_all();
      read_write_lock.unlock();

      reset_timeout(false);

      utils::spin_wait_for_mutex(wd->complete, wd->notify, wd->mutex, "write_bytes");

      reset_timeout(false);

      bytes_written = wd->written;
      if (bytes_written && (size > fsi_.size)) {
        fsi_.size = size;
        completer(original_size, fsi_.size, false);
      }
    }
  }

  return error_;
}
} // namespace repertory
