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
#include "download/download_manager.hpp"
#include "download/direct_download.hpp"
#include "download/download.hpp"
#include "download/events.hpp"
#include "download/ring_download.hpp"
#include "drives/i_open_file_table.hpp"
#include "events/events.hpp"
#include "types/repertory.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/file_utils.hpp"
#include "utils/global_data.hpp"
#include "utils/path_utils.hpp"
#include "utils/rocksdb_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
static json create_resume_entry(i_download &d) {
  filesystem_item fsi;
  std::size_t chunk_size;
  std::size_t last_chunk_size;
  boost::dynamic_bitset<> read_state, write_state;
  d.get_state_information(fsi, chunk_size, last_chunk_size, read_state, write_state);

  return {
      {"chunk_size", chunk_size},
      {"encryption_token", fsi.encryption_token},
      {"last_chunk_size", last_chunk_size},
      {"path", fsi.api_path},
      {"read_state", utils::string::from_dynamic_bitset(read_state)},
      {"source", fsi.source_path},
      {"write_state", utils::string::from_dynamic_bitset(write_state)},
  };
}

static void restore_resume_entry(const json &resume_entry, filesystem_item &fsi,
                                 std::size_t &chunk_size, std::size_t &last_chunk_size,
                                 boost::dynamic_bitset<> &read_state,
                                 boost::dynamic_bitset<> &write_state) {
  chunk_size = resume_entry["chunk_size"].get<std::size_t>();
  fsi.encryption_token = resume_entry["encryption_token"].get<std::string>();
  last_chunk_size = resume_entry["last_chunk_size"].get<std::size_t>();
  fsi.api_path = resume_entry["path"].get<std::string>();
  read_state = utils::string::to_dynamic_bitset(resume_entry["read_state"].get<std::string>());
  fsi.source_path = resume_entry["source"].get<std::string>();
  write_state = utils::string::to_dynamic_bitset(resume_entry["write_state"].get<std::string>());
  fsi.api_path = utils::path::get_parent_api_path(fsi.api_path);
}

static void create_source_file_if_empty(filesystem_item &fsi, const app_config &config) {
  unique_recur_mutex_lock item_lock(*fsi.lock);
  if (fsi.source_path.empty()) {
    fsi.source_path =
        utils::path::combine(config.get_cache_directory(), {utils::create_uuid_string()});
    fsi.source_path_changed = true;
  }
  item_lock.unlock();
}

download_manager::download_manager(const app_config &config, api_reader_callback api_reader,
                                   const bool &force_download)
    : config_(config), api_reader_(api_reader), force_download_(force_download) {
  utils::db::create_rocksdb(config, "resume_db", restore_db_);

  E_SUBSCRIBE_EXACT(download_end, [this](const download_end &downloadEnd) {
    this->handle_download_end(downloadEnd);
  });

  E_SUBSCRIBE_EXACT(filesystem_item_handle_closed,
                    [this](const filesystem_item_handle_closed &fileHandleClosed) {
                      this->on_handle_closed(fileHandleClosed);
                    });

  E_SUBSCRIBE_EXACT(filesystem_item_closed, [this](const filesystem_item_closed &fileClosed) {
    reset_timeout(fileClosed.get_api_path(), true);
  });

  E_SUBSCRIBE_EXACT(filesystem_item_opened, [this](const filesystem_item_opened &fileOpened) {
    reset_timeout(fileOpened.get_api_path(), false);
  });
}

download_manager::~download_manager() {
  E_CONSUMER_RELEASE();
  stop();
}

api_error download_manager::allocate(const std::uint64_t &handle, filesystem_item &fsi,
                                     const std::uint64_t &size,
                                     const i_download::allocator_callback &allocator) {
  create_source_file_if_empty(fsi, config_);

  if (stop_requested_) {
    return api_error::download_stopped;
  }

  const auto completer = [&](std::uint64_t old_size, std::uint64_t new_size, bool changed) {
    fsi.changed = changed;
    if (old_size != new_size) {
      fsi.size = new_size;
      global_data::instance().update_used_space(old_size, new_size, false);
    }

    const auto modified = std::to_string(utils::get_file_time_now());
    oft_->set_item_meta(fsi.api_path, META_MODIFIED, modified);
    oft_->set_item_meta(fsi.api_path, META_WRITTEN, modified);
  };

  if (utils::file::is_file(fsi.source_path)) {
    std::uint64_t file_size = 0u;
    if (utils::file::get_file_size(fsi.source_path, file_size)) {
      if (fsi.size == file_size) {
        native_file_ptr nf;
        auto ret = utils::file::assign_and_get_native_file(fsi, nf);
        if (ret != api_error::success) {
          return ret;
        }

        if ((ret = allocator()) == api_error::success) {
          std::uint64_t new_file_size{};
          utils::file::get_file_size(fsi.source_path, new_file_size);
          completer(fsi.size, new_file_size, true);
        }

        return ret;
      }
    } else {
      return api_error::os_error;
    }
  }

  auto d = get_download(handle, fsi, true);
  const auto result = d->get_result();
  return ((result == api_error::success) ? d->allocate(handle, size, allocator, completer)
                                         : result);
}

