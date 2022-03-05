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
#include "comm/curl/curl_comm.hpp"
#include "comm/curl/curl_resolver.hpp"
#include "curl/curl.h"
#include "types/repertory.hpp"
#include "utils/Base64.hpp"
#include "utils/encryption.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
struct curl_setup {
  bool allow_timeout = false;
  const host_config &hc;
  const http_parameters *parameters = nullptr;
  http_headers *headers = nullptr;
  bool post = false;
  raw_write_data *write_data = nullptr;
  std::string *write_string = nullptr;
  std::unique_ptr<curl_resolver> resolver;
};

struct raw_write_data {
  std::vector<char> *buffer;
  const bool &stop_requested;
};

struct read_data {
  const bool *stop_requested = nullptr;
  native_file *nf = nullptr;
  std::uint64_t offset = 0u;
};

CURL *curl_comm::common_curl_setup(const std::string &path, curl_setup &setup, std::string &url,
                                   std::string &fields) {
  return common_curl_setup(utils::create_curl(), path, setup, url, fields);
}

CURL *curl_comm::common_curl_setup(CURL *curl_handle, const std::string &path, curl_setup &setup,
                                   std::string &url, std::string &fields) {
  url = construct_url(curl_handle, path, setup.hc);
  if (setup.post) {
    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 0);
    curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    if (setup.parameters && not setup.parameters->empty()) {
      for (const auto &param : *setup.parameters) {
        if (not fields.empty()) {
          fields += "&";
        }
        if (param.first ==
            "new" + app_config::get_provider_path_name(config_.get_provider_type())) {
          fields += (param.first + "=" + url_encode(curl_handle, param.second));
        } else {
          fields += (param.first + "=" + param.second);
        }
      }
      curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, &fields[0]);
    } else {
      curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, "");
    }
  } else {
    curl_easy_setopt(curl_handle, CURLOPT_POST, 0);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1L);
    if (setup.parameters && not setup.parameters->empty()) {
      url += "?";
      for (const auto &param : *setup.parameters) {
        if (url[url.size() - 1] != '?') {
          url += "&";
        }
        url += (param.first + "=" + url_encode(curl_handle, param.second));
      }
    }
  }

  if (not setup.hc.agent_string.empty()) {
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, &setup.hc.agent_string[0]);
  }

  if (setup.write_data) {
    setup.write_data->buffer->clear();
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, setup.write_data);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data_callback_);
  } else if (setup.write_string) {
    setup.write_string->clear();
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, setup.write_string);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_string_callback_);
  }

  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

  if (setup.allow_timeout) {
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, setup.hc.timeout_ms);
  }

  if (not setup.hc.api_password.empty()) {
    curl_easy_setopt(curl_handle, CURLOPT_USERNAME, "");
    curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, &setup.hc.api_password[0]);
  }

  if (setup.headers) {
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, setup.headers);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, write_header_callback_);
  }

  std::vector<std::string> items = {"localhost:" + std::to_string(setup.hc.api_port) +
                                    ":127.0.0.1"};
  setup.resolver = std::make_unique<curl_resolver>(curl_handle, items);

  return curl_handle;
}

std::string curl_comm::construct_url(CURL *curl_handle, const std::string &relative_path,
                                     const host_config &hc) {
  auto custom_port = (((hc.protocol == "http") && (hc.api_port == 80u)) ||
                      ((hc.protocol == "https") && (hc.api_port == 443u)))
                         ? ""
                         : ":" + std::to_string(hc.api_port);
  auto ret = hc.protocol + "://" + utils::string::trim_copy(hc.host_name_or_ip) + custom_port;
  auto path = utils::path::combine("/", {hc.path});
  if (relative_path.empty()) {
    ret += utils::path::create_api_path(path);
    if (utils::string::ends_with(hc.path, "/")) {
      ret += '/';
    }
  } else {
    path = utils::path::combine(path, {url_encode(curl_handle, relative_path, true)});
    ret += utils::path::create_api_path(path);
    if (utils::string::ends_with(relative_path, "/")) {
      ret += '/';
    }
  }
  return ret;
}

