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
#ifndef REPERTORY_INCLUDE_DB_I_FILE_DB_HPP_
#define REPERTORY_INCLUDE_DB_I_FILE_DB_HPP_

#include "types/repertory.hpp"

namespace repertory {
class i_file_db {
  INTERFACE_SETUP(i_file_db);

public:
  struct directory_data final {
    std::string api_path;
    std::pair<utils::encryption::kdf_config, utils::encryption::kdf_config>
        kdf_configs;
    std::string source_path;
  };

  struct file_info final {
    std::string api_path;
    bool directory{};
    std::string source_path;
  };

  struct file_data final {
    std::string api_path;
    std::uint64_t file_size{};
    std::vector<
        std::array<unsigned char, crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>>
        iv_list;
    std::pair<utils::encryption::kdf_config, utils::encryption::kdf_config>
        kdf_configs;
    std::string source_path;
  };

public:
  [[nodiscard]] virtual auto add_or_update_directory(const directory_data &data)
      -> api_error = 0;

  [[nodiscard]] virtual auto add_or_update_file(const file_data &data)
      -> api_error = 0;

  virtual void clear() = 0;

  [[nodiscard]] virtual auto count() const -> std::uint64_t = 0;

  virtual void enumerate_item_list(
      std::function<void(const std::vector<i_file_db::file_info> &)> callback,
      stop_type_callback stop_requested_cb) const = 0;

  [[nodiscard]] virtual auto get_api_path(std::string_view source_path,
                                          std::string &api_path) const
      -> api_error = 0;

  [[nodiscard]] virtual auto
  get_directory_api_path(std::string_view source_path,
                         std::string &api_path) const -> api_error = 0;

  [[nodiscard]] virtual auto get_directory_data(std::string_view api_path,
                                                directory_data &data) const
      -> api_error = 0;

  [[nodiscard]] virtual auto
  get_directory_source_path(std::string_view api_path,
                            std::string &source_path) const -> api_error = 0;

  [[nodiscard]] virtual auto get_file_api_path(std::string_view source_path,
                                               std::string &api_path) const
      -> api_error = 0;

  [[nodiscard]] virtual auto get_file_data(std::string_view api_path,
                                           file_data &data) const
      -> api_error = 0;

  [[nodiscard]] virtual auto
  get_file_source_path(std::string_view api_path,
                       std::string &source_path) const -> api_error = 0;

  [[nodiscard]] virtual auto
  get_item_list(stop_type_callback stop_requested_cb) const
      -> std::vector<file_info> = 0;

  [[nodiscard]] virtual auto get_source_path(std::string_view api_path,
                                             std::string &source_path) const
      -> api_error = 0;

  [[nodiscard]] virtual auto remove_item(std::string_view api_path)
      -> api_error = 0;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_DB_I_FILE_DB_HPP_
