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
#ifndef INCLUDE_CLI_TEST_SKYNET_AUTH_HPP_
#define INCLUDE_CLI_TEST_SKYNET_AUTH_HPP_
#ifdef REPERTORY_ENABLE_SKYNET

#include "common.hpp"
#include "comm/curl/curl_comm.hpp"
#include "app_config.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/event_system.hpp"
#include "types/repertory.hpp"
#include "utils/cli_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/utils.hpp"

namespace repertory::cli::actions {
static exit_code test_skynet_auth(const int &argc, char *argv[], const std::string &,
                                  const provider_type &, const std::string &, std::string,
                                  std::string) {
  auto ret = exit_code::success;
  auto data = utils::cli::parse_option(argc, argv, "-tsa", 5u);
  if (data.empty()) {
    data = utils::cli::parse_option(argc, argv, "--test_skynet_auth", 5u);
    if (data.empty()) {
      ret = exit_code::invalid_syntax;
      std::cerr << "Invalid syntax for '-tsa'" << std::endl;
    }
  }

  const auto uuid = utils::create_uuid_string();
  const auto config_directory = utils::path::absolute(utils::path::combine("./", {uuid}));
  if (ret == exit_code::success) {
    utils::file::change_to_process_directory();
    {
      console_consumer c;
      event_system::instance().start();
      app_config config(provider_type::skynet, config_directory);

      host_config hc{};
      hc.auth_url = data[0];
      hc.auth_user = data[1];
      hc.auth_password = data[2];
      hc.agent_string = data[3];
      hc.api_password = data[4];

      curl_comm comm(config);
      std::string session;
      CURL *curl_handle = nullptr;
      ret = (comm.create_auth_session(curl_handle, config, hc, session))
                ? exit_code::success
                : exit_code::communication_error;
      if (ret == exit_code::success) {
        curl_easy_cleanup(curl_handle);
        comm.release_auth_session(config, hc, session);
      }

      event_system::instance().stop();
    }

    if (ret == exit_code::success) {
      std::cout << std::endl << "Authentication Succeeded!" << std::endl;
    } else {
      std::cerr << std::endl << "Authentication Failed!" << std::endl;
    }
  }
  utils::file::delete_directory_recursively(config_directory);

  return ret;
}
} // namespace repertory::cli::actions

#endif // REPERTORY_ENABLE_SKYNET
#endif // INCLUDE_CLI_TEST_SKYNET_AUTH_HPP_
