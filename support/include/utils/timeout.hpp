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
#ifndef REPERTORY_INCLUDE_UTILS_TIMEOUT_HPP_
#define REPERTORY_INCLUDE_UTILS_TIMEOUT_HPP_

#include "utils/config.hpp"

namespace repertory::utils {
class timeout final {
public:
  using callback_t = std::function<void()>;

public:
  timeout(const timeout &) noexcept = delete;
  timeout(timeout &&) noexcept = delete;
  auto operator=(const timeout &) noexcept -> timeout & = delete;
  auto operator=(timeout &&) noexcept -> timeout & = delete;

public:
  timeout(callback_t timeout_callback,
          std::chrono::system_clock::duration duration);

  ~timeout();

private:
  std::chrono::system_clock::duration duration_;
  callback_t timeout_callback_;
  std::atomic<bool> timeout_killed_{false};
  std::unique_ptr<std::thread> timeout_thread_{nullptr};
  std::mutex timeout_mutex_;
  std::condition_variable timeout_notify_;

public:
  void disable();

  void reset();
};
} // namespace repertory::utils

#endif // REPERTORY_INCLUDE_UTILS_TIMEOUT_HPP_
