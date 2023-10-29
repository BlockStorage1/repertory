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
#include "db/directory_db.hpp"

#include "utils/path_utils.hpp"

namespace repertory {
void directory_db::directory_tree::add_path(
    const std::string &api_path, const std::vector<std::string> &files,
    rocksdb::DB &db) {
  const auto create_not_found = [&](const auto &create_path) {
    std::string value;
    if (not db.Get(rocksdb::ReadOptions(), create_path, &value).ok()) {
      json directoryData = {
          {"path", api_path},
          {"files", files},
      };
      db.Put(rocksdb::WriteOptions(), create_path, directoryData.dump());
    }
  };

  const auto parts = utils::string::split(api_path, '/', false);
  std::string previous_directory;
  for (const auto &directory_part : parts) {
    if (directory_part.empty()) {
      sub_directory_lookup_["/"];
      previous_directory = "/";
      create_not_found("/");
    } else {
      auto &sub_directories = sub_directory_lookup_[previous_directory];
      if (std::find(sub_directories.begin(), sub_directories.end(),
                    directory_part) == sub_directories.end()) {
        sub_directories.emplace_back(directory_part);
      }
      previous_directory = utils::path::create_api_path(
          utils::path::combine(previous_directory, {directory_part}));
      sub_directory_lookup_[previous_directory];
      create_not_found(previous_directory);
    }
  }
}

auto directory_db::directory_tree::get_count(const std::string &api_path) const
    -> std::size_t {
  return (sub_directory_lookup_.find(api_path) == sub_directory_lookup_.end())
             ? 0
             : sub_directory_lookup_.at(api_path).size();
}

auto directory_db::directory_tree::get_directories() const
    -> std::vector<std::string> {
  std::vector<std::string> ret;
  std::transform(sub_directory_lookup_.begin(), sub_directory_lookup_.end(),
                 std::back_inserter(ret),
                 [](const auto &kv) { return kv.first; });

  return ret;
}

auto directory_db::directory_tree::get_sub_directories(
    const std::string &api_path) const -> std::vector<std::string> {
  std::vector<std::string> ret;
  if (sub_directory_lookup_.find(api_path) != sub_directory_lookup_.end()) {
    const auto &lookup = sub_directory_lookup_.at(api_path);
    std::transform(lookup.begin(), lookup.end(), std::back_inserter(ret),
                   [&api_path](const auto &directory) {
                     return utils::path::create_api_path(
                         utils::path::combine(api_path, {directory}));
                   });
  }
  return ret;
}

auto directory_db::directory_tree::is_directory(
    const std::string &api_path) const -> bool {
  return sub_directory_lookup_.find(api_path) != sub_directory_lookup_.end();
}

void directory_db::directory_tree::remove_directory(const std::string &api_path,
                                                    rocksdb::DB &db,
                                                    bool allow_remove_root) {
  if ((allow_remove_root || (api_path != "/")) && is_directory(api_path)) {
    sub_directory_lookup_.erase(api_path);
    db.Delete(rocksdb::WriteOptions(), api_path);

    const auto parent_api_path = utils::path::get_parent_api_path(api_path);
    const auto parts = utils::string::split(api_path, '/', false);
    utils::remove_element_from(sub_directory_lookup_[parent_api_path],
                               parts[parts.size() - 1]);
  }
}

directory_db::directory_db(const app_config &config) {
  utils::db::create_rocksdb(config, DIRDB_NAME, db_);
  auto iterator = std::unique_ptr<rocksdb::Iterator>(
      db_->NewIterator(rocksdb::ReadOptions()));
  for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
    auto directory_data = json::parse(iterator->value().ToString());
    tree_.add_path(directory_data["path"].get<std::string>(),
                   directory_data["files"].get<std::vector<std::string>>(),
                   *db_);
  }
}

directory_db::~directory_db() { db_.reset(); }

