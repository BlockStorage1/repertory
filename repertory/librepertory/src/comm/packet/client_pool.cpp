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
#include "comm/packet/client_pool.hpp"

#include "events/event_system.hpp"
#include "events/events.hpp"
#include "platform/platform.hpp"
#include "utils/error_utils.hpp"

namespace repertory {
void client_pool::pool::execute(
    std::uint64_t thread_id, const worker_callback &worker,
    const worker_complete_callback &worker_complete) {
  auto index = thread_id % pool_queues_.size();
  auto job = std::make_shared<work_item>(worker, worker_complete);
  auto &pool_queue = pool_queues_[index];

  unique_mutex_lock queue_lock(pool_queue->mutex);
  pool_queue->queue.emplace_back(job);
  pool_queue->notify.notify_all();
  queue_lock.unlock();
}

client_pool::pool::pool(std::uint8_t pool_size) {
  REPERTORY_USES_FUNCTION_NAME();

  event_system::instance().raise<service_started>("client_pool");

  for (std::uint8_t i = 0U; i < pool_size; i++) {
    pool_queues_.emplace_back(std::make_unique<work_queue>());
  }

  for (std::size_t i = 0U; i < pool_queues_.size(); i++) {
    pool_threads_.emplace_back([this]() {
      auto thread_index = thread_index_++;

      auto &pool_queue = pool_queues_[thread_index];
      auto &queue = pool_queue->queue;
      auto &queue_mutex = pool_queue->mutex;
      auto &queue_notify = pool_queue->notify;

      unique_mutex_lock queue_lock(queue_mutex);
      queue_notify.notify_all();
      queue_lock.unlock();
      while (not shutdown_) {
        queue_lock.lock();
        if (queue.empty()) {
          queue_notify.wait(queue_lock);
        }

        while (not queue.empty()) {
          auto item = queue.front();
          queue.pop_front();
          queue_notify.notify_all();
          queue_lock.unlock();

          try {
            auto result = item->work();
            item->work_complete(result);
          } catch (const std::exception &e) {
            item->work_complete(utils::from_api_error(api_error::error));
            utils::error::raise_error(function_name, e,
                                      "exception occurred in work item");
          }

          queue_lock.lock();
        }

        queue_notify.notify_all();
        queue_lock.unlock();
      }

      queue_lock.lock();
      while (not queue.empty()) {
        auto job = queue.front();
        queue.pop_front();
        queue_notify.notify_all();
        queue_lock.unlock();

        job->work_complete(utils::from_api_error(api_error::download_stopped));

        queue_lock.lock();
      }

      queue_notify.notify_all();
      queue_lock.unlock();
    });
  }
}

void client_pool::pool::shutdown() {
  shutdown_ = true;

  for (auto &pool_queue : pool_queues_) {
    mutex_lock lock(pool_queue->mutex);
    pool_queue->notify.notify_all();
  }

  for (auto &thread : pool_threads_) {
    thread.join();
  }

  pool_queues_.clear();
  pool_threads_.clear();
}

void client_pool::execute(const std::string &client_id, std::uint64_t thread_id,
                          const worker_callback &worker,
                          const worker_complete_callback &worker_complete) {
  unique_mutex_lock pool_lock(pool_mutex_);
  if (shutdown_) {
    pool_lock.unlock();
    throw std::runtime_error("Client pool is shutdown");
  }

  if (not pool_lookup_[client_id]) {
    pool_lookup_[client_id] = std::make_shared<pool>(pool_size_);
  }

  pool_lookup_[client_id]->execute(thread_id, worker, worker_complete);
  pool_lock.unlock();
}

void client_pool::remove_client(const std::string &client_id) {
  mutex_lock pool_lock(pool_mutex_);
  pool_lookup_.erase(client_id);
}

void client_pool::shutdown() {
  if (not shutdown_) {
    event_system::instance().raise<service_shutdown_begin>("client_pool");
    unique_mutex_lock pool_lock(pool_mutex_);
    if (not shutdown_) {
      shutdown_ = true;
      for (auto &pool_entry : pool_lookup_) {
        pool_entry.second->shutdown();
      }
      pool_lookup_.clear();
    }
    pool_lock.unlock();
    event_system::instance().raise<service_shutdown_end>("client_pool");
  }
}
} // namespace repertory
