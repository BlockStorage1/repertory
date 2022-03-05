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
#include "drives/directory_cache.hpp"
#include "drives/directory_iterator.hpp"
#include "events/events.hpp"
#include "events/event_system.hpp"
#include "types/repertory.hpp"

namespace repertory {
bool directory_cache::execute_action(const std::string &api_path, const execute_callback &execute) {
  auto found = false;
  recur_mutex_lock directory_lock(directory_mutex_);
  if ((found = (directory_lookup_.find(api_path) != directory_lookup_.end()))) {
    execute(*directory_lookup_[api_path].iterator);
  }

  return found;
}

void directory_cache::refresh_thread() {
  while (not is_shutdown_) {
    unique_recur_mutex_lock directory_lock(directory_mutex_);
    auto lookup = directory_lookup_;
    directory_lock.unlock();

    for (const auto &kv : lookup) {
      if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() -
                                                           kv.second.last_update) >= 120s) {
        directory_lock.lock();
        directory_lookup_.erase(kv.first);
        directory_lock.unlock();
      }
    }

    if (not is_shutdown_) {
      unique_mutex_lock shutdown_lock(shutdown_mutex_);
      if (not is_shutdown_) {
        shutdown_notify_.wait_for(shutdown_lock, 15s);
      }
    }
  }
}

directory_iterator *directory_cache::remove_directory(const std::string &api_path) {
  directory_iterator *ret = nullptr;

  recur_mutex_lock directory_lock(directory_mutex_);
  if (directory_lookup_.find(api_path) != directory_lookup_.end()) {
    ret = directory_lookup_[api_path].iterator;
    directory_lookup_.erase(api_path);
  }

  return ret;
}

void directory_cache::remove_directory(directory_iterator *iterator) {
  if (iterator) {
    recur_mutex_lock directory_lock(directory_mutex_);
    const auto it = std::find_if(
        directory_lookup_.begin(), directory_lookup_.end(),
        [&iterator](const auto &kv) -> bool { return kv.second.iterator == iterator; });
    if (it != directory_lookup_.end()) {
      directory_lookup_.erase(it->first);
    }
  }
}

void directory_cache::set_directory(const std::string &api_path, directory_iterator *iterator) {
  recur_mutex_lock directory_lock(directory_mutex_);
  directory_lookup_[api_path] = {iterator};
}

void directory_cache::start() {
  unique_mutex_lock shutdown_lock(shutdown_mutex_);
  if (is_shutdown_) {
    is_shutdown_ = false;

    refresh_thread_ = std::make_unique<std::thread>([this]() { this->refresh_thread(); });
  }
}

void directory_cache::stop() {
  unique_mutex_lock shutdown_lock(shutdown_mutex_);
  if (not is_shutdown_) {
    event_system::instance().raise<service_shutdown>("directory_cache");
    is_shutdown_ = true;
    shutdown_notify_.notify_all();
    shutdown_lock.unlock();

    refresh_thread_->join();
    refresh_thread_.reset();
  }
}
} // namespace repertory
