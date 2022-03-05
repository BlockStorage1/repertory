/*
  Copyright <2018-2022> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute,
  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
  OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "rpc/client/client.hpp"
#include "comm/curl/curl_resolver.hpp"
#include "types/repertory.hpp"
#include "utils/Base64.hpp"
#include "utils/utils.hpp"

namespace repertory {
client::client(rpc_host_info host_info) : host_info_(std::move(host_info)) { request_id_ = 0u; }

rpc_response client::export_list(const std::vector<std::string> &paths) {
  auto ret = make_request(rpc_method::export_links, {paths});
  if (ret.response_type == rpc_response_type::success) {
    ret.data = ret.data["result"];
  }

  return ret;
}

rpc_response client::export_all() {
  auto ret = make_request(rpc_method::export_links, {}, 60000u);
  if (ret.response_type == rpc_response_type::success) {
    ret.data = ret.data["result"];
  }

  return ret;
}

rpc_response client::get_drive_information() {
  auto ret = make_request(rpc_method::get_drive_information, {});
  if (ret.response_type == rpc_response_type::success) {
    ret.data = ret.data["result"];
  }

  return ret;
}

rpc_response client::get_config() {
  auto ret = make_request(rpc_method::get_config, {});
  if (ret.response_type == rpc_response_type::success) {
    ret.data = ret.data["result"];
  }

  return ret;
}

rpc_response client::get_config_value_by_name(const std::string &name) {
  auto ret = make_request(rpc_method::get_config_value_by_name, {name});
  if (ret.response_type == rpc_response_type::success) {
    if (ret.data["result"]["value"].get<std::string>().empty()) {
      ret.response_type = rpc_response_type::config_value_not_found;
    } else {
      ret.data = ret.data["result"];
    }
  }

  return ret;
}

rpc_response client::get_directory_items(const std::string &api_path) {
  auto ret = make_request(rpc_method::get_directory_items, {api_path});
  if (ret.response_type == rpc_response_type::success) {
    ret.data = ret.data["result"];
  }

  return ret;
}

rpc_response client::get_open_files() {
  auto ret = make_request(rpc_method::get_open_files, {});
  if (ret.response_type == rpc_response_type::success) {
    ret.data = ret.data["result"];
  }

  return ret;
}

rpc_response client::get_pinned_files() {
  auto ret = make_request(rpc_method::get_pinned_files, {});
  if (ret.response_type == rpc_response_type::success) {
    ret.data = ret.data["result"];
  }

  return ret;
}

rpc_response client::import_skylink(const skylink_import_list &list) {
  std::vector<json> json_list;
  for (const auto &skynet_import : list) {
    json_list.emplace_back(skynet_import.to_json());
  }

  auto ret = make_request(rpc_method::import, {json(json_list)}, 60000u);
  if (ret.response_type == rpc_response_type::success) {
    ret.data = ret.data["result"];
  }

  return ret;
}

rpc_response client::make_request(const std::string &command, const std::vector<json> &args,
                                  std::uint32_t timeout_ms) {
  auto error = rpc_response_type::success;

  auto *curl_handle = utils::create_curl();

  const auto port = static_cast<std::uint16_t>(host_info_.port);
  const auto url = "http://" + host_info_.host + ":" + std::to_string(port) + "/api";
  const auto request = json({{"jsonrpc", "2.0"},
                             {"id", std::to_string(++request_id_)},
                             {"method", command},
                             {"params", args}})
                           .dump();

  struct curl_slist *hs = nullptr;
  hs = curl_slist_append(hs, "Content-Type: application/json;");
  if (not(host_info_.password.empty() && host_info_.user.empty())) {
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, &host_info_.user[0]);
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, &host_info_.password[0]);
  }
#ifndef __APPLE__
  curl_resolver resolver(curl_handle,
                         {"localhost:" + std::to_string(host_info_.port) + ":127.0.0.1"}, true);
#endif
  if (timeout_ms > 0) {
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, timeout_ms);
  }
  curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, hs);
  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, request.c_str());
  curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, request.size());
  curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5L);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,
                   static_cast<size_t (*)(char *, size_t, size_t, void *)>(
                       [](char *buffer, size_t size, size_t nitems, void *outstream) -> size_t {
                         (*reinterpret_cast<std::string *>(outstream)) +=
                             std::string(buffer, size * nitems);
                         return size * nitems;
                       }));

  std::string response;
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response);

  json json_data;
  const auto res = curl_easy_perform(curl_handle);
  if (res == CURLE_OK) {
    long httpErrorCode;
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &httpErrorCode);
    if (httpErrorCode == 200) {
      json_data = json::parse(response.begin(), response.end());
    } else {
      json parsed;
      try {
        parsed = json::parse(response.begin(), response.end());
      } catch (...) {
      }
      error = rpc_response_type::http_error;
      json_data = {{"error", {{"code", std::to_string(httpErrorCode)}}}};
      if (parsed.empty()) {
        json_data["error"]["response"] = response;
      } else {
        json_data["error"]["response"] = parsed;
      }
    }
  } else {
    error = rpc_response_type::curl_error;
    json_data = {{"error", {{"message", curl_easy_strerror(res)}}}};
  }
  curl_easy_cleanup(curl_handle);
  curl_slist_free_all(hs);

  return rpc_response({error, json_data});
}

rpc_response client::pin_file(const std::string &api_path) {
  auto ret = make_request(rpc_method::pin_file, {api_path});
  if (ret.response_type == rpc_response_type::success) {
    ret.data = ret.data["result"];
  }

  return ret;
}

rpc_response client::pinned_status(const std::string &api_path) {
  auto ret = make_request(rpc_method::pinned_status, {api_path});
  if (ret.response_type == rpc_response_type::success) {
    ret.data = ret.data["result"];
  }

  return ret;
}

rpc_response client::set_config_value_by_name(const std::string &name, const std::string &value) {
  auto ret = make_request(rpc_method::set_config_value_by_name, {name, value});
  if (ret.response_type == rpc_response_type::success) {
    if (ret.data["result"]["value"].get<std::string>().empty()) {
      ret.response_type = rpc_response_type::config_value_not_found;
    } else {
      ret.data = ret.data["result"];
    }
  }

  return ret;
}

rpc_response client::unmount() {
  auto ret = make_request(rpc_method::unmount, {});
  if (ret.response_type == rpc_response_type::success) {
    ret.data = ret.data["result"];
  }

  return ret;
}

rpc_response client::unpin_file(const std::string &api_path) {
  auto ret = make_request(rpc_method::unpin_file, {api_path});
  if (ret.response_type == rpc_response_type::success) {
    ret.data = ret.data["result"];
  }

  return ret;
}
} // namespace repertory
