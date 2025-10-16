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
#ifndef REPERTORY_INCLUDE_DB_I_META_DB_HPP_
#define REPERTORY_INCLUDE_DB_I_META_DB_HPP_

#include "types/repertory.hpp"

namespace repertory {
class i_meta_db {
  INTERFACE_SETUP(i_meta_db);

public:
  virtual void clear() = 0;

  virtual void enumerate_api_path_list(
      std::function<void(const std::vector<std::string> &)> callback,
      stop_type_callback stop_requested_cb) const = 0;

  [[nodiscard]] virtual auto get_api_path(std::string_view source_path,
                                          std::string &api_path) const
      -> api_error = 0;

  [[nodiscard]] virtual auto get_api_path_list() const
      -> std::vector<std::string> = 0;

  [[nodiscard]] virtual auto get_item_meta(std::string_view api_path,
                                           api_meta_map &meta) const
      -> api_error = 0;

  [[nodiscard]] virtual auto get_item_meta(std::string_view api_path,
                                           std::string_view key,
                                           std::string &value) const
      -> api_error = 0;

  [[nodiscard]] virtual auto get_pinned_files() const
      -> std::vector<std::string> = 0;

  [[nodiscard]] virtual auto get_total_item_count() const -> std::uint64_t = 0;

  [[nodiscard]] virtual auto get_total_size() const -> std::uint64_t = 0;

  virtual void remove_api_path(std::string_view api_path) = 0;

  [[nodiscard]] virtual auto remove_item_meta(std::string_view api_path,
                                              std::string_view key)
      -> api_error = 0;

  [[nodiscard]] virtual auto rename_item_meta(std::string_view from_api_path,
                                              std::string_view to_api_path)
      -> api_error = 0;

  [[nodiscard]] virtual auto set_item_meta(std::string_view api_path,
                                           std::string_view key,
                                           std::string_view value)
      -> api_error = 0;

  [[nodiscard]] virtual auto set_item_meta(std::string_view api_path,
                                           const api_meta_map &meta)
      -> api_error = 0;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_DB_I_META_DB_HPP_
