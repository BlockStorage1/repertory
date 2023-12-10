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
#ifndef INCLUDE_FILE_MANAGER_FILE_MANAGER_HPP_
#define INCLUDE_FILE_MANAGER_FILE_MANAGER_HPP_

#include "events/event_system.hpp"
#include "events/events.hpp"
#include "file_manager/i_file_manager.hpp"
#include "file_manager/i_open_file.hpp"
#include "file_manager/i_upload_manager.hpp"
#include "types/repertory.hpp"
#include "utils/native_file.hpp"

namespace repertory {
class app_config;
class i_provider;

class file_manager final : public i_file_manager, public i_upload_manager {
  E_CONSUMER();

public:
  class open_file_base : public i_closeable_open_file {
  public:
    open_file_base(std::uint64_t chunk_size, std::uint8_t chunk_timeout,
                   filesystem_item fsi, i_provider &provider);

    open_file_base(std::uint64_t chunk_size, std::uint8_t chunk_timeout,
                   filesystem_item fsi,
                   std::map<std::uint64_t, open_file_data> open_data,
                   i_provider &provider);

    ~open_file_base() override = default;

  public:
    open_file_base() = delete;
    open_file_base(const open_file_base &) noexcept = delete;
    open_file_base(open_file_base &&) noexcept = delete;
    auto operator=(open_file_base &&) noexcept -> open_file_base & = delete;
    auto operator=(const open_file_base &) noexcept
        -> open_file_base & = delete;

  public:
    class download final {
    public:
      download() = default;

      ~download() = default;

    public:
      download(const download &) noexcept = delete;
      download(download &&) noexcept = delete;
      auto operator=(download &&) noexcept -> download & = delete;
      auto operator=(const download &) noexcept -> download & = delete;

    private:
      bool complete_{false};
      api_error error_{api_error::success};
      std::mutex mtx_;
      std::condition_variable notify_;

    public:
      void notify(const api_error &err);

      auto wait() -> api_error;
    };

    class io_item final {
    public:
      io_item(std::function<api_error()> action) : action_(std::move(action)) {}

      ~io_item() = default;

    public:
      io_item() = delete;
      io_item(const io_item &) noexcept = delete;
      io_item(io_item &&) noexcept = delete;
      auto operator=(io_item &&) noexcept -> io_item & = delete;
      auto operator=(const io_item &) noexcept -> io_item & = delete;

    private:
      std::function<api_error()> action_;
      std::mutex mtx_;
      std::condition_variable notify_;
      std::optional<api_error> result_;

    public:
      void action();

      [[nodiscard]] auto get_result() -> api_error;
    };

  protected:
    std::uint64_t chunk_size_;
    std::uint8_t chunk_timeout_;
    filesystem_item fsi_;
    std::size_t last_chunk_size_;
    std::map<std::uint64_t, open_file_data> open_data_;
    i_provider &provider_;

  private:
    api_error error_{api_error::success};
    mutable std::mutex error_mtx_;
    stop_type io_stop_requested_{false};
    std::unique_ptr<std::thread> io_thread_;

  protected:
    std::unordered_map<std::size_t, std::shared_ptr<download>>
        active_downloads_;
    mutable std::recursive_mutex file_mtx_;
    std::atomic<std::chrono::system_clock::time_point> last_access_{
        std::chrono::system_clock::now()};
    bool modified_{false};
    native_file_ptr nf_;
    mutable std::mutex io_thread_mtx_;
    std::condition_variable io_thread_notify_;
    std::deque<std::shared_ptr<io_item>> io_thread_queue_;
    bool removed_{false};

  private:
    void file_io_thread();

  protected:
    [[nodiscard]] auto do_io(std::function<api_error()> action) -> api_error;

    virtual auto is_download_complete() const -> bool = 0;

    void reset_timeout();

    auto set_api_error(const api_error &e) -> api_error;

  public:
    void add(std::uint64_t handle, open_file_data ofd) override;

    [[nodiscard]] auto can_close() const -> bool override;

    auto close() -> bool override;

    [[nodiscard]] auto get_api_error() const -> api_error;

    [[nodiscard]] auto get_api_path() const -> std::string override;

    [[nodiscard]] auto get_chunk_size() const -> std::size_t override {
      return chunk_size_;
    }

    [[nodiscard]] auto get_file_size() const -> std::uint64_t override;

    [[nodiscard]] auto get_filesystem_item() const -> filesystem_item override;

    [[nodiscard]] auto get_handles() const
        -> std::vector<std::uint64_t> override;

    [[nodiscard]] auto get_open_data() const
        -> std::map<std::uint64_t, open_file_data> override;

    [[nodiscard]] auto get_open_data(std::uint64_t handle) const
        -> open_file_data override;

    [[nodiscard]] auto get_open_file_count() const -> std::size_t override;

    [[nodiscard]] auto get_source_path() const -> std::string override {
      return fsi_.source_path;
    }

    [[nodiscard]] auto has_handle(std::uint64_t handle) const -> bool override {
      return open_data_.find(handle) != open_data_.end();
    }

