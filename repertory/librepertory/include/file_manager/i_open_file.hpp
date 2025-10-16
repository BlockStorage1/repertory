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
#ifndef REPERTORY_INCLUDE_FILE_MANAGER_I_OPEN_FILE_HPP_
#define REPERTORY_INCLUDE_FILE_MANAGER_I_OPEN_FILE_HPP_

#include "types/repertory.hpp"

namespace repertory {
class i_open_file {
  INTERFACE_SETUP(i_open_file);

public:
  using native_operation_callback = std::function<api_error(native_handle)>;

public:
  virtual void force_download() = 0;

  [[nodiscard]] virtual auto get_api_path() const -> std::string = 0;

  [[nodiscard]] virtual auto get_chunk_size() const -> std::size_t = 0;

  [[nodiscard]] virtual auto get_file_size() const -> std::uint64_t = 0;

  [[nodiscard]] virtual auto get_filesystem_item() const -> filesystem_item = 0;

  [[nodiscard]] virtual auto get_open_data()
      -> std::map<std::uint64_t, open_file_data> & = 0;

  [[nodiscard]] virtual auto get_open_data() const
      -> const std::map<std::uint64_t, open_file_data> & = 0;

  [[nodiscard]] virtual auto get_open_data(std::uint64_t handle)
      -> open_file_data & = 0;

  [[nodiscard]] virtual auto get_open_data(std::uint64_t handle) const
      -> const open_file_data & = 0;

  [[nodiscard]] virtual auto get_open_file_count() const -> std::size_t = 0;

  [[nodiscard]] virtual auto get_read_state() const
      -> boost::dynamic_bitset<> = 0;

  [[nodiscard]] virtual auto get_read_state(std::size_t chunk) const
      -> bool = 0;

  [[nodiscard]] virtual auto get_source_path() const -> std::string = 0;

  [[nodiscard]] virtual auto get_unlinked_meta() const -> api_meta_map = 0;

  [[nodiscard]] virtual auto has_handle(std::uint64_t handle) const -> bool = 0;

  [[nodiscard]] virtual auto is_complete() const -> bool = 0;

  [[nodiscard]] virtual auto is_directory() const -> bool = 0;

  [[nodiscard]] virtual auto is_unlinked() const -> bool = 0;

  [[nodiscard]] virtual auto is_write_supported() const -> bool = 0;

  [[nodiscard]] virtual auto
  native_operation(native_operation_callback callback) -> api_error = 0;

  [[nodiscard]] virtual auto
  native_operation(std::uint64_t new_file_size,
                   native_operation_callback callback) -> api_error = 0;

  [[nodiscard]] virtual auto read(std::size_t read_size,
                                  std::uint64_t read_offset, data_buffer &data)
      -> api_error = 0;

  [[nodiscard]] virtual auto resize(std::uint64_t new_file_size)
      -> api_error = 0;

  virtual void set_api_path(std::string_view api_path) = 0;

  [[nodiscard]] virtual auto write(std::uint64_t write_offset,
                                   const data_buffer &data,
                                   std::size_t &bytes_written) -> api_error = 0;
};

class i_closeable_open_file : public i_open_file {
  INTERFACE_SETUP(i_closeable_open_file);

public:
  virtual void add(std::uint64_t handle, open_file_data ofd, bool notify) = 0;

  [[nodiscard]] virtual auto can_close() const -> bool = 0;

  virtual auto close() -> bool = 0;

  [[nodiscard]] virtual auto get_allocated() const -> bool = 0;

  [[nodiscard]] virtual auto get_handles() const
      -> std::vector<std::uint64_t> = 0;

  [[nodiscard]] virtual auto is_modified() const -> bool = 0;

  virtual void remove(std::uint64_t handle) = 0;

  virtual void remove_all() = 0;

  virtual void set_unlinked(bool value) = 0;

  virtual void set_unlinked_meta(api_meta_map meta) = 0;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_FILE_MANAGER_I_OPEN_FILE_HPP_
