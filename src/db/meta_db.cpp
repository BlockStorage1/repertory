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
#include "db/meta_db.hpp"

#include "types/repertory.hpp"
#include "types/startup_exception.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
meta_db::meta_db(const app_config &config) {
  const auto create_resources = [this, &config](const std::string &name) {
    auto families = std::vector<rocksdb::ColumnFamilyDescriptor>();
    families.emplace_back(rocksdb::kDefaultColumnFamilyName,
                          rocksdb::ColumnFamilyOptions());
    families.emplace_back("keys", rocksdb::ColumnFamilyOptions());
    families.emplace_back("source", rocksdb::ColumnFamilyOptions());

    auto handles = std::vector<rocksdb::ColumnFamilyHandle *>();
    utils::db::create_rocksdb(config, name, families, handles, db_);

    std::size_t idx{};
    default_family_ = handles[idx++];
    keys_family_ = handles[idx++];
    source_family_ = handles[idx++];
  };

  create_resources(METADB_NAME);
}

meta_db::~meta_db() { db_.reset(); }

auto meta_db::create_iterator(bool source_family) const
    -> std::shared_ptr<rocksdb::Iterator> {
  return std::shared_ptr<rocksdb::Iterator>(
      db_->NewIterator(rocksdb::ReadOptions(),
                       source_family ? source_family_ : default_family_));
}

auto meta_db::get_api_path_from_key(const std::string &key,
                                    std::string &api_path) const -> api_error {
  if (key.empty()) {
    return api_error::item_not_found;
  }

  return perform_action(__FUNCTION__, [&]() -> rocksdb::Status {
    return db_->Get(rocksdb::ReadOptions(), keys_family_, key, &api_path);
  });
}

auto meta_db::get_api_path_from_source(const std::string &source_path,
                                       std::string &api_path) const
    -> api_error {
  if (source_path.empty()) {
    return api_error::item_not_found;
  }

  return perform_action(__FUNCTION__, [&]() -> rocksdb::Status {
    return db_->Get(rocksdb::ReadOptions(), source_family_, source_path,
                    &api_path);
  });
}

auto meta_db::get_item_meta_json(const std::string &api_path,
                                 json &json_data) const -> api_error {
  std::string value;
  const auto res = perform_action(__FUNCTION__, [&]() -> rocksdb::Status {
    return db_->Get(rocksdb::ReadOptions(), default_family_, api_path, &value);
  });
  if (res != api_error::success) {
    return res;
  }

  json_data = json::parse(value);
  return api_error::success;
}

auto meta_db::get_item_meta(const std::string &api_path,
                            api_meta_map &meta) const -> api_error {
  json json_data;
  const auto ret = get_item_meta_json(api_path, json_data);
  if (ret == api_error::success) {
    for (auto it = json_data.begin(); it != json_data.end(); it++) {
      meta[it.key()] = it.value().get<std::string>();
    }
  }

  return ret;
}

auto meta_db::get_item_meta(const std::string &api_path, const std::string &key,
                            std::string &value) const -> api_error {
  json json_data;
  const auto ret = get_item_meta_json(api_path, json_data);
  if (ret == api_error::success) {
    if (json_data.find(key) != json_data.end()) {
      value = json_data[key].get<std::string>();
    }
  }

  return ret;
}

auto meta_db::get_item_meta_exists(const std::string &api_path) const -> bool {
  std::string value;
  return db_->Get(rocksdb::ReadOptions(), api_path, &value).ok();
}

auto meta_db::get_pinned_files() const -> std::vector<std::string> {
  std::vector<std::string> ret;

  auto iterator = const_cast<meta_db *>(this)->create_iterator(false);
  for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
    auto api_path = iterator->key().ToString();
    std::string pinned;
    const auto res = get_item_meta(api_path, META_PINNED, pinned);
    if ((res == api_error::success) && not pinned.empty() &&
        utils::string::to_bool(pinned)) {
      ret.emplace_back(api_path);
    }
  }

  return ret;
}

auto meta_db::get_source_path_exists(const std::string &source_path) const
    -> bool {
  std::string value;
  return db_->Get(rocksdb::ReadOptions(), source_family_, source_path, &value)
      .ok();
}

auto meta_db::perform_action(
    const std::string &function_name,
    const std::function<rocksdb::Status()> &action) const -> api_error {
  const auto res = action();
  if (res.ok()) {
    return api_error::success;
  }

  if (not res.IsNotFound()) {
    utils::error::raise_error(function_name, res.ToString());
  }

  return res.IsNotFound() ? api_error::item_not_found : api_error::error;
}

auto meta_db::get_total_item_count() const -> std::uint64_t {
  std::uint64_t ret = 0u;
  auto iter = create_iterator(false);
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    ret++;
  }

  return ret;
}

