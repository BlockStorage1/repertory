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
#include "download/ring_download.hpp"
#include "app_config.hpp"
#include "download/buffered_reader.hpp"
#include "download/events.hpp"
#include "download/utils.hpp"
#include "events/event_system.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
ring_download::ring_download(const app_config &config, filesystem_item fsi,
                             const api_reader_callback &api_reader, const std::uint64_t &handle,
                             const std::size_t &chunk_size, const std::size_t &ring_buffer_size)
    : config_(config),
      fsi_(std::move(fsi)),
      api_reader_(api_reader),
      handle_(handle),
      chunk_size_(chunk_size),
      ring_state_(ring_buffer_size / chunk_size) {
  if (not chunk_size) {
    error_ = api_error::empty_ring_buffer_chunk_size;
  } else if (not ring_buffer_size) {
    error_ = api_error::empty_ring_buffer_size;
  } else if (ring_buffer_size % chunk_size) {
    error_ = api_error::invalid_ring_buffer_multiple;
  } else if (ring_buffer_size < chunk_size) {
    error_ = api_error::invalid_ring_buffer_size;
  } else {
    *const_cast<std::size_t *>(&total_chunks_) =
        utils::divide_with_ceiling(static_cast<std::size_t>(fsi.size), chunk_size_);
    const auto buffer_directory = utils::path::combine(config_.get_data_directory(), {"buffer"});
    if (utils::file::create_full_directory_path(buffer_directory)) {
      buffer_file_path_ = utils::path::combine(buffer_directory, {utils::create_uuid_string()});
      if ((error_ = native_file::create_or_open(buffer_file_path_, buffer_file_)) ==
          api_error::success) {
        if (not buffer_file_->allocate(ring_buffer_size)) {
          error_ = api_error::os_error;
        }
      }
    } else {
      error_ = api_error::os_error;
    }
  }
}

ring_download::~ring_download() {
  stop();

  if (buffer_file_) {
    buffer_file_->close();
    utils::file::delete_file(buffer_file_path_);
  }
}

void ring_download::buffer_thread(std::size_t start_chunk_index) {
  buffered_reader reader(config_, fsi_, api_reader_, chunk_size_, total_chunks_, start_chunk_index);

  unique_mutex_lock write_lock(write_mutex_);
  buffered_reader_ = &reader;
  read_notify_.notify_all();
  write_lock.unlock();

  const auto ring_size = ring_state_.size();
  const auto half_ring_size = ring_size / 2;
  double progress = 0.0;

  const auto reset_on_overflow = [&](const std::size_t &amt) -> bool {
    if (amt >= ring_size) {
      ring_state_.reset();
      head_chunk_index_ = read_chunk_;
      return true;
    }
    return false;
  };

  const auto decrement_head_chunk = [&](std::size_t amt) {
    if (not reset_on_overflow(amt)) {
      while (amt-- && head_chunk_index_) {
        head_chunk_index_--;
        ring_state_[head_chunk_index_ % ring_size] = false;
      }
    }
    write_chunk_ = read_chunk_;
  };

  const auto increment_head_chunk = [&](std::size_t amt) {
    if (not reset_on_overflow(amt)) {
      while (amt-- && (head_chunk_index_ < (total_chunks_ - 1u))) {
        ring_state_[head_chunk_index_ % ring_size] = false;
        head_chunk_index_++;
      }
    }
    write_chunk_ = read_chunk_;
  };

  while (is_active()) {
    write_lock.lock();

    const auto get_overflow_chunk = [&]() { return head_chunk_index_ + ring_size; };
    while ((write_chunk_ > read_chunk_) && (write_chunk_ >= get_overflow_chunk()) && is_active()) {
      // Buffer 50% for read-ahead/read-behind
      auto buffered = false;
      while ((write_chunk_ > read_chunk_) && ((write_chunk_ - read_chunk_) > half_ring_size) &&
             is_active()) {
        buffered = true;
        read_notify_.wait(write_lock);
      }

      if (not buffered && is_active()) {
        read_notify_.wait(write_lock);
      }

      if ((write_chunk_ > read_chunk_) && ((write_chunk_ - read_chunk_) <= half_ring_size)) {
        increment_head_chunk(1u);
      }
    }

    if (is_active()) {
      if (read_chunk_ >= get_overflow_chunk()) {
        increment_head_chunk(read_chunk_ - get_overflow_chunk() + 1u);
      } else if (read_chunk_ < head_chunk_index_) {
        decrement_head_chunk(head_chunk_index_ - read_chunk_);
      } else if ((write_chunk_ < head_chunk_index_) || (write_chunk_ >= get_overflow_chunk())) {
        write_chunk_ = read_chunk_;
      }

      const auto write_position = write_chunk_ % ring_size;
      if (ring_state_[write_position]) {
        write_chunk_++;
      } else {
        const auto file_offset = write_chunk_ * chunk_size_;
        const auto write_chunk_index = write_chunk_;
        read_notify_.notify_all();
        write_lock.unlock();

        const auto read_size = utils::calculate_read_size(fsi_.size, chunk_size_, file_offset);
        if (read_size > 0u) {
          if (((write_chunk_index == 0u) && reader.has_first_chunk()) ||
              ((write_chunk_index == (total_chunks_ - 1ull)) && reader.has_last_chunk())) {
            write_lock.lock();
            write_chunk_++;
          } else {
            std::vector<char> data;
            auto error = reader.read_chunk(write_chunk_index, data);
            if (error == api_error::success) {
              std::mutex job_mutex;
              std::condition_variable job_notify;
              unique_mutex_lock job_lock(job_mutex);

              if (queue_io_item(job_mutex, job_notify, false, [&]() {
#ifdef _DEBUG
                    write_lock.lock();
                    if (write_chunk_index >= (head_chunk_index_ + ring_size)) {
                      throw std::runtime_error("Invalid write: write chunk >= head");
                    }
                    if (write_chunk_index < head_chunk_index_) {
                      throw std::runtime_error("Invalid write: write chunk < head");
                    }
#endif // _DEBUG
                    std::size_t bytes_written = 0u;
                    if (buffer_file_->write_bytes(&data[0u], data.size(),
                                                  write_position * chunk_size_, bytes_written)) {
                      buffer_file_->flush();
                      utils::download::notify_progress<download_progress>(
                          config_, fsi_.api_path, buffer_file_path_,
                          static_cast<double>(write_chunk_index + 1ull),
                          static_cast<double>(total_chunks_), progress);
                    } else {
                      error = api_error::os_error;
                    }
#ifdef _DEBUG
                    write_lock.unlock();
#endif // _DEBUG
                  })) {
                job_notify.wait(job_lock);
              }
              job_notify.notify_all();
              job_lock.unlock();
            }

            set_api_error(error);

            write_lock.lock();
            if (error == api_error::success) {
              ring_state_[write_position] = true;
              write_chunk_++;
            }
          }
        } else {
          write_lock.lock();
          while ((read_chunk_ <= write_chunk_) && (read_chunk_ >= head_chunk_index_) &&
                 (((write_chunk_ * chunk_size_) >= fsi_.size)) && is_active()) {
            read_notify_.wait(write_lock);
          }
        }
      }
    }

    read_notify_.notify_all();
    write_lock.unlock();
  }

  write_lock.lock();
  buffered_reader_ = nullptr;
  read_notify_.notify_all();
  write_lock.unlock();

  if (not disable_download_end_) {
    event_system::instance().raise<download_end>(fsi_.api_path, buffer_file_path_, handle_, error_);
  }
}

