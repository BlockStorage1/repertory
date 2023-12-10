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

#include "file_manager/events.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "types/startup_exception.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/unix/unix_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
file_manager::open_file::open_file(std::uint64_t chunk_size,
                                   std::uint8_t chunk_timeout,
                                   filesystem_item fsi, i_provider &provider,
                                   i_upload_manager &mgr)
    : open_file(chunk_size, chunk_timeout, fsi, {}, provider, std::nullopt,
                mgr) {}

file_manager::open_file::open_file(
    std::uint64_t chunk_size, std::uint8_t chunk_timeout, filesystem_item fsi,
    std::map<std::uint64_t, open_file_data> open_data, i_provider &provider,
    i_upload_manager &mgr)
    : open_file(chunk_size, chunk_timeout, fsi, open_data, provider,
                std::nullopt, mgr) {}

file_manager::open_file::open_file(
    std::uint64_t chunk_size, std::uint8_t chunk_timeout, filesystem_item fsi,
    i_provider &provider, std::optional<boost::dynamic_bitset<>> read_state,
    i_upload_manager &mgr)
    : open_file(chunk_size, chunk_timeout, fsi, {}, provider, read_state, mgr) {
}

file_manager::open_file::open_file(
    std::uint64_t chunk_size, std::uint8_t chunk_timeout, filesystem_item fsi,
    std::map<std::uint64_t, open_file_data> open_data, i_provider &provider,
    std::optional<boost::dynamic_bitset<>> read_state, i_upload_manager &mgr)
    : open_file_base(chunk_size, chunk_timeout, fsi, open_data, provider),
      mgr_(mgr) {
  if (fsi_.directory && read_state.has_value()) {
    throw startup_exception("cannot resume a directory|" + fsi.api_path);
  }

  if (not fsi.directory) {
    set_api_error(native_file::create_or_open(
        fsi.source_path, not provider_.is_direct_only(), nf_));
    if (get_api_error() == api_error::success) {
      if (read_state.has_value()) {
        read_state_ = read_state.value();
        set_modified();
      } else if (fsi_.size > 0U) {
        read_state_.resize(static_cast<std::size_t>(utils::divide_with_ceiling(
                               fsi_.size, chunk_size)),
                           false);

        std::uint64_t file_size{};
        if (nf_->get_file_size(file_size)) {
          if (provider_.is_direct_only() || file_size == fsi.size) {
            read_state_.set(0U, read_state_.size(), true);
          } else if (not nf_->truncate(fsi.size)) {
            set_api_error(api_error::os_error);
          }
        } else {
          set_api_error(api_error::os_error);
        }
      }

      if (get_api_error() != api_error::success && nf_) {
        nf_->close();
      }
    }
  }
}

file_manager::open_file::~open_file() { close(); }

void file_manager::open_file::download_chunk(std::size_t chunk,
                                             bool skip_active,
                                             bool should_reset) {
  if (should_reset) {
    reset_timeout();
  }

  unique_recur_mutex_lock download_lock(file_mtx_);
  if ((get_api_error() == api_error::success) && (chunk < read_state_.size()) &&
      not read_state_[chunk]) {
    if (active_downloads_.find(chunk) != active_downloads_.end()) {
      if (not skip_active) {
        auto active_download = active_downloads_.at(chunk);
        download_lock.unlock();

        active_download->wait();
      }

      return;
    }

    const auto data_offset = chunk * chunk_size_;
    const auto data_size =
        (chunk == read_state_.size() - 1U) ? last_chunk_size_ : chunk_size_;
    if (active_downloads_.empty() && (read_state_.count() == 0U)) {
      event_system::instance().raise<download_begin>(fsi_.api_path,
                                                     fsi_.source_path);
    }
    event_system::instance().raise<download_chunk_begin>(
        fsi_.api_path, fsi_.source_path, chunk, read_state_.size(),
        read_state_.count());

    active_downloads_[chunk] = std::make_shared<download>();
    download_lock.unlock();

    if (should_reset) {
      reset_timeout();
    }

    std::async(std::launch::async, [this, chunk, data_size, data_offset,
                                    should_reset]() {
      const auto notify_complete = [this, chunk, should_reset]() {
        unique_recur_mutex_lock file_lock(file_mtx_);
        auto active_download = active_downloads_.at(chunk);
        active_downloads_.erase(chunk);
        event_system::instance().raise<download_chunk_end>(
            fsi_.api_path, fsi_.source_path, chunk, read_state_.size(),
            read_state_.count(), get_api_error());
        if (get_api_error() == api_error::success) {
          auto progress = (static_cast<double>(read_state_.count()) /
                           static_cast<double>(read_state_.size()) * 100.0);
          event_system::instance().raise<download_progress>(
              fsi_.api_path, fsi_.source_path, progress);
          if (read_state_.all() && not notified_) {
            notified_ = true;
            event_system::instance().raise<download_end>(
                fsi_.api_path, fsi_.source_path, get_api_error());
          }
        } else if (not notified_) {
          notified_ = true;
          event_system::instance().raise<download_end>(
              fsi_.api_path, fsi_.source_path, get_api_error());
        }
        file_lock.unlock();

        active_download->notify(get_api_error());

        if (should_reset) {
          reset_timeout();
        }
      };

      data_buffer data;
      auto res = provider_.read_file_bytes(get_api_path(), data_size,
                                           data_offset, data, stop_requested_);
      if (res != api_error::success) {
        set_api_error(res);
        notify_complete();
        return;
      }

      if (should_reset) {
        reset_timeout();
      }

      res = do_io([&]() -> api_error {
        std::size_t bytes_written{};
        if (not nf_->write_bytes(data.data(), data.size(), data_offset,
                                 bytes_written)) {
          return api_error::os_error;
        }

        if (should_reset) {
          reset_timeout();
        }
        return api_error::success;
      });
      if (res != api_error::success) {
        set_api_error(res);
        notify_complete();
        return;
      }

      unique_recur_mutex_lock file_lock(file_mtx_);
      read_state_.set(chunk);
      file_lock.unlock();

      notify_complete();
    }).wait();
  }
}

