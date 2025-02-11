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
#include "utils/polling.hpp"

#include "app_config.hpp"
#include "events/event_system.hpp"
#include "events/types/polling_item_begin.hpp"
#include "events/types/polling_item_end.hpp"
#include "events/types/service_start_begin.hpp"
#include "events/types/service_start_end.hpp"
#include "events/types/service_stop_begin.hpp"
#include "events/types/service_stop_end.hpp"
#include "utils/tasks.hpp"

namespace repertory {
polling polling::instance_;

void polling::frequency_thread(
    std::function<std::uint32_t()> get_frequency_seconds, frequency freq) {
  REPERTORY_USES_FUNCTION_NAME();

  while (not get_stop_requested()) {
    unique_mutex_lock lock(mutex_);
    auto futures = std::accumulate(
        items_.begin(), items_.end(), std::deque<tasks::task_ptr>{},
        [this, &freq](auto &&list, auto &&item) -> auto {
          if (item.second.freq != freq) {
            return list;
          }

          auto future = tasks::instance().schedule({
              [this, &freq, item](auto &&task_stopped) {
                if (config_->get_event_level() == event_level::trace ||
                    freq != frequency::second) {
                  event_system::instance().raise<polling_item_begin>(
                      function_name, item.first);
                }
                item.second.action(task_stopped);
                if (config_->get_event_level() == event_level::trace ||
                    freq != frequency::second) {
                  event_system::instance().raise<polling_item_end>(
                      function_name, item.first);
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

    if (get_stop_requested()) {
      return;
    }

    lock.lock();
    notify_.wait_for(lock, std::chrono::seconds(get_frequency_seconds()));
  }
}

auto polling::get_stop_requested() const -> bool {
  return stop_requested_ || app_config::get_stop_requested();
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
  REPERTORY_USES_FUNCTION_NAME();

  mutex_lock lock(start_stop_mutex_);
  if (frequency_threads_.at(0U)) {
    return;
  }

  event_system::instance().raise<service_start_begin>(function_name, "polling");
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
  event_system::instance().raise<service_start_end>(function_name, "polling");
}

void polling::stop() {
  REPERTORY_USES_FUNCTION_NAME();

  mutex_lock lock(start_stop_mutex_);
  if (not frequency_threads_.at(0U)) {
    return;
  }

  event_system::instance().raise<service_stop_begin>(function_name, "polling");

  stop_requested_ = true;

  tasks::instance().stop();

  unique_mutex_lock thread_lock(mutex_);
  std::array<std::unique_ptr<std::thread>,
             static_cast<std::size_t>(frequency::size)>
      threads;
  std::swap(threads, frequency_threads_);
  notify_.notify_all();
  thread_lock.unlock();

  for (auto &thread : threads) {
    thread->join();
    thread.reset();
  }

  event_system::instance().raise<service_stop_end>(function_name, "polling");
}
} // namespace repertory
