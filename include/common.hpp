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
#ifndef INCLUDE_COMMON_HPP_
#define INCLUDE_COMMON_HPP_

#if defined(__GNUC__)
// clang-format off
#define REPERTORY_IGNORE_WARNINGS_ENABLE()                                            \
  _Pragma("GCC diagnostic push")                                               \
  _Pragma("GCC diagnostic ignored \"-Wconversion\"")                           \
  _Pragma("GCC diagnostic ignored \"-Wdouble-promotion\"")                     \
  _Pragma("GCC diagnostic ignored \"-Wduplicated-branches\"")                  \
  _Pragma("GCC diagnostic ignored \"-Wfloat-conversion\"")                     \
  _Pragma("GCC diagnostic ignored \"-Wimplicit-fallthrough\"")                 \
  _Pragma("GCC diagnostic ignored \"-Wnull-dereference\"")                     \
  _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"")                      \
  _Pragma("GCC diagnostic ignored \"-Wunused-but-set-variable\"")              \
  _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")                     \
  _Pragma("GCC diagnostic ignored \"-Wuseless-cast\"")

#define REPERTORY_IGNORE_WARNINGS_DISABLE()                                    \
  _Pragma("GCC diagnostic pop")
#else
#define REPERTORY_IGNORE_WARNINGS_ENABLE() 
#define REPERTORY_IGNORE_WARNINGS_DISABLE()                                    
#endif // defined(__GNUC__)
// clang-format on

#ifdef __cplusplus
REPERTORY_IGNORE_WARNINGS_ENABLE()

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <ciso646>
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <time.h>
#else
#include <climits>
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <sys/file.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/statfs.h>
#endif
#include <unistd.h>
#ifdef HAS_SETXATTR
#include <sys/types.h>
#include <sys/xattr.h>
#endif
#ifdef __APPLE__
#include <libproc.h>
#include <sys/attr.h>
#include <sys/vnode.h>
#endif
#if __APPLE__
#include <sys/mount.h>
#include <sys/statvfs.h>
#endif
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <codecvt>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <sodium.h>
template <typename data_type>
[[nodiscard]] inline auto repertory_rand() -> data_type {
  data_type ret{};
  randombytes_buf(&ret, sizeof(ret));
  return ret;
}

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/dynamic_bitset/serialization.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/serialization/vector.hpp>
#include <curl/curl.h>
#include <curl/multi.h>
#include <json.hpp>
#include <sqlite3.h>
#include <stduuid.h>

#ifdef _WIN32
#include <sddl.h>
#include <winfsp/winfsp.hpp>
#else
#if FUSE_USE_VERSION >= 30
#include <fuse.h>
#include <fuse_lowlevel.h>
#else
#include <fuse/fuse.h>
#endif
#endif

#include <pugixml.hpp>

#define CPPHTTPLIB_TCP_NODELAY true
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

REPERTORY_IGNORE_WARNINGS_DISABLE()

struct sqlite3_deleter {
  void operator()(sqlite3 *db3) {
    if (db3 != nullptr) {
      sqlite3_close_v2(db3);
    }
  }
};

using db3_t = std::unique_ptr<sqlite3, sqlite3_deleter>;

struct sqlite3_statement_deleter {
  void operator()(sqlite3_stmt *stmt) {
    if (stmt != nullptr) {
      sqlite3_finalize(stmt);
    }
  }
};

using db3_stmt_t = std::unique_ptr<sqlite3_stmt, sqlite3_statement_deleter>;

using namespace std::chrono_literals;
using json = nlohmann::json;

#define REPERTORY "repertory"
#define REPERTORY_CONFIG_VERSION 0ull
#define REPERTORY_DATA_NAME "repertory2"
#define REPERTORY_MIN_REMOTE_VERSION "2.0.0"
#define REPERTORY_W L"repertory"

#ifdef _WIN32
#define REPERTORY_INVALID_HANDLE INVALID_HANDLE_VALUE
#define REPERTORY_API_INVALID_HANDLE static_cast<std::uint64_t>(-1)
using native_handle = HANDLE;
#else
#define REPERTORY_INVALID_HANDLE (-1)
#define REPERTORY_API_INVALID_HANDLE REPERTORY_INVALID_HANDLE
using native_handle = int;
#endif

constexpr const auto NANOS_PER_SECOND = 1000000000L;

#ifdef _WIN32
#ifdef CreateDirectory
#undef CreateDirectory
#endif

#ifdef CreateFile
#undef CreateFile
#endif

#ifdef DeleteFile
#undef DeleteFile
#endif

#ifdef RemoveDirectory
#undef RemoveDirectory
#endif

#ifndef _SH_DENYRW
#define _SH_DENYRW 0x10 // deny read/write mode
#endif

#ifndef _SH_DENYWR
#define _SH_DENYWR 0x20 // deny write mode
#endif

