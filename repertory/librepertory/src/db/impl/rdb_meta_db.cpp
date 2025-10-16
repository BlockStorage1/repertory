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
#include "db/impl/rdb_meta_db.hpp"

#include "app_config.hpp"
#include "types/startup_exception.hpp"
#include "utils/collection.hpp"
#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"
#include "utils/utils.hpp"

namespace repertory {
rdb_meta_db::rdb_meta_db(const app_config &cfg) : cfg_(cfg) {
  create_or_open(false);
}

rdb_meta_db::~rdb_meta_db() { db_.reset(); }

void rdb_meta_db::create_or_open(bool clear) {
  db_.reset();

  auto families = std::vector<rocksdb::ColumnFamilyDescriptor>();
  families.emplace_back(rocksdb::kDefaultColumnFamilyName,
                        rocksdb::ColumnFamilyOptions());
  families.emplace_back("pinned", rocksdb::ColumnFamilyOptions());
  families.emplace_back("size", rocksdb::ColumnFamilyOptions());
  families.emplace_back("source", rocksdb::ColumnFamilyOptions());

  auto handles = std::vector<rocksdb::ColumnFamilyHandle *>();
  db_ = utils::create_rocksdb(cfg_, "provider_meta", families, handles, clear);

  std::size_t idx{};
  meta_family_ = handles.at(idx++);
  pinned_family_ = handles.at(idx++);
  size_family_ = handles.at(idx++);
  source_family_ = handles.at(idx++);
}

void rdb_meta_db::clear() { create_or_open(true); }

auto rdb_meta_db::create_iterator(rocksdb::ColumnFamilyHandle *family) const
    -> std::shared_ptr<rocksdb::Iterator> {
  return std::shared_ptr<rocksdb::Iterator>(
      db_->NewIterator(rocksdb::ReadOptions{}, family));
}

void rdb_meta_db::enumerate_api_path_list(
    std::function<void(const std::vector<std::string> &)> callback,
    stop_type_callback stop_requested_cb) const {
  std::vector<std::string> list{};

  auto iter = create_iterator(meta_family_);
  for (iter->SeekToFirst(); not stop_requested_cb() && iter->Valid();
       iter->Next()) {
    list.push_back(iter->key().ToString());

    if (list.size() < 100U) {
      continue;
    }

    callback(list);
    list.clear();
  }

  if (not list.empty()) {
    callback(list);
  }
}

auto rdb_meta_db::get_api_path(std::string_view source_path,
                               std::string &api_path) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  if (source_path.empty()) {
    return api_error::item_not_found;
  }

  return perform_action(function_name, [&]() -> rocksdb::Status {
    return db_->Get(rocksdb::ReadOptions{}, source_family_, source_path,
                    &api_path);
  });
}

auto rdb_meta_db::get_api_path_list() const -> std::vector<std::string> {
  std::vector<std::string> ret;
  auto iter = create_iterator(meta_family_);
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    ret.push_back(iter->key().ToString());
  }

  return ret;
}

auto rdb_meta_db::get_item_meta_json(std::string_view api_path,
                                     json &json_data) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    json_data.clear();

    {
      std::string value;
      auto res = perform_action(function_name, [&]() -> rocksdb::Status {
        return db_->Get(rocksdb::ReadOptions{}, meta_family_, api_path, &value);
      });
      if (res != api_error::success) {
        return res;
      }

      if (not value.empty()) {
        json_data = json::parse(value);
      }
    }

    {
      std::string value;
      auto res = perform_action(function_name, [&]() -> rocksdb::Status {
        return db_->Get(rocksdb::ReadOptions{}, pinned_family_, api_path,
                        &value);
      });
      if (res != api_error::success) {
        return res;
      }
      if (not value.empty()) {
        json_data[META_PINNED] = value;
      }
    }

    {
      std::string value;
      auto res = perform_action(function_name, [&]() -> rocksdb::Status {
        return db_->Get(rocksdb::ReadOptions{}, size_family_, api_path, &value);
      });
      if (res != api_error::success) {
        return res;
      }
      if (not value.empty()) {
        json_data[META_SIZE] = value;
      }
    }

    return json_data.empty() ? api_error::item_not_found : api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to get item meta");
  }

  return api_error::error;
}

auto rdb_meta_db::get_item_meta(std::string_view api_path,
                                api_meta_map &meta) const -> api_error {
  json json_data;
  auto ret = get_item_meta_json(api_path, json_data);
  if (ret != api_error::success) {
    return ret;
  }

  for (auto it = json_data.begin(); it != json_data.end(); ++it) {
    meta[it.key()] = it.value().get<std::string>();
  }

  return api_error::success;
}

