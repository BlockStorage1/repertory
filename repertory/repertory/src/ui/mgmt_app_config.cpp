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
#include "ui/mgmt_app_config.hpp"

#include "app_config.hpp"
#include "platform/platform.hpp"
#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/unix.hpp"

namespace {
template <typename data_t>
[[nodiscard]] auto from_json(const nlohmann::json &json)
    -> std::unordered_map<repertory::provider_type,
                          std::unordered_map<std::string, data_t>> {
  std::unordered_map<repertory::provider_type,
                     std::unordered_map<std::string, data_t>>
      map_of_maps{
          {repertory::provider_type::encrypt, nlohmann::json::object()},
          {repertory::provider_type::remote, nlohmann::json::object()},
          {repertory::provider_type::s3, nlohmann::json::object()},
          {repertory::provider_type::sia, nlohmann::json::object()},
      };

  if (json.is_null() || json.empty()) {
    return map_of_maps;
  }

  for (auto &[prov, map] : map_of_maps) {
    auto prov_str = repertory::provider_type_to_string(prov);
    if (!json.contains(prov_str)) {
      continue;
    }

    for (const auto &[key, value] : json.at(prov_str).items()) {
      map[key] = value;
    }
  }

  return map_of_maps;
}

[[nodiscard]] auto map_to_json(const auto &map_of_maps) -> nlohmann::json {
  auto json = nlohmann::json::object();
  for (const auto &[prov, map] : map_of_maps) {
    for (const auto &[key, value] : map) {
      json[repertory::provider_type_to_string(prov)][key] = value;
    }
  }

  return json;
}
} // namespace

