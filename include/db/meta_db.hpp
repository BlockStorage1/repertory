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
#ifndef INCLUDE_DB_META_DB_HPP_
#define INCLUDE_DB_META_DB_HPP_

#include "common.hpp"
#include "app_config.hpp"
#include "types/repertory.hpp"
#include "utils/rocksdb_utils.hpp"

namespace repertory {
class meta_db final {
public:
  explicit meta_db(const app_config &config);

public:
  ~meta_db();

private:
  std::unique_ptr<rocksdb::DB> db_;
  std::unique_ptr<rocksdb::ColumnFamilyHandle> default_family_;
  std::unique_ptr<rocksdb::ColumnFamilyHandle> source_family_;
  std::unique_ptr<rocksdb::ColumnFamilyHandle> keys_family_;
  const std::string METADB_NAME = "meta_db";

private:
  api_error get_item_meta_json(const std::string &api_path, json &json_data) const;

  void release_resources();

public:
  std::shared_ptr<rocksdb::Iterator> create_iterator(const bool &source_family);

  api_error get_api_path_from_key(const std::string &key, std::string &api_path) const;

  api_error get_api_path_from_source(const std::string &source_path, std::string &api_path) const;

  api_error get_item_meta(const std::string &api_path, api_meta_map &meta) const;

  api_error get_item_meta(const std::string &api_path, const std::string &key,
                          std::string &value) const;

  bool get_item_meta_exists(const std::string &api_path) const;

  std::vector<std::string> get_pinned_files() const;

  bool get_source_path_exists(const std::string &source_path) const;

  void remove_item_meta(const std::string &api_path);

  api_error remove_item_meta(const std::string &api_path, const std::string &key);

  api_error rename_item_meta(const std::string &source_path, const std::string &from_api_path,
                             const std::string &to_api_path);

  api_error set_item_meta(const std::string &api_path, const std::string &key,
                          const std::string &value);

  api_error set_item_meta(const std::string &api_path, const api_meta_map &meta);

  api_error set_source_path(const std::string &api_path, const std::string &source_path);
};
} // namespace repertory

#endif // INCLUDE_DB_META_DB_HPP_