bool curl_comm::create_auth_session(CURL *&curl_handle, const app_config &config, host_config hc,
                                    std::string &session) {
  auto ret = true;
  if (not curl_handle) {
    curl_handle = utils::create_curl();
  }

  if (not hc.auth_url.empty() && not hc.auth_user.empty()) {
    event_system::instance().raise<comm_auth_begin>(hc.auth_url, hc.auth_user);
    ret = false;
    session = utils::create_uuid_string();
    const auto cookie_path = utils::path::combine(config.get_data_directory(), {session + ".txt"});
    http_headers headers{};
    auto url = utils::string::right_trim(utils::string::trim(hc.auth_url), '/') + "/api/login";

    if (not hc.agent_string.empty()) {
      curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, &hc.agent_string[0]);
    }

    if (not hc.api_password.empty()) {
      curl_easy_setopt(curl_handle, CURLOPT_USERNAME, "");
      curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, &hc.api_password[0]);
    }

    struct curl_slist *hs = nullptr;
    hs = curl_slist_append(hs, "Content-Type: application/json");
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, hs);

    const auto payload = json({
                                  {"email", hc.auth_user},
                                  {"password", hc.auth_password},
                              })
                             .dump(2);
    curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, cookie_path.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_COOKIEJAR, cookie_path.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &headers);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, write_header_callback_);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, hc.timeout_ms);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, payload.c_str());

    long code{};
    auto res = curl_easy_perform(curl_handle);
    curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &code);
    if ((res == CURLE_OK) && ((code >= 200) && (code < 300))) {
      auto cookie = headers["set-cookie"];
      if (cookie.empty()) {
        code = 400;
      } else {
        cookie = headers["set-cookie"];
        if (cookie.empty() || not utils::string::contains(cookie, "skynet-jwt=")) {
          code = 401;
        } else {
          ret = true;
        }
      }
    }

    if (ret) {
      curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, "FLUSH");
      update_auth_session(utils::reset_curl(curl_handle), config, session);
    } else {
      curl_easy_cleanup(curl_handle);
      release_auth_session(config, hc, session);
    }
    curl_slist_free_all(hs);

    event_system::instance().raise<comm_auth_end>(hc.auth_url, hc.auth_user, res, code);
  }

  return ret;
}

api_error curl_comm::get_or_post(const host_config &hc, const bool &post, const std::string &path,
                                 const http_parameters &parameters, json &data, json &error,
                                 http_headers *headers, std::function<void(CURL *curl_handle)> cb) {
  std::string result;
  auto setup = curl_setup{not post, hc, &parameters, headers, post, nullptr, &result, nullptr};

  std::string fields;
  std::string url;
  auto *curl_handle = common_curl_setup(path, setup, url, fields);
  if (cb) {
    cb(curl_handle);
  }

  if (config_.get_event_level() >= event_level::verbose) {
    if (post) {
      event_system::instance().raise<comm_post_begin>(url, fields);
    } else {
      event_system::instance().raise<comm_get_begin>(url);
    }
  }

  std::unique_ptr<std::chrono::system_clock::time_point> tp(nullptr);
  if (config_.get_enable_comm_duration_events()) {
    tp = std::make_unique<std::chrono::system_clock::time_point>(std::chrono::system_clock::now());
  }

  const auto curl_code = curl_easy_perform(curl_handle);
  if (curl_code == CURLE_OPERATION_TIMEDOUT) {
    event_system::instance().raise<repertory_exception>(__FUNCTION__, "CURL timeout: " + path);
  }

  long http_code = -1;
  curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
  if (tp != nullptr) {
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - *tp);
    event_system::instance().raise<comm_duration>(url, std::to_string(duration.count()));
  }

  const auto ret = process_json_response(url, curl_code, http_code, result, data, error);
  if (config_.get_event_level() >= event_level::verbose) {
    if (post) {
      event_system::instance().raise<comm_post_end>(
          url, curl_code, http_code, ((ret == api_error::success) ? data.dump(2) : error.dump(2)));
    } else {
      event_system::instance().raise<comm_get_end>(
          url, curl_code, http_code, ((ret == api_error::success) ? data.dump(2) : error.dump(2)));
    }
  }

  curl_easy_cleanup(curl_handle);

  return ret;
}

