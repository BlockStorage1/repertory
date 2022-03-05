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
#include "drives/remote/remote_open_file_table.hpp"
#include "utils/utils.hpp"

namespace repertory {
void remote_open_file_table::add_directory(const std::string &client_id, void *dir) {
  unique_mutex_lock directory_lock(directory_mutex_);
  auto &list = directory_lookup_[client_id];
  if (utils::collection_excludes(list, dir)) {
    directory_lookup_[client_id].emplace_back(dir);
  }
}

void remote_open_file_table::close_all(const std::string &client_id) {
  std::vector<remote::file_handle> compatHandles;
  unique_mutex_lock compat_lock(compat_mutex_);
  for (auto &kv : compat_lookup_) {
    if (kv.second.client_id == client_id) {
      compatHandles.emplace_back(kv.first);
    }
  }
  compat_lock.unlock();

  for (auto &handle : compatHandles) {
#ifdef _WIN32
    _close(static_cast<int>(handle));
#else
    close(static_cast<int>(handle));
#endif
    remove_compat_open_info(handle);
  }

  std::vector<OSHandle> handles;
  unique_mutex_lock file_lock(file_mutex_);
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
  unique_mutex_lock directory_lock(directory_mutex_);
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
bool remote_open_file_table::get_directory_buffer(const OSHandle &handle, PVOID *&buffer) {
  mutex_lock file_lock(file_mutex_);
  if (file_lookup_.find(handle) != file_lookup_.end()) {
    buffer = &file_lookup_[handle].directory_buffer;
    return true;
  }
  return false;
}
#endif

bool remote_open_file_table::get_open_info(const OSHandle &handle, open_info &oi) {
  mutex_lock file_lock(file_mutex_);
  if (file_lookup_.find(handle) != file_lookup_.end()) {
    oi = file_lookup_[handle];
    return true;
  }
  return false;
}

std::string remote_open_file_table::get_open_file_path(const OSHandle &handle) {
  mutex_lock file_lock(file_mutex_);
  if (file_lookup_.find(handle) != file_lookup_.end()) {
    return file_lookup_[handle].path;
  }

  return "";
}

bool remote_open_file_table::has_open_directory(const std::string &client_id, void *dir) {
  unique_mutex_lock directory_lock(directory_mutex_);
  auto &list = directory_lookup_[client_id];
  return (utils::collection_includes(list, dir));
}

int remote_open_file_table::has_compat_open_info(const remote::file_handle &handle,
                                                 const int &errorReturn) {
  mutex_lock compat_lock(compat_mutex_);
  const auto res = ((compat_lookup_.find(handle) == compat_lookup_.end()) ? -1 : 0);
  if (res == -1) {
    errno = errorReturn;
  }
  return res;
}

void remote_open_file_table::remove_compat_open_info(const remote::file_handle &handle) {
  mutex_lock compat_lock(compat_mutex_);
  if (compat_lookup_[handle].count > 0) {
    compat_lookup_[handle].count--;
  }
  if (not compat_lookup_[handle].count) {
    compat_lookup_.erase(handle);
  }
}

bool remote_open_file_table::remove_directory(const std::string &client_id, void *dir) {
  unique_mutex_lock directory_lock(directory_mutex_);
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

void remote_open_file_table::remove_open_info(const OSHandle &handle) {
  mutex_lock file_lock(file_mutex_);
  if (file_lookup_[handle].count > 0) {
    file_lookup_[handle].count--;
  }
  if (not file_lookup_[handle].count) {
#ifdef _WIN32
    if (file_lookup_[handle].directory_buffer) {
      FspFileSystemDeleteDirectoryBuffer(&file_lookup_[handle].directory_buffer);
    }
#endif
    file_lookup_.erase(handle);
  }
}

void remote_open_file_table::set_compat_client_id(const remote::file_handle &handle,
                                                  const std::string &client_id) {
  mutex_lock compat_lock(compat_mutex_);
  compat_lookup_[handle].client_id = client_id;
}

void remote_open_file_table::set_client_id(const OSHandle &handle, const std::string &client_id) {
  mutex_lock file_lock(file_mutex_);
  file_lookup_[handle].client_id = client_id;
}

void remote_open_file_table::set_compat_open_info(const remote::file_handle &handle) {
  mutex_lock compat_lock(compat_mutex_);
  if (compat_lookup_.find(handle) == compat_lookup_.end()) {
    compat_lookup_[handle] = {0, ""};
  }
  compat_lookup_[handle].count++;
}

void remote_open_file_table::set_open_info(const OSHandle &handle, open_info oi) {
  mutex_lock file_lock(file_mutex_);
  if (file_lookup_.find(handle) == file_lookup_.end()) {
    file_lookup_[handle] = std::move(oi);
  }
  file_lookup_[handle].count++;
}
} // namespace repertory
