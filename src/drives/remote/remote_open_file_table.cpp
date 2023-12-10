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
#include "drives/remote/remote_open_file_table.hpp"

#include "events/event_system.hpp"
#include "events/events.hpp"
#include "utils/utils.hpp"

namespace repertory {
void remote_open_file_table::add_directory(const std::string &client_id,
                                           void *dir) {
  recur_mutex_lock directory_lock(directory_mutex_);
  auto &list = directory_lookup_[client_id];
  if (utils::collection_excludes(list, dir)) {
    directory_lookup_[client_id].emplace_back(dir);
  }
}

void remote_open_file_table::close_all(const std::string &client_id) {
  std::vector<remote::file_handle> compat_handles;
  unique_recur_mutex_lock compat_lock(compat_mutex_);
  for (auto &kv : compat_lookup_) {
    if (kv.second.client_id == client_id) {
      compat_handles.emplace_back(kv.first);
    }
  }
  compat_lock.unlock();

  for (auto &handle : compat_handles) {
#ifdef _WIN32
    _close(static_cast<int>(handle));
#else
    close(static_cast<int>(handle));
#endif
    remove_compat_open_info(handle);
  }

  std::vector<native_handle> handles;
  unique_recur_mutex_lock file_lock(file_mutex_);
  for (auto &kv : file_lookup_) {
    if (kv.second.client_id == client_id) {
      handles.emplace_back(kv.first);
    }
  }
  file_lock.unlock();

  for (auto &handle : handles) {
#ifdef _WIN32
    ::CloseHandle(handle);
#else
    close(handle);
#endif
    remove_open_info(handle);
  }

  std::vector<void *> dirs;
  unique_recur_mutex_lock directory_lock(directory_mutex_);
  for (auto &kv : directory_lookup_) {
    if (kv.first == client_id) {
      dirs.insert(dirs.end(), kv.second.begin(), kv.second.end());
    }
  }
  directory_lock.unlock();

  for (auto *dir : dirs) {
    remove_directory(client_id, dir);
  }
}

#ifdef _WIN32
auto remote_open_file_table::get_directory_buffer(const native_handle &handle,
                                                  PVOID *&buffer) -> bool {
  recur_mutex_lock file_lock(file_mutex_);
  if (file_lookup_.find(handle) != file_lookup_.end()) {
    buffer = &file_lookup_[handle].directory_buffer;
    return true;
  }
  return false;
}
#endif

auto remote_open_file_table::get_open_file_count(
    const std::string &file_path) const -> std::size_t {
  unique_recur_mutex_lock file_lock(file_mutex_);
  const auto count = std::accumulate(
      file_lookup_.cbegin(), file_lookup_.cend(), std::size_t(0U),
      [&file_path](std::size_t total, const auto &kv) -> std::size_t {
        if (kv.second.path == file_path) {
          return ++total;
        }
        return total;
      });

  return std::accumulate(
      compat_lookup_.cbegin(), compat_lookup_.cend(), count,
      [&file_path](std::size_t total, const auto &kv) -> std::size_t {
        if (kv.second.path == file_path) {
          return ++total;
        }
        return total;
      });
}

auto remote_open_file_table::get_open_info(const native_handle &handle,
                                           open_info &oi) -> bool {
  recur_mutex_lock file_lock(file_mutex_);
  if (file_lookup_.find(handle) != file_lookup_.end()) {
    oi = file_lookup_[handle];
    return true;
  }
  return false;
}

auto remote_open_file_table::get_open_file_path(const native_handle &handle)
    -> std::string {
  recur_mutex_lock file_lock(file_mutex_);
  if (file_lookup_.find(handle) != file_lookup_.end()) {
    return file_lookup_[handle].path;
  }

  return "";
}

auto remote_open_file_table::has_open_directory(const std::string &client_id,
                                                void *dir) -> bool {
  recur_mutex_lock directory_lock(directory_mutex_);
  auto &list = directory_lookup_[client_id];
  return (utils::collection_includes(list, dir));
}

auto remote_open_file_table::has_compat_open_info(
    const remote::file_handle &handle, int error_return) -> int {
  recur_mutex_lock compat_lock(compat_mutex_);
  const auto res =
      ((compat_lookup_.find(handle) == compat_lookup_.end()) ? -1 : 0);
  if (res == -1) {
    errno = error_return;
  }
  return res;
}

void remote_open_file_table::remove_all(const std::string &file_path) {
  unique_recur_mutex_lock file_lock(file_mutex_);
  const auto open_list = std::accumulate(
      file_lookup_.begin(), file_lookup_.end(), std::vector<native_handle>(),
      [&file_path](std::vector<native_handle> v,
                   const auto &kv) -> std::vector<native_handle> {
        if (kv.second.path == file_path) {
          v.emplace_back(kv.first);
        }
        return v;
      });

  const auto compat_open_list = std::accumulate(
      compat_lookup_.begin(), compat_lookup_.end(),
      std::vector<remote::file_handle>(),
      [&file_path](std::vector<remote::file_handle> v,
                   const auto &kv) -> std::vector<remote::file_handle> {
        if (kv.second.path == file_path) {
          v.emplace_back(kv.first);
        }
        return v;
      });
  file_lock.unlock();

  for (auto &handle : open_list) {
    remove_open_info(handle);
  }

  for (auto &handle : compat_open_list) {
    remove_compat_open_info(handle);
  }
}

void remote_open_file_table::remove_compat_open_info(
    const remote::file_handle &handle) {
  recur_mutex_lock compat_lock(compat_mutex_);
  if (compat_lookup_[handle].count > 0) {
    compat_lookup_[handle].count--;
  }

  if (not compat_lookup_[handle].count) {
    compat_lookup_.erase(handle);
  }
}

auto remote_open_file_table::remove_directory(const std::string &client_id,
                                              void *dir) -> bool {
  recur_mutex_lock directory_lock(directory_mutex_);
  auto &list = directory_lookup_[client_id];
  if (utils::collection_includes(list, dir)) {
    utils::remove_element_from(list, dir);
    delete_open_directory(dir);
    if (directory_lookup_[client_id].empty()) {
      directory_lookup_.erase(client_id);
    }
    return true;
  }
  return false;
}

void remote_open_file_table::remove_open_info(const native_handle &handle) {
  recur_mutex_lock file_lock(file_mutex_);
  if (file_lookup_[handle].count > 0) {
    file_lookup_[handle].count--;
  }
  if (not file_lookup_[handle].count) {
#ifdef _WIN32
    if (file_lookup_[handle].directory_buffer) {
      FspFileSystemDeleteDirectoryBuffer(
          &file_lookup_[handle].directory_buffer);
    }
#endif
    file_lookup_.erase(handle);
  }
}

void remote_open_file_table::set_compat_client_id(
    const remote::file_handle &handle, const std::string &client_id) {
  recur_mutex_lock compat_lock(compat_mutex_);
  compat_lookup_[handle].client_id = client_id;
}

void remote_open_file_table::set_client_id(const native_handle &handle,
                                           const std::string &client_id) {
  recur_mutex_lock file_lock(file_mutex_);
  file_lookup_[handle].client_id = client_id;
}

void remote_open_file_table::set_compat_open_info(
    const remote::file_handle &handle, const std::string &file_path) {
  recur_mutex_lock compat_lock(compat_mutex_);
  if (compat_lookup_.find(handle) == compat_lookup_.end()) {
    compat_lookup_[handle] = {0, "", file_path};
  }
  compat_lookup_[handle].count++;
}

void remote_open_file_table::set_open_info(const native_handle &handle,
                                           open_info oi) {
  recur_mutex_lock file_lock(file_mutex_);
  if (file_lookup_.find(handle) == file_lookup_.end()) {
    file_lookup_[handle] = std::move(oi);
  }
  file_lookup_[handle].count++;
}
} // namespace repertory
