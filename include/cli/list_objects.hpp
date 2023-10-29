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
#ifndef INCLUDE_CLI_LIST_OBJECTS_HPP_
#define INCLUDE_CLI_LIST_OBJECTS_HPP_
#if defined(REPERTORY_ENABLE_S3)

#include "app_config.hpp"
#include "comm/s3/s3_comm.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "providers/provider.hpp"
#include "rpc/client/client.hpp"
#include "types/repertory.hpp"
#include "utils/cli_utils.hpp"

namespace repertory::cli::actions {
[[nodiscard]] inline auto
list_objects(int /* argc */, char * /* argv */[],
             const std::string &data_directory, const provider_type &pt,
             const std::string &unique_id, std::string /* user */,
             std::string /* password */) -> exit_code {
  auto ret = exit_code::invalid_provider_type;

  if (pt == provider_type::s3) {
    lock_data lock(pt, unique_id);
    const auto res = lock.grab_lock(1u);
    /* if (res == lock_result::locked) { */
    /* auto port = app_config::default_api_port(pt); */
    /* utils::cli::get_api_authentication_data(user, password, port, pt,
     * data_directory); */
    /* const auto response = */
    /*     client({"localhost", password, port,
     * user}).create_directory(api_path); */
    /* std::cout << static_cast<int>(response.response_type) << std::endl; */
    /* std::cout << response.data.dump(2) << std::endl; */
    /* } else */
    if (res == lock_result::success) {
      app_config config(pt, data_directory);
      s3_comm comm(config);
      std::vector<directory_item> list{};
      const auto res = comm.get_object_list(list);
      if (res == api_error::success) {
        for (const auto &di : list) {
          std::cout << di.to_json().dump(2) << std::endl;
        }
        ret = exit_code::success;
      } else {
        std::cout << api_error_to_string(res) << std::endl;
      }
    } else {
      std::cout << "failed to grab lock: '" << static_cast<int>(res) << "'"
                << std::endl;
      ret = exit_code::lock_failed;
    }
  }

  return ret;
}
} // namespace repertory::cli::actions

#endif // REPERTORY_ENABLE_S3
#endif // INCLUDE_CLI_LIST_OBJECTS_HPP_
