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
#ifndef INCLUDE_DRIVES_REMOTE_I_REMOTEJSON_HPP_
#define INCLUDE_DRIVES_REMOTE_I_REMOTEJSON_HPP_

#include "common.hpp"
#include "comm/packet/packet.hpp"

namespace repertory {
class i_remote_json {
  INTERFACE_SETUP(i_remote_json);

public:
  virtual packet::error_type json_create_directory_snapshot(const std::string &path,
                                                            json &json_data) = 0;

  virtual packet::error_type json_read_directory_snapshot(const std::string &path,
                                                          const remote::file_handle &handle,
                                                          const std::uint32_t &page,
                                                          json &json_data) = 0;

  virtual packet::error_type
  json_release_directory_snapshot(const std::string &path,
                                  const remote::file_handle &handle) = 0;
};
} // namespace repertory

#endif // INCLUDE_DRIVES_REMOTE_I_REMOTEJSON_HPP_
