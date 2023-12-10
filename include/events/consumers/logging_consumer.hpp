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
#ifndef INCLUDE_EVENTS_CONSUMERS_LOGGING_CONSUMER_HPP_
#define INCLUDE_EVENTS_CONSUMERS_LOGGING_CONSUMER_HPP_

#include "events/event_system.hpp"

namespace repertory {
class logging_consumer {
  E_CONSUMER();

public:
  logging_consumer(const std::string &log_directory, const event_level &level);

  ~logging_consumer();

private:
  const std::uint8_t MAX_LOG_FILES = 5;
  const std::uint64_t MAX_LOG_FILE_SIZE = (1024 * 1024 * 5);

private:
  event_level event_level_ = event_level::normal;
  const std::string log_directory_;
  const std::string log_path_;
  bool logging_active_ = true;
  std::mutex log_mutex_;
  std::condition_variable log_notify_;
  std::deque<std::shared_ptr<event>> event_queue_;
  std::unique_ptr<std::thread> logging_thread_;
  FILE *log_file_ = nullptr;

private:
  void check_log_roll(std::size_t count);

  void close_log_file();

  void logging_thread(bool drain);

  void process_event(const event &event);

  void reopen_log_file();
};
} // namespace repertory

#endif // INCLUDE_EVENTS_CONSUMERS_LOGGING_CONSUMER_HPP_
