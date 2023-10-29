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
#include "providers/encrypt/encrypt_provider.hpp"

#include "events/event_system.hpp"
#include "events/events.hpp"
#include "platform/win32_platform.hpp"
#include "types/repertory.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/path_utils.hpp"
#include "utils/polling.hpp"
#include "utils/rocksdb_utils.hpp"

namespace repertory {
encrypt_provider::encrypt_provider(app_config &config) : config_(config) {}

auto encrypt_provider::create_api_file(const std::string api_path,
                                       bool directory,
                                       const std::string &source_path)
    -> api_file {
#ifdef _WIN32
  struct _stat64 buf {};
  _stat64(source_path.c_str(), &buf);
#else
  struct stat buf {};
  stat(source_path.c_str(), &buf);
#endif

  api_file file{};
  file.api_path = api_path;
  file.api_parent = utils::path::get_parent_api_path(api_path);
  file.file_size =
      directory
          ? 0U
          : utils::encryption::encrypting_reader::calculate_encrypted_size(
                source_path);
  file.source_path = source_path;
#ifdef __APPLE__
  file.changed_date =
      buf.st_ctimespec.tv_nsec + (buf.st_ctimespec.tv_sec * NANOS_PER_SECOND);
  file.accessed_date =
      buf.st_atimespec.tv_nsec + (buf.st_atimespec.tv_sec * NANOS_PER_SECOND);
  file.creation_date = buf.st_birthtimespec.tv_nsec +
                       (buf.st_birthtimespec.tv_sec * NANOS_PER_SECOND);
  file.modified_date =
      buf.st_mtimespec.tv_nsec + (buf.st_mtimespec.tv_sec * NANOS_PER_SECOND);
#elif _WIN32
  FILETIME ft{};
  utils::unix_time_to_filetime(utils::time64_to_unix_time(buf.st_atime), ft);
  file.accessed_date =
      (static_cast<std::uint64_t>(ft.dwHighDateTime) << 32U) | ft.dwLowDateTime;

  utils::unix_time_to_filetime(utils::time64_to_unix_time(buf.st_mtime), ft);
  file.changed_date =
      (static_cast<std::uint64_t>(ft.dwHighDateTime) << 32U) | ft.dwLowDateTime;

  utils::unix_time_to_filetime(utils::time64_to_unix_time(buf.st_ctime), ft);
  file.creation_date =
      (static_cast<std::uint64_t>(ft.dwHighDateTime) << 32U) | ft.dwLowDateTime;

  utils::unix_time_to_filetime(utils::time64_to_unix_time(buf.st_mtime), ft);
  file.modified_date =
      (static_cast<std::uint64_t>(ft.dwHighDateTime) << 32U) | ft.dwLowDateTime;
#else
  file.changed_date =
      buf.st_mtim.tv_nsec + (buf.st_mtim.tv_sec * NANOS_PER_SECOND);
  file.accessed_date =
      buf.st_atim.tv_nsec + (buf.st_atim.tv_sec * NANOS_PER_SECOND);
  file.creation_date =
      buf.st_ctim.tv_nsec + (buf.st_ctim.tv_sec * NANOS_PER_SECOND);
  file.modified_date =
      buf.st_mtim.tv_nsec + (buf.st_mtim.tv_sec * NANOS_PER_SECOND);
#endif

  return file;
}

void encrypt_provider::create_item_meta(api_meta_map &meta, bool directory,
                                        const api_file &file) {
#ifdef _WIN32
  struct _stat64 buf {};
  _stat64(file.source_path.c_str(), &buf);
#else
  struct stat buf {};
  stat(file.source_path.c_str(), &buf);
#endif

  meta[META_ACCESSED] = std::to_string(file.accessed_date);
#ifdef _WIN32
  meta[META_ATTRIBUTES] =
      std::to_string(::GetFileAttributesA(file.source_path.c_str()));
#endif
#ifdef __APPLE__
  meta[META_BACKUP];
#endif
  meta[META_CHANGED] = std::to_string(file.changed_date);
  meta[META_CREATION] = std::to_string(file.creation_date);
  meta[META_DIRECTORY] = utils::string::from_bool(directory);
  meta[META_GID] = std::to_string(buf.st_gid);
  meta[META_MODE] = std::to_string(buf.st_mode);
  meta[META_MODIFIED] = std::to_string(file.modified_date);
#ifdef __APPLE__
  meta[META_OSXFLAGS];
#endif
  meta[META_SIZE] = std::to_string(file.file_size);
  meta[META_SOURCE] = file.source_path;
  meta[META_UID] = std::to_string(buf.st_uid);
  meta[META_WRITTEN] = std::to_string(file.modified_date);
}

auto encrypt_provider::create_directory(const std::string &api_path,
                                        api_meta_map & /*meta*/) -> api_error {
  if (api_path == "/") {
    return api_error::success;
  }

  return api_error::not_implemented;
}

auto encrypt_provider::get_api_path_from_source(const std::string &source_path,
                                                std::string &api_path) const
    -> api_error {
  try {
    std::string api_path_data{};
    db_->Get(rocksdb::ReadOptions(), file_family_, source_path, &api_path_data);
    if (not api_path_data.empty()) {
      api_path = json::parse(api_path_data).at("api_path").get<std::string>();
      return api_error::success;
    }

    std::string dir_api_path{};
    db_->Get(rocksdb::ReadOptions(), dir_family_, source_path, &dir_api_path);
    if (dir_api_path.empty()) {
      return api_error::item_not_found;
    }

    api_path = dir_api_path;
    return api_error::success;
  } catch (const std::exception &ex) {
    utils::error::raise_error(__FUNCTION__, ex, source_path,
                              "failed to get api path from source path");
  }

  return api_error::error;
}

auto encrypt_provider::get_directory_item_count(
    const std::string &api_path) const -> std::uint64_t {
  std::string source_path{};
  db_->Get(rocksdb::ReadOptions(), source_family_, api_path, &source_path);
  if (source_path.empty()) {
    return 0U;
  }

  std::string dir_api_path{};
  db_->Get(rocksdb::ReadOptions(), dir_family_, source_path, &dir_api_path);
  if (dir_api_path.empty()) {
    return 0U;
  }

  const auto cfg = config_.get_encrypt_config();

  std::uint64_t count{};
  try {
    for ([[maybe_unused]] const auto &dir_entry :
         std::filesystem::directory_iterator(source_path)) {
      count++;
    }
  } catch (const std::exception &ex) {
    utils::error::raise_error(__FUNCTION__, ex, cfg.path,
                              "failed to get directory item count");
    return 0U;
  }

  return count;
}

auto encrypt_provider::get_directory_items(const std::string &api_path,
                                           directory_item_list &list) const
    -> api_error {
  bool exists{};
  auto res = is_file(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    return api_error::item_exists;
  }

  std::string source_path{};
  db_->Get(rocksdb::ReadOptions(), source_family_, api_path, &source_path);
  if (source_path.empty()) {
    return api_error::directory_not_found;
  }

  std::string dir_api_path{};
  db_->Get(rocksdb::ReadOptions(), dir_family_, source_path, &dir_api_path);
  if (dir_api_path.empty()) {
    return api_error::directory_not_found;
  }

  try {
    for (const auto &dir_entry :
         std::filesystem::directory_iterator(source_path)) {
      try {
        std::string api_path{};
        if (dir_entry.is_directory()) {
          db_->Get(rocksdb::ReadOptions(), dir_family_,
                   dir_entry.path().string(), &api_path);
          if (api_path.empty()) {
            const auto cfg = config_.get_encrypt_config();
            for (const auto &child_dir_entry :
                 std::filesystem::directory_iterator(dir_entry.path())) {
              if (process_directory_entry(child_dir_entry, cfg, api_path)) {
                api_path = utils::path::get_parent_api_path(api_path);
                break;
              }
            }

            if (api_path.empty()) {
              continue;
            }
          }
        } else {
          std::string api_path_data{};
          db_->Get(rocksdb::ReadOptions(), file_family_,
                   dir_entry.path().string(), &api_path_data);
          if (api_path_data.empty()) {
            const auto cfg = config_.get_encrypt_config();
            if (not process_directory_entry(dir_entry, cfg, api_path)) {
              continue;
            }
          } else {
            api_path =
                json::parse(api_path_data).at("api_path").get<std::string>();
          }
        }

        auto file = create_api_file(api_path, dir_entry.is_directory(),
                                    dir_entry.path().string());

        directory_item di{};
        di.api_parent = file.api_parent;
        di.api_path = file.api_path;
        di.directory = dir_entry.is_directory();
        di.resolved = true;
        di.size = file.file_size;
        create_item_meta(di.meta, di.directory, file);

        list.emplace_back(std::move(di));
      } catch (const std::exception &ex) {
        utils::error::raise_error(__FUNCTION__, ex, dir_entry.path().string(),
                                  "failed to process directory item");
      }
    }
  } catch (const std::exception &ex) {
    utils::error::raise_error(__FUNCTION__, ex, source_path,
                              "failed to get directory items");
    return api_error::error;
  }

  std::sort(list.begin(), list.end(), [](const auto &a, const auto &b) -> bool {
    return (a.directory && not b.directory) ||
           (not(b.directory && not a.directory) &&
            (a.api_path.compare(b.api_path) < 0));
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

auto encrypt_provider::get_file(const std::string &api_path,
                                api_file &file) const -> api_error {
  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    return api_error::directory_exists;
  }

  std::string source_path{};
  db_->Get(rocksdb::ReadOptions(), source_family_, api_path, &source_path);
  if (source_path.empty()) {
    return api_error::item_not_found;
  }

  file = create_api_file(api_path, false, source_path);
  return api_error::success;
}

auto encrypt_provider::process_directory_entry(
    const std::filesystem::directory_entry &dir_entry,
    const encrypt_config &cfg, std::string &api_path) const -> bool {
  if (dir_entry.is_regular_file() && not dir_entry.is_symlink() &&
      not dir_entry.is_directory()) {
    const auto relative_path = dir_entry.path().lexically_relative(cfg.path);

    std::string api_path_data{};
    db_->Get(rocksdb::ReadOptions(), file_family_, dir_entry.path().string(),
             &api_path_data);

    std::string api_parent{};
    db_->Get(rocksdb::ReadOptions(), dir_family_,
             dir_entry.path().parent_path().string(), &api_parent);

    if (api_path_data.empty() || api_parent.empty()) {
      stop_type stop_requested = false;
      utils::encryption::encrypting_reader reader(
          relative_path.filename().string(), dir_entry.path().string(),
          stop_requested, cfg.encryption_token,
          relative_path.parent_path().string());
      if (api_parent.empty()) {
        auto encrypted_parts =
            utils::string::split(reader.get_encrypted_file_path(), '/', false);

        std::size_t idx{1U};

        std::string current_source_path{cfg.path};
        std::string current_encrypted_path{};
        for (const auto &part : relative_path.parent_path()) {
          if (part.string() == "/") {
            continue;
          }

          current_source_path =
              utils::path::combine(current_source_path, {part.string()});

          std::string parent_api_path{};
          db_->Get(rocksdb::ReadOptions(), dir_family_, current_source_path,
                   &parent_api_path);
          if (parent_api_path.empty()) {
            parent_api_path = utils::path::create_api_path(
                current_encrypted_path + '/' + encrypted_parts[idx]);
            db_->Put(rocksdb::WriteOptions(), dir_family_, current_source_path,
                     parent_api_path);
            db_->Put(rocksdb::WriteOptions(), source_family_, parent_api_path,
                     current_source_path);
            event_system::instance().raise<filesystem_item_added>(
                parent_api_path,
                utils::path::get_parent_api_path(parent_api_path), true);
          } else {
            encrypted_parts[idx] =
                utils::string::split(parent_api_path, '/', false)[idx];
          }

          current_encrypted_path = utils::path::create_api_path(
              current_encrypted_path + '/' + encrypted_parts[idx++]);
        }

        api_parent = current_encrypted_path;
      }

      if (api_path_data.empty()) {
        api_path = utils::path::create_api_path(
            api_parent + "/" + reader.get_encrypted_file_name());

        auto iv_list = reader.get_iv_list();
        json data = {
            {"api_path", api_path},
            {"iv_list", iv_list},
            {"original_file_size", dir_entry.file_size()},
        };
        db_->Put(rocksdb::WriteOptions(), file_family_,
                 dir_entry.path().string(), data.dump());
        db_->Put(rocksdb::WriteOptions(), source_family_, api_path,
                 dir_entry.path().string());
        event_system::instance().raise<filesystem_item_added>(
            api_path, api_parent, false);
      } else {
        api_path = json::parse(api_path_data)["api_path"].get<std::string>();
      }
    } else {
      api_path = json::parse(api_path_data)["api_path"].get<std::string>();
    }

    return true;
  }

  return false;
}

auto encrypt_provider::get_file_list(api_file_list &list) const -> api_error {
  const auto cfg = config_.get_encrypt_config();

  try {
    for (const auto &dir_entry :
         std::filesystem::recursive_directory_iterator(cfg.path)) {
      std::string api_path{};
      if (process_directory_entry(dir_entry, cfg, api_path)) {
        list.emplace_back(create_api_file(api_path, dir_entry.is_directory(),
                                          dir_entry.path().string()));
      }
    }

    return api_error::success;
  } catch (const std::exception &ex) {
    utils::error::raise_error(__FUNCTION__, ex, cfg.path,
                              "failed to get file list");
  }

  return api_error::error;
}

auto encrypt_provider::get_file_size(const std::string &api_path,
                                     std::uint64_t &file_size) const
    -> api_error {
  std::string source_path{};
  db_->Get(rocksdb::ReadOptions(), source_family_, api_path, &source_path);
  if (source_path.empty()) {
    return api_error::item_not_found;
  }

  try {
    file_size = utils::encryption::encrypting_reader::calculate_encrypted_size(
        source_path);
    return api_error::success;
  } catch (const std::exception &ex) {
    utils::error::raise_error(__FUNCTION__, ex, api_path,
                              "failed to get file size");
  }

  return api_error::error;
}

auto encrypt_provider::get_filesystem_item(const std::string &api_path,
                                           bool directory,
                                           filesystem_item &fsi) const
    -> api_error {
  std::string source_path{};
  db_->Get(rocksdb::ReadOptions(), source_family_, api_path, &source_path);
  if (source_path.empty()) {
    return api_error::item_not_found;
  }

  if (directory) {
    std::string api_path{};
    db_->Get(rocksdb::ReadOptions(), dir_family_, source_path, &api_path);
    if (api_path.empty()) {
      return api_error::item_not_found;
    }
    fsi.api_parent = utils::path::get_parent_api_path(api_path);
    fsi.api_path = api_path;
    fsi.directory = true;
    fsi.size = 0U;
    fsi.source_path = source_path;
    return api_error::success;
  }

  std::string api_path_data{};
  db_->Get(rocksdb::ReadOptions(), file_family_, source_path, &api_path_data);
  if (api_path_data.empty()) {
    return api_error::item_not_found;
  }

  auto data = json::parse(api_path_data);
  fsi.api_path = data["api_path"].get<std::string>();
  fsi.api_parent = utils::path::get_parent_api_path(fsi.api_path);
  fsi.directory = false;
  fsi.size = utils::encryption::encrypting_reader::calculate_encrypted_size(
      source_path);
  fsi.source_path = source_path;

  return api_error::success;
}

auto encrypt_provider::get_filesystem_item_from_source_path(
    const std::string &source_path, filesystem_item &fsi) const -> api_error {
  std::string api_path{};
  auto res = get_api_path_from_source(source_path, api_path);
  if (res != api_error::success) {
    return res;
  }

  bool exists{};
  res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    return api_error::directory_exists;
  }

  return get_filesystem_item(api_path, false, fsi);
}

auto encrypt_provider::get_filesystem_item_and_file(const std::string &api_path,
                                                    api_file &file,
                                                    filesystem_item &fsi) const
    -> api_error {
  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }
  if (exists) {
    return api_error::directory_exists;
  }

  auto ret = get_filesystem_item(api_path, exists, fsi);
  if (ret != api_error::success) {
    return ret;
  }

  file = create_api_file(api_path, false, fsi.source_path);
  return api_error::success;
}

auto encrypt_provider::get_pinned_files() const -> std::vector<std::string> {
  return {};
}

auto encrypt_provider::get_item_meta(const std::string &api_path,
                                     api_meta_map &meta) const -> api_error {
  std::string source_path{};
  db_->Get(rocksdb::ReadOptions(), source_family_, api_path, &source_path);
  if (source_path.empty()) {
    return api_error::item_not_found;
  }

  bool exists{};
  auto res = is_directory(api_path, exists);
  if (res != api_error::success) {
    return res;
  }

  auto file = create_api_file(api_path, exists, source_path);
  create_item_meta(meta, exists, file);
  return api_error::success;
}

auto encrypt_provider::get_item_meta(const std::string &api_path,
                                     const std::string &key,
                                     std::string &value) const -> api_error {
  api_meta_map meta{};
  auto ret = get_item_meta(api_path, meta);
  if (ret != api_error::success) {
    return ret;
  }

  value = meta[key];
  return api_error::success;
}

auto encrypt_provider::get_total_drive_space() const -> std::uint64_t {
  const auto cfg = config_.get_encrypt_config();
  return utils::file::get_total_drive_space(cfg.path);
}

auto encrypt_provider::get_total_item_count() const -> std::uint64_t {
  std::uint64_t ret{};

  auto iterator = std::unique_ptr<rocksdb::Iterator>(
      db_->NewIterator(rocksdb::ReadOptions(), source_family_));
  for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
    ret++;
  }

