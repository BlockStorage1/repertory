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
#include "db/impl/sqlite_file_mgr_db.hpp"

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
const std::string resume_table = "resume";
const std::string upload_table = "upload";
const std::string upload_active_table = "upload_active";
const std::map<std::string, std::string> sql_create_tables{
    {
        {resume_table},
        {
            "CREATE TABLE IF NOT EXISTS " + resume_table +
                "("
                "api_path TEXT PRIMARY KEY ASC, "
                "chunk_size INTEGER, "
                "read_state TEXT, "
                "source_path TEXT"
                ");",
        },
    },
    {
        {upload_table},
        {
            "CREATE TABLE IF NOT EXISTS " + upload_table +
                "("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "api_path TEXT UNIQUE, "
                "source_path TEXT"
                ");",
        },
    },
    {
        {upload_active_table},
        {
            "CREATE TABLE IF NOT EXISTS " + upload_active_table +
                "("
                "api_path TEXT PRIMARY KEY ASC, "
                "source_path TEXT"
                ");",
        },
    },
};
} // namespace

namespace repertory {
sqlite_file_mgr_db::sqlite_file_mgr_db(const app_config &cfg) {
  auto db_dir = utils::path::combine(cfg.get_data_directory(), {"db"});
  if (not utils::file::directory{db_dir}.create_directory()) {
    throw startup_exception(
        fmt::format("failed to create db directory|", db_dir));
  }

  db_ = utils::db::sqlite::create_db(
      utils::path::combine(db_dir, {"file_mgr.db"}), sql_create_tables);
}

sqlite_file_mgr_db::~sqlite_file_mgr_db() { db_.reset(); }

auto sqlite_file_mgr_db::add_resume(const resume_entry &entry) -> bool {
  return utils::db::sqlite::db_insert{*db_, resume_table}
      .or_replace()
      .column_value("api_path", entry.api_path)
      .column_value("chunk_size", static_cast<std::int64_t>(entry.chunk_size))
      .column_value("read_state",
                    utils::string::from_dynamic_bitset(entry.read_state))
      .column_value("source_path", entry.source_path)
      .go()
      .ok();
}

auto sqlite_file_mgr_db::add_upload(const upload_entry &entry) -> bool {
  return utils::db::sqlite::db_insert{*db_, upload_table}
      .or_replace()
      .column_value("api_path", entry.api_path)
      .column_value("source_path", entry.source_path)
      .go()
      .ok();
}

auto sqlite_file_mgr_db::add_upload_active(const upload_active_entry &entry)
    -> bool {
  return utils::db::sqlite::db_insert{*db_, upload_active_table}
      .or_replace()
      .column_value("api_path", entry.api_path)
      .column_value("source_path", entry.source_path)
      .go()
      .ok();
}

void sqlite_file_mgr_db::clear() {
  REPERTORY_USES_FUNCTION_NAME();

  auto result = utils::db::sqlite::db_delete{*db_, resume_table}.go();
  if (not result.ok()) {
    utils::error::raise_error(function_name,
                              "failed to clear resume table|" +
                                  std::to_string(result.get_error()));
  }

  result = utils::db::sqlite::db_delete{*db_, upload_active_table}.go();
  if (not result.ok()) {
    utils::error::raise_error(function_name,
                              "failed to clear upload active table|" +
                                  std::to_string(result.get_error()));
  }

  result = utils::db::sqlite::db_delete{*db_, upload_table}.go();
  if (not result.ok()) {
    utils::error::raise_error(function_name,
                              "failed to clear upload table|" +
                                  std::to_string(result.get_error()));
  }
}

auto sqlite_file_mgr_db::get_next_upload() const
    -> std::optional<upload_entry> {
  auto result = utils::db::sqlite::db_select{*db_, upload_table}
                    .order_by("id", true)
                    .limit(1)
                    .go();
  std::optional<utils::db::sqlite::db_result::row> row;
  if (not result.get_row(row) || not row.has_value()) {
    return std::nullopt;
  }

  return upload_entry{
      row->get_column("api_path").get_value<std::string>(),
      row->get_column("source_path").get_value<std::string>(),
  };
}

auto sqlite_file_mgr_db::get_resume_list() const -> std::vector<resume_entry> {
  REPERTORY_USES_FUNCTION_NAME();

  std::vector<resume_entry> ret;
  auto result = utils::db::sqlite::db_select{*db_, resume_table}.go();
  while (result.has_row()) {
    try {
      std::optional<utils::db::sqlite::db_result::row> row;
      if (not result.get_row(row)) {
        continue;
      }
      if (not row.has_value()) {
        continue;
      }

      ret.push_back(resume_entry{
          row->get_column("api_path").get_value<std::string>(),
          static_cast<std::uint64_t>(
              row->get_column("chunk_size").get_value<std::int64_t>()),
          utils::string::to_dynamic_bitset(
              row->get_column("read_state").get_value<std::string>()),
          row->get_column("source_path").get_value<std::string>(),
      });
    } catch (const std::exception &ex) {
      utils::error::raise_error(function_name, ex, "query error");
    }
  }

  return ret;
}

auto sqlite_file_mgr_db::get_upload(const std::string &api_path) const
    -> std::optional<upload_entry> {
  auto result = utils::db::sqlite::db_select{*db_, upload_table}
                    .where("api_path")
                    .equals(api_path)
                    .go();
  std::optional<utils::db::sqlite::db_result::row> row;
  if (not result.get_row(row) || not row.has_value()) {
    return std::nullopt;
  }

  return upload_entry{
      row->get_column("api_path").get_value<std::string>(),
      row->get_column("source_path").get_value<std::string>(),
  };
}

auto sqlite_file_mgr_db::get_upload_active_list() const
    -> std::vector<upload_active_entry> {
  REPERTORY_USES_FUNCTION_NAME();

  std::vector<upload_active_entry> ret;
  auto result = utils::db::sqlite::db_select{*db_, upload_active_table}.go();
  while (result.has_row()) {
    try {
      std::optional<utils::db::sqlite::db_result::row> row;
      if (not result.get_row(row)) {
        continue;
      }
      if (not row.has_value()) {
        continue;
      }

      ret.push_back(upload_active_entry{
          row->get_column("api_path").get_value<std::string>(),
          row->get_column("source_path").get_value<std::string>(),
      });
    } catch (const std::exception &ex) {
      utils::error::raise_error(function_name, ex, "query error");
    }
  }

  return ret;
}

auto sqlite_file_mgr_db::remove_resume(const std::string &api_path) -> bool {
  return utils::db::sqlite::db_delete{*db_, resume_table}
      .where("api_path")
      .equals(api_path)
      .go()
      .ok();
}

auto sqlite_file_mgr_db::remove_upload(const std::string &api_path) -> bool {
  return utils::db::sqlite::db_delete{*db_, upload_table}
      .where("api_path")
      .equals(api_path)
      .go()
      .ok();
}

auto sqlite_file_mgr_db::remove_upload_active(const std::string &api_path)
    -> bool {
  return utils::db::sqlite::db_delete{*db_, upload_active_table}
      .where("api_path")
      .equals(api_path)
      .go()
      .ok();
}

auto sqlite_file_mgr_db::rename_resume(const std::string &from_api_path,
                                       const std::string &to_api_path) -> bool {
  return utils::db::sqlite::db_update{*db_, resume_table}
      .column_value("api_path", to_api_path)
      .where("api_path")
      .equals(from_api_path)
      .go()
      .ok();
}
} // namespace repertory
