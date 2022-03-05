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
#ifndef INCLUDE_COMM_I_COMM_HPP_
#define INCLUDE_COMM_I_COMM_HPP_

#include "common.hpp"
#include "types/repertory.hpp"

namespace repertory {
class i_comm {
  INTERFACE_SETUP(i_comm);

public:
  virtual api_error get(const std::string &path, json &data, json &error) = 0;

  virtual api_error get(const host_config &hc, const std::string &path, json &data,
                        json &error) = 0;

  virtual api_error get(const std::string &path, const http_parameters &parameters, json &data,
                        json &error) = 0;

  virtual api_error get(const host_config &hc, const std::string &path,
                        const http_parameters &parameters, json &data, json &error) = 0;

  virtual api_error get_range(const std::string &path, const std::uint64_t &data_size,
                              const http_parameters &parameters,
                              const std::string &encryption_token, std::vector<char> &data,
                              const http_ranges &ranges, json &error,
                              const bool &stop_requested) = 0;

  virtual api_error get_range(const host_config &hc, const std::string &path,
                              const std::uint64_t &data_size, const http_parameters &parameters,
                              const std::string &encryption_token, std::vector<char> &data,
                              const http_ranges &ranges, json &error,
                              const bool &stop_requested) = 0;

  virtual api_error get_range_and_headers(const std::string &path, const std::uint64_t &data_size,
                                          const http_parameters &parameters,
                                          const std::string &encryption_token,
                                          std::vector<char> &data, const http_ranges &ranges,
                                          json &error, http_headers &headers,
                                          const bool &stop_requested) = 0;

  virtual api_error get_range_and_headers(const host_config &hc, const std::string &path,
                                          const std::uint64_t &data_size,
                                          const http_parameters &parameters,
                                          const std::string &encryption_token,
                                          std::vector<char> &data, const http_ranges &ranges,
                                          json &error, http_headers &headers,
                                          const bool &stop_requested) = 0;

  virtual api_error get_raw(const std::string &path, const http_parameters &parameters,
                            std::vector<char> &data, json &error, const bool &stop_requested) = 0;

  virtual api_error get_raw(const host_config &hc, const std::string &path,
                            const http_parameters &parameters, std::vector<char> &data, json &error,
                            const bool &stop_requested) = 0;

  virtual api_error post(const std::string &path, json &data, json &error) = 0;

  virtual api_error post(const host_config &hc, const std::string &path, json &data,
                         json &error) = 0;

  virtual api_error post(const std::string &path, const http_parameters &parameters, json &data,
                         json &error) = 0;

  virtual api_error post(const host_config &hc, const std::string &path,
                         const http_parameters &parameters, json &data, json &error) = 0;

  virtual api_error post_file(const std::string &path, const std::string &sourcePath,
                              const http_parameters &parameters, json &data, json &error,
                              const bool &stop_requested) = 0;

  virtual api_error post_file(const host_config &hc, const std::string &path,
                              const std::string &sourcePath, const http_parameters &parameters,
                              json &data, json &error, const bool &stop_requested) = 0;

  virtual api_error post_multipart_file(const std::string &path, const std::string &file_name,
                                        const std::string &source_path,
                                        const std::string &encryption_token, json &data,
                                        json &error, const bool &stop_requested) = 0;

  virtual api_error post_multipart_file(const host_config &hc, const std::string &path,
                                        const std::string &file_name,
                                        const std::string &source_path,
                                        const std::string &encryption_token, json &data,
                                        json &error, const bool &stop_requested) = 0;
};
} // namespace repertory

#endif // INCLUDE_COMM_I_COMM_HPP_
