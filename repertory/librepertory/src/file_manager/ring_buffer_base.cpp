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
#include "file_manager/ring_buffer_base.hpp"

#include "events/event_system.hpp"
#include "file_manager/events.hpp"
#include "file_manager/open_file_base.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "utils/common.hpp"
#include "utils/error_utils.hpp"

namespace repertory {
ring_buffer_base::ring_buffer_base(std::uint64_t chunk_size,
                                   std::uint8_t chunk_timeout,
                                   filesystem_item fsi, i_provider &provider,
                                   std::size_t ring_size, bool disable_io)
    : open_file_base(chunk_size, chunk_timeout, fsi, provider, disable_io),
      read_state_(ring_size),
      total_chunks_(static_cast<std::size_t>(
          utils::divide_with_ceiling(fsi.size, chunk_size))) {
  if (disable_io) {
    if (fsi.size > 0U) {
      read_state_.resize(std::min(total_chunks_, read_state_.size()));

      ring_end_ =
          std::min(total_chunks_ - 1U, ring_begin_ + read_state_.size() - 1U);
      read_state_.set(0U, read_state_.size(), false);
    }
  } else {
    if (ring_size < min_ring_size) {
      throw std::runtime_error("ring size must be greater than or equal to 5");
    }

    ring_end_ = std::min(total_chunks_ - 1U, ring_begin_ + ring_size - 1U);
    read_state_.set(0U, ring_size, false);
  }
}

auto ring_buffer_base::check_start() -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (on_check_start()) {
      return api_error::success;
    }

    event_system::instance().raise<download_begin>(get_api_path(),
                                                   get_source_path());
    reader_thread_ =
        std::make_unique<std::thread>([this]() { reader_thread(); });
    return api_error::success;
  } catch (const std::exception &ex) {
    utils::error::raise_api_path_error(function_name, get_api_path(),
                                       get_source_path(), ex,
                                       "failed to start");
    return api_error::error;
  }
}

auto ring_buffer_base::close() -> bool {
  stop_requested_ = true;

  unique_mutex_lock chunk_lock(chunk_mtx_);
  chunk_notify_.notify_all();
  chunk_lock.unlock();

  auto res = open_file_base::close();

  if (reader_thread_) {
    reader_thread_->join();
    reader_thread_.reset();
  }

  return res;
}

auto ring_buffer_base::download_chunk(std::size_t chunk,
                                      bool skip_active) -> api_error {
  unique_mutex_lock chunk_lock(chunk_mtx_);
  const auto unlock_and_notify = [this, &chunk_lock]() {
    chunk_notify_.notify_all();
    chunk_lock.unlock();
  };

  const auto unlock_and_return =
      [&unlock_and_notify](api_error res) -> api_error {
    unlock_and_notify();
    return res;
  };

  if (chunk < ring_begin_ || chunk > ring_end_) {
    return unlock_and_return(api_error::invalid_ring_buffer_position);
  }

  if (get_active_downloads().find(chunk) != get_active_downloads().end()) {
    if (skip_active) {
      return unlock_and_return(api_error::success);
    }

    auto active_download = get_active_downloads().at(chunk);
    unlock_and_notify();

    return active_download->wait();
  }

  if (read_state_[chunk % read_state_.size()]) {
    return unlock_and_return(api_error::success);
  }

  auto active_download{std::make_shared<download>()};
  get_active_downloads()[chunk] = active_download;

  return use_buffer(chunk, [&](data_buffer &buffer) -> api_error {
    auto data_offset{chunk * get_chunk_size()};
    auto data_size{
        chunk == (total_chunks_ - 1U) ? get_last_chunk_size()
                                      : get_chunk_size(),
    };
    unlock_and_notify();

    auto result{
        get_provider().read_file_bytes(get_api_path(), data_size, data_offset,
                                       buffer, stop_requested_),
    };

    chunk_lock.lock();
    if (chunk < ring_begin_ || chunk > ring_end_) {
      result = api_error::invalid_ring_buffer_position;
    }

    if (result == api_error::success) {
      result = on_chunk_downloaded(chunk, buffer);
      if (result == api_error::success) {
        read_state_[chunk % read_state_.size()] = true;
        auto progress = (static_cast<double>(chunk + 1U) /
                         static_cast<double>(total_chunks_)) *
                        100.0;
        event_system::instance().raise<download_progress>(
            get_api_path(), get_source_path(), progress);
      }
    }

    get_active_downloads().erase(chunk);
    unlock_and_notify();

    active_download->notify(result);
    return result;
  });
}

void ring_buffer_base::forward(std::size_t count) {
  update_position(count, true);
}

auto ring_buffer_base::get_read_state() const -> boost::dynamic_bitset<> {
  recur_mutex_lock file_lock(get_mutex());
  return read_state_;
}

auto ring_buffer_base::get_read_state(std::size_t chunk) const -> bool {
  recur_mutex_lock file_lock(get_mutex());
  return read_state_[chunk % read_state_.size()];
}

