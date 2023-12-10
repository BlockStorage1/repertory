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
#include "utils/single_thread_service_base.hpp"

#include "events/event_system.hpp"
#include "events/events.hpp"
#include "types/repertory.hpp"

namespace repertory {
void single_thread_service_base::notify_all() const {
  mutex_lock lock(get_mutex());
  notify_.notify_all();
}

void single_thread_service_base::start() {
  mutex_lock lock(mtx_);
  if (not thread_) {
    stop_requested_ = false;
    on_start();
    thread_ = std::make_unique<std::thread>([this]() {
      event_system::instance().raise<service_started>(service_name_);
      while (not stop_requested_) {
        service_function();
      }
    });
  }
}

void single_thread_service_base::stop() {
  if (thread_) {
    event_system::instance().raise<service_shutdown_begin>(service_name_);
    unique_mutex_lock lock(mtx_);
    if (thread_) {
      stop_requested_ = true;
      notify_.notify_all();
      lock.unlock();

      thread_->join();
      thread_.reset();

      on_stop();
    }
    event_system::instance().raise<service_shutdown_end>(service_name_);
  }
}
} // namespace repertory
