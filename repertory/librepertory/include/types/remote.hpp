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
#ifndef REPERTORY_INCLUDE_TYPES_REMOTE_HPP_
#define REPERTORY_INCLUDE_TYPES_REMOTE_HPP_

#include "types/repertory.hpp"

inline constexpr auto PACKET_SERVICE_FUSE{1U};
inline constexpr auto PACKET_SERVICE_WINFSP{2U};

#if defined(_WIN32)
inline constexpr auto PACKET_SERVICE_FLAGS{PACKET_SERVICE_WINFSP};
#else  // !defined(_WIN32)
inline constexpr auto PACKET_SERVICE_FLAGS{PACKET_SERVICE_FUSE};
#endif // defined(_WIN32)

inline constexpr auto default_remote_client_pool_size{20U};
inline constexpr auto default_remote_conn_timeout_ms{3000U};
inline constexpr auto default_remote_directory_page_size{std::size_t(100U)};
inline constexpr auto default_remote_max_connections{20U};
inline constexpr auto default_remote_recv_timeout_ms{6000U};
inline constexpr auto default_remote_send_timeout_ms{6000U};

namespace repertory::remote {
struct remote_config final {
  std::uint16_t api_port{};
  std::uint32_t conn_timeout_ms{default_remote_conn_timeout_ms};
  std::string encryption_token;
  std::string host_name_or_ip;
  std::uint8_t max_connections{default_remote_max_connections};
  std::uint32_t recv_timeout_ms{default_remote_recv_timeout_ms};
  std::uint32_t send_timeout_ms{default_remote_send_timeout_ms};

  auto operator==(const remote_config &cfg) const noexcept -> bool {
    if (&cfg != this) {
      return api_port == cfg.api_port &&
             conn_timeout_ms == cfg.conn_timeout_ms &&
             encryption_token == cfg.encryption_token &&
             host_name_or_ip == cfg.host_name_or_ip &&
             max_connections == cfg.max_connections &&
             recv_timeout_ms == cfg.recv_timeout_ms &&
             send_timeout_ms == cfg.send_timeout_ms;
    }

    return true;
  }

  auto operator!=(const remote_config &cfg) const noexcept -> bool {
    if (&cfg != this) {
      return not(cfg == *this);
    }

    return false;
  }
};

struct remote_mount final {
  std::uint16_t api_port{};
  std::uint8_t client_pool_size{default_remote_client_pool_size};
  bool enable{false};
  std::string encryption_token;

  auto operator==(const remote_mount &cfg) const noexcept -> bool {
    if (&cfg == this) {
      return true;
    }

    return api_port == cfg.api_port &&
           client_pool_size == cfg.client_pool_size && enable == cfg.enable &&
           encryption_token == cfg.encryption_token;
  }

  auto operator!=(const remote_mount &cfg) const noexcept -> bool {
    if (&cfg == this) {
      return false;
    }

    return not(cfg == *this);
  }
};

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
  read_only = 0U,
  write_only = 1U,
  read_write = 2U,
  create = 4U,
  excl = 8U,
  no_ctty = 16U,
  truncate = 32U,
  append = 64U,
  non_blocking = 128U,
  sync = 256U,
  async = 512U,
  directory = 1024U,
  no_follow = 2048U,
  clo_exec = 4096U,
  direct = 8192U,
  no_atime = 16384U,
  path = 32768U,
  temp_file = 65536U,
  dsync = 131072U,
};

#if defined(__GNUG__)
__attribute__((unused))
#endif // defined(__GNUG__)
inline auto
operator|(const open_flags &flag_1, const open_flags &flag_2) -> open_flags {
  using flag_t = std::underlying_type_t<open_flags>;
  return static_cast<open_flags>(static_cast<flag_t>(flag_1) |
                                 static_cast<flag_t>(flag_2));
}

#if defined(__GNUG__)
__attribute__((unused))
#endif // defined(__GNUG__)
inline auto
operator|=(open_flags &flag_1, const open_flags &flag_2) -> open_flags & {
  flag_1 = flag_1 | flag_2;
  return flag_1;
}

#if defined(__GNUG__)
__attribute__((unused))
#endif // defined(__GNUG__)
inline auto
operator&(const open_flags &flag_1, const open_flags &flag_2) -> open_flags {
  using flag_t = std::underlying_type_t<open_flags>;
  return static_cast<open_flags>(static_cast<flag_t>(flag_1) &
                                 static_cast<flag_t>(flag_2));
}