auto meta_db::remove_item_meta(const std::string &api_path) -> api_error {
  json json_data;
  auto res = get_item_meta_json(api_path, json_data);
  if (res != api_error::success) {
    return res == api_error::item_not_found ? api_error::success : res;
  }

  if (not json_data[META_KEY].empty()) {
    if ((res = perform_action(__FUNCTION__, [&]() -> rocksdb::Status {
           return db_->Delete(rocksdb::WriteOptions(), keys_family_,
                              json_data[META_KEY].get<std::string>());
         })) != api_error::success) {
      return res;
    }
  }

  if (not json_data[META_SOURCE].empty()) {
    if ((res = perform_action(__FUNCTION__, [&]() -> rocksdb::Status {
           return db_->Delete(rocksdb::WriteOptions(), source_family_,
                              json_data[META_SOURCE].get<std::string>());
         })) != api_error::success) {
      return res;
    }
  }

  return perform_action(__FUNCTION__, [&]() -> rocksdb::Status {
    return db_->Delete(rocksdb::WriteOptions(), default_family_, api_path);
  });
}

auto meta_db::remove_item_meta(const std::string &api_path,
                               const std::string &key) -> api_error {
  json json_data;
  auto res = get_item_meta_json(api_path, json_data);
  if (res != api_error::success) {
    return res == api_error::item_not_found ? api_error::success : res;
  }

  if ((key == META_KEY) && not json_data[META_KEY].empty()) {
    if ((res = perform_action(__FUNCTION__, [&]() -> rocksdb::Status {
           return db_->Delete(rocksdb::WriteOptions(), keys_family_,
                              json_data[META_KEY].get<std::string>());
         })) != api_error::success) {
      return res;
    }
  }

  if ((key == META_SOURCE) && not json_data[META_SOURCE].empty()) {
    if ((res = perform_action(__FUNCTION__, [&]() -> rocksdb::Status {
           return db_->Delete(rocksdb::WriteOptions(), source_family_,
                              json_data[META_SOURCE].get<std::string>());
         })) != api_error::success) {
      return res;
    }
  }

  json_data.erase(key);
  return perform_action(__FUNCTION__, [&]() -> rocksdb::Status {
    return db_->Put(rocksdb::WriteOptions(), default_family_, api_path,
                    json_data.dump());
  });
}

auto meta_db::rename_item_meta(const std::string &source_path,
                               const std::string &from_api_path,
                               const std::string &to_api_path) -> api_error {
  api_meta_map meta{};
  auto res = get_item_meta(from_api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  if ((res = remove_item_meta(from_api_path)) != api_error::success) {
    return res;
  }

  if (not source_path.empty()) {
    meta[META_SOURCE] = source_path;
  }

  return set_item_meta(to_api_path, meta);
}

auto meta_db::set_item_meta(const std::string &api_path, const std::string &key,
                            const std::string &value) -> api_error {
  if (key == META_SOURCE) {
    return set_source_path(api_path, value);
  }

  if (key == META_KEY) {
    const auto res = remove_item_meta(api_path, META_KEY);
    if ((res != api_error::success) && (res != api_error::item_not_found)) {
      return res;
    }
  }

  auto res = store_item_meta(api_path, key, value);
  if (res != api_error::success) {
    return res;
  }

  if (key == META_KEY) {
    return perform_action(__FUNCTION__, [&]() -> rocksdb::Status {
      return db_->Put(rocksdb::WriteOptions(), keys_family_, value, api_path);
    });
  }

  return api_error::success;
}

auto meta_db::set_item_meta(const std::string &api_path,
                            const api_meta_map &meta) -> api_error {
  auto ret = api_error::success;
  auto it = meta.begin();
  for (std::size_t i = 0u; (ret == api_error::success) && (i < meta.size());
       i++) {
    ret = set_item_meta(api_path, it->first, it->second);
    it++;
  }

  return ret;
}

auto meta_db::set_source_path(const std::string &api_path,
                              const std::string &source_path) -> api_error {
  std::string current_source_path;
  auto res = get_item_meta(api_path, META_SOURCE, current_source_path);
  if ((res != api_error::success) && (res != api_error::item_not_found)) {
    return res;
  }

  // TODO multiple db ops should be in transaction
  if (not current_source_path.empty()) {
    if ((res = perform_action(__FUNCTION__, [&]() -> rocksdb::Status {
           return db_->Delete(rocksdb::WriteOptions(), source_family_,
                              current_source_path);
         })) != api_error::success) {
      return res;
    }
  }

  if (not source_path.empty()) {
    if ((res = perform_action(__FUNCTION__, [&]() -> rocksdb::Status {
           return db_->Put(rocksdb::WriteOptions(), source_family_, source_path,
                           api_path);
         })) != api_error::success) {
      return res;
    }
  }

  return store_item_meta(api_path, META_SOURCE, source_path);
}

auto meta_db::store_item_meta(const std::string &api_path,
                              const std::string &key, const std::string &value)
    -> api_error {
  json json_data;
  auto res = get_item_meta_json(api_path, json_data);
  if ((res != api_error::success) && (res != api_error::item_not_found)) {
    return res;
  }

  json_data[key] = value;
  return perform_action(__FUNCTION__, [&]() -> rocksdb::Status {
    return db_->Put(rocksdb::WriteOptions(), default_family_, api_path,
                    json_data.dump());
  });
}
} // namespace repertory
