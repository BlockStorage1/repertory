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
#ifndef REPERTORY_INCLUDE_PROVIDERS_I_PROVIDER_HPP_
#define REPERTORY_INCLUDE_PROVIDERS_I_PROVIDER_HPP_

#include "types/repertory.hpp"

namespace repertory {
class i_file_manager;

class i_provider {
  INTERFACE_SETUP(i_provider);

public:
  [[nodiscard]] virtual auto check_version(std::string &required_version,
                                           std::string &returned_version) const
      -> bool = 0;

  [[nodiscard]] virtual auto create_directory(const std::string &api_path,
                                              api_meta_map &meta)
      -> api_error = 0;

  [[nodiscard]] virtual auto
  create_directory_clone_source_meta(const std::string &source_api_path,
                                     const std::string &api_path)
      -> api_error = 0;

  [[nodiscard]] virtual auto create_file(const std::string &api_path,
                                         api_meta_map &meta) -> api_error = 0;

  [[nodiscard]] virtual auto
  get_api_path_from_source(const std::string &source_path,
                           std::string &api_path) const -> api_error = 0;

  [[nodiscard]] virtual auto
  get_directory_item_count(const std::string &api_path) const
      -> std::uint64_t = 0;

  [[nodiscard]] virtual auto
  get_directory_items(const std::string &api_path,
                      directory_item_list &list) const -> api_error = 0;

  [[nodiscard]] virtual auto get_file(const std::string &api_path,
                                      api_file &file) const -> api_error = 0;

  [[nodiscard]] virtual auto get_file_list(api_file_list &list,
                                           std::string &marker) const
      -> api_error = 0;

  [[nodiscard]] virtual auto get_file_size(const std::string &api_path,
                                           std::uint64_t &file_size) const
      -> api_error = 0;

  [[nodiscard]] virtual auto get_filesystem_item(const std::string &api_path,
                                                 bool directory,
                                                 filesystem_item &fsi) const
      -> api_error = 0;

  [[nodiscard]] virtual auto
  get_filesystem_item_and_file(const std::string &api_path, api_file &file,
                               filesystem_item &fsi) const -> api_error = 0;

  [[nodiscard]] virtual auto
  get_filesystem_item_from_source_path(const std::string &source_path,
                                       filesystem_item &fsi) const
      -> api_error = 0;

  [[nodiscard]] virtual auto get_item_meta(const std::string &api_path,
                                           api_meta_map &meta) const
      -> api_error = 0;

  [[nodiscard]] virtual auto get_item_meta(const std::string &api_path,
                                           const std::string &key,
                                           std::string &value) const
      -> api_error = 0;

  [[nodiscard]] virtual auto get_pinned_files() const
      -> std::vector<std::string> = 0;

  [[nodiscard]] virtual auto get_provider_type() const -> provider_type = 0;

  [[nodiscard]] virtual auto get_total_drive_space() const -> std::uint64_t = 0;

  [[nodiscard]] virtual auto get_total_item_count() const -> std::uint64_t = 0;

  [[nodiscard]] virtual auto get_used_drive_space() const -> std::uint64_t = 0;

  [[nodiscard]] virtual auto is_directory(const std::string &api_path,
                                          bool &exists) const -> api_error = 0;

  [[nodiscard]] virtual auto is_file(const std::string &api_path,
                                     bool &exists) const -> api_error = 0;

  [[nodiscard]] virtual auto
  is_file_writeable(const std::string &api_path) const -> bool = 0;

  [[nodiscard]] virtual auto is_online() const -> bool = 0;

  [[nodiscard]] virtual auto is_read_only() const -> bool = 0;

  [[nodiscard]] virtual auto is_rename_supported() const -> bool = 0;

  [[nodiscard]] virtual auto
  read_file_bytes(const std::string &api_path, std::size_t size,
                  std::uint64_t offset, data_buffer &data,
                  stop_type &stop_requested) -> api_error = 0;

  [[nodiscard]] virtual auto remove_directory(const std::string &api_path)
      -> api_error = 0;

  [[nodiscard]] virtual auto remove_file(const std::string &api_path)
      -> api_error = 0;

  [[nodiscard]] virtual auto remove_item_meta(const std::string &api_path,
                                              const std::string &key)
      -> api_error = 0;

  [[nodiscard]] virtual auto rename_file(const std::string &from_api_path,
                                         const std::string &to_api_path)
      -> api_error = 0;

  [[nodiscard]] virtual auto set_item_meta(const std::string &api_path,
                                           const std::string &key,
                                           const std::string &value)
      -> api_error = 0;

  [[nodiscard]] virtual auto set_item_meta(const std::string &api_path,
                                           const api_meta_map &meta)
      -> api_error = 0;

  [[nodiscard]] virtual auto start(api_item_added_callback api_item_added,
                                   i_file_manager *mgr) -> bool = 0;

  virtual void stop() = 0;

  [[nodiscard]] virtual auto upload_file(const std::string &api_path,
                                         const std::string &source_path,
                                         stop_type &stop_requested)
      -> api_error = 0;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_PROVIDERS_I_PROVIDER_HPP_
