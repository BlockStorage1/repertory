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
#include "events/consumers/console_consumer.hpp"

#include "events/i_event.hpp"
#include "events/types/event_level_changed.hpp"
#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace repertory {
console_consumer::console_consumer() : console_consumer(event_level::info) {}

console_consumer::console_consumer(event_level level) {
  static const auto set_level = [](auto next_level) {
    switch (next_level) {
    case event_level::critical:
      spdlog::get("console")->set_level(spdlog::level::critical);
      break;
    case event_level::error:
      spdlog::get("console")->set_level(spdlog::level::err);
      break;
    case event_level::warn:
      spdlog::get("console")->set_level(spdlog::level::warn);
      break;
    case event_level::info:
      spdlog::get("console")->set_level(spdlog::level::info);
      break;
    case event_level::debug:
      spdlog::get("console")->set_level(spdlog::level::debug);
      break;
    case event_level::trace:
      spdlog::get("console")->set_level(spdlog::level::trace);
      break;
    default:
      spdlog::get("console")->set_level(spdlog::level::info);
      break;
    }
  };

  spdlog::drop("console");
  spdlog::create_async<spdlog::sinks::stdout_color_sink_mt>("console");

  set_level(level);

  E_SUBSCRIBE_ALL(process_event);
  E_SUBSCRIBE(event_level_changed,
                    [](auto &&event) { set_level(event.new_level); });
}

console_consumer::~console_consumer() { E_CONSUMER_RELEASE(); }

void console_consumer::process_event(const i_event &evt) {
  switch (evt.get_event_level()) {
  case event_level::critical:
    spdlog::get("console")->critical(evt.get_single_line());
    break;
  case event_level::error:
    spdlog::get("console")->error(evt.get_single_line());
    break;
  case event_level::warn:
    spdlog::get("console")->warn(evt.get_single_line());
    break;
  case event_level::info:
    spdlog::get("console")->info(evt.get_single_line());
    break;
  case event_level::debug:
    spdlog::get("console")->debug(evt.get_single_line());
    break;
  case event_level::trace:
  default:
    spdlog::get("console")->trace(evt.get_single_line());
    break;
  }
}
} // namespace repertory
