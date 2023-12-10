/*
  Copyright <2018-2023> <scott.e.graves@protonmail.com>

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
#include "database/db_insert.hpp"

namespace repertory::db {
auto db_insert::column_value(std::string column_name, db_types_t value)
    -> db_insert & {
  context_->values[column_name] = value;
  return *this;
}

auto db_insert::dump() const -> std::string {
  std::stringstream query;
  query << "INSERT ";
  if (context_->or_replace) {
    query << "OR REPLACE ";
  }
  query << "INTO \"" << context_->table_name << "\" (";

  for (std::int32_t idx = 0;
       idx < static_cast<std::int32_t>(context_->values.size()); idx++) {
    if (idx > 0) {
      query << ", ";
    }
    query << '"' << std::next(context_->values.begin(), idx)->first << '"';
  }

  query << ") VALUES (";
  for (std::int32_t idx = 0;
       idx < static_cast<std::int32_t>(context_->values.size()); idx++) {
    if (idx > 0) {
      query << ", ";
    }
    query << "?" << (idx + 1);
  }
  query << ");";

  return query.str();
}

auto db_insert::go() const -> db_result<context> {
  sqlite3_stmt *stmt_ptr{nullptr};
  auto query_str = dump();
  auto res = sqlite3_prepare_v2(&context_->db3, query_str.c_str(), -1,
                                &stmt_ptr, nullptr);
  if (res != SQLITE_OK) {
    utils::error::raise_error(__FUNCTION__, "failed to prepare|" +
                                                std::to_string(res) + '|' +
                                                sqlite3_errstr(res));
    return {context_, res};
  }
  context_->stmt.reset(stmt_ptr);

  for (std::int32_t idx = 0;
       idx < static_cast<std::int32_t>(context_->values.size()); idx++) {
    res = std::visit(
        overloaded{
            [this, &idx](std::int64_t data) -> std::int32_t {
              return sqlite3_bind_int64(context_->stmt.get(), idx + 1, data);
            },
            [this, &idx](const std::string &data) -> std::int32_t {
              return sqlite3_bind_text(context_->stmt.get(), idx + 1,
                                       data.c_str(), -1, nullptr);
            },
        },
        std::next(context_->values.begin(), idx)->second);
    if (res != SQLITE_OK) {
      utils::error::raise_error(__FUNCTION__, "failed to bind|" +
                                                  std::to_string(res) + '|' +
                                                  sqlite3_errstr(res));
      return {context_, res};
    }
  }

  return {context_, res};
}
} // namespace repertory::db
