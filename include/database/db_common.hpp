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
#ifndef INCLUDE_DATABASE_DB_COMMON_HPP_
#define INCLUDE_DATABASE_DB_COMMON_HPP_

#include "utils/error_utils.hpp"

namespace repertory::db {
using db_types_t = std::variant<std::int64_t, std::string>;

struct comp_data_t final {
  std::string column_name;
  std::string op_type;
  db_types_t value;
};

class db_column final {
public:
  db_column() noexcept = default;
  db_column(const db_column &) = default;
  db_column(db_column &&column) noexcept = default;
  ~db_column() = default;

  auto operator=(const db_column &) -> db_column & = default;
  auto operator=(db_column &&) -> db_column & = default;

  db_column(std::int32_t index, std::string name, db_types_t value) noexcept
      : index_(index), name_(std::move(name)), value_(std::move(value)) {}

private:
  std::int32_t index_{};
  std::string name_;
  db_types_t value_;

public:
  [[nodiscard]] auto get_index() const -> std::int32_t { return index_; }

  [[nodiscard]] auto get_name() const -> std::string { return name_; }

  template <typename data_type>
  [[nodiscard]] auto get_value() const -> data_type {
    return std::visit(
        overloaded{
            [](const data_type &value) -> data_type { return value; },
            [](auto &&) -> data_type {
              throw std::runtime_error("data type not supported");
            },
        },
        value_);
  }

  [[nodiscard]] auto get_value_as_json() const -> nlohmann::json {
    return std::visit(
        overloaded{
            [this](std::int64_t value) -> auto {
              return nlohmann::json({{name_, value}});
            },
            [](auto &&value) -> auto { return nlohmann::json::parse(value); },
        },
        value_);
  }
};

template <typename context_t> class db_row final {
public:
  db_row(std::shared_ptr<context_t> context) {
    auto column_count = sqlite3_column_count(context->stmt.get());
    for (std::int32_t col = 0; col < column_count; col++) {
      std::string name{sqlite3_column_name(context->stmt.get(), col)};
      auto column_type = sqlite3_column_type(context->stmt.get(), col);

      db_types_t value;
      switch (column_type) {
      case SQLITE_INTEGER: {
        value = sqlite3_column_int64(context->stmt.get(), col);
      } break;

      case SQLITE_TEXT: {
        const auto *text = reinterpret_cast<const char *>(
            sqlite3_column_text(context->stmt.get(), col));
        value = std::string(text == nullptr ? "" : text);
      } break;

      default:
        throw std::runtime_error("column type not implemented|" + name + '|' +
                                 std::to_string(column_type));
      }

      columns_[name] = db_column{col, name, value};
    }
  }

private:
  std::map<std::string, db_column> columns_;

public:
  [[nodiscard]] auto get_columns() const -> std::vector<db_column> {
    std::vector<db_column> ret;
    for (const auto &item : columns_) {
      ret.push_back(item.second);
    }
    return ret;
  }

  [[nodiscard]] auto get_column(std::int32_t index) const -> db_column {
    auto iter = std::find_if(columns_.begin(), columns_.end(),
                             [&index](auto &&col) -> bool {
                               return col.second.get_index() == index;
                             });
    if (iter == columns_.end()) {
      throw std::out_of_range("");
    }

    return iter->second;
  }

  [[nodiscard]] auto get_column(std::string name) const -> db_column {
    return columns_.at(name);
  }
};

template <typename context_t> struct db_result final {
  db_result(std::shared_ptr<context_t> context, std::int32_t res)
      : context_(std::move(context)), res_(res) {
    if (res == SQLITE_OK) {
      set_res(sqlite3_step(context_->stmt.get()), __FUNCTION__);
    }
  }

private:
  std::shared_ptr<context_t> context_;
  mutable std::int32_t res_;

private:
  void set_res(std::int32_t res, std::string function) const {
    if (res != SQLITE_OK && res != SQLITE_DONE && res != SQLITE_ROW) {
      utils::error::raise_error(function, "failed to step|" +
                                              std::to_string(res) + '|' +
                                              sqlite3_errstr(res));
    }
    res_ = res;
  }

public:
  [[nodiscard]] auto ok() const -> bool {
    return res_ == SQLITE_DONE || res_ == SQLITE_ROW;
  }

  [[nodiscard]] auto get_error() const -> std::int32_t { return res_; }

  [[nodiscard]] auto get_error_str() const -> std::string {
    return sqlite3_errstr(res_);
  }

  [[nodiscard]] auto get_row(std::optional<db_row<context_t>> &row) const
      -> bool {
    row.reset();
    if (has_row()) {
      row = db_row{context_};
      set_res(sqlite3_step(context_->stmt.get()), __FUNCTION__);
      return true;
    }
    return false;
  }

  [[nodiscard]] auto has_row() const -> bool { return res_ == SQLITE_ROW; }

  void next_row() const {
    if (has_row()) {
      set_res(sqlite3_step(context_->stmt.get()), __FUNCTION__);
    }
  }
};

inline void set_journal_mode(sqlite3 &db3) {
  sqlite3_exec(&db3, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
}

[[nodiscard]] inline auto execute_sql(sqlite3 &db3, const std::string &sql,
                                      std::string &err) -> bool {
  char *err_msg{nullptr};
  auto res = sqlite3_exec(&db3, sql.c_str(), nullptr, nullptr, &err_msg);
  if (err_msg != nullptr) {
    err = err_msg;
    sqlite3_free(err_msg);
    err_msg = nullptr;
  }
  if (res != SQLITE_OK) {
    err = "failed to execute sql|" + sql + "|" + std::to_string(res) + '|' +
          (err.empty() ? sqlite3_errstr(res) : err);
    return false;
  }

  return true;
}
} // namespace repertory::db

#endif // INCLUDE_DATABASE_DB_COMMON_HPP_
