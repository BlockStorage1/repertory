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
#ifndef REPERTORY_INCLUDE_UTILS_WINDOWS_HPP_
#define REPERTORY_INCLUDE_UTILS_WINDOWS_HPP_
#if defined(_WIN32)

#include "utils/config.hpp"

namespace repertory::utils {
void create_console();

void free_console();

[[nodiscard]] auto get_local_app_data_directory() -> const std::string &;

[[nodiscard]] auto get_last_error_code() -> DWORD;

[[nodiscard]] auto get_thread_id() -> std::uint64_t;

[[nodiscard]] auto is_process_elevated() -> bool;

[[nodiscard]] auto run_process_elevated(std::vector<const char *> args) -> int;

void set_last_error_code(DWORD errorCode);
} // namespace repertory::utils

#endif // defined(_WIN32)
#endif // REPERTORY_INCLUDE_UTILS_WINDOWS_HPP_
