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
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/unix/unix_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
file_manager::ring_buffer_open_file::ring_buffer_open_file(
    std::string buffer_directory, std::uint64_t chunk_size,
    std::uint8_t chunk_timeout, filesystem_item fsi, i_provider &provider)
    : ring_buffer_open_file(std::move(buffer_directory), chunk_size,
                            chunk_timeout, std::move(fsi), provider,
                            (1024ull * 1024ull * 1024ull) / chunk_size) {}

file_manager::ring_buffer_open_file::ring_buffer_open_file(
    std::string buffer_directory, std::uint64_t chunk_size,
    std::uint8_t chunk_timeout, filesystem_item fsi, i_provider &provider,
    std::size_t ring_size)
    : open_file_base(chunk_size, chunk_timeout, fsi, provider),
      ring_state_(ring_size),
      total_chunks_(static_cast<std::size_t>(
          utils::divide_with_ceiling(fsi.size, chunk_size_))) {
  if (ring_size % 2u) {
    throw std::runtime_error("ring size must be a multiple of 2");
  }

  if (ring_size < 4u) {
    throw std::runtime_error("ring size must be greater than or equal to 4");
  }

  if (fsi.size < (ring_state_.size() * chunk_size)) {
    throw std::runtime_error("file size is less than ring buffer size");
  }

  last_chunk_ = ring_state_.size() - 1u;
  ring_state_.set(0u, ring_state_.size(), true);

  buffer_directory = utils::path::absolute(buffer_directory);
  if (not utils::file::create_full_directory_path(buffer_directory)) {
    throw std::runtime_error("failed to create buffer directory|path|" +
                             buffer_directory + "|err|" +
                             std::to_string(utils::get_last_error_code()));
  }

  fsi_.source_path =
      utils::path::combine(buffer_directory, {utils::create_uuid_string()});
  auto res = native_file::create_or_open(fsi_.source_path, nf_);
  if (res != api_error::success) {
    throw std::runtime_error("failed to create buffer file|err|" +
                             std::to_string(utils::get_last_error_code()));
  }

  if (not nf_->truncate(ring_state_.size() * chunk_size)) {
    nf_->close();
    throw std::runtime_error("failed to resize buffer file|err|" +
                             std::to_string(utils::get_last_error_code()));
  }
}

file_manager::ring_buffer_open_file::~ring_buffer_open_file() {
  close();

  nf_->close();
  if (not utils::file::retry_delete_file(fsi_.source_path)) {
    utils::error::raise_api_path_error(
        __FUNCTION__, fsi_.api_path, fsi_.source_path,
        utils::get_last_error_code(), "failed to delete file");
  }
}

auto file_manager::file_manager::ring_buffer_open_file::download_chunk(
    std::size_t chunk) -> api_error {
  unique_mutex_lock chunk_lock(chunk_mtx_);
  if (active_downloads_.find(chunk) != active_downloads_.end()) {
    auto active_download = active_downloads_.at(chunk);
    chunk_notify_.notify_all();
    chunk_lock.unlock();

    return active_download->wait();
  }

  if (ring_state_[chunk % ring_state_.size()]) {
    auto active_download = std::make_shared<download>();
    active_downloads_[chunk] = active_download;
    ring_state_[chunk % ring_state_.size()] = false;
    chunk_notify_.notify_all();
    chunk_lock.unlock();

    data_buffer buffer((chunk == (total_chunks_ - 1u)) ? last_chunk_size_
                                                       : chunk_size_);

    stop_type stop_requested = !!ring_state_[chunk % ring_state_.size()];
    auto res =
        provider_.read_file_bytes(fsi_.api_path, buffer.size(),
                                  chunk * chunk_size_, buffer, stop_requested);
    if (res == api_error::success) {
      res = do_io([&]() -> api_error {
        std::size_t bytes_written{};
        if (not nf_->write_bytes(buffer.data(), buffer.size(),
                                 (chunk % ring_state_.size()) * chunk_size_,
                                 bytes_written)) {
          return api_error::os_error;
        }

        return api_error::success;
      });
    }

    active_download->notify(res);

    chunk_lock.lock();
    active_downloads_.erase(chunk);
    chunk_notify_.notify_all();
    return res;
  }

  chunk_notify_.notify_all();
  chunk_lock.unlock();

  return api_error::success;
}

void file_manager::ring_buffer_open_file::forward(std::size_t count) {
  mutex_lock chunk_lock(chunk_mtx_);
  if ((current_chunk_ + count) > (total_chunks_ - 1u)) {
    count = (total_chunks_ - 1u) - current_chunk_;
  }

  if ((current_chunk_ + count) <= last_chunk_) {
    current_chunk_ += count;
  } else {
    const auto added = count - (last_chunk_ - current_chunk_);
    if (added >= ring_state_.size()) {
      ring_state_.set(0u, ring_state_.size(), true);
      current_chunk_ += count;
      first_chunk_ += added;
      last_chunk_ =
          std::min(total_chunks_ - 1u, first_chunk_ + ring_state_.size() - 1u);
    } else {
      for (std::size_t i = 0u; i < added; i++) {
        ring_state_[(first_chunk_ + i) % ring_state_.size()] = true;
      }
      first_chunk_ += added;
      current_chunk_ += count;
      last_chunk_ =
          std::min(total_chunks_ - 1u, first_chunk_ + ring_state_.size() - 1u);
    }
  }

  chunk_notify_.notify_all();
}

