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
#ifndef INCLUDE_DOWNLOAD_UTILS_HPP_
#define INCLUDE_DOWNLOAD_UTILS_HPP_

#include "common.hpp"
#include "app_config.hpp"

namespace repertory::utils::download {
template <typename event_type>
void notify_progress(const app_config &config, const std::string &api_path,
                     const std::string &source_path, const double &current, const double &total,
                     double &progress) {
  if (config.get_event_level() >= event_type::level) {
    const double next_progress = (current / total) * 100.0;
    if ((next_progress == 0.0) || (next_progress >= (progress + 0.2)) ||
        ((next_progress == 100.00) && (next_progress != progress))) {
      progress = next_progress;
      event_system::instance().raise<event_type>(api_path, source_path, progress);
    }
  }
}
} // namespace repertory::utils::download

#endif // INCLUDE_DOWNLOAD_UTILS_HPP_
