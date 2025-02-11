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
#if !defined(_WIN32)

#include "utils/unix.hpp"
#include "utils/collection.hpp"

namespace repertory::utils {
#if !defined(__APPLE__)
auto convert_to_uint64(const pthread_t &thread) -> std::uint64_t {
  return static_cast<std::uint64_t>(thread);
}
#endif // !defined(__APPLE__)

auto get_last_error_code() -> int { return errno; }

auto get_thread_id() -> std::uint64_t {
  return convert_to_uint64(pthread_self());
}

auto is_uid_member_of_group(uid_t uid, gid_t gid) -> bool {
  std::vector<gid_t> groups{};
  auto res = use_getpwuid(uid, [&groups](struct passwd *pass) {
    int group_count{};
    if (getgrouplist(pass->pw_name, pass->pw_gid, nullptr, &group_count) < 0) {
      groups.resize(static_cast<std::size_t>(group_count));
#if defined(__APPLE__)
      getgrouplist(pass->pw_name, pass->pw_gid,
                   reinterpret_cast<int *>(groups.data()), &group_count);
#else  // !defined(__APPLE__)
      getgrouplist(pass->pw_name, pass->pw_gid, groups.data(), &group_count);
#endif // defined(__APPLE__)
    }
  });

  if (not res) {
    throw utils::error::create_exception(res.function_name,
                                         {"use_getpwuid failed", res.reason});
  }

  return collection::includes(groups, gid);
}

auto use_getpwuid(uid_t uid, passwd_callback_t callback) -> result {
  REPERTORY_USES_FUNCTION_NAME();

  static std::mutex mtx{};
  mutex_lock lock{mtx};

  auto *temp_pw = getpwuid(uid);
  if (temp_pw == nullptr) {
    return {
        std::string{function_name},
        false,
        "'getpwuid' returned nullptr",
    };
  }

  callback(temp_pw);
  return {std::string{function_name}};
}
} // namespace repertory::utils

#endif // !defined(_WIN32)
