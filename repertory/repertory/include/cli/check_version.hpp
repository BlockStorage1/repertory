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

namespace repertory::cli::actions {
[[nodiscard]] inline auto check_version(std::vector<const char *> /* args */,
                                        const std::string &data_directory,
                                        const provider_type &prov,
                                        const std::string & /*unique_id*/,
                                        std::string /*user*/,
                                        std::string /*password*/) -> exit_code {
  if (prov != provider_type::sia) {
    fmt::println("Success:\n\tNo specific version is required for {} providers",
                 app_config::get_provider_display_name(prov));
    return exit_code::success;
  }

  app_config config(prov, data_directory);
  curl_comm comm(config.get_host_config());
  sia_provider provider(config, comm);

  std::string required_version;
  std::string returned_version;
  if (provider.check_version(required_version, returned_version)) {
    fmt::println("0\nSuccess:\n\tRequired: {}\n\tActual: {}", required_version,
                 returned_version);
    return exit_code::success;
  }

  fmt::println("1\nFailed:\n\tRequired: {}\n\tActual: {}", required_version,
               returned_version);
  return exit_code::incompatible_version;
}
} // namespace repertory::cli::actions

#endif // REPERTORY_INCLUDE_CLI_CHECK_VERSION_HPP_
