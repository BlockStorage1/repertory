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
#ifndef REPERTORY_INCLUDE_DB_IMPL_RDB_FILE_DB_HPP_
#define REPERTORY_INCLUDE_DB_IMPL_RDB_FILE_DB_HPP_

#include "db/i_file_db.hpp"

namespace repertory {
class app_config;

class rdb_file_db final : public i_file_db {
public:
  rdb_file_db(const app_config &cfg);
  ~rdb_file_db() override;

  rdb_file_db(const rdb_file_db &) = delete;
  rdb_file_db(rdb_file_db &&) = delete;
  auto operator=(const rdb_file_db &) -> rdb_file_db & = delete;
  auto operator=(rdb_file_db &&) -> rdb_file_db & = delete;

private:
  const app_config &cfg_;

private:
  std::unique_ptr<rocksdb::TransactionDB> db_{nullptr};
  rocksdb::ColumnFamilyHandle *directory_family_{};
  rocksdb::ColumnFamilyHandle *file_family_{};
  rocksdb::ColumnFamilyHandle *path_family_{};
  rocksdb::ColumnFamilyHandle *source_family_{};

private:
  void create_or_open(bool clear);

  [[nodiscard]] auto create_iterator(rocksdb::ColumnFamilyHandle *family) const
      -> std::shared_ptr<rocksdb::Iterator>;

  [[nodiscard]] static auto
  perform_action(std::string_view function_name,
                 std::function<rocksdb::Status()> action) -> api_error;

  [[nodiscard]] auto perform_action(
      std::string_view function_name,
      std::function<rocksdb::Status(rocksdb::Transaction *txn)> action)
      -> api_error;

  [[nodiscard]] auto remove_item(const std::string &api_path,
                                 const std::string &source_path,
                                 rocksdb::Transaction *txn) -> rocksdb::Status;

public:
  [[nodiscard]] auto
  add_directory(const std::string &api_path,
                const std::string &source_path) -> api_error override;

  [[nodiscard]] auto
  add_or_update_file(const i_file_db::file_data &data) -> api_error override;

  void clear() override;

  [[nodiscard]] auto count() const -> std::uint64_t override;

  [[nodiscard]] auto
  get_api_path(const std::string &source_path,
               std::string &api_path) const -> api_error override;

  [[nodiscard]] auto
  get_directory_api_path(const std::string &source_path,
                         std::string &api_path) const -> api_error override;

  [[nodiscard]] auto get_directory_source_path(const std::string &api_path,
                                               std::string &source_path) const
      -> api_error override;

  [[nodiscard]] auto
  get_file_api_path(const std::string &source_path,
                    std::string &api_path) const -> api_error override;

  [[nodiscard]] auto
  get_file_data(const std::string &api_path,
                i_file_db::file_data &data) const -> api_error override;

  [[nodiscard]] auto
  get_file_source_path(const std::string &api_path,
                       std::string &source_path) const -> api_error override;

  [[nodiscard]] auto
  get_item_list() const -> std::vector<i_file_db::file_info> override;

  [[nodiscard]] auto
  get_source_path(const std::string &api_path,
                  std::string &source_path) const -> api_error override;

  [[nodiscard]] auto
  remove_item(const std::string &api_path) -> api_error override;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_DB_IMPL_RDB_FILE_DB_HPP_