#ifndef _SH_DENYRD
#define _SH_DENYRD 0x30 // deny read mode
#endif

#ifndef _SH_DENYNO
#define _SH_DENYNO 0x40 // deny none mode
#endif

#ifndef _SH_SECURE
#define _SH_SECURE 0x80 // secure mode
#endif
#endif

#ifndef ENETDOWN
#define ENETDOWN 100
#endif

#ifndef SETATTR_WANTS_MODE
#define SETATTR_WANTS_MODE(attr) ((attr)->valid & (1 << 0))
#endif // SETATTR_WANTS_MODE

#ifndef SETATTR_WANTS_UID
#define SETATTR_WANTS_UID(attr) ((attr)->valid & (1 << 1))
#endif // SETATTR_WANTS_UID

#ifndef SETATTR_WANTS_GID
#define SETATTR_WANTS_GID(attr) ((attr)->valid & (1 << 2))
#endif // SETATTR_WANTS_GID

#ifndef SETATTR_WANTS_SIZE
#define SETATTR_WANTS_SIZE(attr) ((attr)->valid & (1 << 3))
#endif // SETATTR_WANTS_SIZE

#ifndef SETATTR_WANTS_ACCTIME
#define SETATTR_WANTS_ACCTIME(attr) ((attr)->valid & (1 << 4))
#endif // SETATTR_WANTS_ACCTIME

#ifndef SETATTR_WANTS_MODTIME
#define SETATTR_WANTS_MODTIME(attr) ((attr)->valid & (1 << 5))
#endif // SETATTR_WANTS_MODTIME

#ifndef SETATTR_WANTS_CRTIME
#define SETATTR_WANTS_CRTIME(attr) ((attr)->valid & (1 << 28))
#endif // SETATTR_WANTS_CRTIME

#ifndef SETATTR_WANTS_CHGTIME
#define SETATTR_WANTS_CHGTIME(attr) ((attr)->valid & (1 << 29))
#endif // SETATTR_WANTS_CHGTIME

#ifndef SETATTR_WANTS_BKUPTIME
#define SETATTR_WANTS_BKUPTIME(attr) ((attr)->valid & (1 << 30))
#endif // SETATTR_WANTS_BKUPTIME

#ifndef SETATTR_WANTS_FLAGS
#define SETATTR_WANTS_FLAGS(attr) ((attr)->valid & (1 << 31))
#endif // SETATTR_WANTS_FLAGS

#ifndef _WIN32
#ifdef __APPLE__
#define G_PREFIX "org"
#define G_KAUTH_FILESEC_XATTR G_PREFIX ".apple.system.Security"
#define A_PREFIX "com"
#define A_KAUTH_FILESEC_XATTR A_PREFIX ".apple.system.Security"
#define XATTR_APPLE_PREFIX "com.apple."
#endif

#ifndef XATTR_NAME_MAX
#define XATTR_NAME_MAX 255 /* # chars in an extended attribute name */
#endif

#ifndef XATTR_SIZE_MAX
#define XATTR_SIZE_MAX 65536
#endif
#endif

#ifndef fstat64
#define fstat64 fstat
#endif

#ifndef pread64
#define pread64 pread
#endif

#ifndef pwrite64
#define pwrite64 pwrite
#endif

#ifndef stat64
#define stat64 stat
#endif

#ifndef statfs64
#define statfs64 statfs
#endif

#define WINFSP_ALLOCATION_UNIT UINT64(4096U)

#ifdef _WIN32
#define UTIME_NOW ((1l << 30) - 1l)
#define UTIME_OMIT ((1l << 30) - 2l)
#define CONVERT_STATUS_NOT_IMPLEMENTED(e) e
#else
using BOOLEAN = std::uint8_t;
using DWORD = std::uint32_t;
using HANDLE = void *;
using PUINT32 = std::uint32_t *;
using PVOID = void *;
using PWSTR = wchar_t *;
using SIZE_T = std::uint64_t;
using UINT16 = std::uint16_t;
using UINT32 = std::uint32_t;
using UINT64 = std::uint64_t;
using VOID = void;
using WCHAR = wchar_t;

