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
#ifndef REPERTORY_INCLUDE_UTILS_CONFIG_HPP_
#define REPERTORY_INCLUDE_UTILS_CONFIG_HPP_

#define NOMINMAX

#if defined(_WIN32)
#define WINVER 0x0602
#define _WIN32_WINNT WINVER
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>
#if defined(PROJECT_ENABLE_WINFSP)
#include <sddl.h>
#endif // defined(PROJECT_ENABLE_WINFSP)

#include <direct.h>
#if !defined(__cplusplus)
#include <errno.h>
#endif // !defined(__cplusplus)
#include <fcntl.h>
#include <io.h>
#include <iphlpapi.h>
#include <objbase.h>
#include <psapi.h>
#include <rpc.h>
#include <share.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#if !defined(__cplusplus)
#include <stdio.h>
#endif // !defined(__cplusplus)
#include <sys/stat.h>
#include <sys/types.h>
#if !defined(__cplusplus)
#include <time.h>
#endif // !defined(__cplusplus)
#else  // !defined(_WIN32)
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/file.h>
#include <sys/socket.h>
#if defined(__LFS64__)
#include <sys/stat64.h>
#else // !defined(__LFS64__)
#include <sys/stat.h>
#endif // defined(__LFS64__)

#if defined(__linux__)
#include <sys/statfs.h>
#endif //  defined(HAS_SETXATTR)

#if defined(HAS_SETXATTR)
#include <sys/types.h>
#include <sys/xattr.h>
#endif // defined(HAS_SETXATTR)

#if defined(__APPLE__)
#include <libproc.h>
#include <sys/attr.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <sys/vnode.h>
#endif // defined(__APPLE__)

#include <unistd.h>
#endif // defined(_WIN32)

#if defined(HAS_WORDEXP_H)
#include <wordexp.h>
#endif // defined(HAS_WORDEXP_H)

#if defined(__cplusplus)
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cerrno>
#include <chrono>
#include <ciso646>
#include <climits>
#include <codecvt>
#include <condition_variable>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <ostream>
#include <queue>
#include <random>
#include <ranges>
#include <regex>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#endif // defined(__cplusplus)

#if defined(PROJECT_ENABLE_CURL)
#include "curl/curl.h"
#include "curl/multi.h"
#endif // defined(PROJECT_ENABLE_CURL)

#if defined(PROJECT_ENABLE_FUSE)
#if FUSE_USE_VERSION >= 30
#include <fuse.h>
#include <fuse_lowlevel.h>
#else
#include <fuse/fuse.h>
#endif
#endif // defined(PROJECT_ENABLE_FUSE)

#if defined(PROJECT_ENABLE_FZF)
#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)
#include "fzf.h"
#if defined(__cplusplus)
}
#endif // defined(__cplusplus)
#endif // defined(PROJECT_ENABLE_FZF)

#if defined(PROJECT_ENABLE_OPENSSL)
#include "openssl/ssl.h"
#endif // defined(PROJECT_ENABLE_OPENSSL)

#if defined(PROJECT_ENABLE_LIBDSM)
#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)
#include "bdsm/bdsm.h"
#if defined(__cplusplus)
}

struct netbios_ns_deleter final {
  void operator()(netbios_ns *ns) const {
    if (ns != nullptr) {
      netbios_ns_destroy(ns);
    }
  }
};
using netbios_ns_t = std::unique_ptr<netbios_ns, netbios_ns_deleter>;

inline const auto smb_session_deleter = [](smb_session *session) {
  if (session != nullptr) {
    smb_session_destroy(session);
  }
};
using smb_session_t = std::shared_ptr<smb_session>;

struct smb_stat_deleter final {
  void operator()(smb_stat st) const {
    if (st != nullptr) {
      smb_stat_destroy(st);
    }
  }
};
using smb_stat_t = std::unique_ptr<smb_file, smb_stat_deleter>;

struct smb_stat_list_deleter final {
  void operator()(smb_file *list) const {
    if (list != nullptr) {
      smb_stat_list_destroy(list);
    }
  }
};
using smb_stat_list_t = std::unique_ptr<smb_file, smb_stat_list_deleter>;
#endif // defined(__cplusplus)
#endif // defined(PROJECT_ENABLE_LIBDSM)

#if defined(PROJECT_ENABLE_LIBEVENT)
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/listener.h"
#include "event2/thread.h"
#include "event2/util.h"
#endif // defined(PROJECT_ENABLE_LIBEVENT)