bool download_manager::contains_handle(const std::string &api_path,
                                       const std::uint64_t &handle) const {
  return ((download_lookup_.find(api_path) != download_lookup_.end()) &&
          (download_lookup_.at(api_path).find(handle) != download_lookup_.at(api_path).end()));
}

bool download_manager::contains_restore(const std::string &api_path) const {
  std::string value;
  restore_db_->Get(rocksdb::ReadOptions(), api_path, &value);
  return not value.empty();
}

api_error download_manager::download_file(const std::uint64_t &handle, filesystem_item &fsi) {
  create_source_file_if_empty(fsi, config_);

  if (stop_requested_) {
    return api_error::download_stopped;
  } else if (fsi.size > 0u) {
    std::uint64_t file_size = 0u;
    if (utils::file::get_file_size(fsi.source_path, file_size)) {
      if (file_size == fsi.size) {
        native_file_ptr nf;
        return utils::file::assign_and_get_native_file(fsi, nf);
      }
    }

    auto d = get_download(handle, fsi, true);
    return ((d->get_result() == api_error::success) ? d->download_all() : d->get_result());
  } else if (fsi.handle == REPERTORY_INVALID_HANDLE) {
    native_file_ptr nf;
    return utils::file::assign_and_get_native_file(fsi, nf);
  } else {
    return api_error::success;
  }
}

download_ptr download_manager::get_download(std::uint64_t handle, filesystem_item &fsi,
                                            const bool &write_supported) {
  const auto create_download = [this, &handle, &fsi, &write_supported]() -> download_ptr {
    const auto chunk_size = utils::encryption::encrypting_reader::get_data_chunk_size();
    if (write_supported || force_download_) {
      handle = 0u;
      return std::dynamic_pointer_cast<i_download>(
          std::make_shared<download>(config_, fsi, api_reader_, chunk_size, *oft_));
    } else {
      const auto dt = config_.get_preferred_download_type();
      const std::size_t ring_buffer_size = config_.get_ring_buffer_file_size() * 1024ull * 1024ull;
      const auto ring_allowed = (fsi.size > ring_buffer_size);

      const auto no_space_remaining =
          (utils::file::get_available_drive_space(config_.get_cache_directory()) < fsi.size);
      if (ring_allowed && (dt != download_type::direct) &&
          (no_space_remaining || (dt == download_type::ring_buffer))) {
        return std::dynamic_pointer_cast<i_download>(std::make_shared<ring_download>(
            config_, fsi, api_reader_, handle, chunk_size, ring_buffer_size));
      }

      if (no_space_remaining || (dt == download_type::direct)) {
        return std::dynamic_pointer_cast<i_download>(
            std::make_shared<direct_download>(config_, fsi, api_reader_, handle));
      } else {
        handle = 0u;
        return std::dynamic_pointer_cast<i_download>(
            std::make_shared<download>(config_, fsi, api_reader_, chunk_size, *oft_));
      }
    }
  };

  download_ptr d;
  {
    recur_mutex_lock download_lock(download_mutex_);
    if (contains_handle(fsi.api_path, 0u)) {
      d = download_lookup_[fsi.api_path][0u];
    } else if (contains_handle(fsi.api_path, handle)) {
      d = download_lookup_[fsi.api_path][handle];
    } else {
      d = create_download();
      download_lookup_[fsi.api_path][handle] = d;
    }
    if (write_supported && not d->get_write_supported()) {
      for (auto &kv : download_lookup_[fsi.api_path]) {
        kv.second->set_disable_download_end(true);
      }
      d = create_download();
      download_lookup_[fsi.api_path] = {{0u, d}};
    }
  }

  return d;
}