auto directory_db::create_directory(const std::string &api_path,
                                    bool create_always) -> api_error {
  recur_mutex_lock directory_lock(directory_mutex_);
  auto ret = api_error::directory_exists;
  if (not is_directory(api_path)) {
    ret = api_error::directory_not_found;
    if (create_always || (api_path == "/") ||
        is_directory(utils::path::get_parent_api_path(api_path))) {
      ret = api_error::item_exists;
      if (not is_file(api_path)) {
        tree_.add_path(api_path, {}, *db_);
        ret = api_error::success;
      }
    }
  }

  return ret;
}

auto directory_db::create_file(const std::string &api_path) -> api_error {
  recur_mutex_lock directory_lock(directory_mutex_);
  if (is_directory(api_path)) {
    return api_error::directory_exists;
  }

  const auto parent_api_path = utils::path::get_parent_api_path(api_path);
  auto directory_data = get_directory_data(parent_api_path);
  if (directory_data.empty()) {
    return api_error::directory_not_found;
  }

  const auto file_name = utils::path::strip_to_file_name(api_path);
  if (utils::collection_includes(directory_data["files"], file_name)) {
    return api_error::item_exists;
  }

  directory_data["files"].emplace_back(file_name);
  db_->Put(rocksdb::WriteOptions(), parent_api_path, directory_data.dump());
  return api_error::success;
}

auto directory_db::get_directory_data(const std::string &api_path) const
    -> json {
  std::string data;
  db_->Get(rocksdb::ReadOptions(), api_path, &data);
  if (data.empty()) {
    return {};
  }

  return json::parse(data);
}

auto directory_db::get_directory_item_count(const std::string &api_path) const
    -> std::uint64_t {
  auto directory_data = get_directory_data(api_path);
  const auto sub_directory_count = get_sub_directory_count(api_path);
  const auto file_count =
      (directory_data.empty() ? 0 : directory_data["files"].size());
  return sub_directory_count + file_count;
}

auto directory_db::get_file(const std::string &api_path, api_file &file,
                            api_file_provider_callback api_file_provider) const
    -> api_error {
  const auto parent_api_path = utils::path::get_parent_api_path(api_path);
  auto directory_data = get_directory_data(parent_api_path);
  if (not directory_data.empty()) {
    const auto file_name = utils::path::strip_to_file_name(api_path);
    if (utils::collection_includes(directory_data["files"], file_name)) {
      file.api_path = utils::path::create_api_path(
          utils::path::combine(parent_api_path, {file_name})),
      api_file_provider(file);
      return api_error::success;
    }
  }

  return api_error::item_not_found;
}

auto directory_db::get_file_list(
    api_file_list &list, api_file_provider_callback api_file_provider) const
    -> api_error {
  auto iterator = std::unique_ptr<rocksdb::Iterator>(
      db_->NewIterator(rocksdb::ReadOptions()));
  for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
    auto directory_data = json::parse(iterator->value().ToString());
    for (const auto &directory_file : directory_data["files"]) {
      api_file file{
          utils::path::create_api_path(utils::path::combine(
              iterator->key().ToString(), {directory_file.get<std::string>()})),
      };
      api_file_provider(file);
      list.emplace_back(file);
    }
  }

  return api_error::success;
}

auto directory_db::get_sub_directory_count(const std::string &api_path) const
    -> std::size_t {
  recur_mutex_lock directoryLock(directory_mutex_);
  return tree_.get_count(api_path);
}

auto directory_db::get_total_item_count() const -> std::uint64_t {
  unique_recur_mutex_lock directory_lock(directory_mutex_);
  const auto directories = tree_.get_directories();
  directory_lock.unlock();

  return std::accumulate(
      directories.begin(), directories.end(), std::uint64_t(directories.size()),
      [this](std::uint64_t c, const std::string &directory) {
        const auto dirData = this->get_directory_data(directory);
        return c + (dirData.empty() ? 0 : dirData["files"].size());
      });
}

