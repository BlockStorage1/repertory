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
#ifndef REPERTORY_INCLUDE_COMMON_HPP_
#define REPERTORY_INCLUDE_COMMON_HPP_

#if defined(__GNUC__)
// clang-format off
#define REPERTORY_IGNORE_WARNINGS_ENABLE()                                     \
  _Pragma("GCC diagnostic push")                                               \
  _Pragma("GCC diagnostic ignored \"-Waggressive-loop-optimizations\"")        \
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

#if defined(__cplusplus)
REPERTORY_IGNORE_WARNINGS_ENABLE()
#include "utils/config.hpp"
REPERTORY_IGNORE_WARNINGS_DISABLE()

using namespace std::chrono_literals;
using json = nlohmann::json;

inline constexpr const std::string_view REPERTORY = "repertory";
inline constexpr const std::wstring_view REPERTORY_W = L"repertory";

inline constexpr const std::uint64_t REPERTORY_CONFIG_VERSION = 0ULL;
inline constexpr const std::string_view REPERTORY_DATA_NAME = "repertory2";
inline constexpr const std::string_view REPERTORY_MIN_REMOTE_VERSION = "2.0.0";

#define REPERTORY_INVALID_HANDLE INVALID_HANDLE_VALUE

#define WINFSP_ALLOCATION_UNIT UINT64(4096U)

#if defined(_WIN32)
#if defined(CreateDirectory)
#undef CreateDirectory
#endif

#if defined(CreateFile)
#undef CreateFile
#endif

#if defined(DeleteFile)
#undef DeleteFile
#endif

#if defined(RemoveDirectory)
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

#if !defined(_WIN32)
#if defined(__APPLE__)
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

#if defined(_WIN32)
#define UTIME_NOW ((1l << 30) - 1l)
#define UTIME_OMIT ((1l << 30) - 2l)
#define CONVERT_STATUS_NOT_IMPLEMENTED(e) e
#else
using BOOLEAN = std::uint8_t;
using DWORD = std::uint32_t;
using HANDLE = void *;
using NTSTATUS = std::uint32_t;
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

#define INVALID_FILE_ATTRIBUTES static_cast<DWORD>(-1)

#define MAX_PATH 260

#define STATUS_SUCCESS                                                         \
  std::uint32_t { 0U }
#define STATUS_ACCESS_DENIED                                                   \
  std::uint32_t { 0xC0000022L }
#define STATUS_DEVICE_BUSY                                                     \
  std::uint32_t { 0x80000011L }
#define STATUS_DEVICE_INSUFFICIENT_RESOURCES                                   \
  std::uint32_t { 0xC0000468L }
#define STATUS_DIRECTORY_NOT_EMPTY                                             \
  std::uint32_t { 0xC0000101L }
#define STATUS_FILE_IS_A_DIRECTORY                                             \
  std::uint32_t { 0xC00000BAL }
#define STATUS_FILE_TOO_LARGE                                                  \
  std::uint32_t { 0xC0000904L }
#define STATUS_INSUFFICIENT_RESOURCES                                          \
  std::uint32_t { 0xC000009AL }
#define STATUS_INTERNAL_ERROR                                                  \
  std::uint32_t { 0xC00000E5L }
#define STATUS_INVALID_ADDRESS                                                 \
  std::uint32_t { 0xC0000141L }
#define STATUS_INVALID_HANDLE                                                  \
  std::uint32_t { 0xC0000006L }
#define STATUS_INVALID_IMAGE_FORMAT                                            \
  std::uint32_t { 0xC000007BL }
#define STATUS_INVALID_PARAMETER                                               \
  std::uint32_t { 0xC000000DL }
#define STATUS_NO_MEMORY                                                       \
  std::uint32_t { 0xC0000017L }
#define STATUS_NOT_IMPLEMENTED                                                 \
  std::uint32_t { 0xC0000002L }
#define STATUS_OBJECT_NAME_EXISTS                                              \
  std::uint32_t { 0x40000000L }
#define STATUS_OBJECT_NAME_NOT_FOUND                                           \
  std::uint32_t { 0xC0000034L }
#define STATUS_OBJECT_PATH_INVALID                                             \
  std::uint32_t { 0xC0000039L }
#define STATUS_UNEXPECTED_IO_ERROR                                             \
  std::uint32_t { 0xC00000E9L }

#define CONVERT_STATUS_NOT_IMPLEMENTED(e)                                      \
  ((std::uint32_t(e) == STATUS_NOT_IMPLEMENTED) ? -ENOTSUP : e)

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

#define INTERFACE_SETUP(name)                                                  \
public:                                                                        \
  name(const name &) noexcept = delete;                                        \
  auto operator=(const name &) noexcept -> name & = delete;                    \
  auto operator=(name &&) noexcept -> name & = delete;                         \
                                                                               \
protected:                                                                     \
  name() = default;                                                            \
  name(name &&) noexcept = default;                                            \
                                                                               \
public:                                                                        \
  virtual ~name() = default

#endif // __cplusplus
#endif // REPERTORY_INCLUDE_COMMON_HPP_
