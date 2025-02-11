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
#ifndef REPERTORY_INCLUDE_UTILS_UNIX_HPP_
#define REPERTORY_INCLUDE_UTILS_UNIX_HPP_
#if !defined(_WIN32)

#include "utils/common.hpp"
#include "utils/config.hpp"

namespace repertory::utils {
using passwd_callback_t = std::function<void(struct passwd *pass)>;

#if defined(__APPLE__)
template <typename thread_t>
[[nodiscard]] auto convert_to_uint64(const thread_t *thread_ptr)
    -> std::uint64_t;
#else  // !defined(__APPLE__)
[[nodiscard]] auto convert_to_uint64(const pthread_t &thread) -> std::uint64_t;
#endif // defined(__APPLE__)

[[nodiscard]] auto get_last_error_code() -> int;

[[nodiscard]] auto get_thread_id() -> std::uint64_t;

[[nodiscard]] auto is_uid_member_of_group(uid_t uid, gid_t gid) -> bool;

void set_last_error_code(int error_code);

[[nodiscard]] auto use_getpwuid(uid_t uid, passwd_callback_t callback)
    -> utils::result;

// template implementations
#if defined(__APPLE__)
template <typename t>
[[nodiscard]] auto convert_to_uint64(const thread_t *thread_ptr)
    -> std::uint64_t {
  return static_cast<std::uint64_t>(
      reinterpret_cast<std::uintptr_t>(thread_ptr));
}
#endif // defined(__APPLE__)
} // namespace repertory::utils

#endif // !defined(_WIN32)
#endif // REPERTORY_INCLUDE_UTILS_UNIX_HPP_
