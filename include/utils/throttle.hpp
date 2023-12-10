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
#ifndef INCLUDE_UTILS_THROTTLE_HPP_
#define INCLUDE_UTILS_THROTTLE_HPP_

namespace repertory {
class throttle final {
public:
  throttle() : max_size_(10u) {}

  explicit throttle(std::size_t max_size) : max_size_(max_size) {}

public:
  throttle(const throttle &) noexcept = delete;
  throttle(throttle &&) noexcept = delete;
  auto operator=(const throttle &) -> throttle & = delete;
  auto operator=(throttle &&) -> throttle & = delete;

public:
  ~throttle() { shutdown(); }

private:
  const std::size_t max_size_;
  std::size_t count_ = 0u;
  bool shutdown_ = false;
  std::mutex mutex_;
  std::condition_variable notify_;

public:
  void decrement();

  void increment_or_wait();

  void reset();

  void shutdown();
};
} // namespace repertory

#endif // INCLUDE_UTILS_THROTTLE_HPP_
