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
#include "comm/curl/multi_request.hpp"

#include "utils/utils.hpp"

namespace repertory {
multi_request::multi_request(CURL *curl_handle, stop_type &stop_requested)
    : curl_handle_(curl_handle),
      stop_requested_(stop_requested),
      multi_handle_(curl_multi_init()) {
  curl_multi_add_handle(multi_handle_, curl_handle);
}

multi_request::~multi_request() {
  curl_multi_remove_handle(multi_handle_, curl_handle_);
  curl_easy_cleanup(curl_handle_);
  curl_multi_cleanup(multi_handle_);
}

void multi_request::get_result(CURLcode &curl_code, long &http_code) {
  static constexpr const auto timeout_ms = 100;

  curl_code = CURLcode::CURLE_ABORTED_BY_CALLBACK;
  http_code = -1;

  auto error = false;
  int running_handles = 0;
  curl_multi_perform(multi_handle_, &running_handles);
  while (not error && (running_handles > 0) && not stop_requested_) {
    int ignored{};
    curl_multi_wait(multi_handle_, nullptr, 0, timeout_ms, &ignored);

    const auto ret = curl_multi_perform(multi_handle_, &running_handles);
    error = (ret != CURLM_CALL_MULTI_PERFORM) && (ret != CURLM_OK);
  }

  if (not stop_requested_) {
    int remaining_messages = 0;
    auto *multi_result =
        curl_multi_info_read(multi_handle_, &remaining_messages);
    if ((multi_result != nullptr) && (multi_result->msg == CURLMSG_DONE)) {
      curl_easy_getinfo(multi_result->easy_handle, CURLINFO_RESPONSE_CODE,
                        &http_code);
      curl_code = multi_result->data.result;
    }
  }
}
} // namespace repertory
