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
#ifndef REPERTORY_INCLUDE_UI_MGMT_APP_CONFIG_HPP_
#define REPERTORY_INCLUDE_UI_MGMT_APP_CONFIG_HPP_

#include "types/repertory.hpp"

namespace repertory::ui {
class mgmt_app_config final {
public:
  mgmt_app_config();

private:
  atomic<std::string> api_password_{"repertory"};
  std::atomic<std::uint16_t> api_port_{default_ui_mgmt_port};
  atomic<std::string> api_user_{"repertory"};
  std::unordered_map<provider_type,
                     std::unordered_map<std::string, std::string>>
      locations_;
  mutable std::recursive_mutex mtx_;

private:
  void save() const;

public:
  [[nodiscard]] auto to_json() const -> nlohmann::json;

  [[nodiscard]] auto get_api_password() const -> std::string {
    return api_password_;
  }

  [[nodiscard]] auto get_api_port() const -> std::uint16_t { return api_port_; }

  [[nodiscard]] auto get_api_user() const -> std::string { return api_user_; }

  [[nodiscard]] auto get_mount_location(provider_type prov,
                                        std::string_view name) const
      -> std::string;

  void set_api_password(std::string_view api_password);

  void set_api_port(std::uint16_t api_port);

  void set_api_user(std::string_view api_user);

  void set_mount_location(provider_type prov, std::string_view name,
                          std::string_view location);
};
} // namespace repertory::ui

#endif // REPERTORY_INCLUDE_UI_MGMT_APP_CONFIG_HPP_
