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
#include "db/impl/rdb_file_mgr_db.hpp"

#include "app_config.hpp"
#include "types/startup_exception.hpp"
#include "utils/config.hpp"
#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"
#include "utils/utils.hpp"

namespace repertory {
rdb_file_mgr_db::rdb_file_mgr_db(const app_config &cfg) : cfg_(cfg) {
  create_or_open(false);
}

rdb_file_mgr_db::~rdb_file_mgr_db() { db_.reset(); }

void rdb_file_mgr_db::create_or_open(bool clear) {
  db_.reset();

  auto families = std::vector<rocksdb::ColumnFamilyDescriptor>();
  families.emplace_back(rocksdb::kDefaultColumnFamilyName,
                        rocksdb::ColumnFamilyOptions());
  families.emplace_back("upload_active", rocksdb::ColumnFamilyOptions());
  families.emplace_back("upload", rocksdb::ColumnFamilyOptions());

  auto handles = std::vector<rocksdb::ColumnFamilyHandle *>();
  db_ = utils::create_rocksdb(cfg_, "file_mgr", families, handles, clear);

  std::size_t idx{};
  resume_family_ = handles.at(idx++);
  upload_active_family_ = handles.at(idx++);
  upload_family_ = handles.at(idx++);
}

auto rdb_file_mgr_db::add_resume(const resume_entry &entry) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  return perform_action(
      function_name,
      [this, &entry](rocksdb::Transaction *txn) -> rocksdb::Status {
        return add_resume(entry, txn);
      });
}

auto rdb_file_mgr_db::add_resume(const resume_entry &entry,
                                 rocksdb::Transaction *txn) -> rocksdb::Status {
  REPERTORY_USES_FUNCTION_NAME();

  auto data = json({
      {"chunk_size", entry.chunk_size},
      {"read_state", utils::string::from_dynamic_bitset(entry.read_state)},
      {"source_path", entry.source_path},
  });
  return txn->Put(resume_family_, entry.api_path, data.dump());
}

auto rdb_file_mgr_db::add_upload(const upload_entry &entry) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  return perform_action(
      function_name,
      [this, &entry](rocksdb::Transaction *txn) -> rocksdb::Status {
        return txn->Put(upload_family_,
                        utils::string::zero_pad(std::to_string(++id_), 20U) +
                            '|' + entry.api_path,
                        entry.source_path);
      });
}

auto rdb_file_mgr_db::add_upload_active(const upload_active_entry &entry)
    -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  return perform_action(
      function_name,
      [this, &entry](rocksdb::Transaction *txn) -> rocksdb::Status {
        return txn->Put(upload_active_family_, entry.api_path,
                        entry.source_path);
      });
}

void rdb_file_mgr_db::clear() { create_or_open(true); }

auto rdb_file_mgr_db::create_iterator(rocksdb::ColumnFamilyHandle *family) const
    -> std::shared_ptr<rocksdb::Iterator> {
  return std::shared_ptr<rocksdb::Iterator>(
      db_->NewIterator(rocksdb::ReadOptions(), family));
}

auto rdb_file_mgr_db::get_next_upload() const -> std::optional<upload_entry> {
  auto iter = create_iterator(upload_family_);
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    auto parts = utils::string::split(iter->key().ToString(), '|', false);
    parts.erase(parts.begin());

    auto api_path = utils::string::join(parts, '|');

    return upload_entry{
        api_path,
        iter->value().ToString(),
    };
  }

  return std::nullopt;
}

auto rdb_file_mgr_db::get_resume_list() const -> std::vector<resume_entry> {
  std::vector<resume_entry> ret;

  auto iter = create_iterator(resume_family_);
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    auto data = json::parse(iter->value().ToString());
    ret.emplace_back(resume_entry{
        iter->key().ToString(),
        data.at("chunk_size").get<std::uint64_t>(),
        utils::string::to_dynamic_bitset(
            data.at("read_state").get<std::string>()),
        data.at("source_path").get<std::string>(),
    });
  }

  return ret;
}

