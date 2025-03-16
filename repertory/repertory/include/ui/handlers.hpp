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
#ifndef REPERTORY_INCLUDE_UI_HANDLERS_HPP_
#define REPERTORY_INCLUDE_UI_HANDLERS_HPP_

#include "events/consumers/console_consumer.hpp"
#include <unordered_map>

namespace repertory::ui {
class mgmt_app_config;

class handlers final {
public:
  handlers(mgmt_app_config *config, httplib::Server *server);

  handlers() = delete;
  handlers(const handlers &) = delete;
  handlers(handlers &&) = delete;

  ~handlers();

  auto operator=(const handlers &) -> handlers & = delete;
  auto operator=(handlers &&) -> handlers & = delete;

private:
  mgmt_app_config *config_;
  std::string repertory_binary_;
  httplib::Server *server_;

private:
  console_consumer console;
  mutable std::mutex mtx_;
  mutable std::unordered_map<std::string, std::recursive_mutex> mtx_lookup_;

private:
  [[nodiscard]] auto data_directory_exists(provider_type prov,
                                           std::string_view name) const -> bool;

  void handle_get_mount(auto &&req, auto &&res) const;

  void handle_get_mount_list(auto &&res) const;

  void handle_get_mount_location(auto &&req, auto &&res) const;

  void handle_get_mount_status(auto &&req, auto &&res) const;

  void handle_get_settings(auto &&res) const;

  void handle_post_add_mount(auto &&req, auto &&res) const;

  void handle_post_mount(auto &&req, auto &&res) const;

  void handle_put_set_value_by_name(auto &&req, auto &&res) const;

  auto launch_process(provider_type prov, std::string_view name,
                      std::string_view args, bool background = false) const
      -> std::vector<std::string>;

  void set_key_value(provider_type prov, std::string_view name,
                     std::string_view key, std::string_view value) const;
};
} // namespace repertory::ui

#endif // REPERTORY_INCLUDE_UI_HANDLERS_HPP_
