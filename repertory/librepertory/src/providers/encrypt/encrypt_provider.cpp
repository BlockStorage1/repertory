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
#include "providers/encrypt/encrypt_provider.hpp"

#include "db/file_db.hpp"
#include "db/meta_db.hpp"
#include "events/event_system.hpp"
#include "events/types/directory_removed_externally.hpp"
#include "events/types/file_removed_externally.hpp"
#include "events/types/filesystem_item_added.hpp"
#include "events/types/service_start_begin.hpp"
#include "events/types/service_start_end.hpp"
#include "events/types/service_stop_begin.hpp"
#include "events/types/service_stop_end.hpp"
#include "platform/platform.hpp"
#include "types/repertory.hpp"
#include "types/startup_exception.hpp"
#include "utils/base64.hpp"
#include "utils/common.hpp"
#include "utils/config.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/encryption.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"
#include "utils/polling.hpp"

namespace repertory {
encrypt_provider::encrypt_provider(app_config &config)
    : config_(config), encrypt_config_(config.get_encrypt_config()) {}

auto encrypt_provider::create_api_file(std::string_view api_path,
                                       bool directory,
                                       std::string_view source_path)
    -> api_file {
  auto times{utils::file::get_times(source_path)};
  if (not times.has_value()) {
    throw std::runtime_error("failed to get file times");
  }

  api_file file{
      .api_path = std::string{api_path},
      .api_parent = utils::path::get_parent_api_path(api_path),
      .accessed_date = times->get(utils::file::time_type::accessed),
      .changed_date = times->get(utils::file::time_type::changed),
      .creation_date = times->get(utils::file::time_type::created),
      .file_size =
          directory
              ? 0U
              : utils::encryption::encrypting_reader::calculate_encrypted_size(
                    source_path, true),
      .key = "",
      .modified_date = times->get(utils::file::time_type::modified),
      .source_path = std::string{source_path},
      .written_date = times->get(utils::file::time_type::written),
  };

  return file;
}

void encrypt_provider::create_item_meta(api_meta_map &meta, bool directory,
                                        const api_file &file) {
#if defined(_WIN32)
  struct _stat64 buf{};
  _stat64(file.source_path.c_str(), &buf);
#else  // !defined(_WIN32)
  struct stat buf{};
  stat(file.source_path.c_str(), &buf);
#endif // defined(_WIN32)

  meta = provider_meta_creator(directory, file);

#if defined(_WIN32)
  meta[META_ATTRIBUTES] =
      std::to_string(::GetFileAttributesA(file.source_path.c_str()) &
                     ~static_cast<DWORD>(FILE_ATTRIBUTE_REPARSE_POINT));
#endif // defined(_WIN32)

  meta[META_MODE] = std::to_string(buf.st_mode);
  meta[META_GID] = std::to_string(buf.st_gid);
  meta[META_UID] = std::to_string(buf.st_uid);

#if defined(__APPLE__)
  meta[META_BACKUP];
  meta[META_OSXFLAGS] = std::to_string(buf.st_flags);
#endif // defined(__APPLE__)
}

auto encrypt_provider::create_directory(std::string_view api_path,
                                        api_meta_map & /*meta*/) -> api_error {
  if (api_path == "/") {
    return api_error::success;
  }

  return api_error::not_implemented;
}

auto encrypt_provider::do_fs_operation(
    std::string_view api_path, bool directory,
    std::function<api_error(const encrypt_config &cfg,
                            std::string_view source_path)>
        callback) const -> api_error {
  const auto &cfg{get_encrypt_config()};

  std::string source_path{api_path};
  if (api_path != "/" &&
      not utils::encryption::decrypt_file_path(master_key_, source_path)) {
    return directory ? api_error::directory_not_found
                     : api_error::item_not_found;
  }

  source_path = utils::path::combine(cfg.path, {source_path});
  if (source_path != cfg.path &&
      not source_path.starts_with(
          cfg.path + std::string{utils::path::directory_seperator})) {
    return directory ? api_error::directory_not_found
                     : api_error::item_not_found;
  }

  auto exists{utils::file::file{source_path}.exists()};
  if (exists && directory) {
    return api_error::item_exists;
  }
  if (not exists && not directory) {
    return api_error::item_not_found;
  }

  exists = utils::file::directory{source_path}.exists();
  if (exists && not directory) {
    return api_error::item_exists;
  }

  if (not exists && directory) {
    return api_error::directory_not_found;
  }

  return callback(cfg, source_path);
}

auto encrypt_provider::get_api_path_from_source(std::string_view source_path,
                                                std::string &api_path) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    return file_db_->get_api_path(source_path, api_path);
  } catch (const std::exception &ex) {
    utils::error::raise_error(function_name, ex, source_path,
                              "failed to get api path from source path");
  }

  return api_error::error;
}

