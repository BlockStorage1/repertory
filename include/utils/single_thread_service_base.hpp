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
#ifndef INCLUDE_UTILS_SINGLE_THREAD_SERVICE_BASE_HPP_
#define INCLUDE_UTILS_SINGLE_THREAD_SERVICE_BASE_HPP_

#include "types/repertory.hpp"

namespace repertory {
class single_thread_service_base {
public:
  explicit single_thread_service_base(std::string service_name)
      : service_name_(std::move(service_name)) {}

  virtual ~single_thread_service_base() { stop(); }

private:
  const std::string service_name_;
  mutable std::mutex mtx_;
  mutable std::condition_variable notify_;
  stop_type stop_requested_ = false;
  std::unique_ptr<std::thread> thread_;

protected:
  [[nodiscard]] auto get_mutex() const -> std::mutex & { return mtx_; }

  [[nodiscard]] auto get_notify() const -> std::condition_variable & {
    return notify_;
  }

  [[nodiscard]] auto get_stop_requested() const -> bool {
    return stop_requested_;
  }

  void notify_all() const;

  virtual void on_start() {}

  virtual void on_stop() {}

  virtual void service_function() = 0;

public:
  void start();

  void stop();
};
} // namespace repertory

#endif // INCLUDE_UTILS_SINGLE_THREAD_SERVICE_BASE_HPP_
