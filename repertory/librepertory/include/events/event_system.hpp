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
#ifndef REPERTORY_INCLUDE_EVENTS_EVENT_SYSTEM_HPP_
#define REPERTORY_INCLUDE_EVENTS_EVENT_SYSTEM_HPP_

namespace repertory {
class i_event;

class event_system final {
private:
  static constexpr std::uint8_t max_queue_retry{
      30U,
  };

  const std::uint32_t max_queue_size{
      std::thread::hardware_concurrency() * 4U,
  };

  static constexpr std::chrono::seconds queue_wait_secs{
      5s,
  };

public:
  event_system(const event_system &) = delete;
  event_system(event_system &&) = delete;
  auto operator=(const event_system &) -> event_system & = delete;
  auto operator=(event_system &&) -> event_system & = delete;

protected:
  event_system() = default;

  ~event_system() { stop(); }

public:
  class event_consumer final {
  public:
    explicit event_consumer(std::function<void(const i_event &)> callback)
        : callback_(std::move(callback)) {
      event_system::instance().attach(this);
    }

    event_consumer(std::string_view event_name,
                   std::function<void(const i_event &)> callback)
        : callback_(std::move(callback)) {
      event_system::instance().attach(event_name, this);
    }

    ~event_consumer() { event_system::instance().release(this); }

  public:
    event_consumer(const event_consumer &) = delete;
    event_consumer(event_consumer &&) = delete;
    auto operator=(const event_consumer &) -> event_consumer & = delete;
    auto operator=(event_consumer &&) -> event_consumer & = delete;

  private:
    std::function<void(const i_event &)> callback_;

  public:
    void notify_event(const i_event &event) { callback_(event); }
  };

private:
  static event_system instance_;

public:
  [[nodiscard]] static auto instance() -> event_system &;

private:
  std::unordered_map<std::string, std::deque<event_consumer *>>
      event_consumers_;
  std::recursive_mutex consumer_mutex_;
  std::vector<std::shared_ptr<i_event>> event_list_;
  std::condition_variable event_notify_;
  std::mutex event_mutex_;
  std::unique_ptr<std::thread> event_thread_;
  std::mutex run_mutex_;
  stop_type stop_requested_{false};

private:
  [[nodiscard]] auto get_stop_requested() const -> bool;

  void process_events();

  void queue_event(std::shared_ptr<i_event> evt);

public:
  void attach(event_consumer *consumer);

  void attach(std::string_view event_name, event_consumer *consumer);

  template <typename evt_t, typename... arg_t> void raise(arg_t &&...args) {
    queue_event(std::make_shared<evt_t>(std::forward<arg_t>(args)...));
  }

  void release(event_consumer *consumer);

  void start();

  void stop();
};

using event_consumer = event_system::event_consumer;

#define E_CONSUMER()                                                           \
private:                                                                       \
  std::vector<std::shared_ptr<repertory::event_consumer>> event_consumers_

#define E_CONSUMER_RELEASE() event_consumers_.clear()

#define E_SUBSCRIBE(event, callback)                                           \
  event_consumers_.emplace_back(std::make_shared<repertory::event_consumer>(   \
      event::name, [this](const i_event &evt) {                                \
        callback(dynamic_cast<const event &>(evt));                            \
      }))

#define E_SUBSCRIBE_ALL(callback)                                              \
  event_consumers_.emplace_back(std::make_shared<repertory::event_consumer>(   \
      [this](const i_event &evt) { callback(evt); }))
} // namespace repertory

#endif // REPERTORY_INCLUDE_EVENTS_EVENT_SYSTEM_HPP_
