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
#ifndef REPERTORY_INCLUDE_UTILS_WINDOWS_WINDOWS_UTILS_HPP_
#define REPERTORY_INCLUDE_UTILS_WINDOWS_WINDOWS_UTILS_HPP_
#if defined(_WIN32)

#include "types/remote.hpp"
#include "types/repertory.hpp"
#include "utils/windows.hpp"

namespace repertory::utils {
[[nodiscard]] auto
get_accessed_time_from_meta(const api_meta_map &meta) -> std::uint64_t;

[[nodiscard]] auto
get_changed_time_from_meta(const api_meta_map &meta) -> std::uint64_t;

[[nodiscard]] auto
get_creation_time_from_meta(const api_meta_map &meta) -> std::uint64_t;

[[nodiscard]] auto
get_written_time_from_meta(const api_meta_map &meta) -> std::uint64_t;

[[nodiscard]] auto from_api_error(const api_error &e) -> NTSTATUS;

[[nodiscard]] auto unix_access_mask_to_windows(std::int32_t mask) -> int;

[[nodiscard]] auto
unix_open_flags_to_flags_and_perms(const remote::file_mode &mode,
                                   const remote::open_flags &flags,
                                   std::int32_t &perms) -> int;
} // namespace repertory::utils

#endif // _WIN32
#endif // REPERTORY_INCLUDE_UTILS_WINDOWS_WINDOWS_UTILS_HPP_
