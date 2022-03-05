/*
  Copyright <2018-2022> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute,
  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
  OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef INCLUDE_DOWNLOAD_DOWNLOAD_HPP_
#define INCLUDE_DOWNLOAD_DOWNLOAD_HPP_

#include "common.hpp"
#include "app_config.hpp"
#include "download/i_download.hpp"
#include "types/repertory.hpp"
#include "utils/native_file.hpp"

namespace repertory {
class i_open_file_table;
class download final : public virtual i_download {
private:
  struct active_chunk {
    explicit active_chunk(std::thread worker) : worker(std::move(worker)) {}

    std::thread worker;
    std::mutex mutex;
    std::condition_variable notify;
  };
  typedef std::shared_ptr<active_chunk> active_chunk_ptr;

  struct read_data {
    read_data(std::vector<char> &data, const std::uint64_t &offset)
        : complete(false), data(data), offset(offset) {}

    bool complete;
    std::vector<char> &data;
    const std::uint64_t offset;
    std::mutex mutex;
    std::condition_variable notify;
  };
  typedef std::shared_ptr<read_data> read_data_ptr;

  struct write_data {
    write_data(const std::size_t &chunk, std::vector<char> data, const std::uint64_t &offset)
        : chunk_index(chunk), data(std::move(data)), offset(offset) {}
    write_data(const std::uint64_t &offset, std::vector<char> data)
        : chunk_index(0u), data(std::move(data)), offset(offset), from_read(false) {}

    std::size_t chunk_index;
    std::vector<char> data;
    const std::uint64_t offset;
    bool from_read = true;
    bool complete = false;
    std::mutex mutex;
    std::condition_variable notify;
    std::size_t written = 0u;
  };
  typedef std::shared_ptr<write_data> write_data_ptr;

public:
  download(const app_config &config, filesystem_item &fsi, const api_reader_callback &api_reader,
           const std::size_t &chunk_size, i_open_file_table &oft);

  download(const app_config &config, filesystem_item &fsi, const api_reader_callback &api_reader,
           const std::size_t &chunk_size, std::size_t &last_chunk_size,
           boost::dynamic_bitset<> &read_state, boost::dynamic_bitset<> &write_state,
           i_open_file_table &oft);

  ~download() override;

private:
  // Constructor initialization
  const app_config &config_;
  filesystem_item fsi_;
  const api_reader_callback &api_reader_;
  i_open_file_table &oft_;
  const std::size_t chunk_size_;
  boost::dynamic_bitset<> read_chunk_state_;
  std::size_t last_chunk_size_;
  boost::dynamic_bitset<> write_chunk_state_;

  // Default initialization
  bool auto_close_ = false;
  std::unordered_map<std::size_t, active_chunk_ptr> active_chunks_;
  api_error error_ = api_error::success;
  std::vector<std::thread> background_workers_;
  std::size_t current_chunk_index_ = 0u;
  bool disable_download_end_ = false;
  std::unique_ptr<std::thread> io_thread_;
  std::uint64_t open_file_handle_ = REPERTORY_API_INVALID_HANDLE;
  bool paused_ = false;
  bool processed_ = false;
  std::condition_variable processed_notify_;
  double progress_ = 0.0;
  std::size_t read_offset_ = 0u;
  std::deque<read_data_ptr> read_queue_;
  native_file_ptr read_write_file_;
  std::mutex read_write_mutex_;
  std::condition_variable read_write_notify_;
  bool stop_requested_ = false;
  std::chrono::system_clock::time_point timeout_ =
      std::chrono::system_clock::now() +
      std::chrono::seconds(config_.get_chunk_downloader_timeout_secs());
  std::deque<write_data_ptr> write_queue_;

private:
  void create_active_chunk(std::size_t chunk_index);

  void download_chunk(std::size_t chunk_index, bool inactive_only);

  bool get_complete() const;

  bool get_timeout_enabled() const;

  void handle_active_chunk_complete(std::size_t chunk_index, unique_mutex_lock &lock);

  void initialize_download(filesystem_item &fsi, const bool &delete_existing);

  void io_data_worker();

  void notify_progress();

  void process_download_complete(unique_mutex_lock &lock);

  void process_read_queue(unique_mutex_lock &lock);

  void process_timeout(unique_mutex_lock &lock);

  void process_write_queue(unique_mutex_lock &lock);

  void read_ahead_worker();

  void read_behind_worker();

  void read_end_worker();

  void shutdown(unique_mutex_lock &lock);

  void wait_for_io(unique_mutex_lock &lock);

public:
  api_error allocate(const std::uint64_t &handle, const std::uint64_t &size,
                     const allocator_callback &allocator,
                     const completer_callback &completer) override;

  api_error download_all() override;

  api_error get_result() const override { return error_; }

  std::string get_source_path() const override { return fsi_.source_path; }

  void get_state_information(filesystem_item &fsi, std::size_t &chunk_size,
                             std::size_t &last_chunk_size, boost::dynamic_bitset<> &read_state,
                             boost::dynamic_bitset<> &write_state) override;

  bool get_write_supported() const override { return true; }

  bool is_active() const { return not read_chunk_state_.all(); }

  void notify_stop_requested() override;

  bool pause() override;

  api_error read_bytes(const std::uint64_t &handle, std::size_t read_size,
                       const std::uint64_t &read_offset, std::vector<char> &data) override;

  void reset_timeout(const bool &) override;

  void resume() override;

  void set_api_path(const std::string &api_path) override;

  void set_disable_download_end(const bool &disable) override { disable_download_end_ = disable; }

  api_error write_bytes(const std::uint64_t &handle, const std::uint64_t &write_offset,
                        std::vector<char> data, std::size_t &bytes_written,
                        const completer_callback &completer) override;
};
} // namespace repertory

#endif // INCLUDE_DOWNLOAD_DOWNLOAD_HPP_
