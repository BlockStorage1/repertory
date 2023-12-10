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
#ifndef INCLUDE_UTILS_WINDOWS_WINDOWS_UTILS_HPP_
#define INCLUDE_UTILS_WINDOWS_WINDOWS_UTILS_HPP_
#ifdef _WIN32

#include "types/remote.hpp"
#include "types/repertory.hpp"

namespace repertory::utils {
[[nodiscard]] auto filetime_to_unix_time(const FILETIME &ft)
    -> remote::file_time;

[[nodiscard]] auto get_last_error_code() -> DWORD;

[[nodiscard]] auto get_local_app_data_directory() -> const std::string &;

[[nodiscard]] auto get_accessed_time_from_meta(const api_meta_map &meta)
    -> std::uint64_t;

[[nodiscard]] auto get_changed_time_from_meta(const api_meta_map &meta)
    -> std::uint64_t;

[[nodiscard]] auto get_creation_time_from_meta(const api_meta_map &meta)
    -> std::uint64_t;

[[nodiscard]] auto get_written_time_from_meta(const api_meta_map &meta)
    -> std::uint64_t;

[[nodiscard]] auto get_thread_id() -> std::uint64_t;

[[nodiscard]] auto is_process_elevated() -> bool;

[[nodiscard]] auto run_process_elevated(std::vector<const char *> args) -> int;

void set_last_error_code(DWORD errorCode);

[[nodiscard]] auto from_api_error(const api_error &e) -> NTSTATUS;

auto strptime(const char *s, const char *f, struct tm *tm) -> const char *;

[[nodiscard]] auto unix_access_mask_to_windows(std::int32_t mask) -> int;

[[nodiscard]] auto
unix_open_flags_to_flags_and_perms(const remote::file_mode &mode,
                                   const remote::open_flags &flags,
                                   std::int32_t &perms) -> int;

void unix_time_to_filetime(const remote::file_time &ts, FILETIME &ft);

[[nodiscard]] auto time64_to_unix_time(const __time64_t &t)
    -> remote::file_time;
} // namespace repertory::utils

#endif // _WIN32
#endif // INCLUDE_UTILS_WINDOWS_WINDOWS_UTILS_HPP_
