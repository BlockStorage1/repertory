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
#ifndef REPERTORY_INCLUDE_DRIVES_EVICTION_HPP_
#define REPERTORY_INCLUDE_DRIVES_EVICTION_HPP_

#include "utils/single_thread_service_base.hpp"

namespace repertory {
class app_config;
class i_file_manager;
class i_provider;

class eviction final : public single_thread_service_base {
public:
  eviction(i_provider &provider, const app_config &config,
           i_file_manager &file_mgr)
      : single_thread_service_base("eviction"),
        config_(config),
        file_mgr_(file_mgr),
        provider_(provider) {}

  ~eviction() override = default;

private:
  const app_config &config_;
  i_file_manager &file_mgr_;
  i_provider &provider_;

private:
  [[nodiscard]] auto check_minimum_requirements(const std::string &file_path)
      -> bool;

  [[nodiscard]] auto get_filtered_cached_files() -> std::deque<std::string>;

protected:
  void service_function() override;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_DRIVES_EVICTION_HPP_
