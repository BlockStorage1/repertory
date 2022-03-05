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
#ifndef INCLUDE_CLI_UNMOUNT_HPP_
#define INCLUDE_CLI_UNMOUNT_HPP_

#include "common.hpp"
#include "app_config.hpp"
#include "rpc/client/client.hpp"
#include "types/repertory.hpp"
#include "types/rpc.hpp"
#include "utils/cli_utils.hpp"

namespace repertory::cli::actions {
static exit_code unmount(const int &, char *[], const std::string &data_directory,
                         const provider_type &pt, const std::string &, std::string user,
                         std::string password) {
  auto ret = exit_code::success;
  auto port = app_config::default_api_port(pt);
  utils::cli::get_api_authentication_data(user, password, port, pt, data_directory);
  const auto response = client({"localhost", password, port, user}).unmount();
  std::cout << static_cast<int>(response.response_type) << std::endl;
  if (response.response_type == rpc_response_type::success) {
    std::cout << response.data.dump(2) << std::endl;
  } else {
    std::cerr << response.data.dump(2) << std::endl;
    ret = exit_code::communication_error;
  }

  return ret;
}
} // namespace repertory::cli::actions

#endif // INCLUDE_CLI_UNMOUNT_HPP_
