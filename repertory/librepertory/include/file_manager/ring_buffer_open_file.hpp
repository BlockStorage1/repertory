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
#ifndef REPERTORY_INCLUDE_FILE_MANAGER_RING_BUFFER_OPEN_FILE_HPP_
#define REPERTORY_INCLUDE_FILE_MANAGER_RING_BUFFER_OPEN_FILE_HPP_

#include "file_manager/ring_buffer_base.hpp"

#include "types/repertory.hpp"
#include "utils/file.hpp"

namespace repertory {
class i_provider;
class i_upload_manager;

class ring_buffer_open_file final : public ring_buffer_base {
public:
  ring_buffer_open_file(std::string buffer_directory, std::uint64_t chunk_size,
                        std::uint8_t chunk_timeout, filesystem_item fsi,
                        i_provider &provider, std::size_t ring_size);

  ~ring_buffer_open_file() override;

public:
  ring_buffer_open_file() = delete;
  ring_buffer_open_file(const ring_buffer_open_file &) noexcept = delete;
  ring_buffer_open_file(ring_buffer_open_file &&) noexcept = delete;
  auto operator=(ring_buffer_open_file &&) noexcept -> ring_buffer_open_file & =
                                                           delete;
  auto operator=(const ring_buffer_open_file &) noexcept
      -> ring_buffer_open_file & = delete;

private:
  std::string source_path_;

private:
  std::unique_ptr<utils::file::i_file> nf_;

protected:
  [[nodiscard]] auto on_check_start() -> bool override;

  [[nodiscard]] auto
  on_chunk_downloaded(std::size_t chunk,
                      const data_buffer &buffer) -> api_error override;

  [[nodiscard]] auto
  on_read_chunk(std::size_t chunk, std::size_t read_size,
                std::uint64_t read_offset, data_buffer &data,
                std::size_t &bytes_read) -> api_error override;

  [[nodiscard]] auto use_buffer(std::size_t chunk,
                                std::function<api_error(data_buffer &)> func)
      -> api_error override;

public:
  [[nodiscard]] static auto can_handle_file(std::uint64_t file_size,
                                            std::size_t chunk_size,
                                            std::size_t ring_size) -> bool;

  [[nodiscard]] auto
  native_operation(native_operation_callback callback) -> api_error override;

  [[nodiscard]] auto native_operation(std::uint64_t /* new_file_size */,
                                      native_operation_callback /* callback */)
      -> api_error override {
    return api_error::not_supported;
  }

  [[nodiscard]] auto get_source_path() const -> std::string override {
    return source_path_;
  }
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_FILE_MANAGER_RING_BUFFER_OPEN_FILE_HPP_