    [[nodiscard]] auto is_directory() const -> bool override {
      return fsi_.directory;
    }

    [[nodiscard]] auto is_modified() const -> bool override;

    void remove(std::uint64_t handle) override;

    void set_api_path(const std::string &api_path) override;
  };

  class open_file final : public open_file_base {
  public:
    open_file(std::uint64_t chunk_size, std::uint8_t chunk_timeout,
              filesystem_item fsi, i_provider &provider, i_upload_manager &mgr);

    open_file(std::uint64_t chunk_size, std::uint8_t chunk_timeout,
              filesystem_item fsi,
              std::map<std::uint64_t, open_file_data> open_data,
              i_provider &provider, i_upload_manager &mgr);

    open_file(std::uint64_t chunk_size, std::uint8_t chunk_timeout,
              filesystem_item fsi, i_provider &provider,
              std::optional<boost::dynamic_bitset<>> read_state,
              i_upload_manager &mgr);

  private:
    open_file(std::uint64_t chunk_size, std::uint8_t chunk_timeout,
              filesystem_item fsi,
              std::map<std::uint64_t, open_file_data> open_data,
              i_provider &provider,
              std::optional<boost::dynamic_bitset<>> read_state,
              i_upload_manager &mgr);

  public:
    open_file() = delete;
    open_file(const open_file &) noexcept = delete;
    open_file(open_file &&) noexcept = delete;
    auto operator=(open_file &&) noexcept -> open_file & = delete;
    auto operator=(const open_file &) noexcept -> open_file & = delete;

  public:
    ~open_file() override;

  private:
    i_upload_manager &mgr_;

  private:
    bool notified_ = false;
    std::size_t read_chunk_index_{};
    boost::dynamic_bitset<> read_state_;
    std::unique_ptr<std::thread> reader_thread_;
    std::unique_ptr<std::thread> download_thread_;
    stop_type stop_requested_ = false;

  private:
    void download_chunk(std::size_t chunk, bool skip_active, bool should_reset);

    void download_range(std::size_t start_chunk_index,
                        std::size_t end_chunk_index_inclusive,
                        bool should_reset);

    void set_modified();

    void update_background_reader(std::size_t read_chunk);

  protected:
    auto is_download_complete() const -> bool override {
      return read_state_.all();
    }

  public:
    auto close() -> bool override;

    [[nodiscard]] auto get_read_state() const
        -> boost::dynamic_bitset<> override;

    [[nodiscard]] auto get_read_state(std::size_t chunk) const -> bool override;

    [[nodiscard]] auto is_complete() const -> bool override;

    auto is_write_supported() const -> bool override { return true; }

    [[nodiscard]] auto
    native_operation(const native_operation_callback &callback)
        -> api_error override;

    [[nodiscard]] auto
    native_operation(std::uint64_t new_file_size,
                     const native_operation_callback &callback)
        -> api_error override;

    void remove(std::uint64_t handle) override;

    [[nodiscard]] auto read(std::size_t read_size, std::uint64_t read_offset,
                            data_buffer &data) -> api_error override;

    [[nodiscard]] auto resize(std::uint64_t new_file_size)
        -> api_error override;

    [[nodiscard]] auto write(std::uint64_t write_offset,
                             const data_buffer &data,
                             std::size_t &bytes_written) -> api_error override;
  };

  class ring_buffer_open_file final : public open_file_base {
  public:
    ring_buffer_open_file(std::string buffer_directory,
                          std::uint64_t chunk_size, std::uint8_t chunk_timeout,
                          filesystem_item fsi, i_provider &provider);

    ring_buffer_open_file(std::string buffer_directory,
                          std::uint64_t chunk_size, std::uint8_t chunk_timeout,
                          filesystem_item fsi, i_provider &provider,
                          std::size_t ring_size);

    ~ring_buffer_open_file() override;

  public:
    ring_buffer_open_file() = delete;
    ring_buffer_open_file(const ring_buffer_open_file &) noexcept = delete;
    ring_buffer_open_file(ring_buffer_open_file &&) noexcept = delete;
    auto operator=(ring_buffer_open_file &&) noexcept
        -> ring_buffer_open_file & = delete;
    auto operator=(const ring_buffer_open_file &) noexcept
        -> ring_buffer_open_file & = delete;

  private:
    boost::dynamic_bitset<> ring_state_;
    std::size_t total_chunks_;

  private:
    std::unique_ptr<std::thread> chunk_forward_thread_;
    std::unique_ptr<std::thread> chunk_reverse_thread_;
    std::condition_variable chunk_notify_;
    mutable std::mutex chunk_mtx_;
    std::size_t current_chunk_{};
    std::size_t first_chunk_{};
    std::size_t last_chunk_;

  private:
    auto download_chunk(std::size_t chunk) -> api_error;

    void forward_reader_thread(std::size_t count);

    void reverse_reader_thread(std::size_t count);

  protected:
    auto is_download_complete() const -> bool override;

  public:
    void forward(std::size_t count);

    [[nodiscard]] auto get_current_chunk() const -> std::size_t {
      return current_chunk_;
    }

