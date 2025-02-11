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
#include "file_manager/open_file.hpp"

#include "app_config.hpp"
#include "events/event_system.hpp"
#include "events/types/download_begin.hpp"
#include "events/types/download_end.hpp"
#include "events/types/download_progress.hpp"
#include "file_manager/cache_size_mgr.hpp"
#include "file_manager/file_manager.hpp"
#include "file_manager/i_upload_manager.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "utils/common.hpp"
#include "utils/error_utils.hpp"
#include "utils/path.hpp"
#include "utils/time.hpp"

namespace repertory {
open_file::open_file(std::uint64_t chunk_size, std::uint8_t chunk_timeout,
                     filesystem_item fsi, i_provider &provider,
                     i_upload_manager &mgr)
    : open_file(chunk_size, chunk_timeout, fsi, {}, provider, std::nullopt,
                mgr) {}

open_file::open_file(std::uint64_t chunk_size, std::uint8_t chunk_timeout,
                     filesystem_item fsi,
                     std::map<std::uint64_t, open_file_data> open_data,
                     i_provider &provider, i_upload_manager &mgr)
    : open_file(chunk_size, chunk_timeout, fsi, open_data, provider,
                std::nullopt, mgr) {}

open_file::open_file(std::uint64_t chunk_size, std::uint8_t chunk_timeout,
                     filesystem_item fsi, i_provider &provider,
                     std::optional<boost::dynamic_bitset<>> read_state,
                     i_upload_manager &mgr)
    : open_file(chunk_size, chunk_timeout, fsi, {}, provider, read_state, mgr) {
}

open_file::open_file(std::uint64_t chunk_size, std::uint8_t chunk_timeout,
                     filesystem_item fsi,
                     std::map<std::uint64_t, open_file_data> open_data,
                     i_provider &provider,
                     std::optional<boost::dynamic_bitset<>> read_state,
                     i_upload_manager &mgr)
    : open_file_base(chunk_size, chunk_timeout, fsi, open_data, provider,
                     false),
      mgr_(mgr) {
  REPERTORY_USES_FUNCTION_NAME();

  if (fsi.directory) {
    if (read_state.has_value()) {
      utils::error::raise_api_path_error(
          function_name, fsi.api_path, fsi.source_path,
          fmt::format("cannot resume a directory|sp|", fsi.api_path));
    }

    return;
  }

  nf_ = utils::file::file::open_or_create_file(fsi.source_path,
                                               get_provider().is_read_only());
  set_api_error(*nf_ ? api_error::success : api_error::os_error);
  if (get_api_error() != api_error::success) {
    return;
  }

  if (read_state.has_value()) {
    read_state_ = read_state.value();
    set_modified();
    allocated = true;
    return;
  }

  if (fsi.size == 0U) {
    return;
  }

  read_state_.resize(static_cast<std::size_t>(
                         utils::divide_with_ceiling(fsi.size, chunk_size)),
                     false);

  auto file_size = nf_->size();
  if (not file_size.has_value()) {
    utils::error::raise_api_path_error(
        function_name, fsi.api_path, fsi.source_path,
        utils::get_last_error_code(), "failed to get file size");
    set_api_error(api_error::os_error);
    return;
  }

  if (get_provider().is_read_only() || file_size.value() == fsi.size) {
    read_state_.set(0U, read_state_.size(), true);
    allocated = true;
  }

  if (get_api_error() != api_error::success && *nf_) {
    nf_->close();
  }
}

open_file::~open_file() { close(); }

auto open_file::adjust_cache_size(std::uint64_t file_size,
                                  bool shrink) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  if (file_size == get_file_size()) {
    return api_error::success;
  }

  if (file_size > get_file_size()) {
    auto size = file_size - get_file_size();
    auto res = shrink ? cache_size_mgr::instance().shrink(size)
                      : cache_size_mgr::instance().expand(size);
    if (res == api_error::success) {
      return res;
    }

    utils::error::raise_api_path_error(
        function_name, get_api_path(), get_source_path(), res,
        fmt::format("failed to {} cache|size|{}",
                    (shrink ? "shrink" : "expand"), size));
    return set_api_error(res);
  }

