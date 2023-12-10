/*
  Copyright <2018-2023> <scott.e.graves@protonmail.com>

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
#ifndef INCLUDE_CLI_SET_HPP_
#define INCLUDE_CLI_SET_HPP_

#include "app_config.hpp"
#include "platform/platform.hpp"
#include "rpc/client/client.hpp"
#include "types/repertory.hpp"
#include "types/rpc.hpp"
#include "utils/cli_utils.hpp"

namespace repertory::cli::actions {
[[nodiscard]] inline auto set(std::vector<const char *> args,
                              const std::string &data_directory,
                              const provider_type &prov,
                              const std::string &unique_id, std::string user,
                              std::string password) -> exit_code {
  auto ret = exit_code::success;
  auto data = utils::cli::parse_option(args, "-set", 2U);
  if (data.empty()) {
    data = utils::cli::parse_option(args, "--set", 2U);
    if (data.empty()) {
      ret = exit_code::invalid_syntax;
      std::cerr << "Invalid syntax for '-set'" << std::endl;
    }
  }
  if (ret == exit_code::success) {
    lock_data lock(prov, unique_id);
    const auto res = lock.grab_lock(1U);
    if (res == lock_result::success) {
      app_config config(prov, data_directory);
      const auto value = config.set_value_by_name(data[0U], data[1U]);
      const auto notFound = value.empty() && not data[1U].empty();
      ret = notFound ? exit_code::set_option_not_found : exit_code::success;
      std::cout << (notFound ? static_cast<int>(
                                   rpc_response_type::config_value_not_found)
                             : 0)
                << std::endl;
      std::cout << json({{"value", value}}).dump(2) << std::endl;
    } else if (res == lock_result::locked) {
      auto port = app_config::default_api_port(prov);
      utils::cli::get_api_authentication_data(user, password, port, prov,
                                              data_directory);
      const auto response = client({"localhost", password, port, user})
                                .set_config_value_by_name(data[0U], data[1U]);
      std::cout << static_cast<int>(response.response_type) << std::endl;
      std::cout << response.data.dump(2) << std::endl;
      ret = response.response_type == rpc_response_type::config_value_not_found
                ? exit_code::set_option_not_found
            : response.response_type == rpc_response_type::success
                ? exit_code::success
                : exit_code::communication_error;
    }
  }

  return ret;
}
} // namespace repertory::cli::actions

#endif // INCLUDE_CLI_SET_HPP_
