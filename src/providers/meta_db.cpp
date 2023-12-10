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
#include "providers/meta_db.hpp"

#include "app_config.hpp"
#include "database/db_common.hpp"
#include "database/db_insert.hpp"
#include "database/db_select.hpp"
#include "utils/error_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/string_utils.hpp"

namespace repertory {
meta_db::meta_db(const app_config &cfg) {
  auto db_path = utils::path::combine(cfg.get_data_directory(), {"meta.db3"});

  sqlite3 *db3{nullptr};
  auto res =
      sqlite3_open_v2(db_path.c_str(), &db3,
                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  if (res != SQLITE_OK) {
    utils::error::raise_error(__FUNCTION__, "failed to open db|" + db_path +
                                                '|' + std::to_string(res) +
                                                '|' + sqlite3_errstr(res));
    return;
  }
  db_.reset(db3);

  const auto *create = "CREATE TABLE IF NOT EXISTS "
                       "meta "
                       "("
                       "api_path TEXT PRIMARY KEY ASC, "
                       "data TEXT, "
                       "directory INTEGER, "
                       "pinned INTEGER, "
                       "source_path TEXT"
                       ");";
  std::string err;
  if (not db::execute_sql(*db_, create, err)) {
    utils::error::raise_error(__FUNCTION__,
                              "failed to create db|" + db_path + '|' + err);
    db_.reset();
    return;
  }

  db::set_journal_mode(*db_);
}

meta_db::~meta_db() { db_.reset(); }

auto meta_db::get_api_path(const std::string &source_path,
                           std::string &api_path) -> api_error {
  auto result = db::db_select{*db_, table_name}
                    .column("api_path")
                    .where("source_path")
                    .equals(source_path)
                    .limit(1)
                    .go();

  std::optional<db::db_select::row> row;
  if (result.get_row(row) && row.has_value()) {
    api_path = row->get_column("api_path").get_value<std::string>();
    return api_error::success;
  }

  return api_error::item_not_found;
}

auto meta_db::get_api_path_list() -> std::vector<std::string> {
  std::vector<std::string> ret{};

  auto result = db::db_select{*db_, table_name}.column("api_path").go();
  while (result.has_row()) {
    std::optional<db::db_select::row> row;
    if (result.get_row(row) && row.has_value()) {
      ret.push_back(row->get_column("api_path").get_value<std::string>());
    }
  }

  return ret;
}

auto meta_db::get_item_meta(const std::string &api_path, api_meta_map &meta)
    -> api_error {
  auto result = db::db_select{*db_, table_name}
                    .column("*")
                    .where("api_path")
                    .equals(api_path)
                    .limit(1)
                    .go();
  if (not result.has_row()) {
    return api_error::item_not_found;
  }

  try {
    std::optional<db::db_select::row> row;
    if (result.get_row(row) && row.has_value()) {
      meta = json::parse(row->get_column("data").get_value<std::string>())
                 .get<api_meta_map>();
      meta[META_DIRECTORY] = utils::string::from_bool(
          row->get_column("directory").get_value<std::int64_t>() == 1);
      meta[META_PINNED] = utils::string::from_bool(
          row->get_column("pinned").get_value<std::int64_t>() == 1);
      meta[META_SOURCE] =
          row->get_column("source_path").get_value<std::string>();

      return api_error::success;
    }

    return api_error::item_not_found;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                       "failed to get item meta");
  }

  return api_error::error;
}

auto meta_db::get_item_meta(const std::string &api_path, const std::string &key,
                            std::string &value) const -> api_error {
  auto result = db::db_select{*db_, table_name}
                    .column("*")
                    .where("api_path")
                    .equals(api_path)
                    .limit(1)
                    .go();
  if (not result.has_row()) {
    return api_error::item_not_found;
  }

  try {
    std::optional<db::db_select::row> row;
    if (result.get_row(row) && row.has_value()) {
      value =
          key == META_SOURCE
              ? row->get_column("source_path").get_value<std::string>()
          : key == META_PINNED
              ? utils::string::from_bool(
                    row->get_column("pinned").get_value<std::int64_t>() == 1)
          : key == META_DIRECTORY
              ? utils::string::from_bool(
                    row->get_column("directory").get_value<std::int64_t>() == 1)
              : json::parse(
                    row->get_column("data").get_value<std::string>())[key]
                    .get<std::string>();
      return api_error::success;
    }

    return api_error::item_not_found;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, e,
                                       "failed to get item meta");
  }

