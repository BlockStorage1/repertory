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
#include "db/impl/sqlite_meta_db.hpp"

#include "app_config.hpp"
#include "types/startup_exception.hpp"
#include "utils/db/sqlite/db_common.hpp"
#include "utils/db/sqlite/db_delete.hpp"
#include "utils/db/sqlite/db_insert.hpp"
#include "utils/db/sqlite/db_select.hpp"
#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"

namespace repertory {
sqlite_meta_db::sqlite_meta_db(const app_config &cfg) {
  const std::map<std::string, std::string> sql_create_tables{
      {
          {"meta"},
          {"CREATE TABLE IF NOT EXISTS "
           "meta "
           "("
           "api_path TEXT PRIMARY KEY ASC, "
           "data TEXT, "
           "directory INTEGER, "
           "pinned INTEGER, "
           "size INTEGER, "
           "source_path TEXT"
           ");"},
      },
  };

  auto db_dir = utils::path::combine(cfg.get_data_directory(), {"db"});
  if (not utils::file::directory{db_dir}.create_directory()) {
    throw startup_exception(
        fmt::format("failed to create db directory|", db_dir));
  }

  db_ = utils::db::sqlite::create_db(utils::path::combine(db_dir, {"meta.db"}),
                                     sql_create_tables);
}

sqlite_meta_db::~sqlite_meta_db() { db_.reset(); }

void sqlite_meta_db::clear() {
  REPERTORY_USES_FUNCTION_NAME();

  auto result = utils::db::sqlite::db_delete{*db_, table_name}.go();
  if (result.ok()) {
    return;
  }

  utils::error::raise_error(function_name,
                            "failed to clear meta db|" +
                                std::to_string(result.get_error()));
}

void sqlite_meta_db::enumerate_api_path_list(
    std::function<void(const std::vector<std::string> &)> callback,
    stop_type_callback stop_requested_cb) const {
  auto result =
      utils::db::sqlite::db_select{*db_, table_name}.column("api_path").go();

  std::vector<std::string> list{};
  while (not stop_requested_cb() && result.has_row()) {
    std::optional<utils::db::sqlite::db_result::row> row;
    if (result.get_row(row) && row.has_value()) {
      list.push_back(row->get_column("api_path").get_value<std::string>());

      if (list.size() < 100U) {
        continue;
      }

      callback(list);
      list.clear();
    }
  }

  if (not list.empty()) {
    callback(list);
  }
}

auto sqlite_meta_db::get_api_path(const std::string &source_path,
                                  std::string &api_path) const -> api_error {
  auto result = utils::db::sqlite::db_select{*db_, table_name}
                    .column("api_path")
                    .where("source_path")
                    .equals(source_path)
                    .op()
                    .limit(1)
                    .go();

  std::optional<utils::db::sqlite::db_result::row> row;
  if (result.get_row(row) && row.has_value()) {
    api_path = row->get_column("api_path").get_value<std::string>();
    return api_error::success;
  }

  return api_error::item_not_found;
}

auto sqlite_meta_db::get_api_path_list() const -> std::vector<std::string> {
  std::vector<std::string> ret{};

  auto result =
      utils::db::sqlite::db_select{*db_, table_name}.column("api_path").go();
  while (result.has_row()) {
    std::optional<utils::db::sqlite::db_result::row> row;
    if (result.get_row(row) && row.has_value()) {
      ret.push_back(row->get_column("api_path").get_value<std::string>());
    }
  }

  return ret;
}

auto sqlite_meta_db::get_item_meta(const std::string &api_path,
                                   api_meta_map &meta) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  auto result = utils::db::sqlite::db_select{*db_, table_name}
                    .column("*")
                    .where("api_path")
                    .equals(api_path)
                    .op()
                    .limit(1)
                    .go();
  if (not result.has_row()) {
    return api_error::item_not_found;
  }

  try {
    std::optional<utils::db::sqlite::db_result::row> row;
    if (result.get_row(row) && row.has_value()) {
      meta = json::parse(row->get_column("data").get_value<std::string>())
                 .get<api_meta_map>();
      meta[META_DIRECTORY] = utils::string::from_bool(
          row->get_column("directory").get_value<std::int64_t>() == 1);
      meta[META_PINNED] = utils::string::from_bool(
          row->get_column("pinned").get_value<std::int64_t>() == 1);
      meta[META_SIZE] = std::to_string(static_cast<std::uint64_t>(
          row->get_column("size").get_value<std::int64_t>()));
      meta[META_SOURCE] =
          row->get_column("source_path").get_value<std::string>();
      return api_error::success;
    }

    return api_error::item_not_found;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to get item meta");
  }

  return api_error::error;
}

auto sqlite_meta_db::get_item_meta(const std::string &api_path,
                                   const std::string &key,
                                   std::string &value) const -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  auto result = utils::db::sqlite::db_select{*db_, table_name}
                    .column("*")
                    .where("api_path")
                    .equals(api_path)
                    .op()
                    .limit(1)
                    .go();
  if (not result.has_row()) {
    return api_error::item_not_found;
  }

  try {
    std::optional<utils::db::sqlite::db_result::row> row;
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
          : key == META_SIZE
              ? std::to_string(static_cast<std::uint64_t>(
                    row->get_column("size").get_value<std::int64_t>()))
              : json::parse(
                    row->get_column("data").get_value<std::string>())[key]
                    .get<std::string>();
      return api_error::success;
    }

    return api_error::item_not_found;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to get item meta");
  }

  return api_error::error;
}

