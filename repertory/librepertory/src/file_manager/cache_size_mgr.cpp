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
#include "file_manager/cache_size_mgr.hpp"

#include "app_config.hpp"
#include "events/event.hpp"
#include "events/event_system.hpp"
#include "types/startup_exception.hpp"
#include "utils/file_utils.hpp"

namespace repertory {
// clang-format off
E_SIMPLE2(invalid_cache_size, warn, true,
  std::uint64_t, cache_size, sz, E_FROM_UINT64,
  std::uint64_t, by, by, E_FROM_UINT64 
);

E_SIMPLE2(max_cache_size_reached, warn, true,
  std::uint64_t, cache_size, sz, E_FROM_UINT64,
  std::uint64_t, max_cache_size, max, E_FROM_UINT64 
);
// clang-format on

cache_size_mgr cache_size_mgr::instance_{};

// TODO add timeout
auto cache_size_mgr::expand(std::uint64_t size) -> api_error {
  if (size == 0U) {
    return api_error::success;
  }

  unique_mutex_lock lock(mtx_);
  if (cfg_ == nullptr) {
    return api_error::cache_not_initialized;
  }

  cache_size_ += size;

  auto max_cache_size = cfg_->get_max_cache_size_bytes();

  auto cache_dir = utils::file::directory{cfg_->get_cache_directory()};
  while (not stop_requested_ && cache_size_ > max_cache_size &&
         cache_dir.count() > 1U) {
    event_system::instance().raise<max_cache_size_reached>(cache_size_,
                                                           max_cache_size);
    notify_.wait(lock);
  }

  notify_.notify_all();

  return api_error::success;
}

void cache_size_mgr::initialize(app_config *cfg) {
  if (cfg == nullptr) {
    throw startup_exception("app_config must not be null");
  }

  mutex_lock lock(mtx_);
  cfg_ = cfg;

  stop_requested_ = false;

  auto cache_dir = utils::file::directory{cfg_->get_cache_directory()};
  if (not cache_dir.create_directory()) {
    throw startup_exception(fmt::format("failed to create cache directory|{}",
                                        cache_dir.get_path()));
  }

  cache_size_ = cache_dir.size(false);

  notify_.notify_all();
}

auto cache_size_mgr::shrink(std::uint64_t size) -> api_error {
  mutex_lock lock(mtx_);
  if (size == 0U) {
    notify_.notify_all();
    return api_error::success;
  }

  if (cache_size_ >= size) {
    cache_size_ -= size;
  } else {
    event_system::instance().raise<invalid_cache_size>(cache_size_, size);
    cache_size_ = 0U;
  }

  notify_.notify_all();

  return api_error::success;
}

auto cache_size_mgr::size() const -> std::uint64_t {
  mutex_lock lock(mtx_);
  return cache_size_;
}

void cache_size_mgr::stop() {
  if (stop_requested_) {
    return;
  }

  stop_requested_ = true;

  mutex_lock lock(mtx_);
  notify_.notify_all();
}
} // namespace repertory
