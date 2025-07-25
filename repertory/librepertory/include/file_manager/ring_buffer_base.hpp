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
#ifndef REPERTORY_INCLUDE_FILE_MANAGER_RING_BUFFER_BASE_HPP_
#define REPERTORY_INCLUDE_FILE_MANAGER_RING_BUFFER_BASE_HPP_

#include "file_manager/open_file_base.hpp"

#include "types/repertory.hpp"
#include "utils/file.hpp"

namespace repertory {
class i_provider;
class i_upload_manager;

class ring_buffer_base : public open_file_base {
public:
  ring_buffer_base(std::uint64_t chunk_size, std::uint8_t chunk_timeout,
                   filesystem_item fsi, i_provider &provider,
                   std::size_t ring_size, bool disable_io);

  ~ring_buffer_base() override = default;

public:
  ring_buffer_base() = delete;
  ring_buffer_base(const ring_buffer_base &) noexcept = delete;
  ring_buffer_base(ring_buffer_base &&) noexcept = delete;
  auto operator=(ring_buffer_base &&) noexcept -> ring_buffer_base & = delete;
  auto operator=(const ring_buffer_base &) noexcept
      -> ring_buffer_base & = delete;

public:
  static constexpr auto min_ring_size{5U};

private:
  boost::dynamic_bitset<> read_state_;
  std::size_t total_chunks_;

private:
  std::condition_variable chunk_notify_;
  mutable std::mutex chunk_mtx_;
  std::mutex read_mtx_;
  std::unique_ptr<std::thread> reader_thread_;
  std::size_t ring_begin_{};
  std::size_t ring_end_{};
  std::size_t ring_pos_{};
  stop_type stop_requested_{false};

private:
  [[nodiscard]] auto check_start() -> api_error;

  auto download_chunk(std::size_t chunk, bool skip_active) -> api_error;

  void reader_thread();

  void update_position(std::size_t count, bool is_forward);

  [[nodiscard]] auto get_stop_requested() const -> bool;

protected:
  [[nodiscard]] auto has_reader_thread() const -> bool {
    return reader_thread_ != nullptr;
  }

  [[nodiscard]] auto get_ring_size() const -> std::size_t {
    return read_state_.size();
  }

  [[nodiscard]] virtual auto on_check_start() -> bool = 0;

  [[nodiscard]] virtual auto on_chunk_downloaded(std::size_t chunk,
                                                 const data_buffer &buffer)
      -> api_error = 0;

  [[nodiscard]] virtual auto
  on_read_chunk(std::size_t chunk, std::size_t read_size,
                std::uint64_t read_offset, data_buffer &data,
                std::size_t &bytes_read) -> api_error = 0;

  [[nodiscard]] virtual auto
  use_buffer(std::size_t chunk, std::function<api_error(data_buffer &)> func)
      -> api_error = 0;

public:
  auto close() -> bool override;

  void forward(std::size_t count);

  [[nodiscard]] auto get_current_chunk() const -> std::size_t {
    return ring_pos_;
  }

  [[nodiscard]] auto get_first_chunk() const -> std::size_t {
    return ring_begin_;
  }

  [[nodiscard]] auto get_last_chunk() const -> std::size_t { return ring_end_; }

  [[nodiscard]] auto get_read_state() const -> boost::dynamic_bitset<> override;

  [[nodiscard]] auto get_read_state(std::size_t chunk) const -> bool override;

  [[nodiscard]] auto get_total_chunks() const -> std::size_t {
    return total_chunks_;
  }

  [[nodiscard]] auto is_complete() const -> bool override { return false; }

  [[nodiscard]] auto is_write_supported() const -> bool override {
    return false;
  }

  [[nodiscard]] auto read(std::size_t read_size, std::uint64_t read_offset,
                          data_buffer &data) -> api_error override;

  [[nodiscard]] auto resize(std::uint64_t /* size */) -> api_error override {
    return api_error::not_supported;
  }

  void reverse(std::size_t count);

  void set(std::size_t first_chunk, std::size_t current_chunk);

  void set_api_path(const std::string &api_path) override;

  [[nodiscard]] auto write(std::uint64_t /* write_offset */,
                           const data_buffer & /* data */,
                           std::size_t & /* bytes_written */)
      -> api_error override {
    return api_error::not_supported;
  }
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_FILE_MANAGER_RING_BUFFER_BASE_HPP_