namespace repertory::ui {
mgmt_app_config::mgmt_app_config(bool hidden, bool launch_only)
    : hidden_(hidden), launch_only_(launch_only) {
  REPERTORY_USES_FUNCTION_NAME();

  auto config_file =
      utils::path::combine(app_config::get_root_data_directory(), {"ui.json"});

  try {
    if (not utils::file::directory{app_config::get_root_data_directory()}
                .create_directory()) {
      throw utils::error::create_exception(
          function_name, {
                             "failed to create directory",
                             app_config::get_root_data_directory(),
                         });
    }

    nlohmann::json data;
    if (utils::file::read_json_file(config_file, data)) {
      api_password_ = data.at(JSON_API_PASSWORD).get<std::string>();
      api_port_ = data.at(JSON_API_PORT).get<std::uint16_t>();
      api_user_ = data.at(JSON_API_USER).get<std::string>();

      auto should_save{not data.contains(JSON_AUTO_START)};
      auto_start_ = should_save ? auto_start_.load()
                                : data.at(JSON_AUTO_START).get<bool>();

      if (data.contains(JSON_ANIMATIONS)) {
        animations_ = data.at(JSON_ANIMATIONS).get<bool>();
      } else {
        should_save = true;
      }

      if (data.contains(JSON_MOUNT_AUTO_START)) {
        mount_auto_start_ = from_json<bool>(data.at(JSON_MOUNT_AUTO_START));
      } else {
        should_save = true;
      }

      if (data.contains(JSON_EVENT_LEVEL)) {
        event_level_ = event_level_from_string(
            data.at(JSON_EVENT_LEVEL).get<std::string>());
      } else {
        should_save = true;
      }

      if (data.contains(JSON_MOUNT_LOCATIONS)) {
        locations_ = from_json<std::string>(data.at(JSON_MOUNT_LOCATIONS));
      } else {
        should_save = true;
      }

      if (should_save) {
        save();
      }

      set_auto_start(get_auto_start());
      return;
    }

    utils::error::raise_error(
        function_name, utils::get_last_error_code(),
        fmt::format("failed to read file|{}", config_file));
    save();

    set_auto_start(get_auto_start());
  } catch (const std::exception &ex) {
    utils::error::raise_error(
        function_name, ex, fmt::format("failed to read file|{}", config_file));
  }
}

auto mgmt_app_config::get_auto_start(provider_type prov,
                                     std::string_view name) const -> bool {
  recur_mutex_lock lock(mtx_);
  if (mount_auto_start_.contains(prov) &&
      mount_auto_start_.at(prov).contains(std::string{name})) {
    return mount_auto_start_.at(prov).at(std::string{name});
  }

  return false;
}

auto mgmt_app_config::get_auto_start_list() const
    -> std::unordered_map<provider_type, std::vector<std::string>> {
  std::unordered_map<provider_type, std::vector<std::string>> ret;

  recur_mutex_lock lock(mtx_);
  for (const auto &prov_pair : mount_auto_start_) {
    for (const auto &pair : prov_pair.second) {
      if (pair.second) {
        ret[prov_pair.first].emplace_back(pair.first);
      }
    }
  }

  return ret;
}

auto mgmt_app_config::get_mount_location(provider_type prov,
                                         std::string_view name) const
    -> std::string {
  recur_mutex_lock lock(mtx_);
  if (locations_.contains(prov) &&
      locations_.at(prov).contains(std::string{name})) {
    return locations_.at(prov).at(std::string{name});
  }

  return "";
}

void mgmt_app_config::save() const {
  REPERTORY_USES_FUNCTION_NAME();

  auto config_file =
      utils::path::combine(app_config::get_root_data_directory(), {"ui.json"});

  try {
    if (not utils::file::directory{app_config::get_root_data_directory()}
                .create_directory()) {
      utils::error::raise_error(
          function_name, fmt::format("failed to create directory|{}",
                                     app_config::get_root_data_directory()));
      return;
    }

    if (utils::file::write_json_file(config_file, to_json())) {
      return;
    }

    utils::error::raise_error(
        function_name, utils::get_last_error_code(),
        fmt::format("failed to save file|{}", config_file));
  } catch (const std::exception &ex) {
    utils::error::raise_error(
        function_name, ex, fmt::format("failed to save file|{}", config_file));
  }
}

void mgmt_app_config::set_animations(bool animations) {
  if (animations_ == animations) {
    return;
  }

  animations_ = animations;
  save();
}

void mgmt_app_config::set_api_password(std::string_view api_password) {
  if (api_password_ == std::string{api_password}) {
    return;
  }

  api_password_ = std::string{api_password};
  save();
}

void mgmt_app_config::set_api_port(std::uint16_t api_port) {
  if (api_port_ == api_port) {
    return;
  }

  api_port_ = api_port;
  save();
}

void mgmt_app_config::set_api_user(std::string_view api_user) {
  if (api_user_ == std::string{api_user}) {
    return;
  }

  api_user_ = std::string{api_user};
  save();
}

void mgmt_app_config::set_auto_start(bool auto_start) {
  REPERTORY_USES_FUNCTION_NAME();

  auto current_directory = std::filesystem::current_path();
  if (utils::file::change_to_process_directory()) {
#if defined(__linux__)
    if (auto_start) {
      utils::autostart_cfg cfg{};
      cfg.app_name = REPERTORY;
      cfg.comment = "Mount utility for AWS S3 and Sia";
      cfg.exec_args = {"-ui", "-lo"};
      cfg.exec_path = utils::path::combine(".", {REPERTORY});
      cfg.icon_path =
          utils::path::combine(".", {std::string{REPERTORY} + ".png"});
      cfg.terminal = true;

      if (utils::create_autostart_entry(cfg, false)) {
        utils::error::handle_info(function_name,
                                  "created auto-start entry|name|repertory");
      } else {
        utils::error::raise_error(
            function_name, utils::get_last_error_code(),
            "failed to create auto-start entry|name|repertory");
      }
    } else if (utils::remove_autostart_entry(REPERTORY)) {
      utils::error::handle_info(function_name,
                                "removed auto-start entry|name|repertory");
    } else {
      utils::error::raise_error(
          function_name, utils::get_last_error_code(),
          "failed to remove auto-start entry|name|repertory");
    }
#endif // defined(__linux__)

#if defined(_WIN32)
    if (auto_start) {
      utils::shortcut_cfg cfg{};
      cfg.arguments = L"-ui -lo --hidden";
      cfg.exe_path = utils::path::combine(L".", {REPERTORY_W});
      cfg.icon_path = utils::path::combine(L".", {L"icon.ico"});
      cfg.shortcut_name = REPERTORY_W;
      cfg.working_directory = utils::path::absolute(L".");

      if (utils::create_shortcut(cfg, false)) {
        utils::error::handle_info(function_name,
                                  "created auto-start entry|name|repertory");
      } else {
        utils::error::raise_error(
            function_name, utils::get_last_error_code(),
            "failed to create auto-start entry|name|repertory");
      }
    } else if (utils::remove_shortcut(std::wstring{REPERTORY_W})) {
      utils::error::handle_info(function_name,
                                "removed auto-start entry|name|repertory");
    } else {
      utils::error::raise_error(
          function_name, utils::get_last_error_code(),
          "failed to remove auto-start entry|name|repertory");
    }
#endif // defined(_WIN32)

#if defined(__APPLE__)
    const auto *label = "com.fifthgrid.blockstorage.repertory.ui";
    auto plist_path = utils::path::combine("~", {"/Library/LaunchAgents"});
    if (auto_start) {
      utils::plist_cfg cfg{};
      cfg.args = {
          utils::path::combine(".", {REPERTORY}),
          "-ui",
          "-lo",
      };
      cfg.keep_alive = false;
      cfg.label = label;
      cfg.plist_path = plist_path;
      cfg.run_at_load = true;
      cfg.stderr_log = "/tmp/repertory_ui.err";
      cfg.stdout_log = "/tmp/repertory_ui.out";
      cfg.working_dir = utils::path::absolute(".");

      if (utils::generate_launchd_plist(cfg, false)) {
        utils::error::handle_info(function_name,
                                  "created auto-start entry|name|repertory");
      } else {
        utils::error::raise_error(
            function_name, utils::get_last_error_code(),
            "failed to create auto-start entry|name|repertory");
      }
    } else if (utils::remove_launchd_plist(plist_path, label, false)) {
      utils::error::handle_info(function_name,
                                "removed auto-start entry|name|repertory");
    } else {
      utils::error::raise_error(
          function_name, "failed to remove auto-start entry|name|repertory");
    }
#endif // defined(__APPLE__)
  } else {
    utils::error::raise_error(function_name, utils::get_last_error_code(),
                              fmt::format("failed to change directory"));
  }

  std::filesystem::current_path(current_directory);
  if (auto_start_ == auto_start) {
    return;
  }

  auto_start_ = auto_start;
  save();
}

void mgmt_app_config::set_auto_start(provider_type prov, std::string_view name,
                                     bool auto_start) {
  if (name.empty()) {
    return;
  }

  recur_mutex_lock lock(mtx_);
  if (mount_auto_start_[prov][std::string{name}] == auto_start) {
    return;
  }

  mount_auto_start_[prov][std::string{name}] = auto_start;

  save();
}

void mgmt_app_config::set_event_level(event_level level) {
  event_level_ = level;
}

void mgmt_app_config::set_hidden(bool hidden) { hidden_ = hidden; }

void mgmt_app_config::set_launch_only(bool launch_only) {
  launch_only_ = launch_only;
}

void mgmt_app_config::set_mount_location(provider_type prov,
                                         std::string_view name,
                                         std::string_view location) {
  if (name.empty()) {
    return;
  }

  recur_mutex_lock lock(mtx_);
  if (locations_[prov][std::string{name}] == std::string{location}) {
    return;
  }

  locations_[prov][std::string{name}] = std::string{location};

  save();
}

auto mgmt_app_config::to_json() const -> nlohmann::json {
  nlohmann::json data;
  data[JSON_ANIMATIONS] = animations_;
  data[JSON_API_PASSWORD] = api_password_;
  data[JSON_API_PORT] = api_port_;
  data[JSON_API_USER] = api_user_;
  data[JSON_AUTO_START] = auto_start_;
  data[JSON_EVENT_LEVEL] = event_level_to_string(event_level_);
  data[JSON_MOUNT_AUTO_START] = map_to_json(mount_auto_start_);
  data[JSON_MOUNT_LOCATIONS] = map_to_json(locations_);
  return data;
}
} // namespace repertory::ui
