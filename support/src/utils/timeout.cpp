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
#include "utils/timeout.hpp"

namespace repertory::utils {
timeout::timeout(callback_t timeout_callback,
                 std::chrono::system_clock::duration duration)
    : duration_(duration),
      timeout_callback_(std::move(timeout_callback)),
      timeout_killed_(duration <= std::chrono::system_clock::duration::zero()) {
  if (timeout_killed_) {
    return;
  }

  timeout_thread_ = std::make_unique<std::thread>([this]() {
    std::unique_lock<std::mutex> loc_lock(timeout_mutex_);

    while (not timeout_killed_) {
      auto res = timeout_notify_.wait_for(loc_lock, duration_);
      if (res != std::cv_status::timeout) {
        continue;
      }

      if (timeout_killed_) {
        return;
      }

      timeout_killed_ = true;
      loc_lock.unlock();

      try {
        timeout_callback_();
      } catch (...) {
      }
      return;
    }
  });
}

timeout::~timeout() { disable(); }

void timeout::disable() {
  unique_mutex_lock lock(timeout_mutex_);
  std::unique_ptr<std::thread> timeout_thread{nullptr};
  std::swap(timeout_thread, timeout_thread_);

  if (not timeout_thread) {
    timeout_notify_.notify_all();
    return;
  }

  timeout_killed_ = true;
  timeout_notify_.notify_all();
  lock.unlock();

  timeout_thread->join();
}

void timeout::reset() {
  mutex_lock lock(timeout_mutex_);
  timeout_notify_.notify_all();
}
} // namespace repertory::utils