api_error curl_comm::get_range(const host_config &hc, const std::string &path,
                               const std::uint64_t &data_size, const http_parameters &parameters,
                               const std::string &encryption_token, std::vector<char> &data,
                               const http_ranges &ranges, json &error, http_headers *headers,
                               const bool &stop_requested) {
  if (encryption_token.empty()) {
    return get_range_unencrypted(hc, path, parameters, data, ranges, error, headers,
                                 stop_requested);
  }

  if (ranges.empty()) {
    return api_error::error;
  }

  const auto key = utils::encryption::generate_key(encryption_token);
  for (const auto &range : ranges) {
    const auto result = utils::encryption::read_encrypted_range(
        range, key,
        [&](std::vector<char> &ct, const std::uint64_t &start_offset,
            const std::uint64_t &end_offset) -> api_error {
          const auto ret =
              get_range_unencrypted(hc, path, parameters, ct, {{start_offset, end_offset}}, error,
                                    headers, stop_requested);
          headers = nullptr;
          return ret;
        },
        data_size, data);
    if (result != api_error::success) {
      return result;
    }
  }

  return api_error::success;
}

api_error curl_comm::get_range_unencrypted(const host_config &hc, const std::string &path,
                                           const http_parameters &parameters,
                                           std::vector<char> &data, const http_ranges &ranges,
                                           json &error, http_headers *headers,
                                           const bool &stop_requested) {
  raw_write_data wd = {&data, stop_requested};

  auto setup = curl_setup{false, hc, &parameters, headers, false, &wd, nullptr, nullptr};

  std::string fields;
  std::string url;
  auto *curl_handle = common_curl_setup(path, setup, url, fields);

  std::string range_list;
  if (not ranges.empty()) {
    range_list = std::accumulate(
        std::next(ranges.begin()), ranges.end(), http_range_to_string(ranges[0]),
        [](const auto &l, const auto &r) { return l + ',' + http_range_to_string(r); });
    curl_easy_setopt(curl_handle, CURLOPT_RANGE, &range_list[0]);
  }

  return execute_binary_operation<comm_get_range_begin, comm_get_range_end>(curl_handle, url, data,
                                                                            error, stop_requested);
}

api_error curl_comm::get_raw(const host_config &hc, const std::string &path,
                             const http_parameters &parameters, std::vector<char> &data,
                             json &error, const bool &stop_requested) {
  raw_write_data wd = {&data, stop_requested};

  auto setup = curl_setup{false, hc, &parameters, nullptr, false, &wd, nullptr, nullptr};

  std::string fields;
  std::string url;
  return execute_binary_operation<comm_get_range_begin, comm_get_range_end>(
      common_curl_setup(path, setup, url, fields), url, data, error, stop_requested);
}

std::string curl_comm::http_range_to_string(const http_range &range) {
  return std::to_string(range.begin) + '-' + std::to_string(range.end);
}

api_error curl_comm::post_file(const host_config &hc, const std::string &path,
                               const std::string &source_path, const http_parameters &parameters,
                               json &data, json &error, const bool &stop_requested) {
  auto ret = api_error::os_error;
  std::uint64_t file_size{};
  if (utils::file::get_file_size(source_path, file_size)) {
    std::string result;
    auto setup = curl_setup{false, hc, &parameters, nullptr, true, nullptr, &result, nullptr};

    std::string fields;
    std::string url;
    auto *curl_handle = common_curl_setup(path, setup, url, fields);
    curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE_LARGE, file_size);

    native_file::native_file_ptr nf;
    native_file::create_or_open(source_path, nf);
    if (nf) {
      read_data rd = {&stop_requested};
      rd.nf = nf.get();
      curl_easy_setopt(curl_handle, CURLOPT_READDATA, &rd);
      curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_data_callback_);

      ret = execute_json_operation<comm_post_file_begin, comm_post_file_end>(
          curl_handle, url, result, data, error, stop_requested);
      nf->close();
    }
  }

  return ret;
}

