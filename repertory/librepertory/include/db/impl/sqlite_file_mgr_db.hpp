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
#ifndef REPERTORY_INCLUDE_DB_IMPL_SQLITE_FILE_MGR_DB_HPP_
#define REPERTORY_INCLUDE_DB_IMPL_SQLITE_FILE_MGR_DB_HPP_

#include "db/i_file_mgr_db.hpp"
#include "utils/db/sqlite/db_common.hpp"

namespace repertory {
class app_config;

class sqlite_file_mgr_db final : public i_file_mgr_db {
public:
  sqlite_file_mgr_db(const app_config &cfg);
  ~sqlite_file_mgr_db() override;

  sqlite_file_mgr_db(const sqlite_file_mgr_db &) = delete;
  sqlite_file_mgr_db(sqlite_file_mgr_db &&) = delete;
  auto operator=(const sqlite_file_mgr_db &) -> sqlite_file_mgr_db & = delete;
  auto operator=(sqlite_file_mgr_db &&) -> sqlite_file_mgr_db & = delete;

private:
  utils::db::sqlite::db3_t db_;

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

#endif // REPERTORY_INCLUDE_DB_IMPL_SQLITE_FILE_MGR_DB_HPP_
