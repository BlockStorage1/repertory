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
#if defined(REPERTORY_ENABLE_SKYNET)

#include "providers/skynet/skynet_provider.hpp"
#include "comm/i_comm.hpp"
#include "app_config.hpp"
#include "drives/i_open_file_table.hpp"
#include "events/events.hpp"
#include "types/repertory.hpp"
#include "types/skynet.hpp"
#include "types/startup_exception.hpp"
#include "utils/encryption.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/file_utils.hpp"
#include "utils/global_data.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
skynet_provider::skynet_provider(app_config &config, i_comm &comm)
    : base_provider(config),
      comm_(comm),
      directory_db_(config),
      upload_manager_(
          config, [this](const std::string &api_path) -> bool { return this->is_file(api_path); },
          [this](const upload_manager::upload &upload, json &data, json &error) -> api_error {
            return this->upload_handler(upload, data, error);
          },
          [this](const std::string &api_path, const std::string &source_path, const json &data) {
            return this->upload_completed(api_path, source_path, data);
          }) {
  next_download_index_ = 0u;
  next_upload_index_ = 0u;
  update_portal_list();
  E_SUBSCRIBE_EXACT(skynet_portal_list_changed,
                    [this](const skynet_portal_list_changed &) { this->update_portal_list(); });

  // Remove legacy encrypted files
  api_file_list list;
  get_file_list(list);
  for (const auto &file : list) {
    std::string token;
    get_item_meta(file.api_path, "token", token);
    if (not token.empty()) {
      remove_file(file.api_path);
    }
  }
}

api_error skynet_provider::create_directory(const std::string &api_path, const api_meta_map &meta) {
  auto ret = api_error::success;
  if (utils::path::is_trash_directory(api_path)) {
    ret = api_error::access_denied;
  } else {
#ifdef _WIN32
    ret = is_directory(api_path) ? api_error::directory_exists
          : is_file(api_path)    ? api_error::file_exists
                                 : api_error::success;
    if (ret != api_error::success) {
      return ret;
    }
#endif
    if ((ret = directory_db_.create_directory(api_path)) != api_error::success) {
      return ret;
    }

    set_item_meta(api_path, meta);
  }

  return ret;
}

api_error skynet_provider::create_file(const std::string &api_path, api_meta_map &meta) {
  if (meta[META_SIZE].empty()) {
    meta[META_SIZE] = "0";
  }

  // When META_ID is present, an external import is occurring.
  //  Need to skip the encryption token in this scenario.
  if (meta[META_ID].empty() && meta[META_ENCRYPTION_TOKEN].empty()) {
    meta[META_ENCRYPTION_TOKEN] = get_config().get_skynet_config().encryption_token;
  }

  auto ret = base_provider::create_file(api_path, meta);
  if (ret == api_error::success) {
    ret = directory_db_.create_file(api_path);
  }

  return ret;
}

json skynet_provider::export_all() const {
  json ret = {{"success", std::vector<json>()}, {"failed", std::vector<std::string>()}};

  api_file_list list;
  get_file_list(list);
  for (const auto &file : list) {
    process_export(ret, file.api_path);
  }

  return ret;
}

json skynet_provider::export_list(const std::vector<std::string> &api_path_list) const {
  json ret = {{"success", std::vector<json>()}, {"failed", std::vector<std::string>()}};

  for (const auto &api_path : api_path_list) {
    process_export(ret, api_path);
  }

  return ret;
}

std::uint64_t skynet_provider::get_directory_item_count(const std::string &api_path) const {
  return is_directory(api_path) ? directory_db_.get_directory_item_count(api_path) : 0u;
}

api_error skynet_provider::get_directory_items(const std::string &api_path,
                                               directory_item_list &list) const {
  if (is_file(api_path)) {
    return api_error::item_is_file;
  }

  if (not is_directory(api_path)) {
    const_cast<skynet_provider *>(this)->remove_item_meta(api_path);
    return api_error::directory_not_found;
  }

  directory_db_.populate_sub_directories(
      api_path,
      [this](directory_item &di, const bool &) {
        this->get_item_meta(di.api_path, di.meta);
        this->oft_->update_directory_item(di);
      },
      list);

  directory_db_.populate_directory_files(
      api_path,
      [this](directory_item &di, const bool &) {
        di.api_parent = utils::path::get_parent_api_path(di.api_path);
        this->get_item_meta(di.api_path, di.meta);
        this->oft_->update_directory_item(di);
      },
      list);

  std::sort(list.begin(), list.end(), [](const auto &a, const auto &b) -> bool {
    return (a.directory && not b.directory)   ? true
           : (b.directory && not a.directory) ? false
                                              : (a.api_path.compare(b.api_path) < 0);
  });

  list.insert(list.begin(), directory_item{
                                "..",
                                "",
                                true,
                            });
  list.insert(list.begin(), directory_item{
                                ".",
                                "",
                                true,
                            });

  return api_error::success;
}

