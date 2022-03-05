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
#ifndef INCLUDE_DOWNLOAD_I_DOWNLOAD_MANAGER_HPP_
#define INCLUDE_DOWNLOAD_I_DOWNLOAD_MANAGER_HPP_

#include "common.hpp"
#include "download/i_download.hpp"
#include "types/repertory.hpp"

namespace repertory {
class i_download_manager {
  INTERFACE_SETUP(i_download_manager);

public:
  virtual api_error allocate(const std::uint64_t &handle, filesystem_item &fsi,
                             const std::uint64_t &size,
                             const i_download::allocator_callback &allocator) = 0;

  virtual bool contains_restore(const std::string &api_path) const = 0;

  virtual api_error download_file(const std::uint64_t &handle, filesystem_item &fsi) = 0;

  virtual bool is_processing(const std::string &api_path) const = 0;

  virtual bool pause_download(const std::string &api_path) = 0;

  virtual api_error read_bytes(const std::uint64_t &handle, filesystem_item &fsi,
                               std::size_t read_size, const std::uint64_t &read_offset,
                               std::vector<char> &data) = 0;

  virtual void rename_download(const std::string &from_api_path,
                               const std::string &to_api_path) = 0;

  virtual api_error resize(const std::uint64_t &handle, filesystem_item &fsi,
                           const std::uint64_t &size) = 0;

  virtual void resume_download(const std::string &api_path) = 0;

  virtual api_error write_bytes(const std::uint64_t &handle, filesystem_item &fsi,
                                const std::uint64_t &write_offset, std::vector<char> data,
                                std::size_t &bytes_written) = 0;
};
} // namespace repertory

#endif // INCLUDE_DOWNLOAD_I_DOWNLOAD_MANAGER_HPP_
