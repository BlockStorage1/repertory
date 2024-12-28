/*
 Copyright <2018-2024> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

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
#include "types/remote.hpp"

namespace repertory::remote {
#if !defined(_WIN32)
auto create_open_flags(std::uint32_t flags) -> open_flags {
  open_flags ret{};
  {
    const auto val = (flags & 3U);
    ret |= (val == 1U)   ? open_flags::write_only
           : (val == 2U) ? open_flags::read_write
                         : open_flags::read_only;
  }

  const auto set_if_has_flag = [&flags, &ret](auto flag, open_flags o_flag) {
    if ((flags & static_cast<std::uint32_t>(flag)) != 0U) {
      ret |= o_flag;
    }
  };

  set_if_has_flag(O_APPEND, open_flags::append);
  set_if_has_flag(O_ASYNC, open_flags::async);
  set_if_has_flag(O_CLOEXEC, open_flags::clo_exec);
  set_if_has_flag(O_CREAT, open_flags::create);
#if defined(O_DIRECT)
  set_if_has_flag(O_DIRECT, open_flags::direct);
#endif
  set_if_has_flag(O_DIRECTORY, open_flags::directory);
#if defined(O_DSYNC)
  set_if_has_flag(O_DSYNC, open_flags::dsync);
#endif
  set_if_has_flag(O_EXCL, open_flags::excl);
#if defined(O_NOATIME)
  set_if_has_flag(O_NOATIME, open_flags::no_atime);
#endif
  set_if_has_flag(O_NOCTTY, open_flags::no_ctty);
  set_if_has_flag(O_NOFOLLOW, open_flags::no_follow);
  set_if_has_flag(O_NONBLOCK, open_flags::non_blocking);
#if defined(O_PATH)
  set_if_has_flag(O_PATH, open_flags::path);
#endif
  set_if_has_flag(O_SYNC, open_flags::sync);
#if defined(O_TMPFILE)
  set_if_has_flag(O_TMPFILE, open_flags::temp_file);
#endif
  set_if_has_flag(O_TRUNC, open_flags::truncate);

  return ret;
}

auto create_os_open_flags(const open_flags &flags) -> std::uint32_t {
  std::uint32_t ret{};
  const auto set_if_has_flag = [&flags, &ret](auto o_flag, auto flag) -> bool {
    if ((flags & o_flag) == o_flag) {
      ret |= static_cast<std::uint32_t>(flag);
      return true;
    }

    return false;
  };

  if (not set_if_has_flag(open_flags::read_write, O_RDWR)) {
    if (not set_if_has_flag(open_flags::write_only, O_WRONLY)) {
      ret |= static_cast<std::uint32_t>(O_RDONLY);
    }
  }

  set_if_has_flag(open_flags::append, O_APPEND);
  set_if_has_flag(open_flags::async, O_ASYNC);
  set_if_has_flag(open_flags::clo_exec, O_CLOEXEC);
  set_if_has_flag(open_flags::create, O_CREAT);
#if defined(O_DIRECT)
  set_if_has_flag(open_flags::direct, O_DIRECT);
#endif
  set_if_has_flag(open_flags::directory, O_DIRECTORY);
#if defined(O_DSYNC)
  set_if_has_flag(open_flags::dsync, O_DSYNC);
#endif
  set_if_has_flag(open_flags::excl, O_EXCL);
#if defined(O_NOATIME)
  set_if_has_flag(open_flags::no_atime, O_NOATIME);
#endif
  set_if_has_flag(open_flags::no_ctty, O_NOCTTY);
  set_if_has_flag(open_flags::no_follow, O_NOFOLLOW);
  set_if_has_flag(open_flags::non_blocking, O_NONBLOCK);
#if defined(O_PATH)
  set_if_has_flag(open_flags::path, O_PATH);
#endif
  set_if_has_flag(open_flags::sync, O_SYNC);
#if defined(O_TMPFILE)
  set_if_has_flag(open_flags::temp_file, O_TMPFILE);
#endif
  set_if_has_flag(open_flags::truncate, O_TRUNC);

  return ret;
}
#endif
} // namespace repertory::remote
