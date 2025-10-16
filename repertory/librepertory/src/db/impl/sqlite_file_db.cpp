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
#include "db/impl/sqlite_file_db.hpp"

#include "app_config.hpp"
#include "types/startup_exception.hpp"
#include "utils/config.hpp"
#include "utils/db/sqlite/db_common.hpp"
#include "utils/db/sqlite/db_delete.hpp"
#include "utils/db/sqlite/db_insert.hpp"
#include "utils/db/sqlite/db_select.hpp"
#include "utils/db/sqlite/db_update.hpp"
#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"

namespace {
const std::string file_table = "file";
const std::map<std::string, std::string> sql_create_tables = {
    {
        {file_table},
        {"CREATE TABLE IF NOT EXISTS " + file_table +
         "("
         "source_path TEXT PRIMARY KEY ASC, "
         "api_path TEXT UNIQUE NOT NULL, "
         "directory INTEGER NOT NULL, "
         "iv TEXT DEFAULT '' NOT NULL, "
         "kdf_configs TEXT NOT NULL, "
         "size INTEGER DEFAULT 0 NOT NULL"
         ");"},
    },
};
} // namespace

namespace repertory {
sqlite_file_db::sqlite_file_db(const app_config &cfg) {
  auto db_dir = utils::path::combine(cfg.get_data_directory(), {"db"});
  if (not utils::file::directory{db_dir}.create_directory()) {
    throw startup_exception(
        fmt::format("failed to create db directory|", db_dir));
  }

  db_ = utils::db::sqlite::create_db(utils::path::combine(db_dir, {"file.db"}),
                                     sql_create_tables);
}

sqlite_file_db::~sqlite_file_db() { db_.reset(); }

auto sqlite_file_db::add_or_update_directory(
    const i_file_db::directory_data &data) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  auto result =
      utils::db::sqlite::db_insert{*db_, file_table}
          .or_replace()
          .column_value("api_path", data.api_path)
          .column_value("directory", 1)
          .column_value("kdf_configs", nlohmann::json(data.kdf_configs).dump())
          .column_value("source_path", data.source_path)
          .go();
  if (result.ok()) {
    return api_error::success;
  }

  utils::error::raise_api_path_error(
      function_name, data.api_path, api_error::error,
      fmt::format("failed to add directory|{}", result.get_error_str()));
  return api_error::error;
}

auto sqlite_file_db::add_or_update_file(const i_file_db::file_data &data)
    -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  auto result =
      utils::db::sqlite::db_insert{*db_, file_table}
          .or_replace()
          .column_value("api_path", data.api_path)
          .column_value("directory", 0)
          .column_value("iv", json(data.iv_list).dump())
          .column_value("kdf_configs", json(data.kdf_configs).dump())
          .column_value("size", static_cast<std::int64_t>(data.file_size))
          .column_value("source_path", data.source_path)
          .go();
  if (result.ok()) {
    return api_error::success;
  }

  utils::error::raise_api_path_error(
      function_name, data.api_path, api_error::error,
      fmt::format("failed to add file|{}", result.get_error_str()));
  return api_error::error;
}

void sqlite_file_db::clear() {
  REPERTORY_USES_FUNCTION_NAME();

  auto result = utils::db::sqlite::db_delete{*db_, file_table}.go();
  if (not result.ok()) {
    utils::error::raise_error(function_name,
                              fmt::format("failed to clear file table|{}",
                                          std::to_string(result.get_error())));
  }
}

auto sqlite_file_db::count() const -> std::uint64_t {
  auto result = utils::db::sqlite::db_select{*db_, file_table}
                    .count("api_path", "count")
                    .go();

  std::optional<utils::db::sqlite::db_result::row> row;
  if (result.get_row(row) && row.has_value()) {
    return static_cast<std::uint64_t>(
        row->get_column("count").get_value<std::int64_t>());
  }

  return 0U;
}