  return ret;
}

auto encrypt_provider::get_used_drive_space() const -> std::uint64_t {
  const auto cfg = config_.get_encrypt_config();
  return get_total_drive_space() - utils::file::get_free_drive_space(cfg.path);
}

auto encrypt_provider::is_directory(const std::string &api_path,
                                    bool &exists) const -> api_error {
  std::string source_path{};
  db_->Get(rocksdb::ReadOptions(), source_family_, api_path, &source_path);
  if (source_path.empty()) {
    exists = false;
    return api_error::success;
  }

  exists = utils::file::is_directory(source_path);
  return api_error::success;
}

auto encrypt_provider::is_file(const std::string &api_path, bool &exists) const
    -> api_error {
  std::string source_path{};
  db_->Get(rocksdb::ReadOptions(), source_family_, api_path, &source_path);
  if (source_path.empty()) {
    exists = false;
    return api_error::success;
  }

  exists = utils::file::is_file(source_path);
  return api_error::success;
}

auto encrypt_provider::is_file_writeable(const std::string & /*api_path*/) const
    -> bool {
  return false;
}

auto encrypt_provider::is_online() const -> bool {
  return std::filesystem::exists(config_.get_encrypt_config().path);
}

auto encrypt_provider::is_rename_supported() const -> bool { return false; }