api_error curl_comm::post_multipart_file(const host_config &hc, const std::string &path,
                                         const std::string &file_name,
                                         const std::string &source_path,
                                         const std::string &encryption_token, json &data,
                                         json &error, const bool &stop_requested) {
  std::string result;
  std::uint64_t file_size{};
  std::unique_ptr<utils::encryption::encrypting_reader> reader;
  if (encryption_token.empty()) {
    if (not utils::file::get_file_size(source_path, file_size)) {
      return api_error::os_error;
    }
  } else {
    try {
      reader = std::make_unique<utils::encryption::encrypting_reader>(
          file_name, source_path, stop_requested, encryption_token);
      file_size = reader->get_total_size();
    } catch (const std::exception &e) {
      event_system::instance().raise<repertory_exception>(__FUNCTION__, e.what());
      return api_error::error;
    }
  }

  std::string session;
  CURL *curl_handle = nullptr;
  if (not session_manager_.create_auth_session(curl_handle, config_, hc, session)) {
    return api_error::access_denied;
  }

  if (config_.get_provider_type() == provider_type::skynet) {
    curl_easy_cleanup(curl_handle);

    const auto fn = reader ? reader->get_encrypted_file_name() : file_name;
    const auto fs = reader ? reader->get_total_size() : file_size;

    std::string location;
    if (tus_upload_create(hc, fn, fs, location)) {
      std::string skylink;
      if (tus_upload(hc, source_path, fn, fs, location, skylink, stop_requested, reader.get())) {
        data["skylink"] = skylink;
        return api_error::success;
      }
    }

    return api_error::comm_error;
  }

  auto setup = curl_setup{false, hc, nullptr, nullptr, true, nullptr, &result, nullptr};

  std::string fields;
  std::string url;
  common_curl_setup(curl_handle, path, setup, url, fields);

  auto *curl_mime = curl_mime_init(curl_handle);
  auto *mime_part = curl_mime_addpart(curl_mime);
  curl_mime_name(mime_part, "file");

  auto ret = api_error::success;
  if (encryption_token.empty()) {
    curl_mime_filename(mime_part, &file_name[0]);
    curl_mime_filedata(mime_part, &source_path[0]);
  } else {
    try {
      curl_mime_filename(mime_part, reader->get_encrypted_file_name().c_str());
      curl_mime_data_cb(
          mime_part, reader->get_total_size(),
          static_cast<curl_read_callback>(utils::encryption::encrypting_reader::reader_function),
          nullptr, nullptr, reader.get());
    } catch (const std::exception &e) {
      event_system::instance().raise<repertory_exception>(__FUNCTION__, e.what());
      ret = api_error::error;
    }
  }

  if (ret == api_error::success) {
    curl_easy_setopt(curl_handle, CURLOPT_MIMEPOST, curl_mime);

    ret = execute_json_operation<comm_post_multi_part_file_begin, comm_post_multi_part_file_end>(
        curl_handle, url, result, data, error, stop_requested,
        (ret == api_error::success) ? CURLE_OK : CURLE_SEND_ERROR);
  }

  curl_mime_free(curl_mime);
  curl_easy_cleanup(curl_handle);

  /*   if (not session.empty() && (ret == api_error::success)) { */
  /*     auto pin_hc = hc; */
  /*  */
  /*     const auto skylink = data["skylink"].get<std::string>(); */
  /*     utils::string::replace(pin_hc.path, "/skyfile", "/pin/" + skylink); */
  /*  */
  /*     ret = api_error::comm_error; */
  /*     for (std::uint8_t i = 0u; not stop_requested && (i < 30u) && (ret != api_error::success);
   * i++) { */
  /*       if (i) { */
  /*         event_system::instance().raise<repertory_exception>( */
  /*             __FUNCTION__, "RETRY [" + std::to_string(i) + "] Pin failed for file: " +
   * file_name); */
  /*         std::this_thread::sleep_for(1s); */
  /*       } */
  /*  */
  /*       json response; */
  /*       http_headers headers{}; */
  /*       ret = get_or_post(pin_hc, true, "", {}, response, error, &headers, [&](CURL *curl_handle)
   * { */
  /*         session_manager_.update_auth_session(curl_handle, config_, hc); */
  /*       }); */
  /*     } */
  /*   } */

  session_manager_.release_auth_session(config_, hc, session);

  return ret;
}

