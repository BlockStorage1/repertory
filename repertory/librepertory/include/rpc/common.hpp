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
#ifndef REPERTORY_INCLUDE_RPC_COMMON_HPP_
#define REPERTORY_INCLUDE_RPC_COMMON_HPP_

#include "utils/base64.hpp"
#include "utils/error_utils.hpp"
#include "utils/string.hpp"

namespace repertory::rpc {
[[nodiscard]] auto create_password_hash(std::string_view password)
    -> std::string;

[[nodiscard]] inline auto check_authorization(const auto &cfg,
                                              const httplib::Request &req)
    -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  if (cfg.get_api_password().empty() || cfg.get_api_user().empty()) {
    utils::error::raise_error(function_name,
                              "authorization user or password is not set");
    return false;
  }

  auto authorization = req.get_header_value("Authorization");
  if (authorization.empty()) {
    utils::error::raise_error(function_name,
                              "'Authorization' header is not set");
    return false;
  }

  auto auth_parts = utils::string::split(authorization, ' ', true);
  if (auth_parts.empty()) {
    utils::error::raise_error(function_name, "'Authorization' header is empty");
    return false;
  }

  auto auth_type = auth_parts[0U];
  if (auth_type != "Basic") {
    utils::error::raise_error(function_name,
                              "authorization type is not 'Basic'");
    return false;
  }

  auto data = macaron::Base64::Decode(authorization.substr(6U));
  auto auth_str = std::string(data.begin(), data.end());

  auto auth = utils::string::split(auth_str, ':', false);
  if (auth.size() != 2U) {
    utils::error::raise_error(function_name, "authorization data is not valid");
    return false;
  }

  auto user = auth.at(0U);
  auto pwd = auth.at(1U);
  if ((user != cfg.get_api_user()) ||
      (pwd != create_password_hash(cfg.get_api_password()))) {
    utils::error::raise_error(function_name, "authorization failed");
    return false;
  }

  return true;
}
} // namespace repertory::rpc

#endif // REPERTORY_INCLUDE_RPC_COMMON_HPP_
