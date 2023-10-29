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
#include "drives/eviction.hpp"

#include "app_config.hpp"
#include "file_manager/i_file_manager.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/unix/unix_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
auto eviction::check_minimum_requirements(const std::string &file_path)
    -> bool {
  std::uint64_t file_size{};
  if (not utils::file::get_file_size(file_path, file_size)) {
    utils::error::raise_error(__FUNCTION__, utils::get_last_error_code(),
                              file_path, "failed to get file size");
    return false;
  }

  auto ret = false;
  if (file_size) {
    std::uint64_t reference_time{};
    if ((ret =
             config_.get_eviction_uses_accessed_time()
                 ? utils::file::get_accessed_time(file_path, reference_time)
                 : utils::file::get_modified_time(file_path, reference_time))) {
#ifdef _WIN32
      const auto now = std::chrono::system_clock::now();
      const auto delay =
          std::chrono::minutes(config_.get_eviction_delay_mins());
      ret = ((std::chrono::system_clock::from_time_t(reference_time) + delay) <=
             now);
#else
      const auto now = utils::get_time_now();
      const auto delay =
          (config_.get_eviction_delay_mins() * 60L) * NANOS_PER_SECOND;
      ret = ((reference_time + delay) <= now);
#endif
    }
  }

  return ret;
}

auto eviction::get_filtered_cached_files() -> std::deque<std::string> {
  auto list =
      utils::file::get_directory_files(config_.get_cache_directory(), true);
  list.erase(std::remove_if(list.begin(), list.end(),
                            [this](const std::string &path) -> bool {
                              return not this->check_minimum_requirements(path);
                            }),
             list.end());
  return list;
}

void eviction::service_function() {
  auto should_evict = true;

  // Handle maximum cache size eviction
  auto used_bytes =
      utils::file::calculate_used_space(config_.get_cache_directory(), false);
  if (config_.get_enable_max_cache_size()) {
    should_evict = (used_bytes > config_.get_max_cache_size_bytes());
  }

  if (should_evict) {
    // Remove cached source files that don't meet minimum requirements
    auto cached_files_list = get_filtered_cached_files();
    while (not get_stop_requested() && should_evict &&
           not cached_files_list.empty()) {
      try {
        std::string api_path;
        if (provider_.get_api_path_from_source(
                cached_files_list.front(), api_path) == api_error::success) {
          api_file file{};
          filesystem_item fsi{};
          if (provider_.get_filesystem_item_and_file(api_path, file, fsi) ==
              api_error::success) {
            // Only evict files that match expected size
            std::uint64_t file_size{};
            if (utils::file::get_file_size(cached_files_list.front(),
                                           file_size)) {
              if (file_size == fsi.size) {
                // Try to evict file
                if (fm_.evict_file(fsi.api_path) &&
                    config_.get_enable_max_cache_size()) {
                  // Restrict number of items evicted if maximum cache size is
                  // enabled
                  used_bytes -= file_size;
                  should_evict =
                      (used_bytes > config_.get_max_cache_size_bytes());
                }
              }
            } else {
              utils::error::raise_api_path_error(
                  __FUNCTION__, file.api_path, file.source_path,
                  utils::get_last_error_code(), "failed to get file size");
            }
          }
        }
      } catch (const std::exception &ex) {
        utils::error::raise_error(__FUNCTION__, ex,
                                  "failed to process cached file|sp|" +
                                      cached_files_list.front());
      }

      cached_files_list.pop_front();
    }
  }

  if (not get_stop_requested()) {
    unique_mutex_lock lock(get_mutex());
    if (not get_stop_requested()) {
      get_notify().wait_for(lock, 30s);
    }
  }
}
} // namespace repertory
