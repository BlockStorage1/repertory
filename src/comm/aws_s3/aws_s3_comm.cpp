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
#if defined(REPERTORY_ENABLE_S3)
#include "comm/aws_s3/aws_s3_comm.hpp"
#include "app_config.hpp"
#include "events/events.hpp"
#include "events/event_system.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "utils/encryption.hpp"
#include "utils/path_utils.hpp"
#include "utils/polling.hpp"
#include "utils/string_utils.hpp"

namespace repertory {
static const i_s3_comm::get_key_callback empty_key = []() { return ""; };

aws_s3_comm::aws_s3_comm(const app_config &config)
    : config_(config), s3_config_(config.get_s3_config()) {
  s3_config_.bucket = utils::string::trim(s3_config_.bucket);
  Aws::InitAPI(sdk_options_);

  if (config_.get_event_level() >= event_level::debug) {
    Aws::Utils::Logging::InitializeAWSLogging(
        Aws::MakeShared<Aws::Utils::Logging::DefaultLogSystem>(
            "Repertory", Aws::Utils::Logging::LogLevel::Trace,
            utils::path::combine(config_.get_log_directory(), {"aws_sdk_"})));
  }

  Aws::Auth::AWSCredentials credentials;
  credentials.SetAWSAccessKeyId(s3_config_.access_key);
  credentials.SetAWSSecretKey(s3_config_.secret_key);

  Aws::Client::ClientConfiguration configuration;
  configuration.endpointOverride = s3_config_.url;
  configuration.httpRequestTimeoutMs = s3_config_.timeout_ms;
  configuration.region = s3_config_.region;
  configuration.requestTimeoutMs = s3_config_.timeout_ms;

  configuration.connectTimeoutMs = s3_config_.timeout_ms;
  // TODO make configurable
  const auto enable_path_style = utils::string::begins_with(s3_config_.url, "http://localhost") ||
                                 utils::string::begins_with(s3_config_.url, "https://localhost") ||
                                 utils::string::begins_with(s3_config_.url, "http://127.0.0.1") ||
                                 utils::string::begins_with(s3_config_.url, "https://127.0.0.1");

  s3_client_ = std::make_unique<Aws::S3::S3Client>(
      credentials, configuration, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never,
      not enable_path_style);
  polling::instance().set_callback(
      {"s3_directory_cache", false, [this]() { this->clear_expired_directories(); }});
}

aws_s3_comm::~aws_s3_comm() {
  polling::instance().remove_callback("s3_directory_cache");
  Aws::ShutdownAPI(sdk_options_);
  Aws::Utils::Logging::ShutdownAWSLogging();
  s3_client_.reset();
}

void aws_s3_comm::clear_expired_directories() {
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

api_error aws_s3_comm::create_bucket(const std::string &api_path) {
  std::string bucket_name, object_name;
  get_bucket_name_and_object_name(api_path, empty_key, bucket_name, object_name);

  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, bucket_name, "begin");
  }

  auto ret = api_error::access_denied;
  if (s3_config_.bucket.empty()) {
    Aws::S3::Model::CreateBucketRequest request{};
    request.SetBucket(bucket_name);
    const auto outcome = s3_client_->CreateBucket(request);
    if (outcome.IsSuccess()) {
      remove_cached_directory(utils::path::get_parent_api_path(api_path));
      ret = api_error::success;
    } else {
      const auto &error = outcome.GetError();
      event_system::instance().raise<repertory_exception>(
          __FUNCTION__, error.GetExceptionName() + "|" + error.GetMessage());
      ret = api_error::comm_error;
    }
  }

  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, bucket_name,
                                              "end|" + std::to_string(static_cast<int>(ret)));
  }
  return ret;
}

