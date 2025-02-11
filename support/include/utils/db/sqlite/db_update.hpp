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
#ifndef REPERTORY_INCLUDE_UTILS_DB_SQLITE_DB_UPDATE_HPP_
#define REPERTORY_INCLUDE_UTILS_DB_SQLITE_DB_UPDATE_HPP_
#if defined(PROJECT_ENABLE_SQLITE)

#include "utils/db/sqlite/db_common.hpp"

#include "utils/db/sqlite/db_where_t.hpp"

namespace repertory::utils::db::sqlite {
class db_update final {
public:
  struct context final : db_context_t {
    context(sqlite3 *db3_, std::string table_name_)
        : db_context_t(db3_, table_name_) {}

    struct db_update_op_t final {
      std::shared_ptr<context> ctx;

      [[nodiscard]] auto dump() const -> std::string;

      [[nodiscard]] auto go() const -> db_result;

      [[nodiscard]] auto limit(std::int32_t value) -> db_update_op_t;

      [[nodiscard]] auto order_by(std::string column_name, bool ascending)

          -> db_update_op_t;
    };

    using w_t = db_where_t<context, db_update_op_t>;
    using wd_t = where_data_t<w_t>;

    std::map<std::string, db_types_t> column_values;
    std::optional<std::int32_t> limit;
    std::optional<std::pair<std::string, bool>> order_by;

    std::unique_ptr<wd_t> where_data;
  };

public:
  db_update(sqlite3 &db3, std::string table_name)
      : ctx_(std::make_shared<context>(&db3, table_name)) {}

  db_update(std::shared_ptr<context> ctx) : ctx_(std::move(ctx)) {}

private:
  std::shared_ptr<context> ctx_;

public:
  [[nodiscard]] auto column_value(std::string column_name, db_types_t value)
      -> db_update;

  [[nodiscard]] auto dump() const -> std::string;

  [[nodiscard]] auto go() const -> db_result;

  [[nodiscard]] auto group(context::w_t::group_func_t func)
      -> context::w_t::wn_t;

  [[nodiscard]] auto limit(std::int32_t value) -> db_update;

  [[nodiscard]] auto order_by(std::string column_name, bool ascending)
      -> db_update;

  [[nodiscard]] auto where(std::string column_name) const -> context::w_t::cn_t;
};
} // namespace repertory::utils::db::sqlite

#endif // defined(PROJECT_ENABLE_SQLITE)
#endif // REPERTORY_INCLUDE_UTILS_DB_SQLITE_DB_UPDATE_HPP_
