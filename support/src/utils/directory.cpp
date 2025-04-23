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
#include "utils/file.hpp"

#include "utils/common.hpp"
#include "utils/error.hpp"
#if defined(_WIN32)
#include "utils/path.hpp"
#endif // defined(_WIN32)

namespace repertory::utils::directory {
auto temp() -> std::string {
  REPERTORY_USES_FUNCTION_NAME();

#if defined(_WIN32)
  auto ret{utils::get_environment_variable("TEMP")};
  if (ret.empty()) {
    ret = utils::path::combine(utils::get_environment_variable("LOCALAPPDATA"),
                               {"Temp"});
  }
#else  // !defined(_WIN32)
  std::string ret{"/tmp"};
#endif // defined(_WIN32)

  if (not utils::file::directory{ret}.create_directory()) {
    utils::error::handle_error(function_name,
                               utils::error::create_error_message(
                                   {"failed to create directory", ret}));
  }

  return ret;
}
} // namespace repertory::utils::directory
