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
#ifndef INCLUDE_DOWNLOAD_I_DOWNLOAD_HPP_
#define INCLUDE_DOWNLOAD_I_DOWNLOAD_HPP_

#include "common.hpp"
#include "types/repertory.hpp"

namespace repertory {
class i_download {
  INTERFACE_SETUP(i_download);

public:
  typedef std::function<api_error()> allocator_callback;

  typedef std::function<void(std::uint64_t old_size, std::uint64_t new_size, bool changed)>
      completer_callback;

public:
  virtual api_error allocate(const std::uint64_t &handle, const std::uint64_t &size,
                             const allocator_callback &allocator,
                             const completer_callback &completer) = 0;

  virtual api_error download_all() = 0;

  virtual api_error get_result() const = 0;

  virtual std::string get_source_path() const = 0;

  virtual void get_state_information(filesystem_item &fsi, std::size_t &chunk_size,
                                     std::size_t &last_chunk_size,
                                     boost::dynamic_bitset<> &read_state,
                                     boost::dynamic_bitset<> &write_state) = 0;

  virtual bool get_write_supported() const = 0;

  virtual void notify_stop_requested() = 0;

  virtual bool pause() = 0;

  virtual api_error read_bytes(const std::uint64_t &handle, std::size_t read_size,
                               const std::uint64_t &read_offset, std::vector<char> &data) = 0;

  virtual void reset_timeout(const bool &file_closed) = 0;

  virtual void resume() = 0;

  virtual void set_api_path(const std::string &api_path) = 0;

  virtual void set_disable_download_end(const bool &disable) = 0;

  virtual api_error write_bytes(const std::uint64_t &handle, const std::uint64_t &write_offset,
                                std::vector<char> data, std::size_t &bytes_written,
                                const completer_callback &completer) = 0;
};

typedef std::shared_ptr<i_download> download_ptr;
} // namespace repertory

#endif // INCLUDE_DOWNLOAD_I_DOWNLOAD_HPP_