void file_manager::open_file::download_range(
    std::size_t start_chunk_index, std::size_t end_chunk_index_inclusive,
    bool should_reset) {
  for (std::size_t chunk = start_chunk_index;
       chunk <= end_chunk_index_inclusive; chunk++) {
    download_chunk(chunk, false, should_reset);
    if (get_api_error() != api_error::success) {
      return;
    }
  }
}

auto file_manager::open_file::get_read_state() const
    -> boost::dynamic_bitset<> {
  recur_mutex_lock file_lock(file_mtx_);
  return read_state_;
}

auto file_manager::open_file::get_read_state(std::size_t chunk) const -> bool {
  recur_mutex_lock file_lock(file_mtx_);
  return read_state_[chunk];
}

auto file_manager::open_file::is_complete() const -> bool {
  recur_mutex_lock file_lock(file_mtx_);
  return read_state_.all();
}

auto file_manager::open_file::native_operation(
    const i_open_file::native_operation_callback &callback) -> api_error {
  unique_recur_mutex_lock file_lock(file_mtx_);
  if (stop_requested_) {
    return api_error::download_stopped;
  }
  file_lock.unlock();

  return do_io([&]() -> api_error { return callback(nf_->get_handle()); });
}

auto file_manager::open_file::native_operation(
    std::uint64_t new_file_size,
    const i_open_file::native_operation_callback &callback) -> api_error {
  if (fsi_.directory) {
    return api_error::invalid_operation;
  }

  unique_recur_mutex_lock file_lock(file_mtx_);
  if (stop_requested_) {
    return api_error::download_stopped;
  }
  file_lock.unlock();

  const auto is_empty_file = new_file_size == 0U;
  const auto last_chunk =
      is_empty_file ? std::size_t(0U)
                    : static_cast<std::size_t>(utils::divide_with_ceiling(
                          new_file_size, chunk_size_)) -
                          1U;

  file_lock.lock();
  if (not is_empty_file && (last_chunk < read_state_.size())) {
    file_lock.unlock();
    update_background_reader(0U);

    download_chunk(last_chunk, false, true);
    if (get_api_error() != api_error::success) {
      return get_api_error();
    }
    file_lock.lock();
  }

  const auto original_file_size = get_file_size();

  auto res = do_io([&]() -> api_error { return callback(nf_->get_handle()); });
  if (res != api_error::success) {
    utils::error::raise_api_path_error(__FUNCTION__, get_api_path(),
                                       utils::get_last_error_code(),
                                       "failed to allocate file");
    return res;
  }

  {
    std::uint64_t file_size{};
    if (not nf_->get_file_size(file_size)) {
      utils::error::raise_api_path_error(__FUNCTION__, get_api_path(),
                                         utils::get_last_error_code(),
                                         "failed to get file size");
      return set_api_error(api_error::os_error);
    }

    if (file_size != new_file_size) {
      utils::error::raise_api_path_error(
          __FUNCTION__, get_api_path(), api_error::file_size_mismatch,
          "allocated file size mismatch|expected|" +
              std::to_string(new_file_size) + "|actual|" +
              std::to_string(file_size));
      return set_api_error(api_error::error);
    }
  }

  if (is_empty_file || (read_state_.size() != (last_chunk + 1U))) {
    read_state_.resize(is_empty_file ? 0U : last_chunk + 1U);

    if (not is_empty_file) {
      read_state_[last_chunk] = true;
    }

    last_chunk_size_ = static_cast<std::size_t>(
        new_file_size <= chunk_size_          ? new_file_size
        : (new_file_size % chunk_size_) == 0U ? chunk_size_
                                              : new_file_size % chunk_size_);
  }

  if (original_file_size != new_file_size) {
    set_modified();

    fsi_.size = new_file_size;
    const auto now = std::to_string(utils::get_file_time_now());
    res = provider_.set_item_meta(
        fsi_.api_path, {
                           {META_CHANGED, now},
                           {META_MODIFIED, now},
                           {META_SIZE, std::to_string(new_file_size)},
                           {META_WRITTEN, now},
                       });
    if (res != api_error::success) {
      utils::error::raise_api_path_error(__FUNCTION__, get_api_path(), res,
                                         "failed to set file meta");
      return set_api_error(res);
    }
  }

  return res;
}

