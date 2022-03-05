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
#ifndef INCLUDE_EVENTS_CONSUMERS_CONSOLE_CONSUMER_HPP_
#define INCLUDE_EVENTS_CONSUMERS_CONSOLE_CONSUMER_HPP_

#include "common.hpp"
#include "events/event_system.hpp"

namespace repertory {
class console_consumer final {
  E_CONSUMER();

public:
  console_consumer() { E_SUBSCRIBE_ALL(process_event); }

public:
  ~console_consumer() { E_CONSUMER_RELEASE(); }

private:
  void process_event(const event &e) {
#ifdef _WIN32
#ifdef _DEBUG
    OutputDebugString((e.get_single_line() + "\n").c_str());
#endif
#endif
    if (e.get_event_level() == event_level::error) {
      std::cerr << e.get_single_line() << std::endl;
    } else {
      std::cout << e.get_single_line() << std::endl;
    }
  }
};
} // namespace repertory

#endif // INCLUDE_EVENTS_CONSUMERS_CONSOLE_CONSUMER_HPP_