bool aws_s3_comm::exists(const std::string &api_path, const get_key_callback &get_key) const {
  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path, "begin");
  }
  if (api_path == "/") {
    return false;
  }

  auto ret = false;
  if (not get_cached_file_exists(api_path, ret)) {
    std::string bucket_name, object_name;
    get_bucket_name_and_object_name(api_path, get_key, bucket_name, object_name);

    Aws::S3::Model::HeadObjectRequest request{};
    request.SetBucket(bucket_name);
    request.SetKey(object_name);
    ret = s3_client_->HeadObject(request).IsSuccess();
  }

  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path, "end|" + std::to_string(ret));
  }
  return ret;
}

void aws_s3_comm::get_bucket_name_and_object_name(const std::string &api_path,
                                                  const get_key_callback &get_key,
                                                  std::string &bucket_name,
                                                  std::string &object_name) const {
  bucket_name = s3_config_.bucket;
  object_name = api_path.substr(1);

  if (bucket_name.empty()) {
    bucket_name = utils::string::split(api_path, '/')[1];
    object_name = object_name.substr(bucket_name.size());
    if (not object_name.empty() && (object_name[0] == '/')) {
      object_name = object_name.substr(1);
    }
  }

  const auto key = get_key();
  if (not key.empty()) {
    auto parts = utils::string::split(object_name, '/', false);
    parts[parts.size() - 1u] = key;
    object_name = utils::string::join(parts, '/');
  }
}

bool aws_s3_comm::get_cached_directory_item_count(const std::string &api_path,
                                                  std::size_t &count) const {
  recur_mutex_lock l(cached_directories_mutex_);
  if (cached_directories_.find(api_path) != cached_directories_.end()) {
    count = cached_directories_.at(api_path).items.size();
    return true;
  }

  return false;
}

bool aws_s3_comm::get_cached_directory_items(const std::string &api_path,
                                             const meta_provider_callback &meta_provider,
                                             directory_item_list &list) const {
  unique_recur_mutex_lock l(cached_directories_mutex_);
  if (cached_directories_.find(api_path) != cached_directories_.end()) {
    auto &cachedEntry = cached_directories_.at(api_path);
    list = cachedEntry.items;
    cached_directories_[api_path].reset_timeout(
        std::chrono::seconds(config_.get_s3_config().cache_timeout_secs));
    l.unlock();

    for (auto &item : list) {
      meta_provider(item, false);
    }
    return true;
  }
  return false;
}

bool aws_s3_comm::get_cached_file_exists(const std::string &api_path, bool &exists) const {
  exists = false;

  unique_recur_mutex_lock l(cached_directories_mutex_);
  const auto parent_api_path = utils::path::get_parent_api_path(api_path);
  if (cached_directories_.find(parent_api_path) != cached_directories_.end()) {
    auto &entry = cached_directories_.at(parent_api_path);
    exists =
        std::find_if(entry.items.begin(), entry.items.end(), [&api_path](const auto &item) -> bool {
          return not item.directory && (api_path == item.api_path);
        }) != entry.items.end();
    if (exists) {
      cached_directories_[api_path].reset_timeout(
          std::chrono::seconds(config_.get_s3_config().cache_timeout_secs));
      return true;
    }
  }

  return false;
}

std::size_t
aws_s3_comm::get_directory_item_count(const std::string &api_path,
                                      const meta_provider_callback &meta_provider) const {
  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path, "begin");
  }

  std::size_t ret = 0u;
  if (not(s3_config_.bucket.empty() && (api_path == "/"))) {
    if (not get_cached_directory_item_count(api_path, ret)) {
      directory_item_list list;
      grab_directory_items(api_path, meta_provider, list);
      return list.size();
    }
  }

  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path, "end|" + std::to_string(ret));
  }
  return ret;
}

api_error aws_s3_comm::get_directory_items(const std::string &api_path,
                                           const meta_provider_callback &meta_provider,
                                           directory_item_list &list) const {
  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path, "begin");
  }

  auto ret = api_error::success;
  if (not get_cached_directory_items(api_path, meta_provider, list)) {
    ret = grab_directory_items(api_path, meta_provider, list);
  }

  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path,
                                              "end|" + std::to_string(std::uint8_t(ret)));
  }
  return ret;
}