  auto size = get_file_size() - file_size;
  auto res = shrink ? cache_size_mgr::instance().expand(size)
                    : cache_size_mgr::instance().shrink(size);
  if (res == api_error::success) {
    return res;
  }

  utils::error::raise_api_path_error(
      function_name, get_api_path(), get_source_path(), res,
      fmt::format("failed to {} cache|size|{}", (shrink ? "expand" : "shrink"),
                  size));
  return set_api_error(res);
}

auto open_file::check_start() -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  unique_recur_mutex_lock file_lock(get_mutex());
  if (allocated) {
    return api_error::success;
  }

  auto file_size = nf_->size();
  if (not file_size.has_value()) {
    utils::error::raise_api_path_error(
        function_name, get_api_path(), get_source_path(),
        utils::get_last_error_code(), "failed to get file size");
    return set_api_error(api_error::os_error);
  }

  if (file_size.value() == get_file_size()) {
    allocated = true;
    return api_error::success;
  }

  file_lock.unlock();
  auto res = adjust_cache_size(file_size.value(), true);
  if (res != api_error::success) {
    return res;
  }
  file_lock.lock();

  if (not nf_->truncate(get_file_size())) {
    utils::error::raise_api_path_error(
        function_name, get_api_path(), get_source_path(),
        utils::get_last_error_code(),
        fmt::format("failed to truncate file|size|{}", get_file_size()));
    return set_api_error(res);
  }

  allocated = true;
  return api_error::success;
}

auto open_file::close() -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  if (is_directory() || stop_requested_) {
    return false;
  }

  stop_requested_ = true;

  notify_io();

  if (reader_thread_) {
    reader_thread_->join();
    reader_thread_.reset();
  }

  if (not open_file_base::close()) {
    return false;
  }

  auto read_state = get_read_state();
  auto err = get_api_error();
  if (err == api_error::success || err == api_error::download_incomplete ||
      err == api_error::download_stopped) {
    if (is_modified() && not read_state.all()) {
      set_api_error(api_error::download_incomplete);
    } else if (not is_modified() && (get_file_size() > 0U) &&
               not read_state.all()) {
      set_api_error(api_error::download_stopped);
    }

    err = get_api_error();
  }

  nf_->close();

  if (is_modified()) {
    if (err == api_error::success) {
      mgr_.queue_upload(*this);
      return true;
    }

    if (err == api_error::download_incomplete) {
      mgr_.store_resume(*this);
      return true;
    }
  }

  if (err != api_error::success || read_state.all()) {
    mgr_.remove_resume(get_api_path(), get_source_path());
  }

  if (err == api_error::success) {
    return true;
  }

  file_manager::remove_source_and_shrink_cache(
      get_api_path(), get_source_path(), get_file_size(), allocated);

  auto parent = utils::path::get_parent_path(get_source_path());
  set_source_path(utils::path::combine(parent, {utils::create_uuid_string()}));

  auto res = get_provider().set_item_meta(get_api_path(), META_SOURCE,
                                          get_source_path());
  if (res != api_error::success) {
    utils::error::raise_api_path_error(function_name, get_api_path(),
                                       get_source_path(), res,
                                       "failed to set new source path");
  }

  return true;
}

