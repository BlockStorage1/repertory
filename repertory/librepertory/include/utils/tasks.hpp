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
#ifndef REPERTORY_INCLUDE_UTILS_TASKS_HPP_
#define REPERTORY_INCLUDE_UTILS_TASKS_HPP_

#include "common.hpp"

namespace repertory {
class app_config;

class tasks final {
public:
  struct task final {
    std::function<void(const stop_type &task_stopped)> action;
  };

  class i_task {
    INTERFACE_SETUP(i_task);

  public:
    virtual auto wait() const -> bool = 0;
  };

  using task_ptr = std::shared_ptr<i_task>;

private:
  class task_wait final : public i_task {
  public:
    task_wait() = default;
    task_wait(const task_wait &) = delete;
    task_wait(task_wait &&) = delete;

    ~task_wait() override { set_result(false); }

    auto operator=(const task_wait &) -> task_wait & = delete;
    auto operator=(task_wait &&) -> task_wait & = delete;

  private:
    bool complete{false};
    mutable std::mutex mtx;
    mutable std::condition_variable notify;
    bool success{false};

  public:
    void set_result(bool result);

    auto wait() const -> bool override;
  };

  struct scheduled_task final {
    task item;

    std::shared_ptr<task_wait> wait{
        std::make_shared<task_wait>(),
    };
  };

public:
  tasks(const tasks &) = delete;
  tasks(tasks &&) = delete;
  auto operator=(const tasks &) -> tasks & = delete;
  auto operator=(tasks &&) -> tasks & = delete;

private:
  tasks() = default;

  ~tasks() { stop(); }

private:
  static tasks instance_;

public:
  static auto instance() -> tasks & { return instance_; }

private:
  app_config *config_{nullptr};
  std::atomic<std::uint64_t> count_{0U};
  std::mutex mutex_;
  std::condition_variable notify_;
  std::mutex start_stop_mutex_;
  stop_type stop_requested_{false};
  std::vector<std::unique_ptr<std::jthread>> task_threads_;
  std::deque<scheduled_task> tasks_;

private:
  void task_thread();

public:
  auto schedule(task item) -> task_ptr;

  void start(app_config *config);

  void stop();
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_UTILS_TASKS_HPP_
