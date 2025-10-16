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
#include "comm/packet/client_pool.hpp"

#include "events/event_system.hpp"
#include "events/types/service_start_begin.hpp"
#include "events/types/service_start_end.hpp"
#include "events/types/service_stop_begin.hpp"
#include "events/types/service_stop_end.hpp"
#include "utils/error.hpp"

namespace repertory {
client_pool::pool::work_queue::work_queue() {
  thread = std::make_unique<std::thread>([this]() { work_thread(); });
}

client_pool::pool::work_queue::~work_queue() {
  shutdown = true;

  unique_mutex_lock lock(mutex);
  notify.notify_all();
  lock.unlock();

  if (thread->joinable()) {
    thread->join();
  }
}

void client_pool::pool::work_queue::work_thread() {
  REPERTORY_USES_FUNCTION_NAME();

  unique_mutex_lock lock(mutex);
  const auto unlock_and_notify = [this, &lock]() {
    lock.unlock();
    notify.notify_all();
  };
  unlock_and_notify();

  const auto process_next_item = [this, &unlock_and_notify]() {
    if (actions.empty()) {
      unlock_and_notify();
      return;
    }

    auto item = actions.front();
    actions.pop_front();
    unlock_and_notify();

    try {
      item->work_complete(item->work());
    } catch (const std::exception &e) {
      utils::error::handle_exception(function_name, e);
    } catch (...) {
      utils::error::handle_exception(function_name);
    }
  };

  while (not shutdown) {
    lock.lock();
    if (actions.empty()) {
      notify.wait(lock, [this]() { return shutdown || not actions.empty(); });
      unlock_and_notify();
      continue;
    }

    process_next_item();
  }

  while (not actions.empty()) {
    lock.lock();
    process_next_item();
  }
}

client_pool::pool::~pool() { shutdown(); }

void client_pool::pool::execute(std::uint64_t thread_id, worker_callback worker,
                                worker_complete_callback worker_complete) {
  REPERTORY_USES_FUNCTION_NAME();

  auto job = std::make_shared<work_item>(std::move(worker),
                                         std::move(worker_complete));

  unique_mutex_lock pool_lock(pool_mtx_);
  if (pool_queues_[thread_id] == nullptr) {
    pool_queues_[thread_id] = std::make_shared<work_queue>();
  }

  auto pool_queue = pool_queues_[thread_id];
  pool_lock.unlock();

  pool_queue->modified = std::chrono::steady_clock::now();

  unique_mutex_lock queue_lock(pool_queue->mutex);
  pool_queue->actions.emplace_back(std::move(job));
  pool_queue->notify.notify_all();
}

void client_pool::pool::remove_expired(std::uint16_t seconds) {
  auto now = std::chrono::steady_clock::now();

  unique_mutex_lock pool_lock(pool_mtx_);
  auto results = std::accumulate(
      pool_queues_.begin(), pool_queues_.end(),
      std::unordered_map<std::uint64_t, std::shared_ptr<work_queue>>(),
      [&now, seconds](auto &&res, auto &&entry) -> auto {
        auto duration = now - entry.second->modified.load();
        if (std::chrono::duration_cast<std::chrono::seconds>(duration) >=
            std::chrono::seconds(seconds)) {
          res[entry.first] = entry.second;
        }

        return res;
      });
  pool_lock.unlock();

  for (const auto &entry : results) {
    pool_lock.lock();
    pool_queues_.erase(entry.first);
    pool_lock.unlock();
  }

  results.clear();
}

void client_pool::pool::shutdown() {
  std::unordered_map<std::uint64_t, std::shared_ptr<work_queue>> pool_queues;

  unique_mutex_lock pool_lock(pool_mtx_);
  std::swap(pool_queues, pool_queues_);
  pool_lock.unlock();

  pool_queues.clear();
}

client_pool::client_pool() noexcept {
  REPERTORY_USES_FUNCTION_NAME();

  event_system::instance().raise<service_start_begin>(function_name,
                                                      "client_pool");

  event_system::instance().raise<service_start_end>(function_name,
                                                    "client_pool");
}

auto client_pool::get_expired_seconds() const -> std::uint16_t {
  return expired_seconds_.load();
}

void client_pool::execute(std::string client_id, std::uint64_t thread_id,
                          worker_callback worker,
                          worker_complete_callback worker_complete) {
  unique_mutex_lock pool_lock(pool_mutex_);
  if (shutdown_) {
    pool_lock.unlock();
    throw std::runtime_error("client pool is shutdown");
  }

  if (not pool_lookup_[client_id]) {
    pool_lookup_[client_id] = std::make_unique<pool>();
  }

  pool_lookup_[client_id]->execute(thread_id, std::move(worker),
                                   std::move(worker_complete));
  pool_lock.unlock();
}

void client_pool::remove_client(std::string client_id) {
  mutex_lock pool_lock(pool_mutex_);
  pool_lookup_.erase(client_id);
}

void client_pool::remove_expired() {
  mutex_lock pool_lock(pool_mutex_);
  for (auto &entry : pool_lookup_) {
    entry.second->remove_expired(expired_seconds_);
  }
}

void client_pool::set_expired_seconds(std::uint16_t seconds) {
  expired_seconds_ = std::max(std::uint16_t{min_expired_seconds}, seconds);
}

void client_pool::shutdown() {
  REPERTORY_USES_FUNCTION_NAME();

  if (shutdown_) {
    return;
  }

  shutdown_ = true;

  event_system::instance().raise<service_stop_begin>(function_name,
                                                     "client_pool");

  unique_mutex_lock pool_lock(pool_mutex_);
  std::unordered_map<std::string, std::unique_ptr<pool>> pool_lookup;
  std::swap(pool_lookup, pool_lookup_);
  pool_lock.unlock();

  if (not pool_lookup.empty()) {
    for (auto &pool_entry : pool_lookup) {
      pool_entry.second->shutdown();
    }
    pool_lookup.clear();
  }

  event_system::instance().raise<service_stop_end>(function_name,
                                                   "client_pool");
}
} // namespace repertory
