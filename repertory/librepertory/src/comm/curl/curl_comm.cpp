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
#include "comm/curl/curl_comm.hpp"

#include "types/repertory.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"

namespace repertory {
const curl_comm::write_callback curl_comm::write_data =
    static_cast<curl_comm::write_callback>([](char *buffer, size_t size,
                                              size_t nitems,
                                              void *outstream) -> size_t {
      auto &info = *reinterpret_cast<read_write_info *>(outstream);
      std::copy(buffer, buffer + (size * nitems),
                std::back_inserter(info.data));
      return info.stop_requested_cb() ? 0 : size * nitems;
    });

const curl_comm::write_callback curl_comm::write_headers =
    static_cast<curl_comm::write_callback>([](char *buffer, size_t size,
                                              size_t nitems,
                                              void *outstream) -> size_t {
      auto &headers = *reinterpret_cast<http_headers *>(outstream);
      auto header = std::string(buffer, size * nitems);
      auto parts = utils::string::split(header, ':', false);
      if (parts.size() > 1U) {
        auto data = header.substr(parts[0U].size() + 1U);
        utils::string::left_trim(data);
        utils::string::right_trim(data, '\r');
        utils::string::right_trim(data, '\n');
        utils::string::right_trim(data, '\r');
        headers[utils::string::to_lower(parts[0U])] = data;
      }
      return size * nitems;
    });

curl_comm::curl_comm(host_config cfg)
    : host_config_(std::move(cfg)), s3_config_(std::nullopt) {}

curl_comm::curl_comm(s3_config cfg)
    : host_config_(std::nullopt), s3_config_(std::move(cfg)) {}

auto curl_comm::construct_url(CURL *curl, const std::string &relative_path,
                              const host_config &cfg) -> std::string {
  static constexpr auto http{80U};
  static constexpr auto https{443U};

  auto custom_port = (((cfg.protocol == "http") &&
                       (cfg.api_port == http || cfg.api_port == 0U)) ||
                      ((cfg.protocol == "https") &&
                       (cfg.api_port == https || cfg.api_port == 0U)))
                         ? ""
                         : ":" + std::to_string(cfg.api_port);
  auto url = cfg.protocol + "://" +
             utils::string::trim_copy(cfg.host_name_or_ip) + custom_port;

  static const auto complete_url = [](const std::string &current_path,
                                      const std::string &parent_path,
                                      std::string &final_url) -> std::string & {
    final_url += utils::path::create_api_path(current_path);
    if (utils::string::ends_with(parent_path, "/")) {
      final_url += '/';
    }
    return final_url;
  };

  auto path = utils::path::combine("/", {cfg.path});
  return relative_path.empty()
             ? complete_url(path, cfg.path, url)
             : complete_url(utils::path::combine(
                                path, {url_encode(curl, relative_path, true)}),
                            relative_path, url);
}

auto curl_comm::create_curl() -> CURL * { return reset_curl(curl_easy_init()); }

auto curl_comm::reset_curl(CURL *curl_handle) -> CURL * {
  curl_easy_reset(curl_handle);
#if defined(__APPLE__)
  curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
#endif // defined(__APPLE__)
  return curl_handle;
}

auto curl_comm::create_host_config(const s3_config &cfg) -> host_config {
  host_config host_cfg{};
  host_cfg.api_password = cfg.secret_key;
  host_cfg.api_user = cfg.access_key;

  auto pos = cfg.url.find(':');
  host_cfg.host_name_or_ip = cfg.url.substr(pos + 3U);
  if (cfg.use_region_in_url && not cfg.region.empty()) {
    auto parts = utils::string::split(host_cfg.host_name_or_ip, '.', true);
    if (parts.size() > 1U) {
      parts.insert(parts.begin() + 1U, cfg.region);
      host_cfg.host_name_or_ip = utils::string::join(parts, '.');
    }
  }

  if (not cfg.use_path_style) {
    host_cfg.host_name_or_ip = cfg.bucket + '.' + host_cfg.host_name_or_ip;
  }

  host_cfg.protocol = cfg.url.substr(0U, pos);
  if (cfg.use_path_style) {
    host_cfg.path = '/' + cfg.bucket;
  }

  return host_cfg;
}

auto curl_comm::make_request(const curl::requests::http_delete &del,
                             long &response_code,
                             stop_type &stop_requested) const -> bool {
  return make_request(s3_config_.has_value()
                          ? create_host_config(s3_config_.value())
                          : host_config_.value_or(host_config{}),
                      del, response_code, stop_requested);
}

auto curl_comm::make_request(const curl::requests::http_get &get,
                             long &response_code,
                             stop_type &stop_requested) const -> bool {
  return make_request(s3_config_.has_value()
                          ? create_host_config(s3_config_.value())
                          : host_config_.value_or(host_config{}),
                      get, response_code, stop_requested);
}

auto curl_comm::make_request(const curl::requests::http_head &head,
                             long &response_code,
                             stop_type &stop_requested) const -> bool {
  return make_request(s3_config_.has_value()
                          ? create_host_config(s3_config_.value())
                          : host_config_.value_or(host_config{}),
                      head, response_code, stop_requested);
}

auto curl_comm::make_request(const curl::requests::http_post &post,
                             long &response_code,
                             stop_type &stop_requested) const -> bool {
  return make_request(s3_config_.has_value()
                          ? create_host_config(s3_config_.value())
                          : host_config_.value_or(host_config{}),
                      post, response_code, stop_requested);
}

auto curl_comm::make_request(const curl::requests::http_put_file &put_file,
                             long &response_code,
                             stop_type &stop_requested) const -> bool {
  return make_request(s3_config_.has_value()
                          ? create_host_config(s3_config_.value())
                          : host_config_.value_or(host_config{}),
                      put_file, response_code, stop_requested);
}

auto curl_comm::url_encode(CURL *curl, const std::string &data,
                           bool allow_slash) -> std::string {
  auto *value =
      curl_easy_escape(curl, data.c_str(), static_cast<int>(data.size()));
  std::string ret = value;
  curl_free(value);

  if (allow_slash) {
    utils::string::replace(ret, "%2F", "/");
  }

  return ret;
}
} // namespace repertory
