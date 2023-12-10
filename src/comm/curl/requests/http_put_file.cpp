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
#include "comm/curl/requests/http_put_file.hpp"

#include "utils/string_utils.hpp"

namespace repertory::curl::requests {
auto http_put_file::set_method(CURL *curl, stop_type &stop_requested) const
    -> bool {
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

  if (source_path.empty()) {
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, 0L);
    return true;
  }

  if (reader) {
    curl_easy_setopt(curl, CURLOPT_READDATA, reader.get());
    curl_easy_setopt(
        curl, CURLOPT_READFUNCTION,
        static_cast<curl_read_callback>(
            utils::encryption::encrypting_reader::reader_function));
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, reader->get_total_size());
    return true;
  }

  read_info = std::make_shared<read_file_info>(read_file_info{
      stop_requested,
  });

  if (native_file::create_or_open(source_path, read_info->nf) !=
      api_error::success) {
    return false;
  }

  read_info->nf->set_auto_close(true);

  std::uint64_t file_size{};
  if (not read_info->nf->get_file_size(file_size)) {
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_READDATA, read_info.get());
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_file_data);
  curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, file_size);

  return true;
}
} // namespace repertory::curl::requests
