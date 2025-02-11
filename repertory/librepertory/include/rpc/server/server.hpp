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
#ifndef REPERTORY_INCLUDE_RPC_SERVER_SERVER_HPP_
#define REPERTORY_INCLUDE_RPC_SERVER_SERVER_HPP_

#include "types/rpc.hpp"

namespace repertory {
class app_config;

class server {
public:
  explicit server(app_config &config);

  virtual ~server() { stop(); }

private:
  app_config &config_;
  std::unique_ptr<httplib::Server> server_;
  std::unique_ptr<std::thread> server_thread_;
  std::mutex start_stop_mutex_;

private:
  [[nodiscard]] auto check_authorization(const httplib::Request &req) -> bool;

  void handle_get_config(const httplib::Request &req, httplib::Response &res);

  void handle_get_config_value_by_name(const httplib::Request &req,
                                       httplib::Response &res);

  void handle_set_config_value_by_name(const httplib::Request &req,
                                       httplib::Response &res);

  void handle_unmount(const httplib::Request &req, httplib::Response &res);

protected:
  [[nodiscard]] auto get_config() -> app_config & { return config_; }

  [[nodiscard]] auto get_config() const -> const app_config & {
    return config_;
  }

  virtual void initialize(httplib::Server &inst);

public:
  void start();

  void stop();
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_RPC_SERVER_SERVER_HPP_