auto encrypt_provider::get_directory_item_count(std::string_view api_path) const
    -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  std::uint64_t count{};
  auto res{
      do_fs_operation(api_path, true,
                      [&api_path, &count](auto && /* cfg */,
                                          auto &&source_path) -> api_error {
                        try {
                          count = utils::file::directory{source_path}.count();
                        } catch (const std::exception &ex) {
                          utils::error::raise_api_path_error(
                              function_name, api_path, source_path, ex,
                              "failed to get directory item count");
                        }
                        return api_error::success;
                      }),
  };
  if (res != api_error::success) {
    utils::error::raise_api_path_error(function_name, api_path, res,
                                       "failed to get directory item count");
  }

  return count;
}

auto encrypt_provider::get_directory_item(std::string_view api_path,
                                          directory_item &item) const
    -> api_error {
  bool directory{};
  auto ret = is_directory(api_path, directory);
  if (ret != api_error::success) {
    return ret;
  }

  filesystem_item fsi{};
  ret = get_filesystem_item(api_path, directory, fsi);
  if (ret != api_error::success) {
    return ret;
  }

  api_meta_map meta;
  ret = get_item_meta(api_path, meta);
  if (ret != api_error::success) {
    return ret;
  }

  item = {
      .api_path = fsi.api_path,
      .api_parent = fsi.api_parent,
      .directory = fsi.directory,
      .size = fsi.size,
      .meta = meta,
  };

  return ret;
}

auto encrypt_provider::get_directory_items(std::string_view api_path,
                                           directory_item_list &list) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  return do_fs_operation(
      api_path, true,
      [this, &list](auto &&cfg, auto &&source_path) -> api_error {
        try {
          for (const auto &dir_entry :
               utils::file::directory{source_path}.get_items()) {
            try {
              std::string current_api_path;
              if (dir_entry->is_directory_item()) {
                auto res{
                    file_db_->get_directory_api_path(dir_entry->get_path(),
                                                     current_api_path),
                };
                if (res != api_error::success &&
                    res != api_error::directory_not_found) {
                  // TODO raise error
                  continue;
                }

                if (res == api_error::directory_not_found) {
                  process_directory_entry(*dir_entry, cfg, current_api_path);

                  res = file_db_->get_directory_api_path(dir_entry->get_path(),
                                                         current_api_path);
                  if (res != api_error::success &&
                      res != api_error::directory_not_found) {
                    // TODO raise error
                    continue;
                  }
                }
              } else {
                auto res{
                    file_db_->get_file_api_path(dir_entry->get_path(),
                                                current_api_path),
                };
                if (res != api_error::success &&
                    res != api_error::item_not_found) {
                  // TODO raise error
                  continue;
                }
                if (res == api_error::item_not_found &&
                    not process_directory_entry(*dir_entry, cfg,
                                                current_api_path)) {
                  continue;
                }
              }

              auto file{
                  create_api_file(current_api_path,
                                  dir_entry->is_directory_item(),
                                  dir_entry->get_path()),
              };

              directory_item dir_item{};
              dir_item.api_parent = file.api_parent;
              dir_item.api_path = file.api_path;
              dir_item.directory = dir_entry->is_directory_item();
              dir_item.size = file.file_size;
              create_item_meta(dir_item.meta, dir_item.directory, file);

              list.emplace_back(std::move(dir_item));
            } catch (const std::exception &ex) {
              utils::error::raise_error(function_name, ex,
                                        dir_entry->get_path(),
                                        "failed to process directory item");
            }
          }
        } catch (const std::exception &ex) {
          utils::error::raise_error(function_name, ex, source_path,
                                    "failed to get directory items");
          return api_error::error;
        }

        std::sort(list.begin(), list.end(),
                  [](const auto &item1, const auto &item2) -> bool {
                    return (item1.directory && not item2.directory) ||
                           (not(item2.directory && not item1.directory) &&
                            (item1.api_path.compare(item2.api_path) < 0));
                  });

        list.insert(list.begin(), directory_item{
                                      .api_path = "..",
                                      .api_parent = "",
                                      .directory = true,
                                      .size = 0U,
                                      .meta =
                                          {
                                              {META_DIRECTORY,
                                               utils::string::from_bool(true)},
                                          },
                                  });
        list.insert(list.begin(), directory_item{
                                      .api_path = ".",
                                      .api_parent = "",
                                      .directory = true,
                                      .size = 0U,
                                      .meta =
                                          {
                                              {META_DIRECTORY,
                                               utils::string::from_bool(true)},
                                          },
                                  });
        return api_error::success;
      });
}

