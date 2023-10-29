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

#include "providers/s3/s3_provider.hpp"

#include "app_config.hpp"
#include "comm/i_s3_comm.hpp"
#include "db/meta_db.hpp"
#include "file_manager/i_file_manager.hpp"
#include "types/repertory.hpp"
#include "types/startup_exception.hpp"
#include "utils/encryption.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/polling.hpp"

namespace repertory {
s3_provider::s3_provider(app_config &config, i_s3_comm &s3_comm)
    : base_provider(config), s3_comm_(s3_comm) {}

auto s3_provider::check_file_exists(const std::string &api_path) const
    -> api_error {
  static const std::string function_name = __FUNCTION__;

  return s3_comm_.file_exists(api_path, [&]() -> std::string {
    std::string key;
    const auto res = meta_db_->get_item_meta(api_path, META_KEY, key);
    if ((res != api_error::success) && (res != api_error::item_not_found)) {
      utils::error::raise_api_path_error(function_name, api_path,
                                         api_error_to_string(res),
                                         "failed to check if file exists");
    }
    return key;
  });
}

void s3_provider::create_directories() {
  api_file_list list{};
  auto ret = s3_comm_.get_directory_list(list);
  if (ret == api_error::success) {
    for (const auto &dir : list) {
      bool exists{};
      ret = is_directory(dir.api_path, exists);
      if (ret == api_error::success && not exists) {
        create_parent_directories(dir, true);
      }
    }
  } else {
    utils::error::raise_error(__FUNCTION__, api_error_to_string(ret));
  }
}

auto s3_provider::create_directory(const std::string &api_path,
                                   api_meta_map &meta) -> api_error {
  if (utils::path::is_trash_directory(api_path)) {
    return api_error::access_denied;
  }

  bool exists{};
  auto res = is_file(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    return api_error::item_exists;
  }

  res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    return api_error::directory_exists;
  }

  res = s3_comm_.create_directory(api_path);
  if ((res != api_error::success) && (res != api_error::directory_exists)) {
    return res;
  }

  if ((res = directory_db_->create_directory(api_path)) != api_error::success) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path,
                                              api_error_to_string(res));
    return res;
  }

  meta[META_DIRECTORY] = utils::string::from_bool(true);
  res = set_item_meta(api_path, meta);
  if (res != api_error::success) {
    event_system::instance().raise<debug_log>(__FUNCTION__, api_path,
                                              api_error_to_string(res));
  }

  return res;
}

auto s3_provider::create_file(const std::string &api_path, api_meta_map &meta)
    -> api_error {
  if (meta[META_ENCRYPTION_TOKEN].empty()) {
    meta[META_ENCRYPTION_TOKEN] = s3_comm_.get_s3_config().encryption_token;
  }

  meta[META_DIRECTORY] = utils::string::from_bool(false);
  return base_provider::create_file(api_path, meta);
}

void s3_provider::create_parent_directories(const api_file &file,
                                            bool directory) {
  const auto dir = directory ? file.api_path : file.api_parent;
  std::unordered_map<std::string, std::uint8_t> directories;
  if (directories.find(dir) == directories.end()) {
    directories[dir] = 0u;

    const auto directory_parts = utils::string::split(dir, '/', false);

    std::string current_directory;
    for (std::size_t i = 0u; i < (directory_parts.size() - 1u); i++) {
      current_directory = utils::path::create_api_path(
          current_directory.empty()
              ? directory_parts[0u]
              : utils::path::combine(current_directory, {directory_parts[i]}));
      directories[current_directory] = 0u;
    }
  }

  for (const auto &kv : directories) {
    if (not directory_db_->is_directory(kv.first)) {
      const auto api_path = kv.first;
      auto res = this->notify_directory_added(
          api_path, utils::path::get_parent_api_path(api_path));
      if (res != api_error::success) {
        utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                           "failed to add directory");
      }
    }
  }
}

auto s3_provider::get_directory_item_count(const std::string &api_path) const
    -> std::uint64_t {
  return s3_comm_.get_directory_item_count(api_path, [this](
                                                         directory_item &di) {
    this->update_item_meta(di);
  }) + directory_db_->get_sub_directory_count(api_path);
}

