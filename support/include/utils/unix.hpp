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
#if defined(__linux__)
struct autostart_cfg final {
  std::string app_name;
  std::optional<std::string> comment;
  bool enabled{true};
  std::vector<std::string> exec_args;
  std::string exec_path;
  std::optional<std::string> icon_path;
  std::vector<std::string> only_show_in;
  bool terminal{false};
};
#endif // defined(__linux__)

#if defined(__APPLE__)
enum class launchctl_type : std::uint8_t {
  bootout,
  bootstrap,
  kickstart,
};

#if defined(PROJECT_ENABLE_PUGIXML)
struct plist_cfg final {
  std::vector<std::string> args;
  bool keep_alive{false};
  std::string label;
  std::string plist_path;
  bool run_at_load{false};
  std::string stderr_log{"/tmp/stderr.log"};
  std::string stdout_log{"/tmp/stdout.log"};
  std::string working_dir{"/tmp"};
};
#endif // defined(PROJECT_ENABLE_PUGIXML)
#endif // defined(__APPLE__)

using passwd_callback_t = std::function<void(struct passwd *pass)>;

#if defined(__APPLE__)
template <typename thread_t>
[[nodiscard]] auto convert_to_uint64(const thread_t *thread_ptr)
    -> std::uint64_t;
#else  // !defined(__APPLE__)
[[nodiscard]] auto convert_to_uint64(const pthread_t &thread) -> std::uint64_t;
#endif // defined(__APPLE__)

#if defined(__linux__)
[[nodiscard]] auto create_autostart_entry(const autostart_cfg &cfg,
                                          bool overwrite_existing = true)
    -> bool;
#endif // defined(__linux__)

[[nodiscard]] auto get_last_error_code() -> int;

[[nodiscard]] auto get_thread_id() -> std::uint64_t;

[[nodiscard]] auto is_uid_member_of_group(uid_t uid, gid_t gid) -> bool;

void set_last_error_code(int error_code);

[[nodiscard]] auto use_getpwuid(uid_t uid, passwd_callback_t callback)
    -> utils::result;

#if defined(__linux__)
[[nodiscard]] auto remove_autostart_entry(std::string_view name) -> bool;
#endif // defined(__linux__)

#if defined(__APPLE__)
#if defined(PROJECT_ENABLE_PUGIXML)
[[nodiscard]] auto generate_launchd_plist(const plist_cfg &cfg,
                                          bool overwrite_existing = true)
    -> bool;
#endif // defined(PROJECT_ENABLE_PUGIXML)

#if defined(PROJECT_ENABLE_SPDLOG) || defined(PROJECT_ENABLE_FMT)
[[nodiscard]] auto launchctl_command(std::string_view label,
                                     launchctl_type type) -> int;

[[nodiscard]] auto remove_launchd_plist(std::string_view plist_path,
                                        std::string_view label,
                                        bool should_bootout) -> bool;
#endif // defined(PROJECT_ENABLE_SPDLOG) || defined(PROJECT_ENABLE_FMT)
#endif // defined(__APPLE__)

// template implementations
#if defined(__APPLE__)
template <typename thread_t>
[[nodiscard]] auto convert_to_uint64(const thread_t *thread_ptr)
    -> std::uint64_t {
  return static_cast<std::uint64_t>(
      reinterpret_cast<std::uintptr_t>(thread_ptr));
}
#endif // defined(__APPLE__)
} // namespace repertory::utils

#endif // !defined(_WIN32)
#endif // REPERTORY_INCLUDE_UTILS_UNIX_HPP_
