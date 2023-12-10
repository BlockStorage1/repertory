/*
  Copyright <2018-2023> <scott.e.graves@protonmail.com>

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

namespace repertory {
polling polling::instance_;

void polling::frequency_thread(
    std::function<std::uint32_t()> get_frequency_seconds, frequency freq) {
  while (not stop_requested_) {
    std::deque<std::future<void>> futures;
    unique_mutex_lock l(mutex_);
    if (not stop_requested_ &&
        notify_.wait_for(l, std::chrono::seconds(get_frequency_seconds())) ==
            std::cv_status::timeout) {
      for (const auto &kv : items_) {
        if (kv.second.freq == freq) {
          futures.emplace_back(
              std::async(std::launch::async, [this, &freq, kv]() -> void {
                if (config_->get_event_level() == event_level::verbose ||
                    freq != frequency::second) {
                  event_system::instance().raise<polling_item_begin>(kv.first);
                }
                kv.second.action();
                if (config_->get_event_level() == event_level::verbose ||
                    freq != frequency::second) {
                  event_system::instance().raise<polling_item_end>(kv.first);
                }
              }));
        }
      }
      l.unlock();

      while (not futures.empty()) {
        futures.front().wait();
        futures.pop_front();
      }
    }
  }
}

void polling::remove_callback(const std::string &name) {
  mutex_lock l(mutex_);
  items_.erase(name);
}

void polling::set_callback(const polling_item &pi) {
  mutex_lock l(mutex_);
  items_[pi.name] = pi;
}

void polling::start(app_config *config) {
  mutex_lock l(start_stop_mutex_);
  if (not high_frequency_thread_) {
    event_system::instance().raise<service_started>("polling");
    config_ = config;
    stop_requested_ = false;
    high_frequency_thread_ = std::make_unique<std::thread>([this]() -> void {
      this->frequency_thread(
          [this]() -> std::uint32_t {
            return config_->get_high_frequency_interval_secs();
          },
          frequency::high);
    });
    low_frequency_thread_ = std::make_unique<std::thread>([this]() -> void {
      this->frequency_thread(
          [this]() -> std::uint32_t {
            return config_->get_low_frequency_interval_secs();
          },
          frequency::low);
    });
    second_frequency_thread_ = std::make_unique<std::thread>([this]() -> void {
      this->frequency_thread([]() -> std::uint32_t { return 1U; },
                             frequency::second);
    });
  }
}

void polling::stop() {
  if (high_frequency_thread_) {
    event_system::instance().raise<service_shutdown_begin>("polling");
    mutex_lock l(start_stop_mutex_);
    if (high_frequency_thread_) {
      {
        stop_requested_ = true;

        mutex_lock l2(mutex_);
        notify_.notify_all();
      }
      high_frequency_thread_->join();
      low_frequency_thread_->join();
      second_frequency_thread_->join();
      high_frequency_thread_.reset();
      low_frequency_thread_.reset();
      second_frequency_thread_.reset();
    }
    event_system::instance().raise<service_shutdown_end>("polling");
  }
}
} // namespace repertory