auto encrypt_provider::get_file(std::string_view api_path, api_file &file) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    bool exists{};
    auto res{is_directory(api_path, exists)};
    if (res != api_error::success) {
      return res;
    }
    if (exists) {
      return api_error::directory_exists;
    }

    std::string source_path;
    res = file_db_->get_file_source_path(api_path, source_path);
    if (res != api_error::success) {
      return res;
    }

    file = create_api_file(api_path, false, source_path);
    return api_error::success;
  } catch (const std::exception &ex) {
    utils::error::raise_error(function_name, ex, api_path,
                              "failed to get file");
  }

  return api_error::error;
}

auto encrypt_provider::get_file_list(api_file_list &list,
                                     std::string & /* marker */) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  const auto &cfg{get_encrypt_config()};

  try {
    using func = std::function<void(std::string path)>;
    const func process_directory = [&](std::string path) {
      for (const auto &dir_entry : utils::file::directory{path}.get_items()) {
        std::string api_path{};
        if (dir_entry->is_directory_item()) {
          process_directory_entry(*dir_entry, cfg, api_path);
          process_directory(dir_entry->get_path());
          continue;
        }

        if (process_directory_entry(*dir_entry, cfg, api_path)) {
          list.emplace_back(create_api_file(
              api_path, dir_entry->is_directory_item(), dir_entry->get_path()));
        }
      }
    };
    process_directory(cfg.path);

    return api_error::success;
  } catch (const std::exception &ex) {
    utils::error::raise_error(function_name, ex, cfg.path,
                              "failed to get file list");
  }

  return api_error::error;
}

auto encrypt_provider::get_file_size(std::string_view api_path,
                                     std::uint64_t &file_size) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    std::string source_path;
    auto res{file_db_->get_file_source_path(api_path, source_path)};
    if (res != api_error::success) {
      return res;
    }

    file_size = utils::encryption::encrypting_reader::calculate_encrypted_size(
        source_path, true);
    return api_error::success;
  } catch (const std::exception &ex) {
    utils::error::raise_error(function_name, ex, api_path,
                              "failed to get file size");
  }

  return api_error::error;
}

auto encrypt_provider::get_filesystem_item(std::string_view api_path,
                                           bool directory,
                                           filesystem_item &fsi) const
    -> api_error {
  std::string source_path;
  if (directory) {
    auto res{file_db_->get_directory_source_path(api_path, source_path)};
    if (res != api_error::success) {
      return res;
    }

    fsi.api_parent = utils::path::get_parent_api_path(api_path);
    fsi.api_path = api_path;
    fsi.directory = true;
    fsi.size = 0U;
    fsi.source_path = source_path;
    return api_error::success;
  }

  auto res{file_db_->get_file_source_path(api_path, source_path)};
  if (res != api_error::success) {
    return res;
  }

  fsi.api_path = api_path;
  fsi.api_parent = utils::path::get_parent_api_path(fsi.api_path);
  fsi.directory = false;
  fsi.size = utils::encryption::encrypting_reader::calculate_encrypted_size(
      source_path, true);
  fsi.source_path = source_path;

  return api_error::success;
}

auto encrypt_provider::get_filesystem_item_from_source_path(
    std::string_view source_path, filesystem_item &fsi) const -> api_error {
  std::string api_path{};
  auto res{get_api_path_from_source(source_path, api_path)};
  if (res != api_error::success) {
    return res;
  }

  return get_filesystem_item(api_path, false, fsi);
}

