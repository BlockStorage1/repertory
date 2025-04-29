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
#ifndef REPERTORY_INCLUDE_EVENTS_CONSUMERS_LOGGING_CONSUMER_HPP_
#define REPERTORY_INCLUDE_EVENTS_CONSUMERS_LOGGING_CONSUMER_HPP_

#include "events/event_system.hpp"
#include "types/repertory.hpp"

namespace repertory {
class logging_consumer final {
  E_CONSUMER();

public:
  logging_consumer(event_level level, std::string log_directory);

  ~logging_consumer();

private:
  static constexpr std::uint8_t MAX_LOG_FILES{5U};
  static constexpr std::uint64_t MAX_LOG_FILE_SIZE{1024ULL * 1024ULL * 5ULL};

private:
  static void process_event(const i_event &evt);
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_EVENTS_CONSUMERS_LOGGING_CONSUMER_HPP_
