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
#include "events/event_system.hpp"

#include "app_config.hpp"
#include "events/i_event.hpp"
#include "utils/collection.hpp"

namespace repertory {
event_system event_system::instance_{};

auto event_system::instance() -> event_system & { return instance_; }

void event_system::attach(event_consumer *consumer) {
  recur_mutex_lock lock(consumer_mutex_);
  event_consumers_[""].push_back(consumer);
}

void event_system::attach(std::string_view event_name,
                          event_consumer *consumer) {
  recur_mutex_lock lock(consumer_mutex_);
  event_consumers_[std::string{event_name}].push_back(consumer);
}

auto event_system::get_stop_requested() const -> bool {
  return stop_requested_ || app_config::get_stop_requested();
}

void event_system::process_events() {
  unique_mutex_lock lock(event_mutex_);
  const auto lock_and_notify = [this, &lock]() {
    lock.lock();
    event_notify_.notify_all();
    lock.unlock();
  };

  if (not get_stop_requested() && event_list_.empty()) {
    event_notify_.wait_for(lock, queue_wait_secs);
  }

  std::vector<std::shared_ptr<i_event>> event_list;
  std::swap(event_list, event_list_);
  lock.unlock();

  if (event_list.empty()) {
    lock_and_notify();
    return;
  }

  const auto notify_events = [this](std::string_view name,
                                    const i_event &event) {
    std::deque<std::future<void>> futures;
    recur_mutex_lock consumer_lock(consumer_mutex_);
    for (auto *consumer : event_consumers_[std::string{name}]) {
      futures.emplace_back(std::async(std::launch::async, [consumer, &event]() {
        consumer->notify_event(event);
      }));
    }

    while (not futures.empty()) {
      futures.front().get();
      futures.pop_front();
    }
  };

  for (const auto &evt : event_list) {
    notify_events("", *evt);
    notify_events(evt->get_name(), *evt);
  }

  lock_and_notify();
}

void event_system::queue_event(std::shared_ptr<i_event> evt) {
  unique_mutex_lock lock(event_mutex_);
  event_list_.push_back(std::move(evt));
  auto size = event_list_.size();
  event_notify_.notify_all();
  lock.unlock();

  for (std::uint8_t retry{0U};
       size > max_queue_size && retry < max_queue_retry &&
       not get_stop_requested();
       ++retry) {
    lock.lock();
    size = event_list_.size();
    if (size > max_queue_size) {
      event_notify_.wait_for(lock, queue_wait_secs);
      size = event_list_.size();
    }

    event_notify_.notify_all();
    lock.unlock();
  }
}

void event_system::release(event_consumer *consumer) {
  recur_mutex_lock lock(consumer_mutex_);
  auto iter =
      std::ranges::find_if(event_consumers_, [&consumer](auto &&item) -> bool {
        return utils::collection::includes(item.second, consumer);
      });

  if (iter != event_consumers_.end()) {
    utils::collection::remove_element(iter->second, consumer);
  }
}

void event_system::start() {
  mutex_lock lock(run_mutex_);
  if (event_thread_) {
    event_notify_.notify_all();
    return;
  }

  stop_requested_ = false;

  event_thread_ = std::make_unique<std::thread>([this]() {
    while (not get_stop_requested()) {
      process_events();
    }
  });
  event_notify_.notify_all();
}

void event_system::stop() {
  unique_mutex_lock lock(run_mutex_);
  if (not event_thread_) {
    event_notify_.notify_all();
    return;
  }

  stop_requested_ = true;

  std::unique_ptr<std::thread> thread{nullptr};
  std::swap(thread, event_thread_);

  event_notify_.notify_all();
  lock.unlock();

  thread->join();
  thread.reset();

  process_events();
}
} // namespace repertory
