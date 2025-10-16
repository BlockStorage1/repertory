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
#ifndef REPERTORY_INCLUDE_CLI_REMOVE_HPP_
#define REPERTORY_INCLUDE_CLI_REMOVE_HPP_

#include "cli/common.hpp"

#include "utils/time.hpp"

namespace repertory::cli::actions {
[[nodiscard]] inline auto remove(std::vector<const char *> /* args */,
                                 std::string_view data_directory,
                                 provider_type prov, std::string_view unique_id,
                                 std::string /* user */,
                                 std::string /* password */) -> exit_code {
  auto ret = cli::check_data_directory(data_directory);
  if (ret != exit_code::success) {
    return ret;
  }

  lock_data lock(data_directory, prov, unique_id);
  auto res = lock.grab_lock(1);
  if (res != lock_result::success) {
    return cli::handle_error(exit_code::lock_failed,
                             "failed to get mount lock");
  }

  auto trash_path = utils::path::combine(
      app_config::get_root_data_directory(),
      {
          "trash",
          app_config::get_provider_name(prov),
          fmt::format("{}_{}", utils::time::get_current_time_utc(),
                      utils::collection::to_hex_string(
                          utils::generate_secure_random<data_buffer>(4U))),
      });

  if (utils::file::directory{data_directory}.move_to(trash_path)) {
    return cli::handle_error(
        exit_code::success,
        fmt::format("successfully removed provider|type|{}|id|{}",
                    app_config::get_provider_name(prov), unique_id));
  }

  return cli::handle_error(
      exit_code::remove_failed,
      fmt::format("failed to remove provider|type|{}|id|{}|directory|{}",
                  app_config::get_provider_name(prov), unique_id,
                  data_directory));
}
} // namespace repertory::cli::actions

#endif // REPERTORY_INCLUDE_CLI_REMOVE_HPP_
