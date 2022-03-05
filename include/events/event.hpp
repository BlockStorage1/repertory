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
#ifndef INCLUDE_EVENTS_EVENT_HPP_
#define INCLUDE_EVENTS_EVENT_HPP_

#include "common.hpp"

namespace repertory {
enum class event_level { error, warn, normal, debug, verbose };

event_level event_level_from_string(std::string level);

std::string event_level_to_string(const event_level &level);

class event {
protected:
  explicit event(const bool &allow_async) : allow_async_(allow_async) {}

  event(const std::stringstream &ss, json j, const bool &allow_async)
      : allow_async_(allow_async), ss_(ss.str()), j_(std::move(j)) {}

public:
  virtual ~event() = default;

private:
  const bool allow_async_;

protected:
  std::stringstream ss_;
  json j_;

public:
  virtual std::shared_ptr<event> clone() const = 0;

  bool get_allow_async() const { return allow_async_; }

  virtual event_level get_event_level() const = 0;

  json get_json() const { return j_; }

  virtual std::string get_name() const = 0;

  virtual std::string get_single_line() const = 0;
};
} // namespace repertory

#endif // INCLUDE_EVENTS_EVENT_HPP_
