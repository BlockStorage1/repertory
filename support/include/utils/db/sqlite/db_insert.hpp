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
#ifndef REPERTORY_INCLUDE_UTILS_DB_SQLITE_DB_INSERT_HPP_
#define REPERTORY_INCLUDE_UTILS_DB_SQLITE_DB_INSERT_HPP_
#if defined(PROJECT_ENABLE_SQLITE)

#include "utils/db/sqlite/db_common.hpp"

namespace repertory::utils::db::sqlite {
class db_insert final {
public:
  struct context final : db_context_t {
    context(sqlite3 *db3_, std::string table_name_)
        : db_context_t(db3_, table_name_) {}

    bool or_replace{false};
    std::map<std::string, db_types_t> values;
  };

public:
  db_insert(sqlite3 &db3, std::string table_name)
      : ctx_(std::make_shared<context>(&db3, table_name)) {}

  db_insert(std::shared_ptr<context> ctx) : ctx_(std::move(ctx)) {}

private:
  std::shared_ptr<context> ctx_;

public:
  [[nodiscard]] auto or_replace() -> db_insert {
    ctx_->or_replace = true;
    return *this;
  }

  [[nodiscard]] auto column_value(std::string column_name, db_types_t value)
      -> db_insert;

  [[nodiscard]] auto dump() const -> std::string;

  [[nodiscard]] auto go() const -> db_result;
};
} // namespace repertory::utils::db::sqlite

#endif // defined(PROJECT_ENABLE_SQLITE)
#endif // REPERTORY_INCLUDE_UTILS_DB_SQLITE_DB_INSERT_HPP_