auto s3_provider::get_file_list(api_file_list &list) const -> api_error {
  try {
    const auto get_name =
        [this](const std::string &key,
               const std::string &object_name) -> std::string {
      std::string real_api_path;
      const auto res = meta_db_->get_api_path_from_key(key, real_api_path);
      if (res != api_error::success && res != api_error::item_not_found) {
        throw std::runtime_error(api_error_to_string(res));
      }
      if (real_api_path.empty()) {
        return object_name;
      }

      auto object_parts = utils::string::split(object_name, '/', false);
      object_parts[object_parts.size() - 1u] =
          *(utils::string::split(real_api_path, '/', false).end() - 1u);
      return utils::string::join(object_parts, '/');
    };

    const auto ret = s3_comm_.get_file_list(
        [this, &get_name](const std::string &api_path) -> std::string {
          std::string encryption_token;
          auto res = meta_db_->get_item_meta(api_path, META_ENCRYPTION_TOKEN,
                                             encryption_token);
          if (res != api_error::success) {
            if (res == api_error::item_not_found) {
              if ((res = const_cast<s3_provider *>(this)->notify_file_added(
                       api_path, "", 0u)) == api_error::success) {
                const auto meta_api_path = get_name(
                    *(utils::string::split(api_path, '/', false).end() - 1u),
                    api_path);
                res = meta_db_->get_item_meta(
                    meta_api_path, META_ENCRYPTION_TOKEN, encryption_token);
              }
            }

            if (res != api_error::success) {
              throw std::runtime_error(api_error_to_string(res));
            }
          }
          return encryption_token;
        },
        get_name, list);
    if (ret != api_error::success) {
      utils::error::raise_error(__FUNCTION__, "failed to get file list|err|" +
                                                  api_error_to_string(ret));
    }

    return ret;
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "failed to get file list");
  }

  return api_error::error;
}

auto s3_provider::get_total_item_count() const -> std::uint64_t {
  return meta_db_->get_total_item_count();
}

auto s3_provider::handle_rename_file(const std::string &from_api_path,
                                     const std::string &to_api_path,
                                     const std::string & /*source_path*/)
    -> api_error {
  // TODO Pending encrypted file name support
  event_system::instance().raise<file_rename_failed>(
      from_api_path, to_api_path,
      api_error_to_string(api_error::not_implemented));
  return api_error::not_implemented;
  /* std::string curSource; */
  /* auto ret = get_item_meta(fromApiPath, META_SOURCE, curSource); */
  /* if (ret == api_error::success) { */
  /*   ret = s3_comm_.RenameFile(fromApiPath, toApiPath); */
  /*   if (ret == api_error::success) { */
  /*     metaDb_.RenameItemMeta(curSource, FormatApiFilePath(fromApiPath), */
  /*                            FormatApiFilePath(toApiPath)); */
  /*   } else { */
  /*     event_system::instance().raise<FileRenameFailed>(fromApiPath,
   * toApiPath, */
  /*                                                     std::to_string(static_cast<int>(ret)));
   */
  /*   } */
  /* } */
  /* return ret; */
}

auto s3_provider::is_directory(const std::string &api_path, bool &exists) const
    -> api_error {
  if (api_path == "/") {
    exists = true;
    return api_error::success;
  }

  if (utils::path::is_trash_directory(api_path)) {
    exists = false;
    return api_error::success;
  }

  exists = directory_db_->is_directory(api_path);
  return api_error::success;
}

auto s3_provider::is_file(const std::string &api_path, bool &exists) const
    -> api_error {
  static const std::string function_name = __FUNCTION__;

  if (api_path == "/") {
    exists = false;
    return api_error::success;
  }

  if (utils::path::is_trash_directory(api_path)) {
    exists = false;
    return api_error::success;
  }

  exists =
      not directory_db_->is_directory(api_path) &&
      (s3_comm_.file_exists(api_path, [&]() -> std::string {
        std::string key;
        const auto res = meta_db_->get_item_meta(api_path, META_KEY, key);
        if ((res != api_error::success) && (res != api_error::item_not_found)) {
          utils::error::raise_api_path_error(function_name, api_path, res,
                                             "failed to get item meta");
        }
        return key;
      }) == api_error::item_exists);
  return api_error::success;
}

