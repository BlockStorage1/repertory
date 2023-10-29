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
#ifndef INCLUDE_DB_DIRECTORY_DB_HPP_
#define INCLUDE_DB_DIRECTORY_DB_HPP_

#include "app_config.hpp"
#include "types/repertory.hpp"
#include "utils/rocksdb_utils.hpp"

namespace repertory {
class directory_db final {
private:
  class directory_tree final {
  private:
    std::unordered_map<std::string, std::vector<std::string>>
        sub_directory_lookup_;

  public:
    void add_path(const std::string &api_path,
                  const std::vector<std::string> &files, rocksdb::DB &db);

    [[nodiscard]] auto get_count(const std::string &api_path) const
        -> std::size_t;

    [[nodiscard]] auto get_directories() const -> std::vector<std::string>;

    [[nodiscard]] auto get_sub_directories(const std::string &api_path) const
        -> std::vector<std::string>;

    [[nodiscard]] auto is_directory(const std::string &api_path) const -> bool;

    void remove_directory(const std::string &api_path, rocksdb::DB &db,
                          bool allow_remove_root = false);
  };

public:
  explicit directory_db(const app_config &config);

public:
  ~directory_db();

private:
  mutable std::recursive_mutex directory_mutex_;
  std::unique_ptr<rocksdb::DB> db_;
  directory_tree tree_;
  const std::string DIRDB_NAME = "directory_db";

private:
  [[nodiscard]] auto get_directory_data(const std::string &api_path) const
      -> json;

public:
  [[nodiscard]] auto create_directory(const std::string &api_path,
                                      bool create_always = false) -> api_error;

  [[nodiscard]] auto create_file(const std::string &api_path) -> api_error;

  [[nodiscard]] auto get_directory_item_count(const std::string &api_path) const
      -> std::uint64_t;

  [[nodiscard]] auto
  get_file(const std::string &api_path, api_file &file,
           api_file_provider_callback api_file_provider) const -> api_error;

  [[nodiscard]] auto
  get_file_list(api_file_list &list,
                api_file_provider_callback api_file_provider) const
      -> api_error;

  [[nodiscard]] auto get_sub_directory_count(const std::string &api_path) const
      -> std::size_t;

  [[nodiscard]] auto get_total_item_count() const -> std::uint64_t;

  [[nodiscard]] auto is_directory(const std::string &api_path) const -> bool;

  [[nodiscard]] auto is_file(const std::string &api_path) const -> bool;

  void populate_directory_files(const std::string &api_path,
                                meta_provider_callback meta_provider,
                                directory_item_list &list) const;

  void populate_sub_directories(const std::string &api_path,
                                meta_provider_callback meta_provider,
                                directory_item_list &list) const;

  [[nodiscard]] auto remove_directory(const std::string &api_path,
                                      bool allow_remove_root = false)
      -> api_error;

  [[nodiscard]] auto remove_file(const std::string &api_path) -> bool;

  [[nodiscard]] auto rename_file(const std::string &from_api_path,
                                 const std::string &to_api_path) -> api_error;
};
} // namespace repertory

#endif // INCLUDE_DB_DIRECTORY_DB_HPP_
