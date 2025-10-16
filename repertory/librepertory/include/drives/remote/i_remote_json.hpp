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
#ifndef REPERTORY_INCLUDE_DRIVES_REMOTE_I_REMOTE_JSON_HPP_
#define REPERTORY_INCLUDE_DRIVES_REMOTE_I_REMOTE_JSON_HPP_

#include "comm/packet/packet.hpp"

namespace repertory {
class i_remote_json {
  INTERFACE_SETUP(i_remote_json);

public:
  [[nodiscard]] virtual auto
  json_create_directory_snapshot(std::string_view path, json &json_data)
      -> packet::error_type = 0;

  [[nodiscard]] virtual auto
  json_read_directory_snapshot(std::string_view path,
                               remote::file_handle handle, std::uint32_t page,
                               json &json_data) -> packet::error_type = 0;

  [[nodiscard]] virtual auto
  json_release_directory_snapshot(std::string_view path,
                                  remote::file_handle handle)
      -> packet::error_type = 0;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_DRIVES_REMOTE_I_REMOTE_JSON_HPP_