#if defined(PROJECT_ENABLE_LIBSODIUM)
#include "sodium.h"
#endif // defined(PROJECT_ENABLE_LIBSODIUM)

#if defined(PROJECT_ENABLE_SDL)
#include "SDL.h"
#include "SDL_gamecontroller.h"
#include "SDL_joystick.h"
#endif // defined(PROJECT_ENABLE_SDL)

#if defined(PROJECT_ENABLE_SQLITE)
#include "sqlite3.h"
#endif // defined(PROJECT_ENABLE_SQLITE)

#if defined(PROJECT_ENABLE_VLC)
#include <vlc/vlc.h>

#if defined(__cplusplus)
[[nodiscard]] inline auto get_libvlc_error_msg() -> std::string {
  const auto *msg = libvlc_errmsg();
  return msg == nullptr ? "none" : msg;
}

struct vlc_deleter final {
  void operator()(libvlc_instance_t *inst) const {
    if (inst != nullptr) {
      libvlc_release(inst);
    }
  }
};
using vlc_t = std::unique_ptr<libvlc_instance_t, vlc_deleter>;

struct vlc_media_deleter final {
  void operator()(libvlc_media_t *media) const {
    if (media != nullptr) {
      libvlc_media_release(media);
    }
  }
};
using vlc_media_t = std::unique_ptr<libvlc_media_t, vlc_media_deleter>;

struct vlc_media_list_deleter final {
  void operator()(libvlc_media_list_t *media_list) const {
    if (media_list != nullptr) {
      libvlc_media_list_release(media_list);
    }
  }
};
using vlc_media_list_t =
    std::unique_ptr<libvlc_media_list_t, vlc_media_list_deleter>;

struct vlc_string_deleter final {
  void operator()(char *str) const {
    if (str != nullptr) {
      libvlc_free(str);
    }
  }
};
using vlc_string_t = std::unique_ptr<char, vlc_string_deleter>;
#endif // defined(__cplusplus)
#endif // defined(PROJECT_ENABLE_VLC)

#if !defined(fstat64)
#define fstat64 fstat
#endif // !defined(fstat64)

#if !defined(pread64)
#define pread64 pread
#endif // !defined(pread64)

#if !defined(pwrite64)
#define pwrite64 pwrite
#endif // !defined(pwrite64)

#if !defined(stat64)
#define stat64 stat
#endif // !defined(stat64)

#if !defined(statfs64)
#define statfs64 statfs
#endif // !defined(statfs64)

#if !defined(off64_t)
#define off64_t std::size_t
#endif // !defined(off64_t)

#if !defined(__off64_t)
#define __off64_t off64_t
#endif // !defined(__off64_t)

#if defined(__cplusplus)
#if defined(PROJECT_ENABLE_BOOST)
#include "boost/archive/text_iarchive.hpp"
#include "boost/archive/text_oarchive.hpp"
#include "boost/asio.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/bind/bind.hpp"
#include "boost/dynamic_bitset.hpp"
#include "boost/dynamic_bitset/serialization.hpp"
#include "boost/endian/conversion.hpp"
#include "boost/integer.hpp"
#include "boost/interprocess/sync/named_mutex.hpp"
#include "boost/interprocess/sync/scoped_lock.hpp"
#include "boost/multiprecision/cpp_dec_float.hpp"
#include "boost/multiprecision/cpp_int.hpp"
#include "boost/serialization/vector.hpp"
#endif // defined(PROJECT_ENABLE_BOOST)

#if defined(PROJECT_ENABLE_CLI11)
#if defined(PROJECT_IS_MINGW) && !defined(PROJECT_IS_MINGW_UNIX)
#include "CLI/CLI.hpp"
#else // !defined(PROJECT_IS_MINGW) || defined(PROJECT_IS_MINGW_UNIX)
#include "CLI11.hpp"
#endif // defined(PROJECT_IS_MINGW) && !defined(PROJECT_IS_MINGW_UNIX)
#endif // defined(PROJECT_ENABLE_CLI11)

#if defined(PROJECT_ENABLE_CPP_HTTPLIB)
#include "httplib.h"
#endif // defined(PROJECT_ENABLE_JSON)

#if defined(PROJECT_ENABLE_DTL)
#include "dtl/dtl.hpp"
#endif // defined(PROJECT_ENABLE_DTL)

#if defined(PROJECT_ENABLE_JSON)
#include "json.hpp"
#endif // defined(PROJECT_ENABLE_JSON)

