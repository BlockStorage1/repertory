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
#ifndef INCLUDE_DATABASE_DB_SELECT_HPP_
#define INCLUDE_DATABASE_DB_SELECT_HPP_

#include "database/db_common.hpp"
#include "utils/error_utils.hpp"

namespace repertory::db {
class db_select final {
public:
  struct context final {
    context(sqlite3 &db3_, std::string table_name_)
        : db3(db3_), table_name(std::move(table_name_)) {}

    sqlite3 &db3;
    std::string table_name;

    std::vector<comp_data_t> ands{};
    std::vector<std::string> columns{};
    std::map<std::string, std::string> count_columns{};
    bool delete_query{false};
    std::optional<std::int32_t> limit;
    std::optional<std::pair<std::string, bool>> order_by;
    db3_stmt_t stmt{nullptr};
  };

  using row = db_row<context>;

public:
  db_select(sqlite3 &db3, std::string table_name)
      : context_(std::make_shared<context>(db3, table_name)) {}

  db_select(std::shared_ptr<context> ctx) : context_(std::move(ctx)) {}

public:
  struct db_where final {
    db_where(std::shared_ptr<context> ctx, std::string column_name)
        : context_(std::move(ctx)), column_name_(std::move(column_name)) {}

  public:
    struct db_where_next final {
      db_where_next(std::shared_ptr<context> ctx) : context_(std::move(ctx)) {}

    private:
      std::shared_ptr<context> context_;

    public:
      [[nodiscard]] auto and_where(std::string column_name) const -> db_where {
        return db_where{context_, column_name};
      }

      [[nodiscard]] auto dump() const -> std::string {
        return db_select{context_}.dump();
      }

      [[nodiscard]] auto go() const -> db_result<context> {
        return db_select{context_}.go();
      }

      [[nodiscard]] auto limit(std::int32_t value) const -> db_select {
        return db_select{context_}.limit(value);
      }

      [[nodiscard]] auto order_by(std::string column_name, bool ascending) const
          -> db_select {
        return db_select{context_}.order_by(column_name, ascending);
      }
    };

  private:
    std::shared_ptr<context> context_;
    std::string column_name_;

  public:
    [[nodiscard]] auto equals(db_types_t value) const -> db_where_next {
      context_->ands.emplace_back(comp_data_t{column_name_, "=", value});
      return db_where_next{context_};
    }
  };

private:
  std::shared_ptr<context> context_;

public:
  [[nodiscard]] auto column(std::string column_name) -> db_select &;

  [[nodiscard]] auto count(std::string column_name, std::string as_column_name)
      -> db_select &;

  [[nodiscard]] auto delete_query() -> db_select &;

  [[nodiscard]] auto dump() const -> std::string;

  [[nodiscard]] auto go() const -> db_result<context>;

  [[nodiscard]] auto limit(std::int32_t value) -> db_select &;

  [[nodiscard]] auto order_by(std::string column_name, bool ascending)
      -> db_select &;

  [[nodiscard]] auto where(std::string column_name) const -> db_where;
};
} // namespace repertory::db

#endif // INCLUDE_DATABASE_DB_SELECT_HPP_
