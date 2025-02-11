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
#if defined(PROJECT_ENABLE_SQLITE)

#include "utils/db/sqlite/db_delete.hpp"

namespace repertory::utils::db::sqlite {
auto db_delete::context::db_delete_op_t::dump() const -> std::string {
  return db_delete{ctx}.dump();
}

auto db_delete::context::db_delete_op_t::go() const -> db_result {
  return db_delete{ctx}.go();
}

auto db_delete::dump() const -> std::string {
  std::stringstream query;
  query << "DELETE FROM \"" << ctx_->table_name << "\"";

  if (ctx_->where_data) {
    std::int32_t idx{};
    query << " WHERE " << ctx_->where_data->base.dump(idx);
  }

  query << ';';

  return query.str();
}

auto db_delete::go() const -> db_result {
  sqlite3_stmt *stmt_ptr{nullptr};
  auto query_str = dump();
  auto res =
      sqlite3_prepare_v2(ctx_->db3, query_str.c_str(), -1, &stmt_ptr, nullptr);

  auto stmt = db3_stmt_t{
      stmt_ptr,
      sqlite3_statement_deleter(),
  };

  if (res != SQLITE_OK) {
    return {std::move(stmt), res};
  }

  if (not ctx_->where_data) {
    return {std::move(stmt), res};
  }

  for (std::int32_t idx = 0;
       idx < static_cast<std::int32_t>(ctx_->where_data->values.size());
       idx++) {
    res =
        std::visit(overloaded{
                       [&stmt, &idx](std::int64_t data) -> std::int32_t {
                         return sqlite3_bind_int64(stmt.get(), idx + 1, data);
                       },
                       [&stmt, &idx](const std::string &data) -> std::int32_t {
                         return sqlite3_bind_text(stmt.get(), idx + 1,
                                                  data.c_str(), -1, nullptr);
                       },
                   },
                   ctx_->where_data->values.at(static_cast<std::size_t>(idx)));
    if (res != SQLITE_OK) {
      return {std::move(stmt), res};
    }
  }

  return {std::move(stmt), res};
}

auto db_delete::group(context::w_t::group_func_t func) -> context::w_t::wn_t {
  if (not ctx_->where_data) {
    ctx_->where_data = std::make_unique<context::wd_t>(context::wd_t{
        context::w_t{0U, ctx_},
        {},
        {},
    });
  }

  return ctx_->where_data->base.group(std::move(func));
}

auto db_delete::where(std::string column_name) const -> context::w_t::cn_t {
  if (not ctx_->where_data) {
    ctx_->where_data = std::make_unique<context::wd_t>(context::wd_t{
        context::w_t{0U, ctx_},
        {},
        {},
    });
  }

  return ctx_->where_data->base.where(column_name);
}
} // namespace repertory::utils::db::sqlite

#endif // defined(PROJECT_ENABLE_SQLITE)
