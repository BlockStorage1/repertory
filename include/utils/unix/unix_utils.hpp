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
#ifndef INCLUDE_UTILS_UNIX_UNIX_UTILS_HPP_
#define INCLUDE_UTILS_UNIX_UNIX_UTILS_HPP_
#ifndef _WIN32

#include "types/remote.hpp"
#include "types/repertory.hpp"

namespace repertory::utils {
#if __linux__
inline const std::array<std::string, 4U> attribute_namespaces = {
    "security",
    "system",
    "trusted",
    "user",
};
#endif

#if __APPLE__
template <typename t>
[[nodiscard]] auto convert_to_uint64(const t *ptr) -> std::uint64_t;
#else
[[nodiscard]] auto convert_to_uint64(const pthread_t &thread) -> std::uint64_t;
#endif

[[nodiscard]] auto from_api_error(const api_error &err) -> int;

[[nodiscard]] auto get_last_error_code() -> int;

[[nodiscard]] auto get_thread_id() -> std::uint64_t;

[[nodiscard]] auto is_uid_member_of_group(const uid_t &uid, const gid_t &gid)
    -> bool;

void set_last_error_code(int error_code);

[[nodiscard]] auto to_api_error(int err) -> api_error;

[[nodiscard]] auto unix_error_to_windows(int err) -> std::int32_t;

[[nodiscard]] auto unix_time_to_windows_time(const remote::file_time &file_time)
    -> UINT64;

void use_getpwuid(uid_t uid, std::function<void(struct passwd *pass)> callback);

void windows_create_to_unix(const UINT32 &create_options,
                            const UINT32 &granted_access, std::uint32_t &flags,
                            remote::file_mode &mode);

[[nodiscard]] auto windows_time_to_unix_time(std::uint64_t win_time)
    -> remote::file_time;

// template implementations
#if __APPLE__
template <typename t>
[[nodiscard]] auto convert_to_uint64(const t *v) -> std::uint64_t {
  return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(v));
}
#endif
} // namespace repertory::utils

#endif // !_WIN32
#endif // INCLUDE_UTILS_UNIX_UNIX_UTILS_HPP_
