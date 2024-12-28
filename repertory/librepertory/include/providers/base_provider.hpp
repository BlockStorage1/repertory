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
#ifndef REPERTORY_INCLUDE_PROVIDERS_BASE_PROVIDER_HPP_
#define REPERTORY_INCLUDE_PROVIDERS_BASE_PROVIDER_HPP_

#include "db/i_meta_db.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"

namespace repertory {
class app_config;
class i_file_manager;
class i_http_comm;

class base_provider : public i_provider {
private:
  struct removed_item final {
    std::string api_path;
    bool directory{};
    std::string source_path;
  };

public:
  base_provider(app_config &config, i_http_comm &comm)
      : config_(config), comm_(comm) {}

private:
  app_config &config_;
  i_http_comm &comm_;

private:
  api_item_added_callback api_item_added_;
  std::unique_ptr<i_meta_db> db3_;
  i_file_manager *fm_{};

private:
  void add_all_items(const stop_type &stop_requested);

  void process_removed_directories(std::deque<removed_item> removed_list,
                                   const stop_type &stop_requested);

  void process_removed_files(std::deque<removed_item> removed_list,
                             const stop_type &stop_requested);

  void process_removed_items(const stop_type &stop_requested);

  void remove_deleted_items(const stop_type &stop_requested);

  void remove_unmatched_source_files(const stop_type &stop_requested);

protected:
  [[nodiscard]] static auto create_api_file(std::string path, std::string key,
                                            std::uint64_t size,
                                            std::uint64_t file_time)
      -> api_file;

  [[nodiscard]] static auto create_api_file(std::string path,
                                            std::uint64_t size,
                                            api_meta_map &meta) -> api_file;

  [[nodiscard]] virtual auto create_directory_impl(const std::string &api_path,
                                                   api_meta_map &meta)
      -> api_error = 0;

  [[nodiscard]] virtual auto
  create_file_extra(const std::string & /* api_path */,
                    api_meta_map & /* meta */) -> api_error {
    return api_error::success;
  }

  [[nodiscard]] auto get_api_item_added() -> api_item_added_callback & {
    return api_item_added_;
  }

  [[nodiscard]] auto get_api_item_added() const
      -> const api_item_added_callback & {
    return api_item_added_;
  }

  [[nodiscard]] auto get_comm() -> i_http_comm & { return comm_; }

  [[nodiscard]] auto get_comm() const -> const i_http_comm & { return comm_; }

  [[nodiscard]] auto get_config() -> app_config & { return config_; }

  [[nodiscard]] auto get_config() const -> const app_config & {
    return config_;
  }

  [[nodiscard]] auto get_db() -> i_meta_db & { return *db3_; }

  [[nodiscard]] auto get_db() const -> const i_meta_db & { return *db3_; }

  [[nodiscard]] virtual auto
  get_directory_items_impl(const std::string &api_path,
                           directory_item_list &list) const -> api_error = 0;

  [[nodiscard]] auto get_file_mgr() -> i_file_manager * { return fm_; }

  [[nodiscard]] auto get_file_mgr() const -> const i_file_manager * {
    return fm_;
  }

  [[nodiscard]] virtual auto remove_directory_impl(const std::string &api_path)
      -> api_error = 0;

  [[nodiscard]] virtual auto remove_file_impl(const std::string &api_path)
      -> api_error = 0;

  [[nodiscard]] virtual auto upload_file_impl(const std::string &api_path,
                                              const std::string &source_path,
                                              stop_type &stop_requested)
      -> api_error = 0;

public:
  [[nodiscard]] auto
  create_directory_clone_source_meta(const std::string &source_api_path,
                                     const std::string &api_path)
      -> api_error override;

  [[nodiscard]] auto create_directory(const std::string &api_path,
                                      api_meta_map &meta) -> api_error override;

  [[nodiscard]] auto create_file(const std::string &api_path,
                                 api_meta_map &meta) -> api_error override;

  [[nodiscard]] auto get_api_path_from_source(const std::string &source_path,
                                              std::string &api_path) const
      -> api_error override;

  [[nodiscard]] auto get_directory_items(const std::string &api_path,
                                         directory_item_list &list) const
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

  [[nodiscard]] auto get_total_item_count() const -> std::uint64_t override;

  [[nodiscard]] auto get_used_drive_space() const -> std::uint64_t override;

  [[nodiscard]] auto is_file_writeable(const std::string &api_path) const
      -> bool override;

  [[nodiscard]] auto is_read_only() const -> bool override { return false; }

  [[nodiscard]] auto remove_directory(const std::string &api_path)
      -> api_error override;

  [[nodiscard]] auto remove_file(const std::string &api_path)
      -> api_error override;

  [[nodiscard]] auto remove_item_meta(const std::string &api_path,
                                      const std::string &key)
      -> api_error override;

  [[nodiscard]] auto set_item_meta(const std::string &api_path,
                                   const std::string &key,
                                   const std::string &value)
      -> api_error override;

  [[nodiscard]] auto set_item_meta(const std::string &api_path,
                                   const api_meta_map &meta)
      -> api_error override;

  [[nodiscard]] auto start(api_item_added_callback api_item_added,
                           i_file_manager *mgr) -> bool override;

  void stop() override;

  [[nodiscard]] auto upload_file(const std::string &api_path,
                                 const std::string &source_path,
                                 stop_type &stop_requested)
      -> api_error override;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_PROVIDERS_BASE_PROVIDER_HPP_