auto file_manager::ring_buffer_open_file::get_read_state() const
    -> boost::dynamic_bitset<> {
  recur_mutex_lock file_lock(file_mtx_);
  auto read_state = ring_state_;
  return read_state.flip();
}

auto file_manager::ring_buffer_open_file::get_read_state(
    std::size_t chunk) const -> bool {
  recur_mutex_lock file_lock(file_mtx_);
  return not ring_state_[chunk % ring_state_.size()];
}

auto file_manager::ring_buffer_open_file::is_download_complete() const -> bool {
  return false;
}

auto file_manager::ring_buffer_open_file::native_operation(
    const i_open_file::native_operation_callback &operation) -> api_error {
  return do_io([&]() -> api_error { return operation(nf_->get_handle()); });
}

void file_manager::ring_buffer_open_file::reverse(std::size_t count) {
  mutex_lock chunk_lock(chunk_mtx_);
  if (current_chunk_ < count) {
    count = current_chunk_;
  }

  if ((current_chunk_ - count) >= first_chunk_) {
    current_chunk_ -= count;
  } else {
    const auto removed = count - (current_chunk_ - first_chunk_);
    if (removed >= ring_state_.size()) {
      ring_state_.set(0u, ring_state_.size(), true);
      current_chunk_ -= count;
      first_chunk_ = current_chunk_;
      last_chunk_ =
          std::min(total_chunks_ - 1u, first_chunk_ + ring_state_.size() - 1u);
    } else {
      for (std::size_t i = 0u; i < removed; i++) {
        ring_state_[(last_chunk_ - i) % ring_state_.size()] = true;
      }
      first_chunk_ -= removed;
      current_chunk_ -= count;
      last_chunk_ =
          std::min(total_chunks_ - 1u, first_chunk_ + ring_state_.size() - 1u);
    }
  }

  chunk_notify_.notify_all();
}

auto file_manager::ring_buffer_open_file::read(std::size_t read_size,
                                               std::uint64_t read_offset,
                                               data_buffer &data) -> api_error {
  if (fsi_.directory) {
    return api_error::invalid_operation;
  }

  reset_timeout();

  read_size = utils::calculate_read_size(fsi_.size, read_size, read_offset);
  if (read_size == 0u) {
    return api_error::success;
  }

  const auto start_chunk_index =
      static_cast<std::size_t>(read_offset / chunk_size_);
  read_offset = read_offset - (start_chunk_index * chunk_size_);
  data_buffer buffer(chunk_size_);

  auto res = api_error::success;
  for (std::size_t chunk = start_chunk_index;
       (res == api_error::success) && (read_size > 0u); chunk++) {
    if (chunk > current_chunk_) {
      forward(chunk - current_chunk_);
    } else if (chunk < current_chunk_) {
      reverse(current_chunk_ - chunk);
    }

    reset_timeout();
    if ((res = download_chunk(chunk)) == api_error::success) {
      const auto to_read = std::min(
          static_cast<std::size_t>(chunk_size_ - read_offset), read_size);
      res = do_io([this, &buffer, &chunk, &data, read_offset,
                   &to_read]() -> api_error {
        std::size_t bytes_read{};
        auto res = nf_->read_bytes(buffer.data(), buffer.size(),
                                   ((chunk % ring_state_.size()) * chunk_size_),
                                   bytes_read)
                       ? api_error::success
                       : api_error::os_error;
        if (res == api_error::success) {
          data.insert(data.end(), buffer.begin() + read_offset,
                      buffer.begin() + read_offset + to_read);
          reset_timeout();
        }

        return res;
      });
      read_offset = 0u;
      read_size -= to_read;
    }
  }

  return res;
}

void file_manager::ring_buffer_open_file::set(std::size_t first_chunk,
                                              std::size_t current_chunk) {
  mutex_lock chunk_lock(chunk_mtx_);
  if (first_chunk >= total_chunks_) {
    chunk_notify_.notify_all();
    throw std::runtime_error("first chunk must be less than total chunks");
  }

  first_chunk_ = first_chunk;
  last_chunk_ = first_chunk_ + ring_state_.size() - 1u;

  if (current_chunk > last_chunk_) {
    chunk_notify_.notify_all();
    throw std::runtime_error(
        "current chunk must be less than or equal to last chunk");
  }

  current_chunk_ = current_chunk;
  ring_state_.set(0u, ring_state_.size(), false);

  chunk_notify_.notify_all();
}

void file_manager::ring_buffer_open_file::set_api_path(
    const std::string &api_path) {
  mutex_lock chunk_lock(chunk_mtx_);
  open_file_base::set_api_path(api_path);
  chunk_notify_.notify_all();
}
} // namespace repertory
