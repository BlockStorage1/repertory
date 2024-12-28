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
#include "utils/utils.hpp"

#include "app_config.hpp"
#include "types/startup_exception.hpp"
#include "utils/common.hpp"
#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"

namespace repertory::utils {
void calculate_allocation_size(bool directory, std::uint64_t file_size,
                               UINT64 allocation_size,
                               std::string &allocation_meta_size) {
  if (directory) {
    allocation_meta_size = "0";
    return;
  }

  if (file_size > allocation_size) {
    allocation_size = file_size;
  }

  allocation_size =
      utils::divide_with_ceiling(allocation_size, WINFSP_ALLOCATION_UNIT) *
      WINFSP_ALLOCATION_UNIT;
  allocation_meta_size = std::to_string(allocation_size);
}

auto create_rocksdb(
    const app_config &cfg, const std::string &name,
    const std::vector<rocksdb::ColumnFamilyDescriptor> &families,
    std::vector<rocksdb::ColumnFamilyHandle *> &handles,
    bool clear) -> std::unique_ptr<rocksdb::TransactionDB> {
  REPERTORY_USES_FUNCTION_NAME();

  auto db_dir = utils::path::combine(cfg.get_data_directory(), {"db"});
  if (not utils::file::directory{db_dir}.create_directory()) {
    throw startup_exception(
        fmt::format("failed to create db directory|", db_dir));
  }

  auto path = utils::path::combine(db_dir, {name});
  if (clear && not utils::file::directory{path}.remove_recursively()) {
    utils::error::raise_error(function_name,
                              "failed to remove " + name + " db|" + path);
  }

  rocksdb::Options options{};
  options.create_if_missing = true;
  options.create_missing_column_families = true;
  options.db_log_dir = cfg.get_log_directory();
  options.keep_log_file_num = 10;

  rocksdb::TransactionDB *ptr{};
  auto status = rocksdb::TransactionDB::Open(
      options, rocksdb::TransactionDBOptions{}, path, families, &handles, &ptr);
  if (not status.ok()) {
    throw startup_exception(fmt::format("failed to open rocksdb|path{}|error{}",
                                        path, status.ToString()));
  }

  return std::unique_ptr<rocksdb::TransactionDB>(ptr);
}

auto create_volume_label(const provider_type &prov) -> std::string {
  return "repertory_" + app_config::get_provider_name(prov);
}

auto get_attributes_from_meta(const api_meta_map &meta) -> DWORD {
  return static_cast<DWORD>(utils::string::to_uint32(meta.at(META_ATTRIBUTES)));
}
} // namespace repertory::utils