api_error aws_s3_comm::get_file(const std::string &api_path, const get_key_callback &get_key,
                                const get_name_callback &get_name,
                                const get_token_callback &get_token, api_file &file) const {
  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path, "begin");
  }
  auto ret = api_error::success;

  std::string bucket_name, object_name;
  get_bucket_name_and_object_name(api_path, get_key, bucket_name, object_name);

  Aws::S3::Model::HeadObjectRequest request{};
  request.SetBucket(bucket_name);
  request.SetKey(object_name);

  const auto outcome = s3_client_->HeadObject(request);
  if (outcome.IsSuccess()) {
    const auto key = get_key();
    auto object = outcome.GetResult();
    object_name = get_name(key, object_name);
    file.accessed_date = utils::get_file_time_now();
    file.api_path = utils::path::create_api_path(utils::path::combine(bucket_name, {object_name}));
    file.api_parent = utils::path::get_parent_api_path(file.api_path);
    file.changed_date = object.GetLastModified().Millis() * 1000u * 1000u;
    file.created_date = object.GetLastModified().Millis() * 1000u * 1000u;
    file.encryption_token = get_token();
    if (file.encryption_token.empty()) {
      file.file_size = object.GetContentLength();
    } else {
      file.file_size =
          utils::encryption::encrypting_reader::calculate_decrypted_size(object.GetContentLength());
    }
    file.modified_date = object.GetLastModified().Millis() * 1000u * 1000u;
    file.recoverable = true;
    file.redundancy = 3.0;
  } else {
    const auto &error = outcome.GetError();
    event_system::instance().raise<repertory_exception>(__FUNCTION__, error.GetExceptionName() +
                                                                          "|" + error.GetMessage());
    ret = api_error::comm_error;
  }

  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path,
                                              "end|" + std::to_string(std::uint8_t(ret)));
  }
  return ret;
}

api_error aws_s3_comm::get_file_list(const get_api_file_token_callback &get_api_file_token,
                                     const get_name_callback &get_name, api_file_list &list) const {
  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, "/", "begin");
  }
  list.clear();
  auto ret = api_error::success;

  const auto bucket_name = s3_config_.bucket;
  if (bucket_name.empty()) {
    const auto outcome = s3_client_->ListBuckets();
    if (outcome.IsSuccess()) {
      const auto &bucket_list = outcome.GetResult().GetBuckets();
      for (const auto &bucket : bucket_list) {
        get_file_list(bucket.GetName(), get_api_file_token, get_name, list);
      }
    } else {
      const auto &error = outcome.GetError();
      event_system::instance().raise<repertory_exception>(
          __FUNCTION__, error.GetExceptionName() + "|" + error.GetMessage());
      ret = api_error::comm_error;
    }
  } else {
    ret = get_file_list("", get_api_file_token, get_name, list);
  }

  return ret;
}

