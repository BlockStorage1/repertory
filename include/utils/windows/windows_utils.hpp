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
#ifndef INCLUDE_UTILS_WINDOWS_WINDOWS_UTILS_HPP_
#define INCLUDE_UTILS_WINDOWS_WINDOWS_UTILS_HPP_
#ifdef _WIN32

#include "common.hpp"
#include "types/remote.hpp"
#include "types/repertory.hpp"

namespace repertory::utils {
remote::file_time filetime_to_unix_time(const FILETIME &ft);

DWORD get_last_error_code();

const std::string &get_local_app_data_directory();

std::uint64_t get_accessed_time_from_meta(const api_meta_map &meta);

DWORD get_attributes_from_meta(const api_meta_map &meta);

std::uint64_t get_changed_time_from_meta(const api_meta_map &meta);

std::uint64_t get_creation_time_from_meta(const api_meta_map &meta);

std::uint64_t get_written_time_from_meta(const api_meta_map &meta);

std::uint64_t get_thread_id();

bool is_process_elevated();

int run_process_elevated(int argc, char *argv[]);

void set_last_error_code(DWORD errorCode);

NTSTATUS translate_api_error(const api_error &e);

int unix_access_mask_to_windows(std::int32_t mask);

int unix_open_flags_to_flags_and_perms(const remote::file_mode &mode, const remote::open_flags &flags,
                                       std::int32_t &perms);

void unix_time_to_filetime(const remote::file_time &ts, FILETIME &ft);

remote::file_time time64_to_unix_time(const __time64_t &t);
} // namespace repertory::utils

#endif // _WIN32
#endif // INCLUDE_UTILS_WINDOWS_WINDOWS_UTILS_HPP_