    [[nodiscard]] auto get_first_chunk() const -> std::size_t {
      return first_chunk_;
    }

    [[nodiscard]] auto get_last_chunk() const -> std::size_t {
      return last_chunk_;
    }

    [[nodiscard]] auto get_read_state() const
        -> boost::dynamic_bitset<> override;

    [[nodiscard]] auto get_read_state(std::size_t chunk) const -> bool override;

    [[nodiscard]] auto get_total_chunks() const -> std::size_t {
      return total_chunks_;
    }

    [[nodiscard]] auto is_complete() const -> bool override { return true; }

    auto is_write_supported() const -> bool override { return false; }

    [[nodiscard]] auto
    native_operation(const native_operation_callback &callback)
        -> api_error override;

    [[nodiscard]] auto native_operation(std::uint64_t,
                                        const native_operation_callback &)
        -> api_error override {
      return api_error::not_supported;
    }

    [[nodiscard]] auto read(std::size_t read_size, std::uint64_t read_offset,
                            data_buffer &data) -> api_error override;

    [[nodiscard]] auto resize(std::uint64_t) -> api_error override {
      return api_error::not_supported;
    }

    void reverse(std::size_t count);

    void set(std::size_t first_chunk, std::size_t current_chunk);

    void set_api_path(const std::string &api_path) override;

    [[nodiscard]] auto write(std::uint64_t, const data_buffer &, std::size_t &)
        -> api_error override {
      return api_error::not_supported;
    }
  };

  class upload final {
  public:
    upload(filesystem_item fsi, i_provider &provider);

    ~upload();

  public:
    upload() = delete;
    upload(const upload &) noexcept = delete;
    upload(upload &&) noexcept = delete;
    auto operator=(upload &&) noexcept -> upload & = delete;
    auto operator=(const upload &) noexcept -> upload & = delete;

  private:
    filesystem_item fsi_;
    i_provider &provider_;

  private:
    bool cancelled_ = false;
    api_error error_ = api_error::success;
    std::unique_ptr<std::thread> thread_;
    stop_type stop_requested_ = false;

  private:
    void upload_thread();

  public:
    void cancel();

    [[nodiscard]] auto get_api_error() const -> api_error { return error_; }

    [[nodiscard]] auto get_api_path() const -> std::string {
      return fsi_.api_path;
    }

    [[nodiscard]] auto get_source_path() const -> std::string {
      return fsi_.source_path;
    }

    [[nodiscard]] auto is_cancelled() const -> bool { return cancelled_; }

    void stop();
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
  db3_t db_{nullptr};
  std::uint64_t next_handle_{0U};
  mutable std::recursive_mutex open_file_mtx_;
  std::unordered_map<std::string, std::shared_ptr<i_closeable_open_file>>
      open_file_lookup_;
  stop_type stop_requested_{false};
  std::unordered_map<std::string, std::unique_ptr<upload>> upload_lookup_;
  mutable std::mutex upload_mtx_;
  std::condition_variable upload_notify_;
  std::unique_ptr<std::thread> upload_thread_;

private:
  void close_timed_out_files();

  auto get_open_file_by_handle(std::uint64_t handle) const
      -> std::shared_ptr<i_closeable_open_file>;

  auto get_open_file_count(const std::string &api_path) const -> std::size_t;

  auto open(const std::string &api_path, bool directory,
            const open_file_data &ofd, std::uint64_t &handle,
            std::shared_ptr<i_open_file> &file,
            std::shared_ptr<i_closeable_open_file> closeable_file) -> api_error;

  void queue_upload(const std::string &api_path, const std::string &source_path,
                    bool no_lock);

  void remove_upload(const std::string &api_path, bool no_lock);

  void swap_renamed_items(std::string from_api_path, std::string to_api_path);

  void upload_completed(const file_upload_completed &evt);

  void upload_handler();

public:
  [[nodiscard]] auto get_next_handle() -> std::uint64_t;

  auto handle_file_rename(const std::string &from_api_path,
                          const std::string &to_api_path) -> api_error;

  void queue_upload(const i_open_file &file) override;

  void remove_resume(const std::string &api_path,
                     const std::string &source_path) override;

  void remove_upload(const std::string &api_path) override;

  void store_resume(const i_open_file &file) override;

public:
  void close(std::uint64_t handle);

  void close_all(const std::string &api_path);

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

  [[nodiscard]] auto get_stored_downloads() const -> std::vector<json>;

  [[nodiscard]] auto has_no_open_file_handles() const -> bool override;

  [[nodiscard]] auto is_processing(const std::string &api_path) const
      -> bool override;

#ifdef REPERTORY_TESTING
  [[nodiscard]] auto open(std::shared_ptr<i_closeable_open_file> of,
                          const open_file_data &ofd, std::uint64_t &handle,
                          std::shared_ptr<i_open_file> &file) -> api_error;
#endif
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

  void update_used_space(std::uint64_t &used_space) const override;
};
} // namespace repertory

#endif // INCLUDE_FILE_MANAGER_FILE_MANAGER_HPP_
