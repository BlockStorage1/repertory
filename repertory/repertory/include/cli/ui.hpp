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
#ifndef REPERTORY_INCLUDE_CLI_UI_HPP_
#define REPERTORY_INCLUDE_CLI_UI_HPP_

#include "cli/common.hpp"
#include "ui/handlers.hpp"
#include "ui/mgmt_app_config.hpp"

namespace repertory::cli::actions {
[[nodiscard]] inline auto
ui(std::vector<const char *> args, const std::string & /*data_directory*/,
   const provider_type & /* prov */, const std::string & /* unique_id */,
   std::string /* user */, std::string /* password */) -> exit_code {

  ui::mgmt_app_config config{};

  std::string data;
  auto res = utils::cli::parse_string_option(
      args, utils::cli::options::ui_port_option, data);
  if (res == exit_code::success && not data.empty()) {
    config.set_api_port(utils::string::to_uint16(data));
  }

  if (not utils::file::change_to_process_directory()) {
    return exit_code::ui_mount_failed;
  }

  httplib::Server server;
  if (not server.set_mount_point("/ui", "./web")) {
    return exit_code::ui_mount_failed;
  }

  ui::handlers handlers(&config, &server);
  return exit_code::success;
}
} // namespace repertory::cli::actions

#endif // REPERTORY_INCLUDE_CLI_UI_HPP_
