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
#include "utils/time.hpp"

namespace repertory::utils::time {
void get_local_time_now(struct tm &local_time) {
  std::memset(&local_time, 0, sizeof(local_time));

  const auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
#if defined(_WIN32)
  localtime_s(&local_time, &now);
#else  // !defined(_WIN32)
  localtime_r(&now, &local_time);
#endif // defined(_WIN32)
}

auto get_time_now() -> std::uint64_t {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

#if defined(_WIN32)
// https://stackoverflow.com/questions/321849/strptime-equivalent-on-windows
auto strptime(const char *s, const char *f, struct tm *tm) -> const char * {
  std::istringstream input{s};
  input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
  input >> std::get_time(tm, f);
  if (input.fail()) {
    return nullptr;
  }

  return reinterpret_cast<const char *>(s + input.tellg());
}

// https://www.frenk.com/2009/12/convert-filetime-to-unix-timestamp/
auto unix_time_to_filetime(std::uint64_t unix_time) -> FILETIME {
  auto win_time = unix_time_to_windows_time(unix_time);

  FILETIME file_time{};
  file_time.dwHighDateTime = static_cast<DWORD>(win_time >> 32U);
  file_time.dwLowDateTime = win_time & 0xFFFFFFFF;
  return file_time;
}

auto windows_file_time_to_unix_time(FILETIME win_time) -> std::uint64_t {
  return windows_time_to_unix_time(
      (static_cast<std::uint64_t>(win_time.dwHighDateTime) << 32ULL) |
      static_cast<std::uint64_t>(win_time.dwLowDateTime));
}

auto windows_time_t_to_unix_time(__time64_t win_time) -> std::uint64_t {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::from_time_t(win_time).time_since_epoch())
          .count());
}
#endif // defined(_WIN32)

auto unix_time_to_windows_time(std::uint64_t unix_time) -> std::uint64_t {
  return (unix_time / WIN32_TIME_NANOS_PER_TICK) + WIN32_TIME_CONVERSION;
}

auto windows_time_to_unix_time(std::uint64_t win_time) -> std::uint64_t {
  return (win_time - WIN32_TIME_CONVERSION) * WIN32_TIME_NANOS_PER_TICK;
}
} // namespace repertory::utils::time
