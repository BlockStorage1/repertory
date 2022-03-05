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
#include "download/buffered_reader.hpp"
#include "app_config.hpp"
#include "download/reader_pool.hpp"

namespace repertory {
buffered_reader::buffered_reader(const app_config &config, const filesystem_item &fsi,
                                 const api_reader_callback &api_reader,
                                 const std::size_t &chunk_size, const std::size_t &total_chunks,
                                 const std::size_t &start_chunk)
    : fsi_(fsi),
      api_reader_(api_reader),
      chunk_size_(chunk_size),
      total_chunks_(total_chunks),
      ring_state_(config.get_read_ahead_count()) {
  ring_data_.resize(ring_state_.size());
  const auto read_chunk = [this](const std::size_t &read_offset,
                                 std::unique_ptr<std::vector<char>> &chunk_ptr) -> api_error {
    auto ret = api_error::success;
    const auto read_size = utils::calculate_read_size(fsi_.size, chunk_size_, read_offset);
    if (read_size) {
      std::vector<char> data;
      if ((ret = api_reader_(fsi_.api_path, read_size, read_offset + read_offset_, data,
                             stop_requested_)) == api_error::success) {
        chunk_ptr = std::make_unique<std::vector<char>>(std::move(data));
      }
    }

    return ret;
  };

  auto first = std::async(std::launch::async, [this, &read_chunk]() -> api_error {
    return read_chunk(0u, first_chunk_data_);
  });

  if (total_chunks_ > 1u) {
    auto last = std::async(std::launch::async, [this, &read_chunk]() -> api_error {
      return read_chunk(((total_chunks_ - 1u) * chunk_size_), last_chunk_data_);
    });
    error_ = last.get();
  }

  if (error_ == api_error::success) {
    if ((error_ = first.get()) == api_error::success) {
      read_chunk_index_ = write_chunk_index_ =
          (first_chunk_data_ ? ((start_chunk == 0u) ? 1u : start_chunk) : start_chunk);
      if (ring_data_.size() > 1u) {
        reader_thread_ = std::make_unique<std::thread>([this]() { this->reader_thread(); });
      }
    }
  }
}

buffered_reader::~buffered_reader() {
  notify_stop_requested();

  unique_mutex_lock write_lock(write_mutex_);
  read_notify_.notify_all();
  write_lock.unlock();

  if (reader_thread_) {
    reader_thread_->join();
    reader_thread_.reset();
  }
}

void buffered_reader::notify_stop_requested() {
  error_ = api_error::download_stopped;
  stop_requested_ = true;
}

api_error buffered_reader::read_chunk(const std::size_t &chunk_index, std::vector<char> &data) {
  if (error_ == api_error::success) {
    data.clear();

    if (last_chunk_data_ && (chunk_index == (total_chunks_ - 1u))) {
      data = *last_chunk_data_;
    } else if (first_chunk_data_ && (chunk_index == 0u)) {
      data = *first_chunk_data_;
    } else if (ring_state_.size() == 1u) {
      const auto read_offset = chunk_index * chunk_size_;
      std::size_t read_size = 0u;
      if ((read_size = utils::calculate_read_size(fsi_.size, chunk_size_, read_offset)) > 0u) {
        error_ = api_reader_(fsi_.api_path, read_size, read_offset + read_offset_, data,
                             stop_requested_);
      }
    } else {
      mutex_lock read_lock(read_mutex_);
      const auto read_position = chunk_index % ring_state_.size();
      unique_mutex_lock write_lock(write_mutex_);
      while ((reset_reader_ ||
              ((chunk_index < read_chunk_index_) || (chunk_index > write_chunk_index_))) &&
             is_active()) {
        reset_reader_ = true;
        read_chunk_index_ = write_chunk_index_ = chunk_index;
        read_notify_.notify_all();
        read_notify_.wait(write_lock);
      }

      if (is_active()) {
        read_chunk_index_ = chunk_index;
        read_notify_.notify_all();
        while (not ring_state_[read_position] && is_active()) {
          read_notify_.wait(write_lock);
        }
        data = ring_data_[read_position];
      }

      read_notify_.notify_all();
      write_lock.unlock();
    }
  }

  return error_;
}

void buffered_reader::reader_thread() {
  reader_pool pool(ring_state_.size(), api_reader_);

  unique_mutex_lock write_lock(write_mutex_);
  read_notify_.notify_all();
  write_lock.unlock();

  while (is_active()) {
    write_lock.lock();
    while ((write_chunk_index_ > read_chunk_index_) &&
           (write_chunk_index_ > (read_chunk_index_ + ring_state_.size() - 1u)) &&
           not reset_reader_ && is_active()) {
      read_notify_.wait(write_lock);
    }

    if (is_active()) {
      if (reset_reader_) {
        read_notify_.notify_all();
        write_lock.unlock();

        pool.restart();

        write_lock.lock();
        write_chunk_index_ = read_chunk_index_;
        ring_state_.reset();
        reset_reader_ = false;
      }

      const auto file_offset = write_chunk_index_ * chunk_size_;
      const auto write_chunk_index = write_chunk_index_;
      const auto write_position = write_chunk_index_ % ring_state_.size();
      ring_state_[write_position] = false;
      read_notify_.notify_all();
      write_lock.unlock();

      const auto read_size = utils::calculate_read_size(fsi_.size, chunk_size_, file_offset);
      if (read_size > 0u) {
        if ((first_chunk_data_ && (write_chunk_index == 0u)) ||
            (last_chunk_data_ && (write_chunk_index == (total_chunks_ - 1u)))) {
          write_lock.lock();
          write_chunk_index_++;
        } else {
          const auto completed = [this, write_position](api_error error) {
            mutex_lock write_lock(write_mutex_);
            if (error == api_error::success) {
              ring_state_[write_position] = true;
            } else if (not reset_reader_) {
              error_ = error;
            }
            read_notify_.notify_all();
          };
          pool.queue_read_bytes(fsi_.api_path, read_size,
                                ((write_chunk_index * chunk_size_) + read_offset_),
                                ring_data_[write_position], completed);

          write_lock.lock();
          write_chunk_index_++;
        }
      } else {
        write_lock.lock();
        while (not reset_reader_ && ((write_chunk_index_ * chunk_size_) >= fsi_.size) &&
               is_active()) {
          read_notify_.wait(write_lock);
        }
      }
    }
    read_notify_.notify_all();
    write_lock.unlock();
  }
}
} // namespace repertory
