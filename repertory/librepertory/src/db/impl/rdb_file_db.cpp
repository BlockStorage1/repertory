/*
  Copyright <2018-2024> <scott.e.graves@protonmail.com>

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
#include "db/impl/rdb_file_db.hpp"

#include "app_config.hpp"
#include "types/startup_exception.hpp"
#include "utils/config.hpp"
#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"
#include "utils/utils.hpp"

namespace repertory {
rdb_file_db::rdb_file_db(const app_config &cfg) : cfg_(cfg) {
  create_or_open(false);
}

rdb_file_db::~rdb_file_db() { db_.reset(); }

void rdb_file_db::create_or_open(bool clear) {
  db_.reset();

  auto families = std::vector<rocksdb::ColumnFamilyDescriptor>();
  families.emplace_back(rocksdb::kDefaultColumnFamilyName,
                        rocksdb::ColumnFamilyOptions());
  families.emplace_back("file", rocksdb::ColumnFamilyOptions());
  families.emplace_back("path", rocksdb::ColumnFamilyOptions());
  families.emplace_back("source", rocksdb::ColumnFamilyOptions());

  auto handles = std::vector<rocksdb::ColumnFamilyHandle *>();
  db_ = utils::create_rocksdb(cfg_, "file", families, handles, clear);

  std::size_t idx{};
  directory_family_ = handles.at(idx++);
  file_family_ = handles.at(idx++);
  path_family_ = handles.at(idx++);
  source_family_ = handles.at(idx++);
}

auto rdb_file_db::add_directory(const std::string &api_path,
                                const std::string &source_path) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  std::string existing_source_path;
  auto result = get_directory_source_path(api_path, existing_source_path);
  if (result != api_error::success &&
      result != api_error::directory_not_found) {
    return result;
  }

  return perform_action(
      function_name, [&](rocksdb::Transaction *txn) -> rocksdb::Status {
        if (not existing_source_path.empty()) {
          auto res = remove_item(api_path, existing_source_path, txn);
          if (not res.ok() && not res.IsNotFound()) {
            return res;
          }
        }

        auto res = txn->Put(directory_family_, api_path, source_path);
        if (not res.ok()) {
          return res;
        }

        res = txn->Put(path_family_, api_path, source_path);
        if (not res.ok()) {
          return res;
        }

        return txn->Put(source_family_, source_path, api_path);
      });
}

auto rdb_file_db::add_or_update_file(const i_file_db::file_data &data)
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  std::string existing_source_path;
  auto result = get_file_source_path(data.api_path, existing_source_path);
  if (result != api_error::success && result != api_error::item_not_found) {
    return result;
  }

  return perform_action(
      function_name, [&](rocksdb::Transaction *txn) -> rocksdb::Status {
        if (not existing_source_path.empty()) {
          auto res = remove_item(data.api_path, existing_source_path, txn);
          if (not res.ok() && not res.IsNotFound()) {
            return res;
          }
        }

        json json_data = {
            {"file_size", data.file_size},
            {"iv", data.iv_list},
            {"source_path", data.source_path},
        };

        auto res = txn->Put(file_family_, data.api_path, json_data.dump());
        if (not res.ok()) {
          return res;
        }

        res = txn->Put(path_family_, data.api_path, data.source_path);
        if (not res.ok()) {
          return res;
        }

        return txn->Put(source_family_, data.source_path, data.api_path);
      });
}

void rdb_file_db::clear() { create_or_open(true); }

auto rdb_file_db::create_iterator(rocksdb::ColumnFamilyHandle *family) const
    -> std::shared_ptr<rocksdb::Iterator> {
  return std::shared_ptr<rocksdb::Iterator>(
      db_->NewIterator(rocksdb::ReadOptions{}, family));
}

auto rdb_file_db::count() const -> std::uint64_t {
  std::uint64_t ret{};

  auto iter = create_iterator(source_family_);
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    ++ret;
  }

  return ret;
}

auto rdb_file_db::get_api_path(const std::string &source_path,
                               std::string &api_path) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  return perform_action(function_name, [&]() -> rocksdb::Status {
    return db_->Get(rocksdb::ReadOptions{}, source_family_, source_path,
                    &api_path);
  });
}

auto rdb_file_db::get_directory_api_path(
    const std::string &source_path, std::string &api_path) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  auto result = perform_action(function_name, [&]() -> rocksdb::Status {
    auto res = db_->Get(rocksdb::ReadOptions{}, source_family_, source_path,
                        &api_path);
    if (not res.ok()) {
      return res;
    }

    std::string value;
    return db_->Get(rocksdb::ReadOptions{}, directory_family_, api_path,
                    &value);
  });

  if (result != api_error::success) {
    api_path.clear();
  }

  return result == api_error::item_not_found ? api_error::directory_not_found
                                             : result;
}

auto rdb_file_db::get_directory_source_path(
    const std::string &api_path, std::string &source_path) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  auto result = perform_action(function_name, [&]() -> rocksdb::Status {
    return db_->Get(rocksdb::ReadOptions{}, directory_family_, api_path,
                    &source_path);
  });

  return result == api_error::item_not_found ? api_error::directory_not_found
                                             : result;
}

auto rdb_file_db::get_file_api_path(const std::string &source_path,
                                    std::string &api_path) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  auto result = perform_action(function_name, [&]() -> rocksdb::Status {
    auto res = db_->Get(rocksdb::ReadOptions{}, source_family_, source_path,
                        &api_path);
    if (not res.ok()) {
      return res;
    }

    std::string value;
    return db_->Get(rocksdb::ReadOptions{}, file_family_, api_path, &value);
  });

  if (result != api_error::success) {
    api_path.clear();
  }

  return result;
}

auto rdb_file_db::get_file_data(const std::string &api_path,
                                i_file_db::file_data &data) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  auto result = perform_action(function_name, [&]() -> rocksdb::Status {
    std::string value;
    auto res = db_->Get(rocksdb::ReadOptions{}, file_family_, api_path, &value);
    if (not res.ok()) {
      return res;
    }

    auto json_data = json::parse(value);
    data.api_path = api_path;
    data.file_size = json_data.at("file_size").get<std::uint64_t>();
    data.iv_list =
        json_data.at("iv")
            .get<std::vector<
                std::array<unsigned char,
                           crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>>>();
    data.source_path = json_data.at("source_path").get<std::string>();

    return res;
  });

  return result;
}

auto rdb_file_db::get_file_source_path(
    const std::string &api_path, std::string &source_path) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  auto result = perform_action(function_name, [&]() -> rocksdb::Status {
    std::string value;
    auto res = db_->Get(rocksdb::ReadOptions{}, file_family_, api_path, &value);
    if (not res.ok()) {
      return res;
    }

    auto json_data = json::parse(value);
    source_path = json_data.at("source_path").get<std::string>();
    return res;
  });

  return result;
}

auto rdb_file_db::get_item_list() const -> std::vector<i_file_db::file_info> {
  std::vector<i_file_db::file_info> ret{};
  {
    auto iter = create_iterator(directory_family_);
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
      ret.emplace_back(i_file_db::file_info{
          iter->key().ToString(),
          true,
          iter->value().ToString(),
      });
    }
  }

  {
    auto iter = create_iterator(file_family_);
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
      auto json_data = json::parse(iter->value().ToString());
      ret.emplace_back(i_file_db::file_info{
          iter->key().ToString(),
          true,
          json_data.at("source_path").get<std::string>(),
      });
    }
  }

  return ret;
}

auto rdb_file_db::get_source_path(const std::string &api_path,
                                  std::string &source_path) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  return perform_action(function_name, [&]() -> rocksdb::Status {
    return db_->Get(rocksdb::ReadOptions{}, path_family_, api_path,
                    &source_path);
  });
}

auto rdb_file_db::perform_action(std::string_view function_name,
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

auto rdb_file_db::perform_action(
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

auto rdb_file_db::remove_item(const std::string &api_path) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  std::string source_path;
  auto res = get_source_path(api_path, source_path);
  if (res != api_error::success) {
    return res;
  }

  return perform_action(function_name,
                        [&](rocksdb::Transaction *txn) -> rocksdb::Status {
                          return remove_item(api_path, source_path, txn);
                        });
}

auto rdb_file_db::remove_item(const std::string &api_path,
                              const std::string &source_path,
                              rocksdb::Transaction *txn) -> rocksdb::Status {
  auto res = txn->Delete(source_family_, source_path);
  if (not res.ok()) {
    return res;
  }

  res = txn->Delete(path_family_, api_path);
  if (not res.ok()) {
    return res;
  }

  res = txn->Delete(directory_family_, api_path);
  if (not res.ok()) {
    return res;
  }

  return txn->Delete(file_family_, api_path);
}
} // namespace repertory
