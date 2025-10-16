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
#include "ui/ui_server.hpp"

#include "app_config.hpp"
#include "events/types/event_level_changed.hpp"
#include "platform/platform.hpp"
#include "types/repertory.hpp"
#include "ui/mgmt_app_config.hpp"
#include "utils/collection.hpp"
#include "utils/common.hpp"
#include "utils/config.hpp"
#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/hash.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"

#include <boost/process/v1/args.hpp>
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/io.hpp>
namespace bp1 = boost::process::v1;

namespace {
[[nodiscard]] auto decrypt(std::string_view data, std::string_view password)
    -> std::string {
  REPERTORY_USES_FUNCTION_NAME();

  if (data.empty()) {
    return std::string{data};
  }

  repertory::data_buffer decoded;
  if (not repertory::utils::collection::from_hex_string(data, decoded)) {
    throw repertory::utils::error::create_exception(function_name,
                                                    {"decryption failed"});
  }
  repertory::data_buffer buffer(decoded.size());

  auto key = repertory::utils::hash::create_hash_blake2b_256(password);

  unsigned long long size{};
  auto res = crypto_aead_xchacha20poly1305_ietf_decrypt(
      reinterpret_cast<unsigned char *>(buffer.data()), &size, nullptr,
      reinterpret_cast<const unsigned char *>(
          &decoded.at(crypto_aead_xchacha20poly1305_IETF_NPUBBYTES)),
      decoded.size() - crypto_aead_xchacha20poly1305_IETF_NPUBBYTES,
      reinterpret_cast<const unsigned char *>(REPERTORY.data()),
      REPERTORY.length(),
      reinterpret_cast<const unsigned char *>(decoded.data()),
      reinterpret_cast<const unsigned char *>(key.data()));
  if (res != 0) {
    throw repertory::utils::error::create_exception(function_name,
                                                    {"decryption failed"});
  }

  return {
      buffer.begin(),
      std::next(buffer.begin(), static_cast<std::int64_t>(size)),
  };
}

[[nodiscard]] auto decrypt_value(const repertory::ui::mgmt_app_config *config,
                                 std::string_view key, std::string_view value,
                                 bool &skip) -> std::string {
  auto last_key{key};
  auto parts = repertory::utils::string::split(key, '.', false);
  if (parts.size() > 1U) {
    last_key = parts.at(parts.size() - 1U);
  }

  if (last_key == repertory::JSON_API_PASSWORD ||
      last_key == repertory::JSON_ENCRYPTION_TOKEN ||
      last_key == repertory::JSON_SECRET_KEY) {
    auto decrypted = decrypt(value, config->get_api_password());
    if (decrypted.empty()) {
      skip = true;
    }

    return decrypted;
  }

  return std::string{value};
}

[[nodiscard]] auto get_last_net_error() -> int {
#if defined(_WIN32)
  return WSAGetLastError();
#else  // !defined(_WIN32)
  return errno;
#endif // defined(_WIN32)
}
} // namespace

namespace repertory::ui {
ui_server::ui_server(mgmt_app_config *config)
    : config_(config),
      logging(config->get_event_level(),
              utils::path::combine(app_config::get_root_data_directory(),
                                   {
                                       "ui",
                                       "logs",
                                   })),
#if defined(_WIN32)
      repertory_binary_(
          utils::path::combine(".", {std::string{REPERTORY} + ".exe"}))
#else  // !defined(_WIN32)
      repertory_binary_(utils::path::combine(".", {REPERTORY}))
#endif // defined(_WIN32)
{
  E_SUBSCRIBE(event_level_changed, [this](auto &&event) {
    config_->set_event_level(event.new_level);
  });

#if defined(_WIN32)
  if (config_->get_hidden()) {
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
  }
#endif // defined(_WIN32)

#if defined(__APPLE__)
  server_.set_mount_point("/ui", "../Resources/web");
#else  // !defined(__APPLE__)
  server_.set_mount_point("/ui", "./web");
#endif // defined(__APPLE__)

  server_.set_socket_options([](auto &&sock) {
#if defined(_WIN32)
    BOOL enable{TRUE};
    ::setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                 reinterpret_cast<const char *>(&enable), sizeof(enable));
#else // !defined(_WIN32)!
    int one{1};
    ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                 reinterpret_cast<const char *>(&one), sizeof(one));
#ifdef SO_REUSEPORT
    ::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT,
                 reinterpret_cast<const char *>(&one), sizeof(one));
#endif // SO_REUSEPORT
#ifdef SO_NOSIGPIPE
    ::setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE,
                 reinterpret_cast<const char *>(&one), sizeof(one));
