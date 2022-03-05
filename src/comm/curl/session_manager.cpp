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
#include "comm/curl/session_manager.hpp"
#include "app_config.hpp"
#include "comm/curl/curl_comm.hpp"
#include "utils/utils.hpp"

namespace repertory {
bool session_manager::create_auth_session(CURL *&curl_handle, const app_config &config,
                                          host_config hc, std::string &session) {
  auto ret = true;
  if (not curl_handle) {
    curl_handle = utils::create_curl();
  }

  if (not hc.auth_url.empty() && not hc.auth_user.empty()) {
    mutex_lock l(session_mutex_);
    if (session_.empty()) {
      if ((ret = curl_comm::create_auth_session(curl_handle, config, hc, session))) {
        session_ = session;
      }
    } else {
      session = session_;
      curl_comm::update_auth_session(curl_handle, config, session_);
    }

    if (ret) {
      session_count_++;
    }
  }

  return ret;
}

void session_manager::release_auth_session(const app_config &config, host_config hc,
                                           const std::string &session) {
  if (not hc.auth_url.empty() && not hc.auth_user.empty()) {
    mutex_lock l(session_mutex_);
    if (not session_.empty() && (session == session_) && session_count_) {
      if (not --session_count_) {
        curl_comm::release_auth_session(config, hc, session_);
        session_.clear();
      }
    }
  }
}

void session_manager::update_auth_session(CURL *curl_handle, const app_config &config,
                                          const host_config &hc) {
  mutex_lock l(session_mutex_);
  if (hc.auth_url.empty() || hc.auth_user.empty() || session_.empty()) {
    return;
  }

  curl_comm::update_auth_session(curl_handle, config, session_);
}
} // namespace repertory
