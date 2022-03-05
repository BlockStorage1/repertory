/*
  Copyright <2018-2022> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute,
  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
  OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef INCLUDE_UTILS_TIMEOUT_HPP_
#define INCLUDE_UTILS_TIMEOUT_HPP_

#include "common.hpp"

namespace repertory {
class timeout final {
public:
  timeout(const timeout &) noexcept = delete;
  timeout(timeout &&) noexcept = delete;
  timeout &operator=(const timeout &) noexcept = delete;
  timeout &operator=(timeout &&) noexcept = delete;

public:
  timeout(std::function<void()> timeout_callback,
          const std::chrono::system_clock::duration &duration = 10s);

  ~timeout() { disable(); }

private:
  bool timeout_killed_;
  std::unique_ptr<std::thread> timeout_thread_;
  std::mutex timeout_mutex_;
  std::condition_variable timeout_notify_;

public:
  void disable();
};
} // namespace repertory

#endif // INCLUDE_UTILS_TIMEOUT_HPP_
