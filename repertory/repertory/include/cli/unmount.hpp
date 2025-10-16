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
#ifndef REPERTORY_INCLUDE_CLI_UNMOUNT_HPP_
#define REPERTORY_INCLUDE_CLI_UNMOUNT_HPP_

#include "cli/common.hpp"

namespace repertory::cli::actions {
[[nodiscard]] inline auto
unmount(std::vector<const char *> /* args */, std::string_view data_directory,
        provider_type prov, std::string_view /* unique_id */, std::string user,
        std::string password) -> exit_code {
  constexpr const std::uint8_t retry_count{30U};

  auto port{app_config::default_api_port(prov)};
  utils::cli::get_api_authentication_data(user, password, port, data_directory);
  auto response = client({
                             .host = "localhost",
                             .password = password,
                             .port = port,
                             .user = user,
                         })
                      .unmount();
  if (response.response_type == rpc_response_type::success) {
    auto orig_response{response};
    std::cout << "Waiting for unmount..." << std::flush;
    for (std::uint8_t retry{0U};
         retry < retry_count &&
         response.response_type == rpc_response_type::success;
         ++retry) {
      std::this_thread::sleep_for(1s);
      response = client({
                            .host = "localhost",
                            .password = password,
                            .port = port,
                            .user = user,
                        })
                     .unmount();
      std::cout << "." << std::flush;
    }

    if (response.response_type == rpc_response_type::success) {
      std::cout << "failed!" << std::endl;
      return cli::handle_error(exit_code::mount_active,
                               "mount is still active");
    }

    std::cout << " done!" << std::endl;
    return cli::handle_error(exit_code::success, orig_response.response_type,
                             orig_response.data.dump(2));
  }

  return cli::handle_error(exit_code::communication_error,
                           response.response_type, response.data.dump(2));
}
} // namespace repertory::cli::actions

#endif // REPERTORY_INCLUDE_CLI_UNMOUNT_HPP_
