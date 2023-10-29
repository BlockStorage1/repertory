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
#ifndef INCLUDE_EVENTS_EVENT_HPP_
#define INCLUDE_EVENTS_EVENT_HPP_

namespace repertory {
enum class event_level { error, warn, normal, debug, verbose };

auto event_level_from_string(std::string level) -> event_level;

auto event_level_to_string(const event_level &level) -> std::string;

class event {
protected:
  explicit event(bool allow_async) : allow_async_(allow_async) {}

  event(const std::stringstream &ss, json j, bool allow_async)
      : allow_async_(allow_async), ss_(ss.str()), j_(std::move(j)) {}

public:
  virtual ~event() = default;

private:
  const bool allow_async_;

protected:
  std::stringstream ss_;
  json j_;

public:
  [[nodiscard]] virtual auto clone() const -> std::shared_ptr<event> = 0;

  [[nodiscard]] auto get_allow_async() const -> bool { return allow_async_; }

  [[nodiscard]] virtual auto get_event_level() const -> event_level = 0;

  [[nodiscard]] auto get_json() const -> json { return j_; }

  [[nodiscard]] virtual auto get_name() const -> std::string = 0;

  [[nodiscard]] virtual auto get_single_line() const -> std::string = 0;
};
} // namespace repertory

#endif // INCLUDE_EVENTS_EVENT_HPP_