auto rdb_meta_db::get_item_meta(std::string_view api_path, std::string_view key,
                                std::string &value) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  if (key == META_PINNED) {
    return perform_action(function_name, [&]() -> rocksdb::Status {
      return db_->Get(rocksdb::ReadOptions{}, pinned_family_, api_path, &value);
    });
  }

  if (key == META_SIZE) {
    return perform_action(function_name, [&]() -> rocksdb::Status {
      return db_->Get(rocksdb::ReadOptions{}, size_family_, api_path, &value);
    });
  }

  json json_data;
  auto ret = get_item_meta_json(api_path, json_data);
  if (ret != api_error::success) {
    return ret;
  }

  if (json_data.find(key) != json_data.end()) {
    value = json_data[key].get<std::string>();
  }

  return api_error::success;
}

auto rdb_meta_db::get_pinned_files() const -> std::vector<std::string> {
  std::vector<std::string> ret;
  auto iter = create_iterator(pinned_family_);
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    if (not utils::string::to_bool(iter->value().ToString())) {
      continue;
    }

    ret.push_back(iter->key().ToString());
  }

  return ret;
}

auto rdb_meta_db::get_total_item_count() const -> std::uint64_t {
  std::uint64_t ret{};
  auto iter = create_iterator(meta_family_);
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    ++ret;
  }

  return ret;
}

auto rdb_meta_db::get_total_size() const -> std::uint64_t {
  std::uint64_t ret{};
  auto iter = create_iterator(size_family_);
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    ret += utils::string::to_uint64(iter->value().ToString());
  }

  return ret;
}

auto rdb_meta_db::perform_action(std::string_view function_name,
                                 std::function<rocksdb::Status()> action)
    -> api_error {
  auto res = action();
  if (res.ok()) {
    return api_error::success;
  }

  if (not res.IsNotFound()) {
    utils::error::raise_error(function_name, res.ToString());
  }

  return res.IsNotFound() ? api_error::item_not_found : api_error::error;
}

auto rdb_meta_db::perform_action(
    std::string_view function_name,
    std::function<rocksdb::Status(rocksdb::Transaction *txn)> action)
    -> api_error {
  std::unique_ptr<rocksdb::Transaction> txn{
      db_->BeginTransaction(rocksdb::WriteOptions{},
                            rocksdb::TransactionOptions{}),
  };

  try {
    auto res = action(txn.get());
    if (res.ok()) {
      auto commit_res = txn->Commit();
      if (commit_res.ok()) {
        return api_error::success;
      }

      utils::error::raise_error(function_name,
                                "rocksdb commit failed|" + res.ToString());
      return api_error::error;
    }

    utils::error::raise_error(function_name,
                              "rocksdb action failed|" + res.ToString());
  } catch (const std::exception &ex) {
    utils::error::raise_error(function_name, ex,
                              "failed to handle rocksdb action");
  }

  auto rollback_res = txn->Rollback();
  utils::error::raise_error(function_name, "rocksdb rollback failed|" +
                                               rollback_res.ToString());
  return api_error::error;
}

void rdb_meta_db::remove_api_path(std::string_view api_path) {
  REPERTORY_USES_FUNCTION_NAME();

  std::string source_path;
  auto res = get_item_meta(api_path, META_SOURCE, source_path);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(function_name, api_path, res,
                                       "failed to get source path");
  }

  res = perform_action(function_name,
                       [this, &api_path, &source_path](
                           rocksdb::Transaction *txn) -> rocksdb::Status {
                         return remove_api_path(api_path, source_path, txn);
                       });
  if (res != api_error::success) {
    utils::error::raise_api_path_error(function_name, api_path, res,
                                       "failed to remove api path");
  }
}

auto rdb_meta_db::remove_api_path(std::string_view api_path,
                                  std::string_view source_path,
                                  rocksdb::Transaction *txn)
    -> rocksdb::Status {
  auto txn_res = txn->Delete(pinned_family_, api_path);
  if (not txn_res.ok()) {
    return txn_res;
  }

  txn_res = txn->Delete(size_family_, api_path);
  if (not txn_res.ok()) {
    return txn_res;
  }

  if (not source_path.empty()) {
    txn_res = txn->Delete(source_family_, source_path);
    if (not txn_res.ok()) {
      return txn_res;
    }
  }

  return txn->Delete(meta_family_, api_path);
}

auto rdb_meta_db::remove_item_meta(std::string_view api_path,
                                   std::string_view key) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  if (utils::collection::includes(META_USED_NAMES, std::string{key})) {
    utils::error::raise_api_path_error(
        function_name, api_path,
        fmt::format("failed to remove item meta-key is restricted|key|{}",
                    key));
    return api_error::permission_denied;
  }

  json json_data;
  auto res = get_item_meta_json(api_path, json_data);
  if (res != api_error::success) {
    return res;
  }

  json_data.erase(key);
  return update_item_meta(api_path, json_data);
}

