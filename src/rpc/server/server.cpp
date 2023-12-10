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
#include "rpc/server/server.hpp"

#include "app_config.hpp"
#include "utils/Base64.hpp"
#include "utils/error_utils.hpp"

namespace repertory {
server::server(app_config &config) : config_(config) {}

auto server::check_authorization(const httplib::Request &req) -> bool {
  if (config_.get_api_auth().empty() || config_.get_api_user().empty()) {
    utils::error::raise_error(__FUNCTION__,
                              "authorization user or password is not set");
    return false;
  }

  const auto authorization = req.get_header_value("Authorization");
  if (authorization.empty()) {
    return false;
  }

  const auto auth_parts = utils::string::split(authorization, ' ');
  if (auth_parts.empty()) {
    return false;
  }

  const auto auth_type = auth_parts[0U];
  if (auth_type != "Basic") {
    return false;
  }

  const auto data = macaron::Base64::Decode(authorization.substr(6U));
  const auto auth =
      utils::string::split(std::string(data.begin(), data.end()), ':');
  if (auth.size() != 2U) {
    return false;
  }

  const auto &user = auth[0U];
  const auto &pwd = auth[1U];
  return (user == config_.get_api_user()) && (pwd == config_.get_api_auth());
}

void server::handle_get_config(const httplib::Request & /*req*/,
                               httplib::Response &res) {
  auto data = config_.get_json();
  res.set_content(data.dump(), "application/json");
  res.status = 200;
}

void server::handle_get_config_value_by_name(const httplib::Request &req,
                                             httplib::Response &res) {
  auto name = req.get_param_value("name");
  auto data = json({{"value", config_.get_value_by_name(name)}});
  res.set_content(data.dump(), "application/json");
  res.status = 200;
}

void server::handle_set_config_value_by_name(const httplib::Request &req,
                                             httplib::Response &res) {
  auto name = req.get_param_value("name");
  auto value = req.get_param_value("value");

  json data = {{"value", config_.set_value_by_name(name, value)}};
  res.set_content(data.dump(), "application/json");
  res.status = 200;
}

void server::handle_unmount(const httplib::Request & /*req*/,
                            httplib::Response &res) {
  event_system::instance().raise<unmount_requested>();
  res.status = 200;
}

void server::initialize(httplib::Server &inst) {
  inst.Get("/api/v1/" + rpc_method::get_config, [this](auto &&req, auto &&res) {
    handle_get_config(std::forward<decltype(req)>(req),
                      std::forward<decltype(res)>(res));
  });

  inst.Get("/api/v1/" + rpc_method::get_config_value_by_name,
           [this](auto &&req, auto &&res) {
             handle_get_config_value_by_name(std::forward<decltype(req)>(req),
                                             std::forward<decltype(res)>(res));
           });

  inst.Post("/api/v1/" + rpc_method::set_config_value_by_name,
            [this](auto &&req, auto &&res) {
              handle_set_config_value_by_name(std::forward<decltype(req)>(req),
                                              std::forward<decltype(res)>(res));
            });

  inst.Post("/api/v1/" + rpc_method::unmount, [this](auto &&req, auto &&res) {
    handle_unmount(std::forward<decltype(req)>(req),
                   std::forward<decltype(res)>(res));
  });
}

void server::start() {
  mutex_lock l(start_stop_mutex_);
  if (not started_) {
    event_system::instance().raise<service_started>("server");
    server_ = std::make_unique<httplib::Server>();

    server_->set_exception_handler([](const httplib::Request &req,
                                      httplib::Response &res,
                                      std::exception_ptr ep) {
      json data = {{"path", req.path}};

      try {
        std::rethrow_exception(ep);
      } catch (std::exception &e) {
        data["error"] = e.what() ? e.what() : "unknown error";
        utils::error::raise_error(__FUNCTION__, e,
                                  "failed request: " + req.path);
      } catch (...) {
        data["error"] = "unknown error";
        utils::error::raise_error(__FUNCTION__, "unknown error",
                                  "failed request: " + req.path);
      }

      res.set_content(data.dump(), "application/json");
      res.status = 500;
    });

    server_->set_pre_routing_handler(
        [this](auto &&req, auto &&res) -> httplib::Server::HandlerResponse {
          if (check_authorization(req)) {
            return httplib::Server::HandlerResponse::Unhandled;
          }

          res.status = 401;
          return httplib::Server::HandlerResponse::Handled;
        });

    initialize(*server_);

    server_thread_ = std::make_unique<std::thread>(
        [this]() { server_->listen("127.0.0.1", config_.get_api_port()); });

    started_ = true;
  }
}

void server::stop() {
  if (started_) {
    mutex_lock l(start_stop_mutex_);
    if (started_) {
      event_system::instance().raise<service_shutdown_begin>("server");

      server_->stop();
      server_thread_->join();
      server_thread_.reset();

      started_ = false;
      event_system::instance().raise<service_shutdown_end>("server");
    }
  }
}
} // namespace repertory