api_error skynet_provider::get_file(const std::string &api_path, api_file &file) const {
  const auto ret =
      directory_db_.get_file(api_path, file, [this](api_file &file) { populate_api_file(file); });

  if (ret != api_error::success) {
    if (not is_directory(api_path)) {
      const_cast<skynet_provider *>(this)->remove_item_meta(api_path);
    }
    event_system::instance().raise<file_get_failed>(api_path,
                                                    std::to_string(static_cast<int>(ret)));
  }

  return ret;
}

api_error skynet_provider::get_file_list(api_file_list &list) const {
  const auto ret =
      directory_db_.get_file_list(list, [this](api_file &file) { populate_api_file(file); });
  if (ret != api_error::success) {
    event_system::instance().raise<file_get_api_list_failed>(std::to_string(static_cast<int>(ret)));
  }

  return ret;
}

api_error skynet_provider::get_file_size(const std::string &api_path,
                                         std::uint64_t &file_size) const {
  api_file file{};
  const auto ret = get_file(api_path, file);
  if (ret == api_error::success) {
    file_size = file.file_size;
  } else {
    event_system::instance().raise<file_get_size_failed>(api_path,
                                                         std::to_string(static_cast<int>(ret)));
  }
  return ret;
}

host_config skynet_provider::get_host_config(const bool &upload) {
  const auto format_host_config = [&](host_config hc) {
    hc.path = upload ? "/skynet/skyfile" : "/";
    return hc;
  };

  unique_mutex_lock portal_lock(portal_mutex_);
  const auto portal_list = upload ? upload_list_ : download_list_;
  portal_lock.unlock();

  auto &next_upload_index = upload ? next_upload_index_ : next_download_index_;
  auto idx = next_upload_index++;
  if (idx >= portal_list->size()) {
    idx = next_upload_index = 0u;
  }

  return format_host_config((*portal_list)[idx]);
}

std::size_t skynet_provider::get_retry_count() const {
  mutex_lock portal_lock(portal_mutex_);
  return std::max(download_list_->size(),
                  static_cast<std::size_t>(get_config().get_retry_read_count()));
}

api_error skynet_provider::get_skynet_metadata(const std::string &skylink, json &json_meta) {
  auto ret = api_error::error;

  http_headers headers;
  const auto retry_count = get_retry_count();

  for (std::size_t i = 0u; (ret != api_error::success) && (i < retry_count); i++) {
    json data, error;
    const auto hc = get_host_config(false);
    if (comm_.get(hc, "/skynet/metadata/" + skylink, data, error) == api_error::success) {
      headers["skynet-file-metadata"] = data.dump();
      ret = api_error::success;
    } else {
      std::vector<char> buffer;
      if (comm_.get_range_and_headers(hc, utils::path::create_api_path(skylink), 0u,
                                      {{"format", "concat"}}, "", buffer, {{0, 0}}, error, headers,
                                      stop_requested_) == api_error::success) {
        ret = api_error::success;
      } else if (not error.empty()) {
        event_system::instance().raise<repertory_exception>(__FUNCTION__, error.dump(2));
      }
    }
  }

  if (ret == api_error::success) {
    json_meta = json::parse(headers["skynet-file-metadata"]);
    if (json_meta["subfiles"].empty()) {
      auto sub_file = json_meta;
      sub_file["len"] =
          utils::string::to_uint64(utils::string::split(headers["content-range"], '/')[1]);
      json sub_files = {{json_meta["filename"], sub_file}};
      json_meta["subfiles"] = sub_files;
    }
  }

  return ret;
}