void sqlite_file_db::enumerate_item_list(
    std::function<void(const std::vector<i_file_db::file_info> &)> callback,
    stop_type_callback stop_requested_cb) const {
  std::vector<i_file_db::file_info> list;

  auto result = utils::db::sqlite::db_select{*db_, file_table}.go();
  while (not stop_requested_cb() && result.has_row()) {
    std::optional<utils::db::sqlite::db_result::row> row;
    if (result.get_row(row) && row.has_value()) {
      list.emplace_back(i_file_db::file_info{
          .api_path = row->get_column("api_path").get_value<std::string>(),
          .directory =
              row->get_column("directory").get_value<std::int64_t>() == 1,
          .source_path =
              row->get_column("source_path").get_value<std::string>(),
      });

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

auto sqlite_file_db::get_api_path(std::string_view source_path,
                                  std::string &api_path) const -> api_error {
  auto result = utils::db::sqlite::db_select{*db_, file_table}
                    .column("api_path")
                    .where("source_path")
                    .equals(std::string{source_path})
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

auto sqlite_file_db::get_directory_api_path(std::string_view source_path,
                                            std::string &api_path) const
    -> api_error {
  auto result = utils::db::sqlite::db_select{*db_, file_table}
                    .column("api_path")
                    .where("source_path")
                    .equals(std::string{source_path})
                    .and_()
                    .where("directory")
                    .equals(1)
                    .op()
                    .limit(1)
                    .go();

  std::optional<utils::db::sqlite::db_result::row> row;
  if (result.get_row(row) && row.has_value()) {
    api_path = row->get_column("api_path").get_value<std::string>();
    return api_error::success;
  }

  return api_error::directory_not_found;
}

auto sqlite_file_db::get_directory_data(std::string_view api_path,
                                        i_file_db::directory_data &data) const
    -> api_error {
  auto result = utils::db::sqlite::db_select{*db_, file_table}
                    .column("kdf_configs")
                    .column("source_path")
                    .where("api_path")
                    .equals(std::string{api_path})
                    .and_()
                    .where("directory")
                    .equals(0)
                    .op()
                    .limit(1)
                    .go();

  std::optional<utils::db::sqlite::db_result::row> row;
  if (result.get_row(row) && row.has_value()) {
    data.api_path = api_path;
    data.source_path = row->get_column("source_path").get_value<std::string>();

    auto str_data = row->get_column("kdf_configs").get_value<std::string>();
    if (not str_data.empty()) {
      json::parse(str_data).get_to(data.kdf_configs);
    }

    return api_error::success;
  }

  return api_error::directory_not_found;
}

auto sqlite_file_db::get_directory_source_path(std::string_view api_path,
                                               std::string &source_path) const
    -> api_error {
  auto result = utils::db::sqlite::db_select{*db_, file_table}
                    .column("source_path")
                    .where("api_path")
                    .equals(std::string{api_path})
                    .and_()
                    .where("directory")
                    .equals(1)
                    .op()
                    .limit(1)
                    .go();

  std::optional<utils::db::sqlite::db_result::row> row;
  if (result.get_row(row) && row.has_value()) {
    source_path = row->get_column("source_path").get_value<std::string>();
    return api_error::success;
  }

  return api_error::directory_not_found;
}

auto sqlite_file_db::get_file_api_path(std::string_view source_path,
                                       std::string &api_path) const
    -> api_error {
  auto result = utils::db::sqlite::db_select{*db_, file_table}
                    .column("api_path")
                    .where("source_path")
                    .equals(std::string{source_path})
                    .and_()
                    .where("directory")
                    .equals(0)
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

auto sqlite_file_db::get_file_data(std::string_view api_path,
                                   i_file_db::file_data &data) const
    -> api_error {
  auto result = utils::db::sqlite::db_select{*db_, file_table}
                    .column("iv")
                    .column("kdf_configs")
                    .column("size")
                    .column("source_path")
                    .where("api_path")
                    .equals(std::string{api_path})
                    .and_()
                    .where("directory")
                    .equals(0)
                    .op()
                    .limit(1)
                    .go();

  std::optional<utils::db::sqlite::db_result::row> row;
  if (result.get_row(row) && row.has_value()) {
    data.api_path = api_path;
    data.file_size = static_cast<std::uint64_t>(
        row->get_column("size").get_value<std::int64_t>());
    data.source_path = row->get_column("source_path").get_value<std::string>();

    auto str_data = row->get_column("iv").get_value<std::string>();
    if (not str_data.empty()) {
      data.iv_list =
          json::parse(str_data)
              .get<std::vector<
                  std::array<unsigned char,
                             crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>>>();
    }

    str_data = row->get_column("kdf_configs").get_value<std::string>();
    if (not str_data.empty()) {
      json::parse(str_data).get_to(data.kdf_configs);
    }

    return api_error::success;
  }

  return api_error::item_not_found;
}

auto sqlite_file_db::get_file_source_path(std::string_view api_path,
                                          std::string &source_path) const
    -> api_error {
  auto result = utils::db::sqlite::db_select{*db_, file_table}
                    .column("source_path")
                    .where("api_path")
                    .equals(std::string{api_path})
                    .and_()
                    .where("directory")
                    .equals(0)
                    .op()
                    .limit(1)
                    .go();

  std::optional<utils::db::sqlite::db_result::row> row;
  if (result.get_row(row) && row.has_value()) {
    source_path = row->get_column("source_path").get_value<std::string>();
    return api_error::success;
  }

  return api_error::item_not_found;
}

auto sqlite_file_db::get_item_list(stop_type_callback stop_requested_cb) const
    -> std::vector<i_file_db::file_info> {
  std::vector<i_file_db::file_info> ret;

  auto result = utils::db::sqlite::db_select{*db_, file_table}.go();
  while (not stop_requested_cb() && result.has_row()) {
    std::optional<utils::db::sqlite::db_result::row> row;
    if (result.get_row(row) && row.has_value()) {
      ret.emplace_back(i_file_db::file_info{
          .api_path = row->get_column("api_path").get_value<std::string>(),
          .directory =
              row->get_column("directory").get_value<std::int64_t>() == 1,
          .source_path =
              row->get_column("source_path").get_value<std::string>(),
      });
    }
  }

  return ret;
}

auto sqlite_file_db::get_source_path(std::string_view api_path,
                                     std::string &source_path) const
    -> api_error {
  auto result = utils::db::sqlite::db_select{*db_, file_table}
                    .column("source_path")
                    .where("api_path")
                    .equals(std::string{api_path})
                    .op()
                    .limit(1)
                    .go();

  std::optional<utils::db::sqlite::db_result::row> row;
  if (result.get_row(row) && row.has_value()) {
    source_path = row->get_column("source_path").get_value<std::string>();
    return api_error::success;
  }

  return api_error::item_not_found;
}

auto sqlite_file_db::remove_item(std::string_view api_path) -> api_error {
  auto result = utils::db::sqlite::db_delete{*db_, file_table}
                    .where("api_path")
                    .equals(std::string{api_path})
                    .go();

  return result.ok() ? api_error::success : api_error::error;
}
} // namespace repertory