#pragma pack(1)
struct file_info final {
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

struct setattr_x final {
  std::int32_t valid{};
  file_mode mode{};
  user_id uid{};
  group_id gid{};
  file_size size{};
  file_time acctime{};
  file_time modtime{};
  file_time crtime{};
  file_time chgtime{};
  file_time bkuptime{};
  std::uint32_t flags{};
};

struct stat final {
  file_mode st_mode{};
  file_nlink st_nlink{};
  user_id st_uid{};
  group_id st_gid{};
  file_time st_atimespec{};
  file_time st_mtimespec{};
  file_time st_ctimespec{};
  file_time st_birthtimespec{};
  file_size st_size{};
  block_count st_blocks{};
  block_size st_blksize{};
  std::uint32_t st_flags{};
};

struct statfs {
  std::uint64_t f_bavail{};
  std::uint64_t f_bfree{};
  std::uint64_t f_blocks{};
  std::uint64_t f_favail{};
  std::uint64_t f_ffree{};
  std::uint64_t f_files{};
};

struct statfs_x final : public statfs {
  std::array<char, 1024U> f_mntfromname{};
};
#pragma pack()

#if !defined(_WIN32)
[[nodiscard]] auto create_open_flags(std::uint32_t flags) -> open_flags;

[[nodiscard]] auto create_os_open_flags(const open_flags &flags)
    -> std::uint32_t;
#endif // !defined(_WIN32)
} // namespace repertory::remote

NLOHMANN_JSON_NAMESPACE_BEGIN
template <> struct adl_serializer<repertory::remote::remote_config> {
  static void to_json(json &data,
                      const repertory::remote::remote_config &value) {
    data[repertory::JSON_API_PORT] = value.api_port;
    data[repertory::JSON_CONNECT_TIMEOUT_MS] = value.conn_timeout_ms;
    data[repertory::JSON_ENCRYPTION_TOKEN] = value.encryption_token;
    data[repertory::JSON_HOST_NAME_OR_IP] = value.host_name_or_ip;
    data[repertory::JSON_MAX_CONNECTIONS] = value.max_connections;
    data[repertory::JSON_RECV_TIMEOUT_MS] = value.recv_timeout_ms;
    data[repertory::JSON_SEND_TIMEOUT_MS] = value.send_timeout_ms;
  }

  static void from_json(const json &data,
                        repertory::remote::remote_config &value) {
    data.at(repertory::JSON_API_PORT).get_to(value.api_port);
    if (data.contains(repertory::JSON_CONNECT_TIMEOUT_MS)) {
      data.at(repertory::JSON_CONNECT_TIMEOUT_MS).get_to(value.conn_timeout_ms);
    }
    data.at(repertory::JSON_ENCRYPTION_TOKEN).get_to(value.encryption_token);
    data.at(repertory::JSON_HOST_NAME_OR_IP).get_to(value.host_name_or_ip);
    data.at(repertory::JSON_MAX_CONNECTIONS).get_to(value.max_connections);
    data.at(repertory::JSON_RECV_TIMEOUT_MS).get_to(value.recv_timeout_ms);
    data.at(repertory::JSON_SEND_TIMEOUT_MS).get_to(value.send_timeout_ms);
  }
};

template <> struct adl_serializer<repertory::remote::remote_mount> {
  static void to_json(json &data,
                      const repertory::remote::remote_mount &value) {
    data[repertory::JSON_API_PORT] = value.api_port;
    data[repertory::JSON_CLIENT_POOL_SIZE] = value.client_pool_size;
    data[repertory::JSON_ENABLE_REMOTE_MOUNT] = value.enable;
    data[repertory::JSON_ENCRYPTION_TOKEN] = value.encryption_token;
  }

  static void from_json(const json &data,
                        repertory::remote::remote_mount &value) {
    data.at(repertory::JSON_API_PORT).get_to(value.api_port);
    data.at(repertory::JSON_CLIENT_POOL_SIZE).get_to(value.client_pool_size);
    data.at(repertory::JSON_ENABLE_REMOTE_MOUNT).get_to(value.enable);
    data.at(repertory::JSON_ENCRYPTION_TOKEN).get_to(value.encryption_token);
  }
};
NLOHMANN_JSON_NAMESPACE_END

#endif // REPERTORY_INCLUDE_TYPES_REMOTE_HPP_