auto encrypt_provider::get_pinned_files() const -> std::vector<std::string> {
  return {};
}

auto encrypt_provider::get_item_meta(std::string_view api_path,
                                     api_meta_map &meta) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    std::string source_path;
    auto res{file_db_->get_source_path(api_path, source_path)};
    if (res != api_error::success) {
      return res;
    }

    bool is_dir{};
    res = is_directory(api_path, is_dir);
    if (res != api_error::success) {
      return res;
    }

    auto file{create_api_file(api_path, is_dir, source_path)};
    create_item_meta(meta, is_dir, file);
    return api_error::success;
  } catch (const std::exception &ex) {
    utils::error::raise_error(function_name, ex, api_path,
                              "failed to get item meta");
  }

  return api_error::error;
}

auto encrypt_provider::get_item_meta(std::string_view api_path,
                                     std::string_view key,
                                     std::string &value) const -> api_error {
  api_meta_map meta{};
  auto ret{get_item_meta(api_path, meta)};
  if (ret != api_error::success) {
    return ret;
  }

  value = meta[std::string{key}];
  return api_error::success;
}

auto encrypt_provider::get_total_drive_space() const -> std::uint64_t {
  return utils::file::get_total_drive_space(get_encrypt_config().path)
      .value_or(0U);
}

auto encrypt_provider::get_total_item_count() const -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    return file_db_->count();
  } catch (const std::exception &ex) {
    utils::error::raise_error(function_name, ex,
                              "failed to get total item count");
  }

  return 0U;
}

auto encrypt_provider::get_used_drive_space() const -> std::uint64_t {
  auto free_space{
      utils::file::get_free_drive_space(get_encrypt_config().path),
  };
  return free_space.has_value() ? get_total_drive_space() - free_space.value()
                                : 0U;
}

auto encrypt_provider::is_directory(std::string_view api_path,
                                    bool &exists) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    std::string source_path;
    auto res{file_db_->get_directory_source_path(api_path, source_path)};

    if (res != api_error::success) {
      if (res != api_error::directory_not_found) {
        return res;
      }

      exists = false;
      return api_error::success;
    }

    exists = utils::file::directory{source_path}.exists();
    return api_error::success;
  } catch (const std::exception &ex) {
    utils::error::raise_error(function_name, ex, api_path,
                              "failed determin if api path is directory");
  }

  return api_error::error;
}

auto encrypt_provider::is_file(std::string_view api_path, bool &exists) const
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    std::string source_path;
    auto res{file_db_->get_file_source_path(api_path, source_path)};
    if (res != api_error::success) {
      if (res != api_error::item_not_found) {
        return res;
      }

      exists = false;
      return api_error::success;
    }

    exists = utils::file::file{source_path}.exists();
    return api_error::success;
  } catch (const std::exception &ex) {
    utils::error::raise_error(function_name, ex, api_path,
                              "failed determin if api path is directory");
  }

  return api_error::error;
}

auto encrypt_provider::is_online() const -> bool {
  return utils::file::directory{get_encrypt_config().path}.exists();
}