auto sqlite_meta_db::get_pinned_files() const -> std::vector<std::string> {
  REPERTORY_USES_FUNCTION_NAME();

  std::vector<std::string> ret{};

  try {
    auto result = utils::db::sqlite::db_select{*db_, table_name}
                      .column("api_path")
                      .where("pinned")
                      .equals(1)
                      .go();
    while (result.has_row()) {
      std::optional<utils::db::sqlite::db_result::row> row;
      if (result.get_row(row) && row.has_value()) {
        ret.emplace_back(row->get_column("api_path").get_value<std::string>());
      }
    }
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "failed to get pinned files");
  }

  return ret;
}

auto sqlite_meta_db::get_total_item_count() const -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  std::uint64_t ret{};

  try {
    auto result = utils::db::sqlite::db_select{*db_, table_name}
                      .count("api_path", "count")
                      .go();

    std::optional<utils::db::sqlite::db_result::row> row;
    if (result.get_row(row) && row.has_value()) {
      ret = static_cast<std::uint64_t>(
          row->get_column("count").get_value<std::int64_t>());
    }
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e,
                              "failed to get total item count");
  }

  return ret;
}

auto sqlite_meta_db::get_total_size() const -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    auto result = utils::db::sqlite::db_select{*db_, table_name}
                      .column("SUM(size) as total_size")
                      .where("directory")
                      .equals(0)
                      .go();

    std::optional<utils::db::sqlite::db_result::row> row;
    if (result.get_row(row) && row.has_value()) {
      return static_cast<std::uint64_t>(
          row->get_column("total_size").get_value<std::int64_t>());
    }
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "failed to get total size");
  }

  return 0U;
}

void sqlite_meta_db::remove_api_path(const std::string &api_path) {
  REPERTORY_USES_FUNCTION_NAME();

  auto result = utils::db::sqlite::db_delete{*db_, table_name}
                    .where("api_path")
                    .equals(api_path)
                    .go();
  if (not result.ok()) {
    utils::error::raise_api_path_error(
        function_name, api_path, result.get_error(), "failed to remove meta");
  }
}

auto sqlite_meta_db::remove_item_meta(const std::string &api_path,
                                      const std::string &key) -> api_error {
  if (key == META_DIRECTORY || key == META_PINNED || key == META_SIZE ||
      key == META_SOURCE) {
    // TODO log warning for unsupported attributes
    return api_error::success;
  }

  api_meta_map meta{};
  auto res = get_item_meta(api_path, meta);
  if (res != api_error::success) {
    return res;
  }

  meta.erase(key);
  return update_item_meta(api_path, meta);
}

auto sqlite_meta_db::rename_item_meta(const std::string &from_api_path,
                                      const std::string &to_api_path)
    -> api_error {
  api_meta_map meta{};
  auto res = get_item_meta(from_api_path, meta);
  if (res != api_error::success) {
    return res;
  }
  remove_api_path(from_api_path);

  return update_item_meta(to_api_path, meta);
}

auto sqlite_meta_db::set_item_meta(const std::string &api_path,
                                   const std::string &key,
                                   const std::string &value) -> api_error {
  return set_item_meta(api_path, {{key, value}});
}

auto sqlite_meta_db::set_item_meta(const std::string &api_path,
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

auto sqlite_meta_db::update_item_meta(const std::string &api_path,
                                      api_meta_map meta) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (meta[META_PINNED].empty()) {
      meta[META_PINNED] = utils::string::from_bool(false);
    }
    if (meta[META_SIZE].empty()) {
      meta[META_SIZE] = "0";
    }
    if (meta[META_SOURCE].empty()) {
      meta[META_SOURCE] = "";
    }

    auto directory = utils::string::to_bool(meta.at(META_DIRECTORY));
    auto pinned =
        directory ? false : utils::string::to_bool(meta.at(META_PINNED));
    auto size = directory ? std::uint64_t(0U)
                          : utils::string::to_uint64(meta.at(META_SIZE));
    auto source_path = directory ? std::string("") : meta.at(META_SOURCE);

    meta.erase(META_DIRECTORY);
    meta.erase(META_PINNED);
    meta.erase(META_SIZE);
    meta.erase(META_SOURCE);

    auto result = utils::db::sqlite::db_insert{*db_, table_name}
                      .or_replace()
                      .column_value("api_path", api_path)
                      .column_value("data", nlohmann::json(meta).dump())
                      .column_value("directory", directory ? 1 : 0)
                      .column_value("pinned", pinned ? 1 : 0)
                      .column_value("size", static_cast<std::int64_t>(size))
                      .column_value("source_path", source_path)
                      .go();
    if (not result.ok()) {
      utils::error::raise_api_path_error(function_name, api_path,
                                         result.get_error(),
                                         "failed to update item meta");
      return api_error::error;
    }

    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_api_path_error(function_name, api_path, e,
                                       "failed to update item meta");
  }

  return api_error::error;
}
} // namespace repertory