api_error aws_s3_comm::get_file_list(const std::string &bucket_name,
                                     const get_api_file_token_callback &get_api_file_token,
                                     const get_name_callback &get_name, api_file_list &list) const {
  auto ret = api_error::success;

  Aws::S3::Model::ListObjectsRequest request{};
  request.SetBucket(bucket_name.empty() ? s3_config_.bucket : bucket_name);

  const auto outcome = s3_client_->ListObjects(request);
  if (outcome.IsSuccess()) {
    const auto &object_list = outcome.GetResult().GetContents();
    for (auto const &object : object_list) {
      api_file file{};
      file.accessed_date = utils::get_file_time_now();

      std::string object_name = object.GetKey();
      object_name =
          get_name(*(utils::string::split(object_name, '/', false).end() - 1u), object_name);
      file.api_path = utils::path::create_api_path(utils::path::combine(
          bucket_name.empty() ? s3_config_.bucket : bucket_name, {object_name}));
      file.api_parent = utils::path::get_parent_api_path(file.api_path);
      file.changed_date = object.GetLastModified().Millis() * 1000u * 1000u;
      file.created_date = object.GetLastModified().Millis() * 1000u * 1000u;
      file.encryption_token = get_api_file_token(file.api_path);
      if (file.encryption_token.empty()) {
        file.file_size = object.GetSize();
      } else {
        file.file_size =
            object.GetSize() -
            (utils::divide_with_ceiling(
                 static_cast<std::uint64_t>(object.GetSize()),
                 static_cast<std::uint64_t>(
                     utils::encryption::encrypting_reader::get_encrypted_chunk_size())) *
             utils::encryption::encrypting_reader::get_header_size());
      }
      file.modified_date = object.GetLastModified().Millis() * 1000u * 1000u;
      file.recoverable = true;
      file.redundancy = 3.0;
      list.emplace_back(std::move(file));
    }
  } else {
    const auto &error = outcome.GetError();
    event_system::instance().raise<repertory_exception>(__FUNCTION__, error.GetExceptionName() +
                                                                          "|" + error.GetMessage());
    ret = api_error::comm_error;
  }

  return ret;
}

api_error aws_s3_comm::grab_directory_items(const std::string &api_path,
                                            const meta_provider_callback &meta_provider,
                                            directory_item_list &list) const {
  auto ret = api_error::success;
  if (s3_config_.bucket.empty() && (api_path == "/")) {
    const auto outcome = s3_client_->ListBuckets();
    if (outcome.IsSuccess()) {
      const auto &bucket_list = outcome.GetResult().GetBuckets();
      for (const auto &bucket : bucket_list) {
        directory_item di{};
        di.api_path = utils::path::create_api_path(utils::path::combine("/", {bucket.GetName()}));
        di.api_parent = utils::path::get_parent_api_path(di.api_path);
        di.directory = true;
        di.size = get_directory_item_count(di.api_path, meta_provider);
        meta_provider(di, true);
        list.emplace_back(std::move(di));
      }

      set_cached_directory_items(api_path, list);
    } else {
      const auto &error = outcome.GetError();
      event_system::instance().raise<repertory_exception>(
          __FUNCTION__, error.GetExceptionName() + "|" + error.GetMessage());
      ret = api_error::comm_error;
    }
  } else {
    std::string bucket_name, object_name;
    get_bucket_name_and_object_name(api_path, empty_key, bucket_name, object_name);

    Aws::S3::Model::ListObjectsRequest request{};
    request.SetBucket(bucket_name);
    request.SetDelimiter("/");
    request.SetPrefix(object_name.empty() ? object_name : object_name + "/");

    const auto outcome = s3_client_->ListObjects(request);
    if (outcome.IsSuccess()) {
      const auto &object_list = outcome.GetResult().GetContents();
      for (auto const &object : object_list) {
        directory_item item{};
        item.api_path =
            utils::path::create_api_path(utils::path::combine(bucket_name, {object.GetKey()}));
        item.api_parent = utils::path::get_parent_api_path(item.api_path);
        item.directory = false;
        item.size = object.GetSize();
        meta_provider(item, true);
        list.emplace_back(std::move(item));
      }

      set_cached_directory_items(api_path, list);
    } else {
      const auto &error = outcome.GetError();
      event_system::instance().raise<repertory_exception>(
          __FUNCTION__, error.GetExceptionName() + "|" + error.GetMessage());
      ret = api_error::comm_error;
    }
  }

  return ret;
}