auto rdb_meta_db::rename_item_meta(std::string_view from_api_path,
                                   std::string_view to_api_path) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  json json_data;
  auto res = get_item_meta_json(from_api_path, json_data);
  if (res != api_error::success) {
    return res;
  }

  return perform_action(
      function_name, [&](rocksdb::Transaction *txn) -> rocksdb::Status {
        auto txn_res = remove_api_path(
            from_api_path, json_data[META_SOURCE].get<std::string>(), txn);
        if (not txn_res.ok()) {
          return txn_res;
        }

        rocksdb::Status status;
        [[maybe_unused]] auto api_res =
            update_item_meta(to_api_path, json_data, txn, &status);
        return status;
      });
}

auto rdb_meta_db::set_item_meta(std::string_view api_path, std::string_view key,
                                std::string_view value) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  if (key == META_PINNED) {
    return perform_action(function_name,
                          [&](rocksdb::Transaction *txn) -> rocksdb::Status {
                            return txn->Put(pinned_family_, api_path, value);
                          });
  }

  if (key == META_SIZE) {
    return perform_action(function_name,
                          [&](rocksdb::Transaction *txn) -> rocksdb::Status {
                            return txn->Put(size_family_, api_path, value);
                          });
  }

  json json_data;
  auto res = get_item_meta_json(api_path, json_data);
  if (res != api_error::success && res != api_error::item_not_found) {
    return res;
  }

  json_data[key] = value;

  return update_item_meta(api_path, json_data);
}

auto rdb_meta_db::set_item_meta(std::string_view api_path,
                                const api_meta_map &meta) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  json json_data;
  auto res = get_item_meta_json(api_path, json_data);
  if (res != api_error::success && res != api_error::item_not_found) {
    utils::error::raise_api_path_error(function_name, api_path, res,
                                       "failed to get item meta");
  }

  for (const auto &data : meta) {
    json_data[data.first] = data.second;
  }

  return update_item_meta(api_path, json_data);
}

auto rdb_meta_db::update_item_meta(std::string_view api_path, json json_data,
                                   rocksdb::Transaction *base_txn,
                                   rocksdb::Status *status) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not json_data.contains(META_PINNED)) {
      json_data[META_PINNED] = utils::string::from_bool(false);
    }
    if (not json_data.contains(META_SIZE)) {
      json_data[META_SIZE] = "0";
    }
    if (not json_data.contains(META_SOURCE)) {
      json_data[META_SOURCE] = "";
    }

    auto directory =
        utils::string::to_bool(json_data.at(META_DIRECTORY).get<std::string>());

    auto pinned = directory ? false
                            : utils::string::to_bool(
                                  json_data.at(META_PINNED).get<std::string>());
    auto size = directory ? std::uint64_t(0U)
                          : utils::string::to_uint64(
                                json_data.at(META_SIZE).get<std::string>());
    auto source_path = directory ? std::string("")
                                 : json_data.at(META_SOURCE).get<std::string>();

    json_data[META_PINNED] = utils::string::from_bool(pinned);
    json_data[META_SIZE] = std::to_string(size);
    json_data[META_SOURCE] = source_path;

    auto should_del_source{false};
    std::string orig_source_path;
    if (not directory) {
      auto res = get_item_meta(api_path, META_SOURCE, orig_source_path);
      if (res != api_error::success && res != api_error::item_not_found) {
        return res;
      }

      should_del_source =
          not orig_source_path.empty() && orig_source_path != source_path;
    }

    json_data.erase(META_PINNED);
    json_data.erase(META_SIZE);

    const auto set_status = [&status](rocksdb::Status res) -> rocksdb::Status {
      if (status != nullptr) {
        *status = res;
      }

      return res;
    };

    const auto do_transaction =
        [&](rocksdb::Transaction *txn) -> rocksdb::Status {
      if (should_del_source) {
        auto res = set_status(txn->Delete(source_family_, orig_source_path));
        if (not res.ok()) {
          return res;
        }
      }

      auto res = set_status(
          txn->Put(pinned_family_, api_path, utils::string::from_bool(pinned)));
      if (not res.ok()) {
        return res;
      }

      res = set_status(txn->Put(size_family_, api_path, std::to_string(size)));
      if (not res.ok()) {
        return res;
      }

      if (not source_path.empty()) {
        res = set_status(txn->Put(source_family_, source_path, api_path));
        if (not res.ok()) {
          return res;
        }
      }

      return set_status(txn->Put(meta_family_, api_path, json_data.dump()));
    };

    if (base_txn == nullptr) {
      return perform_action(function_name, do_transaction);
    }

    auto res = set_status(do_transaction(base_txn));
    if (res.ok()) {
      return api_error::success;
    }
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to update item meta");
  }

  return api_error::error;
}
} // namespace repertory
