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
#ifndef INCLUDE_EVENTS_T_EVENT_SYSTEM_HPP_
#define INCLUDE_EVENTS_T_EVENT_SYSTEM_HPP_

#include "events/event.hpp"
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
      unique_mutex_lock l(event_mutex_);
      if (not stop_requested_ && event_list_.empty()) {
        event_notify_.wait_for(l, 1s);
      }

      if (not event_list_.empty()) {
        events.insert(events.end(), event_list_.begin(), event_list_.end());
        event_list_.clear();
      }
    }

    const auto notify_events = [this](const std::string &name,
                                      const event_type &event) {
      std::deque<std::future<void>> futures;
      recur_mutex_lock l(consumer_mutex_);
      if (event_consumers_.find(name) != event_consumers_.end()) {
        for (auto *ec : event_consumers_[name]) {
          if (event.get_allow_async()) {
            futures.emplace_back(std::async(std::launch::async, [ec, &event]() {
              ec->notify_event(event);
            }));
          } else {
            ec->notify_event(event);
          }
        }
      }

      while (not futures.empty()) {
        futures.front().get();
        futures.pop_front();
      }
    };

    for (const auto &e : events) {
      notify_events("", *e.get());
      notify_events(e->get_name(), *e.get());
    }
  }

  void queue_event(event_type *e) {
    mutex_lock l(event_mutex_);
    event_list_.emplace_back(std::shared_ptr<event_type>(e));
    event_notify_.notify_all();
  }

public:
  void attach(event_consumer *ec) {
    recur_mutex_lock l(consumer_mutex_);
    event_consumers_[""].push_back(ec);
  }

  void attach(const std::string &event_name, event_consumer *ec) {
    recur_mutex_lock l(consumer_mutex_);
    event_consumers_[event_name].push_back(ec);
  }

  template <typename t, typename... args> void raise(args &&...a) {
    queue_event(new t(std::forward<args>(a)...));
  }

  void release(event_consumer *ec) {
    recur_mutex_lock l(consumer_mutex_);
    auto it = std::find_if(event_consumers_.begin(), event_consumers_.end(),
                           [&](const auto &kv) -> bool {
                             return utils::collection_includes(kv.second, ec);
                           });

    if (it != event_consumers_.end()) {
      auto &q = (*it).second;
      utils::remove_element_from(q, ec);
    }
  }

  void start() {
    mutex_lock l(run_mutex_);
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
    mutex_lock l(run_mutex_);
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

#endif // INCLUDE_EVENTS_T_EVENT_SYSTEM_HPP_