#endif // SO_NOSIGPIPE
#endif
  });

  server_.set_pre_routing_handler(
      [this](const httplib::Request &req,
             auto &&res) -> httplib::Server::HandlerResponse {
        if (req.path == "/api/v1/nonce" || req.path == "/ui" ||
            req.path.starts_with("/ui/")) {
          return httplib::Server::HandlerResponse::Unhandled;
        }

        auto auth =
            decrypt(req.get_param_value("auth"), config_->get_api_password());
        if (utils::string::begins_with(
                auth, fmt::format("{}_", config_->get_api_user()))) {
          auto nonce = auth.substr(config_->get_api_user().length() + 1U);

          mutex_lock nonce_lock(nonce_mtx_);
          if (nonce_lookup_.contains(nonce)) {
            nonce_lookup_.erase(nonce);
            return httplib::Server::HandlerResponse::Unhandled;
          }
        }

        res.status = http_error_codes::unauthorized;
        return httplib::Server::HandlerResponse::Handled;
      });

  server_.set_exception_handler([](const httplib::Request &req,
                                   httplib::Response &res,
                                   std::exception_ptr ptr) {
    REPERTORY_USES_FUNCTION_NAME();

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
    res.status = utils::string::ends_with(data["error"].get<std::string>(),
                                          "|decryption failed")
                     ? http_error_codes::unauthorized
                     : http_error_codes::internal_error;
  });

  server_.Delete("/api/v1/remove_mount", [this](auto &&req, auto &&res) {
    handle_del_remove_mount(req, res);
  });

  server_.Get("/api/v1/locations", [](auto && /* req */, auto &&res) {
    handle_get_available_locations(res);
  });

  server_.Get("/api/v1/mount",
              [this](auto &&req, auto &&res) { handle_get_mount(req, res); });

  server_.Get("/api/v1/mount_location", [this](auto &&req, auto &&res) {
    handle_get_mount_location(req, res);
  });

  server_.Get("/api/v1/mount_list", [](auto && /* req */, auto &&res) {
    handle_get_mount_list(res);
  });

  server_.Get("/api/v1/mount_status", [this](auto &&req, auto &&res) {
    handle_get_mount_status(req, res);
  });

  server_.Get("/api/v1/nonce",
              [this](auto && /* req */, auto &&res) { handle_get_nonce(res); });

  server_.Get("/api/v1/settings", [this](auto && /* req */, auto &&res) {
    handle_get_settings(res);
  });

  server_.Get("/api/v1/test",
              [this](auto &&req, auto &&res) { handle_get_test(req, res); });

  server_.Post("/api/v1/add_mount", [this](auto &&req, auto &&res) {
    handle_post_add_mount(req, res);
  });

  server_.Post("/api/v1/mount",
               [this](auto &&req, auto &&res) { handle_post_mount(req, res); });

  server_.Put("/api/v1/mount_auto_start", [this](auto &&req, auto &&res) {
    handle_put_mount_auto_start(req, res);
  });

  server_.Put("/api/v1/mount_location", [this](auto &&req, auto &&res) {
    handle_put_mount_location(req, res);
  });

  server_.Put("/api/v1/set_value_by_name", [this](auto &&req, auto &&res) {
    handle_put_set_value_by_name(req, res);
  });

  server_.Put("/api/v1/setting",
              [this](auto &&req, auto &&res) { handle_put_setting(req, res); });

  server_.Put("/api/v1/settings", [this](auto &&req, auto &&res) {
    handle_put_settings(req, res);
  });
}