auto encrypt_provider::process_directory_entry(
    const utils::file::i_fs_item &dir_entry, const encrypt_config &cfg,
    std::string &api_path) const -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    const auto do_add_directory =
        [this, &cfg](std::string_view dir_path) -> std::string {
      auto path_parts{
          utils::string::split(utils::path::create_api_path(dir_path), '/',
                               false),
      };

      auto current_source_path{cfg.path};
      std::string current_api_path;
      for (std::size_t part_idx = 1U; part_idx < path_parts.size();
           ++part_idx) {
        current_source_path = utils::path::combine(current_source_path,
                                                   {path_parts.at(part_idx)});

        std::string dir_api_path;
        auto res =
            file_db_->get_directory_api_path(current_source_path, dir_api_path);
        if (res == api_error::success) {
          current_api_path = dir_api_path;
          continue;
        }

        if (res == api_error::directory_not_found) {
          i_file_db::directory_data dir_data{};
          dir_data.kdf_configs.first = dir_data.kdf_configs.second =
              cfg.kdf_cfg;
          dir_data.kdf_configs.first.unique_id =
              utils::generate_secure_random<std::uint64_t>();
          dir_data.kdf_configs.second.unique_id =
              utils::generate_secure_random<std::uint64_t>();
          dir_data.kdf_configs.first.seal();
          dir_data.kdf_configs.second.seal();

          auto path_key = dir_data.kdf_configs.second.recreate_subkey(
              utils::encryption::kdf_context::path, master_key_);
          data_buffer encrypted_data;
          utils::encryption::encrypt_data(
              path_key,
              reinterpret_cast<const unsigned char *>(
                  path_parts.at(part_idx).c_str()),
              strnlen(path_parts.at(part_idx).c_str(),
                      path_parts.at(part_idx).size()),
              encrypted_data);

          auto hdr = dir_data.kdf_configs.second.to_header();
          encrypted_data.insert(encrypted_data.begin(), hdr.begin(), hdr.end());

          current_api_path = utils::path::create_api_path(
              current_api_path + '/' +
              macaron::Base64::EncodeUrlSafe(encrypted_data.data(),
                                             encrypted_data.size()));
          dir_data.api_path = current_api_path;
          dir_data.source_path = current_source_path;
          res = file_db_->add_or_update_directory(dir_data);
          if (res != api_error::success) {
            throw std::runtime_error(
                fmt::format("failed to set directory file data|{}",
                            api_error_to_string(res)));
          }

          event_system::instance().raise<filesystem_item_added>(
              utils::path::get_parent_api_path(current_api_path),
              current_api_path, true, function_name);
          continue;
        }

        throw std::runtime_error(fmt::format(
            "failed to get directory api path|{}", api_error_to_string(res)));
      }

      return current_api_path;
    };

    if (dir_entry.is_directory_item()) {
      api_path = do_add_directory(
          utils::path::get_relative_path(dir_entry.get_path(), cfg.path));
      return false;
    }

    if (dir_entry.is_file_item() && not dir_entry.is_symlink()) {
      auto relative_path{
          utils::path::get_relative_path(dir_entry.get_path(), cfg.path),
      };

      auto file_res{
          file_db_->get_file_api_path(dir_entry.get_path(), api_path)};
      if (file_res == api_error::success) {
        return true;
      }

      if (file_res != api_error::item_not_found) {
        // TODO raise error
        return false;
      }

      std::string api_parent{};
      auto parent_res{
          file_db_->get_directory_api_path(
              utils::path::get_parent_path(dir_entry.get_path()), api_parent),
      };
      if (parent_res != api_error::success &&
          parent_res != api_error::directory_not_found) {
        // TODO raise error
        return false;
      }

      if (parent_res == api_error::directory_not_found) {
        api_parent =
            do_add_directory(utils::path::get_parent_path(relative_path));
      }

      utils::encryption::encrypting_reader reader(
          utils::path::strip_to_file_name(relative_path), dir_entry.get_path(),
          []() -> bool { return app_config::get_stop_requested(); },
          master_key_, get_encrypt_config().kdf_cfg, std::nullopt);
      api_path = utils::path::create_api_path(api_parent + "/" +
                                              reader.get_encrypted_file_name());

      file_res = file_db_->add_or_update_file(i_file_db::file_data{
          .api_path = api_path,
          .file_size = dynamic_cast<const utils::file::i_file *>(&dir_entry)
                           ->size()
                           .value_or(0U),
          .iv_list = reader.get_iv_list(),
          .kdf_configs =
              {
                  *reader.get_kdf_config_for_data(),
                  *reader.get_kdf_config_for_path(),
              },
          .source_path = dir_entry.get_path(),
      });
      if (file_res != api_error::success) {
        // TODO raise error
        return false;
      }

      event_system::instance().raise<filesystem_item_added>(
          api_parent, api_path, false, function_name);

      return true;
    }
  } catch (const std::exception &ex) {
    utils::error::raise_error(function_name, ex, dir_entry.get_path(),
                              "failed to process directory entry");
  }

  return false;
}

