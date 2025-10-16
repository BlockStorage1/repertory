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
#ifndef REPERTORY_INCLUDE_CLI_TEST_HPP_
#define REPERTORY_INCLUDE_CLI_TEST_HPP_

#include "cli/common.hpp"

namespace repertory::cli::actions {
[[nodiscard]] inline auto
test(std::vector<const char *> /* args */, std::string_view data_directory,
     provider_type prov, std::string_view /* unique_id */, std::string /*user*/,
     std::string /*password*/) -> exit_code {
  auto ret = cli::check_data_directory(data_directory);
  if (ret != exit_code::success) {
    return ret;
  }

  app_config config(prov, data_directory);
  auto is_online{
      (prov == provider_type::remote)
          ? remote_client(config).check() == 0
          : create_provider(prov, config)->is_online(),
  };

  return cli::handle_error(
      is_online ? exit_code::success : exit_code::provider_offline,
      fmt::format("provider is {}", is_online ? "online" : "offline"));
}
} // namespace repertory::cli::actions

#endif // REPERTORY_INCLUDE_CLI_TEST_HPP_
