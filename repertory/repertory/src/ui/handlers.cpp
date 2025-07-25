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

  auto key = repertory::utils::encryption::create_hash_blake2b_256(password);

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

  server_->set_socket_options([](auto &&sock) {
#if defined(_WIN32)
    int enable{1};
    setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
               reinterpret_cast<const char *>(&enable), sizeof(enable));
#else  //  !defined(_WIN32)
    linger opt{
        .l_onoff = 1,
        .l_linger = 0,
    };
    setsockopt(sock, SOL_SOCKET, SO_LINGER,
               reinterpret_cast<const char *>(&opt), sizeof(opt));
#endif // defined(_WIN32)
  });

  server_->set_pre_routing_handler(
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

          mutex_lock lock(nonce_mtx_);
          if (nonce_lookup_.contains(nonce)) {
            nonce_lookup_.erase(nonce);
            return httplib::Server::HandlerResponse::Unhandled;
          }
        }

        res.status = http_error_codes::unauthorized;
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
    res.status = utils::string::ends_with(data["error"].get<std::string>(),
                                          "|decryption failed")
                     ? http_error_codes::unauthorized
                     : http_error_codes::internal_error;
  });

  server->Get("/api/v1/locations", [](auto && /* req */, auto &&res) {
    handle_get_available_locations(res);
  });

  server->Get("/api/v1/mount",
              [this](auto &&req, auto &&res) { handle_get_mount(req, res); });

  server->Get("/api/v1/mount_location", [this](auto &&req, auto &&res) {
    handle_get_mount_location(req, res);
  });

  server->Get("/api/v1/mount_list", [](auto && /* req */, auto &&res) {
    handle_get_mount_list(res);
  });

  server->Get("/api/v1/mount_status", [this](auto &&req, auto &&res) {
    handle_get_mount_status(req, res);
  });

  server->Get("/api/v1/nonce",
              [this](auto && /* req */, auto &&res) { handle_get_nonce(res); });

  server->Get("/api/v1/settings", [this](auto && /* req */, auto &&res) {
    handle_get_settings(res);
  });

  server->Get("/api/v1/test",
              [this](auto &&req, auto &&res) { handle_get_test(req, res); });

  server->Post("/api/v1/add_mount", [this](auto &&req, auto &&res) {
    handle_post_add_mount(req, res);
  });

  server->Post("/api/v1/mount",
               [this](auto &&req, auto &&res) { handle_post_mount(req, res); });

  server->Put("/api/v1/mount_location", [this](auto &&req, auto &&res) {
    handle_put_mount_location(req, res);
  });

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

  nonce_thread_ =
      std::make_unique<std::thread>([this]() { removed_expired_nonces(); });

  server_->listen("127.0.0.1", config_->get_api_port());
  if (this_server != nullptr) {
    this_server = nullptr;
    server_->stop();
  }
}

handlers::~handlers() {
  if (nonce_thread_) {
    stop_requested = true;

    unique_mutex_lock lock(nonce_mtx_);
    nonce_notify_.notify_all();
    lock.unlock();

    nonce_thread_->join();
    nonce_thread_.reset();
  }

  event_system::instance().stop();
}

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

