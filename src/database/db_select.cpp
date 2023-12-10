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
#include "database/db_select.hpp"

namespace repertory::db {

auto db_select::column(std::string column_name) -> db_select & {
  if (context_->delete_query) {
    throw std::runtime_error("columns may not be specified for delete");
  }

  context_->columns.push_back(column_name);
  return *this;
}

auto db_select::count(std::string column_name, std::string as_column_name)
    -> db_select & {
  context_->count_columns[column_name] = as_column_name;
  return *this;
}

auto db_select::delete_query() -> db_select & {
  if (not context_->columns.empty()) {
    throw std::runtime_error("columns must be empty for delete");
  }

  context_->delete_query = true;
  return *this;
}

auto db_select::dump() const -> std::string {
  std::stringstream query;
  query << (context_->delete_query ? "DELETE " : "SELECT ");
  if (not context_->delete_query) {
    bool has_column{false};
    if (context_->columns.empty()) {
      if (context_->count_columns.empty()) {
        query << "*";
        has_column = true;
      }
    } else {
      has_column = not context_->columns.empty();
      for (std::size_t idx = 0U; idx < context_->columns.size(); idx++) {
        if (idx > 0U) {
          query << ", ";
        }
        query << context_->columns.at(idx);
      }
    }

    for (std::int32_t idx = 0U;
         idx < static_cast<std::int32_t>(context_->count_columns.size());
         idx++) {
      if (has_column || idx > 0) {
        query << ", ";
      }
      query << "COUNT(\"";
      auto &count_column = *std::next(context_->count_columns.begin(), idx);
      query << count_column.first << "\") AS \"" << count_column.second << '"';
    }
  }
  query << " FROM \"" << context_->table_name << "\"";

  if (not context_->ands.empty()) {
    query << " WHERE (";
    for (std::int32_t idx = 0;
         idx < static_cast<std::int32_t>(context_->ands.size()); idx++) {
      if (idx > 0) {
        query << " AND ";
      }

      auto &item = context_->ands.at(static_cast<std::size_t>(idx));
      query << '"' << item.column_name << '"' << item.op_type << "?"
            << (idx + 1);
    }
    query << ")";
  }

  if (not context_->delete_query) {
    if (context_->order_by.has_value()) {
      query << " ORDER BY \"" << context_->order_by.value().first << "\" ";
      query << (context_->order_by.value().second ? "ASC" : "DESC");
    }

    if (context_->limit.has_value()) {
      query << " LIMIT " << context_->limit.value();
    }
  }

  query << ';';

  return query.str();
}

auto db_select::go() const -> db_result<context> {
  sqlite3_stmt *stmt_ptr{nullptr};
  auto query_str = dump();
  auto res = sqlite3_prepare_v2(&context_->db3, query_str.c_str(), -1,
                                &stmt_ptr, nullptr);
  if (res != SQLITE_OK) {
    utils::error::raise_error(__FUNCTION__,
                              "failed to prepare|" + std::to_string(res) + '|' +
                                  sqlite3_errstr(res) + '|' + query_str);
    return {context_, res};
  }
  context_->stmt.reset(stmt_ptr);

  for (std::int32_t idx = 0;
       idx < static_cast<std::int32_t>(context_->ands.size()); idx++) {
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
        context_->ands.at(static_cast<std::size_t>(idx)).value);
    if (res != SQLITE_OK) {
      utils::error::raise_error(__FUNCTION__,
                                "failed to bind|" + std::to_string(res) + '|' +
                                    sqlite3_errstr(res) + '|' + query_str);
      return {context_, res};
    }
  }

  return {context_, res};
}

auto db_select::limit(std::int32_t value) -> db_select & {
  if (context_->delete_query) {
    throw std::runtime_error("limit may not be specified for delete");
  }

  context_->limit = value;
  return *this;
}

auto db_select::order_by(std::string column_name, bool ascending)
    -> db_select & {
  if (context_->delete_query) {
    throw std::runtime_error("order_by may not be specified for delete");
  }

  context_->order_by = {column_name, ascending};
  return *this;
}

auto db_select::where(std::string column_name) const -> db_where {
  return db_where{context_, column_name};
}
} // namespace repertory::db
