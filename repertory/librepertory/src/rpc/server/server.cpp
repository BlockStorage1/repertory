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
#include "rpc/server/server.hpp"

#include "app_config.hpp"
#include "events/event_system.hpp"
#include "events/types/service_start_begin.hpp"
#include "events/types/service_start_end.hpp"
#include "events/types/service_stop_begin.hpp"
#include "events/types/service_stop_end.hpp"
#include "events/types/unmount_requested.hpp"
#include "rpc/common.hpp"
#include "utils/error_utils.hpp"

namespace repertory {
server::server(app_config &config) : config_(config) {}

server::~server() { stop(); }

void server::handle_get_config(const httplib::Request & /*req*/,
                               httplib::Response &res) {
  auto data = config_.get_json();
  clean_json_config(config_.get_provider_type(), data);
  res.set_content(data.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void server::handle_get_config_value_by_name(const httplib::Request &req,
                                             httplib::Response &res) {
  auto name = req.get_param_value("name");
  auto data = json({{
      "value",
      clean_json_value(name, config_.get_value_by_name(name)),
  }});
  res.set_content(data.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void server::handle_set_config_value_by_name(const httplib::Request &req,
                                             httplib::Response &res) {
  auto name = req.get_param_value("name");
  auto value = req.get_param_value("value");

  json data = {{
      "value",
      clean_json_value(name, config_.set_value_by_name(name, value)),
  }};
  res.set_content(data.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void server::handle_unmount(const httplib::Request & /*req*/,
                            httplib::Response &res) {
  REPERTORY_USES_FUNCTION_NAME();

  event_system::instance().raise<unmount_requested>(function_name);
  res.status = http_error_codes::ok;
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
  REPERTORY_USES_FUNCTION_NAME();

  mutex_lock lock(start_stop_mutex_);
  if (server_thread_) {
    return;
  }

  event_system::instance().raise<service_start_begin>(function_name, "server");

  server_ = std::make_unique<httplib::Server>();

  server_->set_exception_handler([](const httplib::Request &req,
                                    httplib::Response &res,
                                    std::exception_ptr ptr) {
    json data = {{"path", req.path}};

    try {
      std::rethrow_exception(ptr);
    } catch (std::exception &e) {
      data["error"] = (e.what() == nullptr) ? "unknown error" : e.what();
      utils::error::raise_error(function_name, e,
                                "failed request: " + req.path);
    } catch (...) {
      data["error"] = "unknown error";
      utils::error::raise_error(function_name, "unknown error",
                                "failed request: " + req.path);
    }

    res.set_content(data.dump(), "application/json");
    res.status = http_error_codes::internal_error;
  });

  server_->set_pre_routing_handler(
      [this](auto &&req, auto &&res) -> httplib::Server::HandlerResponse {
        if (rpc::check_authorization(config_, req)) {
          return httplib::Server::HandlerResponse::Unhandled;
        }

        res.status = http_error_codes::unauthorized;
        return httplib::Server::HandlerResponse::Handled;
      });

  initialize(*server_);

  server_thread_ = std::make_unique<std::thread>([this]() {
    server_->set_socket_options([](auto &&sock) {
#if defined(_WIN32)
      int enable{1};
      setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                 reinterpret_cast<const char *>(&enable), sizeof(enable));
#else  //  !defined(_WIN32)
      linger opt{1, 0};
      setsockopt(sock, SOL_SOCKET, SO_LINGER,
                 reinterpret_cast<const char *>(&opt), sizeof(opt));
#endif // defined(_WIN32)
    });

    server_->listen("127.0.0.1", config_.get_api_port());
  });
  event_system::instance().raise<service_start_end>(function_name, "server");
}

void server::stop() {
  REPERTORY_USES_FUNCTION_NAME();

  unique_mutex_lock lock(start_stop_mutex_);
  if (not server_thread_) {
    return;
  }

  event_system::instance().raise<service_stop_begin>(function_name, "server");

  server_->stop();

  std::unique_ptr<std::thread> thread{nullptr};
  std::swap(thread, server_thread_);
  lock.unlock();

  thread->join();
  thread.reset();

  lock.lock();
  server_.reset();
  lock.unlock();

  event_system::instance().raise<service_stop_end>(function_name, "server");
}
} // namespace repertory
