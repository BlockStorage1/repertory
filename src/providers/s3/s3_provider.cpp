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
#include "providers/s3/s3_provider.hpp"
#include "comm/i_s3_comm.hpp"
#include "app_config.hpp"
#include "db/meta_db.hpp"
#include "drives/i_open_file_table.hpp"
#include "types/repertory.hpp"
#include "types/startup_exception.hpp"
#include "utils/encryption.hpp"
#include "utils/file_utils.hpp"
#include "utils/global_data.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
s3_provider::s3_provider(app_config &config, i_s3_comm &s3_comm)
    : base_provider(config),
      s3_comm_(s3_comm),
      directory_db_(config),
      upload_manager_(
          config, [this](const std::string &api_path) -> bool { return this->is_file(api_path); },
          [this](const upload_manager::upload &upload, json &data, json &error) -> api_error {
            return this->upload_handler(upload, data, error);
          },
          [this](const std::string &api_path, const std::string &source_path, const json &data) {
            return this->upload_completed(api_path, source_path, data);
          }) {}

void s3_provider::build_directories() {
  api_file_list list{};
  s3_comm_.get_file_list([](const std::string & /*api_path*/) -> std::string { return ""; },
                         [](const std::string & /*key*/,
                            const std::string &object_name) -> std::string { return object_name; },
                         list);
  total_item_count_ = list.size();

  std::unordered_map<std::string, std::uint8_t> directories;
  for (const auto &file : list) {
    if (directories.find(file.api_parent) == directories.end()) {
      directories[file.api_parent] = 0u;

      const auto directory_parts = utils::string::split(file.api_parent, '/', false);

      std::string current_directory;
      for (std::size_t i = 0u; i < (directory_parts.size() - 1u); i++) {
        current_directory = current_directory.empty()
                                ? utils::path::create_api_path(directory_parts[0u])
                                : utils::path::combine(current_directory, {directory_parts[i]});
        directories[current_directory] = 0u;
      }
    }
  }
  list.clear();

  for (const auto &kv : directories) {
    if (not directory_db_.is_directory(kv.first)) {
      auto api_path = kv.first;
      restore_api_path(api_path);
      this->notify_directory_added(api_path, utils::path::get_parent_api_path(api_path));
    }
  }
}

api_error s3_provider::create_directory(const std::string &api_path, const api_meta_map &meta) {
#ifdef _WIN32
  auto ret = is_directory(api_path) ? api_error::directory_exists
             : is_file(api_path)    ? api_error::file_exists
                                    : api_error::success;
  if (ret != api_error::success) {
    return ret;
  }
#else
  auto ret = api_error::success;
#endif
  if (utils::path::is_trash_directory(api_path)) {
    ret = api_error::access_denied;
  } else {
    if ((utils::path::get_parent_api_path(api_path) == "/") &&
        s3_comm_.get_s3_config().bucket.empty()) {
      ret = s3_comm_.create_bucket(api_path);
    }

    if (ret == api_error::success) {
      ret = directory_db_.create_directory(format_api_path(api_path));
    }
  }

  if (ret == api_error::success) {
    set_item_meta(api_path, meta);
  }

  return ret;
}

api_error s3_provider::create_file(const std::string &api_path, api_meta_map &meta) {
  if (meta[META_SIZE].empty()) {
    meta[META_SIZE] = "0";
  }

  if (meta[META_ENCRYPTION_TOKEN].empty()) {
    meta[META_ENCRYPTION_TOKEN] = s3_comm_.get_s3_config().encryption_token;
  }

  return base_provider::create_file(api_path, meta);
}

std::string s3_provider::format_api_path(std::string api_path) const {
  return s3_comm_.get_s3_config().bucket.empty()
             ? api_path
             : utils::path::create_api_path(
                   utils::path::combine(s3_comm_.get_s3_config().bucket, {api_path}));
}

std::uint64_t s3_provider::get_directory_item_count(const std::string &api_path) const {
  return s3_comm_.get_directory_item_count(api_path, [this](directory_item &di,
                                                            const bool &format_path) {
    this->update_item_meta(di, format_path);
  }) + directory_db_.get_sub_directory_count(format_api_path(api_path));
}

