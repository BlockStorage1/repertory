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
#include "events/event.hpp"

#include "utils/string.hpp"

namespace repertory {
auto event_level_from_string(std::string level, event_level default_level)
    -> event_level {
  level = utils::string::to_lower(level);
  if (level == "critical" || level == "event_level::critical") {
    return event_level::critical;
  }

  if (level == "debug" || level == "event_level::debug") {
    return event_level::debug;
  }

  if (level == "warn" || level == "event_level::warn") {
    return event_level::warn;
  }

  if (level == "info" || level == "event_level::info") {
    return event_level::info;
  }

  if (level == "error" || level == "event_level::error") {
    return event_level::error;
  }

  if (level == "trace" || level == "event_level::trace") {
    return event_level::trace;
  }

  return default_level;
}

auto event_level_to_string(event_level level) -> std::string {
  switch (level) {
  case event_level::critical:
    return "critical";
  case event_level::debug:
    return "debug";
  case event_level::error:
    return "error";
  case event_level::info:
    return "info";
  case event_level::warn:
    return "warn";
  case event_level::trace:
    return "trace";
  default:
    return "info";
  }
}
} // namespace repertory
