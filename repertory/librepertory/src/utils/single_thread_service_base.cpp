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
#include "utils/single_thread_service_base.hpp"

#include "app_config.hpp"
#include "events/event_system.hpp"
#include "events/types/service_start_begin.hpp"
#include "events/types/service_start_end.hpp"
#include "events/types/service_stop_begin.hpp"
#include "events/types/service_stop_end.hpp"
#include "types/repertory.hpp"

namespace repertory {
auto single_thread_service_base::get_stop_requested() const -> bool {
  return stop_requested_ || app_config::get_stop_requested();
}

void single_thread_service_base::notify_all() const {
  mutex_lock lock(get_mutex());
  notify_.notify_all();
}

void single_thread_service_base::start() {
  REPERTORY_USES_FUNCTION_NAME();

  mutex_lock lock(mtx_);
  if (thread_) {
    return;
  }

  stop_requested_ = false;
  on_start();
  thread_ = std::make_unique<std::thread>([this]() {
    event_system::instance().raise<service_start_begin>(function_name,
                                                        service_name_);
    event_system::instance().raise<service_start_end>(function_name,
                                                      service_name_);
    while (not get_stop_requested()) {
      service_function();
    }
  });
}

void single_thread_service_base::stop() {
  REPERTORY_USES_FUNCTION_NAME();

  unique_mutex_lock lock(mtx_);
  if (not thread_) {
    return;
  }

  event_system::instance().raise<service_stop_begin>(function_name,
                                                     service_name_);

  stop_requested_ = true;

  std::unique_ptr<std::thread> thread{nullptr};
  std::swap(thread, thread_);

  notify_.notify_all();
  lock.unlock();

  thread->join();
  thread.reset();

  on_stop();

  event_system::instance().raise<service_stop_end>(function_name,
                                                   service_name_);
}
} // namespace repertory