api_error curl_comm::process_binary_response(const std::string &url, const CURLcode &res,
                                             const long &http_code, std::vector<char> data,
                                             json &error) {
  const auto ret = process_response(
      url, res, http_code, data.size(),
      [&]() -> std::string { return (data.empty() ? "" : std::string(&data[0], data.size())); },
      nullptr, error);

  if (ret != api_error::success) {
    data.clear();
  }

  return ret;
}

api_error curl_comm::process_json_response(const std::string &url, const CURLcode &res,
                                           const long &http_code, const std::string &result,
                                           json &data, json &error) {
  const auto ret = process_response(
      url, res, http_code, result.size(), [&]() -> std::string { return result; },
      [&]() {
        if (result.length()) {
          data = json::parse(result.c_str());
        }
      },
      error);

  if (ret != api_error::success) {
    data.clear();
  }

  return ret;
}

api_error curl_comm::process_response(const std::string &url, const CURLcode &res,
                                      const long &http_code, const std::size_t &data_size,
                                      const std::function<std::string()> &to_string_convertor,
                                      const std::function<void()> &success_handler,
                                      json &error) const {
  auto ret = api_error::success;

  auto construct_error = [&]() {
    ret = api_error::comm_error;
    const auto *curl_string = curl_easy_strerror(res);
    std::string error_string(curl_string ? curl_string : "");
    error["message"] = error_string + ":" + std::to_string(http_code);
    error["url"] = url;
  };

  if ((res == CURLE_OK) && ((http_code >= 200) && (http_code < 300))) {
    if (success_handler) {
      success_handler();
    }
  } else if (data_size == 0u) {
    construct_error();
  } else {
    try {
      const auto tmp = json::parse(to_string_convertor().c_str());
      if (tmp.find("message") != tmp.end()) {
        ret = api_error::comm_error;
        error = tmp;
        error["url"] = url;
      } else {
        construct_error();
      }
    } catch (...) {
      construct_error();
    }
  }

  return ret;
}

void curl_comm::release_auth_session(const app_config &config, host_config hc,
                                     const std::string &session) {
  if (not hc.auth_url.empty() && not hc.auth_user.empty()) {
    event_system::instance().raise<comm_auth_logout_begin>(hc.auth_url, hc.auth_user);
    const auto cookie_path = utils::path::combine(config.get_data_directory(), {session + ".txt"});
    const auto url =
        utils::string::right_trim(utils::string::trim(hc.auth_url), '/') + "/api/logout";

    auto *curl_handle = utils::create_curl();
    if (not hc.agent_string.empty()) {
      curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, &hc.agent_string[0]);
    }

    if (not hc.api_password.empty()) {
      curl_easy_setopt(curl_handle, CURLOPT_USERNAME, "");
      curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, &hc.api_password[0]);
    }

    curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, cookie_path.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_COOKIEJAR, cookie_path.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, hc.timeout_ms);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_null_callback_);

    const auto curl_code = curl_easy_perform(curl_handle);

    long http_code = -1;
    curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &http_code);
    curl_easy_cleanup(curl_handle);

    event_system::instance().raise<comm_auth_logout_end>(hc.auth_url, hc.auth_user, curl_code,
                                                         http_code);

    utils::file::delete_file(cookie_path);
  }
}

