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
#ifndef REPERTORY_INCLUDE_DB_IMPL_RDB_META_DB_HPP_
#define REPERTORY_INCLUDE_DB_IMPL_RDB_META_DB_HPP_

#include "db/i_meta_db.hpp"
#include "types/repertory.hpp"

namespace repertory {
class app_config;

class rdb_meta_db final : public i_meta_db {
public:
  rdb_meta_db(const app_config &cfg);
  ~rdb_meta_db() override;

  rdb_meta_db(const rdb_meta_db &) = delete;
  rdb_meta_db(rdb_meta_db &&) = delete;
  auto operator=(const rdb_meta_db &) -> rdb_meta_db & = delete;
  auto operator=(rdb_meta_db &&) -> rdb_meta_db & = delete;

private:
  const app_config &cfg_;

private:
  std::unique_ptr<rocksdb::TransactionDB> db_{nullptr};
  rocksdb::ColumnFamilyHandle *meta_family_{};
  rocksdb::ColumnFamilyHandle *pinned_family_{};
  rocksdb::ColumnFamilyHandle *size_family_{};
  rocksdb::ColumnFamilyHandle *source_family_{};

private:
  [[nodiscard]] auto create_iterator(rocksdb::ColumnFamilyHandle *family) const
      -> std::shared_ptr<rocksdb::Iterator>;

  void create_or_open(bool clear);

  [[nodiscard]] auto get_item_meta_json(const std::string &api_path,
                                        json &json_data) const -> api_error;

  [[nodiscard]] static auto
  perform_action(std::string_view function_name,
                 std::function<rocksdb::Status()> action) -> api_error;

  [[nodiscard]] auto perform_action(
      std::string_view function_name,
      std::function<rocksdb::Status(rocksdb::Transaction *txn)> action)
      -> api_error;

  [[nodiscard]] auto remove_api_path(const std::string &api_path,
                                     const std::string &source_path,
                                     rocksdb::Transaction *txn)
      -> rocksdb::Status;

  [[nodiscard]] auto update_item_meta(const std::string &api_path,
                                      json json_data,
                                      rocksdb::Transaction *base_txn = nullptr,
                                      rocksdb::Status *status = nullptr)
      -> api_error;

public:
  void clear() override;

  void enumerate_api_path_list(
      std::function<void(const std::vector<std::string> &)> callback,
      stop_type_callback stop_requested_cb) const override;

  [[nodiscard]] auto get_api_path(const std::string &source_path,
                                  std::string &api_path) const
      -> api_error override;

  [[nodiscard]] auto get_api_path_list() const
      -> std::vector<std::string> override;

  [[nodiscard]] auto get_item_meta(const std::string &api_path,
                                   api_meta_map &meta) const
      -> api_error override;

  [[nodiscard]] auto get_item_meta(const std::string &api_path,
                                   const std::string &key,
                                   std::string &value) const
      -> api_error override;

  [[nodiscard]] auto get_pinned_files() const
      -> std::vector<std::string> override;

  [[nodiscard]] auto get_total_item_count() const -> std::uint64_t override;

  [[nodiscard]] auto get_total_size() const -> std::uint64_t override;

  void remove_api_path(const std::string &api_path) override;

  [[nodiscard]] auto remove_item_meta(const std::string &api_path,
                                      const std::string &key)
      -> api_error override;

  [[nodiscard]] auto rename_item_meta(const std::string &from_api_path,
                                      const std::string &to_api_path)
      -> api_error override;

  [[nodiscard]] auto set_item_meta(const std::string &api_path,
                                   const std::string &key,
                                   const std::string &value)
      -> api_error override;

  [[nodiscard]] auto set_item_meta(const std::string &api_path,
                                   const api_meta_map &meta)
      -> api_error override;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_DB_IMPL_RDB_META_DB_HPP_
