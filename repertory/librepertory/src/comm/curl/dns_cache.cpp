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
#include "comm/curl/dns_cache.hpp"

namespace repertory {
dns_cache::curl_sh_t dns_cache::cache_;

std::unique_ptr<unique_recur_mutex_lock> dns_cache::lock_;

std::recursive_mutex dns_cache::mtx_;

void dns_cache::cleanup() { cache_.reset(nullptr); }

void dns_cache::init() {
  lock_ = std::make_unique<unique_recur_mutex_lock>(mtx_);
  auto *cache = curl_share_init();
  if (cache == nullptr) {
    return;
  }

  curl_share_setopt(cache, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
  curl_share_setopt(cache, CURLSHOPT_LOCKFUNC, lock_callback);
  curl_share_setopt(cache, CURLSHOPT_UNLOCKFUNC, unlock_callback);
  cache_.reset(cache);
}

void dns_cache::lock_callback(CURL * /* curl */, curl_lock_data data,
                              curl_lock_access /* access */, void * /* ptr */) {
  if (data != CURL_LOCK_DATA_DNS) {
    return;
  }

  lock_->lock();
}

void dns_cache::set_cache(CURL *curl) {
  curl_easy_setopt(curl, CURLOPT_SHARE, cache_.get());
}

void dns_cache::unlock_callback(CURL * /* curl */, curl_lock_data data,
                                curl_lock_access /* access */,
                                void * /* ptr */) {
  if (data != CURL_LOCK_DATA_DNS) {
    return;
  }

  lock_->unlock();
}
} // namespace repertory