auto ring_buffer_base::read(std::size_t read_size, std::uint64_t read_offset,
                            data_buffer &data) -> api_error {
  if (is_directory()) {
    return api_error::invalid_operation;
  }

  reset_timeout();

  read_size =
      utils::calculate_read_size(get_file_size(), read_size, read_offset);
  if (read_size == 0U) {
    return api_error::success;
  }

  auto begin_chunk{static_cast<std::size_t>(read_offset / get_chunk_size())};
  read_offset = read_offset - (begin_chunk * get_chunk_size());

  unique_mutex_lock read_lock(read_mtx_);
  auto res = check_start();
  if (res != api_error::success) {
    return res;
  }

  for (std::size_t chunk = begin_chunk;
       not stop_requested_ && (res == api_error::success) && (read_size > 0U);
       ++chunk) {
    reset_timeout();

    if (chunk > ring_pos_) {
      forward(chunk - ring_pos_);
    } else if (chunk < ring_pos_) {
      reverse(ring_pos_ - chunk);
    }

    res = download_chunk(chunk, false);
    if (res != api_error::success) {
      if (res == api_error::invalid_ring_buffer_position) {
        read_lock.unlock();

        // TODO limit retry
        return read(read_size, read_offset, data);
      }

      return res;
    }

    reset_timeout();

    std::size_t bytes_read{};
    res = on_read_chunk(
        chunk,
        std::min(static_cast<std::size_t>(get_chunk_size() - read_offset),
                 read_size),
        read_offset, data, bytes_read);
    if (res != api_error::success) {
      return res;
    }

    reset_timeout();

    read_size -= bytes_read;
    read_offset = 0U;
  }

  return stop_requested_ ? api_error::download_stopped : res;
}

void ring_buffer_base::reader_thread() {
  unique_mutex_lock chunk_lock(chunk_mtx_);
  auto next_chunk{ring_pos_};
  chunk_notify_.notify_all();
  chunk_lock.unlock();

  while (not stop_requested_) {
    chunk_lock.lock();

    next_chunk = next_chunk + 1U > ring_end_ ? ring_begin_ : next_chunk + 1U;
    const auto check_and_wait = [this, &chunk_lock, &next_chunk]() {
      if (stop_requested_) {
        chunk_notify_.notify_all();
        chunk_lock.unlock();
        return;
      }

      if (get_read_state().all()) {
        chunk_notify_.wait(chunk_lock);
        next_chunk = ring_pos_;
      }

      chunk_notify_.notify_all();
      chunk_lock.unlock();
    };

    if (read_state_[next_chunk % read_state_.size()]) {
      check_and_wait();
      continue;
    }

    chunk_notify_.notify_all();
    chunk_lock.unlock();

    download_chunk(next_chunk, true);
  }

  event_system::instance().raise<download_end>(
      get_api_path(), get_source_path(), api_error::download_stopped);
}

void ring_buffer_base::reverse(std::size_t count) {
  update_position(count, false);
}

void ring_buffer_base::set(std::size_t first_chunk, std::size_t current_chunk) {
  mutex_lock chunk_lock(chunk_mtx_);
  if (first_chunk >= total_chunks_) {
    chunk_notify_.notify_all();
    throw std::runtime_error("first chunk must be less than total chunks");
  }

  ring_begin_ = first_chunk;
  ring_end_ =
      std::min(total_chunks_ - 1U, ring_begin_ + read_state_.size() - 1U);

  if (current_chunk > ring_end_) {
    chunk_notify_.notify_all();
    throw std::runtime_error(
        "current chunk must be less than or equal to last chunk");
  }

  ring_pos_ = current_chunk;
  read_state_.set(0U, read_state_.size(), true);

  chunk_notify_.notify_all();
}

void ring_buffer_base::set_api_path(const std::string &api_path) {
  mutex_lock chunk_lock(chunk_mtx_);
  open_file_base::set_api_path(api_path);
  chunk_notify_.notify_all();
}

void ring_buffer_base::update_position(std::size_t count, bool is_forward) {
  mutex_lock chunk_lock(chunk_mtx_);

  if (is_forward) {
    if ((ring_pos_ + count) > (total_chunks_ - 1U)) {
      count = (total_chunks_ - 1U) - ring_pos_;
    }
  } else {
    count = std::min(ring_pos_, count);
  }

  if (is_forward ? (ring_pos_ + count) <= ring_end_
                 : (ring_pos_ - count) >= ring_begin_) {
    ring_pos_ += is_forward ? count : -count;
  } else {
    auto delta = is_forward ? count - (ring_end_ - ring_pos_)
                            : count - (ring_pos_ - ring_begin_);

    if (delta >= read_state_.size()) {
      read_state_.set(0U, read_state_.size(), false);
      ring_pos_ += is_forward ? count : -count;
      ring_begin_ += is_forward ? delta : -delta;
    } else {
      for (std::size_t idx = 0U; idx < delta; ++idx) {
        if (is_forward) {
          read_state_[(ring_begin_ + idx) % read_state_.size()] = false;
        } else {
          read_state_[(ring_end_ - idx) % read_state_.size()] = false;
        }
      }
      ring_begin_ += is_forward ? delta : -delta;
      ring_pos_ += is_forward ? count : -count;
    }

    ring_end_ =
        std::min(total_chunks_ - 1U, ring_begin_ + read_state_.size() - 1U);
  }

  chunk_notify_.notify_all();
}
} // namespace repertory
