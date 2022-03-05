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
#include "drives/eviction.hpp"
#include "app_config.hpp"
#include "drives/i_open_file_table.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "utils/file_utils.hpp"
#include "utils/global_data.hpp"

namespace repertory {
void eviction::check_items_thread() {
  while (not stop_requested_) {
    auto should_evict = true;

    // Handle maximum cache size eviction
    auto used_bytes = global_data::instance().get_used_cache_space();
    if (config_.get_enable_max_cache_size()) {
      should_evict = (used_bytes > config_.get_max_cache_size_bytes());
    }

    // Evict all items if minimum redundancy eviction is enabled; otherwise, evict
    //  until required space is reclaimed.
    if (should_evict) {
      // Remove cached source files that don't meet minimum requirements
      auto cached_files_list = get_filtered_cached_files();
      if (not cached_files_list.empty()) {
        while (not stop_requested_ && should_evict && not cached_files_list.empty()) {
          std::string api_path;
          if (provider_.get_api_path_from_source(cached_files_list.front(), api_path) ==
              api_error::success) {
            std::string pinned;
            provider_.get_item_meta(api_path, META_PINNED, pinned);

            api_file file{};
            filesystem_item fsi{};
            if ((pinned.empty() || not utils::string::to_bool(pinned)) &&
                provider_.get_filesystem_item_and_file(api_path, file, fsi) == api_error::success) {
              // Only evict files that match expected size
              std::uint64_t file_size = 0u;
              utils::file::get_file_size(cached_files_list.front(), file_size);
              if (file_size == fsi.size) {
                // Ensure minimum file redundancy has been met or source path is not being
                // used for local recovery
                const auto different_source = file.source_path != fsi.source_path;
                if (file.recoverable &&
                    ((file.redundancy >= config_.get_minimum_redundancy()) || different_source)) {
                  // Try to evict file
                  if (oft_.evict_file(fsi.api_path) && config_.get_enable_max_cache_size()) {
                    // Restrict number of items evicted if maximum cache size is enabled
                    used_bytes -= file_size;
                    should_evict = (used_bytes > config_.get_max_cache_size_bytes());
                  }
                }
              }
            }
          }
          cached_files_list.pop_front();
        }
      }
    }

    if (not stop_requested_) {
      unique_mutex_lock l(eviction_mutex_);
      if (not stop_requested_) {
        stop_notify_.wait_for(l, 30s);
      }
    }
  }
}

bool eviction::check_minimum_requirements(const std::string &file_path) {
  auto ret = false;
  // Only evict cachedFileList that are > 0
  std::uint64_t file_size = 0u;
  utils::file::get_file_size(file_path, file_size);
  if (file_size) {
    // Check modified/accessed date/time
    std::uint64_t reference_time = 0u;
    if ((ret = config_.get_eviction_uses_accessed_time()
                   ? utils::file::get_accessed_time(file_path, reference_time)
                   : utils::file::get_modified_time(file_path, reference_time))) {
#ifdef _WIN32
      const auto now = std::chrono::system_clock::now();
      const auto delay = std::chrono::minutes(config_.get_eviction_delay_mins());
      ret = ((std::chrono::system_clock::from_time_t(reference_time) + delay) <= now);
#else
      const auto now = utils::get_time_now();
      const auto delay = (config_.get_eviction_delay_mins() * 60L) * NANOS_PER_SECOND;
      ret = ((reference_time + delay) <= now);
#endif
    }
  }

  return ret;
}

std::deque<std::string> eviction::get_filtered_cached_files() {
  auto list = utils::file::get_directory_files(config_.get_cache_directory(), true);
  list.erase(std::remove_if(list.begin(), list.end(),
                            [this](const std::string &path) -> bool {
                              return not this->check_minimum_requirements(path);
                            }),
             list.end());
  return list;
}

void eviction::start() {
  mutex_lock l(start_stop_mutex_);
  if (not eviction_thread_) {
    stop_requested_ = false;
    eviction_thread_ = std::make_unique<std::thread>([this] { this->check_items_thread(); });
  }
}

void eviction::stop() {
  mutex_lock l(start_stop_mutex_);
  if (eviction_thread_) {
    event_system::instance().raise<service_shutdown>("eviction");
    stop_requested_ = true;
    {
      mutex_lock l2(eviction_mutex_);
      stop_notify_.notify_all();
    }
    eviction_thread_->join();
    eviction_thread_.reset();
  }
}
} // namespace repertory
