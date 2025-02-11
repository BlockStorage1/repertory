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
#include "file_manager/direct_open_file.hpp"

#include "file_manager/open_file_base.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"

namespace repertory {
direct_open_file::direct_open_file(std::uint64_t chunk_size,
                                   std::uint8_t chunk_timeout,
                                   filesystem_item fsi, i_provider &provider)
    : ring_buffer_base(chunk_size, chunk_timeout, fsi, provider,
                         min_ring_size, true) {}

direct_open_file::~direct_open_file() {
  REPERTORY_USES_FUNCTION_NAME();

  close();
}

auto direct_open_file::on_check_start() -> bool {
  return (get_file_size() == 0U || has_reader_thread());
}

auto direct_open_file::on_read_chunk(std::size_t chunk, std::size_t read_size,
                                     std::uint64_t read_offset,
                                     data_buffer &data,
                                     std::size_t &bytes_read) -> api_error {
  auto &buffer = ring_data_.at(chunk % get_ring_size());
  auto begin =
      std::next(buffer.begin(), static_cast<std::int64_t>(read_offset));
  auto end = std::next(begin, static_cast<std::int64_t>(read_size));
  data.insert(data.end(), begin, end);
  bytes_read = read_size;
  return api_error::success;
}

auto direct_open_file::use_buffer(std::size_t chunk,
                                  std::function<api_error(data_buffer &)> func)
    -> api_error {
  return func(ring_data_.at(chunk % get_ring_size()));
}
} // namespace repertory