void ring_download::io_thread() {
  const auto process_all = [&]() {
    unique_mutex_lock io_lock(io_mutex_);
    while (not io_queue_.empty()) {
      auto &action = io_queue_.front();
      io_lock.unlock();

      unique_mutex_lock job_lock(action->mutex);
      action->action();
      action->notify.notify_all();
      job_lock.unlock();

      io_lock.lock();
      utils::remove_element_from(io_queue_, action);
    }
    io_lock.unlock();
  };

  while (not stop_requested_) {
    if (io_queue_.empty()) {
      unique_mutex_lock io_lock(io_mutex_);
      if (not stop_requested_ && io_queue_.empty()) {
        io_notify_.wait(io_lock);
      }
    } else {
      process_all();
    }
  }

  process_all();
}

void ring_download::notify_stop_requested() {
  set_api_error(api_error::download_stopped);
  stop_requested_ = true;

  unique_mutex_lock write_lock(write_mutex_);
  read_notify_.notify_all();
  write_lock.unlock();

  unique_mutex_lock io_lock(io_mutex_);
  io_notify_.notify_all();
  io_lock.unlock();
}

bool ring_download::queue_io_item(std::mutex &m, std::condition_variable &cv, const bool &is_read,
                                  std::function<void()> action) {
  auto ret = false;

  mutex_lock io_lock(io_mutex_);
  if (not stop_requested_) {
    ret = true;
    if (is_read) {
      io_queue_.insert(io_queue_.begin(), std::make_unique<io_action>(m, cv, action));
    } else {
      io_queue_.emplace_back(std::make_unique<io_action>(m, cv, action));
    }
  }
  io_notify_.notify_all();

  return ret;
}

