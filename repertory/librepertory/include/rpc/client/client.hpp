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
#ifndef REPERTORY_INCLUDE_RPC_CLIENT_CLIENT_HPP_
#define REPERTORY_INCLUDE_RPC_CLIENT_CLIENT_HPP_

#include "types/rpc.hpp"

namespace repertory {
class client {
public:
  explicit client(rpc_host_info host_info);

private:
  const rpc_host_info host_info_;
  std::atomic<std::uint32_t> request_id_{0U};

public:
  [[nodiscard]] auto get_drive_information() -> rpc_response;

  [[nodiscard]] auto get_config() -> rpc_response;

  [[nodiscard]] auto get_config_value_by_name(const std::string &name)
      -> rpc_response;

  [[nodiscard]] auto get_directory_items(const std::string &api_path)
      -> rpc_response;

  [[nodiscard]] auto get_open_files() -> rpc_response;

  [[nodiscard]] auto get_pinned_files() -> rpc_response;

  [[nodiscard]] auto pin_file(const std::string &api_file) -> rpc_response;

  [[nodiscard]] auto pinned_status(const std::string &api_file) -> rpc_response;

  [[nodiscard]] auto set_config_value_by_name(const std::string &name,
                                              const std::string &value)
      -> rpc_response;

  [[nodiscard]] auto unmount() -> rpc_response;

  [[nodiscard]] auto unpin_file(const std::string &api_file) -> rpc_response;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_RPC_CLIENT_CLIENT_HPP_