api_error s3_provider::get_directory_items(const std::string &api_path,
                                           directory_item_list &list) const {
  const auto res = s3_comm_.get_directory_items(
      api_path,
      [this](directory_item &di, const bool &format_path) {
        this->update_item_meta(di, format_path);
      },
      list);

  directory_db_.populate_sub_directories(
      format_api_path(api_path),
      [this](directory_item &di, const bool &format_path) {
        if (format_path) {
          restore_api_path(di.api_path);
          di.api_parent = utils::path::get_parent_api_path(di.api_path);
        }
        this->get_item_meta(di.api_path, di.meta);
        this->oft_->update_directory_item(di);
      },
      list);

  std::sort(list.begin(), list.end(), [](const auto &a, const auto &b) -> bool {
    return (a.directory && not b.directory) ||
           (not(b.directory && not a.directory) && (a.api_path.compare(b.api_path) < 0));
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

  return ((res == api_error::success) ? api_error::success : api_error::error);
}

api_error s3_provider::get_file(const std::string &api_path, api_file &file) const {
  auto real_api_path = format_api_path(api_path);
  const auto ret = s3_comm_.get_file(
      api_path,
      [this, &api_path, &real_api_path]() -> std::string {
        auto key = *(utils::string::split(api_path, '/', false).end() - 1u);
        std::string meta_api_path;
        meta_db_.get_api_path_from_key(key, meta_api_path);
        if (meta_api_path.empty()) {
          meta_db_.get_item_meta(real_api_path, META_KEY, key);
        }
        return key;
      },
      [this, &real_api_path](const std::string &key,
                             const std::string &object_name) -> std::string {
        if (key.empty()) {
          return object_name;
        }

        meta_db_.get_api_path_from_key(key, real_api_path);

        auto object_parts = utils::string::split(object_name, '/', false);
        object_parts[object_parts.size() - 1u] =
            *(utils::string::split(real_api_path, '/', false).end() - 1u);
        return utils::string::join(object_parts, '/');
      },
      [this, &real_api_path]() -> std::string {
        std::string encryption_token;
        meta_db_.get_item_meta(real_api_path, META_ENCRYPTION_TOKEN, encryption_token);
        return encryption_token;
      },
      file);
  if (ret == api_error::success) {
    restore_api_path(file.api_path);
    file.api_parent = utils::path::get_parent_api_path(file.api_path);
  } else {
    event_system::instance().raise<file_get_failed>(api_path,
                                                    std::to_string(static_cast<int>(ret)));
  }
  return ret;
}

api_error s3_provider::get_file_list(api_file_list &list) const {
  const auto ret = s3_comm_.get_file_list(
      [this](const std::string &api_path) -> std::string {
        std::string encryption_token;
        meta_db_.get_item_meta(api_path, META_ENCRYPTION_TOKEN, encryption_token);
        return encryption_token;
      },
      [this](const std::string &key, const std::string &object_name) -> std::string {
        std::string real_api_path;
        meta_db_.get_api_path_from_key(key, real_api_path);
        if (real_api_path.empty()) {
          return object_name;
        }

        auto object_parts = utils::string::split(object_name, '/', false);
        object_parts[object_parts.size() - 1u] =
            *(utils::string::split(real_api_path, '/', false).end() - 1u);
        return utils::string::join(object_parts, '/');
      },
      list);

  for (auto &file : list) {
    restore_api_path(file.api_path);
    file.api_parent = utils::path::get_parent_api_path(file.api_path);
  }

  return ret;
}

api_error s3_provider::get_file_size(const std::string &api_path, std::uint64_t &file_size) const {
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

bool s3_provider::is_directory(const std::string &api_path) const {
  if (api_path == "/") {
    return true;
  }
  return directory_db_.is_directory(format_api_path(api_path));
}

bool s3_provider::is_file(const std::string &api_path) const {
  if (api_path == "/") {
    return false;
  }

  return not directory_db_.is_directory(format_api_path(api_path)) &&
         s3_comm_.exists(api_path, [&]() -> std::string {
           std::string key;
           meta_db_.get_item_meta(format_api_path(api_path), META_KEY, key);
           return key;
         });
}

bool s3_provider::is_online() const { return s3_comm_.is_online(); }

bool s3_provider::is_processing(const std::string &api_path) const {
  return upload_manager_.is_processing(api_path);
}

void s3_provider::notify_directory_added(const std::string &api_path,
                                         const std::string &api_parent) {
  base_provider::notify_directory_added(api_path, api_parent);
  directory_db_.create_directory(format_api_path(api_path), true);
}

api_error s3_provider::notify_file_added(const std::string &api_path,
                                         const std::string & /*api_parent*/,
                                         const std::uint64_t & /*size*/) {
  recur_mutex_lock l(notify_added_mutex_);

  api_file file{};
  auto decrypted = false;
  std::string key;
  std::string file_name;

  auto ret = s3_comm_.get_file(
      api_path,
      [this, &api_path, &decrypted, &file_name, &key]() -> std::string {
        const auto temp_key = *(utils::string::split(api_path, '/', false).end() - 1u);
        file_name = temp_key;
        auto decrypted_file_name = file_name;
        if (utils::encryption::decrypt_file_name(s3_comm_.get_s3_config().encryption_token,
                                                 decrypted_file_name) == api_error::success) {
          decrypted = true;
          key = temp_key;
          file_name = decrypted_file_name;
          return key;
        }

        return "";
      },
      [&file_name](const std::string &, const std::string &object_name) -> std::string {
        auto object_parts = utils::string::split(object_name, '/', false);
        object_parts[object_parts.size() - 1u] = file_name;
        return utils::string::join(object_parts, '/');
      },
      [this, &decrypted]() -> std::string {
        return decrypted ? s3_comm_.get_s3_config().encryption_token : "";
      },
      file);

  restore_api_path(file.api_path);

  file.api_parent = utils::path::get_parent_api_path(file.api_path);
  if (ret == api_error::success) {
    api_item_added_(file.api_path, file.api_parent, file.source_path, false, file.created_date,
                    file.accessed_date, file.modified_date, file.changed_date);

    set_item_meta(file.api_path, META_SIZE, std::to_string(file.file_size));
    set_item_meta(file.api_path, META_KEY, key);
    set_item_meta(file.api_path, META_ENCRYPTION_TOKEN,
                  decrypted ? s3_comm_.get_s3_config().encryption_token : "");

    if (file.file_size) {
      global_data::instance().increment_used_drive_space(file.file_size);
    }
  }

  return ret;
}

api_error s3_provider::read_file_bytes(const std::string &api_path, const std::size_t &size,
                                       const std::uint64_t &offset, std::vector<char> &data,
                                       const bool &stop_requested) {
  const auto res = s3_comm_.read_file_bytes(
      api_path, size, offset, data,
      [this, &api_path]() -> std::string {
        std::string key;
        meta_db_.get_item_meta(format_api_path(api_path), META_KEY, key);
        return key;
      },
      [this, &api_path]() -> std::uint64_t {
        std::string temp;
        meta_db_.get_item_meta(format_api_path(api_path), META_SIZE, temp);
        return utils::string::to_uint64(temp);
      },
      [this, &api_path]() -> std::string {
        std::string encryption_token;
        meta_db_.get_item_meta(format_api_path(api_path), META_ENCRYPTION_TOKEN, encryption_token);
        return encryption_token;
      },
      stop_requested);
  return ((res == api_error::success) ? api_error::success : api_error::download_failed);
}

api_error s3_provider::remove_directory(const std::string &api_path) {
  auto ret = directory_db_.remove_directory(format_api_path(api_path));
  if (ret == api_error::success) {
    if ((utils::path::get_parent_api_path(api_path) == "/") &&
        s3_comm_.get_s3_config().bucket.empty()) {
      ret = s3_comm_.remove_bucket(api_path);
    }
  }

  if (ret == api_error::success) {
    remove_item_meta(api_path);
    event_system::instance().raise<directory_removed>(api_path);
  } else {
    event_system::instance().raise<directory_remove_failed>(api_path,
                                                            std::to_string(static_cast<int>(ret)));
  }

  return ret;
}

api_error s3_provider::remove_file(const std::string &api_path) {
  upload_manager_.remove_upload(api_path);
  const auto res = s3_comm_.remove_file(api_path, [&]() -> std::string {
    std::string key;
    meta_db_.get_item_meta(format_api_path(api_path), META_KEY, key);
    return key;
  });

  if (res == api_error::success) {
    remove_item_meta(api_path);
    event_system::instance().raise<file_removed>(api_path);
    return api_error::success;
  }

  event_system::instance().raise<file_remove_failed>(api_path,
                                                     std::to_string(static_cast<int>(res)));
  return api_error::error;
}

api_error s3_provider::rename_file(const std::string & /*from_api_path*/,
                                   const std::string & /*to_api_path*/) {
  // TODO Pending encrypted file name support
  return api_error::not_implemented;
  /* std::string curSource; */
  /* auto ret = get_item_meta(fromApiPath, META_SOURCE, curSource); */
  /* if (ret == api_error::success) { */
  /*   ret = s3_comm_.RenameFile(fromApiPath, toApiPath); */
  /*   if (ret == api_error::success) { */
  /*     metaDb_.RenameItemMeta(curSource, FormatApiFilePath(fromApiPath), */
  /*                            FormatApiFilePath(toApiPath)); */
  /*   } else { */
  /*     event_system::instance().raise<FileRenameFailed>(fromApiPath, toApiPath, */
  /*                                                     std::to_string(static_cast<int>(ret))); */
  /*   } */
  /* } */
  /* return ret; */
}

std::string &s3_provider::restore_api_path(std::string &api_path) const {
  if (api_path != "/") {
    if (not s3_comm_.get_s3_config().bucket.empty()) {
      api_path = utils::path::create_api_path(
          api_path.substr(s3_comm_.get_s3_config().bucket.length() + 1u));
    }
  }
  return api_path;
}

bool s3_provider::start(api_item_added_callback api_item_added, i_open_file_table *oft) {
  const auto unmount_requested = base_provider::start(api_item_added, oft);
  if (not unmount_requested && is_online()) {
    build_directories();
    upload_manager_.start();
  }

  api_file_list list;
  if (get_file_list(list) != api_error::success) {
    throw startup_exception("failed to determine used space");
  }

  const auto total_size =
      std::accumulate(list.begin(), list.end(), std::uint64_t(0),
                      [](std::uint64_t t, const api_file &file) { return t + file.file_size; });
  global_data::instance().initialize_used_drive_space(total_size);

  return unmount_requested;
}

void s3_provider::stop() { upload_manager_.stop(); }

void s3_provider::update_item_meta(directory_item &di, const bool &format_path) const {
  if (format_path) {
    restore_api_path(di.api_path);
    di.api_parent = utils::path::get_parent_api_path(di.api_path);
  }

  if (not di.directory && not di.resolved) {
    std::string real_api_path{};
    const auto get_api_path = [&]() -> api_error {
      if (meta_db_.get_api_path_from_key(
              *(utils::string::split(di.api_path, '/', false).end() - 1u), real_api_path) ==
          api_error::success) {
        di.api_path = real_api_path;
        if (format_path) {
          restore_api_path(di.api_path);
          di.api_parent = utils::path::get_parent_api_path(di.api_path);
        }
        return api_error::success;
      }
      return api_error::item_not_found;
    };

    if (get_api_path() != api_error::success) {
      this->get_item_meta(di.api_path, di.meta);
      get_api_path();
    }

    di.resolved = true;
  }

  if (di.meta.empty()) {
    this->get_item_meta(di.api_path, di.meta);
  }

  if (not di.directory) {
    if (di.meta[META_SIZE].empty()) {
      const_cast<s3_provider *>(this)->set_item_meta(di.api_path, META_SIZE,
                                                     std::to_string(di.size));
    } else {
      di.size = utils::string::to_uint64(di.meta[META_SIZE]);
    }
    this->oft_->update_directory_item(di);
  }
}

api_error s3_provider::upload_file(const std::string &api_path, const std::string &source_path,
                                   const std::string &encryption_token) {
  std::uint64_t file_size = 0u;
  utils::file::get_file_size(source_path, file_size);

  auto ret = set_source_path(api_path, source_path);
  if (ret == api_error::success) {
    if (((ret = set_item_meta(api_path, META_SIZE, std::to_string(file_size))) ==
         api_error::success) &&
        ((ret = set_item_meta(api_path, META_ENCRYPTION_TOKEN, encryption_token)) ==
         api_error::success)) {
      if (file_size == 0) {
        ret = s3_comm_.upload_file(
            api_path, source_path, encryption_token,
            [this, &api_path]() -> std::string {
              std::string key;
              meta_db_.get_item_meta(format_api_path(api_path), META_KEY, key);
              return key;
            },
            [this, &api_path](const std::string &key) -> api_error {
              return set_item_meta(api_path, META_KEY, key);
            },
            false);
      } else {
        ret = upload_manager_.queue_upload(api_path, source_path, encryption_token);
      }
    }
  }

  return ret;
}

api_error s3_provider::upload_handler(const upload_manager::upload &upload, json &, json &) {
  event_system::instance().raise<file_upload_begin>(upload.api_path, upload.source_path);
  auto ret = api_error::upload_failed;
  if (not upload.cancel) {
    ret = (s3_comm_.upload_file(
               upload.api_path, upload.source_path, upload.encryption_token,
               [this, &upload]() -> std::string {
                 std::string key;
                 meta_db_.get_item_meta(format_api_path(upload.api_path), META_KEY, key);
                 return key;
               },
               [this, &upload](const std::string &key) -> api_error {
                 return set_item_meta(upload.api_path, META_KEY, key);
               },
               upload.cancel) == api_error::success)
              ? api_error::success
              : api_error::upload_failed;
  }
  return ret;
}
} // namespace repertory
#endif // REPERTORY_ENABLE_S3