void open_file::download_chunk(std::size_t chunk, bool skip_active,
                               bool should_reset) {
  REPERTORY_USES_FUNCTION_NAME();

  if (should_reset) {
    reset_timeout();
  }

  unique_recur_mutex_lock rw_lock(rw_mtx_);
  auto read_state = get_read_state();
  if ((get_api_error() == api_error::success) && (chunk < read_state.size()) &&
      not read_state[chunk]) {
    if (get_active_downloads().find(chunk) != get_active_downloads().end()) {
      if (skip_active) {
        return;
      }

      auto active_download = get_active_downloads().at(chunk);
      rw_lock.unlock();

      active_download->wait();
      return;
    }

    auto data_offset = chunk * get_chunk_size();
    auto data_size = (chunk == read_state.size() - 1U) ? get_last_chunk_size()
                                                       : get_chunk_size();
    if (get_active_downloads().empty() && (read_state.count() == 0U)) {
      event_system::instance().raise<download_begin>(
          get_api_path(), get_source_path(), function_name);
    }

    get_active_downloads()[chunk] = std::make_shared<download>();
    rw_lock.unlock();

    if (should_reset) {
      reset_timeout();
    }

    std::async(std::launch::async, [this, chunk, data_size, data_offset,
                                    should_reset]() {
      const auto notify_complete = [this, chunk, should_reset]() {
        auto state = get_read_state();

        unique_recur_mutex_lock lock(rw_mtx_);
        auto active_download = get_active_downloads().at(chunk);
        get_active_downloads().erase(chunk);
        if (get_api_error() == api_error::success) {
          auto progress = (static_cast<double>(state.count()) /
                           static_cast<double>(state.size())) *
                          100.0;
          event_system::instance().raise<download_progress>(
              get_api_path(), get_source_path(), function_name, progress);
          if (state.all() && not notified_) {
            notified_ = true;
            event_system::instance().raise<download_end>(
                get_api_path(), get_source_path(), get_api_error(),
                function_name);
          }
        } else if (not notified_) {
          notified_ = true;
          event_system::instance().raise<download_end>(
              get_api_path(), get_source_path(), get_api_error(),
              function_name);
        }
        lock.unlock();

        active_download->notify(get_api_error());

        if (should_reset) {
          reset_timeout();
        }
      };

      data_buffer buffer;
      auto res = get_provider().read_file_bytes(
          get_api_path(), data_size, data_offset, buffer, stop_requested_);
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
        if (not nf_->write(buffer, data_offset, &bytes_written)) {
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

      set_read_state(chunk);

      notify_complete();
    }).wait();
  }
}

void open_file::download_range(std::size_t begin_chunk, std::size_t end_chunk,
                               bool should_reset) {
  for (std::size_t chunk = begin_chunk;
       (get_api_error() == api_error::success) && (chunk <= end_chunk);
       ++chunk) {
    download_chunk(chunk, false, should_reset);
  }
}

auto open_file::get_allocated() const -> bool {
  recur_mutex_lock file_lock(get_mutex());
  return allocated;
}

auto open_file::get_read_state() const -> boost::dynamic_bitset<> {
  recur_mutex_lock file_lock(get_mutex());
  return read_state_;
}

auto open_file::get_read_state(std::size_t chunk) const -> bool {
  return get_read_state()[chunk];
}

auto open_file::get_stop_requested() const -> bool {
  return stop_requested_ || app_config::get_stop_requested();
}

auto open_file::is_complete() const -> bool { return get_read_state().all(); }

auto open_file::native_operation(
    i_open_file::native_operation_callback callback) -> api_error {
  if (get_stop_requested()) {
    return set_api_error(api_error::download_stopped);
  }

  auto res = check_start();
  if (res != api_error::success) {
    return res;
  }

  unique_recur_mutex_lock rw_lock(rw_mtx_);
  return do_io([&]() -> api_error { return callback(nf_->get_handle()); });
}

auto open_file::native_operation(
    std::uint64_t new_file_size,
    i_open_file::native_operation_callback callback) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  if (is_directory()) {
    return set_api_error(api_error::invalid_operation);
  }

  if (get_stop_requested()) {
    return set_api_error(api_error::download_stopped);
  }

  auto res = check_start();
  if (res != api_error::success) {
    return res;
  }

  res = adjust_cache_size(new_file_size, false);
  if (res != api_error::success) {
    return res;
  }

  auto is_empty_file = new_file_size == 0U;
  auto last_chunk = is_empty_file
                        ? std::size_t(0U)
                        : static_cast<std::size_t>(utils::divide_with_ceiling(
                              new_file_size, get_chunk_size())) -
                              1U;

  unique_recur_mutex_lock rw_lock(rw_mtx_);
  auto read_state = get_read_state();
  if (not is_empty_file && (last_chunk < read_state.size())) {
    rw_lock.unlock();
    update_reader(0U);

    download_chunk(last_chunk, false, true);
    if (get_api_error() != api_error::success) {
      return get_api_error();
    }
    rw_lock.lock();
  }

  read_state = get_read_state();
  auto original_file_size = get_file_size();

  res = do_io([&]() -> api_error { return callback(nf_->get_handle()); });
  if (res != api_error::success) {
    utils::error::raise_api_path_error(function_name, get_api_path(),
                                       utils::get_last_error_code(),
                                       "failed to allocate file");
    return res;
  }

  {
    auto file_size = nf_->size();
    if (not file_size.has_value()) {
      utils::error::raise_api_path_error(
          function_name, get_api_path(), api_error::file_size_mismatch,
          fmt::format("failed to get file size|error|{}",
                      utils::get_last_error_code()));
      return set_api_error(api_error::error);
    }

    if (file_size.value() != new_file_size) {
      utils::error::raise_api_path_error(
          function_name, get_api_path(), api_error::file_size_mismatch,
          fmt::format("file size mismatch|expected|{}|actual|{}", new_file_size,
                      file_size.value()));
      return set_api_error(api_error::error);
    }
  }

  if (is_empty_file || (read_state.size() != (last_chunk + 1U))) {
    auto old_size = read_state.size();
    read_state.resize(is_empty_file ? 0U : last_chunk + 1U);

    if (not is_empty_file) {
      for (std::size_t chunk = old_size; chunk <= last_chunk; ++chunk) {
        read_state.set(chunk);
      }
    }
    set_read_state(read_state);

    set_last_chunk_size(static_cast<std::size_t>(
        new_file_size <= get_chunk_size() ? new_file_size
        : (new_file_size % get_chunk_size()) == 0U
            ? get_chunk_size()
            : new_file_size % get_chunk_size()));
  }

  if (original_file_size == new_file_size) {
    return res;
  }
  set_modified();

  set_file_size(new_file_size);
  auto now = std::to_string(utils::time::get_time_now());
  res = get_provider().set_item_meta(
      get_api_path(), {
                          {META_CHANGED, now},
                          {META_MODIFIED, now},
                          {META_SIZE, std::to_string(new_file_size)},
                          {META_WRITTEN, now},
                      });
  if (res == api_error::success) {
    return res;
  }

  utils::error::raise_api_path_error(function_name, get_api_path(), res,
                                     "failed to set file meta");
  return set_api_error(res);
}

