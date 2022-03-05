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
#include "download/reader_pool.hpp"

namespace repertory {
void reader_pool::pause() {
  unique_mutex_lock work_lock(work_mutex_);
  if (not paused_) {
    paused_ = true;
    while (not stop_requested_ && not work_queue_.empty()) {
      work_notify_.notify_all();
      work_notify_.wait_for(work_lock, 2s);
    }
  }
  work_notify_.notify_all();
}

void reader_pool::process_work_item(pool_work_item &work) {
  const auto result =
      api_reader_(work.api_path, work.read_size, work.read_offset, work.data, restart_active_);
  work.completed(result);
}

void reader_pool::queue_read_bytes(const std::string &api_path, const std::size_t &read_size,
                                   const std::uint64_t &read_offset, std::vector<char> &data,
                                   completed_callback completed) {
  data.clear();

  unique_mutex_lock work_lock(work_mutex_);
  wait_for_resume(work_lock);
  if (not stop_requested_) {
    if (pool_size_ == 1ull) {
      work_notify_.notify_all();
      work_lock.unlock();
      completed(api_reader_(api_path, read_size, read_offset, data, restart_active_));
    } else {
      auto work =
          std::make_shared<pool_work_item>(api_path, read_size, read_offset, data, completed);
      if (not restart_active_) {
        while ((work_queue_.size() >= pool_size_) && not restart_active_) {
          work_notify_.wait(work_lock);
        }

        if (not restart_active_) {
          work_queue_.emplace_back(work);
          work_notify_.notify_all();
        }

        work_lock.unlock();
      }
    }
  }
}

void reader_pool::restart() {
  restart_active_ = true;

  unique_mutex_lock work_lock(work_mutex_);
  wait_for_resume(work_lock);
  work_notify_.notify_all();
  work_lock.unlock();

  while (not work_queue_.empty() || active_count_) {
    work_lock.lock();
    if (not work_queue_.empty() || active_count_) {
      work_notify_.wait(work_lock);
    }
    work_lock.unlock();
  }

  work_lock.lock();
  restart_active_ = false;
  work_notify_.notify_all();
  work_lock.unlock();
}

void reader_pool::resume() {
  unique_mutex_lock work_lock(work_mutex_);
  if (paused_) {
    paused_ = false;
  }
  work_notify_.notify_all();
}

void reader_pool::start() {
  restart_active_ = stop_requested_ = false;
  active_count_ = 0u;
  if (pool_size_ > 1u) {
    for (std::size_t i = 0u; i < pool_size_; i++) {
      work_threads_.emplace_back(std::thread([this]() {
        unique_mutex_lock work_lock(work_mutex_);
        work_notify_.notify_all();
        work_lock.unlock();

        while (not stop_requested_) {
          work_lock.lock();
          if (not stop_requested_) {
            if (work_queue_.empty()) {
              work_notify_.wait(work_lock);
            }

            if (not work_queue_.empty()) {
              active_count_++;
              auto work = work_queue_.front();
              work_queue_.pop_front();
              work_notify_.notify_all();
              work_lock.unlock();

              process_work_item(*work);

              work_lock.lock();
              active_count_--;
            }
          }

          work_notify_.notify_all();
          work_lock.unlock();
        }

        work_lock.lock();
        work_notify_.notify_all();
        work_lock.unlock();
      }));
    }
  }
}

void reader_pool::stop() {
  stop_requested_ = true;
  paused_ = false;
  restart();
  restart_active_ = true;

  unique_mutex_lock work_lock(work_mutex_);
  work_notify_.notify_all();
  work_lock.unlock();

  for (auto &work_thread : work_threads_) {
    work_thread.join();
  }
  work_threads_.clear();
}

void reader_pool::wait_for_resume(unique_mutex_lock &lock) {
  while (paused_ && not stop_requested_) {
    work_notify_.notify_all();
    work_notify_.wait_for(lock, 2s);
  }
}
} // namespace repertory