auto rdb_file_mgr_db::get_upload(const std::string &api_path) const
    -> std::optional<upload_entry> {
  auto iter = create_iterator(upload_family_);
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    auto parts = utils::string::split(iter->key().ToString(), '|', false);
    parts.erase(parts.begin());

    if (api_path != utils::string::join(parts, '|')) {
      continue;
    }

    return upload_entry{
        api_path,
        iter->value().ToString(),
    };
  }

  return std::nullopt;
}

auto rdb_file_mgr_db::get_upload_active_list() const
    -> std::vector<upload_active_entry> {
  std::vector<upload_active_entry> ret;

  auto iter = create_iterator(upload_active_family_);
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    ret.emplace_back(upload_active_entry{
        iter->key().ToString(),
        iter->value().ToString(),
    });
  }

  return ret;
}

auto rdb_file_mgr_db::perform_action(std::string_view function_name,
                                     std::function<rocksdb::Status()> action)
    -> bool {
  try {
    auto res = action();
    if (not res.ok()) {
      utils::error::raise_error(function_name, res.ToString());
    }

    return res.ok();
  } catch (const std::exception &ex) {
    utils::error::raise_error(function_name, ex);
  }

  return false;
}

auto rdb_file_mgr_db::perform_action(
    std::string_view function_name,
    std::function<rocksdb::Status(rocksdb::Transaction *txn)> action) -> bool {
  std::unique_ptr<rocksdb::Transaction> txn{
      db_->BeginTransaction(rocksdb::WriteOptions{},
                            rocksdb::TransactionOptions{}),
  };

  try {
    auto res = action(txn.get());
    if (res.ok()) {
      auto commit_res = txn->Commit();
      if (commit_res.ok()) {
        return true;
      }

      utils::error::raise_error(function_name,
                                "rocksdb commit failed|" + res.ToString());
      return false;
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
  return false;
}

auto rdb_file_mgr_db::remove_resume(const std::string &api_path) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  return perform_action(
      function_name,
      [this, &api_path](rocksdb::Transaction *txn) -> rocksdb::Status {
        return remove_resume(api_path, txn);
      });
}

auto rdb_file_mgr_db::remove_resume(
    const std::string &api_path, rocksdb::Transaction *txn) -> rocksdb::Status {
  return txn->Delete(resume_family_, api_path);
}

auto rdb_file_mgr_db::remove_upload(const std::string &api_path) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  auto iter = create_iterator(upload_family_);
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    auto parts = utils::string::split(iter->key().ToString(), '|', false);
    parts.erase(parts.begin());

    if (api_path != utils::string::join(parts, '|')) {
      continue;
    }

    return perform_action(
        function_name,
        [this, &iter](rocksdb::Transaction *txn) -> rocksdb::Status {
          return txn->Delete(upload_family_, iter->key());
        });
  }

  return true;
}

auto rdb_file_mgr_db::remove_upload_active(const std::string &api_path)
    -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  return perform_action(
      function_name,
      [this, &api_path](rocksdb::Transaction *txn) -> rocksdb::Status {
        return txn->Delete(upload_active_family_, api_path);
      });
}

auto rdb_file_mgr_db::rename_resume(const std::string &from_api_path,
                                    const std::string &to_api_path) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  bool not_found{false};
  std::string value;
  auto res = perform_action(
      function_name,
      [this, &from_api_path, &not_found, &value]() -> rocksdb::Status {
        auto result = db_->Get(rocksdb::ReadOptions{}, from_api_path, &value);
        not_found = result.IsNotFound();
        return result;
      });
  if (not_found) {
    return true;
  }

  if (not res) {
    return false;
  }

  if (value.empty()) {
    return true;
  }

  auto data = json::parse(value);
  resume_entry entry{
      to_api_path,
      data.at("chunk_size").get<std::uint64_t>(),
      utils::string::to_dynamic_bitset(
          data.at("read_state").get<std::string>()),
      data.at("source_path").get<std::string>(),
  };

  return perform_action(function_name,
                        [this, &entry, &from_api_path](
                            rocksdb::Transaction *txn) -> rocksdb::Status {
                          auto txn_res = remove_resume(from_api_path, txn);
                          if (not txn_res.ok()) {
                            return txn_res;
                          }

                          return add_resume(entry, txn);
                        });
}
} // namespace repertory