#if defined(PROJECT_ENABLE_NANA)
#include "nana/gui.hpp"
#include "nana/gui/timer.hpp"
#include "nana/gui/widgets/button.hpp"
#include "nana/gui/widgets/combox.hpp"
#include "nana/gui/widgets/group.hpp"
#include "nana/gui/widgets/label.hpp"
#include "nana/gui/widgets/panel.hpp"
#include "nana/gui/widgets/picture.hpp"
#include "nana/gui/widgets/tabbar.hpp"
#endif // defined(PROJECT_ENABLE_NANA)

#if defined(PROJECT_ENABLE_PUGIXML)
#include "pugixml.hpp"
#endif // defined(PROJECT_ENABLE_PUGIXML)

#if defined(PROJECT_ENABLE_ROCKSDB)
#include "rocksdb/db.h"
#include "rocksdb/utilities/transaction_db.h"
#endif // defined(PROJECT_ENABLE_ROCKSDB)

#if defined(PROJECT_ENABLE_SFML)
#include "RoundedRectangleShape.hpp"
#include "SFML/Graphics.hpp"
#include "Text2.hpp"
#endif // defined(PROJECT_ENABLE_SFML)

#if defined(PROJECT_ENABLE_SAGO_PLATFORM_FOLDERS)
#include "platform_folders.hpp"
#endif // defined(PROJECT_ENABLE_SAGO_PLATFORM_FOLDERS)

#if defined(PROJECT_ENABLE_SPDLOG)
#include "spdlog/async.h"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/fmt/bundled/format.h"
#include "spdlog/fmt/bundled/ranges.h"
#include "spdlog/fmt/chrono.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#endif // defined(PROJECT_ENABLE_SPDLOG)

#if defined(PROJECT_ENABLE_STDUUID)
#include "uuid.h"
#endif // defined(PROJECT_ENABLE_STDUUID)

#if defined(PROJECT_ENABLE_TPL)
#include "process.hpp"
#endif // defined(PROJECT_ENABLE_TPL)

#if defined(PROJECT_ENABLE_WINFSP)
#include "winfsp/winfsp.hpp"
#endif // defined(PROJECT_ENABLE_WINFSP)

namespace repertory {
using data_buffer = std::vector<unsigned char>;
using mutex_lock = std::lock_guard<std::mutex>;
using recur_mutex_lock = std::lock_guard<std::recursive_mutex>;
using stop_type = std::atomic_bool;
using stop_type_callback = std::function<std::atomic_bool()>;
using unique_mutex_lock = std::unique_lock<std::mutex>;
using unique_recur_mutex_lock = std::unique_lock<std::recursive_mutex>;

#if defined(_WIN32)
#if defined(PROJECT_ENABLE_WIN32_LONG_PATH_NAMES)
inline constexpr const auto max_path_length = std::size_t{32767U};
#else  // !defined(PROJECT_ENABLE_WIN32_LONG_PATH_NAMES)
inline constexpr const auto max_path_length = std::size_t{MAX_PATH};
#endif // defined(PROJECT_ENABLE_WIN32_LONG_PATH_NAMES)

using native_handle = HANDLE;
#else // !defined(_WIN32)
inline constexpr const auto max_path_length = std::size_t{PATH_MAX};
using native_handle = int;
#if !defined(INVALID_HANDLE_VALUE)
#define INVALID_HANDLE_VALUE (-1)
#endif // !defined(INVALID_HANDLE_VALUE)
#endif // defined(_WIN32)

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <typename T> struct is_collection {
  static const bool value = false;
};

template <typename T, typename A>
struct is_collection<std::vector<T, A>> : std::true_type {};

template <typename T> struct is_collection<std::deque<T>> : std::true_type {};

template <> struct is_collection<std::string> : std::true_type {};

template <> struct is_collection<std::wstring> : std::true_type {};

struct file_deleter final {
  void operator()(FILE *file) {
    if (file != nullptr) {
      fclose(file);
    }
  }
};
using file_t = std::unique_ptr<FILE, file_deleter>;

struct http_range final {
  std::uint64_t begin{};
  std::uint64_t end{};
};

using http_headers = std::map<std::string, std::string>;
using http_query_parameters = std::map<std::string, std::string>;
using http_ranges = std::vector<http_range>;
} // namespace repertory
#endif // defined(__cplusplus)

#define REPERTORY_USES_FUNCTION_NAME()                                         \
  static constexpr const std::string_view function_name {                      \
    static_cast<const char *>(__FUNCTION__),                                   \
  }

#endif // REPERTORY_INCLUDE_UTILS_CONFIG_HPP_
