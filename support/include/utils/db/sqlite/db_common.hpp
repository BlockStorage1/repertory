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
#ifndef REPERTORY_INCLUDE_UTILS_DB_SQLITE_DB_COMMON_HPP_
#define REPERTORY_INCLUDE_UTILS_DB_SQLITE_DB_COMMON_HPP_
#if defined(PROJECT_ENABLE_SQLITE)

#include "utils/config.hpp"

#include "utils/error.hpp"

namespace repertory::utils::db::sqlite {
using db_types_t = std::variant<std::int64_t, std::string>;

struct sqlite3_deleter final {
  void operator()(sqlite3 *db3) const;
};

using db3_t = std::unique_ptr<sqlite3, sqlite3_deleter>;

struct sqlite3_statement_deleter final {
  void operator()(sqlite3_stmt *stmt) const {
    if (stmt != nullptr) {
      sqlite3_finalize(stmt);
    }
  }
};

using db3_stmt_t = std::unique_ptr<sqlite3_stmt, sqlite3_statement_deleter>;

[[nodiscard]] auto
create_db(std::string db_path,
          const std::map<std::string, std::string> &sql_create_tables) -> db3_t;

[[nodiscard]] auto execute_sql(sqlite3 &db3, const std::string &sql,
                               std::string &err) -> bool;

void set_journal_mode(sqlite3 &db3);

struct db_comp_data_t final {
  std::string column_name;
  std::string op_type;
};

struct db_context_t {
  db_context_t(sqlite3 *db3_, std::string table_name_)
      : db3(db3_), table_name(std::move(table_name_)) {}

  sqlite3 *db3{};
  std::string table_name;
};

struct db_result final {
  struct context final {
    db3_stmt_t stmt;
  };

  class db_column final {
  public:
    db_column(std::int32_t index, std::string name, db_types_t value) noexcept;

    db_column() noexcept = default;
    db_column(const db_column &) = default;
    db_column(db_column &&column) noexcept = default;

    ~db_column() = default;

    auto operator=(const db_column &) -> db_column & = default;
    auto operator=(db_column &&) -> db_column & = default;

  private:
    std::int32_t index_{};
    std::string name_;
    db_types_t value_;

  public:
    [[nodiscard]] auto get_index() const -> std::int32_t { return index_; }

    [[nodiscard]] auto get_name() const -> std::string { return name_; }

    template <typename data_type>
    [[nodiscard]] auto get_value() const -> data_type {
      REPERTORY_USES_FUNCTION_NAME();

      return std::visit(
          overloaded{
              [](const data_type &value) -> data_type { return value; },
              [](auto &&) -> data_type {
                throw utils::error::create_exception(
                    function_name, {
                                       "data type not supported",
                                   });
              },
          },
          value_);
    }

#if defined(PROJECT_ENABLE_JSON)
    [[nodiscard]] auto get_value_as_json() const -> nlohmann::json;
#endif // defined(PROJECT_ENABLE_JSON)
  };

  class db_row final {
  public:
    db_row(std::shared_ptr<context> ctx);

  private:
    std::map<std::string, db_column> columns_;

  public:
    [[nodiscard]] auto get_columns() const -> std::vector<db_column>;

    [[nodiscard]] auto get_column(std::int32_t index) const -> db_column;

    [[nodiscard]] auto get_column(std::string name) const -> db_column;
  };

  db_result(db3_stmt_t stmt, std::int32_t res);

  db_result() = default;
  db_result(const db_result &) = default;
  db_result(db_result &&) noexcept = default;

  auto operator=(const db_result &) -> db_result & = default;
  auto operator=(db_result &&) -> db_result & = default;

  using row = db_row;

private:
  std::shared_ptr<context> ctx_;
  mutable std::int32_t res_{};

private:
  void set_res(std::int32_t res) const { res_ = res; }

public:
  [[nodiscard]] auto get_error() const -> std::int32_t { return res_; }

  [[nodiscard]] auto get_error_str() const -> std::string;

  [[nodiscard]] auto get_row(std::optional<row> &opt_row) const -> bool;

  [[nodiscard]] auto has_row() const -> bool;

  void next_row() const;

  [[nodiscard]] auto ok() const -> bool;
};
} // namespace repertory::utils::db::sqlite

#endif // defined(PROJECT_ENABLE_SQLITE)
#endif // REPERTORY_INCLUDE_UTILS_DB_SQLITE_DB_COMMON_HPP_
