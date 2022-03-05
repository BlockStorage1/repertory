/*
  Copyright <2018-2022> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute,
  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
  OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "utils/rocksdb_utils.hpp"
#include "app_config.hpp"
#include "events/event_system.hpp"
#include "types/startup_exception.hpp"
#include "utils/path_utils.hpp"

namespace repertory::utils::db {
void create_rocksdb(const app_config &config, const std::string &name,
                    std::unique_ptr<rocksdb::DB> &db) {
  rocksdb::Options options{};
  options.create_if_missing = true;
  options.db_log_dir = config.get_log_directory();
  options.keep_log_file_num = 10;

  rocksdb::DB *db_ptr = nullptr;
  const auto status = rocksdb::DB::Open(
      options, utils::path::combine(config.get_data_directory(), {name}), &db_ptr);
  if (status.ok()) {
    db.reset(db_ptr);
  } else {
    event_system::instance().raise<repertory_exception>(__FUNCTION__, status.ToString());
    throw startup_exception(status.ToString());
  }
}

void create_rocksdb(const app_config &config, const std::string &name,
                    const std::vector<rocksdb::ColumnFamilyDescriptor> &families,
                    std::vector<rocksdb::ColumnFamilyHandle *> &handles,
                    std::unique_ptr<rocksdb::DB> &db) {
  rocksdb::Options options{};
  options.create_if_missing = true;
  options.create_missing_column_families = true;
  options.db_log_dir = config.get_log_directory();
  options.keep_log_file_num = 10;

  rocksdb::DB *db_ptr = nullptr;
  const auto status =
      rocksdb::DB::Open(options, utils::path::combine(config.get_data_directory(), {name}),
                        families, &handles, &db_ptr);
  if (status.ok()) {
    db.reset(db_ptr);
  } else {
    event_system::instance().raise<repertory_exception>(__FUNCTION__, status.ToString());
    throw startup_exception(status.ToString());
  }
}
} // namespace repertory::utils::db
