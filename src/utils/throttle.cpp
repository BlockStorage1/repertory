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
#include "utils/throttle.hpp"
#include "types/repertory.hpp"

namespace repertory {
void throttle::decrement() {
  mutex_lock l(mutex_);
  if (not shutdown_) {
    if (count_ > 0) {
      count_--;
    }
    notify_.notify_one();
  }
}

void throttle::increment_or_wait() {
  unique_mutex_lock l(mutex_);
  if (not shutdown_) {
    if (count_ >= max_size_) {
      notify_.wait(l);
    }
    if (not shutdown_) {
      count_++;
    }
  }
}

void throttle::reset() {
  unique_mutex_lock l(mutex_);
  if (shutdown_) {
    count_ = 0;
    shutdown_ = false;
  }
}

void throttle::shutdown() {
  if (not shutdown_) {
    unique_mutex_lock l(mutex_);
    shutdown_ = true;
    notify_.notify_all();
  }
}
} // namespace repertory
