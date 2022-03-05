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
#include "download/direct_download.hpp"
#include "app_config.hpp"
#include "download/buffered_reader.hpp"
#include "download/events.hpp"
#include "download/utils.hpp"
#include "events/event_system.hpp"
#include "utils/encrypting_reader.hpp"

namespace repertory {
direct_download::direct_download(const app_config &config, filesystem_item fsi,
                                 const api_reader_callback &api_reader, const std::uint64_t &handle)
    : config_(config), fsi_(std::move(fsi)), api_reader_(api_reader), handle_(handle) {}

direct_download::~direct_download() {
  stop_requested_ = true;

  unique_mutex_lock read_lock(read_mutex_);
  if (buffered_reader_) {
    buffered_reader_->notify_stop_requested();
  }
  read_lock.unlock();

  if (buffered_reader_) {
    buffered_reader_.reset();
  }
}

void direct_download::notify_download_end() {
  unique_mutex_lock read_lock(read_mutex_);
  if (not disable_download_end_ && not download_end_notified_) {
    download_end_notified_ = true;
    read_lock.unlock();
    event_system::instance().raise<download_end>(fsi_.api_path, "direct", handle_, error_);
  }
}

void direct_download::notify_stop_requested() {
  set_api_error(api_error::download_stopped);
  stop_requested_ = true;
  notify_download_end();
}

api_error direct_download::read_bytes(const std::uint64_t &, std::size_t read_size,
                                      const std::uint64_t &read_offset, std::vector<char> &data) {
  data.clear();

  if ((read_size = utils::calculate_read_size(fsi_.size, read_size, read_offset)) > 0u) {
    unique_mutex_lock read_lock(read_mutex_);
    if (is_active()) {
      if (not buffered_reader_) {
        const auto chunk_size = utils::encryption::encrypting_reader::get_data_chunk_size();
        const auto read_chunk = static_cast<std::size_t>(read_offset / chunk_size);
        event_system::instance().raise<download_begin>(fsi_.api_path, "direct");
        buffered_reader_ = std::make_unique<buffered_reader>(
            config_, fsi_, api_reader_, chunk_size,
            static_cast<std::size_t>(
                utils::divide_with_ceiling(fsi_.size, static_cast<std::uint64_t>(chunk_size))),
            read_chunk);
      }

      auto read_chunk = static_cast<std::size_t>(read_offset / buffered_reader_->get_chunk_size());
      auto offset = static_cast<std::size_t>(read_offset % buffered_reader_->get_chunk_size());
      while (is_active() && (read_size > 0u)) {
        std::vector<char> buffer;
        auto ret = api_error::success;
        if ((ret = buffered_reader_->read_chunk(read_chunk, buffer)) == api_error::success) {
          const auto total_read = std::min(buffered_reader_->get_chunk_size() - offset, read_size);
          data.insert(data.end(), buffer.begin() + offset, buffer.begin() + offset + total_read);
          buffer.clear();
          read_chunk++;
          offset = 0u;
          read_size -= total_read;
        } else {
          set_api_error(ret);

          read_lock.unlock();
          notify_download_end();
          read_lock.lock();
        }
      }

      if (is_active()) {
        utils::download::notify_progress<download_progress>(
            config_, fsi_.api_path, "direct", static_cast<double>(data.size() + read_offset),
            static_cast<double>(fsi_.size), progress_);
      }
    }
  }

  return error_;
}

void direct_download::set_api_error(const api_error &error) {
  if ((error_ == api_error::success) && (error_ != error)) {
    error_ = error;
  }
}
} // namespace repertory
