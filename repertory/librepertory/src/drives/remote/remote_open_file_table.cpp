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
#include "drives/remote/remote_open_file_table.hpp"

#include "events/event_system.hpp"
#include "events/events.hpp"
#include "utils/collection.hpp"
#include "utils/config.hpp"
#include "utils/utils.hpp"

namespace repertory {
void remote_open_file_table::add_directory(const std::string &client_id,
                                           std::uint64_t handle) {
  recur_mutex_lock lock(file_mutex_);
  auto &list = directory_lookup_[client_id];
  if (utils::collection::excludes(list, handle)) {
    directory_lookup_[client_id].emplace_back(handle);
  }
}

void remote_open_file_table::close_all(const std::string &client_id) {
  unique_recur_mutex_lock lock(file_mutex_);
  auto compat_handles =
      std::accumulate(compat_file_lookup_.begin(), compat_file_lookup_.end(),
                      std::vector<remote::file_handle>(),
                      [&client_id](auto &&list, auto &&value) {
                        auto &&op_info = value.second;
                        if (op_info->client_id == client_id) {
                          list.insert(list.end(), op_info->handles.begin(),
                                      op_info->handles.end());
                        }

                        return list;
                      });

  auto handles = std::accumulate(
      file_lookup_.begin(), file_lookup_.end(), std::vector<native_handle>(),
      [&client_id](auto &&list, auto &&value) {
        auto &&op_info = value.second;
        if (op_info->client_id == client_id) {
          list.insert(list.end(), op_info->handles.begin(),
                      op_info->handles.end());
        }

        return list;
      });
  lock.unlock();

  for (auto &&handle : compat_handles) {
#if defined(_WIN32)
    _close(static_cast<int>(handle));
#else
    close(static_cast<int>(handle));
#endif
    remove_compat_open_info(handle);
  }

  for (auto &&handle : handles) {
#if defined(_WIN32)
    ::CloseHandle(handle);
#else  // !defined(_WIN32)
    close(handle);
#endif // defined(_WIN32)
    remove_open_info(handle);
  }

  std::vector<std::uint64_t> dirs;
  lock.lock();
  for (auto &&kv : directory_lookup_) {
    if (kv.first == client_id) {
      dirs.insert(dirs.end(), kv.second.begin(), kv.second.end());
    }
  }
  lock.unlock();

  for (auto &&dir : dirs) {
    remove_directory(client_id, dir);
  }
}

#if defined(_WIN32)
auto remote_open_file_table::get_directory_buffer(const native_handle &handle,
                                                  PVOID *&buffer) -> bool {
  recur_mutex_lock lock(file_mutex_);
  if (not handle_lookup_.contains(handle)) {
    return false;
  }

  buffer = &file_lookup_.at(handle_lookup_.at(handle))->directory_buffer;
  return true;
}
#endif // defined(_WIN32)

auto remote_open_file_table::get_open_file_count(
    const std::string &file_path) const -> std::size_t {
  recur_mutex_lock lock(file_mutex_);
  return (file_lookup_.contains(file_path)
              ? file_lookup_.at(file_path)->handles.size()
              : 0ULL) +
         (compat_file_lookup_.contains(file_path)
              ? compat_file_lookup_.at(file_path)->handles.size()
              : 0ULL);
}

auto remote_open_file_table::get_open_info(const native_handle &handle,
                                           open_info &oi) -> bool {
  recur_mutex_lock lock(file_mutex_);
  if (not handle_lookup_.contains(handle)) {
    return false;
  }

  oi = *file_lookup_.at(handle_lookup_.at(handle)).get();
  return true;
}

auto remote_open_file_table::get_open_file_path(const native_handle &handle)
    -> std::string {
  recur_mutex_lock lock(file_mutex_);
  if (not handle_lookup_.contains(handle)) {
    return "";
  }

  return file_lookup_.at(handle_lookup_.at(handle))->path;
}

auto remote_open_file_table::has_open_directory(const std::string &client_id,
                                                std::uint64_t handle) -> bool {
  recur_mutex_lock lock(file_mutex_);
  return (utils::collection::includes(directory_lookup_[client_id], handle));
}

auto remote_open_file_table::has_compat_open_info(
    const remote::file_handle &handle, int error_return) -> int {
  recur_mutex_lock compat_lock(file_mutex_);
  auto res = compat_handle_lookup_.contains(handle) ? 0 : -1;
  if (res == -1) {
    errno = error_return;
  }

  return res;
}

void remote_open_file_table::remove_all(const std::string &file_path) {
  unique_recur_mutex_lock lock(file_mutex_);
  auto compat_open_list = std::accumulate(
      compat_file_lookup_.begin(), compat_file_lookup_.end(),
      std::vector<remote::file_handle>(),
      [&file_path](auto &&list, auto &&kv) -> std::vector<remote::file_handle> {
        if (kv.first == file_path) {
          auto *op_info = kv.second.get();
          list.insert(list.end(), op_info->handles.begin(),
                      op_info->handles.end());
        }
        return list;
      });

  auto open_list = std::accumulate(
      file_lookup_.begin(), file_lookup_.end(), std::vector<native_handle>(),
      [&file_path](auto &&list, auto &&kv) -> std::vector<native_handle> {
        if (kv.first == file_path) {
          auto *op_info = kv.second.get();
          list.insert(list.end(), op_info->handles.begin(),
                      op_info->handles.end());
        }
        return list;
      });
  lock.unlock();

  for (auto &&handle : compat_open_list) {
    remove_compat_open_info(handle);
  }

  for (auto &&handle : open_list) {
    remove_open_info(handle);
  }
}

void remote_open_file_table::remove_compat_open_info(
    const remote::file_handle &handle) {
  recur_mutex_lock compat_lock(file_mutex_);
  if (not compat_handle_lookup_.contains(handle)) {
    return;
  }

  auto *op_info =
      compat_file_lookup_.at(compat_handle_lookup_.at(handle)).get();
  utils::collection::remove_element(op_info->handles, handle);
  compat_handle_lookup_.erase(handle);

  if (not op_info->handles.empty()) {
    return;
  }

  auto path = op_info->path;
  compat_file_lookup_.erase(path);
}

auto remote_open_file_table::remove_directory(const std::string &client_id,
                                              std::uint64_t handle) -> bool {
  recur_mutex_lock lock(file_mutex_);
  auto &list = directory_lookup_[client_id];
  if (utils::collection::includes(list, handle)) {
    utils::collection::remove_element(list, handle);
    if (directory_lookup_[client_id].empty()) {
      directory_lookup_.erase(client_id);
    }
    return true;
  }

  return false;
}

void remote_open_file_table::remove_open_info(const native_handle &handle) {
  recur_mutex_lock lock(file_mutex_);
  if (not handle_lookup_.contains(handle)) {
    return;
  }

  auto *op_info = file_lookup_.at(handle_lookup_.at(handle)).get();
  utils::collection::remove_element(op_info->handles, handle);
  handle_lookup_.erase(handle);

  if (not op_info->handles.empty()) {
    return;
  }

#if defined(_WIN32)
  if (op_info->directory_buffer) {
    FspFileSystemDeleteDirectoryBuffer(&op_info->directory_buffer);
  }
#endif

  auto path = op_info->path;
  file_lookup_.erase(path);
}

void remote_open_file_table::remove_and_close_all(const native_handle &handle) {
  unique_recur_mutex_lock lock(file_mutex_);
  if (not handle_lookup_.contains(handle)) {
    return;
  }

  auto op_info = *file_lookup_.at(handle_lookup_.at(handle));
  lock.unlock();

  for (auto &&open_handle : op_info.handles) {
#if defined(_WIN32)
    ::CloseHandle(open_handle);
#else  // !defined(_WIN32)
    close(open_handle);
#endif // defined(_WIN32)
    remove_open_info(open_handle);
  }
}

void remote_open_file_table::set_compat_client_id(
    const remote::file_handle &handle, const std::string &client_id) {
  recur_mutex_lock compat_lock(file_mutex_);
  compat_file_lookup_.at(compat_handle_lookup_.at(handle))->client_id =
      client_id;
}

void remote_open_file_table::set_client_id(const native_handle &handle,
                                           const std::string &client_id) {
  recur_mutex_lock lock(file_mutex_);
  file_lookup_.at(handle_lookup_.at(handle))->client_id = client_id;
}

void remote_open_file_table::set_compat_open_info(
    const remote::file_handle &handle, const std::string &file_path) {
  recur_mutex_lock compat_lock(file_mutex_);
  if (compat_handle_lookup_.contains(handle)) {
    return;
  }

  if (not compat_file_lookup_.contains(file_path)) {
    compat_file_lookup_[file_path] =
        std::make_unique<compat_open_info>(compat_open_info{
            "",
            {},
            file_path,
        });
  }

  compat_handle_lookup_[handle] = file_path;
  compat_file_lookup_.at(file_path)->handles.emplace_back(handle);
}

void remote_open_file_table::set_open_info(const native_handle &handle,
                                           open_info op_info) {
  recur_mutex_lock lock(file_mutex_);
  if (handle_lookup_.contains(handle)) {
    return;
  }

  if (not file_lookup_.contains(op_info.path)) {
    file_lookup_[op_info.path] = std::make_unique<open_info>(op_info);
  }

  handle_lookup_[handle] = op_info.path;
  file_lookup_.at(op_info.path)->handles.emplace_back(handle);
}
} // namespace repertory
