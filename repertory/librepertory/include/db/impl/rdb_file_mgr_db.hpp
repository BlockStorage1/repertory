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
#ifndef REPERTORY_INCLUDE_DB_IMPL_RDB_FILE_MGR_DB_HPP_
#define REPERTORY_INCLUDE_DB_IMPL_RDB_FILE_MGR_DB_HPP_

#include "db/i_file_mgr_db.hpp"

namespace repertory {
class app_config;

class rdb_file_mgr_db final : public i_file_mgr_db {
public:
  rdb_file_mgr_db(const app_config &cfg);
  ~rdb_file_mgr_db() override;

  rdb_file_mgr_db(const rdb_file_mgr_db &) = delete;
  rdb_file_mgr_db(rdb_file_mgr_db &&) = delete;
  auto operator=(const rdb_file_mgr_db &) -> rdb_file_mgr_db & = delete;
  auto operator=(rdb_file_mgr_db &&) -> rdb_file_mgr_db & = delete;

private:
  const app_config &cfg_;

private:
  std::unique_ptr<rocksdb::TransactionDB> db_{nullptr};
  std::atomic<std::uint64_t> id_{0U};
  rocksdb::ColumnFamilyHandle *resume_family_{};
  rocksdb::ColumnFamilyHandle *upload_active_family_{};
  rocksdb::ColumnFamilyHandle *upload_family_{};

private:
  void create_or_open(bool clear);

  [[nodiscard]] auto create_iterator(rocksdb::ColumnFamilyHandle *family) const
      -> std::shared_ptr<rocksdb::Iterator>;

  [[nodiscard]] static auto
  perform_action(std::string_view function_name,
                 std::function<rocksdb::Status()> action) -> bool;

  [[nodiscard]] auto perform_action(
      std::string_view function_name,
      std::function<rocksdb::Status(rocksdb::Transaction *txn)> action) -> bool;

  [[nodiscard]] auto remove_resume(std::string_view api_path,
                                   rocksdb::Transaction *txn)
      -> rocksdb::Status;

  [[nodiscard]] auto add_resume(const resume_entry &entry,
                                rocksdb::Transaction *txn) -> rocksdb::Status;

public:
  [[nodiscard]] auto add_resume(const resume_entry &entry) -> bool override;

  [[nodiscard]] auto add_upload(const upload_entry &entry) -> bool override;

  [[nodiscard]] auto add_upload_active(const upload_active_entry &entry)
      -> bool override;

  void clear() override;

  [[nodiscard]] auto get_next_upload() const
      -> std::optional<upload_entry> override;

  [[nodiscard]] auto get_resume_list() const
      -> std::vector<resume_entry> override;

  [[nodiscard]] auto get_upload(std::string_view api_path) const
      -> std::optional<upload_entry> override;

  [[nodiscard]] auto get_upload_active_list() const
      -> std::vector<upload_active_entry> override;

  [[nodiscard]] auto remove_resume(std::string_view api_path) -> bool override;

  [[nodiscard]] auto remove_upload(std::string_view api_path) -> bool override;

  [[nodiscard]] auto remove_upload_active(std::string_view api_path)
      -> bool override;

  [[nodiscard]] auto rename_resume(std::string_view from_api_path,
                                   std::string_view to_api_path)
      -> bool override;
};

} // namespace repertory

#endif // REPERTORY_INCLUDE_DB_IMPL_RDB_FILE_MGR_DB_HPP_