api_error aws_s3_comm::read_file_bytes(const std::string &api_path, const std::size_t &size,
                                       const std::uint64_t &offset, std::vector<char> &data,
                                       const get_key_callback &get_key,
                                       const get_size_callback &get_size,
                                       const get_token_callback &get_token,
                                       const bool &stop_requested) const {
  auto ret = api_error::success;
  data.clear();

  std::string bucket_name, object_name;
  get_bucket_name_and_object_name(api_path, get_key, bucket_name, object_name);

  const auto encryption_token = get_token();
  const auto data_size = get_size();
  if (encryption_token.empty()) {
    Aws::S3::Model::GetObjectRequest request{};
    request.SetBucket(bucket_name);
    request.SetKey(object_name);
    request.SetResponseContentType("application/octet-stream");
    request.SetRange("bytes=" + utils::string::from_uint64(offset) + "-" +
                     utils::string::from_uint64(offset + size - 1u));
    request.SetContinueRequestHandler(
        [&stop_requested](const Aws::Http::HttpRequest *) { return not stop_requested; });

    auto outcome = s3_client_->GetObject(request);
    if (outcome.IsSuccess()) {
      auto result = outcome.GetResultWithOwnership();
      const auto len = result.GetContentLength();
      data.resize(len);
      result.GetBody().read(&data[0], len);
    } else {
      const auto &error = outcome.GetError();
      event_system::instance().raise<repertory_exception>(
          __FUNCTION__, error.GetExceptionName() + "|" + error.GetMessage());
      ret = api_error::comm_error;
    }
  } else {
    const auto key = utils::encryption::generate_key(encryption_token);
    ret = utils::encryption::read_encrypted_range(
        {offset, offset + size - 1}, key,
        [&](std::vector<char> &ct, const std::uint64_t &start_offset,
            const std::uint64_t &end_offset) -> api_error {
          return read_file_bytes(
              api_path, (end_offset - start_offset + 1u), start_offset, ct, get_key, get_size,
              []() -> std::string { return ""; }, stop_requested);
        },
        data_size, data);
  }

  return ret;
}

api_error aws_s3_comm::remove_bucket(const std::string &api_path) {
  std::string bucket_name, object_name;
  get_bucket_name_and_object_name(api_path, empty_key, bucket_name, object_name);

  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, bucket_name, "begin");
  }

  auto ret = api_error::access_denied;
  if (s3_config_.bucket.empty()) {
    Aws::S3::Model::DeleteBucketRequest request{};
    request.SetBucket(bucket_name);
    const auto outcome = s3_client_->DeleteBucket(request);
    if (outcome.IsSuccess()) {
      remove_cached_directory(api_path);
      ret = api_error::success;
    } else {
      const auto &error = outcome.GetError();
      event_system::instance().raise<repertory_exception>(
          __FUNCTION__, error.GetExceptionName() + "|" + error.GetMessage());
      ret = api_error::comm_error;
    }
  }

  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, bucket_name,
                                              "end|" + std::to_string(static_cast<int>(ret)));
  }
  return ret;
}

void aws_s3_comm::remove_cached_directory(const std::string &api_path) {
  recur_mutex_lock l(cached_directories_mutex_);
  cached_directories_.erase(api_path);
}

api_error aws_s3_comm::remove_file(const std::string &api_path, const get_key_callback &get_key) {
  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path, "begin");
  }
  auto ret = api_error::success;

  std::string bucket_name, object_name;
  get_bucket_name_and_object_name(api_path, get_key, bucket_name, object_name);

  Aws::S3::Model::DeleteObjectRequest request{};
  request.SetBucket(bucket_name);
  request.SetKey(object_name);
  const auto outcome = s3_client_->DeleteObject(request);
  if (outcome.IsSuccess()) {
    remove_cached_directory(utils::path::get_parent_api_path(api_path));
  } else {
    const auto &error = outcome.GetError();
    event_system::instance().raise<repertory_exception>(__FUNCTION__, error.GetExceptionName() +
                                                                          "|" + error.GetMessage());
    ret = api_error::comm_error;
  }

  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path,
                                              "end|" + std::to_string(std::uint8_t(ret)));
  }
  return ret;
}

