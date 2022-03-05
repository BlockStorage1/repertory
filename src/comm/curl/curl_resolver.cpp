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
#include "comm/curl/curl_resolver.hpp"

namespace repertory {
curl_resolver::curl_resolver(CURL *handle, std::vector<std::string> items, const bool &ignore_root)
    : items_(std::move(items)) {
#ifndef _WIN32
  if (ignore_root && (getuid() == 0)) {
    items_.clear();
  }
#endif
  for (const auto &item : items_) {
    host_list_ = curl_slist_append(host_list_, &item[0u]);
  }

  if (host_list_) {
    curl_easy_setopt(handle, CURLOPT_RESOLVE, host_list_);
  }
}

curl_resolver::~curl_resolver() {
  if (host_list_) {
    curl_slist_free_all(host_list_);
  }
}
} // namespace repertory