api_error skynet_provider::import_skylink(const skylink_import &si) {
  json json_meta;
  auto ret = get_skynet_metadata(si.skylink, json_meta);
  if (ret == api_error::success) {
    const auto encrypted = not si.token.empty();
    for (auto sub_file : json_meta["subfiles"]) {
      const auto meta_file_name = sub_file["filename"].get<std::string>();
      auto file_name = meta_file_name;
      if (encrypted) {
        if ((ret = utils::encryption::decrypt_file_name(si.token, file_name)) !=
            api_error::success) {
          event_system::instance().raise<skynet_import_decryption_failed>(
              si.skylink, sub_file["filename"], ret);
        }
      }

      if (ret == api_error::success) {
        const auto api_path =
            utils::path::create_api_path(utils::path::combine(si.directory, {file_name}));
        const auto api_parent = utils::path::get_parent_api_path(api_path);
        const auto parts = utils::string::split(api_parent, '/', false);

        std::string sub_directory = "/";
        for (std::size_t i = 0u; (ret == api_error::success) && (i < parts.size()); i++) {
          sub_directory =
              utils::path::create_api_path(utils::path::combine(sub_directory, {parts[i]}));
          if (not is_directory(sub_directory)) {
            if ((ret = directory_db_.create_directory(sub_directory)) == api_error::success) {
              base_provider::notify_directory_added(
                  sub_directory, utils::path::get_parent_api_path(sub_directory));
            } else {
              event_system::instance().raise<skynet_import_directory_failed>(si.skylink,
                                                                             sub_directory, ret);
            }
          }
        }

        if (ret == api_error::success) {
          auto file_size = sub_file["len"].get<std::uint64_t>();
          if (encrypted) {
            file_size = utils::encryption::encrypting_reader::calculate_decrypted_size(file_size);
          }

          const auto skylink =
              si.skylink + ((json_meta["filename"].get<std::string>() == meta_file_name)
                                ? ""
                                : "/" + meta_file_name);
          api_meta_map meta{};
          meta[META_ID] = json({{"skylink", skylink}}).dump();
          meta[META_ENCRYPTION_TOKEN] = si.token;
          if ((ret = create_file(api_path, meta)) == api_error::success) {
            const auto now = utils::get_file_time_now();
            api_item_added_(api_path, api_parent, "", false, now, now, now, now);

            if (file_size > 0u) {
              set_item_meta(api_path, META_SIZE, std::to_string(file_size));
              global_data::instance().increment_used_drive_space(file_size);
            }
          } else {
            event_system::instance().raise<skynet_import_file_failed>(si.skylink, api_path, ret);
          }
        }
      }
    }
  }

  return ret;
}

bool skynet_provider::is_directory(const std::string &api_path) const {
  return (api_path == "/") || directory_db_.is_directory(api_path);
}

bool skynet_provider::is_file(const std::string &api_path) const {
  return (api_path != "/") && directory_db_.is_file(api_path);
}

bool skynet_provider::is_file_writeable(const std::string &api_path) const {
  auto ret = true;
  std::string id;
  get_item_meta(api_path, META_ID, id);
  if (not id.empty()) {
    try {
      const auto skynet_data = json::parse(id);
      const auto skylink = skynet_data["skylink"].get<std::string>();
      ret = not utils::string::contains(skylink, "/");
    } catch (const std::exception &e) {
      event_system::instance().raise<repertory_exception>(
          __FUNCTION__, e.what() ? e.what() : "exception occurred");
      ret = false;
    }
  }

  return ret;
}

bool skynet_provider::is_processing(const std::string &api_path) const {
  return upload_manager_.is_processing(api_path);
}

void skynet_provider::notify_directory_added(const std::string &api_path,
                                             const std::string &api_parent) {
  if (api_path == "/") {
    if (directory_db_.create_directory("/") == api_error::success) {
      base_provider::notify_directory_added(api_path, api_parent);
    }
  }
}

api_error skynet_provider::notify_file_added(const std::string &, const std::string &,
                                             const std::uint64_t &) {
  return api_error::not_implemented;
}

void skynet_provider::populate_api_file(api_file &file) const {
  api_meta_map meta{};
  this->get_item_meta(file.api_path, meta);

  file.api_parent = utils::path::get_parent_api_path(file.api_path);
  file.accessed_date = utils::string::to_uint64(meta[META_ACCESSED]);
  file.changed_date = utils::string::to_uint64(meta[META_MODIFIED]);
  file.created_date = utils::string::to_uint64(meta[META_CREATION]);
  file.encryption_token = meta[META_ENCRYPTION_TOKEN].empty() ? "" : meta[META_ENCRYPTION_TOKEN];
  file.file_size = utils::string::to_uint64(meta[META_SIZE]);
  file.modified_date = utils::string::to_uint64(meta[META_MODIFIED]);
  file.recoverable = not is_processing(file.api_path);
  file.redundancy = 3.0;
  file.source_path = meta[META_SOURCE];
}