auto file_manager::open_file::read(std::size_t read_size,
                                   std::uint64_t read_offset, data_buffer &data)
    -> api_error {
  if (fsi_.directory) {
    return api_error::invalid_operation;
  }

  read_size =
      utils::calculate_read_size(get_file_size(), read_size, read_offset);
  if (read_size == 0U) {
    return api_error::success;
  }

  const auto read_from_source = [this, &data, &read_offset,
                                 &read_size]() -> api_error {
    return do_io([this, &data, &read_offset, &read_size]() -> api_error {
      if (provider_.is_direct_only()) {
        return provider_.read_file_bytes(fsi_.api_path, read_size, read_offset,
                                         data, stop_requested_);
      }

      data.resize(read_size);
      std::size_t bytes_read{};
      return nf_->read_bytes(data.data(), read_size, read_offset, bytes_read)
                 ? api_error::success
                 : api_error::os_error;
    });
  };

  unique_recur_mutex_lock file_lock(file_mtx_);
  if (read_state_.all()) {
    reset_timeout();
    return read_from_source();
  }
  file_lock.unlock();

  const auto start_chunk_index =
      static_cast<std::size_t>(read_offset / chunk_size_);
  const auto end_chunk_index =
      static_cast<std::size_t>((read_size + read_offset) / chunk_size_);

  update_background_reader(start_chunk_index);

  download_range(start_chunk_index, end_chunk_index, true);
  if (get_api_error() != api_error::success) {
    return get_api_error();
  }

  file_lock.lock();
  return get_api_error() == api_error::success ? read_from_source()
                                               : get_api_error();
}

void file_manager::open_file::remove(std::uint64_t handle) {
  recur_mutex_lock file_lock(file_mtx_);
  open_file_base::remove(handle);
  if (modified_ && read_state_.all() &&
      (get_api_error() == api_error::success)) {
    mgr_.queue_upload(*this);
    modified_ = false;
  }

  if (removed_ && (get_open_file_count() == 0U)) {
    removed_ = false;
  }
}

auto file_manager::open_file::resize(std::uint64_t new_file_size) -> api_error {
  if (fsi_.directory) {
    return api_error::invalid_operation;
  }

  return native_operation(
      new_file_size, [this, &new_file_size](native_handle) -> api_error {
        return nf_->truncate(new_file_size) ? api_error::success
                                            : api_error::os_error;
      });
}

