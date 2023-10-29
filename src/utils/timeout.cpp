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
#include "utils/timeout.hpp"

#include "types/repertory.hpp"

namespace repertory {
timeout::timeout(std::function<void()> timeout_callback,
                 const std::chrono::system_clock::duration &duration)
    : timeout_killed_(duration == 0s) {
  if (not timeout_killed_) {
    timeout_thread_ =
        std::make_unique<std::thread>([this, duration, timeout_callback]() {
          unique_mutex_lock lock(timeout_mutex_);
          if (not timeout_killed_) {
            timeout_notify_.wait_for(lock, duration);
            if (not timeout_killed_) {
              timeout_callback();
            }
          }
        });
  }
}

void timeout::disable() {
  if (not timeout_killed_) {
    timeout_killed_ = true;
    unique_mutex_lock lock(timeout_mutex_);
    timeout_notify_.notify_all();
    lock.unlock();
    timeout_thread_->join();
    timeout_thread_.reset();
  }
}
} // namespace repertory