auto encrypt_provider::read_file_bytes(std::string_view api_path,
                                       std::size_t size, std::uint64_t offset,
                                       data_buffer &data,
                                       stop_type &stop_requested) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  i_file_db::file_data file_data{};
  auto res{file_db_->get_file_data(api_path, file_data)};
  if (res != api_error::success) {
    return res;
  }

  auto opt_size{utils::file::file{file_data.source_path}.size()};
  if (not opt_size.has_value()) {
    return api_error::os_error;
  }

  auto file_size{opt_size.value()};

  const auto &cfg{get_encrypt_config()};

  unique_recur_mutex_lock reader_lookup_lock(reader_lookup_mtx_);

  if (file_data.file_size != file_size) {
    auto relative_path{
        utils::path::get_relative_path(file_data.source_path, cfg.path),
    };

    auto info{std::make_shared<reader_info>()};
    info->reader = std::make_unique<utils::encryption::encrypting_reader>(
        relative_path, file_data.source_path,
        [&stop_requested]() -> bool {
          return stop_requested || app_config::get_stop_requested();
        },
        master_key_, file_data.kdf_configs,
        utils::path::get_parent_path(relative_path));
    reader_lookup_[file_data.source_path] = info;
    file_data.file_size = file_size;
    file_data.iv_list = info->reader->get_iv_list();

    res = file_db_->add_or_update_file(file_data);
    file_data.iv_list.clear();

    if (res != api_error::success) {
      utils::error::raise_error(function_name, res, file_data.source_path,
                                "failed to update file");
      return res;
    }
  } else if (not reader_lookup_.contains(file_data.source_path)) {
    auto info{std::make_shared<reader_info>()};
    info->reader = std::make_unique<utils::encryption::encrypting_reader>(
        [&stop_requested]() -> bool {
          return stop_requested || app_config::get_stop_requested();
        },
        api_path, file_data.source_path, master_key_, file_data.kdf_configs,
        std::move(file_data.iv_list));
    reader_lookup_[file_data.source_path] = info;
  }

  if (file_size == 0U || size == 0U) {
    return api_error::success;
  }

  auto info{reader_lookup_.at(file_data.source_path)};
  info->last_access_time = std::chrono::system_clock::now();
  reader_lookup_lock.unlock();

  mutex_lock reader_lock(info->reader_mtx);
  info->reader->set_read_position(offset);
  data.resize(size);

  auto ret{
      utils::encryption::encrypting_reader::reader_function(
          reinterpret_cast<char *>(data.data()), 1U, data.size(),
          info->reader.get()),
  };
  return ret == 0 ? api_error::os_error : api_error::success;
}

void encrypt_provider::remove_deleted_files(stop_type &stop_requested) {
  REPERTORY_USES_FUNCTION_NAME();

  const auto get_stop_requested = [&stop_requested]() -> bool {
    return stop_requested || app_config::get_stop_requested();
  };

  file_db_->enumerate_item_list(
      [this, &get_stop_requested](auto &&list) {
        std::vector<i_file_db::file_info> removed_list{};
        for (const auto &item : list) {
          if (get_stop_requested()) {
            return;
          }

          if (utils::path::exists(item.source_path)) {
            continue;
          }

          removed_list.emplace_back(item);
        }

        for (const auto &item : removed_list) {
          if (get_stop_requested()) {
            return;
          }

          auto res{file_db_->remove_item(item.api_path)};
          if (res != api_error::success) {
            utils::error::raise_api_path_error(
                function_name, item.api_path, item.source_path, res,
                fmt::format("failed to process externally removed item|dir|{}",
                            utils::string::from_bool(item.directory)));
            continue;
          }

          if (item.directory) {
            event_system::instance().raise<directory_removed_externally>(
                item.api_path, function_name, item.source_path);
            continue;
          }

          event_system::instance().raise<file_removed_externally>(
              item.api_path, function_name, item.source_path);
        }
      },
      get_stop_requested);
}

void encrypt_provider::remove_expired_files() {
  recur_mutex_lock reader_lookup_lock(reader_lookup_mtx_);
  auto remove_list = std::accumulate(
      reader_lookup_.begin(), reader_lookup_.end(), std::vector<std::string>(),
      [this](auto &&val, auto &&pair) -> auto {
        const auto &[key, value] = pair;
        auto diff = std::chrono::system_clock::now() - value->last_access_time;
        if (std::chrono::duration_cast<std::chrono::seconds>(diff) >=
            std::chrono::seconds(config_.get_download_timeout_secs())) {
          val.emplace_back(key);
        }
        return val;
      });
  for (const auto &key : remove_list) {
    reader_lookup_.erase(key);
  }
}

