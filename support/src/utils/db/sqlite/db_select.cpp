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

#include "utils/db/sqlite/db_select.hpp"

namespace repertory::utils::db::sqlite {
auto db_select::context::db_select_op_t::dump() const -> std::string {
  return db_select{ctx}.dump();
}

auto db_select::context::db_select_op_t::go() const -> db_result {
  return db_select{ctx}.go();
}

auto db_select::context::db_select_op_t::group_by(std::string column_name)
    -> db_select::context::db_select_op_t {
  db_select{ctx}.group_by(column_name);
  return *this;
}

auto db_select::context::db_select_op_t::limit(std::int32_t value)
    -> db_select::context::db_select_op_t {
  db_select{ctx}.limit(value);
  return *this;
}

auto db_select::context::db_select_op_t::offset(std::int32_t value)
    -> db_select::context::db_select_op_t {
  db_select{ctx}.offset(value);
  return *this;
}

auto db_select::context::db_select_op_t::order_by(std::string column_name,
                                                  bool ascending)
    -> db_select::context::db_select_op_t {
  db_select{ctx}.order_by(column_name, ascending);
  return *this;
}

auto db_select::column(std::string column_name) -> db_select {
  ctx_->columns.push_back(column_name);
  return *this;
}

auto db_select::count(std::string column_name, std::string as_column_name)
    -> db_select {
  ctx_->count_columns[column_name] = as_column_name;
  return *this;
}

auto db_select::dump() const -> std::string {
  std::stringstream query;
  query << "SELECT ";
  bool has_column{false};
  if (ctx_->columns.empty()) {
    if (ctx_->count_columns.empty()) {
      query << "*";
      has_column = true;
    }
  } else {
    has_column = not ctx_->columns.empty();
    for (std::size_t idx = 0U; idx < ctx_->columns.size(); idx++) {
      if (idx > 0U) {
        query << ", ";
      }
      query << ctx_->columns.at(idx);
    }
  }

  for (std::int32_t idx = 0U;
       idx < static_cast<std::int32_t>(ctx_->count_columns.size()); idx++) {
    if (has_column || idx > 0) {
      query << ", ";
    }
    query << "COUNT(\"";
    auto &count_column = *std::next(ctx_->count_columns.begin(), idx);
    query << count_column.first << "\") AS \"" << count_column.second << '"';
  }
  query << " FROM \"" << ctx_->table_name << "\"";

  if (ctx_->where_data) {
    std::int32_t idx{};
    query << " WHERE " << ctx_->where_data->base.dump(idx);
  }

  if (not ctx_->group_by.empty()) {
    query << " GROUP BY ";
    for (std::size_t idx = 0U; idx < ctx_->group_by.size(); idx++) {
      if (idx > 0U) {
        query << ", ";
      }

      query << "\"" << ctx_->group_by.at(idx) << "\"";
    }
  }

  if (ctx_->order_by.has_value()) {
    query << " ORDER BY \"" << ctx_->order_by.value().first << "\" ";
    query << (ctx_->order_by.value().second ? "ASC" : "DESC");
  }

  if (ctx_->limit.has_value()) {
    query << " LIMIT " << ctx_->limit.value();
  }

  if (ctx_->offset.has_value()) {
    query << " OFFSET " << ctx_->offset.value();
  }

  query << ';';

  return query.str();
}

auto db_select::go() const -> db_result {
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
                       [&idx, &stmt](std::int64_t data) -> std::int32_t {
                         return sqlite3_bind_int64(stmt.get(), idx + 1, data);
                       },
                       [&idx, &stmt](const std::string &data) -> std::int32_t {
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

auto db_select::group(context::w_t::group_func_t func) -> context::w_t::wn_t {
  if (not ctx_->where_data) {
    ctx_->where_data = std::make_unique<context::wd_t>(context::wd_t{
        context::w_t{0U, ctx_},
        {},
        {},
    });
  }

  return ctx_->where_data->base.group(std::move(func));
}

auto db_select::group_by(std::string column_name) -> db_select {
  ctx_->group_by.emplace_back(std::move(column_name));
  return *this;
}

auto db_select::limit(std::int32_t value) -> db_select {
  ctx_->limit = value;
  return *this;
}

auto db_select::offset(std::int32_t value) -> db_select {
  ctx_->offset = value;
  return *this;
}

auto db_select::order_by(std::string column_name, bool ascending) -> db_select {
  ctx_->order_by = {column_name, ascending};
  return *this;
}

auto db_select::where(std::string column_name) const -> context::w_t::cn_t {
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
