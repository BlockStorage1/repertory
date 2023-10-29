/*
  Copyright <2018-2023> <scott.e.graves@protonmail.com>

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
#if defined(REPERTORY_ENABLE_S3)

#include "comm/s3/s3_comm.hpp"

#include "app_config.hpp"
#include "comm/curl/curl_comm.hpp"
#include "comm/s3/s3_requests.hpp"
#include "events/event_system.hpp"
#include "events/events.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "types/s3.hpp"
#include "utils/encryption.hpp"
#include "utils/error_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/polling.hpp"
#include "utils/string_utils.hpp"

namespace repertory {
static const get_key_callback empty_key = []() { return ""; };

s3_comm::s3_comm(const app_config &config)
    : config_(config), s3_config_(config.get_s3_config()) {
  s3_config_.bucket = utils::string::trim(s3_config_.bucket);

  // TODO make configurable
  const auto enable_path_style =
      utils::string::begins_with(s3_config_.url, "http://localhost") ||
      utils::string::begins_with(s3_config_.url, "https://localhost") ||
      utils::string::begins_with(s3_config_.url, "http://127.0.0.1") ||
      utils::string::begins_with(s3_config_.url, "https://127.0.0.1");

  s3_client_ = std::make_unique<curl_comm>(s3_config_);
  s3_client_->enable_s3_path_style(enable_path_style);

  polling::instance().set_callback(
      {"s3_directory_cache", polling::frequency::high,
       [this]() { this->clear_expired_directories(); }});
}

s3_comm::s3_comm(s3_comm &&comm)
    : config_(std::move(comm.config_)),
      s3_config_(std::move(comm.s3_config_)),
      s3_client_(std::move(comm.s3_client_)) {
  comm.active_ = false;

  polling::instance().set_callback(
      {"s3_directory_cache", polling::frequency::high,
       [this]() { this->clear_expired_directories(); }});
}

s3_comm::~s3_comm() {
  if (active_) {
    polling::instance().remove_callback("s3_directory_cache");
  }
}

void s3_comm::clear_expired_directories() {
  recur_mutex_lock l(cached_directories_mutex_);
  std::vector<std::string> expired_list;
  for (const auto &kv : cached_directories_) {
    if (kv.second.expiration <= std::chrono::system_clock::now()) {
      expired_list.emplace_back(kv.first);
    }
  }

  for (const auto &expired : expired_list) {
    event_system::instance().raise<debug_log>(__FUNCTION__, expired, "expired");
    cached_directories_.erase(expired);
  }
}

auto s3_comm::create_directory(const std::string &api_path) -> api_error {
  raise_begin(__FUNCTION__, api_path);

  long response_code{};

  auto object_name = get_object_name(api_path, empty_key) + '/';
  if (not create_directory_object_request(*s3_client_, s3_config_, object_name,
                                          response_code)) {
    return raise_end(__FUNCTION__, api_path, api_error::comm_error,
                     response_code);
  }

  return raise_end(__FUNCTION__, api_path,
                   response_code == 200 ? api_error::success
                                        : api_error::comm_error,
                   response_code);
}

auto s3_comm::directory_exists(const std::string &api_path) const -> api_error {
  raise_begin(__FUNCTION__, api_path);

  auto object_name = get_object_name(api_path, empty_key) + "/";

  head_object_result result{};
  long response_code{};
  if (head_object_request(*s3_client_, s3_config_, object_name, result,
                          response_code)) {
    if (response_code == 404) {
      return raise_end(__FUNCTION__, api_path, api_error::directory_not_found,
                       response_code);
    }

    return raise_end(__FUNCTION__, api_path, api_error::directory_exists,
                     response_code);
  }

  return raise_end(__FUNCTION__, api_path, api_error::comm_error,
                   response_code);
}

auto s3_comm::file_exists(const std::string &api_path,
                          const get_key_callback &get_key) const -> api_error {
  raise_begin(__FUNCTION__, api_path);

  if (get_cached_file_exists(api_path)) {
    return raise_end(__FUNCTION__, api_path, api_error::item_exists, 200);
  }

  auto object_name = get_object_name(api_path, get_key);

  head_object_result result{};
  long response_code{};
  if (head_object_request(*s3_client_, s3_config_, object_name, result,
                          response_code)) {
    if (response_code == 404) {
      return raise_end(__FUNCTION__, api_path, api_error::item_not_found,
                       response_code);
    }

    return raise_end(__FUNCTION__, api_path, api_error::item_exists,
                     response_code);
  }

  return raise_end(__FUNCTION__, api_path, api_error::directory_exists,
                   response_code);
}

auto s3_comm::get_object_list(std::vector<directory_item> &list) const
    -> api_error {
  raise_begin(__FUNCTION__, "/");

  long response_code{};
  auto success =
      list_objects_request(*s3_client_, s3_config_, list, response_code);
  return raise_end(__FUNCTION__, "/",
                   success ? api_error::success : api_error::comm_error,
                   response_code);
}

auto s3_comm::get_object_name(const std::string &api_path,
                              const get_key_callback &get_key) const
    -> std::string {
  auto object_name = utils::path::create_api_path(api_path).substr(1);

  const auto key = get_key();
  if (not key.empty()) {
    auto parts = utils::string::split(object_name, '/', false);
    parts[parts.size() - 1u] = key;
    object_name = utils::string::join(parts, '/');
  }

  return object_name;
}

auto s3_comm::get_cached_directory_item_count(const std::string &api_path,
                                              std::size_t &count) const
    -> bool {
  recur_mutex_lock l(cached_directories_mutex_);
  if (cached_directories_.find(api_path) != cached_directories_.end()) {
    count = cached_directories_.at(api_path).items.size();
    return true;
  }

  return false;
}

auto s3_comm::get_cached_directory_items(const std::string &api_path,
                                         meta_provider_callback meta_provider,
                                         directory_item_list &list) const
    -> bool {
  unique_recur_mutex_lock l(cached_directories_mutex_);
  if (cached_directories_.find(api_path) != cached_directories_.end()) {
    auto &cachedEntry = cached_directories_.at(api_path);
    list = cachedEntry.items;
    cached_directories_[api_path].reset_timeout(
        std::chrono::seconds(config_.get_s3_config().cache_timeout_secs));
    l.unlock();

    for (auto &item : list) {
      meta_provider(item);
    }
    return true;
  }
  return false;
}

auto s3_comm::get_cached_file_exists(const std::string &api_path) const
    -> bool {
  unique_recur_mutex_lock l(cached_directories_mutex_);
  const auto parent_api_path = utils::path::get_parent_api_path(api_path);
  if (cached_directories_.find(parent_api_path) != cached_directories_.end()) {
    auto &entry = cached_directories_.at(parent_api_path);
    if (std::find_if(entry.items.begin(), entry.items.end(),
                     [&api_path](const auto &item) -> bool {
                       return not item.directory && (api_path == item.api_path);
                     }) != entry.items.end()) {
      cached_directories_[api_path].reset_timeout(
          std::chrono::seconds(config_.get_s3_config().cache_timeout_secs));
      return true;
    }
  }

  return false;
}

auto s3_comm::get_directory_item_count(
    const std::string &api_path, meta_provider_callback meta_provider) const
    -> std::size_t {
  raise_begin(__FUNCTION__, api_path);

  std::size_t ret = 0u;
  if (not get_cached_directory_item_count(api_path, ret)) {
    directory_item_list list;
    const auto res = grab_directory_items(api_path, meta_provider, list);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                         "failed to grab directory items");
    }
    return list.size();
  }

  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path,
                                              "end|" + std::to_string(ret));
  }

  return ret;
}

auto s3_comm::get_directory_items(const std::string &api_path,
                                  meta_provider_callback meta_provider,
                                  directory_item_list &list) const
    -> api_error {
  raise_begin(__FUNCTION__, api_path);

  auto ret = api_error::success;
  if (not get_cached_directory_items(api_path, meta_provider, list)) {
    ret = grab_directory_items(api_path, meta_provider, list);
  }

  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(
        __FUNCTION__, api_path, "end|" + api_error_to_string(ret));
  }
  return ret;
}

auto s3_comm::get_directory_list(api_file_list &list) const -> api_error {
  raise_begin(__FUNCTION__, "/");

  long response_code{};
  auto success =
      list_directories_request(*s3_client_, s3_config_, list, response_code);
  return raise_end(__FUNCTION__, "/",
                   success ? api_error::success : api_error::comm_error,
                   response_code);
}

auto s3_comm::get_file(const std::string &api_path,
                       const get_key_callback &get_key,
                       const get_name_callback &get_name,
                       const get_token_callback &get_token,
                       api_file &file) const -> api_error {
  raise_begin(__FUNCTION__, api_path);

  auto ret = api_error::success;

  auto object_name = get_object_name(api_path, get_key);

  head_object_result result{};
  long response_code{};
  if (head_object_request(*s3_client_, s3_config_, object_name, result,
                          response_code)) {
    const auto key = get_key();
    object_name = get_name(key, object_name);
    file.accessed_date = utils::get_file_time_now();
    file.api_path = utils::path::create_api_path(object_name);
    file.api_parent = utils::path::get_parent_api_path(file.api_path);
    file.changed_date = utils::aws::format_time(result.last_modified);
    file.creation_date = utils::aws::format_time(result.last_modified);
    file.encryption_token = get_token();
    file.file_size =
        file.encryption_token.empty()
            ? result.content_length
            : utils::encryption::encrypting_reader::calculate_decrypted_size(
                  result.content_length);
    file.modified_date = utils::aws::format_time(result.last_modified);
  } else {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, response_code,
                                       "head object request failed");
    ret = api_error::comm_error;
  }

  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(
        __FUNCTION__, api_path, "end|" + api_error_to_string(ret));
  }

  return ret;
}

auto s3_comm::get_file_list(
    const get_api_file_token_callback &get_api_file_token,
    const get_name_callback &get_name, api_file_list &list) const -> api_error {
  raise_begin(__FUNCTION__, "/");

  long response_code{};
  auto success = list_files_request(*s3_client_, s3_config_, get_api_file_token,
                                    get_name, list, response_code);
  return raise_end(__FUNCTION__, "/",
                   success ? api_error::success : api_error::comm_error,
                   response_code);
}

auto s3_comm::grab_directory_items(const std::string &api_path,
                                   meta_provider_callback meta_provider,
                                   directory_item_list &list) const
    -> api_error {
  auto object_name = get_object_name(api_path, empty_key);
  long response_code{};
  if (list_objects_in_directory_request(*s3_client_, s3_config_, object_name,
                                        meta_provider, list, response_code)) {
    if (response_code == 404) {
      return api_error::directory_not_found;
    }

    if (response_code != 200) {
      return api_error::comm_error;
    }

    set_cached_directory_items(api_path, list);
    return api_error::success;
  }

  return api_error::comm_error;
}

void s3_comm::raise_begin(const std::string &function_name,
                          const std::string &api_path) const {
  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(function_name, api_path,
                                              "begin|");
  }
}

auto s3_comm::raise_end(const std::string &function_name,
                        const std::string &api_path, const api_error &error,
                        long code) const -> api_error {
  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(
        function_name, api_path,
        "end|" + api_error_to_string(error) + '|' + std::to_string(code));
  }

  return error;
}

auto s3_comm::read_file_bytes(const std::string &api_path, std::size_t size,
                              std::uint64_t offset, data_buffer &data,
                              const get_key_callback &get_key,
                              const get_size_callback &get_size,
                              const get_token_callback &get_token,
                              stop_type &stop_requested) const -> api_error {
  data.clear();

  auto object_name = get_object_name(api_path, get_key);

  const auto encryption_token = get_token();
  const auto data_size = get_size();
  if (encryption_token.empty()) {
    long response_code{};
    if (not read_object_request(*s3_client_, s3_config_, object_name, size,
                                offset, data, response_code, stop_requested)) {
      auto res =
          stop_requested ? api_error::download_stopped : api_error::comm_error;
      utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                         "failed to read file bytes");
      return res;
    }

    if (response_code < 200 || response_code >= 300) {
      utils::error::raise_api_path_error(__FUNCTION__, api_path, response_code,
                                         "failed to read file bytes");
      return api_error::comm_error;
    }

    return api_error::success;
  }

  const auto key = utils::encryption::generate_key(encryption_token);
  return utils::encryption::read_encrypted_range(
      {offset, offset + size - 1}, key,
      [&](data_buffer &ct, std::uint64_t start_offset,
          std::uint64_t end_offset) -> api_error {
        return read_file_bytes(
            api_path, (end_offset - start_offset + 1u), start_offset, ct,
            get_key, get_size, []() -> std::string { return ""; },
            stop_requested);
      },
      data_size, data);
}

void s3_comm::remove_cached_directory(const std::string &api_path) {
  recur_mutex_lock l(cached_directories_mutex_);
  cached_directories_.erase(api_path);
}

auto s3_comm::remove_directory(const std::string &api_path) -> api_error {
  raise_begin(__FUNCTION__, api_path);

  auto object_name = get_object_name(api_path, empty_key) + "/";

  long response_code{};
  if (delete_object_request(*s3_client_, s3_config_, object_name,
                            response_code)) {
    if (response_code == 404) {
      return raise_end(__FUNCTION__, api_path, api_error::directory_not_found,
                       response_code);
    }

    if (response_code != 204) {
      return raise_end(__FUNCTION__, api_path, api_error::comm_error,
                       response_code);
    }

    remove_cached_directory(utils::path::get_parent_api_path(api_path));
    remove_cached_directory(api_path);

    return raise_end(__FUNCTION__, api_path, api_error::success, response_code);
  }

  return raise_end(__FUNCTION__, api_path, api_error::comm_error,
                   response_code);
}

auto s3_comm::remove_file(const std::string &api_path,
                          const get_key_callback &get_key) -> api_error {
  raise_begin(__FUNCTION__, api_path);

  auto object_name = get_object_name(api_path, get_key);

  long response_code{};
  if (delete_object_request(*s3_client_, s3_config_, object_name,
                            response_code)) {
    if (response_code == 404) {
      return raise_end(__FUNCTION__, api_path, api_error::item_not_found,
                       response_code);
    }

    if (response_code != 204) {
      return raise_end(__FUNCTION__, api_path, api_error::comm_error,
                       response_code);
    }

    remove_cached_directory(utils::path::get_parent_api_path(api_path));
    return raise_end(__FUNCTION__, api_path, api_error::success, response_code);
  }

  return raise_end(__FUNCTION__, api_path, api_error::comm_error,
                   response_code);
}

auto s3_comm::rename_file(const std::string & /*api_path*/,
                          const std::string & /*new_api_path*/) -> api_error {
  return api_error::not_implemented;
  /*   if (config_.get_event_level() >= event_level::debug) { */
  /*     event_system::instance().raise<debug_log>(__FUNCTION__, api_path,
   * "begin"); */
  /*   } */
  /*   auto ret = api_error::success; */
  /*  */
  /*   std::string bucket_name, object_name; */
  /*   get_object_name(api_path, bucket_name, object_name); */
  /*  */
  /*   std::string new_object_name; */
  /*   get_object_name(new_api_path, bucket_name, new_object_name); */
  /*  */
  /*   Aws::S3::Model::CopyObjectRequest request{}; */
  /*   request.SetBucket(bucket_name); */
  /*   request.SetCopySource(bucket_name + '/' + object_name); */
  /*   request.SetKey(new_object_name); */
  /*  */
  /*   const auto outcome = s3_client_->CopyObject(request); */
  /*   if (outcome.IsSuccess()) { */
  /*     ret = remove_file(api_path); */
  /*   } else { */
  /*     const auto &error = outcome.GetError(); */
  /*     event_system::instance().raise<repertory_exception>(__FUNCTION__,
   * error.GetExceptionName()
   * +
   * "|" + */
  /*                                                                         error.GetMessage());
   */
  /*     ret = api_error::comm_error; */
  /*   } */
  /*  */
  /*   if (config_.get_event_level() >= event_level::debug) { */
  /*     event_system::instance().raise<debug_log>(__FUNCTION__, api_path, */
  /*                                             "end|" +
   * std::to_string(std::uint8_t(ret))); */
  /*   } */
  /*   return ret; */
}

