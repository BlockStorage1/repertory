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
#include "ui/handlers.hpp"

#include "app_config.hpp"
#include "events/event_system.hpp"
#include "rpc/common.hpp"
#include "types/repertory.hpp"
#include "ui/mgmt_app_config.hpp"
#include "utils/common.hpp"
#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"

namespace {
[[nodiscard]] constexpr auto is_restricted(std::string_view data) -> bool {
  constexpr std::string_view invalid_chars = "&;|><$()`{}!*?";
  return data.find_first_of(invalid_chars) != std::string_view::npos;
}
} // namespace

namespace repertory::ui {
handlers::handlers(mgmt_app_config *config, httplib::Server *server)
    : config_(config),
#if defined(_WIN32)
      repertory_binary_(utils::path::combine(".", {"repertory.exe"})),
#else  // !defined(_WIN32)
      repertory_binary_(utils::path::combine(".", {"repertory"})),
#endif // defined(_WIN32)
      server_(server) {
  REPERTORY_USES_FUNCTION_NAME();

  server_->set_pre_routing_handler(
      [this](auto &&req, auto &&res) -> httplib::Server::HandlerResponse {
        if (rpc::check_authorization(*config_, req)) {
          return httplib::Server::HandlerResponse::Unhandled;
        }

        res.status = http_error_codes::unauthorized;
        res.set_header(
            "WWW-Authenticate",
            R"(Basic realm="Repertory Management Portal", charset="UTF-8")");
        return httplib::Server::HandlerResponse::Handled;
      });

  server_->set_exception_handler([](const httplib::Request &req,
                                    httplib::Response &res,
                                    std::exception_ptr ptr) {
    json data{
        {"path", req.path},
    };

    try {
      std::rethrow_exception(ptr);
    } catch (const std::exception &e) {
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

  server->Get("/api/v1/mount",
              [this](auto &&req, auto &&res) { handle_get_mount(req, res); });

  server->Get("/api/v1/mount_location", [this](auto &&req, auto &&res) {
    handle_get_mount_location(req, res);
  });

  server->Get("/api/v1/mount_list", [this](auto && /* req */, auto &&res) {
    handle_get_mount_list(res);
  });

  server->Get("/api/v1/mount_status",
              [this](const httplib::Request &req, auto &&res) {
                handle_get_mount_status(req, res);
              });

  server->Get("/api/v1/settings",
              [this](const httplib::Request & /* req */, auto &&res) {
                handle_get_settings(res);
              });

  server->Post("/api/v1/add_mount", [this](auto &&req, auto &&res) {
    handle_post_add_mount(req, res);
  });

  server->Post("/api/v1/mount",
               [this](auto &&req, auto &&res) { handle_post_mount(req, res); });

  server->Put("/api/v1/set_value_by_name", [this](auto &&req, auto &&res) {
    handle_put_set_value_by_name(req, res);
  });

  server->Put("/api/v1/settings", [this](auto &&req, auto &&res) {
    handle_put_settings(req, res);
  });

  static std::atomic<httplib::Server *> this_server{server_};
  static const auto quit_handler = [](int /* sig */) {
    auto *ptr = this_server.load();
    if (ptr == nullptr) {
      return;
    }

    this_server = nullptr;
    ptr->stop();
  };

  std::signal(SIGINT, quit_handler);
#if !defined(_WIN32)
  std::signal(SIGQUIT, quit_handler);
#endif // !defined(_WIN32)
  std::signal(SIGTERM, quit_handler);

#if defined(_WIN32)
  system(fmt::format(
             R"(start "Repertory Management Portal" "http://127.0.0.1:{}/ui")",
             config_->get_api_port())
             .c_str());
#elif defined(__linux__)
  system(fmt::format(R"(xdg-open "http://127.0.0.1:{}/ui")",
                     config_->get_api_port())
             .c_str());
#else // error
  build fails here
#endif

  std::uint16_t port{};
  if (not utils::get_next_available_port(config_->get_api_port(), port)) {
    fmt::println("failed to detect if port is available|{}",
                 config_->get_api_port());
    return;
  }

  if (port != config_->get_api_port()) {
    fmt::println("failed to listen on port|{}|next available|{}",
                 config_->get_api_port(), port);
    return;
  }

  event_system::instance().start();

  server_->listen("127.0.0.1", config_->get_api_port());
  server_->stop();
  this_server = nullptr;
}

handlers::~handlers() { event_system::instance().stop(); }

auto handlers::data_directory_exists(provider_type prov,
                                     std::string_view name) const -> bool {
  auto data_dir = utils::path::combine(app_config::get_root_data_directory(),
                                       {
                                           app_config::get_provider_name(prov),
                                           name,
                                       });
  auto ret = utils::file::directory{data_dir}.exists();
  if (ret) {
    return ret;
  }

  unique_mutex_lock lock(mtx_);
  mtx_lookup_.erase(
      fmt::format("{}-{}", name, app_config::get_provider_name(prov)));
  lock.unlock();
  return ret;
}

void handlers::handle_get_mount(auto &&req, auto &&res) const {
  REPERTORY_USES_FUNCTION_NAME();

  auto prov = provider_type_from_string(req.get_param_value("type"));
  auto name = req.get_param_value("name");

  if (not data_directory_exists(prov, name)) {
    res.status = http_error_codes::not_found;
    return;
  }

  auto lines = launch_process(prov, name, "-dc");

  if (lines.at(0U) != "0") {
    throw utils::error::create_exception(function_name, {
                                                            "command failed",
                                                            lines.at(0U),
                                                        });
  }

  lines.erase(lines.begin());

  auto result = nlohmann::json::parse(utils::string::join(lines, '\n'));
  res.set_content(result.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void handlers::handle_get_mount_list(auto &&res) const {
  auto data_dir = utils::file::directory{app_config::get_root_data_directory()};

  nlohmann::json result;
  const auto process_dir = [&data_dir, &result](std::string_view name) {
    auto name_dir = data_dir.get_directory(name);
    if (not name_dir) {
      return;
    }

    for (const auto &dir : name_dir->get_directories()) {
      if (not dir->get_file("config.json")) {
        continue;
      }

      result[name].emplace_back(
          utils::path::strip_to_file_name(dir->get_path()));
    }
  };

  process_dir("encrypt");
  process_dir("remote");
  process_dir("s3");
  process_dir("sia");

  res.set_content(result.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void handlers::handle_get_mount_location(auto &&req, auto &&res) const {
  auto name = req.get_param_value("name");
  auto prov = provider_type_from_string(req.get_param_value("type"));

  if (not data_directory_exists(prov, name)) {
    res.status = http_error_codes::not_found;
    return;
  }

  res.set_content(
      nlohmann::json({
                         {"Location", config_->get_mount_location(prov, name)},
                     })
          .dump(),
      "application/json");
  res.status = http_error_codes::ok;
}

void handlers::handle_get_mount_status(auto &&req, auto &&res) const {
  REPERTORY_USES_FUNCTION_NAME();

  auto name = req.get_param_value("name");
  auto prov = provider_type_from_string(req.get_param_value("type"));

  if (not data_directory_exists(prov, name)) {
    res.status = http_error_codes::not_found;
    return;
  }

  auto status_name = app_config::get_provider_display_name(prov);

  switch (prov) {
  case provider_type::remote: {
    auto parts = utils::string::split(name, '_', false);
    status_name = fmt::format("{}{}:{}", status_name, parts[0U], parts[1U]);
  } break;

  case provider_type::encrypt:
  case provider_type::sia:
  case provider_type::s3:
    status_name = fmt::format("{}{}", status_name, name);
    break;

  default:
    throw utils::error::create_exception(function_name,
                                         {
                                             "provider is not supported",
                                             provider_type_to_string(prov),
                                             name,
                                         });
  }

  auto lines = launch_process(prov, name, "-status");

  nlohmann::json result(
      nlohmann::json::parse(utils::string::join(lines, '\n')).at(status_name));
  if (result.at("Location").get<std::string>().empty()) {
    result.at("Location") = config_->get_mount_location(prov, name);
  } else if (result.at("Active").get<bool>()) {
    config_->set_mount_location(prov, name,
                                result.at("Location").get<std::string>());
  }

  res.set_content(result.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void handlers::handle_get_settings(auto &&res) const {
  auto settings = config_->to_json();
  settings.erase(JSON_MOUNT_LOCATIONS);
  res.set_content(settings.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void handlers::handle_post_add_mount(auto &&req, auto &&res) const {
  auto name = req.get_param_value("name");
  auto prov = provider_type_from_string(req.get_param_value("type"));
  if (data_directory_exists(prov, name)) {
    res.status = http_error_codes::ok;
    return;
  }

  auto cfg = nlohmann::json::parse(req.get_param_value("config"));

  launch_process(prov, name, "-gc");
  for (const auto &[key, value] : cfg.items()) {
    if (value.is_object()) {
      for (const auto &[key2, value2] : value.items()) {
        set_key_value(prov, name, fmt::format("{}.{}", key, key2),
                      value2.template get<std::string>());
      }
    } else {
      set_key_value(prov, name, key, value.template get<std::string>());
    }
  }

  res.status = http_error_codes::ok;
}

void handlers::handle_post_mount(auto &&req, auto &&res) const {
  auto name = req.get_param_value("name");
  auto prov = provider_type_from_string(req.get_param_value("type"));

  if (not data_directory_exists(prov, name)) {
    res.status = http_error_codes::not_found;
    return;
  }

  auto location = utils::path::absolute(req.get_param_value("location"));
  auto unmount = utils::string::to_bool(req.get_param_value("unmount"));

  if (unmount) {
    launch_process(prov, name, "-unmount");
  } else {
    if (not utils::file::directory{location}.exists()) {
      res.status = http_error_codes::internal_error;
      return;
    }

    launch_process(prov, name, fmt::format(R"("{}")", location), true);
  }

  res.status = http_error_codes::ok;
}

void handlers::handle_put_set_value_by_name(auto &&req, auto &&res) const {
  auto name = req.get_param_value("name");
  auto prov = provider_type_from_string(req.get_param_value("type"));

  if (not data_directory_exists(prov, name)) {
    res.status = http_error_codes::not_found;
    return;
  }

  auto key = req.get_param_value("key");
  auto value = req.get_param_value("value");

  set_key_value(prov, name, key, value);

  res.status = http_error_codes::ok;
}

void handlers::handle_put_settings(auto &&req, auto &&res) const {
  nlohmann::json data = nlohmann::json::parse(req.get_param_value("data"));

  if (data.contains(JSON_API_PASSWORD)) {
    config_->set_api_password(data.at(JSON_API_PASSWORD).get<std::string>());
  }

  if (data.contains(JSON_API_PORT)) {
    config_->set_api_port(
        utils::string::to_uint16(data.at(JSON_API_PORT).get<std::string>()));
  }

  if (data.contains(JSON_API_USER)) {
    config_->set_api_user(data.at(JSON_API_USER).get<std::string>());
  }

  res.status = http_error_codes::ok;
}

auto handlers::launch_process(provider_type prov, std::string_view name,
                              std::string_view args, bool background) const
    -> std::vector<std::string> {
  REPERTORY_USES_FUNCTION_NAME();

  if (is_restricted(name) || is_restricted(args)) {
    throw utils::error::create_exception(function_name,
                                         {
                                             "invalid data detected",
                                         });
  }

  std::string str_type;
  switch (prov) {
  case provider_type::encrypt:
    str_type = fmt::format("-en -na {}", name);
    break;

  case provider_type::remote: {
    auto parts = utils::string::split(name, '_', false);
    str_type = fmt::format("-rm {}:{}", parts[0U], parts[1U]);
  } break;

  case provider_type::s3:
    str_type = fmt::format("-s3 -na {}", name);
    break;

  case provider_type::sia:
    str_type = fmt::format("-na {}", name);
    break;

  default:
    throw utils::error::create_exception(function_name,
                                         {
                                             "provider is not supported",
                                             provider_type_to_string(prov),
                                             name,
                                         });
  }

  auto cmd_line = fmt::format(R"({} {} {})", repertory_binary_, str_type, args);

  unique_mutex_lock lock(mtx_);
  auto &inst_mtx = mtx_lookup_[fmt::format(
      "{}-{}", name, app_config::get_provider_name(prov))];
  lock.unlock();

  recur_mutex_lock inst_lock(inst_mtx);
  if (background) {
#if defined(_WIN32)
    system(fmt::format(R"(start "" /b {})", cmd_line).c_str());
#elif defined(__linux__) // defined(__linux__)
    system(fmt::format("nohup {} 1>/dev/null 2>&1", cmd_line).c_str());
#else                    // !defined(__linux__) && !defined(_WIN32)
    build fails here
#endif                   // defined(_WIN32)
    return {};
  }

  auto *pipe = popen(cmd_line.c_str(), "r");
  if (pipe == nullptr) {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to execute command",
                                             provider_type_to_string(prov),
                                             name,
                                         });
  }

  std::string data;
  std::array<char, 1024U> buffer{};
  while (std::feof(pipe) == 0) {
    while (std::fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
      data += buffer.data();
    }
  }
  pclose(pipe);

  return utils::string::split(utils::string::replace(data, "\r", ""), '\n',
                              false);
}

void handlers::set_key_value(provider_type prov, std::string_view name,
                             std::string_view key,
                             std::string_view value) const {
#if defined(_WIN32)
  launch_process(prov, name, fmt::format(R"(-set {} "{}")", key, value));
#else  // !defined(_WIN32)
  launch_process(prov, name, fmt::format("-set {} '{}'", key, value));
#endif // defined(_WIN32)
}
} // namespace repertory::ui
