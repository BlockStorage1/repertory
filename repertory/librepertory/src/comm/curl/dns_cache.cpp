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
dns_cache::curl_sh_t dns_cache::cache_{dns_cache::create_cache()};

std::unique_ptr<unique_recur_mutex_lock> dns_cache::lock_;

std::recursive_mutex dns_cache::mtx_;

auto dns_cache::create_cache() -> CURLSH * {
  auto *ret = curl_share_init();
  if (ret == nullptr) {
    return ret;
  }

  lock_ = std::make_unique<unique_recur_mutex_lock>(mtx_);

  curl_share_setopt(ret, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
  curl_share_setopt(ret, CURLSHOPT_LOCKFUNC, lock_callback);
  curl_share_setopt(ret, CURLSHOPT_UNLOCKFUNC, unlock_callback);

  return ret;
}

void dns_cache::lock_callback(CURL * /* curl */, curl_lock_data /* data */,
                              curl_lock_access /* access */, void * /* ptr */) {
  lock_->lock();
}

void dns_cache::set_cache(CURL *curl) {
  curl_easy_setopt(curl, CURLOPT_SHARE, cache_.get());
}

void dns_cache::unlock_callback(CURL * /* curl */, curl_lock_data /* data */,
                                curl_lock_access /* access */,
                                void * /* ptr */) {
  lock_->unlock();
}
} // namespace repertory
