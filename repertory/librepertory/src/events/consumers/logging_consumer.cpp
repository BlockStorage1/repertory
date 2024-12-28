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
#include "events/consumers/logging_consumer.hpp"

#include "events/events.hpp"
#include "spdlog/async.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/spdlog.h"
#include "utils/path.hpp"

namespace repertory {
logging_consumer::logging_consumer(event_level level,
                                   std::string log_directory) {
  log_directory = utils::path::absolute(log_directory);

  static const auto set_level = [](auto next_level) {
    switch (next_level) {
    case event_level::critical:
      spdlog::get("file")->set_level(spdlog::level::critical);
      break;
    case event_level::error:
      spdlog::get("file")->set_level(spdlog::level::err);
      break;
    case event_level::warn:
      spdlog::get("file")->set_level(spdlog::level::warn);
      break;
    case event_level::info:
      spdlog::get("file")->set_level(spdlog::level::info);
      break;
    case event_level::debug:
      spdlog::get("file")->set_level(spdlog::level::debug);
      break;
    case event_level::trace:
      spdlog::get("file")->set_level(spdlog::level::trace);
      break;
    default:
      spdlog::get("file")->set_level(spdlog::level::info);
      break;
    }
  };

  spdlog::drop("file");
  spdlog::create_async<spdlog::sinks::rotating_file_sink_mt>(
      "file", utils::path::combine(log_directory, {"repertory.log"}),
      MAX_LOG_FILE_SIZE, MAX_LOG_FILES);

  set_level(level);

  E_SUBSCRIBE_ALL(process_event);
  E_SUBSCRIBE_EXACT(event_level_changed,
                    [](const event_level_changed &changed) {
                      set_level(event_level_from_string(
                          changed.get_new_event_level().get<std::string>()));
                    });
}

logging_consumer::~logging_consumer() { E_CONSUMER_RELEASE(); }

void logging_consumer::process_event(const event &event) const {
  switch (event.get_event_level()) {
  case event_level::critical:
    spdlog::get("file")->critical(event.get_single_line());
    break;
  case event_level::error:
    spdlog::get("file")->error(event.get_single_line());
    break;
  case event_level::warn:
    spdlog::get("file")->warn(event.get_single_line());
    break;
  case event_level::info:
    spdlog::get("file")->info(event.get_single_line());
    break;
  case event_level::debug:
    spdlog::get("file")->debug(event.get_single_line());
    break;
  case event_level::trace:
  default:
    spdlog::get("file")->trace(event.get_single_line());
    break;
  }
}
} // namespace repertory
