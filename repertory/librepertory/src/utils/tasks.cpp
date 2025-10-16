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
#include "utils/tasks.hpp"

#include "app_config.hpp"
#include "utils/error_utils.hpp"

namespace repertory {
tasks tasks::instance_;

auto tasks::get_stop_requested() const -> bool {
  return stop_requested_ || app_config::get_stop_requested();
}

void tasks::task_wait::set_result(bool result) {
  unique_mutex_lock lock(mtx);
  if (complete) {
    notify.notify_all();
    return;
  }

  complete = true;
  success = result;
  notify.notify_all();
}

auto tasks::task_wait::wait() const -> bool {
  unique_mutex_lock lock(mtx);
  while (not complete) {
    notify.wait(lock);
  }

  return success;
}

auto tasks::schedule(task item) -> task_ptr {
  ++count_;
  while (not get_stop_requested() && (count_ >= task_threads_.size())) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(config_->get_task_wait_ms()));
  }

  scheduled_task runnable{item};

  unique_mutex_lock lock(mutex_);
  if (get_stop_requested()) {
    runnable.wait->set_result(false);
    notify_.notify_all();
    return runnable.wait;
  }

  tasks_.push_back(runnable);
  notify_.notify_all();
  return runnable.wait;
}

void tasks::start(app_config *config) {
  mutex_lock start_stop_lock(start_stop_mutex_);
  if (not task_threads_.empty()) {
    return;
  }

  config_ = config;
  count_ = 0U;
  stop_requested_ = false;
  tasks_.clear();

  for (std::uint32_t idx = 0U; idx < std::thread::hardware_concurrency() * 2U;
       ++idx) {
    auto thread{std::make_unique<std::thread>([this]() { task_thread(); })};
    thread->detach();
    task_threads_.emplace_back(std::move(thread));
  }
}

void tasks::stop() {
  mutex_lock start_stop_lock(start_stop_mutex_);
  if (task_threads_.empty()) {
    return;
  }

  stop_requested_ = true;

  unique_mutex_lock lock(mutex_);
  std::vector<std::unique_ptr<std::thread>> threads;
  std::swap(threads, task_threads_);

  std::deque<scheduled_task> task_list;
  std::swap(task_list, tasks_);

  notify_.notify_all();
  lock.unlock();

  threads.clear();
  task_list.clear();
}

void tasks::task_thread() {
  REPERTORY_USES_FUNCTION_NAME();

  unique_mutex_lock lock(mutex_);

  const auto release = [&]() {
    notify_.notify_all();
    lock.unlock();
  };

  release();

  while (not get_stop_requested()) {
    lock.lock();

    while (not get_stop_requested() && tasks_.empty()) {
      notify_.wait(lock);
    }

    if (get_stop_requested()) {
      release();
      return;
    }

    if (tasks_.empty()) {
      release();
      continue;
    }

    auto runnable = tasks_.front();
    tasks_.pop_front();
    release();

    try {
      runnable.item.action(stop_requested_);
      runnable.wait->set_result(true);

      --count_;
    } catch (const std::exception &e) {
      runnable.wait->set_result(false);
      utils::error::raise_error(function_name, e, "failed to execute task");
    }

    lock.lock();
    release();
  }
}
} // namespace repertory
