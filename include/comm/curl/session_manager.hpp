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
#ifndef INCLUDE_COMM_CURL_SESSION_MANAGER_HPP_
#define INCLUDE_COMM_CURL_SESSION_MANAGER_HPP_

#include "common.hpp"
#include "types/repertory.hpp"

namespace repertory {
class app_config;
class session_manager final {
private:
  std::string session_;
  std::uint64_t session_count_ = 0u;
  std::mutex session_mutex_;

public:
  bool create_auth_session(CURL *&curl_handle, const app_config &config, host_config hc,
                           std::string &session);

  void release_auth_session(const app_config &config, host_config hc, const std::string &session);

  void update_auth_session(CURL *curl_handle, const app_config &config, const host_config &hc);
};
} // namespace repertory

#endif // INCLUDE_COMM_CURL_SESSION_MANAGER_HPP_