auto open_file::read(std::size_t read_size, std::uint64_t read_offset,
                     data_buffer &data) -> api_error {
  if (is_directory()) {
    return set_api_error(api_error::invalid_operation);
  }

  if (get_stop_requested()) {
    return set_api_error(api_error::download_stopped);
  }

  read_size =
      utils::calculate_read_size(get_file_size(), read_size, read_offset);
  if (read_size == 0U) {
    return api_error::success;
  }

  auto res = check_start();
  if (res != api_error::success) {
    return res;
  }

  const auto read_from_source = [this, &data, &read_offset,
                                 &read_size]() -> api_error {
    return do_io([this, &data, &read_offset, &read_size]() -> api_error {
      if (get_provider().is_read_only()) {
        return get_provider().read_file_bytes(
            get_api_path(), read_size, read_offset, data, stop_requested_);
      }

      data.resize(read_size);
      std::size_t bytes_read{};
      return nf_->read(data.data(), read_size, read_offset, &bytes_read)
                 ? api_error::success
                 : api_error::os_error;
    });
  };

  if (get_read_state().all()) {
    reset_timeout();
    return read_from_source();
  }

  auto begin_chunk = static_cast<std::size_t>(read_offset / get_chunk_size());
  auto end_chunk =
      static_cast<std::size_t>((read_size + read_offset) / get_chunk_size());

  update_reader(begin_chunk);

  download_range(begin_chunk, end_chunk, true);
  if (get_api_error() != api_error::success) {
    return get_api_error();
  }

  unique_recur_mutex_lock rw_lock(rw_mtx_);
  return get_api_error() == api_error::success ? read_from_source()
                                               : get_api_error();
}

void open_file::remove(std::uint64_t handle) {
  open_file_base::remove(handle);

  recur_mutex_lock rw_lock(rw_mtx_);
  if (is_modified() && get_read_state().all() &&
      (get_api_error() == api_error::success)) {
    mgr_.queue_upload(*this);
    open_file_base::set_modified(false);
  }

  if (is_removed() && (get_open_file_count() == 0U)) {
    open_file_base::set_removed(false);
  }
}