auto s3_provider::is_online() const -> bool { return s3_comm_.is_online(); }

auto s3_provider::notify_directory_added(const std::string &api_path,
                                         const std::string &api_parent)
    -> api_error {
  recur_mutex_lock l(notify_added_mutex_);

  auto res = base_provider::notify_directory_added(api_path, api_parent);
  if (res != api_error::success) {
    return res;
  }

  return directory_db_->create_directory(api_path, true);
}

auto s3_provider::notify_file_added(const std::string &api_path,
                                    const std::string & /*api_parent*/,
                                    std::uint64_t /*size*/) -> api_error {
  recur_mutex_lock l(notify_added_mutex_);

  api_file file{};
  auto decrypted = false;
  std::string key;
  std::string file_name;

  auto ret = s3_comm_.get_file(
      api_path,
      [this, &api_path, &decrypted, &file_name, &key]() -> std::string {
        const auto temp_key =
            *(utils::string::split(api_path, '/', false).end() - 1u);
        file_name = temp_key;
        auto decrypted_file_name = file_name;
        if (utils::encryption::decrypt_file_name(
                s3_comm_.get_s3_config().encryption_token,
                decrypted_file_name) == api_error::success) {
          decrypted = true;
          key = temp_key;
          file_name = decrypted_file_name;
          return key;
        }

        return "";
      },
      [&file_name](const std::string &,
                   const std::string &object_name) -> std::string {
        auto object_parts = utils::string::split(object_name, '/', false);
        object_parts[object_parts.size() - 1u] = file_name;
        return utils::string::join(object_parts, '/');
      },
      [this, &decrypted]() -> std::string {
        return decrypted ? s3_comm_.get_s3_config().encryption_token : "";
      },
      file);

  if (ret != api_error::success) {
    return ret;
  }
  create_parent_directories(file, false);

  file.encryption_token =
      decrypted ? s3_comm_.get_s3_config().encryption_token : "";
  file.key = key;
  return api_item_added_(false, file);
}

auto s3_provider::populate_directory_items(const std::string &api_path,
                                           directory_item_list &list) const
    -> api_error {
  bool exists{};
  auto ret = is_directory(api_path, exists);
  if (ret != api_error::success) {
    return ret;
  }
  if (not exists) {
    return api_error::directory_not_found;
  }

  ret = s3_comm_.get_directory_items(
      api_path, [this](directory_item &di) { this->update_item_meta(di); },
      list);

  if (ret == api_error::success) {
    directory_db_->populate_sub_directories(
        api_path,
        [this](directory_item &di) {
          auto res = this->get_item_meta(di.api_path, di.meta);
          if (res == api_error::success) {
            di.resolved = true;
          } else {
            utils::error::raise_api_path_error("populate_directory_items",
                                               di.api_path, res,
                                               "failed to get item meta");
          }
        },
        list);
  }

  return ret;
}

auto s3_provider::populate_file(const std::string &api_path,
                                api_file &file) const -> api_error {
  auto real_api_path = api_path;
  return s3_comm_.get_file(
      api_path,
      [this, &api_path, &real_api_path]() -> std::string {
        auto key = *(utils::string::split(api_path, '/', false).end() - 1u);
        const auto handle_error = [&key](api_error res) {
          if (res != api_error::success) {
            if (res == api_error::item_not_found) {
              key = "";
            } else {
              throw std::runtime_error(api_error_to_string(res));
            }
          }
        };

        std::string meta_api_path;
        handle_error(meta_db_->get_api_path_from_key(key, meta_api_path));

        if (meta_api_path.empty()) {
          handle_error(meta_db_->get_item_meta(real_api_path, META_KEY, key));
        }

        return key;
      },
      [this, &real_api_path](const std::string &key,
                             const std::string &object_name) -> std::string {
        if (key.empty()) {
          return object_name;
        }

        const auto res = meta_db_->get_api_path_from_key(key, real_api_path);
        if (res != api_error::success) {
          throw std::runtime_error(api_error_to_string(res));
        }

        auto object_parts = utils::string::split(object_name, '/', false);
        object_parts[object_parts.size() - 1u] =
            *(utils::string::split(real_api_path, '/', false).end() - 1u);
        return utils::string::join(object_parts, '/');
      },
      [this, &real_api_path]() -> std::string {
        std::string encryption_token;
        const auto res = meta_db_->get_item_meta(
            real_api_path, META_ENCRYPTION_TOKEN, encryption_token);
        if (res != api_error::success && res != api_error::item_not_found) {
          throw std::runtime_error(api_error_to_string(res));
        }
        return encryption_token;
      },
      file);
}

