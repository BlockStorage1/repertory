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
#ifndef REPERTORY_INCLUDE_PROVIDERS_ENCRYPT_ENCRYPT_PROVIDER_HPP_
#define REPERTORY_INCLUDE_PROVIDERS_ENCRYPT_ENCRYPT_PROVIDER_HPP_

#include "app_config.hpp"
#include "db/i_file_db.hpp"
#include "providers/i_provider.hpp"
#include "utils/encrypting_reader.hpp"

namespace repertory {
class encrypt_provider final : public i_provider {
public:
  static const constexpr auto type{provider_type::encrypt};

public:
  explicit encrypt_provider(app_config &config);

  ~encrypt_provider() override = default;

public:
  encrypt_provider(const encrypt_provider &) = delete;
  encrypt_provider(encrypt_provider &&) = delete;
  auto operator=(const encrypt_provider &) -> encrypt_provider & = delete;
  auto operator=(encrypt_provider &&) -> encrypt_provider & = delete;

private:
  struct reader_info final {
    std::chrono::system_clock::time_point last_access_time{
        std::chrono::system_clock::now(),
    };
    std::unique_ptr<utils::encryption::encrypting_reader> reader;
    std::mutex reader_mtx;
  };

private:
  app_config &config_;
  encrypt_config encrypt_config_;

private:
  std::unique_ptr<i_file_db> file_db_{nullptr};
  utils::hash::hash_256_t master_key_{};
  std::unordered_map<std::string, std::shared_ptr<reader_info>> reader_lookup_;
  std::recursive_mutex reader_lookup_mtx_;

private:
  [[nodiscard]] static auto create_api_file(std::string_view api_path,
                                            bool directory,
                                            std::string_view source_path)
      -> api_file;

  static void create_item_meta(api_meta_map &meta, bool directory,
                               const api_file &file);

  [[nodiscard]] auto do_fs_operation(
      std::string_view api_path, bool directory,
      std::function<api_error(const encrypt_config &cfg,
                              std::string_view source_path)> callback) const
      -> api_error;

  [[nodiscard]] auto get_encrypt_config() const -> const encrypt_config & {
    return encrypt_config_;
  }

  auto process_directory_entry(const utils::file::i_fs_item &dir_entry,
                               const encrypt_config &cfg,
                               std::string &api_path) const -> bool;

  void remove_deleted_files(stop_type &stop_requested);

  void remove_expired_files();

public:
  [[nodiscard]] auto check_version(std::string &required_version,
                                   std::string &returned_version) const
      -> bool override {
    required_version = returned_version = "";
    return true;
  }

  [[nodiscard]] auto create_directory(std::string_view api_path,
                                      api_meta_map &meta) -> api_error override;

  [[nodiscard]] auto
  create_directory_clone_source_meta(std::string_view /*source_api_path*/,
                                     std::string_view /*api_path*/)
      -> api_error override {
    return api_error::not_implemented;
  }

  [[nodiscard]] auto create_file(std::string_view /*api_path*/,
                                 api_meta_map & /*meta*/)
      -> api_error override {
    return api_error::not_implemented;
  }

  [[nodiscard]] auto get_api_path_from_source(std::string_view /*source_path*/,
                                              std::string & /*api_path*/) const
      -> api_error override;

  [[nodiscard]] auto get_directory_item_count(std::string_view api_path) const
      -> std::uint64_t override;

  [[nodiscard]] auto get_directory_item(std::string_view api_path,
                                        directory_item &item) const
      -> api_error override;

  [[nodiscard]] auto get_directory_items(std::string_view api_path,
                                         directory_item_list &list) const
      -> api_error override;

  [[nodiscard]] auto get_file(std::string_view api_path, api_file &file) const
      -> api_error override;

  [[nodiscard]] auto get_file_list(api_file_list &list,
                                   std::string &marker) const
      -> api_error override;

  [[nodiscard]] auto get_file_size(std::string_view api_path,
                                   std::uint64_t &file_size) const
      -> api_error override;

  [[nodiscard]] auto get_filesystem_item(std::string_view api_path,
                                         bool directory,
                                         filesystem_item &fsi) const
      -> api_error override;

  [[nodiscard]] auto
  get_filesystem_item_from_source_path(std::string_view source_path,
                                       filesystem_item &fsi) const
      -> api_error override;

  [[nodiscard]] auto get_pinned_files() const
      -> std::vector<std::string> override;

  [[nodiscard]] auto get_item_meta(std::string_view api_path,
                                   api_meta_map &meta) const
      -> api_error override;

  [[nodiscard]] auto get_item_meta(std::string_view api_path,
                                   std::string_view key,
                                   std::string &value) const
      -> api_error override;

  [[nodiscard]] auto get_total_drive_space() const -> std::uint64_t override;

  [[nodiscard]] auto get_total_item_count() const -> std::uint64_t override;

  [[nodiscard]] auto get_provider_type() const -> provider_type override {
    return type;
  }

  [[nodiscard]] auto get_used_drive_space() const -> std::uint64_t override;

  [[nodiscard]] auto is_directory(std::string_view api_path, bool &exists) const
      -> api_error override;

  [[nodiscard]] auto is_file(std::string_view api_path, bool &exists) const
      -> api_error override;

  [[nodiscard]] auto is_online() const -> bool override;

  [[nodiscard]] auto is_read_only() const -> bool override { return true; }

  [[nodiscard]] auto is_rename_supported() const -> bool override {
    return false;
  }

  [[nodiscard]] auto read_file_bytes(std::string_view api_path,
                                     std::size_t size, std::uint64_t offset,
                                     data_buffer &data,
                                     stop_type &stop_requested)
      -> api_error override;

  [[nodiscard]] auto remove_directory(std::string_view /*api_path*/)
      -> api_error override {
    return api_error::not_implemented;
  }

  [[nodiscard]] auto remove_file(std::string_view /*api_path*/)
      -> api_error override {
    return api_error::not_implemented;
  }

  [[nodiscard]] auto remove_item_meta(std::string_view api_path,
                                      std::string_view key)
      -> api_error override;

  [[nodiscard]] auto rename_file(std::string_view /*from_api_path*/,
                                 std::string_view /*to_api_path*/)
      -> api_error override {
    return api_error::not_implemented;
  }

  [[nodiscard]] auto set_item_meta(std::string_view /*api_path*/,
                                   std::string_view /*key*/,
                                   std::string_view /*value*/)
      -> api_error override {
    return api_error::success;
  }

  [[nodiscard]] auto set_item_meta(std::string_view /*api_path*/,
                                   api_meta_map /*meta*/)
      -> api_error override {
    return api_error::success;
  }

  [[nodiscard]] auto start(api_item_added_callback api_item_added,
                           i_file_manager *mgr) -> bool override;

  void stop() override;

  [[nodiscard]] auto upload_file(std::string_view /*api_path*/,
                                 std::string_view /*source_path*/,
                                 stop_type & /*stop_requested*/)
      -> api_error override {
    return api_error::not_implemented;
  }
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_PROVIDERS_ENCRYPT_ENCRYPT_PROVIDER_HPP_