auto encrypt_provider::read_file_bytes(const std::string &api_path,
                                       std::size_t size, std::uint64_t offset,
                                       data_buffer &data,
                                       stop_type &stop_requested) -> api_error {
  std::string source_path{};
  db_->Get(rocksdb::ReadOptions(), source_family_, api_path, &source_path);
  if (source_path.empty()) {
    return api_error::item_not_found;
  }

  std::string api_path_data{};
  db_->Get(rocksdb::ReadOptions(), file_family_, source_path, &api_path_data);
  if (api_path_data.empty()) {
    return api_error::item_not_found;
  }

  std::uint64_t file_size{};
  if (not utils::file::get_file_size(source_path, file_size)) {
    return api_error::os_error;
  }

  std::vector<
      std::array<unsigned char, crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>>
      iv_list{};

  const auto cfg = config_.get_encrypt_config();

  unique_recur_mutex_lock reader_lookup_lock(reader_lookup_mtx_);

  auto file_data = json::parse(api_path_data);
  if (file_data.at("original_file_size").get<std::uint64_t>() != file_size) {
    const auto relative_path =
        std::filesystem::path(source_path).lexically_relative(cfg.path);

    auto ri = std::make_shared<reader_info>();
    ri->reader = std::make_unique<utils::encryption::encrypting_reader>(
        relative_path.filename().string(), source_path, stop_requested,
        cfg.encryption_token, relative_path.parent_path().string());
    reader_lookup_[source_path] = ri;
    iv_list = ri->reader->get_iv_list();

    file_data["original_file_size"] = file_size;
    file_data["iv_list"] = iv_list;
    auto res = db_->Put(rocksdb::WriteOptions(), file_family_, source_path,
                        file_data.dump());
    if (not res.ok()) {
      utils::error::raise_error(__FUNCTION__, res.code(), source_path,
                                "failed to update meta db");
      return api_error::error;
    }
  } else {
    iv_list =
        file_data["iv_list"]
            .get<std::vector<
                std::array<unsigned char,
                           crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>>>();
    if (reader_lookup_.find(source_path) == reader_lookup_.end()) {
      auto ri = std::make_shared<reader_info>();
      ri->reader = std::make_unique<utils::encryption::encrypting_reader>(
          api_path, source_path, stop_requested, cfg.encryption_token,
          std::move(iv_list));
      reader_lookup_[source_path] = ri;
    }
  }

  if (file_size == 0U || size == 0U) {
    return api_error::success;
  }

  auto ri = reader_lookup_.at(source_path);
  ri->last_access_time = std::chrono::system_clock::now();
  reader_lookup_lock.unlock();

  mutex_lock reader_lock(ri->reader_mtx);
  ri->reader->set_read_position(offset);
  data.resize(size);

  const auto res = ri->reader->reader_function(data.data(), 1u, data.size(),
                                               ri->reader.get());
  if (res == 0) {
    return api_error::os_error;
  }

  return api_error::success;
}

