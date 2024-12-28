/*
  Copyright <2018-2024> <scott.e.graves@protonmail.com>

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
#include "rpc/client/client.hpp"

#include "types/repertory.hpp"
#include "utils/base64.hpp"
#include "utils/utils.hpp"

namespace repertory {
client::client(rpc_host_info host_info) : host_info_(std::move(host_info)) {
  request_id_ = 0u;
}

auto client::get_drive_information() -> rpc_response {
  const auto base_url =
      "http://" + host_info_.host + ":" + std::to_string(host_info_.port);

  httplib::Client cli{base_url};
  cli.set_basic_auth(host_info_.user, host_info_.password);

  auto resp = cli.Get("/api/v1/" + rpc_method::get_drive_information);
  if (resp.error() != httplib::Error::Success) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", httplib::to_string(resp.error())}}};
  }
  if (resp->status != 200) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", std::to_string(resp->status)}}};
  }

  return rpc_response{rpc_response_type::success, json::parse(resp->body)};
}

auto client::get_config() -> rpc_response {
  const auto base_url =
      "http://" + host_info_.host + ":" + std::to_string(host_info_.port);

  httplib::Client cli{base_url};
  cli.set_basic_auth(host_info_.user, host_info_.password);

  auto resp = cli.Get("/api/v1/" + rpc_method::get_config);
  if (resp.error() != httplib::Error::Success) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", httplib::to_string(resp.error())}}};
  }
  if (resp->status != 200) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", std::to_string(resp->status)}}};
  }

  return rpc_response{rpc_response_type::success, json::parse(resp->body)};
}

auto client::get_config_value_by_name(const std::string &name) -> rpc_response {
  const auto base_url =
      "http://" + host_info_.host + ":" + std::to_string(host_info_.port);

  httplib::Params params{{"name", name}};
  httplib::Client cli{base_url};
  cli.set_basic_auth(host_info_.user, host_info_.password);

  auto resp =
      cli.Get("/api/v1/" + rpc_method::get_config_value_by_name, params, {});
  if (resp.error() != httplib::Error::Success) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", httplib::to_string(resp.error())}}};
  }
  if (resp->status != 200) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", std::to_string(resp->status)}}};
  }

  return rpc_response{rpc_response_type::success, json::parse(resp->body)};
}

auto client::get_directory_items(const std::string &api_path) -> rpc_response {
  const auto base_url =
      "http://" + host_info_.host + ":" + std::to_string(host_info_.port);

  httplib::Params params{{"api_path", api_path}};
  httplib::Client cli{base_url};
  cli.set_basic_auth(host_info_.user, host_info_.password);

  auto resp = cli.Get("/api/v1/" + rpc_method::get_directory_items, params, {});
  if (resp.error() != httplib::Error::Success) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", httplib::to_string(resp.error())}}};
  }
  if (resp->status != 200) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", std::to_string(resp->status)}}};
  }

  return rpc_response{rpc_response_type::success, json::parse(resp->body)};
}

auto client::get_open_files() -> rpc_response {
  const auto base_url =
      "http://" + host_info_.host + ":" + std::to_string(host_info_.port);

  httplib::Client cli{base_url};
  cli.set_basic_auth(host_info_.user, host_info_.password);

  auto resp = cli.Get("/api/v1/" + rpc_method::get_open_files);
  if (resp.error() != httplib::Error::Success) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", httplib::to_string(resp.error())}}};
  }
  if (resp->status != 200) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", std::to_string(resp->status)}}};
  }

  return rpc_response{rpc_response_type::success, json::parse(resp->body)};
}

auto client::get_pinned_files() -> rpc_response {
  const auto base_url =
      "http://" + host_info_.host + ":" + std::to_string(host_info_.port);

  httplib::Client cli{base_url};
  cli.set_basic_auth(host_info_.user, host_info_.password);

  auto resp = cli.Get("/api/v1/" + rpc_method::get_pinned_files);
  if (resp.error() != httplib::Error::Success) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", httplib::to_string(resp.error())}}};
  }
  if (resp->status != 200) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", std::to_string(resp->status)}}};
  }

  return rpc_response{rpc_response_type::success, json::parse(resp->body)};
}

auto client::pin_file(const std::string &api_path) -> rpc_response {
  const auto base_url =
      "http://" + host_info_.host + ":" + std::to_string(host_info_.port);

  httplib::Params params{{"api_path", api_path}};
  httplib::Client cli{base_url};
  cli.set_basic_auth(host_info_.user, host_info_.password);

  auto resp = cli.Post("/api/v1/" + rpc_method::pin_file, params);
  if (resp.error() != httplib::Error::Success) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", httplib::to_string(resp.error())}}};
  }
  if (resp->status != 200) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", std::to_string(resp->status)}}};
  }

  return rpc_response{rpc_response_type::success, {}};
}

auto client::pinned_status(const std::string &api_path) -> rpc_response {
  const auto base_url =
      "http://" + host_info_.host + ":" + std::to_string(host_info_.port);

  httplib::Params params{{"api_path", api_path}};
  httplib::Client cli{base_url};
  cli.set_basic_auth(host_info_.user, host_info_.password);

  auto resp = cli.Get("/api/v1/" + rpc_method::pinned_status, params, {});
  if (resp.error() != httplib::Error::Success) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", httplib::to_string(resp.error())}}};
  }
  if (resp->status != 200) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", std::to_string(resp->status)}}};
  }

  return rpc_response{rpc_response_type::success, json::parse(resp->body)};
}

auto client::set_config_value_by_name(
    const std::string &name, const std::string &value) -> rpc_response {
  const auto base_url =
      "http://" + host_info_.host + ":" + std::to_string(host_info_.port);

  httplib::Params params{
      {"name", name},
      {"value", value},
  };

  httplib::Client cli{base_url};
  cli.set_basic_auth(host_info_.user, host_info_.password);

  auto resp =
      cli.Post("/api/v1/" + rpc_method::set_config_value_by_name, params);
  if (resp.error() != httplib::Error::Success) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", httplib::to_string(resp.error())}}};
  }
  if (resp->status != 200) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", std::to_string(resp->status)}}};
  };

  return rpc_response{rpc_response_type::success,
                      nlohmann::json::parse(resp->body)};
}

auto client::unmount() -> rpc_response {
  const auto base_url =
      "http://" + host_info_.host + ":" + std::to_string(host_info_.port);

  httplib::Client cli{base_url};
  cli.set_basic_auth(host_info_.user, host_info_.password);

  auto resp = cli.Post("/api/v1/" + rpc_method::unmount);
  if (resp.error() != httplib::Error::Success) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", httplib::to_string(resp.error())}}};
  }
  if (resp->status != 200) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", std::to_string(resp->status)}}};
  }

  return rpc_response{rpc_response_type::success, {}};
}

auto client::unpin_file(const std::string &api_path) -> rpc_response {
  const auto base_url =
      "http://" + host_info_.host + ":" + std::to_string(host_info_.port);

  httplib::Params params{{"api_path", api_path}};
  httplib::Client cli{base_url};
  cli.set_basic_auth(host_info_.user, host_info_.password);

  auto resp = cli.Post("/api/v1/" + rpc_method::unpin_file, params);
  if (resp.error() != httplib::Error::Success) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", httplib::to_string(resp.error())}}};
  }
  if (resp->status != 200) {
    return rpc_response{rpc_response_type::http_error,
                        {{"error", std::to_string(resp->status)}}};
  }

  return rpc_response{rpc_response_type::success, {}};
}
} // namespace repertory