void skynet_provider::process_export(json &result, const std::string &api_path) const {
  try {
    std::string id;
    std::string token;
    if (is_file(api_path) && (get_item_meta(api_path, META_ID, id) == api_error::success) &&
        (get_item_meta(api_path, META_ENCRYPTION_TOKEN, token) == api_error::success)) {
      auto directory = utils::path::get_parent_api_path(api_path);

      const auto skylink = json::parse(id)["skylink"].get<std::string>();
      if (utils::string::contains(skylink, "/")) {
        const auto pos = skylink.find('/');
        const auto path =
            utils::path::create_api_path(utils::path::remove_file_name(skylink.substr(pos)));
        if (path != "/") {
          directory =
              utils::path::create_api_path(directory.substr(0, directory.length() - path.length()));
        }
      }

      result["success"].emplace_back(
          json({{"skylink", skylink},
                {"token", token},
                {"directory", directory},
                {"filename", utils::path::strip_to_file_name(api_path)}}));
    } else {
      result["failed"].emplace_back(api_path);
    }
  } catch (const std::exception &e) {
    result["failed"].emplace_back(api_path);
    event_system::instance().raise<repertory_exception>(
        __FUNCTION__, e.what() ? e.what() : "export failed: " + api_path);
  }
}

api_error skynet_provider::read_file_bytes(const std::string &api_path, const std::size_t &size,
                                           const std::uint64_t &offset, std::vector<char> &data,
                                           const bool &stop_requested) {
  if (size == 0u) {
    return api_error::success;
  }

  std::string id;
  auto ret = get_item_meta(api_path, META_ID, id);
  if (ret == api_error::success) {
    ret = api_error::download_failed;

    const auto skynet_data = json::parse(id);
    const auto path = utils::path::create_api_path(skynet_data["skylink"].get<std::string>());
    const auto ranges = http_ranges({{offset, offset + size - 1}});

    std::string encryption_token;
    get_item_meta(api_path, META_ENCRYPTION_TOKEN, encryption_token);

    std::uint64_t file_size{};
    {
      std::string temp;
      get_item_meta(api_path, META_SIZE, temp);
      file_size = utils::string::to_uint64(temp);
    }

    const auto retry_count = get_retry_count();
    for (std::size_t i = 0u; not stop_requested && (ret != api_error::success) && (i < retry_count);
         i++) {
      json error;
      ret = (comm_.get_range(get_host_config(false), path, file_size, {{"format", "concat"}},
                             encryption_token, data, ranges, error,
                             stop_requested) == api_error::success)
                ? api_error::success
                : api_error::download_failed;
      if (ret != api_error::success) {
        event_system::instance().raise<file_read_bytes_failed>(api_path, error.dump(2), i + 1);
      }
    }
  }

  return ret;
}

api_error skynet_provider::remove_directory(const std::string &api_path) {
  const auto ret = directory_db_.remove_directory(api_path);
  if (ret == api_error::success) {
    remove_item_meta(api_path);
    event_system::instance().raise<directory_removed>(api_path);
  } else {
    event_system::instance().raise<directory_remove_failed>(api_path,
                                                            std::to_string(static_cast<int>(ret)));
  }

  return ret;
}

api_error skynet_provider::remove_file(const std::string &api_path) {
  upload_manager_.remove_upload(api_path);
  const auto ret =
      directory_db_.remove_file(api_path) ? api_error::success : api_error::item_not_found;
  if (ret == api_error::success) {
    remove_item_meta(api_path);
    event_system::instance().raise<file_removed>(api_path);
  } else {
    event_system::instance().raise<file_remove_failed>(api_path,
                                                       std::to_string(static_cast<int>(ret)));
  }

  return ret;
}

