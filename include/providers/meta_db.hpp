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
#ifndef INCLUDE_PROVIDERS_META_DB_HPP_
#define INCLUDE_PROVIDERS_META_DB_HPP_

#include "types/repertory.hpp"

namespace repertory {
class app_config;

class meta_db final {
public:
  meta_db(const app_config &cfg);
  ~meta_db();

  meta_db(const meta_db &) = delete;
  meta_db(meta_db &&) = delete;
  auto operator=(const meta_db &) -> meta_db & = delete;
  auto operator=(meta_db &&) -> meta_db & = delete;

private:
  db3_t db_;
  constexpr static const auto table_name = "meta";

private:
  [[nodiscard]] auto update_item_meta(const std::string &api_path,
                                      api_meta_map meta) -> api_error;

public:
  [[nodiscard]] auto get_api_path(const std::string &source_path,
                                  std::string &api_path) -> api_error;

  [[nodiscard]] auto get_api_path_list() -> std::vector<std::string>;

  [[nodiscard]] auto get_item_meta(const std::string &api_path,
                                   api_meta_map &meta) -> api_error;

  [[nodiscard]] auto get_item_meta(const std::string &api_path,
                                   const std::string &key,
                                   std::string &value) const -> api_error;

  [[nodiscard]] auto get_pinned_files() const -> std::vector<std::string>;

  [[nodiscard]] auto get_total_item_count() const -> std::uint64_t;

  void remove_api_path(const std::string &api_path);

  [[nodiscard]] auto remove_item_meta(const std::string &api_path,
                                      const std::string &key) -> api_error;

  [[nodiscard]] auto rename_item_meta(const std::string &from_api_path,
                                      const std::string &to_api_path)
      -> api_error;

  [[nodiscard]] auto set_item_meta(const std::string &api_path,
                                   const std::string &key,
                                   const std::string &value) -> api_error;

  [[nodiscard]] auto set_item_meta(const std::string &api_path,
                                   const api_meta_map &meta) -> api_error;
};
} // namespace repertory

#endif // INCLUDE_PROVIDERS_META_DB_HPP_
