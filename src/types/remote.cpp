/*
 Copyright <2018-2023> <scott.e.graves@protonmail.com>

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
#ifndef _WIN32
auto create_open_flags(std::uint32_t flags) -> open_flags {
  open_flags ret{};
  {
    const auto f = (flags & 3u);
    ret |= (f == 1u)   ? open_flags::write_only
           : (f == 2u) ? open_flags::read_write
                       : open_flags::read_only;
  }
  if (flags & static_cast<std::uint32_t>(O_CREAT)) {
    ret |= open_flags::create;
  }

  if (flags & static_cast<std::uint32_t>(O_EXCL)) {
    ret |= open_flags::excl;
  }

  if (flags & static_cast<std::uint32_t>(O_NOCTTY)) {
    ret |= open_flags::no_ctty;
  }

  if (flags & static_cast<std::uint32_t>(O_TRUNC)) {
    ret |= open_flags::truncate;
  }

  if (flags & static_cast<std::uint32_t>(O_APPEND)) {
    ret |= open_flags::append;
  }

  if (flags & static_cast<std::uint32_t>(O_NONBLOCK)) {
    ret |= open_flags::non_blocking;
  }

  if (flags & static_cast<std::uint32_t>(O_SYNC)) {
    ret |= open_flags::sync;
  }

  if (flags & static_cast<std::uint32_t>(O_ASYNC)) {
    ret |= open_flags::async;
  }

  if (flags & static_cast<std::uint32_t>(O_DIRECTORY)) {
    ret |= open_flags::directory;
  }

  if (flags & static_cast<std::uint32_t>(O_NOFOLLOW)) {
    ret |= open_flags::no_follow;
  }

  if (flags & static_cast<std::uint32_t>(O_CLOEXEC)) {
    ret |= open_flags::clo_exec;
  }
#ifdef O_DIRECT
  if (flags & static_cast<std::uint32_t>(O_DIRECT)) {
    ret |= open_flags::direct;
  }
#endif
#ifdef O_NOATIME
  if (flags & static_cast<std::uint32_t>(O_NOATIME)) {
    ret |= open_flags::no_atime;
  }
#endif
#ifdef O_PATH
  if (flags & static_cast<std::uint32_t>(O_PATH)) {
    ret |= open_flags::path;
  }
#endif
#ifdef O_TMPFILE
  if (flags & static_cast<std::uint32_t>(O_TMPFILE)) {
    ret |= open_flags::temp_file;
  }
#endif
#ifdef O_DSYNC
  if (flags & static_cast<std::uint32_t>(O_DSYNC)) {
    ret |= open_flags::dsync;
  }
#endif
  return ret;
}

auto create_os_open_flags(const open_flags &flags) -> std::uint32_t {
  std::uint32_t ret = 0u;
  if ((flags & open_flags::read_write) == open_flags::read_write) {
    ret |= static_cast<std::uint32_t>(O_RDWR);
  } else if ((flags & open_flags::write_only) == open_flags::write_only) {
    ret |= static_cast<std::uint32_t>(O_WRONLY);
  } else {
    ret |= static_cast<std::uint32_t>(O_RDONLY);
  }

  if ((flags & open_flags::create) == open_flags::create) {
    ret |= static_cast<std::uint32_t>(O_CREAT);
  }

  if ((flags & open_flags::excl) == open_flags::excl) {
    ret |= static_cast<std::uint32_t>(O_EXCL);
  }

  if ((flags & open_flags::no_ctty) == open_flags::no_ctty) {
    ret |= static_cast<std::uint32_t>(O_NOCTTY);
  }

  if ((flags & open_flags::truncate) == open_flags::truncate) {
    ret |= static_cast<std::uint32_t>(O_TRUNC);
  }

  if ((flags & open_flags::append) == open_flags::append) {
    ret |= static_cast<std::uint32_t>(O_APPEND);
  }

  if ((flags & open_flags::non_blocking) == open_flags::non_blocking) {
    ret |= static_cast<std::uint32_t>(O_NONBLOCK);
  }

  if ((flags & open_flags::sync) == open_flags::sync) {
    ret |= static_cast<std::uint32_t>(O_SYNC);
  }

  if ((flags & open_flags::async) == open_flags::async) {
    ret |= static_cast<std::uint32_t>(O_ASYNC);
  }

  if ((flags & open_flags::directory) == open_flags::directory) {
    ret |= static_cast<std::uint32_t>(O_DIRECTORY);
  }

  if ((flags & open_flags::no_follow) == open_flags::no_follow) {
    ret |= static_cast<std::uint32_t>(O_NOFOLLOW);
  }

  if ((flags & open_flags::clo_exec) == open_flags::clo_exec) {
    ret |= static_cast<std::uint32_t>(O_CLOEXEC);
  }
#ifdef O_DIRECT
  if ((flags & open_flags::direct) == open_flags::direct) {
    ret |= static_cast<std::uint32_t>(O_DIRECT);
  }
#endif
#ifdef O_NOATIME
  if ((flags & open_flags::no_atime) == open_flags::no_atime) {
    ret |= static_cast<std::uint32_t>(O_NOATIME);
  }
#endif
#ifdef O_PATH
  if ((flags & open_flags::path) == open_flags::path) {
    ret |= static_cast<std::uint32_t>(O_PATH);
  }
#endif
#ifdef O_TMPFILE
  if ((flags & open_flags::temp_file) == open_flags::temp_file) {
    ret |= static_cast<std::uint32_t>(O_TMPFILE);
  }
#endif
#ifdef O_DSYNC
  if ((flags & open_flags::dsync) == open_flags::dsync) {
    ret |= static_cast<std::uint32_t>(O_DSYNC);
  }
#endif
  return ret;
}
#endif
} // namespace repertory::remote
