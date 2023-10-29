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
#ifndef INCLUDE_TYPES_RPC_HPP_
#define INCLUDE_TYPES_RPC_HPP_

namespace repertory {
struct rpc_host_info {
  std::string host;
  std::string password;
  std::uint16_t port;
  std::string user;
};

enum class rpc_response_type {
  success,
  config_value_not_found,
  http_error,
};

struct rpc_response {
  rpc_response_type response_type;
  json data;
};

namespace rpc_method {
const std::string get_config = "get_config";
const std::string get_config_value_by_name = "get_config_value_by_name";
const std::string get_directory_items = "get_directory_items";
const std::string get_drive_information = "get_drive_information";
const std::string get_open_files = "get_open_files";
const std::string get_pinned_files = "get_pinned_files";
const std::string pin_file = "pin_file";
const std::string pinned_status = "pinned_status";
const std::string set_config_value_by_name = "set_config_value_by_name";
const std::string unmount = "unmount";
const std::string unpin_file = "unpin_file";
} // namespace rpc_method
} // namespace repertory

#endif // INCLUDE_TYPES_RPC_HPP_
