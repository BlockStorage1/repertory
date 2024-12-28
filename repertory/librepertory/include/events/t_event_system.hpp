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
#ifndef REPERTORY_INCLUDE_EVENTS_T_EVENT_SYSTEM_HPP_
#define REPERTORY_INCLUDE_EVENTS_T_EVENT_SYSTEM_HPP_

#include "events/event.hpp"
#include "utils/collection.hpp"
#include "utils/utils.hpp"

namespace repertory {
template <typename event_type> class t_event_system final {
public:
  t_event_system(const t_event_system &) = delete;
  t_event_system(t_event_system &&) = delete;
  auto operator=(const t_event_system &) -> t_event_system & = delete;
  auto operator=(t_event_system &&) -> t_event_system & = delete;

protected:
  t_event_system() = default;

  ~t_event_system() { stop(); }

public:
  class event_consumer final {
  public:
    explicit event_consumer(std::function<void(const event &)> callback)
        : callback_(std::move(callback)) {
      t_event_system::instance().attach(this);
    }

    event_consumer(const std::string &event_name,
                   std::function<void(const event &)> callback)
        : callback_(std::move(callback)) {
      t_event_system::instance().attach(event_name, this);
    }

    ~event_consumer() { t_event_system::instance().release(this); }

  public:
    event_consumer(const event_consumer &) = delete;
    event_consumer(event_consumer &&) = delete;
    auto operator=(const event_consumer &) -> event_consumer & = delete;
    auto operator=(event_consumer &&) -> event_consumer & = delete;

  private:
    std::function<void(const event &)> callback_;

  public:
    void notify_event(const event &event) { callback_(event); }
  };

private:
  static t_event_system event_system_;

public:
  static auto instance() -> t_event_system &;

private:
  std::unordered_map<std::string, std::deque<event_consumer *>>
      event_consumers_;
  std::recursive_mutex consumer_mutex_;
  std::vector<std::shared_ptr<event_type>> event_list_;
  std::condition_variable event_notify_;
  std::mutex event_mutex_;
  std::unique_ptr<std::thread> event_thread_;
  std::mutex run_mutex_;
  stop_type stop_requested_ = false;

private:
  void process_events() {
    std::vector<std::shared_ptr<event_type>> events;
    {
      unique_mutex_lock lock(event_mutex_);
      if (not stop_requested_ && event_list_.empty()) {
        event_notify_.wait_for(lock, 1s);
      }

      if (not event_list_.empty()) {
        events.insert(events.end(), event_list_.begin(), event_list_.end());
        event_list_.clear();
      }
    }

    const auto notify_events = [this](const std::string &name,
                                      const event_type &event) {
      std::deque<std::future<void>> futures;
      recur_mutex_lock lock(consumer_mutex_);
      if (event_consumers_.find(name) != event_consumers_.end()) {
        for (auto *consumer : event_consumers_[name]) {
          if (event.get_allow_async()) {
            futures.emplace_back(
                std::async(std::launch::async, [consumer, &event]() {
                  consumer->notify_event(event);
                }));
          } else {
            consumer->notify_event(event);
          }
        }
      }

      while (not futures.empty()) {
        futures.front().get();
        futures.pop_front();
      }
    };

    for (const auto &evt : events) {
      notify_events("", *evt.get());
      notify_events(evt->get_name(), *evt.get());
    }
  }

  void queue_event(std::shared_ptr<event_type> evt) {
    mutex_lock lock(event_mutex_);
    event_list_.push_back(std::move(evt));
    event_notify_.notify_all();
  }

public:
  void attach(event_consumer *consumer) {
    recur_mutex_lock lock(consumer_mutex_);
    event_consumers_[""].push_back(consumer);
  }

  void attach(const std::string &event_name, event_consumer *consumer) {
    recur_mutex_lock lock(consumer_mutex_);
    event_consumers_[event_name].push_back(consumer);
  }

  template <typename event_t, typename... arg_t> void raise(arg_t &&...args) {
    queue_event(std::make_shared<event_t>(std::forward<arg_t>(args)...));
  }

  void release(event_consumer *consumer) {
    recur_mutex_lock lock(consumer_mutex_);
    auto iter = std::find_if(event_consumers_.begin(), event_consumers_.end(),
                             [&](const auto &item) -> bool {
                               return utils::collection::includes(item.second,
                                                                  consumer);
                             });

    if (iter != event_consumers_.end()) {
      utils::collection::remove_element((*iter).second, consumer);
    }
  }

  void start() {
    mutex_lock lock(run_mutex_);
    if (not event_thread_) {
      stop_requested_ = false;
      event_thread_ = std::make_unique<std::thread>([this]() {
        while (not stop_requested_) {
          process_events();
        }
      });
    }
  }

  void stop() {
    mutex_lock lock(run_mutex_);
    if (event_thread_) {
      stop_requested_ = true;
      event_notify_.notify_all();

      event_thread_->join();
      event_thread_.reset();

      process_events();
    }
  }
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_EVENTS_T_EVENT_SYSTEM_HPP_