void open_file::remove_all() {
  open_file_base::remove_all();

  recur_mutex_lock rw_lock(rw_mtx_);
  open_file_base::set_modified(false);
  open_file_base::set_removed(true);

  mgr_.remove_upload(get_api_path());

  set_api_error(api_error::success);
}

auto open_file::resize(std::uint64_t new_file_size) -> api_error {
  if (is_directory()) {
    return set_api_error(api_error::invalid_operation);
  }

  if (new_file_size == get_file_size()) {
    return api_error::success;
  }

  return native_operation(
      new_file_size, [this, &new_file_size](native_handle) -> api_error {
        return nf_->truncate(new_file_size) ? api_error::success
                                            : api_error::os_error;
      });
}

void open_file::set_modified() {
  if (not is_modified()) {
    open_file_base::set_modified(true);
    mgr_.store_resume(*this);
  }

  if (not is_removed()) {
    open_file_base::set_removed(true);
    mgr_.remove_upload(get_api_path());
  }
}

void open_file::set_read_state(std::size_t chunk) {
  recur_mutex_lock file_lock(get_mutex());
  read_state_.set(chunk);
}

void open_file::set_read_state(boost::dynamic_bitset<> read_state) {
  recur_mutex_lock file_lock(get_mutex());
  read_state_ = std::move(read_state);
}

void open_file::update_reader(std::size_t chunk) {
  recur_mutex_lock rw_lock(rw_mtx_);
  read_chunk_ = chunk;

  if (reader_thread_ || get_stop_requested()) {
    return;
  }

  reader_thread_ = std::make_unique<std::thread>([this]() {
    unique_recur_mutex_lock lock(rw_mtx_);
    auto next_chunk{read_chunk_};
    auto read_chunk{read_chunk_};
    lock.unlock();

    while (not get_stop_requested()) {
      lock.lock();

      auto read_state = get_read_state();
      if ((get_file_size() == 0U) || read_state.all()) {
        lock.unlock();
        wait_for_io([this]() -> bool { return this->get_stop_requested(); });
        continue;
      }

      if (read_chunk != read_chunk_) {
        next_chunk = read_chunk = read_chunk_;
      }

      next_chunk = next_chunk + 1U >= read_state.size() ? 0U : next_chunk + 1U;
      lock.unlock();

      download_chunk(next_chunk, true, false);
    }
  });
}

auto open_file::write(std::uint64_t write_offset, const data_buffer &data,
                      std::size_t &bytes_written) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  bytes_written = 0U;

  if (is_directory() || get_provider().is_read_only()) {
    return set_api_error(api_error::invalid_operation);
  }

  if (data.empty()) {
    return api_error::success;
  }

  if (get_stop_requested()) {
    return set_api_error(api_error::download_stopped);
  }

  auto res = check_start();
  if (res != api_error::success) {
    return res;
  }

  auto begin_chunk = static_cast<std::size_t>(write_offset / get_chunk_size());
  auto end_chunk =
      static_cast<std::size_t>((write_offset + data.size()) / get_chunk_size());

  update_reader(begin_chunk);

  download_range(begin_chunk, std::min(get_read_state().size() - 1U, end_chunk),
                 true);
  if (get_api_error() != api_error::success) {
    return get_api_error();
  }

  unique_recur_mutex_lock rw_lock(rw_mtx_);
  if ((write_offset + data.size()) > get_file_size()) {
    res = resize(write_offset + data.size());
    if (res != api_error::success) {
      return res;
    }
  }

  res = do_io([&]() -> api_error {
    if (not nf_->write(data, write_offset, &bytes_written)) {
      return api_error::os_error;
    }

    reset_timeout();
    return api_error::success;
  });
  if (res != api_error::success) {
    return set_api_error(res);
  }

  auto now = std::to_string(utils::time::get_time_now());
  res = get_provider().set_item_meta(get_api_path(), {
                                                         {META_CHANGED, now},
                                                         {META_MODIFIED, now},
                                                         {META_WRITTEN, now},
                                                     });
  if (res != api_error::success) {
    utils::error::raise_api_path_error(function_name, get_api_path(), res,
                                       "failed to set file meta");
    return set_api_error(res);
  }

  set_modified();
  return api_error::success;
}
} // namespace repertory