void handlers::generate_config(provider_type prov, std::string_view name,
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

void handlers::handle_put_mount_location(const httplib::Request &req,
                                         httplib::Response &res) const {
  REPERTORY_USES_FUNCTION_NAME();

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

void handlers::handle_get_available_locations(httplib::Response &res) {
#if defined(_WIN32)
  constexpr std::array<std::string_view, 26U> letters{
      "a:", "b:", "c:", "d:", "e:", "f:", "g:", "h:", "i:",
      "j:", "k:", "l:", "m:", "n:", "o:", "p:", "q:", "r:",
      "s:", "t:", "u:", "v:", "w:", "x:", "y:", "z:",
  };

  auto available = std::accumulate(
      letters.begin(), letters.end(), std::vector<std::string_view>(),
      [](auto &&vec, auto &&letter) -> std::vector<std::string_view> {
        if (utils::file::directory{utils::path::combine(letter, {"\\"})}
                .exists()) {
          return vec;
        }

        vec.emplace_back(letter);
        return vec;
      });

  res.set_content(nlohmann::json(available).dump(), "application/json");
#else  // !defined(_WIN32)
  res.set_content(nlohmann::json(std::vector<std::string_view>()).dump(),
                  "application/json");
#endif // defined(_WIN32)

  res.status = http_error_codes::ok;
}

void handlers::handle_get_mount(const httplib::Request &req,
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

void handlers::handle_get_mount_list(httplib::Response &res) {
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

void handlers::handle_get_mount_location(const httplib::Request &req,
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

void handlers::handle_get_mount_status(const httplib::Request &req,
                                       httplib::Response &res) const {
  auto name = req.get_param_value("name");
  auto prov = provider_type_from_string(req.get_param_value("type"));

  if (not data_directory_exists(prov, name)) {
    res.status = http_error_codes::not_found;
    return;
  }

  auto lines = launch_process(prov, name, {"-status"});

  auto result = nlohmann::json::parse(utils::string::join(lines, '\n'));
  if (result.at("Location").get<std::string>().empty()) {
    result.at("Location") = config_->get_mount_location(prov, name);
  } else if (result.at("Active").get<bool>()) {
    config_->set_mount_location(prov, name,
                                result.at("Location").get<std::string>());
  }

  res.set_content(result.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void handlers::handle_get_nonce(httplib::Response &res) {
  mutex_lock lock(nonce_mtx_);

  nonce_data nonce{};
  nonce_lookup_[nonce.nonce] = nonce;

  nlohmann::json data({{"nonce", nonce.nonce}});
  res.set_content(data.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void handlers::handle_get_settings(httplib::Response &res) const {
  auto settings = config_->to_json();
  settings[JSON_API_PASSWORD] = "";
  settings.erase(JSON_MOUNT_LOCATIONS);
  res.set_content(settings.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void handlers::handle_get_test(const httplib::Request &req,
                               httplib::Response &res) const {
  REPERTORY_USES_FUNCTION_NAME();

  unique_mutex_lock lock(test_mtx_);

  auto name = req.get_param_value("name");
  auto prov = provider_type_from_string(req.get_param_value("type"));
  auto cfg = nlohmann::json::parse(req.get_param_value("config"));

  auto data_dir = utils::path::combine(
      utils::directory::temp(), {utils::file::create_temp_name("repertory")});

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

void handlers::handle_post_add_mount(const httplib::Request &req,
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

void handlers::handle_post_mount(const httplib::Request &req,
                                 httplib::Response &res) {
  auto name = req.get_param_value("name");
  auto prov = provider_type_from_string(req.get_param_value("type"));

  if (not data_directory_exists(prov, name)) {
    res.status = http_error_codes::not_found;
    return;
  }

  auto location = utils::path::absolute(req.get_param_value("location"));
  auto unmount = utils::string::to_bool(req.get_param_value("unmount"));

  if (unmount) {
    launch_process(prov, name, {"-unmount"});
  } else {
#if defined(_WIN32)
    if (utils::file::directory{location}.exists()) {
#else  // !defined(_WIN32)
    if (not utils::file::directory{location}.exists()) {
#endif // defined(_WIN32)
      config_->set_mount_location(prov, name, "");
      res.status = http_error_codes::internal_error;
      return;
    }

    config_->set_mount_location(prov, name, location);

    static std::mutex mount_mtx;
    mutex_lock lock(mount_mtx);

    launch_process(prov, name, {location}, true);
    launch_process(prov, name, {"-status"});
  }

  res.status = http_error_codes::ok;
}

void handlers::handle_put_set_value_by_name(const httplib::Request &req,
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

void handlers::handle_put_settings(const httplib::Request &req,
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

auto handlers::launch_process(provider_type prov, std::string_view name,
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

  unique_mutex_lock lock(mtx_);
  auto &inst_mtx = mtx_lookup_[fmt::format(
      "{}-{}", name, app_config::get_provider_name(prov))];
  lock.unlock();

  recur_mutex_lock inst_lock(inst_mtx);
  if (background) {
#if defined(_WIN32)
    std::array<char, MAX_PATH + 1U> path{};
    ::GetSystemDirectoryA(path.data(), path.size());

    args.insert(args.begin(), utils::path::combine(path.data(), {"cmd.exe"}));
    args.insert(std::next(args.begin()), "/c");
    args.insert(std::next(args.begin(), 2U), "start");
    args.insert(std::next(args.begin(), 3U), "");
    args.insert(std::next(args.begin(), 4U), "/MIN");
    args.insert(std::next(args.begin(), 5U), repertory_binary_);
#else  // !defined(_WIN32)
    args.insert(args.begin(), "-f");
    args.insert(args.begin(), repertory_binary_);
#endif // defined(_WIN32)

    std::vector<const char *> exec_args;
    exec_args.reserve(args.size() + 1U);
    for (const auto &arg : args) {
      exec_args.push_back(arg.c_str());
    }
    exec_args.push_back(nullptr);

#if defined(_WIN32)
    _spawnv(_P_DETACH, exec_args.at(0U),
            const_cast<char *const *>(exec_args.data()));
#else  // !defined(_WIN32)
    auto pid = fork();
    if (pid < 0) {
      exit(1);
    }

    if (pid == 0) {
      signal(SIGCHLD, SIG_DFL);

      if (setsid() < 0) {
        exit(1);
      }

      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);

      if (open("/dev/null", O_RDONLY) != 0 ||
          open("/dev/null", O_WRONLY) != 1 ||
          open("/dev/null", O_WRONLY) != 2) {
        exit(1);
      }

      chdir(utils::path::get_parent_path(repertory_binary_).c_str());
      execvp(exec_args.at(0U), const_cast<char *const *>(exec_args.data()));
      exit(1);
    } else {
      signal(SIGCHLD, SIG_IGN);
    }
#endif // defined(_WIN32)

    return {};
  }

  boost::process::v1::ipstream out;
  boost::process::v1::child proc(repertory_binary_,
                                 boost::process::v1::args(args),
                                 boost::process::v1::std_out > out);

  std::string data;
  std::string line;
  while (out && std::getline(out, line)) {
    data += line + "\n";
  }

  return utils::string::split(utils::string::replace(data, "\r", ""), '\n',
                              false);
}

void handlers::removed_expired_nonces() {
  unique_mutex_lock lock(nonce_mtx_);
  lock.unlock();

  while (not stop_requested) {
    lock.lock();
    auto nonces = nonce_lookup_;
    lock.unlock();

    for (const auto &[key, value] : nonces) {
      if (std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now() - value.creation)
              .count() >= nonce_timeout) {
        lock.lock();
        nonce_lookup_.erase(key);
        lock.unlock();
      }
    }

    if (stop_requested) {
      break;
    }

    lock.lock();
    if (stop_requested) {
      break;
    }
    nonce_notify_.wait_for(lock, std::chrono::seconds(1U));
    lock.unlock();
  }
}

void handlers::set_key_value(provider_type prov, std::string_view name,
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
} // namespace repertory::ui
