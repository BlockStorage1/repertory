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
#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/unix.hpp"
#include "utils/windows.hpp"

namespace {
[[nodiscard]] auto from_json(const nlohmann::json &json)
    -> std::unordered_map<repertory::provider_type,
                          std::unordered_map<std::string, std::string>> {
  std::unordered_map<repertory::provider_type,
                     std::unordered_map<std::string, std::string>>
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
mgmt_app_config::mgmt_app_config() {
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
      locations_ = from_json(data.at(JSON_MOUNT_LOCATIONS));
      return;
    }

    utils::error::raise_error(
        function_name, utils::get_last_error_code(),
        fmt::format("failed to read file|{}", config_file));
    save();
  } catch (const std::exception &ex) {
    utils::error::raise_error(
        function_name, ex, fmt::format("failed to read file|{}", config_file));
  }
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
  data[JSON_API_PASSWORD] = api_password_;
  data[JSON_API_PORT] = api_port_;
  data[JSON_API_USER] = api_user_;
  data[JSON_MOUNT_LOCATIONS] = map_to_json(locations_);
  return data;
}
} // namespace repertory::ui
