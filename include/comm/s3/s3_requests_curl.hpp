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
#ifndef INCLUDE_COMM_S3_S3_REQUESTS_CURL_HPP_
#define INCLUDE_COMM_S3_S3_REQUESTS_CURL_HPP_
#if defined(REPERTORY_ENABLE_S3)

#include "comm/i_http_comm.hpp"
#include "types/repertory.hpp"
#include "types/s3.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
[[nodiscard]] auto create_directory_object_request_impl(
    i_http_comm &client, const s3_config &config,
    const std::string &object_name, long &response_code) -> bool;

[[nodiscard]] auto delete_object_request_impl(i_http_comm &client,
                                              const s3_config &config,
                                              const std::string &object_name,
                                              long &response_code) -> bool;

[[nodiscard]] auto head_object_request_impl(i_http_comm &client,
                                            const s3_config &config,
                                            const std::string &object_name,
                                            head_object_result &result,
                                            long &response_code) -> bool;

[[nodiscard]] auto
list_directories_request_impl(i_http_comm &client, const s3_config &config,
                              list_directories_result &result,
                              long &response_code) -> bool;

[[nodiscard]] auto
list_files_request_impl(i_http_comm &client, const s3_config &config,
                        const get_api_file_token_callback &get_api_file_token,
                        const get_name_callback &get_name,
                        list_files_result &result, long &response_code) -> bool;

[[nodiscard]] auto list_objects_in_directory_request_impl(
    i_http_comm &client, const s3_config &config,
    const std::string &object_name, meta_provider_callback meta_provider,
    list_objects_result &result, long &response_code) -> bool;

[[nodiscard]] auto list_objects_request_impl(i_http_comm &client,
                                             const s3_config &config,
                                             list_objects_result &result,
                                             long &response_code) -> bool;

[[nodiscard]] auto
put_object_request_impl(i_http_comm &client, const s3_config &config,
                        std::string object_name, const std::string &source_path,
                        const std::string &encryption_token,
                        get_key_callback get_key, set_key_callback set_key,
                        long &response_code, stop_type &stop_requested) -> bool;

[[nodiscard]] auto read_object_request_impl(
    i_http_comm &client, const s3_config &config,
    const std::string &object_name, std::size_t size, std::uint64_t offset,
    data_buffer &data, long &response_code, stop_type &stop_requested) -> bool;
} // namespace repertory

#endif // REPERTORY_ENABLE_S3
#endif // INCLUDE_COMM_S3_S3_REQUESTS_CURL_HPP_