auto encrypt_provider::remove_item_meta(std::string_view api_path,
                                        std::string_view key) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  if (utils::collection::includes(META_USED_NAMES, std::string{key})) {
    utils::error::raise_api_path_error(
        function_name, api_path,
        fmt::format("failed to remove item meta-key is restricted|key|{}",
                    key));
    return api_error::permission_denied;
  }

  return api_error::success;
}

auto encrypt_provider::start(api_item_added_callback /*api_item_added*/,
                             i_file_manager * /*mgr*/) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  if (not is_online()) {
    return false;
  }

  event_system::instance().raise<service_start_begin>(function_name,
                                                      "encrypt_provider");
  file_db_ = create_file_db(config_);

  if (encrypt_config_.kdf_cfg.checksum == 0U) {
    i_file_db::directory_data data{};
    if (file_db_->get_directory_data("/", data) == api_error::success) {
      if (data.kdf_configs.first.checksum == 0U) {
        encrypt_config_.kdf_cfg.seal();
      } else {
        encrypt_config_.kdf_cfg = data.kdf_configs.first;
      }
    } else {
      encrypt_config_.kdf_cfg.seal();
    }

    config_.set_encrypt_config(encrypt_config_);
    data.kdf_configs = {
        encrypt_config_.kdf_cfg,
        encrypt_config_.kdf_cfg,
    };
    auto res = file_db_->add_or_update_directory(data);
    if (res != api_error::success) {
      throw startup_exception(fmt::format("failed to update existing kdf|{}",
                                          api_error_to_string(res)));
    }
  }

  if (encrypt_config_.kdf_cfg.checksum !=
      encrypt_config_.kdf_cfg.generate_checksum()) {
    utils::error::raise_error(
        function_name, "existing kdf configuration failed checksum validation");
    return false;
  }

  if (not utils::encryption::recreate_key_argon2id(
          get_encrypt_config().encryption_token, get_encrypt_config().kdf_cfg,
          master_key_)) {
    utils::error::raise_error(function_name,
                              "failed to recreate master key from kdf config");
    return false;
  }

  std::string source_path;
  auto res{file_db_->get_directory_source_path("/", source_path)};
  if (res != api_error::success && res != api_error::directory_not_found) {
    throw startup_exception(
        fmt::format("failed to get root|{}", api_error_to_string(res)));
  }

  auto cfg_path{utils::path::absolute(get_encrypt_config().path)};
  if (res == api_error::success) {
    auto cur_path{utils::path::absolute(source_path)};
#if defined(_WIN32)
    if (utils::string::to_lower(cur_path) !=
        utils::string::to_lower(cfg_path)) {
#else  // !defined(_WIN32)
    if (cur_path != cfg_path) {
#endif // defined(_WIN32)
      throw startup_exception(fmt::format(
          "source path has changed|cur|{}|cfg|{}", cur_path, cfg_path));
    }
  } else {
    i_file_db::directory_data dir_data{
        .api_path = "/",
        .kdf_configs =
            {
                get_encrypt_config().kdf_cfg,
                get_encrypt_config().kdf_cfg,
            },
        .source_path = cfg_path,
    };
    res = file_db_->add_or_update_directory(dir_data);
    if (res != api_error::success) {
      throw startup_exception(
          fmt::format("failed to create root|{}", api_error_to_string(res)));
    }
  }

  polling::instance().set_callback({
      .name = "check_deleted",
      .freq = polling::frequency::low,
      .action =
          [this](auto &&stop_requested) {
            remove_deleted_files(stop_requested);
          },
  });

  polling::instance().set_callback({
      .name = "remove_expired",
      .freq = polling::frequency::second,
      .action =
          [this](auto && /* stop_requested */) { remove_expired_files(); },
  });

  event_system::instance().raise<service_start_end>(function_name,
                                                    "encrypt_provider");
  return true;
}

void encrypt_provider::stop() {
  REPERTORY_USES_FUNCTION_NAME();

  event_system::instance().raise<service_stop_begin>(function_name,
                                                     "encrypt_provider");
  polling::instance().remove_callback("check_deleted");
  polling::instance().remove_callback("remove_expired");

  unique_recur_mutex_lock reader_lookup_lock(reader_lookup_mtx_);
  reader_lookup_.clear();
  reader_lookup_lock.unlock();

  file_db_.reset();
  event_system::instance().raise<service_stop_end>(function_name,
                                                   "encrypt_provider");
}
} // namespace repertory
