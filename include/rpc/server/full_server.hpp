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
#ifndef INCLUDE_RPC_SERVER_FULL_SERVER_HPP_
#define INCLUDE_RPC_SERVER_FULL_SERVER_HPP_

#include "rpc/server/server.hpp"

namespace repertory {
class i_file_manager;
class i_provider;

class full_server final : public server {
public:
  explicit full_server(app_config &config, i_provider &provider,
                       i_file_manager &fm);

  ~full_server() override = default;

private:
  i_provider &provider_;
  i_file_manager &fm_;

private:
  void handle_get_directory_items(const httplib::Request &req,
                                  httplib::Response &res);

  void handle_get_drive_information(const httplib::Request &req,
                                    httplib::Response &res);

  void handle_get_open_files(const httplib::Request &req,
                             httplib::Response &res);

  void handle_get_pinned_files(const httplib::Request &req,
                               httplib::Response &res);

  void handle_get_pinned_status(const httplib::Request &req,
                                httplib::Response &res);

  void handle_pin_file(const httplib::Request &req, httplib::Response &res);

  void handle_unpin_file(const httplib::Request &req, httplib::Response &res);

protected:
  void initialize(httplib::Server &inst) override;
};
} // namespace repertory

#endif // INCLUDE_RPC_SERVER_FULL_SERVER_HPP_
