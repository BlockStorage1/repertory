/*
  Copyright <2018-2022> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute,
  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
  OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef INCLUDE_COMMON_HPP_
#define INCLUDE_COMMON_HPP_

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <ciso646>
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#else
#define FUSE_USE_VERSION 29
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
#if __linux__
#include <utils/uuid++.hh>
#else
#include <uuid/uuid.h>
#endif
#endif
#include <algorithm>
#include <atomic>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/dynamic_bitset/serialization.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/filesystem.hpp>
#include <boost/serialization/vector.hpp>
#include <chacha.h>
#include <chachapoly.h>
#include <chrono>
#include <codecvt>
#include <condition_variable>
#include <curl/curl.h>
#include <curl/multi.h>
#include <deque>
#include <fstream>
#include <future>
#include <iostream>
#include <jsonrp.hpp>
#include <limits>
#include <microhttpd.h>
#include <httpserver.hpp>
#include <mutex>

#if !IS_DEBIAN9_DISTRO && HAS_STD_OPTIONAL
#include <optional>
#else
#include <utils/optional.h>
#endif
#include <files.h>
#include <hex.h>
#include <json.hpp>
#include <osrng.h>
#include <random>
#include <regex>
#include <rocksdb/db.h>
#include <sha.h>
#include <sstream>
#include <string>
#include <thread>
#include <ttmath.h>
#include <type_traits>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <sddl.h>
#include <winfsp/winfsp.hpp>
#else
#include <fuse/fuse.h>
#endif

#if defined(REPERTORY_ENABLE_S3)
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/utils/logging/AWSLogging.h>
#include <aws/core/utils/logging/DefaultLogSystem.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CopyObjectRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/PutObjectRequest.h>
#endif

using namespace std::chrono_literals;
using json = nlohmann::json;

const std::string &get_repertory_git_revision();
const std::string &get_repertory_version();

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
#endif

#define MIN_REMOTE_VERSION "2.0.0"
#define MIN_SIA_VERSION "1.4.1"
#define MIN_SP_VERSION "1.4.1.2"
#define REPERTORY_CONFIG_VERSION 0ull
#define REPERTORY "repertory"
#define REPERTORY_DATA_NAME "repertory2"
#define REPERTORY_W L"repertory"

#define NANOS_PER_SECOND 1000000000L

#ifdef _WIN32
#define REPERTORY_INVALID_HANDLE INVALID_HANDLE_VALUE
#define REPERTORY_API_INVALID_HANDLE static_cast<std::uint64_t>(-1)
#define OSHandle HANDLE
#else
#define REPERTORY_INVALID_HANDLE -1
#define REPERTORY_API_INVALID_HANDLE REPERTORY_INVALID_HANDLE
#define OSHandle int
#endif

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

#if __APPLE__
#define pread64 pread
#define pwrite64 pwrite
#endif

#define WINFSP_ALLOCATION_UNIT UINT64(4096U)

#ifdef _WIN32
#define UTIME_NOW ((1l << 30) - 1l)
#define UTIME_OMIT ((1l << 30) - 2l)
#define CONVERT_STATUS_NOT_IMPLEMENTED(e) e
#else
#define VOID void
#define PVOID VOID *
typedef PVOID HANDLE;
#define WCHAR wchar_t
#define PWSTR WCHAR *
#define BOOLEAN std::uint8_t
#define UINT16 std::uint16_t
#define UINT32 std::uint32_t
#define PUINT32 UINT32 *
#define UINT64 std::uint64_t
#define SIZE_T std::uint64_t
#define DWORD std::uint32_t

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
#define CONVERT_STATUS_NOT_IMPLEMENTED(e)                                                          \
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
  UINT32 FileAttributes;
  UINT32 ReparseTag;
  UINT64 AllocationSize;
  UINT64 FileSize;
  UINT64 CreationTime;
  UINT64 LastAccessTime;
  UINT64 LastWriteTime;
  UINT64 ChangeTime;
  UINT64 IndexNumber;
  UINT32 HardLinks;
  UINT32 EaSize;
};

typedef FSP_FSCTL_FILE_INFO FileInfo;
} // namespace Fsp::FileSystemBase
#endif

using namespace Fsp;

#define INTERFACE_SETUP(name)                                                                      \
public:                                                                                            \
  name(const name &) noexcept = delete;                                                            \
  name(name &&) noexcept = delete;                                                                 \
  name &operator=(const name &) noexcept = delete;                                                 \
  name &operator=(name &&) noexcept = delete;                                                      \
                                                                                                   \
protected:                                                                                         \
  name() = default;                                                                                \
                                                                                                   \
public:                                                                                            \
  virtual ~name() = default

#endif // INCLUDE_COMMON_HPP_
