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
#ifndef REPERTORY_INCLUDE_COMM_CURL_CURL_SHARED_HPP_
#define REPERTORY_INCLUDE_COMM_CURL_CURL_SHARED_HPP_

namespace repertory {
class curl_shared final {
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
  curl_shared() = delete;
  curl_shared(const curl_shared &) = delete;
  curl_shared(curl_shared &&) = delete;
  ~curl_shared() = delete;

  auto operator=(const curl_shared &) -> curl_shared & = delete;
  auto operator=(curl_shared &&) -> curl_shared & = delete;

private:
  static curl_sh_t cache_;
  static std::recursive_mutex mtx_;

private:
  static void lock_callback(CURL * /* curl */, curl_lock_data /* data */,
                            curl_lock_access /* access */, void * /* ptr */);

  static void unlock_callback(CURL * /* curl */, curl_lock_data /* data */,
                              curl_lock_access /* access */, void * /* ptr */);

public:
  static void cleanup();

  [[nodiscard]] static auto init() -> bool;

  static void set_share(CURL *curl);
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_COMM_CURL_DNS_CACHE_HPP_