api_error aws_s3_comm::rename_file(const std::string & /*api_path*/,
                                   const std::string & /*new_api_path*/) {
  return api_error::not_implemented;
  /*   if (config_.get_event_level() >= event_level::debug) { */
  /*     event_system::instance().raise<debug_log>(__FUNCTION__, api_path, "begin"); */
  /*   } */
  /*   auto ret = api_error::success; */
  /*  */
  /*   std::string bucket_name, object_name; */
  /*   get_bucket_name_and_object_name(api_path, bucket_name, object_name); */
  /*  */
  /*   std::string new_object_name; */
  /*   get_bucket_name_and_object_name(new_api_path, bucket_name, new_object_name); */
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
  /*     event_system::instance().raise<repertory_exception>(__FUNCTION__, error.GetExceptionName()
   * +
   * "|" + */
  /*                                                                         error.GetMessage()); */
  /*     ret = api_error::comm_error; */
  /*   } */
  /*  */
  /*   if (config_.get_event_level() >= event_level::debug) { */
  /*     event_system::instance().raise<debug_log>(__FUNCTION__, api_path, */
  /*                                             "end|" + std::to_string(std::uint8_t(ret))); */
  /*   } */
  /*   return ret; */
}

void aws_s3_comm::set_cached_directory_items(const std::string &api_path,
                                             directory_item_list list) const {
  recur_mutex_lock l(cached_directories_mutex_);
  cached_directories_[api_path].items = std::move(list);
  cached_directories_[api_path].reset_timeout(
      std::chrono::seconds(config_.get_s3_config().cache_timeout_secs));
}

api_error aws_s3_comm::upload_file(const std::string &api_path, const std::string &source_path,
                                   const std::string &encryption_token,
                                   const get_key_callback &get_key, const set_key_callback &set_key,
                                   const bool &stop_requested) {
  static const auto no_stop = false;
  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path, "begin");
  }
  auto ret = api_error::success;

  std::string bucket_name, object_name;
  get_bucket_name_and_object_name(api_path, get_key, bucket_name, object_name);

  std::shared_ptr<Aws::IOStream> file_stream;
  if (encryption_token.empty()) {
    file_stream = Aws::MakeShared<Aws::FStream>(&source_path[0], &source_path[0],
                                                std::ios_base::in | std::ios_base::binary);
  } else {
    const auto file_name = ([&api_path]() -> std::string {
      return *(utils::string::split(api_path, '/', false).end() - 1u);
    })();

    const auto reader =
        utils::encryption::encrypting_reader(file_name, source_path, no_stop, encryption_token, -1);
    auto key = get_key();
    if (key.empty()) {
      key = reader.get_encrypted_file_name();
      set_key(key);

      auto parts = utils::string::split(object_name, '/', false);
      parts[parts.size() - 1u] = key;
      object_name = utils::string::join(parts, '/');
    }

    file_stream = reader.create_iostream();
  }
  file_stream->seekg(0);

  Aws::S3::Model::PutObjectRequest request{};
  request.SetBucket(bucket_name);
  request.SetKey(object_name);
  request.SetBody(file_stream);
  request.SetContinueRequestHandler(
      [&stop_requested](const Aws::Http::HttpRequest *) { return not stop_requested; });

  const auto outcome = s3_client_->PutObject(request);
  if (outcome.IsSuccess()) {
    remove_cached_directory(utils::path::get_parent_api_path(api_path));
  } else {
    const auto &error = outcome.GetError();
    event_system::instance().raise<repertory_exception>(__FUNCTION__, error.GetExceptionName() +
                                                                          "|" + error.GetMessage());
    ret = api_error::upload_failed;
  }

  if (config_.get_event_level() >= event_level::debug) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path,
                                              "end|" + std::to_string(std::uint8_t(ret)));
  }
  return ret;
}
} // namespace repertory

#endif // REPERTORY_ENABLE_S3