bool curl_comm::tus_upload(host_config hc, const std::string &source_path,
                           const std::string &file_name, std::uint64_t file_size,
                           const std::string &location, std::string &skylink,
                           const bool &stop_requested,
                           utils::encryption::encrypting_reader *reader) {
  static const constexpr std::uint64_t max_skynet_file_size = (1ull << 22ull) * 10ull;

  auto tus_hc = hc;
  utils::string::replace(tus_hc.path, "/skyfile", "/tus");

  auto ret = true;
  std::uint64_t offset = 0u;
  native_file::native_file_ptr nf;
  while (ret && file_size) {
    const auto chunk_size = std::min(max_skynet_file_size, file_size);

    auto *curl_handle = utils::create_curl();
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 0);
    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 0);
    const auto upload_offset = "Upload-Offset: " + std::to_string(offset);
    const auto content_length = "Content-Length: " + std::to_string(chunk_size);

    struct curl_slist *hs = nullptr;
    hs = curl_slist_append(hs, "tus-resumable: 1.0.0");
    hs = curl_slist_append(hs, "Content-Type: application/offset+octet-stream");
    hs = curl_slist_append(hs, content_length.c_str());
    hs = curl_slist_append(hs, upload_offset.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, hs);

    curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE_LARGE, chunk_size);
    curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1L);

    session_manager_.update_auth_session(curl_handle, config_, hc);

    read_data rd = {&stop_requested};
    if (reader) {
      curl_easy_setopt(curl_handle, CURLOPT_READDATA, reader);
      curl_easy_setopt(
          curl_handle, CURLOPT_READFUNCTION,
          static_cast<curl_read_callback>(utils::encryption::encrypting_reader::reader_function));
    } else {
      if (not nf) {
        native_file::create_or_open(source_path, nf);
      }

      if (nf) {
        rd.nf = nf.get();
        rd.offset = offset;
        curl_easy_setopt(curl_handle, CURLOPT_READDATA, &rd);
        curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_data_callback_);
      } else {
        ret = false;
      }
    }

    if (ret) {
      event_system::instance().raise<comm_tus_upload_begin>(file_name, location, file_size, offset);
      curl_easy_setopt(curl_handle, CURLOPT_URL, location.c_str());

      const auto curl_code = curl_easy_perform(curl_handle);

      long http_code = -1;
      curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &http_code);

      event_system::instance().raise<comm_tus_upload_end>(file_name, location, file_size, offset,
                                                          curl_code, http_code);
      if ((ret = ((curl_code == CURLE_OK) && (http_code >= 200) && (http_code < 300)))) {
        file_size -= chunk_size;
        offset += chunk_size;
      }
    }

    curl_easy_cleanup(curl_handle);
    curl_slist_free_all(hs);
  }

  if (nf) {
    nf->close();
  }

  if (ret) {
    auto *curl_handle = utils::create_curl();

    http_headers headers{};
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "HEAD");
    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &headers);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, write_header_callback_);

    session_manager_.update_auth_session(curl_handle, config_, hc);

    struct curl_slist *hs = nullptr;
    hs = curl_slist_append(hs, "tus-resumable: 1.0.0");
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, hs);

    curl_easy_setopt(curl_handle, CURLOPT_URL, location.c_str());

    const auto res = curl_easy_perform(curl_handle);
    long http_code = -1;
    curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &http_code);

    ret = ((res == CURLE_OK) && (http_code >= 200) && (http_code < 300));

    skylink = headers["skynet-skylink"];

    curl_easy_cleanup(curl_handle);
    curl_slist_free_all(hs);
  }

  return ret;
}

