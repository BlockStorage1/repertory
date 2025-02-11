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
#ifndef REPERTORY_INCLUDE_FILE_MANAGER_OPEN_FILE_HPP_
#define REPERTORY_INCLUDE_FILE_MANAGER_OPEN_FILE_HPP_

#include "file_manager/open_file_base.hpp"

#include "types/repertory.hpp"
#include "utils/types/file/i_file.hpp"

namespace repertory {
class i_provider;
class i_upload_manager;

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
  bool allocated{false};
  std::unique_ptr<utils::file::i_file> nf_;
  bool notified_{false};
  std::size_t read_chunk_{};
  boost::dynamic_bitset<> read_state_;
  std::unique_ptr<std::thread> reader_thread_;
  mutable std::recursive_mutex rw_mtx_;
  stop_type stop_requested_{false};

private:
  [[nodiscard]] auto adjust_cache_size(std::uint64_t file_size, bool shrink)
      -> api_error;

  [[nodiscard]] auto check_start() -> api_error;

  void download_chunk(std::size_t chunk, bool skip_active, bool should_reset);

  void download_range(std::size_t begin_chunk, std::size_t end_chunk,
                      bool should_reset);

  [[nodiscard]] auto get_stop_requested() const -> bool;

  void set_modified();

  void set_read_state(std::size_t chunk);

  void set_read_state(boost::dynamic_bitset<> read_state);

  void update_reader(std::size_t chunk);

public:
  auto close() -> bool override;

  [[nodiscard]] auto get_allocated() const -> bool override;

  [[nodiscard]] auto get_read_state() const -> boost::dynamic_bitset<> override;

  [[nodiscard]] auto get_read_state(std::size_t chunk) const -> bool override;

  [[nodiscard]] auto is_complete() const -> bool override;

  [[nodiscard]] auto is_write_supported() const -> bool override {
    return true;
  }

  [[nodiscard]] auto native_operation(native_operation_callback callback)
      -> api_error override;

  [[nodiscard]] auto native_operation(std::uint64_t new_file_size,
                                      native_operation_callback callback)
      -> api_error override;

  void remove(std::uint64_t handle) override;

  void remove_all() override;

  [[nodiscard]] auto read(std::size_t read_size, std::uint64_t read_offset,
                          data_buffer &data) -> api_error override;

  [[nodiscard]] auto resize(std::uint64_t new_file_size) -> api_error override;

  [[nodiscard]] auto write(std::uint64_t write_offset, const data_buffer &data,
                           std::size_t &bytes_written) -> api_error override;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_FILE_MANAGER_OPEN_FILE_HPP_
