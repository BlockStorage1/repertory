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
#include "comm/curl/requests/http_put_file.hpp"

#include "utils/error_utils.hpp"

namespace repertory::curl::requests {
auto http_put_file::set_method(CURL *curl, stop_type &stop_requested) const
    -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(curl, CURLOPT_INFILE, nullptr);

  if (source_path.empty()) {
    curl_easy_setopt(curl, CURLOPT_INFILESIZE, 0L);
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
      utils::file::file::open_or_create_file(source_path),
  });

  if (not *read_info->file) {
    utils::error::raise_url_error(function_name, get_path(), source_path,
                                  std::runtime_error("failed to open file"));
    return false;
  }

  auto opt_size = read_info->file->size();
  if (not opt_size.has_value()) {
    utils::error::raise_url_error(
        function_name, get_path(), source_path,
        std::runtime_error("failed to get file size"));
    return false;
  }

  auto file_size = opt_size.value();
  if (file_size == 0U) {
    curl_easy_setopt(curl, CURLOPT_INFILESIZE, 0L);
    return true;
  }

  curl_easy_setopt(curl, CURLOPT_READDATA, read_info.get());
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, curl_file_reader);
  curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, file_size);

  return true;
}
} // namespace repertory::curl::requests