auto file_manager::open_file::close() -> bool {
  if (not fsi_.directory && not stop_requested_) {
    stop_requested_ = true;

    unique_mutex_lock reader_lock(io_thread_mtx_);
    io_thread_notify_.notify_all();
    reader_lock.unlock();

    if (reader_thread_) {
      reader_thread_->join();
      reader_thread_.reset();
    }

    if (open_file_base::close()) {
      {
        const auto err = get_api_error();
        if (err == api_error::success ||
            err == api_error::download_incomplete ||
            err == api_error::download_stopped) {
          if (modified_ && not read_state_.all()) {
            set_api_error(api_error::download_incomplete);
          } else if (not modified_ && (fsi_.size > 0U) &&
                     not read_state_.all()) {
            set_api_error(api_error::download_stopped);
          }
        }
      }

      nf_->close();
      nf_.reset();

      if (modified_ && (get_api_error() == api_error::success)) {
        mgr_.queue_upload(*this);
      } else if (modified_ &&
                 (get_api_error() == api_error::download_incomplete)) {
        mgr_.store_resume(*this);
      } else if (get_api_error() != api_error::success) {
        mgr_.remove_resume(get_api_path(), get_source_path());
        if (not utils::file::retry_delete_file(fsi_.source_path)) {
          utils::error::raise_api_path_error(
              __FUNCTION__, get_api_path(), fsi_.source_path,
              utils::get_last_error_code(), "failed to delete file");
        }

        auto parent = utils::path::remove_file_name(fsi_.source_path);
        fsi_.source_path =
            utils::path::combine(parent, {utils::create_uuid_string()});
        const auto res = provider_.set_item_meta(fsi_.api_path, META_SOURCE,
                                                 fsi_.source_path);
        if (res != api_error::success) {
          utils::error::raise_api_path_error(__FUNCTION__, get_api_path(),
                                             fsi_.source_path, res,
                                             "failed to set file meta");
        }
      }
    }

    return true;
  }

  return false;
}

void file_manager::open_file::set_modified() {
  if (not modified_) {
    modified_ = true;
    mgr_.store_resume(*this);
  }

  if (not removed_) {
    removed_ = true;
    mgr_.remove_upload(get_api_path());
  }
}

void file_manager::open_file::update_background_reader(std::size_t read_chunk) {
  recur_mutex_lock reader_lock(file_mtx_);
  read_chunk_index_ = read_chunk;

  if (not reader_thread_ && not stop_requested_) {
    reader_thread_ = std::make_unique<std::thread>([this]() {
      std::size_t next_chunk{};
      while (not stop_requested_) {
        unique_recur_mutex_lock file_lock(file_mtx_);
        if ((fsi_.size == 0U) || read_state_.all()) {
          file_lock.unlock();

          unique_mutex_lock io_lock(io_thread_mtx_);
          if (not stop_requested_ && io_thread_queue_.empty()) {
            io_thread_notify_.wait(io_lock);
          }
          io_thread_notify_.notify_all();
          io_lock.unlock();
        } else {
          do {
            next_chunk = read_chunk_index_ =
                ((read_chunk_index_ + 1U) >= read_state_.size())
                    ? 0U
                    : read_chunk_index_ + 1U;
          } while ((next_chunk != 0U) && (active_downloads_.find(next_chunk) !=
                                          active_downloads_.end()));

          file_lock.unlock();
          download_chunk(next_chunk, true, false);
        }
      }
    });
  }
}

auto file_manager::open_file::write(std::uint64_t write_offset,
                                    const data_buffer &data,
                                    std::size_t &bytes_written) -> api_error {
  bytes_written = 0U;

  if (fsi_.directory || provider_.is_direct_only()) {
    return api_error::invalid_operation;
  }

  if (data.empty()) {
    return api_error::success;
  }

  unique_recur_mutex_lock write_lock(file_mtx_);
  if (stop_requested_) {
    return api_error::download_stopped;
  }
  write_lock.unlock();

  const auto start_chunk_index =
      static_cast<std::size_t>(write_offset / chunk_size_);
  const auto end_chunk_index =
      static_cast<std::size_t>((write_offset + data.size()) / chunk_size_);

  update_background_reader(start_chunk_index);

  download_range(start_chunk_index,
                 std::min(read_state_.size() - 1U, end_chunk_index), true);
  if (get_api_error() != api_error::success) {
    return get_api_error();
  }

  write_lock.lock();
  if ((write_offset + data.size()) > fsi_.size) {
    auto res = resize(write_offset + data.size());
    if (res != api_error::success) {
      return res;
    }
  }

  auto res = do_io([&]() -> api_error {
    if (not nf_->write_bytes(data.data(), data.size(), write_offset,
                             bytes_written)) {
      return api_error::os_error;
    }

    reset_timeout();
    return api_error::success;
  });
  if (res != api_error::success) {
    return set_api_error(res);
  }

  const auto now = std::to_string(utils::get_file_time_now());
  res = provider_.set_item_meta(fsi_.api_path, {
                                                   {META_CHANGED, now},
                                                   {META_MODIFIED, now},
                                                   {META_WRITTEN, now},
                                               });
  if (res != api_error::success) {
    utils::error::raise_api_path_error(__FUNCTION__, get_api_path(), res,
                                       "failed to set file meta");
    return set_api_error(res);
  }

  set_modified();
  return api_error::success;
}
} // namespace repertory
