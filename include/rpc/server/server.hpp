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
#ifndef INCLUDE_RPC_SERVER_SERVER_HPP_
#define INCLUDE_RPC_SERVER_SERVER_HPP_

#include "common.hpp"
#include "types/rpc.hpp"

namespace repertory {
class app_config;
class server {
  typedef std::map<std::string, std::string, httpserver::http::arg_comparator> query_map;

private:
  class json_response final : public httpserver::http_response {
  public:
    json_response() = default;

    explicit json_response(const json &content,
                           int response_code = httpserver::http::http_utils::http_ok,
                           const std::string &content_type = "application/json")
        : http_response(response_code, content_type), content(content.dump(0)) {}

    json_response(const json_response &other) = default;
    json_response(json_response &&other) noexcept = default;

    json_response &operator=(const json_response &b) = default;
    json_response &operator=(json_response &&b) = default;

    ~json_response() override = default;

    MHD_Response *get_raw_response() override {
      const auto size = content.length();
      return MHD_create_response_from_buffer(size, (void *)content.c_str(), MHD_RESPMEM_PERSISTENT);
    }

  private:
    std::string content = "";
  };

  class rpc_resource final : public httpserver::http_resource {
  public:
    explicit rpc_resource(server &owner);

  private:
    server &owner_;

  public:
    const std::shared_ptr<httpserver::http_response>
    render(const httpserver::http_request &request) override;
  };

public:
  explicit server(app_config &config);

  virtual ~server() { stop(); }

private:
  app_config &config_;
  rpc_resource resource_;
  std::unique_ptr<httpserver::webserver> ws_;
  std::mutex start_stop_mutex_;
  bool started_ = false;

private:
  bool check_authorization(const httpserver::http_request &request);

protected:
  app_config &get_config() { return config_; }

  const app_config &get_config() const { return config_; }

  virtual bool handle_request(jsonrpcpp::request_ptr &request,
                              std::unique_ptr<jsonrpcpp::Response> &response);

public:
  void start();

  void stop();
};
} // namespace repertory

#endif // INCLUDE_RPC_SERVER_SERVER_HPP_
