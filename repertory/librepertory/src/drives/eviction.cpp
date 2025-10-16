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
#include "drives/eviction.hpp"

#include "app_config.hpp"
#include "file_manager/i_file_manager.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/time.hpp"
#include "utils/utils.hpp"

namespace repertory {
auto eviction::check_minimum_requirements(std::string_view file_path) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  auto file = utils::file::file{file_path};
  auto reference_time = file.get_time(config_.get_eviction_uses_accessed_time()
                                          ? utils::file::time_type::accessed
                                          : utils::file::time_type::modified);

  if (not reference_time.has_value()) {
    utils::error::raise_error(function_name, utils::get_last_error_code(),
                              file_path, "failed to get file time");
    return false;
  }

  auto delay =
      static_cast<std::uint64_t>(config_.get_eviction_delay_mins() * 60U) *
      utils::time::NANOS_PER_SECOND;
  return (reference_time.value() + delay) <= utils::time::get_time_now();
}

auto eviction::get_filtered_cached_files() -> std::deque<std::string> {
  auto list =
      utils::file::get_directory_files(config_.get_cache_directory(), true);
  list.erase(std::remove_if(list.begin(), list.end(),
                            [this](auto &&path) -> bool {
                              return not this->check_minimum_requirements(path);
                            }),
             list.end());
  return list;
}

void eviction::service_function() {
  REPERTORY_USES_FUNCTION_NAME();

  auto cached_files_list = get_filtered_cached_files();
  auto was_file_evicted{false};
  while (not get_stop_requested() && not cached_files_list.empty()) {
    auto file_path = cached_files_list.front();
    cached_files_list.pop_front();

    try {
      std::string api_path;
      auto res = provider_.get_api_path_from_source(file_path, api_path);
      if (res != api_error::success) {
        continue;
      }

      if (file_mgr_.evict_file(api_path)) {
        was_file_evicted = true;
      }
    } catch (const std::exception &ex) {
      utils::error::raise_error(
          function_name, ex,
          fmt::format("failed to process cached file|sp|{}", file_path));
    }
  }

  if (get_stop_requested() || was_file_evicted) {
    return;
  }

  unique_mutex_lock lock(get_mutex());
  if (get_stop_requested()) {
    return;
  }

  get_notify().wait_for(lock, 30s);
}
} // namespace repertory
