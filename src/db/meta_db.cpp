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
#include "db/meta_db.hpp"
#include "types/repertory.hpp"
#include "types/startup_exception.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
meta_db::meta_db(const app_config &config) {
  const auto create_resources = [this, &config](const std::string &name) {
    auto families = std::vector<rocksdb::ColumnFamilyDescriptor>();
    families.emplace_back(rocksdb::ColumnFamilyDescriptor{rocksdb::kDefaultColumnFamilyName,
                                                          rocksdb::ColumnFamilyOptions()});
    families.emplace_back(
        rocksdb::ColumnFamilyDescriptor{"source", rocksdb::ColumnFamilyOptions()});
    families.emplace_back(rocksdb::ColumnFamilyDescriptor{"keys", rocksdb::ColumnFamilyOptions()});

    auto handles = std::vector<rocksdb::ColumnFamilyHandle *>();
    utils::db::create_rocksdb(config, name, families, handles, db_);
    default_family_.reset(handles[0u]);
    source_family_.reset(handles[1u]);
    keys_family_.reset(handles[2u]);
  };
  create_resources(METADB_NAME);
}

meta_db::~meta_db() { release_resources(); }

void meta_db::release_resources() {
  default_family_.reset();
  source_family_.reset();
  keys_family_.reset();
  db_.reset();
}

std::shared_ptr<rocksdb::Iterator> meta_db::create_iterator(const bool &source_family) {
  return std::shared_ptr<rocksdb::Iterator>(db_->NewIterator(
      rocksdb::ReadOptions(), source_family ? source_family_.get() : default_family_.get()));
}

api_error meta_db::get_api_path_from_key(const std::string &key, std::string &api_path) const {
  auto ret = api_error::item_not_found;
  if (not key.empty()) {
    if (db_->Get(rocksdb::ReadOptions(), keys_family_.get(), key, &api_path).ok()) {
      ret = api_error::success;
    }
  }

  return ret;
}

api_error meta_db::get_api_path_from_source(const std::string &source_path,
                                            std::string &api_path) const {
  auto ret = api_error::item_not_found;
  if (not source_path.empty()) {
    if (db_->Get(rocksdb::ReadOptions(), source_family_.get(), source_path, &api_path).ok()) {
      ret = api_error::success;
    }
  }

  return ret;
}

api_error meta_db::get_item_meta_json(const std::string &api_path, json &json_data) const {
  auto ret = api_error::item_not_found;

  std::string value;
  if (db_->Get(rocksdb::ReadOptions(), default_family_.get(), api_path, &value).ok()) {
    json_data = json::parse(value);
    ret = api_error::success;
  }

  return ret;
}

api_error meta_db::get_item_meta(const std::string &api_path, api_meta_map &meta) const {
  json json_data;
  const auto ret = get_item_meta_json(api_path, json_data);
  if (ret == api_error::success) {
    for (auto it = json_data.begin(); it != json_data.end(); it++) {
      meta.insert({it.key(), it.value().get<std::string>()});
    }
  }

  return ret;
}

api_error meta_db::get_item_meta(const std::string &api_path, const std::string &key,
                                 std::string &value) const {
  json json_data;
  const auto ret = get_item_meta_json(api_path, json_data);
  if (ret == api_error::success) {
    if (json_data.find(key) != json_data.end()) {
      value = json_data[key].get<std::string>();
    }
  }

  return ret;
}

bool meta_db::get_item_meta_exists(const std::string &api_path) const {
  std::string value;
  return db_->Get(rocksdb::ReadOptions(), api_path, &value).ok();
}

std::vector<std::string> meta_db::get_pinned_files() const {
  std::vector<std::string> ret;

  auto iterator = const_cast<meta_db *>(this)->create_iterator(false);
  for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
    auto api_path = iterator->key().ToString();
    std::string pinned;
    get_item_meta(api_path, META_PINNED, pinned);
    if (not pinned.empty() && utils::string::to_bool(pinned)) {
      ret.emplace_back(api_path);
    }
  }

  return ret;
}