auto s3_provider::read_file_bytes(const std::string &api_path, std::size_t size,
                                  std::uint64_t offset, data_buffer &data,
                                  stop_type &stop_requested) -> api_error {
  try {
    const auto do_read = [&]() -> api_error {
      return s3_comm_.read_file_bytes(
          api_path, size, offset, data,
          [this, &api_path]() -> std::string {
            std::string key;
            const auto res = meta_db_->get_item_meta(api_path, META_KEY, key);
            if (res != api_error::success) {
              throw std::runtime_error(api_error_to_string(res));
            }
            return key;
          },
          [this, &api_path]() -> std::uint64_t {
            std::string temp;
            const auto res = meta_db_->get_item_meta(api_path, META_SIZE, temp);
            if (res != api_error::success) {
              throw std::runtime_error(api_error_to_string(res));
            }
            return utils::string::to_uint64(temp);
          },
          [this, &api_path]() -> std::string {
            std::string encryption_token;
            const auto res = meta_db_->get_item_meta(
                api_path, META_ENCRYPTION_TOKEN, encryption_token);
            if (res != api_error::success) {
              throw std::runtime_error(api_error_to_string(res));
            }
            return encryption_token;
          },
          stop_requested);
    };

    auto res = do_read();
    for (std::uint16_t i = 0U;
         not stop_requested && res != api_error::success &&
         i < get_config().get_retry_read_count();
         i++) {
      utils::error::raise_api_path_error(
          __FUNCTION__, api_path, res,
          "read file bytes failed|offset|" + std::to_string(offset) + "|size|" +
              std::to_string(size) + "|retry|" + std::to_string(i + 1U));
      std::this_thread::sleep_for(1s);
      res = do_read();
    }

    return ((res == api_error::success) ? api_error::success
                                        : api_error::download_failed);
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                       "failed to read file bytes ");
    return api_error::error;
  }
}

auto s3_provider::remove_directory(const std::string &api_path) -> api_error {
  auto ret = directory_db_->remove_directory(api_path);
  if (ret == api_error::success) {
    if ((ret = s3_comm_.remove_directory(api_path)) != api_error::success) {
      event_system::instance().raise<directory_remove_failed>(
          api_path, api_error_to_string(ret));
      return ret;
    }

    if ((ret = remove_item_meta(api_path)) != api_error::success) {
      event_system::instance().raise<directory_remove_failed>(
          api_path, api_error_to_string(ret));
      return ret;
    }

    event_system::instance().raise<directory_removed>(api_path);
  } else {
    event_system::instance().raise<directory_remove_failed>(
        api_path, api_error_to_string(ret));
  }

  return ret;
}

auto s3_provider::remove_file(const std::string &api_path) -> api_error {
  try {
    auto res = s3_comm_.remove_file(api_path, [&]() -> std::string {
      std::string key;
      const auto res2 = meta_db_->get_item_meta(api_path, META_KEY, key);
      if (res2 != api_error::success && res2 != api_error::item_not_found) {
        throw std::runtime_error(api_error_to_string(res2));
      }
      return key;
    });

    if (res == api_error::success || res == api_error::item_not_found) {
      if ((res = remove_item_meta(api_path)) != api_error::success) {
        utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                           "failed to remove item meta");
      }
      event_system::instance().raise<file_removed>(api_path);
      return api_error::success;
    }

    event_system::instance().raise<file_remove_failed>(
        api_path, api_error_to_string(res));
    return res;
  } catch (const std::exception &e) {
    event_system::instance().raise<file_remove_failed>(
        api_path, e.what() ? e.what() : "file remove failed");
    return api_error::error;
  }
}

