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
#include "file_manager/upload.hpp"

#include "events/event_system.hpp"
#include "file_manager/events.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"

namespace repertory {
upload::upload(filesystem_item fsi, i_provider &provider)
    : fsi_(std::move(fsi)), provider_(provider) {
  thread_ = std::make_unique<std::thread>([this] { upload_thread(); });
}

upload::~upload() {
  stop();

  thread_->join();
  thread_.reset();
}

void upload::cancel() {
  cancelled_ = true;
  stop();
}

void upload::stop() { stop_requested_ = true; }

void upload::upload_thread() {
  REPERTORY_USES_FUNCTION_NAME();

  error_ =
      provider_.upload_file(fsi_.api_path, fsi_.source_path, stop_requested_);
  if (error_ == api_error::success &&
      not utils::file::reset_modified_time(fsi_.source_path)) {
    utils::error::raise_api_path_error(
        function_name, fsi_.api_path, fsi_.source_path,
        utils::get_last_error_code(), "failed to reset modified time");
  }

  event_system::instance().raise<file_upload_completed>(
      get_api_path(), get_source_path(), get_api_error(), cancelled_);
}
} // namespace repertory
