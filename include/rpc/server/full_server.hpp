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
#ifndef INCLUDE_RPC_SERVER_FULL_SERVER_HPP_
#define INCLUDE_RPC_SERVER_FULL_SERVER_HPP_

#include "common.hpp"
#include "rpc/server/server.hpp"

namespace repertory {
class i_open_file_table;
class i_provider;
class full_server final : public server {
public:
  explicit full_server(app_config &config, i_provider &provider, i_open_file_table &oft);

  ~full_server() override = default;

private:
  i_provider &provider_;
  i_open_file_table &oft_;

protected:
  bool handle_request(jsonrpcpp::request_ptr &request,
                      std::unique_ptr<jsonrpcpp::Response> &response) override;
};
} // namespace repertory

#endif // INCLUDE_RPC_SERVER_FULL_SERVER_HPP_