void encrypt_provider::remove_deleted_files() {
  struct removed_item {
    std::string api_path{};
    bool directory{};
    std::string source_path{};
  };

  std::vector<removed_item> removed_list{};
  auto iterator = std::unique_ptr<rocksdb::Iterator>(
      db_->NewIterator(rocksdb::ReadOptions(), source_family_));
  for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
    auto source_path = iterator->value().ToString();
    if (not std::filesystem::exists(source_path)) {
      auto api_path =
          utils::string::split(iterator->key().ToString(), '|', false)[1U];

      std::string value{};
      db_->Get(rocksdb::ReadOptions(), file_family_, source_path, &value);

      removed_list.emplace_back(
          removed_item{api_path, value.empty(), source_path});
    }
  }

  for (const auto &item : removed_list) {
    if (not item.directory) {
      db_->Delete(rocksdb::WriteOptions(), source_family_, item.api_path);
      db_->Delete(rocksdb::WriteOptions(), file_family_, item.source_path);
      event_system::instance().raise<file_removed_externally>(item.api_path,
                                                              item.source_path);
    }
  }

  for (const auto &item : removed_list) {
    if (item.directory) {
      db_->Delete(rocksdb::WriteOptions(), source_family_, item.api_path);
      db_->Delete(rocksdb::WriteOptions(), dir_family_, item.source_path);
      event_system::instance().raise<directory_removed_externally>(
          item.api_path, item.source_path);
    }
  }
}

