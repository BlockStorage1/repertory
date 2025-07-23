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
#ifndef REPERTORY_INCLUDE_COMM_CURL_DNS_CACHE_HPP_
#define REPERTORY_INCLUDE_COMM_CURL_DNS_CACHE_HPP_

namespace repertory {
class dns_cache final {
private:
  struct curl_sh_deleter final {
    void operator()(CURLSH *ptr) {
      if (ptr != nullptr) {
        curl_share_cleanup(ptr);
      }
    }
  };

  using curl_sh_t = std::unique_ptr<CURLSH, curl_sh_deleter>;

public:
  dns_cache() = delete;
  dns_cache(const dns_cache &) = delete;
  dns_cache(dns_cache &&) = delete;
  ~dns_cache() = delete;

  auto operator=(const dns_cache &) -> dns_cache & = delete;
  auto operator=(dns_cache &&) -> dns_cache & = delete;

private:
  static curl_sh_t cache_;
  static std::unique_ptr<unique_recur_mutex_lock> lock_;
  static std::recursive_mutex mtx_;

private:
  static auto create_cache() -> CURLSH *;

  static void lock_callback(CURL * /* curl */, curl_lock_data /* data */,
                            curl_lock_access /* access */, void * /* ptr */);

  static void unlock_callback(CURL * /* curl */, curl_lock_data /* data */,
                              curl_lock_access /* access */, void * /* ptr */);

public:
  static void set_cache(CURL *curl);
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_COMM_CURL_DNS_CACHE_HPP_
