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
#include "utils/polling.hpp"

#include "app_config.hpp"
#include "events/event_system.hpp"
#include "events/events.hpp"
#include "utils/tasks.hpp"

namespace repertory {
polling polling::instance_;

void polling::frequency_thread(
    std::function<std::uint32_t()> get_frequency_seconds, frequency freq) {
  while (not stop_requested_) {
    unique_mutex_lock lock(mutex_);
    auto futures = std::accumulate(
        items_.begin(), items_.end(), std::deque<tasks::task_ptr>{},
        [this, &freq](auto &&list, auto &&item) {
          if (item.second.freq != freq) {
            return list;
          }

          auto future = tasks::instance().schedule({
              [this, &freq, item](auto &&task_stopped) {
                if (config_->get_event_level() == event_level::trace ||
                    freq != frequency::second) {
                  event_system::instance().raise<polling_item_begin>(
                      item.first);
                }
                item.second.action(task_stopped);
                if (config_->get_event_level() == event_level::trace ||
                    freq != frequency::second) {
                  event_system::instance().raise<polling_item_end>(item.first);
                }
              },
          });

          list.emplace_back(future);
          return list;
        });
    lock.unlock();

    while (not futures.empty()) {
      futures.front()->wait();
      futures.pop_front();
    }

    if (stop_requested_) {
      return;
    }

    lock.lock();
    notify_.wait_for(lock, std::chrono::seconds(get_frequency_seconds()));
  }
}

void polling::remove_callback(const std::string &name) {
  mutex_lock lock(mutex_);
  items_.erase(name);
}

void polling::set_callback(const polling_item &item) {
  mutex_lock lock(mutex_);
  items_[item.name] = item;
}

void polling::start(app_config *config) {
  mutex_lock lock(start_stop_mutex_);
  if (frequency_threads_.at(0U)) {
    return;
  }

  event_system::instance().raise<service_started>("polling");
  config_ = config;
  stop_requested_ = false;

  tasks::instance().start(config);

  auto idx{0U};
  frequency_threads_.at(idx++) =
      std::make_unique<std::thread>([this]() -> void {
        this->frequency_thread(
            [this]() -> std::uint32_t {
              return config_->get_high_frequency_interval_secs();
            },
            frequency::high);
      });

  frequency_threads_.at(idx++) =
      std::make_unique<std::thread>([this]() -> void {
        this->frequency_thread(
            [this]() -> std::uint32_t {
              return config_->get_low_frequency_interval_secs();
            },
            frequency::low);
      });

  frequency_threads_.at(idx++) =
      std::make_unique<std::thread>([this]() -> void {
        this->frequency_thread(
            [this]() -> std::uint32_t {
              return config_->get_med_frequency_interval_secs();
            },
            frequency::medium);
      });

  frequency_threads_.at(idx++) =
      std::make_unique<std::thread>([this]() -> void {
        this->frequency_thread([]() -> std::uint32_t { return 1U; },
                               frequency::second);
      });
}

void polling::stop() {
  mutex_lock lock(start_stop_mutex_);
  if (not frequency_threads_.at(0U)) {
    return;
  }

  event_system::instance().raise<service_shutdown_begin>("polling");
  stop_requested_ = true;

  tasks::instance().stop();

  unique_mutex_lock thread_lock(mutex_);
  notify_.notify_all();
  thread_lock.unlock();

  for (auto &&thread : frequency_threads_) {
    thread->join();
    thread.reset();
  }

  event_system::instance().raise<service_shutdown_end>("polling");
}
} // namespace repertory