auto directory_db::is_directory(const std::string &api_path) const -> bool {
  recur_mutex_lock directory_lock(directory_mutex_);
  return tree_.is_directory(api_path);
}

auto directory_db::is_file(const std::string &api_path) const -> bool {
  auto directory_data =
      get_directory_data(utils::path::get_parent_api_path(api_path));
  if (directory_data.empty()) {
    return false;
  }

  const auto file_name = utils::path::strip_to_file_name(api_path);
  return utils::collection_includes(directory_data["files"], file_name);
}

void directory_db::populate_directory_files(
    const std::string &api_path, meta_provider_callback meta_provider,
    directory_item_list &list) const {
  auto directory_data = get_directory_data(api_path);
  if (not directory_data.empty()) {
    for (const auto &directory_file : directory_data["files"]) {
      directory_item di{};
      di.api_path = utils::path::create_api_path(
          utils::path::combine(api_path, {directory_file.get<std::string>()}));
      di.directory = false;
      meta_provider(di);
      di.size = utils::string::to_uint64(di.meta[META_SIZE]);
      list.emplace_back(std::move(di));
    }
  }
}

void directory_db::populate_sub_directories(
    const std::string &api_path, meta_provider_callback meta_provider,
    directory_item_list &list) const {
  unique_recur_mutex_lock directory_lock(directory_mutex_);
  const auto directories = tree_.get_sub_directories(api_path);
  directory_lock.unlock();

  std::size_t offset{};
  for (const auto &directory : directories) {
    if (std::find_if(list.begin(), list.end(),
                     [&directory](const auto &di) -> bool {
                       return directory == di.api_path;
                     }) == list.end()) {
      directory_item di{};
      di.api_path = directory;
      di.api_parent = utils::path::get_parent_api_path(directory);
      di.directory = true;
      di.size = get_sub_directory_count(directory);
      meta_provider(di);

      list.insert(list.begin() + static_cast<std::int64_t>(offset++),
                  std::move(di));
    }
  }
}

auto directory_db::remove_directory(const std::string &api_path,
                                    bool allow_remove_root) -> api_error {
  recur_mutex_lock directory_lock(directory_mutex_);
  if ((api_path == "/") && not allow_remove_root) {
    return api_error::access_denied;
  }

  if (is_file(api_path) || not is_directory(api_path)) {
    return api_error::directory_not_found;
  }

  if (tree_.get_count(api_path) == 0) {
    auto directory_data = get_directory_data(api_path);
    if (directory_data.empty() || directory_data["files"].empty()) {
      tree_.remove_directory(api_path, *db_, allow_remove_root);
      return api_error::success;
    }
  }

  return api_error::directory_not_empty;
}

auto directory_db::remove_file(const std::string &api_path) -> bool {
  recur_mutex_lock directory_lock(directory_mutex_);

  if (is_directory(api_path)) {
    return false;
  }

  const auto parent_api_path = utils::path::get_parent_api_path(api_path);
  auto directory_data = get_directory_data(parent_api_path);
  if (directory_data.empty()) {
    return false;
  }

  const auto file_name = utils::path::strip_to_file_name(api_path);
  if (utils::collection_excludes(directory_data["files"], file_name)) {
    return false;
  }

  utils::remove_element_from(directory_data["files"], file_name);
  db_->Put(rocksdb::WriteOptions(), parent_api_path, directory_data.dump());
  return true;
}

auto directory_db::rename_file(const std::string &from_api_path,
                               const std::string &to_api_path) -> api_error {
  recur_mutex_lock directory_lock(directory_mutex_);

  if (is_directory(from_api_path) || is_directory(to_api_path)) {
    return api_error::directory_exists;
  }

  if (not is_directory(utils::path::get_parent_api_path(to_api_path))) {
    return api_error::directory_not_found;
  }

  if (is_file(to_api_path)) {
    return api_error::item_exists;
  }

  if (not remove_file(from_api_path)) {
    return api_error::item_not_found;
  }

  return create_file(to_api_path);
}
} // namespace repertory