ui_server::~ui_server() {
  stop();

  E_CONSUMER_RELEASE();
}

void ui_server::auto_start_mounts() {
  REPERTORY_USES_FUNCTION_NAME();

  std::thread([this]() {
    for (const auto &[prov, names] : config_->get_auto_start_list()) {
      for (const auto &name : names) {
        try {
          auto location = config_->get_mount_location(prov, name);
          if (location.empty()) {
            utils::error::raise_error(function_name,
                                      utils::error::create_error_message({
                                          "failed to auto-mount",
                                          "provider",
                                          provider_type_to_string(prov),
                                          "name",
                                          name,
                                          "location is empty",
                                      }));
          } else if (not mount(prov, name, location)) {
            utils::error::raise_error(function_name,
                                      utils::error::create_error_message({
                                          "failed to auto-mount",
                                          "provider",
                                          provider_type_to_string(prov),
                                          "name",
                                          name,
                                          "mount failed",
                                      }));
          }
        } catch (const std::exception &e) {
          utils::error::raise_error(function_name, e,
                                    utils::error::create_error_message({
                                        "failed to auto-mount",
                                        "provider",
                                        provider_type_to_string(prov),
                                        "name",
                                        name,
                                    }));
        } catch (...) {
          utils::error::raise_error(function_name, "unknown error",
                                    utils::error::create_error_message({
                                        "failed to auto-mount",
                                        "provider",
                                        provider_type_to_string(prov),
                                        "name",
                                        name,
                                    }));
        }
      }
    }
  }).join();
}

auto ui_server::data_directory_exists(provider_type prov,
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

  unique_mutex_lock nonce_lock(mtx_);
  mtx_lookup_.erase(
      fmt::format("{}-{}", name, app_config::get_provider_name(prov)));
  nonce_lock.unlock();
  return ret;
}

void ui_server::generate_config(provider_type prov, std::string_view name,
                                const json &cfg,
                                std::optional<std::string> data_dir) const {
  REPERTORY_USES_FUNCTION_NAME();

  std::map<std::string, std::string> values{};
  for (const auto &[key, value] : cfg.items()) {
    if (value.is_object()) {
      for (const auto &[key2, value2] : value.items()) {
        auto sub_key = fmt::format("{}.{}", key, key2);
        auto skip{false};
        auto decrypted = decrypt_value(
            config_, sub_key, value2.template get<std::string>(), skip);
        if (skip) {
          continue;
        }
        values[sub_key] = decrypted;
      }

      continue;
    }

    auto skip{false};
    auto decrypted =
        decrypt_value(config_, key, value.template get<std::string>(), skip);
    if (skip) {
      continue;
    }
    values[key] = decrypted;
  }

  if (data_dir.has_value()) {
    if (not utils::file::directory{data_dir.value()}.create_directory()) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to create data diretory",
                                               data_dir.value(),
                                           });
    }
    launch_process(prov, name, {"-dd", data_dir.value(), "-gc"});
  } else {
    launch_process(prov, name, {"-gc"});
  }

  for (const auto &[key, value] : values) {
    set_key_value(prov, name, key, value, data_dir);
  }
}

void ui_server::handle_del_remove_mount(const httplib::Request &req,
                                        httplib::Response &res) {
  auto prov = provider_type_from_string(req.get_param_value("type"));
  auto name = req.get_param_value("name");

  if (not data_directory_exists(prov, name)) {
    res.status = http_error_codes::not_found;
    return;
  }

  auto lines = launch_process(prov, name, {"-rp", "-f"}, false);
  res.status = lines.at(0U) == "0" ? http_error_codes::ok
                                   : http_error_codes::internal_error;
}

void ui_server::handle_put_mount_auto_start(const httplib::Request &req,
                                            httplib::Response &res) const {
  auto prov = provider_type_from_string(req.get_param_value("type"));
  auto name = req.get_param_value("name");

  if (not data_directory_exists(prov, name)) {
    res.status = http_error_codes::not_found;
    return;
  }

  auto auto_start = utils::string::to_bool(req.get_param_value("auto_start"));
  config_->set_auto_start(prov, name, auto_start);
  res.status = http_error_codes::ok;
}

