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
#include "types/repertory.hpp"

#include "types/startup_exception.hpp"

namespace repertory {
static const std::unordered_map<api_error, std::string> LOOKUP = {
    {api_error::success, "success"},
    {api_error::access_denied, "access_denied"},
    {api_error::bad_address, "bad_address"},
    {api_error::buffer_overflow, "buffer_overflow"},
    {api_error::buffer_too_small, "buffer_too_small"},
    {api_error::comm_error, "comm_error"},
    {api_error::decryption_error, "decryption_error"},
    {api_error::directory_end_of_files, "directory_end_of_files"},
    {api_error::directory_exists, "directory_exists"},
    {api_error::directory_not_empty, "directory_not_empty"},
    {api_error::directory_not_found, "directory_not_found"},
    {api_error::download_failed, "download_failed"},
    {api_error::download_incomplete, "download_incomplete"},
    {api_error::download_stopped, "download_stopped"},
    {api_error::empty_ring_buffer_chunk_size, "empty_ring_buffer_chunk_size"},
    {api_error::empty_ring_buffer_size, "empty_ring_buffer_size"},
    {api_error::error, "error"},
    {api_error::file_in_use, "file_in_use"},
    {api_error::file_size_mismatch, "file_size_mismatch"},
    {api_error::incompatible_version, "incompatible_version"},
    {api_error::invalid_handle, "invalid_handle"},
    {api_error::invalid_operation, "invalid_operation"},
    {api_error::invalid_ring_buffer_multiple, "invalid_ring_buffer_multiple"},
    {api_error::invalid_ring_buffer_size, "invalid_ring_buffer_size"},
    {api_error::invalid_version, "invalid_version"},
    {api_error::item_exists, "item_exists"},
    {api_error::item_not_found, "item_not_found"},
    {api_error::no_disk_space, "no_disk_space"},
    {api_error::not_implemented, "not_implemented"},
    {api_error::not_supported, "not_supported"},
    {api_error::os_error, "os_error"},
    {api_error::out_of_memory, "out_of_memory"},
    {api_error::permission_denied, "permission_denied"},
    {api_error::upload_failed, "upload_failed"},
    {api_error::upload_stopped, "upload_stopped"},
    {api_error::xattr_buffer_small, "xattr_buffer_small"},
    {api_error::xattr_exists, "xattr_exists"},
    {api_error::xattr_not_found, "xattr_not_found"},
    {api_error::xattr_too_big, "xattr_too_big"},
};

auto api_error_from_string(std::string_view s) -> api_error {
  if (LOOKUP.size() != static_cast<std::size_t>(api_error::ERROR_COUNT)) {
    throw startup_exception("undefined api_error strings");
  }

  const auto it =
      std::find_if(LOOKUP.begin(), LOOKUP.end(),
                   [&s](const std::pair<api_error, std::string> &kv) -> bool {
                     return kv.second == s;
                   });
  return it == LOOKUP.end() ? api_error::error : it->first;
}

auto api_error_to_string(const api_error &error) -> const std::string & {
  if (LOOKUP.size() != static_cast<std::size_t>(api_error::ERROR_COUNT)) {
    throw startup_exception("undefined api_error strings");
  }

  return LOOKUP.at(error);
}
} // namespace repertory