bool meta_db::get_source_path_exists(const std::string &sourceFilePath) const {
  std::string value;
  return db_->Get(rocksdb::ReadOptions(), source_family_.get(), sourceFilePath, &value).ok();
}

void meta_db::remove_item_meta(const std::string &api_path) {
  std::string source;
  get_item_meta(api_path, META_SOURCE, source);

  std::string key;
  get_item_meta(api_path, META_SOURCE, key);

  db_->Delete(rocksdb::WriteOptions(), source_family_.get(), source);
  db_->Delete(rocksdb::WriteOptions(), default_family_.get(), api_path);
  db_->Delete(rocksdb::WriteOptions(), keys_family_.get(), key);
}

api_error meta_db::remove_item_meta(const std::string &api_path, const std::string &key) {
  json json_data;
  auto ret = get_item_meta_json(api_path, json_data);

  if (ret == api_error::success) {
    if ((key == META_KEY) && not json_data[META_KEY].empty()) {
      db_->Delete(rocksdb::WriteOptions(), keys_family_.get(),
                  json_data[META_KEY].get<std::string>());
    }

    json_data.erase(key);
    ret = db_->Put(rocksdb::WriteOptions(), default_family_.get(), api_path, json_data.dump()).ok()
              ? api_error::success
              : api_error::error;
  }

  return ret;
}

api_error meta_db::rename_item_meta(const std::string &source_path,
                                    const std::string &from_api_path,
                                    const std::string &to_api_path) {
  auto ret = api_error::success;

  std::string key;
  get_item_meta(from_api_path, META_KEY, key);

  if (not key.empty()) {
    remove_item_meta(from_api_path, META_KEY);
  }

  std::string value;
  if (db_->Get(rocksdb::ReadOptions(), default_family_.get(), from_api_path, &value).ok()) {
    db_->Delete(rocksdb::WriteOptions(), default_family_.get(), from_api_path);
    db_->Put(rocksdb::WriteOptions(), default_family_.get(), to_api_path, value);
  }

  if (not key.empty()) {
    set_item_meta(to_api_path, META_KEY, key);
  }

  db_->Put(rocksdb::WriteOptions(), source_family_.get(), source_path, to_api_path);
  return ret;
}

api_error meta_db::set_item_meta(const std::string &api_path, const std::string &key,
                                 const std::string &value) {
  if (key == META_KEY) {
    remove_item_meta(api_path, META_KEY);
  }

  json json_data;
  get_item_meta_json(api_path, json_data);

  json_data[key] = value;

  auto ret =
      db_->Put(rocksdb::WriteOptions(), default_family_.get(), api_path, json_data.dump()).ok()
          ? api_error::success
          : api_error::error;
  if ((key == META_KEY) && (ret == api_error::success)) {
    ret = db_->Put(rocksdb::WriteOptions(), keys_family_.get(), value, api_path).ok()
              ? api_error::success
              : api_error::error;
  }

  return ret;
}

api_error meta_db::set_item_meta(const std::string &api_path, const api_meta_map &meta) {
  auto ret = api_error::success;
  auto it = meta.begin();
  for (std::size_t i = 0u; (ret == api_error::success) && (i < meta.size()); i++) {
    ret = set_item_meta(api_path, it->first, it->second);
    it++;
  }

  return ret;
}

api_error meta_db::set_source_path(const std::string &api_path, const std::string &source_path) {
  std::string current_source_path;
  get_item_meta(api_path, META_SOURCE, current_source_path);
  if (not current_source_path.empty()) {
    db_->Delete(rocksdb::WriteOptions(), source_family_.get(), current_source_path);
  }

  auto ret = set_item_meta(api_path, META_SOURCE, source_path);
  if (ret == api_error::success) {
    db_->Put(rocksdb::WriteOptions(), source_family_.get(), source_path, api_path);
  }

  return ret;
}
} // namespace repertory
