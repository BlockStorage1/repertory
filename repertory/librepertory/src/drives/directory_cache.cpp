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
#include "drives/directory_cache.hpp"

#include "drives/directory_iterator.hpp"
#include "types/repertory.hpp"
#include "utils/collection.hpp"
#include "utils/utils.hpp"

namespace repertory {
void directory_cache::execute_action(const std::string &api_path,
                                     const execute_callback &execute) {
  recur_mutex_lock directory_lock(directory_mutex_);
  if ((directory_lookup_.find(api_path) != directory_lookup_.end())) {
    execute(*directory_lookup_[api_path].iterator);
  }
}

auto directory_cache::get_directory(std::uint64_t handle)
    -> std::shared_ptr<directory_iterator> {
  recur_mutex_lock directory_lock(directory_mutex_);
  auto it = std::find_if(directory_lookup_.begin(), directory_lookup_.end(),
                         [handle](auto &&kv) -> bool {
                           auto &&handles = kv.second.handles;
                           return std::find(handles.begin(), handles.end(),
                                            handle) != kv.second.handles.end();
                         });
  if (it != directory_lookup_.end()) {
    return it->second.iterator;
  }

  return nullptr;
}

auto directory_cache::remove_directory(const std::string &api_path)
    -> std::shared_ptr<directory_iterator> {
  std::shared_ptr<directory_iterator> ret;

  recur_mutex_lock directory_lock(directory_mutex_);
  if (directory_lookup_.find(api_path) != directory_lookup_.end()) {
    ret = directory_lookup_[api_path].iterator;
    directory_lookup_.erase(api_path);
  }

  return ret;
}

void directory_cache::remove_directory(std::uint64_t handle) {
  recur_mutex_lock directory_lock(directory_mutex_);
  auto it = std::find_if(directory_lookup_.begin(), directory_lookup_.end(),
                         [handle](auto &&kv) -> bool {
                           auto &&handles = kv.second.handles;
                           return std::find(handles.begin(), handles.end(),
                                            handle) != kv.second.handles.end();
                         });
  if (it != directory_lookup_.end()) {
    utils::collection::remove_element(it->second.handles, handle);
    if (it->second.handles.empty()) {
      directory_lookup_.erase(it);
    }
  }
}

void directory_cache::service_function() {
  unique_recur_mutex_lock directory_lock(directory_mutex_);
  auto lookup = directory_lookup_;
  directory_lock.unlock();

  for (auto &&kv : lookup) {
    if (std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - kv.second.last_update) >= 120s) {
      directory_lock.lock();
      directory_lookup_.erase(kv.first);
      directory_lock.unlock();
    }
  }

  if (not get_stop_requested()) {
    unique_mutex_lock shutdown_lock(get_mutex());
    if (not get_stop_requested()) {
      get_notify().wait_for(shutdown_lock, 15s);
    }
  }
}

void directory_cache::set_directory(
    const std::string &api_path, std::uint64_t handle,
    std::shared_ptr<directory_iterator> iterator) {
  recur_mutex_lock directory_lock(directory_mutex_);
  auto &entry = directory_lookup_[api_path];
  entry.iterator = std::move(iterator);
  entry.handles.push_back(handle);
}
} // namespace repertory