#define FILE_ATTRIBUTE_READONLY 0x00000001
#define FILE_ATTRIBUTE_HIDDEN 0x00000002
#define FILE_ATTRIBUTE_SYSTEM 0x00000004
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_ARCHIVE 0x00000020
#define FILE_ATTRIBUTE_DEVICE 0x00000040
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define FILE_ATTRIBUTE_TEMPORARY 0x00000100
#define FILE_ATTRIBUTE_SPARSE_FILE 0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT 0x00000400
#define FILE_ATTRIBUTE_COMPRESSED 0x00000800
#define FILE_ATTRIBUTE_OFFLINE 0x00001000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED 0x00002000
#define FILE_ATTRIBUTE_ENCRYPTED 0x00004000
#define FILE_ATTRIBUTE_INTEGRITY_STREAM 0x00008000
#define FILE_ATTRIBUTE_VIRTUAL 0x00010000
#define FILE_ATTRIBUTE_NO_SCRUB_DATA 0x00020000
#define FILE_ATTRIBUTE_EA 0x00040000
#define FILE_ATTRIBUTE_PINNED 0x00080000
#define FILE_ATTRIBUTE_UNPINNED 0x00100000
#define FILE_ATTRIBUTE_RECALL_ON_OPEN 0x00040000
#define FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS 0x00400000

#define FILE_DIRECTORY_FILE 0x00000001

#define FILE_EXECUTE (0x0020)
#define FILE_GENERIC_EXECUTE (0x00020000L | 0x0080 | 0x0020 | 0x00100000L)

#define GENERIC_READ (0x80000000L)
#define GENERIC_WRITE (0x40000000L)
#define GENERIC_EXECUTE (0x20000000L)
#define GENERIC_ALL (0x10000000L)

#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)

#define MAX_PATH 260

#define STATUS_SUCCESS std::int32_t(0)
#define STATUS_ACCESS_DENIED std::int32_t(0xC0000022L)
#define STATUS_DEVICE_BUSY std::int32_t(0x80000011L)
#define STATUS_DEVICE_INSUFFICIENT_RESOURCES std::int32_t(0xC0000468L)
#define STATUS_DIRECTORY_NOT_EMPTY std::int32_t(0xC0000101L)
#define STATUS_FILE_IS_A_DIRECTORY std::int32_t(0xC00000BAL)
#define STATUS_FILE_TOO_LARGE std::int32_t(0xC0000904L)
#define STATUS_INSUFFICIENT_RESOURCES std::int32_t(0xC000009AL)
#define STATUS_INTERNAL_ERROR std::int32_t(0xC00000E5L)
#define STATUS_INVALID_ADDRESS std::int32_t(0xC0000141L)
#define STATUS_INVALID_HANDLE std::int32_t(0xC0000006L)
#define STATUS_INVALID_IMAGE_FORMAT std::int32_t(0xC000007BL)
#define STATUS_INVALID_PARAMETER std::int32_t(0xC000000DL)
#define STATUS_NO_MEMORY std::int32_t(0xC0000017L)
#define STATUS_NOT_IMPLEMENTED std::int32_t(0xC0000002L)
#define STATUS_OBJECT_NAME_EXISTS std::int32_t(0x40000000L)
#define STATUS_OBJECT_NAME_NOT_FOUND std::int32_t(0xC0000034L)
#define STATUS_OBJECT_PATH_INVALID std::int32_t(0xC0000039L)
#define STATUS_UNEXPECTED_IO_ERROR std::int32_t(0xC00000E9L)

#define CONVERT_STATUS_NOT_IMPLEMENTED(e)                                      \
  ((std::int32_t(e) == STATUS_NOT_IMPLEMENTED) ? -ENOTSUP : e)

namespace Fsp::FileSystemBase {
enum {
  FspCleanupDelete = 0x01,
  FspCleanupSetAllocationSize = 0x02,
  FspCleanupSetArchiveBit = 0x10,
  FspCleanupSetLastAccessTime = 0x20,
  FspCleanupSetLastWriteTime = 0x40,
  FspCleanupSetChangeTime = 0x80
};

struct FSP_FSCTL_FILE_INFO {
  UINT32 FileAttributes{};
  UINT32 ReparseTag{};
  UINT64 AllocationSize{};
  UINT64 FileSize{};
  UINT64 CreationTime{};
  UINT64 LastAccessTime{};
  UINT64 LastWriteTime{};
  UINT64 ChangeTime{};
  UINT64 IndexNumber{};
  UINT32 HardLinks{};
  UINT32 EaSize{};
};

using FileInfo = FSP_FSCTL_FILE_INFO;
} // namespace Fsp::FileSystemBase
#endif

using namespace Fsp;

namespace repertory {
auto get_repertory_git_revision() -> const std::string &;
auto get_repertory_version() -> const std::string &;
void repertory_init();
void repertory_shutdown();
} // namespace repertory

namespace {
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
} // namespace

#define INTERFACE_SETUP(name)                                                  \
public:                                                                        \
  name(const name &) noexcept = delete;                                        \
  name &operator=(const name &) noexcept = delete;                             \
  name &operator=(name &&) noexcept = delete;                                  \
                                                                               \
protected:                                                                     \
  name() = default;                                                            \
  name(name &&) noexcept = default;                                            \
                                                                               \
public:                                                                        \
  virtual ~name() = default

#endif // __cplusplus
#endif // INCLUDE_COMMON_HPP_
