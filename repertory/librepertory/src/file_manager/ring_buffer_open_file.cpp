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
#include "file_manager/ring_buffer_open_file.hpp"

#include "file_manager/open_file_base.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "utils/common.hpp"
#include "utils/error_utils.hpp"
#include "utils/path.hpp"

namespace repertory {
ring_buffer_open_file::ring_buffer_open_file(std::string buffer_directory,
                                             std::uint64_t chunk_size,
                                             std::uint8_t chunk_timeout,
                                             filesystem_item fsi,
                                             i_provider &provider,
                                             std::size_t ring_size)
    : ring_buffer_base(chunk_size, chunk_timeout, fsi, provider, ring_size,
                       false),
      source_path_(utils::path::combine(buffer_directory,
                                        {
                                            utils::create_uuid_string(),
                                        })) {
  if (not can_handle_file(fsi.size, chunk_size, ring_size)) {
    throw std::runtime_error("file size is less than ring buffer size");
  }
}

ring_buffer_open_file::~ring_buffer_open_file() {
  REPERTORY_USES_FUNCTION_NAME();

  close();

  if (not nf_) {
    return;
  }

  nf_->close();
  nf_.reset();

  if (not utils::file::file(source_path_).remove()) {
    utils::error::raise_api_path_error(
        function_name, get_api_path(), source_path_,
        utils::get_last_error_code(), "failed to delete file");
  }
}

auto ring_buffer_open_file::can_handle_file(std::uint64_t file_size,
                                            std::size_t chunk_size,
                                            std::size_t ring_size) -> bool {
  return file_size >= (static_cast<std::uint64_t>(ring_size) * chunk_size);
}

auto ring_buffer_open_file::native_operation(
    i_open_file::native_operation_callback callback) -> api_error {
  return do_io([&]() -> api_error { return callback(nf_->get_handle()); });
}

auto ring_buffer_open_file::on_check_start() -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  if (nf_) {
    return true;
  }

  auto buffer_directory{utils::path::get_parent_path(source_path_)};
  if (not utils::file::directory(buffer_directory).create_directory()) {
    throw std::runtime_error(
        fmt::format("failed to create buffer directory|path|{}|err|{}",
                    buffer_directory, utils::get_last_error_code()));
  }

  nf_ = utils::file::file::open_or_create_file(source_path_);
  if (not nf_ || not *nf_) {
    throw std::runtime_error(fmt::format("failed to create buffer file|err|{}",
                                         utils::get_last_error_code()));
  }

  if (not nf_->truncate(get_ring_size() * get_chunk_size())) {
    nf_->close();
    nf_.reset();

    throw std::runtime_error(fmt::format("failed to resize buffer file|err|{}",
                                         utils::get_last_error_code()));
  }

  return false;
}

auto ring_buffer_open_file::on_chunk_downloaded(
    std::size_t chunk, const data_buffer &buffer) -> api_error {
  return do_io([&]() -> api_error {
    std::size_t bytes_written{};
    if (nf_->write(buffer, (chunk % get_ring_size()) * get_chunk_size(),
                   &bytes_written)) {
      return api_error::success;
    }

    return api_error::os_error;
  });
}

auto ring_buffer_open_file::on_read_chunk(
    std::size_t chunk, std::size_t read_size, std::uint64_t read_offset,
    data_buffer &data, std::size_t &bytes_read) -> api_error {
  data_buffer buffer(read_size);
  auto res = do_io([&]() -> api_error {
    return nf_->read(
               buffer,
               (((chunk % get_ring_size()) * get_chunk_size()) + read_offset),
               &bytes_read)
               ? api_error::success
               : api_error::os_error;
  });

  if (res != api_error::success) {
    return res;
  }

  data.insert(data.end(), buffer.begin(), buffer.end());
  return api_error::success;
}

auto ring_buffer_open_file::use_buffer(
    std::size_t /* chunk */,
    std::function<api_error(data_buffer &)> func) -> api_error {
  data_buffer buffer;
  return func(buffer);
}
} // namespace repertory
