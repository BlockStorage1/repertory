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
#ifndef INCLUDE_CLI_PINNED_STATUS_HPP_
#define INCLUDE_CLI_PINNED_STATUS_HPP_

#include "app_config.hpp"
#include "rpc/client/client.hpp"
#include "types/repertory.hpp"
#include "types/rpc.hpp"
#include "utils/cli_utils.hpp"

namespace repertory::cli::actions {
[[nodiscard]] inline auto
pinned_status(std::vector<const char *> args, const std::string &data_directory,
              const provider_type &prov, const std::string & /*unique_id*/,
              std::string user, std::string password) -> exit_code {
  std::string data;
  auto ret = utils::cli::parse_string_option(
      args, repertory::utils::cli::options::pinned_status_option, data);
  if (ret == exit_code::success) {
    auto port = app_config::default_api_port(prov);
    utils::cli::get_api_authentication_data(user, password, port, prov,
                                            data_directory);
    const auto response =
        client({"localhost", password, port, user}).pinned_status(data);
    if (response.response_type == rpc_response_type::success) {
      std::cout << response.data.dump(2) << std::endl;
    } else {
      std::cerr << response.data.dump(2) << std::endl;
      ret = exit_code::export_failed;
    }
  }

  return ret;
}
} // namespace repertory::cli::actions

#endif // INCLUDE_CLI_PINNED_STATUS_HPP_