  return api_error::error;
}

auto meta_db::get_pinned_files() const -> std::vector<std::string> {
  std::vector<std::string> ret{};

  try {
    auto result = db::db_select{*db_, table_name}
                      .column("api_path")
                      .where("pinned")
                      .equals(1)
                      .go();
    while (result.has_row()) {
      std::optional<db::db_select::row> row;
      if (result.get_row(row) && row.has_value()) {
        ret.emplace_back(row->get_column("api_path").get_value<std::string>());
      }
    }
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "failed to get pinned files");
  }

  return ret;
}

auto meta_db::get_total_item_count() const -> std::uint64_t {
  std::uint64_t ret{};

  try {
    auto result =
        db::db_select{*db_, table_name}.count("api_path", "count").go();

    std::optional<db::db_select::row> row;
    if (result.get_row(row) && row.has_value()) {
      ret = static_cast<std::uint64_t>(
          row->get_column("count").get_value<std::int64_t>());
    }
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e,
                              "failed to get total item count");
  }

  return ret;
}

void meta_db::remove_api_path(const std::string &api_path) {
  auto result = db::db_select{*db_, table_name}
                    .delete_query()
                    .where("api_path")
                    .equals(api_path)
                    .go();
  if (not result.ok()) {
    utils::error::raise_api_path_error(
        __FUNCTION__, api_path, result.get_error(), "failed to remove meta");
  }
}

auto meta_db::remove_item_meta(const std::string &api_path,
                               const std::string &key) -> api_error {
  api_meta_map meta{};
  auto res = get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  meta.erase(key);
  return update_item_meta(api_path, meta);
}

auto meta_db::rename_item_meta(const std::string &from_api_path,
                               const std::string &to_api_path) -> api_error {
  api_meta_map meta{};
  auto res = get_item_meta(from_api_path, meta);
  if (res != api_error::success) {
    return res;
  }
  remove_api_path(from_api_path);

  return update_item_meta(to_api_path, meta);
}

auto meta_db::set_item_meta(const std::string &api_path, const std::string &key,
                            const std::string &value) -> api_error {
  return set_item_meta(api_path, {{key, value}});
}

auto meta_db::set_item_meta(const std::string &api_path,
                            const api_meta_map &meta) -> api_error {
  api_meta_map existing_meta{};
  if (get_item_meta(api_path, existing_meta) != api_error::success) {
    // TODO handle error
  }

  for (const auto &item : meta) {
    existing_meta[item.first] = item.second;
  }

  return update_item_meta(api_path, existing_meta);
}

auto meta_db::update_item_meta(const std::string &api_path, api_meta_map meta)
    -> api_error {
  auto directory = utils::string::to_bool(meta[META_DIRECTORY]);
  auto pinned = utils::string::to_bool(meta[META_PINNED]);
  auto source_path = meta[META_SOURCE];

  meta.erase(META_DIRECTORY);
  meta.erase(META_PINNED);
  meta.erase(META_SOURCE);

  auto result = db::db_insert{*db_, table_name}
                    .or_replace()
                    .column_value("api_path", api_path)
                    .column_value("data", nlohmann::json(meta).dump())
                    .column_value("directory", directory ? 1 : 0)
                    .column_value("pinned", pinned ? 1 : 0)
                    .column_value("source_path", source_path)
                    .go();
  if (not result.ok()) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path,
                                       result.get_error(),
                                       "failed to update item meta");
    return api_error::error;
  }

  return api_error::success;
}
} // namespace repertory
