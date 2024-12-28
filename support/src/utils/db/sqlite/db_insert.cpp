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
#if defined(PROJECT_ENABLE_SQLITE)

#include "utils/db/sqlite/db_insert.hpp"

namespace repertory::utils::db::sqlite {
auto db_insert::column_value(std::string column_name, db_types_t value)
    -> db_insert {
  ctx_->values[column_name] = value;
  return *this;
}

auto db_insert::dump() const -> std::string {
  std::stringstream query;
  query << "INSERT ";
  if (ctx_->or_replace) {
    query << "OR REPLACE ";
  }
  query << "INTO \"" << ctx_->table_name << "\" (";

  for (std::int32_t idx = 0;
       idx < static_cast<std::int32_t>(ctx_->values.size()); idx++) {
    if (idx > 0) {
      query << ", ";
    }
    query << '"' << std::next(ctx_->values.begin(), idx)->first << '"';
  }

  query << ") VALUES (";
  for (std::int32_t idx = 0;
       idx < static_cast<std::int32_t>(ctx_->values.size()); idx++) {
    if (idx > 0) {
      query << ", ";
    }
    query << "?" << (idx + 1);
  }
  query << ");";

  return query.str();
}

auto db_insert::go() const -> db_result {
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

  for (std::int32_t idx = 0;
       idx < static_cast<std::int32_t>(ctx_->values.size()); idx++) {
    res =
        std::visit(overloaded{
                       [&idx, &stmt](std::int64_t data) -> std::int32_t {
                         return sqlite3_bind_int64(stmt.get(), idx + 1, data);
                       },
                       [&idx, &stmt](const std::string &data) -> std::int32_t {
                         return sqlite3_bind_text(stmt.get(), idx + 1,
                                                  data.c_str(), -1, nullptr);
                       },
                   },
                   std::next(ctx_->values.begin(), idx)->second);
    if (res != SQLITE_OK) {
      return {std::move(stmt), res};
    }
  }

  return {std::move(stmt), res};
}
} // namespace repertory::utils::db::sqlite

#endif // defined(PROJECT_ENABLE_SQLITE)
