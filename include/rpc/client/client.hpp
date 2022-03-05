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
#ifndef INCLUDE_RPC_CLIENT_CLIENT_HPP_
#define INCLUDE_RPC_CLIENT_CLIENT_HPP_

#include "common.hpp"
#include "types/rpc.hpp"
#include "types/skynet.hpp"

namespace repertory {
class client {
public:
  explicit client(rpc_host_info host_info);

private:
  const rpc_host_info host_info_;
  std::atomic<std::uint32_t> request_id_;

private:
  rpc_response make_request(const std::string &command, const std::vector<json> &args,
                            std::uint32_t timeout_ms = 10000u);

public:
  rpc_response export_list(const std::vector<std::string> &paths);

  rpc_response export_all();

  rpc_response get_drive_information();

  rpc_response get_config();

  rpc_response get_config_value_by_name(const std::string &name);

  rpc_response get_directory_items(const std::string &api_path);

  rpc_response get_open_files();

  rpc_response get_pinned_files();

  rpc_response import_skylink(const skylink_import_list &list);

  rpc_response pin_file(const std::string &api_file);

  rpc_response pinned_status(const std::string &api_file);

  rpc_response set_config_value_by_name(const std::string &name, const std::string &value);

  rpc_response unmount();

  rpc_response unpin_file(const std::string &api_file);
};
} // namespace repertory

#endif // INCLUDE_RPC_CLIENT_CLIENT_HPP_
