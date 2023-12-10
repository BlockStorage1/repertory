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
#include "events/event.hpp"

#include "utils/string_utils.hpp"

namespace repertory {
auto event_level_from_string(std::string level) -> event_level {
  level = utils::string::to_lower(level);
  if (level == "debug" || level == "event_level::debug") {
    return event_level::debug;
  } else if (level == "warn" || level == "event_level::warn") {
    return event_level::warn;
  } else if (level == "normal" || level == "event_level::normal") {
    return event_level::normal;
  } else if (level == "error" || level == "event_level::error") {
    return event_level::error;
  } else if (level == "verbose" || level == "event_level::verbose") {
    return event_level::verbose;
  }
  return event_level::normal;
}

auto event_level_to_string(const event_level &level) -> std::string {
  switch (level) {
  case event_level::debug:
    return "debug";
  case event_level::error:
    return "error";
  case event_level::normal:
    return "normal";
  case event_level::warn:
    return "warn";
  case event_level::verbose:
    return "verbose";
  default:
    return "normal";
  }
}
} // namespace repertory
