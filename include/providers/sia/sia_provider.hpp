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
#ifndef INCLUDE_PROVIDERS_SIA_SIA_PROVIDER_HPP_
#define INCLUDE_PROVIDERS_SIA_SIA_PROVIDER_HPP_

#include "providers/i_provider.hpp"
#include "types/repertory.hpp"

namespace repertory {
class app_config;
class i_file_manager;
class i_http_comm;

class sia_provider : public i_provider {
public:
  sia_provider(app_config &config, i_http_comm &comm);

  ~sia_provider() override = default;

private:
  app_config &config_;
  i_http_comm &comm_;

private:
  const std::string DB_NAME = "meta_db";
  api_item_added_callback api_item_added_;
  std::unique_ptr<rocksdb::DB> db_;
  i_file_manager *fm_ = nullptr;

private:
  [[nodiscard]] static auto create_api_file(std::string path,
                                            std::uint64_t size) -> api_file;

  [[nodiscard]] static auto create_api_file(std::string path,
                                            std::uint64_t size,
                                            api_meta_map &meta) -> api_file;

  [[nodiscard]] auto get_object_info(const std::string &api_path,
                                     json &object_info) const -> api_error;

  [[nodiscard]] auto get_object_list(const std::string api_path,
                                     nlohmann::json &object_list) const -> bool;

  void remove_deleted_files();

public:
  [[nodiscard]] auto create_directory(const std::string &api_path,
                                      api_meta_map &meta) -> api_error override;

  [[nodiscard]] auto
  create_directory_clone_source_meta(const std::string &source_api_path,
                                     const std::string &api_path)
      -> api_error override;

  [[nodiscard]] auto get_directory_item_count(const std::string &api_path) const
      -> std::uint64_t override;

  [[nodiscard]] auto create_file(const std::string &api_path,
                                 api_meta_map &meta) -> api_error override;

  [[nodiscard]] auto get_api_path_from_source(const std::string &source_path,
                                              std::string &api_path) const
      -> api_error override;

  [[nodiscard]] auto get_directory_items(const std::string &api_path,
                                         directory_item_list &list) const
      -> api_error override;

  [[nodiscard]] auto get_file(const std::string &api_path, api_file &file) const
      -> api_error override;

  [[nodiscard]] auto get_file_list(api_file_list &list) const
      -> api_error override;

  [[nodiscard]] auto get_file_size(const std::string &api_path,
                                   std::uint64_t &file_size) const
      -> api_error override;

  [[nodiscard]] auto get_filesystem_item(const std::string &api_path,
                                         bool directory,
                                         filesystem_item &fsi) const
      -> api_error override;

  [[nodiscard]] auto get_filesystem_item_and_file(const std::string &api_path,
                                                  api_file &f,
                                                  filesystem_item &fsi) const
      -> api_error override;

  [[nodiscard]] auto
  get_filesystem_item_from_source_path(const std::string &source_path,
                                       filesystem_item &fsi) const
      -> api_error override;

  [[nodiscard]] auto get_item_meta(const std::string &api_path,
                                   api_meta_map &meta) const
      -> api_error override;

  [[nodiscard]] auto get_item_meta(const std::string &api_path,
                                   const std::string &key,
                                   std::string &value) const
      -> api_error override;

  [[nodiscard]] auto get_pinned_files() const
      -> std::vector<std::string> override;

  [[nodiscard]] auto get_provider_type() const -> provider_type override {
    return provider_type::sia;
  }

  [[nodiscard]] auto get_total_drive_space() const -> std::uint64_t override;

  [[nodiscard]] auto get_total_item_count() const -> std::uint64_t override;

  [[nodiscard]] auto get_used_drive_space() const -> std::uint64_t override;

  [[nodiscard]] auto is_direct_only() const -> bool override { return false; }

  [[nodiscard]] auto is_directory(const std::string &api_path,
                                  bool &exists) const -> api_error override;

  [[nodiscard]] auto is_file(const std::string &api_path, bool &exists) const
      -> api_error override;

  [[nodiscard]] auto is_file_writeable(const std::string &api_path) const
      -> bool override;

  [[nodiscard]] auto is_online() const -> bool override;

  [[nodiscard]] auto is_rename_supported() const -> bool override {
    return false;
  }

  [[nodiscard]] auto read_file_bytes(const std::string &api_path,
                                     std::size_t size, std::uint64_t offset,
                                     data_buffer &buffer,
                                     stop_type &stop_requested)
      -> api_error override;

  [[nodiscard]] auto remove_directory(const std::string &api_path)
      -> api_error override;

  [[nodiscard]] auto remove_file(const std::string &api_path)
      -> api_error override;

  [[nodiscard]] auto remove_item_meta(const std::string &api_path,
                                      const std::string &key)
      -> api_error override;

  [[nodiscard]] auto rename_file(const std::string &from_api_path,
                                 const std::string &to_api_path)
      -> api_error override;

  [[nodiscard]] auto set_item_meta(const std::string &api_path,
                                   const std::string &key,
                                   const std::string &value)
      -> api_error override;

  [[nodiscard]] auto set_item_meta(const std::string &api_path,
                                   const api_meta_map &meta)
      -> api_error override;

  [[nodiscard]] auto start(api_item_added_callback api_item_added,
                           i_file_manager *fm) -> bool override;

  void stop() override;

  [[nodiscard]] auto upload_file(const std::string &api_path,
                                 const std::string &source_path,
                                 const std::string & /* encryption_token */,
                                 stop_type &stop_requested)
      -> api_error override;
};
} // namespace repertory

#endif // INCLUDE_PROVIDERS_SIA_SIA_PROVIDER_HPP_
