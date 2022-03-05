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
#ifndef INCLUDE_DRIVES_EVICTION_HPP_
#define INCLUDE_DRIVES_EVICTION_HPP_

#include "common.hpp"

namespace repertory {
class app_config;
class i_open_file_table;
class i_provider;
class eviction final {
public:
  eviction(i_provider &provider, const app_config &config, i_open_file_table &oft)
      : provider_(provider), config_(config), oft_(oft) {}

  ~eviction() = default;

private:
  i_provider &provider_;
  const app_config &config_;
  i_open_file_table &oft_;
  bool stop_requested_ = false;
  std::mutex start_stop_mutex_;
  std::condition_variable stop_notify_;
  std::unique_ptr<std::thread> eviction_thread_;
  std::mutex eviction_mutex_;

private:
  void check_items_thread();

  bool check_minimum_requirements(const std::string &file_path);

  std::deque<std::string> get_filtered_cached_files();

public:
  void start();

  void stop();
};
} // namespace repertory

#endif // INCLUDE_DRIVES_EVICTION_HPP_
