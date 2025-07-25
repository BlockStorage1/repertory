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
#include "comm/curl/curl_shared.hpp"
#include "utils/error.hpp"

namespace repertory {
curl_shared::curl_sh_t curl_shared::cache_;

std::recursive_mutex curl_shared::mtx_;

void curl_shared::cleanup() {
  cache_.reset(nullptr);
  curl_global_cleanup();
}

auto curl_shared::init() -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  auto res = curl_global_init(CURL_GLOBAL_ALL);
  if (res != 0) {
    utils::error::handle_error(function_name,
                               "failed to initialize curl|result|" +
                                   std::to_string(res));
    return false;
  }

  auto *cache = curl_share_init();
  if (cache == nullptr) {
    utils::error::handle_error(function_name, "failed to create curl share");
    return false;
  }
  cache_.reset(cache);

  curl_share_setopt(cache, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
  curl_share_setopt(cache, CURLSHOPT_LOCKFUNC, lock_callback);
  curl_share_setopt(cache, CURLSHOPT_UNLOCKFUNC, unlock_callback);
  return true;
}

void curl_shared::lock_callback(CURL * /* curl */, curl_lock_data data,
                                curl_lock_access /* access */,
                                void * /* ptr */) {
  if (data != CURL_LOCK_DATA_DNS) {
    return;
  }

  mtx_.lock();
}

void curl_shared::set_share(CURL *curl) {
  curl_easy_setopt(curl, CURLOPT_SHARE, cache_.get());
}

void curl_shared::unlock_callback(CURL * /* curl */, curl_lock_data data,
                                  curl_lock_access /* access */,
                                  void * /* ptr */) {
  if (data != CURL_LOCK_DATA_DNS) {
    return;
  }

  mtx_.unlock();
}
} // namespace repertory
