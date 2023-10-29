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
#ifndef INCLUDE_DB_META_DB_HPP_
#define INCLUDE_DB_META_DB_HPP_

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
  rocksdb::ColumnFamilyHandle *default_family_{};
  rocksdb::ColumnFamilyHandle *source_family_{};
  rocksdb::ColumnFamilyHandle *keys_family_{};
  const std::string METADB_NAME = "meta_db";

private:
  [[nodiscard]] auto
  perform_action(const std::string &function_name,
                 const std::function<rocksdb::Status()> &action) const
      -> api_error;

  [[nodiscard]] auto get_item_meta_json(const std::string &api_path,
                                        json &json_data) const -> api_error;

  [[nodiscard]] auto store_item_meta(const std::string &api_path,
                                     const std::string &key,
                                     const std::string &value) -> api_error;

public:
  [[nodiscard]] auto create_iterator(bool source_family) const
      -> std::shared_ptr<rocksdb::Iterator>;

  [[nodiscard]] auto get_api_path_from_key(const std::string &key,
                                           std::string &api_path) const
      -> api_error;

  [[nodiscard]] auto get_api_path_from_source(const std::string &source_path,
                                              std::string &api_path) const
      -> api_error;

  [[nodiscard]] auto get_item_meta(const std::string &api_path,
                                   api_meta_map &meta) const -> api_error;

  [[nodiscard]] auto get_item_meta(const std::string &api_path,
                                   const std::string &key,
                                   std::string &value) const -> api_error;

  [[nodiscard]] auto get_item_meta_exists(const std::string &api_path) const
      -> bool;

  [[nodiscard]] auto get_total_item_count() const -> std::uint64_t;

  [[nodiscard]] auto get_pinned_files() const -> std::vector<std::string>;

  [[nodiscard]] auto
  get_source_path_exists(const std::string &source_path) const -> bool;

  [[nodiscard]] auto remove_item_meta(const std::string &api_path) -> api_error;

  [[nodiscard]] auto remove_item_meta(const std::string &api_path,
                                      const std::string &key) -> api_error;

  [[nodiscard]] auto rename_item_meta(const std::string &source_path,
                                      const std::string &from_api_path,
                                      const std::string &to_api_path)
      -> api_error;

  [[nodiscard]] auto set_item_meta(const std::string &api_path,
                                   const std::string &key,
                                   const std::string &value) -> api_error;

  [[nodiscard]] auto set_item_meta(const std::string &api_path,
                                   const api_meta_map &meta) -> api_error;

  [[nodiscard]] auto set_source_path(const std::string &api_path,
                                     const std::string &source_path)
      -> api_error;
};
} // namespace repertory

#endif // INCLUDE_DB_META_DB_HPP_
