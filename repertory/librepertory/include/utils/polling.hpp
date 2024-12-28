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
#ifndef REPERTORY_INCLUDE_UTILS_POLLING_HPP_
#define REPERTORY_INCLUDE_UTILS_POLLING_HPP_

#include "types/repertory.hpp"

namespace repertory {
class app_config;

class polling final {
public:
  enum struct frequency {
    high,
    low,
    medium,
    second,
    size,
  };

  struct polling_item final {
    std::string name;
    frequency freq;
    std::function<void(const stop_type &stop_requested)> action;
  };

public:
  polling(const polling &) = delete;
  polling(polling &&) = delete;
  auto operator=(const polling &) -> polling & = delete;
  auto operator=(polling &&) -> polling & = delete;

private:
  polling() = default;

  ~polling() { stop(); }

private:
  static polling instance_;

public:
  static auto instance() -> polling & { return instance_; }

private:
  app_config *config_{nullptr};
  std::array<std::unique_ptr<std::thread>,
             static_cast<std::size_t>(frequency::size)>
      frequency_threads_;
  std::unordered_map<std::string, polling_item> items_;
  std::mutex mutex_;
  std::condition_variable notify_;
  std::mutex start_stop_mutex_;
  stop_type stop_requested_{false};

private:
  void frequency_thread(std::function<std::uint32_t()> get_frequency_seconds,
                        frequency freq);

public:
  void remove_callback(const std::string &name);

  void set_callback(const polling_item &item);

  void start(app_config *config);

  void stop();
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_UTILS_POLLING_HPP_
