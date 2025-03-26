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
#include "utils/common.hpp"

namespace repertory::ui {
class mgmt_app_config;

class handlers final {
private:
  static constexpr const auto nonce_length{128U};
  static constexpr const auto nonce_timeout{15U};

  struct nonce_data final {
    std::chrono::system_clock::time_point creation{
        std::chrono::system_clock::now(),
    };

    std::string nonce{
        utils::generate_random_string(nonce_length),
    };
  };

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
  std::mutex nonce_mtx_;
  std::unordered_map<std::string, nonce_data> nonce_lookup_;
  std::condition_variable nonce_notify_;
  std::unique_ptr<std::thread> nonce_thread_;
  stop_type stop_requested{false};

private:
  [[nodiscard]] auto data_directory_exists(provider_type prov,
                                           std::string_view name) const -> bool;

  static void handle_get_available_locations(httplib::Response &res);

  void handle_get_mount(const httplib::Request &req,
                        httplib::Response &res) const;

  void handle_get_mount_list(httplib::Response &res) const;

  void handle_get_mount_location(const httplib::Request &req,
                                 httplib::Response &res) const;

  void handle_get_mount_status(const httplib::Request &req,
                               httplib::Response &res) const;

  void handle_get_nonce(httplib::Response &res);

  void handle_get_settings(httplib::Response &res) const;

  void handle_post_add_mount(const httplib::Request &req,
                             httplib::Response &res) const;

  void handle_post_mount(const httplib::Request &req, httplib::Response &res);

  void handle_put_mount_location(const httplib::Request &req,
                                 httplib::Response &res) const;

  void handle_put_set_value_by_name(const httplib::Request &req,
                                    httplib::Response &res) const;

  void handle_put_settings(const httplib::Request &req,
                           httplib::Response &res) const;

  auto launch_process(provider_type prov, std::string_view name,
                      std::vector<std::string> args,
                      bool background = false) const
      -> std::vector<std::string>;

  void removed_expired_nonces();

  void set_key_value(provider_type prov, std::string_view name,
                     std::string_view key, std::string_view value) const;
};
} // namespace repertory::ui

#endif // REPERTORY_INCLUDE_UI_HANDLERS_HPP_
