/*
  Copyright <2018-2024> <scott.e.graves@protonmail.com>

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
#include "drives/fuse/remotefuse/remote_client.hpp"

#include "app_config.hpp"
#include "comm/packet/packet.hpp"
#include "utils/path.hpp"

namespace repertory::remote_fuse {
remote_client::remote_client(const app_config &config)
    : config_(config), packet_client_(config.get_remote_config()) {}

auto remote_client::fuse_access(const char *path, const std::int32_t &mask)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(mask);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_chflags(const char *path, std::uint32_t flags)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(flags);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_chmod(const char *path, const remote::file_mode &mode)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(mode);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_chown(const char *path, const remote::user_id &uid,
                               const remote::group_id &gid)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(uid);
  request.encode(gid);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_destroy() -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, service_flags);
}

/*packet::error_type remote_client::fuse_fallocate(const char *path, const
std::int32_t &mode, const remote::file_offset &offset, const remote::file_offset
&length, const remote::file_handle &handle) { packet request;
  request.encode(path);
  request.encode(mode);
  request.encode(offset);
  request.encode(length);
  request.encode(handle);

  std::uint32_t service_flags {};
  return packetClient_.send(function_name, request, service_flags);
}*/

auto remote_client::fuse_fgetattr(const char *path, remote::stat &st,
                                  bool &directory,
                                  const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(handle);
  request.encode(uid_);
  request.encode(gid_);

  packet response;
  std::uint32_t service_flags{};
  auto ret =
      packet_client_.send(function_name, request, response, service_flags);
  if (ret == 0) {
    if ((ret = response.decode(st)) == 0) {
      std::uint8_t d{};
      if ((ret = response.decode(d)) == 0) {
        directory = static_cast<bool>(d);
      }
    }
  }

  return ret;
}

auto remote_client::fuse_fsetattr_x(const char *path,
                                    const remote::setattr_x &attr,
                                    const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(attr);
  request.encode(handle);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_fsync(const char *path, const std::int32_t &datasync,
                               const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(datasync);
  request.encode(handle);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_ftruncate(const char *path,
                                   const remote::file_offset &size,
                                   const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(size);
  request.encode(handle);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_getattr(const char *path, remote::stat &st,
                                 bool &directory) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(uid_);
  request.encode(gid_);

  packet response;
  std::uint32_t service_flags{};
  auto ret =
      packet_client_.send(function_name, request, response, service_flags);
  if (ret == 0) {
    if ((ret = response.decode(st)) == 0) {
      std::uint8_t d = 0;
      if ((ret = response.decode(d)) == 0) {
        directory = static_cast<bool>(d);
      }
    }
  }

  return ret;
}

/*packet::error_type remote_client::fuse_getxattr(const char *path, const char
*name, char *value, const remote::file_size &size) { packet::error_type ret = 0;
if (size > std::numeric_limits<std::size_t>::max()) { ret = -ERANGE; } else {
packet request; request.encode(path); request.encode(name);
    request.encode(size);

    packet response;
    std::uint32_t service_flags {};
    if ((ret = packetClient_.send(function_name, request, response,
service_flags)) == 0) { remote::file_size size2; if ((ret =
response.decode(size2)) == 0) { if (size2 >
std::numeric_limits<std::size_t>::max()) { ret = -ERANGE; } else { memcpy(value,
response.CurrentPointer(), static_cast<std::size_t>(size2));
        }
      }
    }
  }

  return ret;
}

packet::error_type remote_client::fuse_getxattr_osx(const char *path, const char
*name, char *value, const remote::file_size &size, std::uint32_t position) {
packet::error_type ret = 0; if (size > std::numeric_limits<std::size_t>::max())
{ ret = -ERANGE; } else { packet request; request.encode(path);
    request.encode(name);
    request.encode(size);
    request.encode(position);

    packet response;
    std::uint32_t service_flags {};
    if ((ret = packetClient_.send(function_name, request, response,
service_flags)) == 0) { remote::file_size size2; if ((ret =
response.decode(size2)) == 0) { if (size2 >
std::numeric_limits<std::size_t>::max()) { ret = -ERANGE; } else { memcpy(value,
response.CurrentPointer(), static_cast<std::size_t>(size2));
        }
      }
    }
  }

  return ret;
}*/

auto remote_client::fuse_getxtimes(const char *path,
                                   remote::file_time &bkuptime,
                                   remote::file_time &crtime)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);

  packet response;
  std::uint32_t service_flags{};
  auto ret =
      packet_client_.send(function_name, request, response, service_flags);
  if (ret == 0) {
    DECODE_OR_RETURN(&response, bkuptime);
    DECODE_OR_RETURN(&response, crtime);
  }

  return ret;
}

