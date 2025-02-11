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
#ifndef REPERTORY_INCLUDE_FILE_MANAGER_FILE_MANAGER_HPP_
#define REPERTORY_INCLUDE_FILE_MANAGER_FILE_MANAGER_HPP_

#include "db/i_file_mgr_db.hpp"
#include "events/event_system.hpp"
#include "events/types/file_upload_completed.hpp"
#include "file_manager/i_file_manager.hpp"
#include "file_manager/i_open_file.hpp"
#include "file_manager/i_upload_manager.hpp"
#include "file_manager/upload.hpp"
#include "types/repertory.hpp"
#include "utils/file.hpp"

namespace repertory {
class app_config;
class i_provider;

class file_manager final : public i_file_manager, public i_upload_manager {
  E_CONSUMER();

private:
  static constexpr const std::chrono::seconds queue_wait_secs{
      5s,
  };

public:
  file_manager(app_config &config, i_provider &provider);

  ~file_manager() override;

public:
  file_manager() = delete;
  file_manager(const file_manager &) noexcept = delete;
  file_manager(file_manager &&) noexcept = delete;
  auto operator=(file_manager &&) noexcept -> file_manager & = delete;
  auto operator=(const file_manager &) noexcept -> file_manager & = delete;

private:
  app_config &config_;
  i_provider &provider_;

private:
  std::unique_ptr<i_file_mgr_db> mgr_db_;
  std::atomic<std::uint64_t> next_handle_{0U};
  mutable std::recursive_mutex open_file_mtx_;
  std::unordered_map<std::string, std::shared_ptr<i_closeable_open_file>>
      open_file_lookup_;
  stop_type stop_requested_{false};
  std::unordered_map<std::string, std::unique_ptr<upload>> upload_lookup_;
  mutable std::mutex upload_mtx_;
  std::condition_variable upload_notify_;
  std::unique_ptr<std::thread> upload_thread_;

private:
  [[nodiscard]] auto close_all(const std::string &api_path) -> bool;

  void close_timed_out_files();

  [[nodiscard]] auto get_open_file_by_handle(std::uint64_t handle) const
      -> std::shared_ptr<i_closeable_open_file>;

  [[nodiscard]] auto get_open_file_count(const std::string &api_path) const
      -> std::size_t;

  [[nodiscard]] auto get_stop_requested() const -> bool;

  [[nodiscard]] auto open(const std::string &api_path, bool directory,
                          const open_file_data &ofd, std::uint64_t &handle,
                          std::shared_ptr<i_open_file> &file,
                          std::shared_ptr<i_closeable_open_file> closeable_file)
      -> api_error;

  void queue_upload(const std::string &api_path, const std::string &source_path,
                    bool no_lock);

  void remove_resume(const std::string &api_path,
                     const std::string &source_path, bool no_lock);

  void remove_upload(const std::string &api_path, bool no_lock);

  void swap_renamed_items(std::string from_api_path, std::string to_api_path,
                          bool directory);

  void upload_completed(const file_upload_completed &evt);

  void upload_handler();

public:
  [[nodiscard]] auto get_next_handle() -> std::uint64_t;

  auto handle_file_rename(const std::string &from_api_path,
                          const std::string &to_api_path) -> api_error;

  void queue_upload(const i_open_file &file) override;

  void remove_resume(const std::string &api_path,
                     const std::string &source_path) override;

  static auto remove_source_and_shrink_cache(const std::string &api_path,
                                             const std::string &source_path,
                                             std::uint64_t file_size,
                                             bool allocated) -> bool;

  void remove_upload(const std::string &api_path) override;

  void store_resume(const i_open_file &file) override;

public:
  void close(std::uint64_t handle);

  [[nodiscard]] auto create(const std::string &api_path, api_meta_map &meta,
                            open_file_data ofd, std::uint64_t &handle,
                            std::shared_ptr<i_open_file> &file) -> api_error;

  [[nodiscard]] auto evict_file(const std::string &api_path) -> bool override;

  [[nodiscard]] auto get_directory_items(const std::string &api_path) const
      -> directory_item_list override;

  [[nodiscard]] auto get_open_file(std::uint64_t handle, bool write_supported,
                                   std::shared_ptr<i_open_file> &file) -> bool;

  [[nodiscard]] auto get_open_file_count() const -> std::size_t;

  [[nodiscard]] auto get_open_files() const
      -> std::unordered_map<std::string, std::size_t> override;

  [[nodiscard]] auto get_open_handle_count() const -> std::size_t;

  [[nodiscard]] auto get_stored_downloads() const
      -> std::vector<i_file_mgr_db::resume_entry>;

  [[nodiscard]] auto has_no_open_file_handles() const -> bool override;

  [[nodiscard]] auto is_processing(const std::string &api_path) const
      -> bool override;

#if defined(PROJECT_TESTING)
  [[nodiscard]] auto open(std::shared_ptr<i_closeable_open_file> of,
                          const open_file_data &ofd, std::uint64_t &handle,
                          std::shared_ptr<i_open_file> &file) -> api_error;
#endif // defined(PROJECT_TESTING)
  [[nodiscard]] auto open(const std::string &api_path, bool directory,
                          const open_file_data &ofd, std::uint64_t &handle,
                          std::shared_ptr<i_open_file> &file) -> api_error;

  [[nodiscard]] auto remove_file(const std::string &api_path) -> api_error;

  [[nodiscard]] auto rename_directory(const std::string &from_api_path,
                                      const std::string &to_api_path)
      -> api_error;

  [[nodiscard]] auto rename_file(const std::string &from_api_path,
                                 const std::string &to_api_path, bool overwrite)
      -> api_error;

  void start();

  void stop();
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_FILE_MANAGER_FILE_MANAGER_HPP_
