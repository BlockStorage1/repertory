/*
  Copyright <2018-2024> <scott.e.graves@protonmail.com>

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
#ifndef REPERTORY_INCLUDE_CLI_STATUS_HPP_
#define REPERTORY_INCLUDE_CLI_STATUS_HPP_

#include "platform/platform.hpp"
#include "types/repertory.hpp"

namespace repertory::cli::actions {
[[nodiscard]] inline auto status(std::vector<const char *> /* args */,
                                 const std::string & /*data_directory*/,
                                 const provider_type &prov,
                                 const std::string &unique_id,
                                 std::string /* user */,
                                 std::string /* password */) -> exit_code {
  auto ret = exit_code::success;
  lock_data lock(prov, unique_id);
  [[maybe_unused]] auto status = lock.grab_lock(10U);
  json mount_state;
  if (lock.get_mount_state(mount_state)) {
    std::cout << mount_state.dump(2) << std::endl;
  } else {
    std::cout << "{}" << std::endl;
    ret = exit_code::failed_to_get_mount_state;
  }

  return ret;
}
} // namespace repertory::cli::actions

#endif // REPERTORY_INCLUDE_CLI_STATUS_HPP_
