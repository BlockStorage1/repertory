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
#ifndef INCLUDE_CLI_CHECK_VERSION_HPP_
#define INCLUDE_CLI_CHECK_VERSION_HPP_

#include "app_config.hpp"
#include "types/repertory.hpp"

namespace repertory::cli::actions {
[[nodiscard]] inline auto
check_version(std::vector<const char *> /* args */,
              const std::string & /* data_directory */,
              const provider_type & /* pt */, const std::string & /*unique_id*/,
              std::string /*user*/, std::string /*password*/) -> exit_code {
  auto ret = exit_code::success;

  // TODO need to updated way to check version
  // if (not((pt == provider_type::remote) || (pt == provider_type::s3))) {
  //   app_config config(pt, data_directory);
  //   curl_comm comm(config);
  //   json data, err;
  //
  //   if (comm.get("/daemon/version", data, err) == api_error::success) {
  //     const auto res = utils::compare_version_strings(
  //         data["version"].get<std::string>(),
  //         app_config::get_provider_minimum_version(pt));
  //     if (res < 0) {
  //       ret = exit_code::incompatible_version;
  //       std::cerr << "Failed!" << std::endl;
  //       std::cerr << "   Actual: " << data["version"].get<std::string>()
  //                 << std::endl;
  //       std::cerr << "  Minimum: "
  //                 << app_config::get_provider_minimum_version(pt) <<
  //                 std::endl;
  //     } else {
  //       std::cout << "Success!" << std::endl;
  //       std::cout << "   Actual: " << data["version"].get<std::string>()
  //                 << std::endl;
  //       std::cout << "  Minimum: "
  //                 << app_config::get_provider_minimum_version(pt) <<
  //                 std::endl;
  //     }
  //   } else {
  //     std::cerr << "Failed!" << std::endl;
  //     std::cerr << err.dump(2) << std::endl;
  //     ret = exit_code::communication_error;
  //   }
  // }

  return ret;
}
} // namespace repertory::cli::actions

#endif // INCLUDE_CLI_CHECK_VERSION_HPP_
