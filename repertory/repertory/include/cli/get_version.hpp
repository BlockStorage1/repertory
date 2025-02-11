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
#ifndef REPERTORY_INCLUDE_CLI_VERSION_HPP_
#define REPERTORY_INCLUDE_CLI_VERSION_HPP_

#include "version.hpp"

namespace repertory::cli::actions {
template <typename drive> inline void version(std::vector<const char *> args) {
  std::cout << "Repertory core version: " << project_get_version() << std::endl;
  std::cout << "Repertory git revision: " << project_get_git_rev() << std::endl;
  drive::display_version_information(args);
}
} // namespace repertory::cli::actions

#endif // REPERTORY_INCLUDE_CLI_VERSION_HPP_
