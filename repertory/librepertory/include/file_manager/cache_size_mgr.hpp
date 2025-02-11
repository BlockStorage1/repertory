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
#ifndef REPERTORY_INCLUDE_FILE_MANAGER_CACHE_SIZE_MGR_HPP_
#define REPERTORY_INCLUDE_FILE_MANAGER_CACHE_SIZE_MGR_HPP_

#include "types/repertory.hpp"

namespace repertory {
class app_config;

class cache_size_mgr final {
private:
  static constexpr const std::chrono::seconds cache_wait_secs{
      5s,
  };

public:
  cache_size_mgr(const cache_size_mgr &) = delete;
  cache_size_mgr(cache_size_mgr &&) = delete;
  auto operator=(const cache_size_mgr &) -> cache_size_mgr & = delete;
  auto operator=(cache_size_mgr &&) -> cache_size_mgr & = delete;

protected:
  cache_size_mgr() = default;

  ~cache_size_mgr() { stop(); }

private:
  static cache_size_mgr instance_;

private:
  app_config *cfg_{nullptr};
  std::uint64_t cache_size_{0U};
  mutable std::mutex mtx_;
  std::condition_variable notify_;
  stop_type stop_requested_{false};

private:
  [[nodiscard]] auto get_stop_requested() const -> bool;

public:
  [[nodiscard]] auto expand(std::uint64_t size) -> api_error;

  void initialize(app_config *cfg);

  [[nodiscard]] static auto instance() -> cache_size_mgr & { return instance_; }

  [[nodiscard]] auto shrink(std::uint64_t size) -> api_error;

  [[nodiscard]] auto size() const -> std::uint64_t;

  void stop();
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_FILE_MANAGER_CACHE_SIZE_MGR_HPP_