auto remote_client::fuse_init() -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, service_flags);
}

/*packet::error_type remote_client::fuse_listxattr(const char *path, char
*buffer, const remote::file_size &size) { packet::error_type ret = 0; if (size >
std::numeric_limits<std::size_t>::max()) { ret = -ERANGE; } else { packet
request; request.encode(path); request.encode(size);

    packet response;
    std::uint32_t service_flags {};
    if ((ret = packetClient_.send(function_name, request, response,
service_flags)) == 0) { remote::file_size size2; if ((ret =
response.decode(size2)) == 0) { if (size2 >
std::numeric_limits<std::size_t>::max()) { ret = -ERANGE; } else {
          memcpy(buffer, response.CurrentPointer(),
static_cast<std::size_t>(size2));
        }
      }
    }
  }

  return ret;
}*/

auto remote_client::fuse_mkdir(const char *path, const remote::file_mode &mode)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(mode);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_opendir(const char *path, remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);

  packet response;
  std::uint32_t service_flags{};
  auto ret =
      packet_client_.send(function_name, request, response, service_flags);
  if (ret == 0) {
    ret = response.decode(handle);
  }

  return ret;
}

auto remote_client::fuse_create(const char *path, const remote::file_mode &mode,
                                const remote::open_flags &flags,
                                remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(mode);
  request.encode(flags);

  packet response;
  std::uint32_t service_flags{};
  auto ret =
      packet_client_.send(function_name, request, response, service_flags);
  if (ret == 0) {
    ret = response.decode(handle);
  }

  return ret;
}

auto remote_client::fuse_open(const char *path, const remote::open_flags &flags,
                              remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(flags);

  packet response;
  std::uint32_t service_flags{};
  auto ret =
      packet_client_.send(function_name, request, response, service_flags);
  if (ret == 0) {
    ret = response.decode(handle);
  }

  return ret;
}

auto remote_client::fuse_read(const char *path, char *buffer,
                              const remote::file_size &read_size,
                              const remote::file_offset &read_offset,
                              const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(read_size);
  request.encode(read_offset);
  request.encode(handle);

  packet response;
  std::uint32_t service_flags{};
  auto ret =
      packet_client_.send(function_name, request, response, service_flags);
  if (ret > 0) {
    memcpy(buffer, response.current_pointer(), static_cast<std::size_t>(ret));
  }

  return ret;
}

