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
#ifndef REPERTORY_INCLUDE_DB_IMPL_SQLITE_FILE_DB_HPP_
#define REPERTORY_INCLUDE_DB_IMPL_SQLITE_FILE_DB_HPP_

#include "db/i_file_db.hpp"
#include "utils/db/sqlite/db_common.hpp"

namespace repertory {
class app_config;

class sqlite_file_db final : public i_file_db {
public:
  sqlite_file_db(const app_config &cfg);
  ~sqlite_file_db() override;

  sqlite_file_db(const sqlite_file_db &) = delete;
  sqlite_file_db(sqlite_file_db &&) = delete;
  auto operator=(const sqlite_file_db &) -> sqlite_file_db & = delete;
  auto operator=(sqlite_file_db &&) -> sqlite_file_db & = delete;

private:
  utils::db::sqlite::db3_t db_;

public:
  [[nodiscard]] auto
  add_or_update_directory(const i_file_db::directory_data &data)
      -> api_error override;

  [[nodiscard]] auto add_or_update_file(const i_file_db::file_data &data)
      -> api_error override;

  void clear() override;

  [[nodiscard]] auto count() const -> std::uint64_t override;

  void enumerate_item_list(
      std::function<void(const std::vector<i_file_db::file_info> &)> callback,
      stop_type_callback stop_requested_cb) const override;

  [[nodiscard]] auto get_api_path(std::string_view source_path,
                                  std::string &api_path) const
      -> api_error override;

  [[nodiscard]] auto get_directory_api_path(std::string_view source_path,
                                            std::string &api_path) const
      -> api_error override;

  [[nodiscard]] auto get_directory_data(std::string_view api_path,
                                        i_file_db::directory_data &data) const
      -> api_error override;

  [[nodiscard]] auto get_directory_source_path(std::string_view api_path,
                                               std::string &source_path) const
      -> api_error override;

  [[nodiscard]] auto get_file_api_path(std::string_view source_path,
                                       std::string &api_path) const
      -> api_error override;

  [[nodiscard]] auto get_file_data(std::string_view api_path,
                                   i_file_db::file_data &data) const
      -> api_error override;

  [[nodiscard]] auto get_file_source_path(std::string_view api_path,
                                          std::string &source_path) const
      -> api_error override;

  [[nodiscard]] auto get_item_list(stop_type_callback stop_requested_cb) const
      -> std::vector<i_file_db::file_info> override;

  [[nodiscard]] auto get_source_path(std::string_view api_path,
                                     std::string &source_path) const
      -> api_error override;

  [[nodiscard]] auto remove_item(std::string_view api_path)
      -> api_error override;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_DB_IMPL_SQLITE_FILE_DB_HPP_
