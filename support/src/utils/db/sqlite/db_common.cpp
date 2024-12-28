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

#include "utils/db/sqlite/db_common.hpp"

#include "utils/common.hpp"
#include "utils/error.hpp"

namespace repertory::utils::db::sqlite {
void sqlite3_deleter::operator()(sqlite3 *db3) const {
  REPERTORY_USES_FUNCTION_NAME();

  if (db3 == nullptr) {
    return;
  }

  std::string err_msg;
  if (not execute_sql(*db3, "VACUUM;", err_msg)) {
    utils::error::handle_error(function_name,
                               utils::error::create_error_message({
                                   "failed to vacuum database",
                                   err_msg,
                               }));
  }

  if (not utils::retry_action([&db3]() -> bool {
        auto res = sqlite3_close_v2(db3);
        if (res == SQLITE_OK) {
          return true;
        }

        auto &&err_str = sqlite3_errstr(res);
        utils::error::handle_error(
            function_name,
            utils::error::create_error_message({
                "failed to close database",
                (err_str == nullptr ? std::to_string(res) : err_str),
            }));
        return false;
      })) {
    repertory::utils::error::handle_error(function_name,
                                          "failed to close database");
  }
}

db_result::db_column::db_column(std::int32_t index, std::string name,
                                db_types_t value) noexcept
    : index_(index), name_(std::move(name)), value_(std::move(value)) {}

#if defined(PROJECT_ENABLE_JSON)
auto db_result::db_column::get_value_as_json() const -> nlohmann::json {
  return std::visit(
      overloaded{
          [this](std::int64_t value) -> auto {
            return nlohmann::json({{name_, value}});
          },
          [](auto &&value) -> auto { return nlohmann::json::parse(value); },
      },
      value_);
}
#endif // defined(PROJECT_ENABLE_JSON)

db_result::db_row::db_row(std::shared_ptr<context> ctx) {
  REPERTORY_USES_FUNCTION_NAME();

  auto column_count = sqlite3_column_count(ctx->stmt.get());
  for (std::int32_t col = 0; col < column_count; col++) {
    std::string name{sqlite3_column_name(ctx->stmt.get(), col)};
    auto column_type = sqlite3_column_type(ctx->stmt.get(), col);

    db_types_t value;
    switch (column_type) {
    case SQLITE_INTEGER: {
      value = sqlite3_column_int64(ctx->stmt.get(), col);
    } break;

    case SQLITE_TEXT: {
      const auto *text = reinterpret_cast<const char *>(
          sqlite3_column_text(ctx->stmt.get(), col));
      value = std::string(text == nullptr ? "" : text);
    } break;

    default:
      throw utils::error::create_exception(function_name,
                                           {
                                               "column type not implemented",
                                               std::to_string(column_type),
                                           });
    }

    columns_[name] = db_column{col, name, value};
  }
}

auto db_result::db_row::get_columns() const -> std::vector<db_column> {
  std::vector<db_column> ret;
  for (const auto &item : columns_) {
    ret.push_back(item.second);
  }
  return ret;
}

auto db_result::db_row::get_column(std::int32_t index) const -> db_column {
  auto iter = std::find_if(
      columns_.begin(), columns_.end(),
      [&index](auto &&col) -> bool { return col.second.get_index() == index; });
  if (iter == columns_.end()) {
    throw std::out_of_range("");
  }

  return iter->second;
}

auto db_result::db_row::get_column(std::string name) const -> db_column {
  return columns_.at(name);
}

db_result::db_result(db3_stmt_t stmt, std::int32_t res)
    : ctx_(std::make_shared<context>()), res_(res) {
  ctx_->stmt = std::move(stmt);
  if (res == SQLITE_OK) {
    set_res(sqlite3_step(ctx_->stmt.get()));
  }
}

auto db_result::get_error_str() const -> std::string {
  auto &&err_msg = sqlite3_errstr(res_);
  return err_msg == nullptr ? std::to_string(res_) : err_msg;
}

auto db_result::get_row(std::optional<row> &opt_row) const -> bool {
  opt_row.reset();

  if (not has_row()) {
    return false;
  }

  opt_row = db_row{ctx_};
  set_res(sqlite3_step(ctx_->stmt.get()));
  return true;
}

auto db_result::has_row() const -> bool { return res_ == SQLITE_ROW; }

void db_result::next_row() const {
  if (not has_row()) {
    return;
  }

  set_res(sqlite3_step(ctx_->stmt.get()));
}

auto db_result::ok() const -> bool {
  return res_ == SQLITE_DONE || res_ == SQLITE_ROW;
}

auto create_db(std::string db_path,
               const std::map<std::string, std::string> &sql_create_tables)
    -> db3_t {
  REPERTORY_USES_FUNCTION_NAME();

  sqlite3 *db_ptr{nullptr};
  auto db_res =
      sqlite3_open_v2(db_path.c_str(), &db_ptr,
                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  if (db_res != SQLITE_OK) {
    const auto *msg = sqlite3_errstr(db_res);
    throw utils::error::create_exception(
        function_name, {
                           "failed to open db",
                           db_path,
                           (msg == nullptr ? std::to_string(db_res) : msg),
                       });
  }

  auto db3 = db3_t{
      db_ptr,
      sqlite3_deleter(),
  };

  for (auto &&create_item : sql_create_tables) {
    std::string err_msg;
    if (not sqlite::execute_sql(*db3, create_item.second, err_msg)) {
      db3.reset();
      throw utils::error::create_exception(function_name, {
                                                              err_msg,
                                                          });
    }
  }

  set_journal_mode(*db3);

  return db3;
}

auto execute_sql(sqlite3 &db3, const std::string &sql, std::string &err)
    -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  char *err_msg{nullptr};
  auto res = sqlite3_exec(&db3, sql.c_str(), nullptr, nullptr, &err_msg);
  if (err_msg != nullptr) {
    err = err_msg;
    sqlite3_free(err_msg);
    err_msg = nullptr;
  }

  if (res == SQLITE_OK) {
    return true;
  }

  err = utils::error::create_error_message({
      function_name,
      "failed to execute sql",
      err,
      sql,
  });

  return false;
}

void set_journal_mode(sqlite3 &db3) {
  sqlite3_exec(&db3,
               "PRAGMA journal_mode = WAL;PRAGMA synchronous = NORMAL;PRAGMA "
               "auto_vacuum = NONE;",
               nullptr, nullptr, nullptr);
}
} // namespace repertory::utils::db::sqlite

#endif // defined(PROJECT_ENABLE_SQLITE)
