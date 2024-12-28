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
#ifndef REPERTORY_TEST_INCLUDE_UTILS_EVENT_CAPTURE_HPP_
#define REPERTORY_TEST_INCLUDE_UTILS_EVENT_CAPTURE_HPP_

#include "test_common.hpp"

#include "utils/collection.hpp"
#include "utils/utils.hpp"

namespace repertory {
class event_capture final {
  E_CONSUMER();

public:
  explicit event_capture(std::vector<std::string> event_names,
                         std::vector<std::string> non_fired_event_names = {})
      : event_names_(std::move(event_names)),
        non_fired_event_names_(std::move(non_fired_event_names)) {
    E_SUBSCRIBE_ALL(process_event);
  }

  ~event_capture() {
    wait_for_empty();

    E_CONSUMER_RELEASE();

    EXPECT_TRUE(event_names_.empty());
    for (const auto &event_name : event_names_) {
      std::cerr << '\t' << event_name << std::endl;
    }
  }

private:
  std::vector<std::string> event_names_;
  std::vector<std::string> fired_event_names_;
  std::vector<std::string> non_fired_event_names_;
  std::mutex mutex_;
  std::condition_variable notify_;

public:
  void process_event(const event &event) {
    unique_mutex_lock l(mutex_);
    utils::collection::remove_element(event_names_, event.get_name());
    fired_event_names_.emplace_back(event.get_name());
    notify_.notify_all();
    l.unlock();

    for (size_t i = 0; i < non_fired_event_names_.size(); i++) {
      const auto it = std::find(non_fired_event_names_.begin(),
                                non_fired_event_names_.end(), event.get_name());
      EXPECT_EQ(non_fired_event_names_.end(), it);
      if (it != non_fired_event_names_.end()) {
        std::cerr << '\t' << *it << std::endl;
      }
    }
  }

  void wait_for_empty() {
    const auto start_time = std::chrono::system_clock::now();
    unique_mutex_lock l(mutex_);
    while (not event_names_.empty() &&
           (std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() - start_time)
                .count() < 10)) {
      notify_.wait_for(l, 1s);
    }
    l.unlock();
  }

  [[nodiscard]] auto wait_for_event(const std::string &event_name) -> bool {
    auto missing = true;
    const auto start_time = std::chrono::system_clock::now();

    unique_mutex_lock l(mutex_);
    while ((std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() - start_time)
                .count() < 10) &&
           (missing =
                (std::find(fired_event_names_.begin(), fired_event_names_.end(),
                           event_name) == fired_event_names_.end()))) {
      notify_.wait_for(l, 1s);
    }
    l.unlock();
    return not missing;
  }
};
} // namespace repertory

#endif // REPERTORY_TEST_INCLUDE_UTILS_EVENT_CAPTURE_HPP_
