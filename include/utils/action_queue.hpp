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
#ifndef INCLUDE_UTILS_ACTION_QUEUE_HPP_
#define INCLUDE_UTILS_ACTION_QUEUE_HPP_

#include "utils/single_thread_service_base.hpp"

namespace repertory::utils::action_queue {
class action_queue final : single_thread_service_base {
  explicit action_queue(const std::string &id,
                        std::uint8_t max_concurrent_actions = 5u);

private:
  std::string id_;
  std::uint8_t max_concurrent_actions_;

private:
  std::deque<std::function<void()>> queue_;
  mutable std::mutex queue_mtx_;
  std::condition_variable queue_notify_;

protected:
  void service_function() override;

public:
  void push(std::function<void()> action);
};
} // namespace repertory::utils::action_queue

#endif // INCLUDE_UTILS_ACTION_QUEUE_HPP_
