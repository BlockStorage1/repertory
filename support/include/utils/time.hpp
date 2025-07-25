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
#ifndef REPERTORY_INCLUDE_UTILS_TIME_HPP_
#define REPERTORY_INCLUDE_UTILS_TIME_HPP_

#include "utils/config.hpp"

namespace repertory::utils::time {
inline constexpr auto NANOS_PER_SECOND{1000000000ULL};
inline constexpr auto WIN32_TIME_CONVERSION{116444736000000000ULL};
inline constexpr auto WIN32_TIME_NANOS_PER_TICK{100ULL};

#if defined(PROJECT_ENABLE_SPDLOG) || defined(PROJECT_ENABLE_FMT)
[[nodiscard]] inline auto convert_to_utc(time_t time) -> std::time_t {
  auto calendar_time = fmt::gmtime(time);
  return std::mktime(&calendar_time);
}
#endif // defined(PROJECT_ENABLE_SPDLOG) || defined(PROJECT_ENABLE_FMT)

#if defined(PROJECT_ENABLE_SPDLOG) || defined(PROJECT_ENABLE_FMT)
[[nodiscard]] inline auto get_current_time_utc() -> std::time_t {
  auto calendar_time = fmt::gmtime(std::time(nullptr));
  return std::mktime(&calendar_time);
}
#endif // defined(PROJECT_ENABLE_SPDLOG) || defined(PROJECT_ENABLE_FMT)

void get_local_time_now(struct tm &local_time);

[[nodiscard]] auto get_time_now() -> std::uint64_t;

#if defined(_WIN32)
auto strptime(const char *s, const char *f, struct tm *tm) -> const char *;

[[nodiscard]] auto unix_time_to_filetime(std::uint64_t unix_time) -> FILETIME;

[[nodiscard]] auto
windows_file_time_to_unix_time(FILETIME win_time) -> std::uint64_t;

[[nodiscard]] auto
windows_time_t_to_unix_time(__time64_t win_time) -> std::uint64_t;
#endif // defined(_WIN32)

[[nodiscard]] auto
unix_time_to_windows_time(std::uint64_t unix_time) -> std::uint64_t;

[[nodiscard]] auto
windows_time_to_unix_time(std::uint64_t win_time) -> std::uint64_t;
} // namespace repertory::utils::time

#endif // REPERTORY_INCLUDE_UTILS_TIME_HPP_
