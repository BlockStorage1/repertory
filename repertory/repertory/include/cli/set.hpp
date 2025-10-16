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
#ifndef REPERTORY_INCLUDE_CLI_SET_HPP_
#define REPERTORY_INCLUDE_CLI_SET_HPP_

#include "cli/common.hpp"

namespace repertory::cli::actions {
[[nodiscard]] inline auto set(std::vector<const char *> args,
                              std::string_view data_directory,
                              provider_type prov, std::string_view unique_id,
                              std::string user, std::string password)
    -> exit_code {
  auto ret = cli::check_data_directory(data_directory);
  if (ret != exit_code::success) {
    return ret;
  }

  auto data = utils::cli::parse_option(args, "-set", 2U);
  if (data.empty()) {
    data = utils::cli::parse_option(args, "--set", 2U);
  }

  if (data.empty()) {
    return cli::handle_error(exit_code::invalid_syntax,
                             "missing option name or option data");
  }

  lock_data lock(data_directory, prov, unique_id);
  auto lock_res = lock.grab_lock(1U);
  if (lock_res == lock_result::success) {
    app_config config(prov, data_directory);
    auto value = config.set_value_by_name(data[0U], data[1U]);
    auto notFound = value.empty() && not data[1U].empty();
    return cli::handle_error(
        notFound ? exit_code::set_option_not_found : exit_code::success,
        notFound ? rpc_response_type::config_value_not_found
                 : rpc_response_type::success,
        json({{"value", value}}).dump(2));
  }

  if (lock_res == lock_result::locked) {
    auto port = app_config::default_api_port(prov);
    utils::cli::get_api_authentication_data(user, password, port,
                                            data_directory);
    auto response = client({
                               .host = "localhost",
                               .password = password,
                               .port = port,
                               .user = user,
                           })
                        .set_config_value_by_name(data[0U], data[1U]);
    return cli::handle_error(
        response.response_type == rpc_response_type::config_value_not_found
            ? exit_code::set_option_not_found
        : response.response_type == rpc_response_type::success
            ? exit_code::success
            : exit_code::communication_error,
        response.response_type, response.data.dump(2));
  }

  return cli::handle_error(exit_code::lock_failed, "failed to get mount lock");
}
} // namespace repertory::cli::actions

#endif // REPERTORY_INCLUDE_CLI_SET_HPP_
