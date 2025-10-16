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
#ifndef REPERTORY_INCLUDE_CLI_CHECK_VERSION_HPP_
#define REPERTORY_INCLUDE_CLI_CHECK_VERSION_HPP_

#include "cli/common.hpp"
#include "comm/packet/packet_client.hpp"
#include "utils/utils.hpp"
#include "version.hpp"

namespace repertory::cli::actions {
[[nodiscard]] inline auto
check_version(std::string_view data_directory, provider_type prov,
              std::string_view remote_host, std::uint16_t remote_port)
    -> exit_code {
  auto ret = cli::check_data_directory(data_directory);
  if (ret != exit_code::success) {
    return ret;
  }

  if (prov == provider_type::remote) {
    app_config config(prov, data_directory);
    auto remote_cfg = config.get_remote_config();
    remote_cfg.host_name_or_ip = remote_host;
    remote_cfg.api_port = remote_port;
    config.set_remote_config(remote_cfg);
    packet_client client(config.get_remote_config());

    std::uint32_t min_version{};
    auto client_version = utils::get_version_number(project_get_version());
    auto res = client.check_version(client_version, min_version);
    return cli::handle_error(
        res == api_error::success ? exit_code::success
        : res == api_error::incompatible_version
            ? exit_code::incompatible_version
            : exit_code::communication_error,
        fmt::format("required|{}|actual|{}", min_version, client_version));
  }

  if (prov != provider_type::sia) {
    return cli::handle_error(
        exit_code::success,
        fmt::format("version not required|provider|",
                    app_config::get_provider_display_name(prov)));
  }

  app_config config(prov, data_directory);
  curl_comm comm(config.get_host_config());
  sia_provider provider(config, comm);

  std::string required_version;
  std::string returned_version;
  if (provider.check_version(required_version, returned_version)) {
    return cli::handle_error(exit_code::success,
                             fmt::format("required|{}|actual|{}",
                                         required_version, returned_version));
  }

  return cli::handle_error(
      exit_code::incompatible_version,
      fmt::format("required|{}|actual|{}", required_version, returned_version));
}
} // namespace repertory::cli::actions

#endif // REPERTORY_INCLUDE_CLI_CHECK_VERSION_HPP_