api_error skynet_provider::rename_file(const std::string &from_api_path,
                                       const std::string &to_api_path) {
  std::string id;
  auto ret = get_item_meta(from_api_path, META_ID, id);
  if (ret == api_error::success) {
    ret = api_error::access_denied;

    const auto skynet_data = json::parse(id.empty() ? R"({"skylink":""})" : id);
    const auto skylink = skynet_data["skylink"].get<std::string>();
    if (utils::string::contains(skylink, "/")) {
      const auto pos = skylink.find('/');
      const auto len = skylink.size() - pos;
      if (to_api_path.size() >= len) {
        const auto comp1 = to_api_path.substr(to_api_path.size() - len);
        const auto comp2 = skylink.substr(pos);
        ret = (comp1 == comp2) ? api_error::success : ret;
      }
    } else {
      ret = (skylink.empty() || (utils::path::strip_to_file_name(from_api_path) ==
                                 utils::path::strip_to_file_name(to_api_path)))
                ? api_error::success
                : ret;
    }

    if (ret == api_error::success) {
      std::string current_source;
      if ((ret = get_item_meta(from_api_path, META_SOURCE, current_source)) == api_error::success) {
        if (not upload_manager_.execute_if_not_processing({from_api_path, to_api_path}, [&]() {
              if ((ret = directory_db_.rename_file(from_api_path, to_api_path)) ==
                  api_error::success) {
                meta_db_.rename_item_meta(current_source, from_api_path, to_api_path);
              }
            })) {
          ret = api_error::file_in_use;
        }
      }
    }
  }

  return ret;
}

bool skynet_provider::start(api_item_added_callback api_item_added, i_open_file_table *oft) {
  const auto ret = base_provider::start(api_item_added, oft);
  if (not ret) {
    api_file_list list;
    if (get_file_list(list) != api_error::success) {
      throw startup_exception("failed to determine used space");
    }

    const auto total_size =
        std::accumulate(list.begin(), list.end(), std::uint64_t(0),
                        [](std::uint64_t t, const api_file &file) { return t + file.file_size; });
    global_data::instance().initialize_used_drive_space(total_size);
    upload_manager_.start();
  }
  return ret;
}

void skynet_provider::stop() {
  stop_requested_ = true;
  upload_manager_.stop();
}

bool skynet_provider::update_portal_list() {
  auto portal_list = get_config().get_skynet_config().portal_list;
  auto download_portal_list = std::make_shared<std::vector<host_config>>();
  auto upload_portal_list = std::make_shared<std::vector<host_config>>();

  std::copy_if(portal_list.begin(), portal_list.end(), std::back_inserter(*upload_portal_list),
               [](const auto &portal) -> bool {
                 return not portal.auth_url.empty() && not portal.auth_user.empty();
               });
  for (const auto &portal : portal_list) {
    if (upload_portal_list->empty()) {
      upload_portal_list->emplace_back(portal);
    }
    download_portal_list->emplace_back(portal);
  }

  unique_mutex_lock portal_lock(portal_mutex_);
  download_list_ = std::move(download_portal_list);
  upload_list_ = std::move(upload_portal_list);
  portal_lock.unlock();

  return true;
}

void skynet_provider::upload_completed(const std::string &api_path, const std::string &,
                                       const json &data) {
  set_item_meta(api_path, META_ID, data.dump());
}

api_error skynet_provider::upload_file(const std::string &api_path, const std::string &source_path,
                                       const std::string &encryption_token) {
  std::uint64_t file_size = 0u;
  utils::file::get_file_size(source_path, file_size);
  auto ret = set_source_path(api_path, source_path);
  if (ret == api_error::success) {
    if (((ret = set_item_meta(api_path, META_SIZE, std::to_string(file_size))) ==
         api_error::success) &&
        ((ret = set_item_meta(api_path, META_ENCRYPTION_TOKEN, encryption_token)) ==
         api_error::success) &&
        (file_size != 0u)) {
      ret = upload_manager_.queue_upload(api_path, source_path, encryption_token);
    }
  }

  return ret;
}

api_error skynet_provider::upload_handler(const upload_manager::upload &upload, json &data,
                                          json &error) {
  const auto file_name = utils::path::strip_to_file_name(upload.api_path);

  auto ret = api_error::upload_failed;
  if (not upload.cancel) {
    event_system::instance().raise<file_upload_begin>(upload.api_path, upload.source_path);
    ret = (comm_.post_multipart_file(get_host_config(true), "", file_name, upload.source_path,
                                     upload.encryption_token, data, error,
                                     upload.cancel) == api_error::success)
              ? api_error::success
              : api_error::upload_failed;
  }

  event_system::instance().raise<file_upload_end>(upload.api_path, upload.source_path, ret);
  return ret;
}
} // namespace repertory

#endif // defined(REPERTORY_ENABLE_SKYNET)
