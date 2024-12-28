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
#ifndef REPERTORY_INCLUDE_FILE_MANAGER_OPEN_FILE_BASE_HPP_
#define REPERTORY_INCLUDE_FILE_MANAGER_OPEN_FILE_BASE_HPP_

#include "file_manager/i_open_file.hpp"

namespace repertory {
class i_provider;

class open_file_base : public i_closeable_open_file {
public:
  open_file_base(std::uint64_t chunk_size, std::uint8_t chunk_timeout,
                 filesystem_item fsi, i_provider &provider, bool disable_io);

  open_file_base(std::uint64_t chunk_size, std::uint8_t chunk_timeout,
                 filesystem_item fsi,
                 std::map<std::uint64_t, open_file_data> open_data,
                 i_provider &provider, bool disable_io);

  ~open_file_base() override = default;

public:
  open_file_base() = delete;
  open_file_base(const open_file_base &) noexcept = delete;
  open_file_base(open_file_base &&) noexcept = delete;
  auto operator=(open_file_base &&) noexcept -> open_file_base & = delete;
  auto operator=(const open_file_base &) noexcept -> open_file_base & = delete;

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

private:
  std::uint64_t chunk_size_;
  std::uint8_t chunk_timeout_;
  filesystem_item fsi_;
  std::size_t last_chunk_size_;
  std::map<std::uint64_t, open_file_data> open_data_;
  i_provider &provider_;

private:
  std::unordered_map<std::size_t, std::shared_ptr<download>> active_downloads_;
  api_error error_{api_error::success};
  mutable std::mutex error_mtx_;
  mutable std::recursive_mutex file_mtx_;
  stop_type io_stop_requested_{false};
  std::unique_ptr<std::thread> io_thread_;
  mutable std::mutex io_thread_mtx_;
  std::condition_variable io_thread_notify_;
  std::deque<std::shared_ptr<io_item>> io_thread_queue_;
  std::atomic<std::chrono::system_clock::time_point> last_access_{
      std::chrono::system_clock::now(),
  };
  bool modified_{false};
  bool removed_{false};

private:
  void file_io_thread();

protected:
  [[nodiscard]] auto do_io(std::function<api_error()> action) -> api_error;

  [[nodiscard]] auto get_active_downloads()
      -> std::unordered_map<std::size_t, std::shared_ptr<download>> & {
    return active_downloads_;
  }

  [[nodiscard]] auto get_mutex() const -> std::recursive_mutex & {
    return file_mtx_;
  }

  [[nodiscard]] auto get_last_chunk_size() const -> std::size_t;

  [[nodiscard]] auto get_provider() -> i_provider & { return provider_; }

  [[nodiscard]] auto get_provider() const -> const i_provider & {
    return provider_;
  }

  [[nodiscard]] auto is_removed() const -> bool;

  void notify_io();

  void reset_timeout();

  auto set_api_error(const api_error &err) -> api_error;

  void set_file_size(std::uint64_t size);

  void set_last_chunk_size(std::size_t size);

  void set_modified(bool modified);

  void set_removed(bool removed);

  void set_source_path(std::string source_path);

  void wait_for_io(stop_type &stop_requested);

public:
  void add(std::uint64_t handle, open_file_data ofd) override;

  [[nodiscard]] auto can_close() const -> bool override;

  auto close() -> bool override;

  [[nodiscard]] auto get_allocated() const -> bool override { return false; }

  [[nodiscard]] auto get_api_error() const -> api_error;

  [[nodiscard]] auto get_api_path() const -> std::string override;

  [[nodiscard]] auto get_chunk_size() const -> std::size_t override {
    return chunk_size_;
  }

  [[nodiscard]] auto get_file_size() const -> std::uint64_t override;

  [[nodiscard]] auto get_filesystem_item() const -> filesystem_item override;

  [[nodiscard]] auto get_handles() const -> std::vector<std::uint64_t> override;

  [[nodiscard]] auto
  get_open_data() -> std::map<std::uint64_t, open_file_data> & override;

  [[nodiscard]] auto get_open_data() const
      -> const std::map<std::uint64_t, open_file_data> & override;

  [[nodiscard]] auto
  get_open_data(std::uint64_t handle) -> open_file_data & override;

  [[nodiscard]] auto
  get_open_data(std::uint64_t handle) const -> const open_file_data & override;

  [[nodiscard]] auto get_open_file_count() const -> std::size_t override;

  [[nodiscard]] auto get_source_path() const -> std::string override;

  [[nodiscard]] auto has_handle(std::uint64_t handle) const -> bool override;

  [[nodiscard]] auto is_directory() const -> bool override {
    return fsi_.directory;
  }

  [[nodiscard]] auto is_modified() const -> bool override;

  void remove(std::uint64_t handle) override;

  void remove_all() override;

  void set_api_path(const std::string &api_path) override;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_FILE_MANAGER_OPEN_FILE_BASE_HPP_