void ring_download::read(std::size_t read_chunk_index, std::size_t read_size,
                         std::size_t read_offset, std::vector<char> &data) {
  while (is_active() && (read_size > 0u)) {
    unique_mutex_lock write_lock(write_mutex_);
    if ((read_chunk_index == (total_chunks_ - 1ull)) && buffered_reader_ &&
        buffered_reader_->has_last_chunk()) {
      std::vector<char> *buffer = nullptr;
      buffered_reader_->get_last_chunk(buffer);
      if (buffer) {
        data.insert(data.end(), buffer->begin() + read_offset,
                    buffer->begin() + read_offset + read_size);
      }
      read_notify_.notify_all();
      write_lock.unlock();
      read_size = 0u;
    } else {
      const auto notify_read_location = [&]() {
        if (read_chunk_index != read_chunk_) {
          write_lock.lock();
          read_chunk_ = read_chunk_index;
          read_notify_.notify_all();
          write_lock.unlock();
        }
      };

      if ((read_chunk_index == 0u) && buffered_reader_ && buffered_reader_->has_first_chunk()) {
        const auto to_read = std::min(chunk_size_ - read_offset, read_size);
        std::vector<char> *buffer = nullptr;
        buffered_reader_->get_first_chunk(buffer);
        if (buffer) {
          data.insert(data.end(), buffer->begin() + read_offset,
                      buffer->begin() + read_offset + to_read);
        }
        read_notify_.notify_all();
        write_lock.unlock();

        read_size -= to_read;
        read_offset = 0u;
        read_chunk_index++;
        notify_read_location();
      } else {
        const auto ring_size = ring_state_.size();
        while (((read_chunk_index > write_chunk_) || (read_chunk_index < head_chunk_index_) ||
                (read_chunk_index >= (head_chunk_index_ + ring_size))) &&
               is_active()) {
          read_chunk_ = read_chunk_index;
          read_notify_.notify_all();
          read_notify_.wait(write_lock);
        }
        read_notify_.notify_all();
        write_lock.unlock();

        if (is_active()) {
          const auto ringPos = read_chunk_index % ring_size;
          notify_read_location();
          if (ring_state_[ringPos]) {
            const auto to_read = std::min(chunk_size_ - read_offset, read_size);
            std::mutex job_mutex;
            std::condition_variable job_notify;
            unique_mutex_lock job_lock(job_mutex);

            if (queue_io_item(job_mutex, job_notify, true, [&]() {
                  std::vector<char> buffer(to_read);
                  std::size_t bytes_read = 0u;
                  if (buffer_file_->read_bytes(&buffer[0u], buffer.size(),
                                               (ringPos * chunk_size_) + read_offset, bytes_read)) {
                    data.insert(data.end(), buffer.begin(), buffer.end());
                    buffer.clear();

                    read_size -= to_read;
                    read_offset = 0u;
                  } else {
                    set_api_error(api_error::os_error);
                  }
                })) {
              job_notify.wait(job_lock);
              read_chunk_index++;
            }
            job_notify.notify_all();
            job_lock.unlock();
          } else {
            write_lock.lock();
            while (not ring_state_[ringPos] && is_active()) {
              read_notify_.wait(write_lock);
            }
            read_notify_.notify_all();
            write_lock.unlock();
          }
        }
      }
    }
  }
}

api_error ring_download::read_bytes(const std::uint64_t &, std::size_t read_size,
                                    const std::uint64_t &read_offset, std::vector<char> &data) {
  data.clear();

  mutex_lock read_lock(read_mutex_);
  if (is_active()) {
    if (((read_size = utils::calculate_read_size(fsi_.size, read_size, read_offset)) > 0u)) {
      const auto read_chunk_index = static_cast<std::size_t>(read_offset / chunk_size_);
      if (not buffer_thread_) {
        start(read_chunk_index);
      }

      read(read_chunk_index, read_size, read_offset % chunk_size_, data);
      set_api_error(((error_ == api_error::success) && stop_requested_)
                        ? api_error::download_stopped
                        : error_);
    }
  }

  return error_;
}

void ring_download::set_api_error(const api_error &error) {
  if ((error_ == api_error::success) && (error_ != error)) {
    error_ = error;
  }
}

void ring_download::start(const std::size_t &startChunk) {
  event_system::instance().raise<download_begin>(fsi_.api_path, buffer_file_path_);

  error_ = api_error::success;
  stop_requested_ = false;
  head_chunk_index_ = read_chunk_ = write_chunk_ = startChunk;
  ring_state_.reset();

  io_thread_ = std::make_unique<std::thread>([this] { this->io_thread(); });

  unique_mutex_lock write_lock(write_mutex_);
  buffer_thread_ =
      std::make_unique<std::thread>([this, startChunk]() { this->buffer_thread(startChunk); });
  read_notify_.wait(write_lock);
  read_notify_.notify_all();
  write_lock.unlock();
}

void ring_download::stop() {
  notify_stop_requested();

  if (buffer_thread_) {
    buffer_thread_->join();
    buffer_thread_.reset();
  }

  if (io_thread_) {
    io_thread_->join();
    io_thread_.reset();

    io_queue_.clear();
  }
}
} // namespace repertory
