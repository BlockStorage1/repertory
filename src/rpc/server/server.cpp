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
#include "rpc/server/server.hpp"
#include "app_config.hpp"
#include "utils/Base64.hpp"

namespace repertory {
server::rpc_resource::rpc_resource(server &owner) : httpserver::http_resource(), owner_(owner) {
  disallow_all();
  set_allowing("POST", true);
}

const std::shared_ptr<httpserver::http_response>
server::rpc_resource::render(const httpserver::http_request &request) {
  std::shared_ptr<json_response> ret;

  try {
    if (owner_.check_authorization(request)) {
      std::unique_ptr<jsonrpcpp::Response> response;
      auto entity = jsonrpcpp::Parser::do_parse(utils::string::to_utf8(request.get_content()));
      if (entity->is_request()) {
        auto request = std::dynamic_pointer_cast<jsonrpcpp::Request>(entity);
        if (not owner_.handle_request(request, response)) {
          throw jsonrpcpp::MethodNotFoundException(*request);
        }
        ret = std::make_shared<json_response>(response->to_json());
      }
    } else {
      ret = std::make_shared<json_response>(json({{"error", "unauthorized"}}),
                                            httpserver::http::http_utils::http_unauthorized);
    }
  } catch (const jsonrpcpp::RequestException &e) {
    ret = std::make_shared<json_response>(e.to_json(),
                                          httpserver::http::http_utils::http_bad_request);
    event_system::instance().raise<rpc_server_exception>(e.to_json().dump());
  } catch (const std::exception &e2) {
    ret = std::make_shared<json_response>(json({{"exception", e2.what()}}),
                                          httpserver::http::http_utils::http_internal_server_error);
    event_system::instance().raise<rpc_server_exception>(e2.what());
  }

  return ret;
}

server::server(app_config &config) : config_(config), resource_(*this) {}

bool server::check_authorization(const httpserver::http_request &request) {
  auto ret = (config_.get_api_auth().empty() && config_.get_api_user().empty());
  if (not ret) {
    const auto authorization = request.get_header("Authorization");
    if (not authorization.empty()) {
      const auto auth_parts = utils::string::split(authorization, ' ');
      if (not auth_parts.empty()) {
        const auto auth_type = auth_parts[0];
        if (auth_type == "Basic") {
          const auto data = macaron::Base64::Decode(authorization.substr(6));
          const auto auth = utils::string::split(std::string(data.begin(), data.end()), ':');
          if (auth.size() == 2) {
            const auto &user = auth[0];
            const auto &pwd = auth[1];
            ret = (user == config_.get_api_user()) && (pwd == config_.get_api_auth());
          }
        }
      }
    }
  }

  return ret;
}

bool server::handle_request(jsonrpcpp::request_ptr &request,
                            std::unique_ptr<jsonrpcpp::Response> &response) {
  auto handled = true;
  if (request->method == rpc_method::get_config) {
    if (request->params.param_array.empty()) {
      response = std::make_unique<jsonrpcpp::Response>(*request, config_.get_json());
    } else {
      throw jsonrpcpp::InvalidParamsException(*request);
    }
  } else if (request->method == rpc_method::get_config_value_by_name) {
    if (request->params.param_array.size() == 1) {
      response = std::make_unique<jsonrpcpp::Response>(
          *request, json({{"value", config_.get_value_by_name(request->params.param_array[0])}}));
    } else {
      throw jsonrpcpp::InvalidParamsException(*request);
    }
  } else if (request->method == rpc_method::set_config_value_by_name) {
    if (request->params.param_array.size() == 2) {
      response = std::make_unique<jsonrpcpp::Response>(
          *request, json({{"value", config_.set_value_by_name(request->params.param_array[0],
                                                              request->params.param_array[1])}}));
    } else {
      throw jsonrpcpp::InvalidParamsException(*request);
    }
  } else if (request->method == rpc_method::unmount) {
    if (request->params.param_array.empty()) {
      event_system::instance().raise<unmount_requested>();
      response = std::make_unique<jsonrpcpp::Response>(*request, json({{"success", true}}));
    } else {
      throw jsonrpcpp::InvalidParamsException(*request);
    }
  } else {
    handled = false;
  }
  return handled;
}

/*void server::Start() {
  mutex_lock l(startStopMutex_);
  if (not started_) {
    struct sockaddr_in localHost{};
    inet_pton(AF_INET, "127.0.0.1", &localHost.sin_addr);

    ws_ = std::make_unique<httpserver::webserver>(
        httpserver::create_webserver(config_.GetAPIPort())
            .bind_address((sockaddr*)&localHost)
            .default_policy(httpserver::http::http_utils::REJECT));
    ws_->allow_ip("127.0.0.1");
    ws_->register_resource("/api", &rpcResource_);
    ws_->start(false);
    started_ = true;
  }
}*/

void server::start() {
  mutex_lock l(start_stop_mutex_);
  if (not started_) {
    ws_ = std::make_unique<httpserver::webserver>(
        httpserver::create_webserver(config_.get_api_port())
            .default_policy(httpserver::http::http_utils::REJECT));
    ws_->allow_ip("127.0.0.1");
    ws_->register_resource("/api", &resource_);
    ws_->start(false);
    started_ = true;
  }
}

void server::stop() {
  mutex_lock l(start_stop_mutex_);
  if (started_) {
    event_system::instance().raise<service_shutdown>("server");
    ws_->stop();
    ws_.reset();
    started_ = false;
  }
}
} // namespace repertory