bool curl_comm::tus_upload_create(host_config hc, const std::string &fileName,
                                  const std::uint64_t &fileSize, std::string &location) {
  auto tus_hc = hc;
  utils::string::replace(tus_hc.path, "/skyfile", "/tus");

  auto *curl_handle = utils::create_curl();

  const auto url = construct_url(curl_handle, "", tus_hc);
  event_system::instance().raise<comm_tus_upload_create_begin>(fileName, url);

  http_headers headers{};
  curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 0);
  curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
  curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, "");
  curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &headers);
  curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, write_header_callback_);

  const auto upload_length = "upload-length: " + utils::string::from_uint64(fileSize);
  const auto upload_metadata = "upload-metadata: filename " + macaron::Base64::Encode(fileName) +
                               ",filetype " + macaron::Base64::Encode("application/octet-stream");

  struct curl_slist *hs = nullptr;
  hs = curl_slist_append(hs, "tus-resumable: 1.0.0");
  hs = curl_slist_append(hs, "Content-Length: 0");
  hs = curl_slist_append(hs, upload_length.c_str());
  hs = curl_slist_append(hs, upload_metadata.c_str());
  curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, hs);

  session_manager_.update_auth_session(curl_handle, config_, hc);

  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

  const auto res = curl_easy_perform(curl_handle);

  long http_code = -1;
  curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &http_code);

  const auto ret = (res == CURLE_OK && http_code == 201);
  if (ret) {
    location = headers["location"];
  }

  curl_easy_cleanup(curl_handle);
  curl_slist_free_all(hs);

  event_system::instance().raise<comm_tus_upload_create_end>(fileName, url, res, http_code);

  return ret;
}

void curl_comm::update_auth_session(CURL *curl_handle, const app_config &config,
                                    const std::string &session) {
  const auto cookie_path = utils::path::combine(config.get_data_directory(), {session + ".txt"});
  curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, cookie_path.c_str());
  curl_easy_setopt(curl_handle, CURLOPT_COOKIEJAR, cookie_path.c_str());
}

std::string curl_comm::url_encode(CURL *curl_handle, const std::string &data,
                                  const bool &allow_slash) {
  auto *value = curl_easy_escape(curl_handle, data.c_str(), 0);
  std::string ret = value;
  curl_free(value);

  if (allow_slash) {
    utils::string::replace(ret, "%2F", "/");
  }
  return ret;
}

curl_comm::curl_read_callback curl_comm::read_data_callback_ =
    static_cast<curl_comm::curl_read_callback>(
        [](char *buffer, size_t size, size_t nitems, void *instream) -> size_t {
          auto *rd = reinterpret_cast<read_data *>(instream);
          std::size_t bytes_read{};
          const auto ret = rd->nf->read_bytes(buffer, size * nitems, rd->offset, bytes_read);
          if (ret) {
            rd->offset += bytes_read;
          }
          return ret && not *rd->stop_requested ? bytes_read : CURL_READFUNC_ABORT;
        });

curl_comm::curl_write_callback curl_comm::write_data_callback_ =
    static_cast<curl_comm::curl_write_callback>(
        [](char *buffer, size_t size, size_t nitems, void *outstream) -> size_t {
          auto *wd = reinterpret_cast<raw_write_data *>(outstream);
          std::copy(buffer, buffer + (size * nitems), std::back_inserter(*wd->buffer));
          return wd->stop_requested ? 0 : size * nitems;
        });

curl_comm::curl_write_callback curl_comm::write_header_callback_ =
    static_cast<curl_comm::curl_write_callback>(
        [](char *buffer, size_t size, size_t nitems, void *outstream) -> size_t {
          auto &headers = *reinterpret_cast<http_headers *>(outstream);
          const auto header = std::string(buffer, size * nitems);
          const auto parts = utils::string::split(header, ':');
          if (parts.size() > 1u) {
            auto data = header.substr(parts[0u].size() + 1u);
            utils::string::left_trim(data);
            utils::string::right_trim(data, '\r');
            utils::string::right_trim(data, '\n');
            utils::string::right_trim(data, '\r');
            headers[utils::string::to_lower(parts[0u])] = data;
          }
          return size * nitems;
        });

curl_comm::curl_write_callback curl_comm::write_null_callback_ =
    static_cast<curl_comm::curl_write_callback>(
        [](char *, size_t size, size_t nitems, void *) -> size_t { return size * nitems; });

curl_comm::curl_write_callback curl_comm::write_string_callback_ =
    static_cast<curl_comm::curl_write_callback>(
        [](char *buffer, size_t size, size_t nitems, void *outstream) -> size_t {
          (*reinterpret_cast<std::string *>(outstream)) += std::string(buffer, size * nitems);
          return size * nitems;
        });
} // namespace repertory
