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
#ifndef INCLUDE_UTILS_PATH_UTILS_HPP_
#define INCLUDE_UTILS_PATH_UTILS_HPP_

#include "common.hpp"

namespace repertory::utils::path {
#ifdef _WIN32
static const std::string directory_seperator = "\\";
static const std::string not_directory_seperator = "/";
#else
static const std::string directory_seperator = "/";
static const std::string not_directory_seperator = "\\";
#endif

// Prototypes
std::string absolute(std::string path);

std::string combine(std::string path, const std::vector<std::string> &paths);

std::string create_api_path(std::string path);

std::string finalize(std::string path);

std::string get_parent_api_path(const std::string &path);

#ifndef _WIN32
std::string get_parent_directory(std::string path);
#endif

bool is_ads_file_path(const std::string &path);

bool is_trash_directory(std::string path);

std::string remove_file_name(std::string path);

#ifndef _WIN32
std::string resolve(std::string path);
#endif

std::string strip_to_file_name(std::string path);
} // namespace repertory::utils::path

#endif // INCLUDE_UTILS_PATH_UTILS_HPP_
