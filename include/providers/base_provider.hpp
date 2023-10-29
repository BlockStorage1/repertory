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
#ifndef INCLUDE_PROVIDERS_BASE_PROVIDER_HPP_
#define INCLUDE_PROVIDERS_BASE_PROVIDER_HPP_

#include "db/meta_db.hpp"
#include "providers/i_provider.hpp"

namespace repertory {
class app_config;
class i_file_manager;

class base_provider : public i_provider {
public:
  explicit base_provider(app_config &config);

  ~base_provider() override = default;

private:
  app_config &config_;
  std::atomic<std::uint64_t> used_space_{0U};

protected:
  api_item_added_callback api_item_added_;
  std::unique_ptr<meta_db> meta_db_;
  mutable std::recursive_mutex notify_added_mutex_;
  i_file_manager *fm_ = nullptr;
  stop_type stop_requested_ = false;

protected:
  void calculate_used_drive_space(bool add_missing);

  [[nodiscard]] virtual auto
  check_file_exists(const std::string &api_path) const -> api_error = 0;

  void cleanup();

  [[nodiscard]] auto get_config() -> app_config & { return config_; }

  [[nodiscard]] auto get_config() const -> app_config & { return config_; }

  [[nodiscard]] virtual auto
  handle_rename_file(const std::string &from_api_path,
                     const std::string &to_api_path,
                     const std::string &source_path) -> api_error = 0;

  [[nodiscard]] auto notify_directory_added(const std::string &api_path,
                                            const std::string &api_parent) const
      -> api_error {
    return const_cast<base_provider *>(this)->notify_directory_added(
        api_path, api_parent);
  }

  [[nodiscard]] virtual auto
  notify_directory_added(const std::string &api_path,
                         const std::string &api_parent) -> api_error;

  [[nodiscard]] auto notify_file_added(const std::string &api_path,
                                       const std::string &api_parent,
                                       std::uint64_t size) const -> api_error {
    return const_cast<base_provider *>(this)->notify_file_added(
        api_path, api_parent, size);
  }

  [[nodiscard]] virtual auto notify_file_added(const std::string &api_path,
                                               const std::string &api_parent,
                                               std::uint64_t size)
      -> api_error = 0;

  [[nodiscard]] virtual auto
  populate_directory_items(const std::string &api_path,
                           directory_item_list &list) const -> api_error = 0;

  [[nodiscard]] virtual auto populate_file(const std::string &api_path,
                                           api_file &file) const
      -> api_error = 0;

  auto processed_orphaned_file(const std::string &source_path,
                               const std::string &api_path = "") const -> bool;

  void remove_deleted_files();

  void remove_expired_orphaned_files();

  void remove_unknown_source_files();

  [[nodiscard]] auto remove_item_meta(const std::string &api_path)
      -> api_error {
    return meta_db_->remove_item_meta(api_path);
  }

  void update_filesystem_item(bool directory, const api_error &error,
                              const std::string &api_path,
                              filesystem_item &fsi) const;

public:
  [[nodiscard]] auto
  create_directory_clone_source_meta(const std::string &source_api_path,
                                     const std::string &api_path)
      -> api_error override;

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

  [[nodiscard]] auto get_file_size(const std::string &api_path,
                                   std::uint64_t &file_size) const
      -> api_error override;

  [[nodiscard]] auto get_filesystem_item(const std::string &api_path,
                                         bool directory,
                                         filesystem_item &fsi) const
      -> api_error override;

  [[nodiscard]] auto get_filesystem_item_and_file(const std::string &api_path,
                                                  api_file &file,
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
      -> std::vector<std::string> override {
    return meta_db_->get_pinned_files();
  }

  [[nodiscard]] auto get_used_drive_space() const -> std::uint64_t override;

  [[nodiscard]] auto is_file_writeable(const std::string &) const
      -> bool override {
    return true;
  }

  [[nodiscard]] auto remove_item_meta(const std::string &api_path,
                                      const std::string &key)
      -> api_error override {
    return meta_db_->remove_item_meta(api_path, key);
  }

  [[nodiscard]] auto rename_file(const std::string &from_api_path,
                                 const std::string &to_api_path)
      -> api_error override;

  [[nodiscard]] auto set_item_meta(const std::string &api_path,
                                   const std::string &key,
                                   const std::string &value)
      -> api_error override {
    return meta_db_->set_item_meta(api_path, key, value);
  }

  [[nodiscard]] auto set_item_meta(const std::string &api_path,
                                   const api_meta_map &meta)
      -> api_error override {
    return meta_db_->set_item_meta(api_path, meta);
  }

  [[nodiscard]] auto start(api_item_added_callback api_item_added,
                           i_file_manager *fm) -> bool override;

  void stop() override;
};
} // namespace repertory

#endif // INCLUDE_PROVIDERS_BASE_PROVIDER_HPP_