auto s3_provider::start(api_item_added_callback api_item_added,
                        i_file_manager *fm) -> bool {
  if (s3_comm_.get_s3_config().bucket.empty()) {
    throw startup_exception("s3 bucket name cannot be empty");
  }

  directory_db_ = std::make_unique<directory_db>(get_config());

  auto success = base_provider::start(api_item_added, fm);
  if (success) {
    create_directories();

    background_thread_ = std::make_unique<std::thread>([this] {
      cleanup();

      polling::instance().set_callback(
          {"s3_cleanup", polling::frequency::low, [this]() {
             create_directories();
             remove_deleted_files();
             remove_expired_orphaned_files();
           }});
    });
  }

  event_system::instance().raise<service_started>("s3_provider");
  return success;
}

void s3_provider::stop() {
  event_system::instance().raise<service_shutdown_begin>("s3_provider");

  if (background_thread_) {
    background_thread_->join();
    background_thread_.reset();
  }

  polling::instance().remove_callback("s3_cleanup");
  base_provider::stop();
  directory_db_.reset();

  event_system::instance().raise<service_shutdown_end>("s3_provider");
}

void s3_provider::update_item_meta(directory_item &di) const {
  if (not di.directory && not di.resolved) {
    std::string real_api_path{};
    const auto get_api_path = [&]() -> api_error {
      if (meta_db_->get_api_path_from_key(
              *(utils::string::split(di.api_path, '/', false).end() - 1u),
              real_api_path) == api_error::success) {
        di.api_path = real_api_path;
        di.api_parent = utils::path::get_parent_api_path(di.api_path);
        return api_error::success;
      }
      return api_error::item_not_found;
    };

    if (get_api_path() != api_error::success) {
      auto res = this->get_item_meta(di.api_path, di.meta);
      if (res != api_error::success) {
        utils::error::raise_api_path_error(__FUNCTION__, di.api_path, res,
                                           "failed to get item meta");
      }
      get_api_path();
    }

    di.resolved = true;
  }

  if (di.meta.empty()) {
    auto res = this->get_item_meta(di.api_path, di.meta);
    if (res != api_error::success) {
      utils::error::raise_api_path_error(__FUNCTION__, di.api_path, res,
                                         "failed to get item meta");
    }
  }

  if (not di.directory) {
    // const auto file_size = ([this, &di]() -> std::string {
    //   const auto file_size = std::to_string(di.size);
    //   auto meta_size = di.meta[META_SIZE];
    //   if ((meta_size != file_size) && not fm_->is_processing(di.api_path)) {
    //     meta_size = file_size;
    //   }
    //   return meta_size;
    // })();
    //
    // if (di.meta[META_SIZE] != file_size) {
    //   // TODO-revisit this
    //   const_cast<s3_provider *>(this)->set_item_meta(di.api_path, META_SIZE,
    //   file_size);
    // }
    //
    // di.size = utils::string::to_uint64(file_size);
    di.size = utils::string::to_uint64(di.meta[META_SIZE]);
  }
}

auto s3_provider::upload_file(const std::string &api_path,
                              const std::string &source_path,
                              const std::string &encryption_token,
                              stop_type &stop_requested) -> api_error {
  event_system::instance().raise<provider_upload_begin>(api_path, source_path);
  auto ret = api_error::upload_stopped;
  try {
    ret = s3_comm_.upload_file(
        api_path, source_path, encryption_token,
        [this, &api_path]() -> std::string {
          std::string key;
          const auto res = meta_db_->get_item_meta(api_path, META_KEY, key);
          if (res != api_error::success) {
            throw std::runtime_error(api_error_to_string(res));
          }
          return key;
        },
        [this, &api_path](const std::string &key) -> api_error {
          return set_item_meta(api_path, META_KEY, key);
        },
        stop_requested);
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, source_path, e,
                                       "failed to upload file");
    ret = api_error::error;
  }

  event_system::instance().raise<provider_upload_end>(api_path, source_path,
                                                      ret);
  calculate_used_drive_space(false);
  return ret;
}
} // namespace repertory

#endif // REPERTORY_ENABLE_S3
