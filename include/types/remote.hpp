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
#ifndef INCLUDE_TYPES_REMOTE_HPP_
#define INCLUDE_TYPES_REMOTE_HPP_

#define PACKET_SERVICE_FUSE std::uint32_t(1)
#define PACKET_SERVICE_WINFSP std::uint32_t(2)

#ifdef _WIN32
#define PACKET_SERVICE_FLAGS PACKET_SERVICE_WINFSP
#else
#define PACKET_SERVICE_FLAGS PACKET_SERVICE_FUSE
#endif

namespace repertory::remote {
using block_count = std::uint64_t;
using block_size = std::uint32_t;
using file_handle = std::uint64_t;
using file_mode = std::uint16_t;
using file_nlink = std::uint16_t;
using file_offset = std::uint64_t;
using file_size = std::uint64_t;
using file_time = std::uint64_t;
using group_id = std::uint32_t;
using user_id = std::uint32_t;

enum class open_flags : std::uint32_t {
  read_only = 0u,
  write_only = 1u,
  read_write = 2u,
  create = 4u,
  excl = 8u,
  no_ctty = 16u,
  truncate = 32u,
  append = 64u,
  non_blocking = 128u,
  sync = 256u,
  async = 512u,
  directory = 1024u,
  no_follow = 2048u,
  clo_exec = 4096u,
  direct = 8192u,
  no_atime = 16384u,
  path = 32768u,
  temp_file = 65536u,
  dsync = 131072u,
};

inline auto operator|(const open_flags &a, const open_flags &b) -> open_flags {
  using t = std::underlying_type_t<open_flags>;
  return static_cast<open_flags>(static_cast<t>(a) | static_cast<t>(b));
}

#ifdef __GNUG__
__attribute__((unused))
#endif
inline auto
operator|=(open_flags &a, const open_flags &b) -> open_flags & {
  a = a | b;
  return a;
}

#ifdef __GNUG__
__attribute__((unused))
#endif
inline auto
operator&(const open_flags &a, const open_flags &b) -> open_flags {
  using t = std::underlying_type_t<open_flags>;
  return static_cast<open_flags>(static_cast<t>(a) & static_cast<t>(b));
}

#pragma pack(1)
struct file_info {
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

struct setattr_x {
  std::int32_t valid;
  file_mode mode;
  user_id uid;
  group_id gid;
  file_size size;
  file_time acctime;
  file_time modtime;
  file_time crtime;
  file_time chgtime;
  file_time bkuptime;
  std::uint32_t flags;
};

struct stat {
  file_mode st_mode;
  file_nlink st_nlink;
  user_id st_uid;
  group_id st_gid;
  file_time st_atimespec;
  file_time st_mtimespec;
  file_time st_ctimespec;
  file_time st_birthtimespec;
  file_size st_size;
  block_count st_blocks;
  block_size st_blksize;
  std::uint32_t st_flags;
};

struct statfs {
  std::uint64_t f_bavail;
  std::uint64_t f_bfree;
  std::uint64_t f_blocks;
  std::uint64_t f_favail;
  std::uint64_t f_ffree;
  std::uint64_t f_files;
};

struct statfs_x : public statfs {
  char f_mntfromname[1024];
};
#pragma pack()

#ifndef _WIN32
[[nodiscard]] auto create_open_flags(std::uint32_t flags) -> open_flags;

[[nodiscard]] auto create_os_open_flags(const open_flags &flags)
    -> std::uint32_t;
#endif
} // namespace repertory::remote

#endif // INCLUDE_TYPES_REMOTE_HPP_