void s3_comm::set_cached_directory_items(const std::string &api_path,
                                         directory_item_list list) const {
  recur_mutex_lock l(cached_directories_mutex_);
  cached_directories_[api_path].items = std::move(list);
  cached_directories_[api_path].reset_timeout(
      std::chrono::seconds(config_.get_s3_config().cache_timeout_secs));
}

auto s3_comm::upload_file(const std::string &api_path,
                          const std::string &source_path,
                          const std::string &encryption_token,
                          const get_key_callback &get_key,
                          const set_key_callback &set_key,
                          stop_type &stop_requested) -> api_error {
  raise_begin(__FUNCTION__, api_path);

  auto object_name = get_object_name(api_path, get_key);
  long response_code{};
  if (not put_object_request(*s3_client_, s3_config_, object_name, source_path,
                             encryption_token, get_key, set_key, response_code,
                             stop_requested)) {
    return raise_end(__FUNCTION__, api_path,
                     stop_requested ? api_error::upload_stopped
                                    : api_error::upload_failed,
                     response_code);
  }

  if (response_code != 200) {
    return raise_end(__FUNCTION__, api_path, api_error::comm_error,
                     response_code);
  }

  remove_cached_directory(utils::path::get_parent_api_path(api_path));
  return raise_end(__FUNCTION__, api_path, api_error::success, response_code);
}
} // namespace repertory

#endif // REPERTORY_ENABLE_S3