auto remote_client::fuse_rename(const char *from, const char *to)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(from);
  request.encode(to);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_write(const char *path, const char *buffer,
                               const remote::file_size &write_size,
                               const remote::file_offset &write_offset,
                               const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  if (write_size > std::numeric_limits<std::size_t>::max()) {
    return -ERANGE;
  }

  packet request;
  request.encode(path);
  request.encode(write_size);
  request.encode(buffer, static_cast<std::size_t>(write_size));
  request.encode(write_offset);
  request.encode(handle);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_write_base64(const char *path, const char *buffer,
                                      const remote::file_size &write_size,
                                      const remote::file_offset &write_offset,
                                      const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  if (write_size > std::numeric_limits<std::size_t>::max()) {
    return -ERANGE;
  }

  packet request;
  request.encode(path);
  request.encode(write_size);
  request.encode(buffer, static_cast<std::size_t>(write_size));
  request.encode(write_offset);
  request.encode(handle);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_readdir(const char *path,
                                 const remote::file_offset &offset,
                                 const remote::file_handle &handle,
                                 std::string &item_path) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(offset);
  request.encode(handle);

  packet response;
  std::uint32_t service_flags{};
  auto ret =
      packet_client_.send(function_name, request, response, service_flags);
  if (ret == 0) {
    DECODE_OR_IGNORE(&response, item_path);
  }

  return ret;
}

auto remote_client::fuse_release(const char *path,
                                 const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(handle);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_releasedir(const char *path,
                                    const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(handle);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

/*packet::error_type remote_client::fuse_removexattr(const char *path, const
char *name) override { packet request; request.encode(path);
request.Encode(name);

  std::uint32_t service_flags {};
  return packetClient_.send(function_name, request, service_flags);
}*/

auto remote_client::fuse_rmdir(const char *path) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_setattr_x(const char *path, remote::setattr_x &attr)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(attr);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_setbkuptime(const char *path,
                                     const remote::file_time &bkuptime)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(bkuptime);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_setchgtime(const char *path,
                                    const remote::file_time &chgtime)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(chgtime);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_setcrtime(const char *path,
                                   const remote::file_time &crtime)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(crtime);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_setvolname(const char *volname) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(volname);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

/*packet::error_type remote_client::fuse_setxattr(const char *path, const char
*name, const char *value, const remote::file_size &size, const std::int32_t
&flags) { packet::error_type ret = 0; if (size >
std::numeric_limits<std::size_t>::max()) { ret = -ERANGE; } else { packet
request; request.encode(path); request.encode(name); request.encode(size);
    request.encode(value, static_cast<std::size_t>(size));
    request.encode(flags);

    std::uint32_t service_flags {};
    ret = packetClient_.send(function_name, request, service_flags);
  }

  return ret;
}

packet::error_type remote_client::fuse_setxattr_osx(const char *path, const char
*name, const char *value, const remote::file_size &size, const std::int32_t
&flags, const std::uint32_t &position) override { packet::error_type ret = 0; if
(size > std::numeric_limits<std::size_t>::max()) { ret = -ERANGE; } else {
packet request; request.encode(path); request.Encode(name);
request.Encode(size); request.encode(value, static_cast<std::size_t>(size));
request.encode(flags); request.encode(position);

    std::uint32_t service_flags {};
    ret = packetClient_.send(function_name, request, service_flags);
  }

  return ret;
}*/

auto remote_client::fuse_statfs(const char *path, std::uint64_t frsize,
                                remote::statfs &st) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(frsize);

  packet response;
  std::uint32_t service_flags{};
  auto ret =
      packet_client_.send(function_name, request, response, service_flags);
  if (ret == 0) {
    ret = response.decode(st);
  }

  return ret;
}

auto remote_client::fuse_statfs_x(const char *path, std::uint64_t bsize,
                                  remote::statfs_x &st) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(bsize);

  packet response;
  std::uint32_t service_flags{};
  auto ret =
      packet_client_.send(function_name, request, response, service_flags);
  if (ret == 0) {
    ret = response.decode(st);
  }

  return ret;
}

auto remote_client::fuse_truncate(const char *path,
                                  const remote::file_offset &size)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(size);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_unlink(const char *path) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::fuse_utimens(const char *path, const remote::file_time *tv,
                                 std::uint64_t op0, std::uint64_t op1)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(&tv[0], sizeof(remote::file_time) * 2);
  request.encode(op0);
  request.encode(op1);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

auto remote_client::json_create_directory_snapshot(const std::string &path,
                                                   json &json_data)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);

  packet response;
  std::uint32_t service_flags{};
  auto ret =
      packet_client_.send(function_name, request, response, service_flags);
  if (ret == 0) {
    ret = packet::decode_json(response, json_data);
  }

  return ret;
}

auto remote_client::json_read_directory_snapshot(
    const std::string &path, const remote::file_handle &handle,
    std::uint32_t page, json &json_data) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(handle);
  request.encode(page);

  packet response;
  std::uint32_t service_flags{};
  auto ret =
      packet_client_.send(function_name, request, response, service_flags);
  if (ret == 0) {
    ret = packet::decode_json(response, json_data);
  }

  return ret;
}

auto remote_client::json_release_directory_snapshot(
    const std::string &path, const remote::file_handle &handle)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);
  request.encode(handle);

  std::uint32_t service_flags{};
  return packet_client_.send(function_name, request, service_flags);
}

void remote_client::set_fuse_uid_gid(const remote::user_id &uid,
                                     const remote::group_id &gid) {
  uid_ = uid;
  gid_ = gid;
}
} // namespace repertory::remote_fuse
