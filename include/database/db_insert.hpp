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
#ifndef INCLUDE_DATABASE_DB_INSERT_HPP_
#define INCLUDE_DATABASE_DB_INSERT_HPP_

#include "database/db_common.hpp"
#include "utils/error_utils.hpp"

namespace repertory::db {
class db_insert final {
public:
  struct context final {
    context(sqlite3 &db3_, std::string table_name_)
        : db3(db3_), table_name(std::move(table_name_)) {}

    sqlite3 &db3;
    std::string table_name;

    bool or_replace{false};
    std::map<std::string, db_types_t> values{};
    db3_stmt_t stmt{nullptr};
  };

  using row = db_row<context>;

public:
  db_insert(sqlite3 &db3, std::string table_name)
      : context_(std::make_shared<context>(db3, table_name)) {}

  db_insert(std::shared_ptr<context> ctx) : context_(std::move(ctx)) {}

private:
  std::shared_ptr<context> context_;

public:
  [[nodiscard]] auto or_replace() -> db_insert & {
    context_->or_replace = true;
    return *this;
  }

  [[nodiscard]] auto column_value(std::string column_name, db_types_t value)
      -> db_insert &;

  [[nodiscard]] auto dump() const -> std::string;

  [[nodiscard]] auto go() const -> db_result<context>;
};
} // namespace repertory::db

#endif // INCLUDE_DATABASE_DB_INSERT_HPP_
