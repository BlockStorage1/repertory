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
#ifndef TESTS_MOCKS_MOCK_OPEN_FILE_HPP_
#define TESTS_MOCKS_MOCK_OPEN_FILE_HPP_

#include "test_common.hpp"

#include "file_manager/i_open_file.hpp"

namespace repertory {
class mock_open_file : public virtual i_closeable_open_file {
public:
  MOCK_METHOD(std::string, get_api_path, (), (const, override));

  MOCK_METHOD(std::size_t, get_chunk_size, (), (const, override));

  MOCK_METHOD(std::uint64_t, get_file_size, (), (const, override));

  MOCK_METHOD(filesystem_item, get_filesystem_item, (), (const, override));

  MOCK_METHOD(open_file_data, get_open_data, (std::uint64_t handle),
              (const, override));

  MOCK_METHOD(std::size_t, get_open_file_count, (), (const, override));

  MOCK_METHOD(boost::dynamic_bitset<>, get_read_state, (), (const, override));

  MOCK_METHOD(bool, get_read_state, (std::size_t chunk), (const, override));

  MOCK_METHOD(std::string, get_source_path, (), (const, override));

  MOCK_METHOD(bool, is_directory, (), (const, override));

  MOCK_METHOD(api_error, native_operation,
              (const native_operation_callback &cb), (override));

  MOCK_METHOD(api_error, native_operation,
              (std::uint64_t new_file_size,
               const native_operation_callback &cb),
              (override));

  MOCK_METHOD(api_error, read,
              (std::size_t read_size, std::uint64_t read_offset,
               data_buffer &data),
              (override));

  MOCK_METHOD(api_error, resize, (std::uint64_t new_file_size), (override));

  MOCK_METHOD(void, set_api_path, (const std::string &api_path), (override));

  MOCK_METHOD(api_error, write,
              (std::uint64_t write_offset, const data_buffer &data,
               std::size_t &bytes_written),
              (override));

  MOCK_METHOD(void, add, (std::uint64_t handle, open_file_data ofd),
              (override));

  MOCK_METHOD(bool, can_close, (), (const, override));

  MOCK_METHOD(bool, close, (), (override));

  MOCK_METHOD(std::vector<std::uint64_t>, get_handles, (), (const, override));

  MOCK_METHOD((std::map<std::uint64_t, open_file_data>), get_open_data, (),
              (const, override));

  MOCK_METHOD(bool, is_complete, (), (const, override));

  MOCK_METHOD(bool, is_modified, (), (const, override));

  MOCK_METHOD(bool, is_write_supported, (), (const, override));

  MOCK_METHOD(void, remove, (std::uint64_t handle), (override));
};
} // namespace repertory

#endif // TESTS_MOCKS_MOCK_OPEN_FILE_HPP_