void ui_server::handle_put_mount_location(const httplib::Request &req,
                                          httplib::Response &res) const {
  auto prov = provider_type_from_string(req.get_param_value("type"));
  auto name = req.get_param_value("name");
  auto location = req.get_param_value("location");

  if (not data_directory_exists(prov, name)) {
    res.status = http_error_codes::not_found;
    return;
  }

  config_->set_mount_location(prov, name, location);
  res.status = http_error_codes::ok;
}

void ui_server::handle_get_available_locations(httplib::Response &res) {
#if defined(_WIN32)
  res.set_content(nlohmann::json(utils::get_available_drive_letters()).dump(),
                  "application/json");
#else  // !defined(_WIN32)
  res.set_content(nlohmann::json(std::vector<std::string_view>()).dump(),
                  "application/json");
#endif // defined(_WIN32)

  res.status = http_error_codes::ok;
}

void ui_server::handle_get_mount(const httplib::Request &req,
                                 httplib::Response &res) const {
  REPERTORY_USES_FUNCTION_NAME();

  auto prov = provider_type_from_string(req.get_param_value("type"));
  auto name = req.get_param_value("name");

  if (not data_directory_exists(prov, name)) {
    res.status = http_error_codes::not_found;
    return;
  }

  auto lines = launch_process(prov, name, {"-dc"});
  if (lines.at(0U) != "0") {
    throw utils::error::create_exception(function_name, {
                                                            "command failed",
                                                            lines.at(0U),
                                                        });
  }

  lines.erase(lines.begin());

  auto result = nlohmann::json::parse(utils::string::join(lines, '\n'));
  clean_json_config(prov, result);

  res.set_content(result.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void ui_server::handle_get_mount_list(httplib::Response &res) {
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

void ui_server::handle_get_mount_location(const httplib::Request &req,
                                          httplib::Response &res) const {
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

void ui_server::handle_get_mount_status(const httplib::Request &req,
                                        httplib::Response &res) const {
  REPERTORY_USES_FUNCTION_NAME();

  auto name = req.get_param_value("name");
  auto prov = provider_type_from_string(req.get_param_value("type"));

  if (not data_directory_exists(prov, name)) {
    res.status = http_error_codes::not_found;
    return;
  }

  auto lines = launch_process(prov, name, {"-status"});
  if (lines.at(0U) != "0") {
    throw utils::error::create_exception(function_name, {
                                                            "command failed",
                                                            lines.at(0U),
                                                        });
  }

  lines.erase(lines.begin());

  auto result = nlohmann::json::parse(utils::string::join(lines, '\n'));
  if (result.at("Location").get<std::string>().empty()) {
    result.at("Location") = config_->get_mount_location(prov, name);
  } else if (result.at("Active").get<bool>()) {
    config_->set_mount_location(prov, name,
                                result.at("Location").get<std::string>());
  }

  result["AutoStart"] = config_->get_auto_start(prov, name);

  res.set_content(result.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void ui_server::handle_get_nonce(httplib::Response &res) {
  mutex_lock nonce_lock(nonce_mtx_);

  nonce_data nonce{};
  nonce_lookup_[nonce.nonce] = nonce;

  nlohmann::json data({{"nonce", nonce.nonce}});
  res.set_content(data.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void ui_server::handle_get_settings(httplib::Response &res) const {
  auto settings = config_->to_json();
  settings[JSON_API_PASSWORD] = "";
  settings.erase(JSON_MOUNT_LOCATIONS);
  res.set_content(settings.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void ui_server::handle_get_test(const httplib::Request &req,
                                httplib::Response &res) const {
  REPERTORY_USES_FUNCTION_NAME();

  unique_mutex_lock nonce_lock(test_mtx_);

  auto name = req.get_param_value("name");
  auto prov = provider_type_from_string(req.get_param_value("type"));
  auto cfg = nlohmann::json::parse(req.get_param_value("config"));

  auto data_dir = utils::path::combine(
      utils::directory::temp(), {utils::file::create_temp_name(REPERTORY)});

  try {
    generate_config(prov, name, cfg, data_dir);

    auto lines = launch_process(prov, name, {"-dd", data_dir, "-test"});
    res.status = lines.at(0U) == "0" ? http_error_codes::ok
                                     : http_error_codes::internal_error;
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "test provider config failed");
    res.status = http_error_codes::internal_error;
  }

  if (utils::file::directory{data_dir}.remove_recursively()) {
    return;
  }

  utils::error::raise_error(
      function_name, utils::get_last_error_code(),
      fmt::format("failed to remove data directory|{}", data_dir));
}

void ui_server::handle_post_add_mount(const httplib::Request &req,
                                      httplib::Response &res) const {
  auto name = req.get_param_value("name");
  auto prov = provider_type_from_string(req.get_param_value("type"));
  if (data_directory_exists(prov, name)) {
    res.status = http_error_codes::ok;
    return;
  }

  auto cfg = nlohmann::json::parse(req.get_param_value("config"));
  generate_config(prov, name, cfg);

  res.status = http_error_codes::ok;
}

void ui_server::handle_post_mount(const httplib::Request &req,
                                  httplib::Response &res) {
  auto name = req.get_param_value("name");
  auto prov = provider_type_from_string(req.get_param_value("type"));

  if (not data_directory_exists(prov, name)) {
    res.status = http_error_codes::not_found;
    return;
  }

  res.status = http_error_codes::ok;

  auto location = utils::path::absolute(req.get_param_value("location"));
  auto unmount = utils::string::to_bool(req.get_param_value("unmount"));
  if (unmount) {
    launch_process(prov, name, {"-unmount"});
    return;
  }

  if (mount(prov, name, location)) {
    launch_process(prov, name, {"-status"});
    return;
  }

  res.status = http_error_codes::internal_error;
}

void ui_server::handle_put_set_value_by_name(const httplib::Request &req,
                                             httplib::Response &res) const {
  auto name = req.get_param_value("name");
  auto prov = provider_type_from_string(req.get_param_value("type"));

  if (not data_directory_exists(prov, name)) {
    res.status = http_error_codes::not_found;
    return;
  }

  auto key = req.get_param_value("key");
  auto value = req.get_param_value("value");

  auto skip{false};
  value = decrypt_value(config_, key, value, skip);
  if (not skip) {
    set_key_value(prov, name, key, value);
  }

  res.status = http_error_codes::ok;
}

void ui_server::handle_put_setting(const httplib::Request &req,
                                   httplib::Response &res) const {
  auto name = req.get_param_value("name");
  auto value = req.get_param_value("value");

  if (name == JSON_ANIMATIONS) {
    config_->set_animations(utils::string::to_bool(value));
  } else if (name == JSON_AUTO_START) {
    config_->set_auto_start(utils::string::to_bool(value));
  }

  res.status = http_error_codes::ok;
}

void ui_server::handle_put_settings(const httplib::Request &req,
                                    httplib::Response &res) const {
  auto data = nlohmann::json::parse(req.get_param_value("data"));

  if (data.contains(JSON_API_PASSWORD)) {
    auto password = decrypt(data.at(JSON_API_PASSWORD).get<std::string>(),
                            config_->get_api_password());
    if (not password.empty()) {
      config_->set_api_password(password);
    }
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

auto ui_server::launch_process(provider_type prov, std::string_view name,
                               std::vector<std::string> args,
                               bool background) const
    -> std::vector<std::string> {
  REPERTORY_USES_FUNCTION_NAME();

  switch (prov) {
  case provider_type::encrypt:
    args.insert(args.begin(), "-en");
    args.insert(std::next(args.begin()), "-na");
    args.insert(std::next(args.begin(), 2U), std::string{name});
    break;

  case provider_type::remote: {
    auto parts = utils::string::split(name, '_', false);
    args.insert(args.begin(), "-rm");
    args.insert(std::next(args.begin()),
                fmt::format("{}:{}", parts.at(0U), parts.at(1U)));
  } break;

  case provider_type::s3:
    args.insert(args.begin(), "-s3");
    args.insert(std::next(args.begin()), "-na");
    args.insert(std::next(args.begin(), 2U), std::string{name});
    break;

  case provider_type::sia:
    args.insert(args.begin(), "-na");
    args.insert(std::next(args.begin()), std::string{name});
    break;

  default:
    throw utils::error::create_exception(function_name,
                                         {
                                             "provider is not supported",
                                             provider_type_to_string(prov),
                                             name,
                                         });
  }

  unique_mutex_lock nonce_lock(mtx_);
  auto &inst_mtx = mtx_lookup_[fmt::format(
      "{}-{}", name, app_config::get_provider_name(prov))];
  nonce_lock.unlock();

  recur_mutex_lock inst_lock(inst_mtx);
  if (background) {
#if defined(_WIN32)
    args.insert(args.begin(), "--hidden");

    auto cmdline = fmt::format("{}{}{}", '"', repertory_binary_, '"');
    for (const auto &arg : args) {
      cmdline += " ";
      if (arg.find_first_of(" \t") != std::string::npos) {
        cmdline += fmt::format("{}{}{}", '"', arg, '"');
      } else {
        cmdline += arg;
      }
    }

    STARTUPINFOA start_info{};
    start_info.cb = sizeof(start_info);
    start_info.dwFlags = STARTF_USESHOWWINDOW;
    start_info.wShowWindow = SW_SHOWMINNOACTIVE;

    PROCESS_INFORMATION proc_info{};
    auto result = ::CreateProcessA(
        nullptr, &cmdline[0U], nullptr, nullptr, FALSE,
        CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP, nullptr,
        utils::path::get_parent_path(repertory_binary_).c_str(), &start_info,
        &proc_info);

    if (result) {
      ::CloseHandle(proc_info.hProcess);
      ::CloseHandle(proc_info.hThread);
    }
#else // !defined(_WIN32)
    args.insert(args.begin(), repertory_binary_);
#if !defined(__APPLE__)
    args.insert(std::next(args.begin()), "-f");
#endif // defined(__APPLE_)

    std::vector<const char *> exec_args;
    exec_args.reserve(args.size() + 1U);
    for (const auto &arg : args) {
      exec_args.push_back(arg.c_str());
    }
    exec_args.push_back(nullptr);

    [[maybe_unused]] auto ret = utils::create_daemon([&]() -> int {
      chdir(utils::path::get_parent_path(repertory_binary_).c_str());
      return execvp(exec_args.at(0U),
                    const_cast<char *const *>(exec_args.data()));
    });
#endif // defined(_WIN32)

    return {};
  }

  bp1::ipstream out;
  bp1::child proc(repertory_binary_, bp1::args(args), bp1::std_out > out);

  std::string data;
  std::string line;
  while (out && std::getline(out, line)) {
    data += line + "\n";
  }

  return utils::string::split(utils::string::replace(data, "\r", ""), '\n',
                              false);
}

auto ui_server::mount(provider_type prov, std::string_view name,
                      std::string_view location) -> bool {
#if defined(_WIN32)
  if (utils::file::directory{location}.exists()) {
#else  // !defined(_WIN32)
  if (not utils::file::directory{location}.exists()) {
#endif // defined(_WIN32)

    config_->set_mount_location(prov, name, "");
    return false;
  }

  config_->set_mount_location(prov, name, location);

  static std::mutex mount_mtx;
  mutex_lock nonce_lock(mount_mtx);

  launch_process(prov, name, {std::string{location}}, true);
  return true;
}

void ui_server::notify_and_unlock(unique_mutex_lock &nonce_lock) {
  nonce_notify_.notify_all();
  nonce_lock.unlock();
}

void ui_server::open_ui() const {
  if (config_->get_launch_only()) {
    return;
  }

#if defined(_WIN32)
  system(fmt::format(
             R"(start "Repertory Management Portal" "http://127.0.0.1:{}/ui")",
             config_->get_api_port())
             .c_str());
#elif defined(__linux__)
  system(fmt::format(R"(xdg-open "http://127.0.0.1:{}/ui")",
                     config_->get_api_port())
             .c_str());
#elif defined(__APPLE__)
  system(
      fmt::format(R"(open "http://127.0.0.1:{}/ui")", config_->get_api_port())
          .c_str());
#else
  build fails here
#endif
}

void ui_server::removed_expired_nonces() {
  unique_mutex_lock nonce_lock(nonce_mtx_);
  notify_and_unlock(nonce_lock);

  while (not stop_requested) {
    nonce_lock.lock();
    auto nonces = nonce_lookup_;
    notify_and_unlock(nonce_lock);

    for (const auto &[key, value] : nonces) {
      if (std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now() - value.creation)
              .count() >= nonce_timeout) {
        nonce_lock.lock();
        nonce_lookup_.erase(key);
        notify_and_unlock(nonce_lock);
      }
    }

    if (stop_requested) {
      break;
    }

    nonce_lock.lock();
    if (stop_requested) {
      break;
    }
    nonce_notify_.wait_for(nonce_lock, std::chrono::seconds(1U));
    notify_and_unlock(nonce_lock);
  }
}

void ui_server::set_key_value(provider_type prov, std::string_view name,
                              std::string_view key, std::string_view value,
                              std::optional<std::string> data_dir) const {
  std::vector<std::string> args;
  if (data_dir.has_value()) {
    args.emplace_back("-dd");
    args.emplace_back(data_dir.value());
  }
  args.emplace_back("-set");
  args.emplace_back(key);
  args.emplace_back(value);
  launch_process(prov, name, args, false);
}

void ui_server::start() {
  REPERTORY_USES_FUNCTION_NAME();

  unique_mutex_lock nonce_lock(nonce_mtx_);
  if (nonce_thread_) {
    return;
  }

  stop_requested = false;
  event_system::instance().start();

  nonce_thread_ =
      std::make_unique<std::thread>([this]() { removed_expired_nonces(); });

  lock_data ui_lock(app_config::get_root_data_directory(),
                    provider_type::unknown, "ui");
  auto res = ui_lock.grab_lock(1U);
  if (res != lock_result::success) {
    notify_and_unlock(nonce_lock);
    open_ui();
    return;
  }

  auto deadline{std::chrono::steady_clock::now() + 30s};
  std::string host{"127.0.0.1"};
  auto desired_port{config_->get_api_port()};

  auto success{false};
  while (not success && std::chrono::steady_clock::now() < deadline) {
    success = server_.bind_to_port(host, desired_port);
    if (success) {
      break;
    }

    utils::error::raise_error(function_name,
                              utils::error::create_error_message({
                                  "failed to bind",
                                  "host",
                                  host,
                                  "port",
                                  std::to_string(desired_port),
                                  "error",
                                  std::to_string(get_last_net_error()),
                              }));
    std::this_thread::sleep_for(250ms);
  }

  if (not success) {
    utils::error::raise_error(function_name,
                              utils::error::create_error_message({
                                  "bind timeout (port in use)",
                                  "host",
                                  host,
                                  "port",
                                  std::to_string(desired_port),
                              }));
    notify_and_unlock(nonce_lock);
    return;
  }

  auto_start_mounts();
  open_ui();

  notify_and_unlock(nonce_lock);

  server_.listen_after_bind();
}

void ui_server::stop() {
  unique_mutex_lock nonce_lock(nonce_mtx_);
  if (not nonce_thread_) {
    notify_and_unlock(nonce_lock);
    return;
  }

  stop_requested = true;
  notify_and_unlock(nonce_lock);

  server_.stop();
  nonce_thread_->join();

  nonce_lock.lock();
  nonce_thread_.reset();
  notify_and_unlock(nonce_lock);

  event_system::instance().stop();
}
} // namespace repertory::ui
