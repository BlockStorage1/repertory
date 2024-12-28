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
#include "comm/curl/requests/http_request_base.hpp"

#include "utils/file.hpp"
#include "utils/string.hpp"

namespace repertory::curl::requests {
auto curl_file_reader(char *buffer, size_t size, size_t nitems, void *instream)
    -> size_t {
  auto *read_info = reinterpret_cast<read_file_info *>(instream);

  std::size_t bytes_read{};
  auto ret =
      read_info->file->read(reinterpret_cast<unsigned char *>(buffer),
                            size * nitems, read_info->offset, &bytes_read);
  if (ret) {
    read_info->offset += bytes_read;
  }

  return ret && not read_info->stop_requested ? bytes_read
                                              : CURL_READFUNC_ABORT;
}
} // namespace repertory::curl::requests