std::string download_manager::get_source_path(const std::string &api_path) const {
  recur_mutex_lock download_lock(download_mutex_);
  if (contains_handle(api_path, 0u)) {
    return download_lookup_.at(api_path).at(0u)->get_source_path();
  }
  return "";
}

void download_manager::handle_download_end(const download_end &de) {
  download_ptr d;
  const auto api_path = de.get_api_path();
  const auto handle = utils::string::to_uint64(de.get_handle());
  unique_recur_mutex_lock download_lock(download_mutex_);
  if (contains_handle(api_path, handle)) {
    d = download_lookup_[api_path][handle];
  }
  download_lock.unlock();
  if (d) {
    download_lock.lock();
    download_lookup_[api_path].erase(handle);
    if (download_lookup_[api_path].empty()) {
      download_lookup_.erase(api_path);
    }

    if (d->get_result() == api_error::download_incomplete) {
      const auto res = restore_db_->Put(rocksdb::WriteOptions(), api_path.get<std::string>(),
                                        create_resume_entry(*d).dump());
      if (res.ok()) {
        event_system::instance().raise<download_stored>(api_path, d->get_source_path());
      } else {
        event_system::instance().raise<download_store_failed>(api_path.get<std::string>(),
                                                              d->get_source_path(), res.ToString());
      }
    }

    download_lock.unlock();
  }
}

void download_manager::on_handle_closed(const filesystem_item_handle_closed &handle_closed) {
  download_ptr d;
  const auto api_path = handle_closed.get_api_path();
  const auto handle = utils::string::to_uint64(handle_closed.get_handle());
  unique_recur_mutex_lock download_lock(download_mutex_);
  if (contains_handle(api_path, handle)) {
    d = download_lookup_[api_path][handle];
  }
  download_lock.unlock();
  if (d) {
    d->notify_stop_requested();
  }
}

bool download_manager::is_processing(const std::string &api_path) const {
  recur_mutex_lock download_lock(download_mutex_);
  return download_lookup_.find(api_path) != download_lookup_.end();
}

bool download_manager::pause_download(const std::string &api_path) {
  auto ret = true;

  recur_mutex_lock download_lock(download_mutex_);
  if (download_lookup_.find(api_path) != download_lookup_.end()) {
    ret = false;
    if (download_lookup_[api_path].find(0u) != download_lookup_[api_path].end()) {
      if ((ret = (download_lookup_[api_path].size() == 1))) {
        ret = download_lookup_[api_path][0u]->pause();
      }
    }
  }

  return ret;
}

api_error download_manager::read_bytes(const std::uint64_t &handle, filesystem_item &fsi,
                                       std::size_t read_size, const std::uint64_t &read_offset,
                                       std::vector<char> &data) {
  create_source_file_if_empty(fsi, config_);

  if (stop_requested_) {
    return api_error::download_stopped;
  }

  read_size = utils::calculate_read_size(fsi.size, read_size, read_offset);
  if (read_size == 0u) {
    return api_error::success;
  }

  if (utils::file::is_file(fsi.source_path)) {
    std::uint64_t file_size = 0u;
    if (utils::file::get_file_size(fsi.source_path, file_size)) {
      if ((read_offset + read_size) <= file_size) {
        return utils::file::read_from_source(fsi, read_size, read_offset, data);
      }
    } else {
      return api_error::os_error;
    }
  }

  auto d = get_download(handle, fsi, false);
  const auto result = d->get_result();
  return ((result == api_error::success) ? d->read_bytes(handle, read_size, read_offset, data)
                                         : result);
}

void download_manager::rename_download(const std::string &from_api_path,
                                       const std::string &to_api_path) {
  recur_mutex_lock download_lock(download_mutex_);
  if (download_lookup_.find(from_api_path) != download_lookup_.end()) {
    if (download_lookup_[from_api_path].find(0u) != download_lookup_[from_api_path].end()) {
      download_lookup_[from_api_path][0u]->set_api_path(to_api_path);
      download_lookup_[to_api_path][0u] = download_lookup_[from_api_path][0u];
      download_lookup_.erase(from_api_path);
    }
  }
}

void download_manager::reset_timeout(const std::string &api_path, const bool &file_closed) {
  download_ptr d;
  unique_recur_mutex_lock download_lock(download_mutex_);
  if (contains_handle(api_path, 0u)) {
    d = download_lookup_[api_path][0u];
  }
  download_lock.unlock();
  if (d) {
    d->reset_timeout(file_closed);
  }
}