auto encrypt_provider::start(api_item_added_callback /*api_item_added*/,
                             i_file_manager * /*fm*/) -> bool {
  if (not is_online()) {
    return false;
  }

  auto families = std::vector<rocksdb::ColumnFamilyDescriptor>();
  families.emplace_back(rocksdb::kDefaultColumnFamilyName,
                        rocksdb::ColumnFamilyOptions());
  families.emplace_back("dir", rocksdb::ColumnFamilyOptions());
  families.emplace_back("file", rocksdb::ColumnFamilyOptions());
  families.emplace_back("source", rocksdb::ColumnFamilyOptions());

  auto handles = std::vector<rocksdb::ColumnFamilyHandle *>();

  utils::db::create_rocksdb(config_, DB_NAME, families, handles, db_);

  std::size_t idx{};
  dir_family_ = handles[idx++];
  file_family_ = handles[idx++];
  source_family_ = handles[idx++];

  const auto cfg = config_.get_encrypt_config();

  std::string source_path{};
  db_->Get(rocksdb::ReadOptions(), source_family_, "/", &source_path);
  if (source_path.empty()) {
    db_->Put(rocksdb::WriteOptions(), source_family_, "/", cfg.path);
    source_path = cfg.path;
  }

  std::string dir_api_path{};
  db_->Get(rocksdb::ReadOptions(), dir_family_, source_path, &dir_api_path);
  if (dir_api_path.empty()) {
    db_->Put(rocksdb::WriteOptions(), dir_family_, source_path, "/");
  }

  polling::instance().set_callback({"check_deleted", polling::frequency::low,
                                    [this]() { remove_deleted_files(); }});

  event_system::instance().raise<service_started>("encrypt_provider");
  return true;
}

void encrypt_provider::stop() {
  event_system::instance().raise<service_shutdown_begin>("encrypt_provider");
  polling::instance().remove_callback("check_deleted");
  db_.reset();
  event_system::instance().raise<service_shutdown_end>("encrypt_provider");
}
} // namespace repertory
