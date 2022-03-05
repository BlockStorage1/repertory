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
#ifndef INCLUDE_TYPES_SKYNET_HPP_
#define INCLUDE_TYPES_SKYNET_HPP_

#include "common.hpp"
#include "types/repertory.hpp"

namespace repertory {
static const std::array<std::string, 2u> DEFAULT_SKYNET_URLS = {
    "siasky.net",
    "https://account.siasky.net",
};

struct skynet_config {
  std::string encryption_token;
  std::vector<host_config> portal_list = {
      {"", "", 443, DEFAULT_SKYNET_URLS[0u], "", "https", 60000, DEFAULT_SKYNET_URLS[1u], "", ""},
  };

  static std::vector<host_config> from_string(const std::string &list);

  std::string to_string() const { return to_string(portal_list); }

  static std::string to_string(const std::vector<host_config> &list);
};

struct skylink_import {
  std::string directory;
  std::string file_name; // Ignored - informational only
  std::string skylink;
  std::string token;

  static skylink_import from_json(const json &j);

  static skylink_import from_string(const std::string &str);

  json to_json() const;
};

typedef std::vector<skylink_import> skylink_import_list;
} // namespace repertory

#endif // INCLUDE_TYPES_SKYNET_HPP_