api_error download_manager::resize(const std::uint64_t &handle, filesystem_item &fsi,
                                   const std::uint64_t &size) {
  return allocate(handle, fsi, size,
                  [&]() -> api_error { return utils::file::truncate_source(fsi, size); });
}

void download_manager::resume_download(const std::string &api_path) {
  recur_mutex_lock download_lock(download_mutex_);
  if (download_lookup_.find(api_path) != download_lookup_.end()) {
    if (download_lookup_[api_path].find(0u) != download_lookup_[api_path].end()) {
      download_lookup_[api_path][0u]->resume();
    }
  }
}

void download_manager::start(i_open_file_table *oft) {
  recur_mutex_lock start_stop_lock(start_stop_mutex_);
  if (stop_requested_) {
    stop_requested_ = false;
    oft_ = oft;
    start_incomplete();
  }
}

void download_manager::start_incomplete() {
  auto iterator =
      std::shared_ptr<rocksdb::Iterator>(restore_db_->NewIterator(rocksdb::ReadOptions()));
  for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
    filesystem_item fsi{};
    fsi.lock = std::make_shared<std::recursive_mutex>();

    std::size_t chunk_size, last_chunk_size;
    boost::dynamic_bitset<> readState, writeState;
    restore_resume_entry(json::parse(iterator->value().ToString()), fsi, chunk_size,
                         last_chunk_size, readState, writeState);
    if (utils::file::get_file_size(fsi.source_path, fsi.size)) {
      download_lookup_[fsi.api_path][0u] = std::make_shared<download>(
          config_, fsi, api_reader_, chunk_size, last_chunk_size, readState, writeState, *oft_);
      event_system::instance().raise<download_restored>(fsi.api_path, fsi.source_path);
    } else {
      event_system::instance().raise<download_restore_failed>(
          fsi.api_path, fsi.source_path,
          "failed to get file size: " + utils::string::from_uint64(utils::get_last_error_code()));
    }
    restore_db_->Delete(rocksdb::WriteOptions(), iterator->key());
  }
}

void download_manager::stop() {
  unique_recur_mutex_lock start_stop_lock(start_stop_mutex_);
  if (not stop_requested_) {
    event_system::instance().raise<service_shutdown>("download_manager");
    stop_requested_ = true;
    start_stop_lock.unlock();

    std::vector<download_ptr> downloads;
    unique_recur_mutex_lock download_lock(download_mutex_);
    for (auto &d : download_lookup_) {
      for (auto &kv : d.second) {
        auto download = kv.second;
        if (download) {
          downloads.emplace_back(download);
        }
      }
    }
    download_lock.unlock();

    for (auto &d : downloads) {
      d->notify_stop_requested();
    }

    while (not download_lookup_.empty()) {
      std::this_thread::sleep_for(1ms);
    }
  }
}

api_error download_manager::write_bytes(const std::uint64_t &handle, filesystem_item &fsi,
                                        const std::uint64_t &write_offset, std::vector<char> data,
                                        std::size_t &bytes_written) {
  bytes_written = 0u;
  create_source_file_if_empty(fsi, config_);

  if (stop_requested_) {
    return api_error::download_stopped;
  }

  if (data.empty()) {
    return api_error::success;
  }

  const auto completer = [&](std::uint64_t old_size, std::uint64_t new_size, bool changed) {
    fsi.changed = changed;
    if (old_size != new_size) {
      fsi.size = new_size;
      global_data::instance().update_used_space(old_size, new_size, false);
    }

    const auto modified = std::to_string(utils::get_file_time_now());
    oft_->set_item_meta(fsi.api_path, META_MODIFIED, modified);
    oft_->set_item_meta(fsi.api_path, META_WRITTEN, modified);
  };

  if (utils::file::is_file(fsi.source_path)) {
    std::uint64_t file_size = 0u;
    if (utils::file::get_file_size(fsi.source_path, file_size)) {
      if (fsi.size == file_size) {
        auto ret = utils::file::write_to_source(fsi, write_offset, data, bytes_written);
        if (ret == api_error::success) {
          std::uint64_t new_size{};
          utils::file::get_file_size(fsi.source_path, new_size);
          completer(fsi.size, new_size, true);
        }

        return ret;
      }
    } else {
      return api_error::os_error;
    }
  }

  auto d = get_download(handle, fsi, true);
  const auto result = d->get_result();
  return ((result == api_error::success)
              ? d->write_bytes(handle, write_offset, std::move(data), bytes_written, completer)
              : result);
}
} // namespace repertory
