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
#ifndef REPERTORY_INCLUDE_UTILS_DB_SQLITE_DB_WHERE_T_HPP_
#define REPERTORY_INCLUDE_UTILS_DB_SQLITE_DB_WHERE_T_HPP_
#if defined(PROJECT_ENABLE_SQLITE)

#include "utils/db/sqlite/db_common.hpp"

namespace repertory::utils::db::sqlite {
template <typename w_t> struct where_data_t final {
  w_t base;
  std::map<std::size_t, std::vector<typename w_t::action_t>> actions;
  std::vector<db_types_t> values;
};

template <typename cn_t, typename ctx_t, typename op_t, typename w_t,
          typename wn_t>
struct db_next_t final {
  std::size_t action_idx{};
  std::shared_ptr<ctx_t> ctx;
  std::string action;

  using group_func_t = std::function<void(w_t &)>;

  [[nodiscard]] auto where(std::string column_name) -> cn_t {
    return w_t{action_idx, ctx}.where(column_name);
  }

  [[nodiscard]] auto dump() const -> std::string { return op_t{ctx}.dump(); }

  [[nodiscard]] auto dump(std::int32_t &idx) const -> std::string {
    return ctx->where_data->base.dump(idx);
  }

  [[nodiscard]] auto go() const -> auto { return op_t{ctx}.go(); }

  [[nodiscard]] auto group(group_func_t func) -> wn_t {
    return w_t{action_idx, ctx}.group(std::move(func));
  }

  [[nodiscard]] auto op() -> op_t {
    return op_t{
        ctx,
    };
  }
};

template <typename cn_t, typename ctx_t, typename op_t, typename w_t>
struct db_where_next_t final {
  std::size_t action_idx{};
  std::shared_ptr<ctx_t> ctx;

  using n_t = db_next_t<cn_t, ctx_t, op_t, w_t, db_where_next_t>;

  [[nodiscard]] auto and_() -> n_t {
    n_t next{
        action_idx,
        ctx,
        "AND",
    };

    ctx->where_data->actions[action_idx].emplace_back(next);
    return next;
  }

  [[nodiscard]] auto dump() const -> std::string { return op_t{ctx}.dump(); }

  [[nodiscard]] auto dump(std::int32_t &idx) const -> std::string {
    return ctx->where_data->base.dump(idx);
  }

  [[nodiscard]] auto go() const -> auto { return op_t{ctx}.go(); }

  [[nodiscard]] auto op() -> op_t {
    return op_t{
        ctx,
    };
  }

  [[nodiscard]] auto or_() -> n_t {
    n_t next{
        action_idx,
        ctx,
        "OR",
    };

    ctx->where_data->actions[action_idx].emplace_back(next);
    return next;
  }
};

template <typename ctx_t, typename op_t, typename w_t>
struct db_comp_next_t final {
  std::size_t action_idx{};
  std::shared_ptr<ctx_t> ctx;
  std::string column_name;

  using wn_t = db_where_next_t<db_comp_next_t, ctx_t, op_t, w_t>;

  [[nodiscard]] auto create(std::string operation, db_types_t value) {
    ctx->where_data->actions[action_idx].emplace_back(db_comp_data_t{
        column_name,
        operation,
    });

    ctx->where_data->values.push_back(value);

    return wn_t{
        action_idx,
        ctx,
    };
  }

  auto equals(db_types_t value) -> wn_t { return create("=", value); };

  auto gt(db_types_t value) -> wn_t { return create(">", value); }

  auto gte(db_types_t value) -> wn_t { return create(">=", value); }

  auto like(db_types_t value) -> wn_t { return create("LIKE", value); }

  auto lt(db_types_t value) -> wn_t { return create("<", value); }

  auto lte(db_types_t value) -> wn_t { return create("<=", value); }

  auto not_equals(db_types_t value) -> wn_t { return create("!=", value); };
};

template <typename ctx_t, typename op_t> struct db_where_t final {
  std::size_t action_idx{0U};
  std::shared_ptr<ctx_t> ctx;

  using cn_t = db_comp_next_t<ctx_t, op_t, db_where_t>;
  using wn_t = db_where_next_t<cn_t, ctx_t, op_t, db_where_t>;
  using n_t = db_next_t<cn_t, ctx_t, op_t, db_where_t, wn_t>;

  using group_func_t = std::function<void(db_where_t &)>;

  using action_t = std::variant<db_comp_data_t, n_t, db_where_t>;

  [[nodiscard]] static auto dump(std::int32_t &idx,
                                 auto &&actions) -> std::string {
    std::stringstream stream;

    for (auto &&action : actions) {
      std::visit(overloaded{
                     [&idx, &stream](const db_comp_data_t &comp) {
                       stream << '"' << comp.column_name << '"' << comp.op_type
                              << '?' + std::to_string(++idx);
                     },
                     [&idx, &stream](const n_t &next) {
                       stream << ' ' << next.action << ' ';
                     },
                     [&idx, &stream](const db_where_t &where) {
                       stream << '(' << dump(idx, where.get_actions()) << ')';
                     },
                 },
                 action);
    }

    return stream.str();
  }

  [[nodiscard]] auto dump() const -> std::string { return op_t{ctx}.dump(); }

  [[nodiscard]] auto dump(std::int32_t &idx) const -> std::string {
    return dump(idx, ctx->where_data->actions[action_idx]);
  }

  [[nodiscard]] auto get_actions() -> auto & {
    return ctx->where_data->actions[action_idx];
  }

  [[nodiscard]] auto get_actions() const -> const auto & {
    return ctx->where_data->actions[action_idx];
  }

  [[nodiscard]] auto group(group_func_t func) -> wn_t {
    ctx->where_data->actions[action_idx];

    db_where_t where{ctx->where_data->actions.size(), ctx};
    func(where);

    ctx->where_data->actions[action_idx].emplace_back(where);

    return wn_t{
        action_idx,
        ctx,
    };
  }

  [[nodiscard]] auto where(std::string column_name) -> cn_t {
    ctx->where_data->actions[action_idx];

    return cn_t{
        action_idx,
        ctx,
        column_name,
    };
  }
};
} // namespace repertory::utils::db::sqlite

#endif // defined(PROJECT_ENABLE_SQLITE)
#endif // REPERTORY_INCLUDE_UTILS_DB_SQLITE_DB_WHERE_T_HPP_
