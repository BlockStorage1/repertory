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
#include "drives/fuse/remotefuse/remote_client.hpp"
#include "comm/packet/packet.hpp"
#include "app_config.hpp"
#include "events/events.hpp"
#include "utils/path_utils.hpp"

namespace repertory::remote_fuse {
// clang-format off
E_SIMPLE3(remote_fuse_client_event, debug, true,
  std::string, function, func, E_STRING,
  std::string, api_path, ap, E_STRING,
  int, result, res, E_FROM_INT32
);
// clang-format on

#define RAISE_REMOTE_CLIENT_FUSE_EVENT(func, file, ret)                                            \
  if (config_.get_enable_drive_events() &&                                                         \
      (((config_.get_event_level() >= remote_fuse_client_event::level) && (ret != 0)) ||           \
       (config_.get_event_level() >= event_level::verbose)))                                       \
  event_system::instance().raise<remote_fuse_client_event>(func, file, ret)

remote_client::remote_client(const app_config &config)
    : config_(config),
      packet_client_(config.get_remote_host_name_or_ip(), config.get_remote_max_connections(),
                     config.get_remote_port(), config.get_remote_receive_timeout_secs(),
                     config.get_remote_send_timeout_secs(), config.get_remote_token()) {}

packet::error_type remote_client::fuse_access(const char *path, const std::int32_t &mask) {
  packet request;
  request.encode(path);
  request.encode(mask);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_chflags(const char *path, const std::uint32_t &flags) {
  packet request;
  request.encode(path);
  request.encode(flags);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_chmod(const char *path, const remote::file_mode &mode) {
  packet request;
  request.encode(path);
  request.encode(mode);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_chown(const char *path, const remote::user_id &uid,
                                             const remote::group_id &gid) {
  packet request;
  request.encode(path);
  request.encode(uid);
  request.encode(gid);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_destroy() {
  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, "", ret);
  return ret;
}

/*packet::error_type remote_client::fuse_fallocate(const char *path, const std::int32_t &mode,
                                       const remote::file_offset &offset,
                                       const remote::file_offset &length,
                                       const remote::file_handle &handle) {
  packet request;
  request.encode(path);
  request.encode(mode);
  request.encode(offset);
  request.encode(length);
  request.encode(handle);

  std::uint32_t service_flags = 0;
  const auto ret = packetClient_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}*/

packet::error_type remote_client::fuse_fgetattr(const char *path, remote::stat &st, bool &directory,
                                                const remote::file_handle &handle) {
  packet request;
  request.encode(path);
  request.encode(handle);
  request.encode(uid_);
  request.encode(gid_);

  packet response;
  std::uint32_t service_flags = 0;
  auto ret = packet_client_.send(__FUNCTION__, request, response, service_flags);
  if (ret == 0) {
    if ((ret = response.decode(st)) == 0) {
      std::uint8_t d = 0;
      if ((ret = response.decode(d)) == 0) {
        directory = static_cast<bool>(d);
      }
    }
  }
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_fsetattr_x(const char *path, const remote::setattr_x &attr,
                                                  const remote::file_handle &handle) {
  packet request;
  request.encode(path);
  request.encode(attr);
  request.encode(handle);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_fsync(const char *path, const std::int32_t &datasync,
                                             const remote::file_handle &handle) {
  packet request;
  request.encode(path);
  request.encode(datasync);
  request.encode(handle);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_ftruncate(const char *path, const remote::file_offset &size,
                                                 const remote::file_handle &handle) {
  packet request;
  request.encode(path);
  request.encode(size);
  request.encode(handle);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_getattr(const char *path, remote::stat &st,
                                               bool &directory) {
  packet request;
  request.encode(path);
  request.encode(uid_);
  request.encode(gid_);

  packet response;
  std::uint32_t service_flags = 0;
  auto ret = packet_client_.send(__FUNCTION__, request, response, service_flags);
  if (ret == 0) {
    if ((ret = response.decode(st)) == 0) {
      std::uint8_t d = 0;
      if ((ret = response.decode(d)) == 0) {
        directory = static_cast<bool>(d);
      }
    }
  }
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

/*packet::error_type remote_client::fuse_getxattr(const char *path, const char *name, char
*value, const remote::file_size &size) { packet::error_type ret = 0; if (size >
std::numeric_limits<std::size_t>::max()) { ret = -ERANGE; } else { packet request;
    request.encode(path);
    request.encode(name);
    request.encode(size);

    packet response;
    std::uint32_t service_flags = 0;
    if ((ret = packetClient_.send(__FUNCTION__, request, response, service_flags)) == 0) {
      remote::file_size size2;
      if ((ret = response.decode(size2)) == 0) {
        if (size2 > std::numeric_limits<std::size_t>::max()) {
          ret = -ERANGE;
        } else {
          memcpy(value, response.CurrentPointer(), static_cast<std::size_t>(size2));
        }
      }
    }
  }
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_getxattr_osx(const char *path, const char *name, char
*value, const remote::file_size &size, const std::uint32_t &position) { packet::error_type ret
= 0; if (size > std::numeric_limits<std::size_t>::max()) { ret = -ERANGE; } else { packet request;
    request.encode(path);
    request.encode(name);
    request.encode(size);
    request.encode(position);

    packet response;
    std::uint32_t service_flags = 0;
    if ((ret = packetClient_.send(__FUNCTION__, request, response, service_flags)) == 0) {
      remote::file_size size2;
      if ((ret = response.decode(size2)) == 0) {
        if (size2 > std::numeric_limits<std::size_t>::max()) {
          ret = -ERANGE;
        } else {
          memcpy(value, response.CurrentPointer(), static_cast<std::size_t>(size2));
        }
      }
    }
  }
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}*/

packet::error_type remote_client::fuse_getxtimes(const char *path, remote::file_time &bkuptime,
                                                 remote::file_time &crtime) {
  packet request;
  request.encode(path);

  packet response;
  std::uint32_t service_flags = 0;
  auto ret = packet_client_.send(__FUNCTION__, request, response, service_flags);
  if (ret == 0) {
    response.decode(bkuptime);
    response.decode(crtime);
  }
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_init() {
  std::uint32_t service_flags = 0;
  auto ret = packet_client_.send(__FUNCTION__, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, "", ret);
  return ret;
}

/*packet::error_type remote_client::fuse_listxattr(const char *path, char *buffer,
                                       const remote::file_size &size) {
  packet::error_type ret = 0;
  if (size > std::numeric_limits<std::size_t>::max()) {
    ret = -ERANGE;
  } else {
    packet request;
    request.encode(path);
    request.encode(size);

    packet response;
    std::uint32_t service_flags = 0;
    if ((ret = packetClient_.send(__FUNCTION__, request, response, service_flags)) == 0) {
      remote::file_size size2;
      if ((ret = response.decode(size2)) == 0) {
        if (size2 > std::numeric_limits<std::size_t>::max()) {
          ret = -ERANGE;
        } else {
          memcpy(buffer, response.CurrentPointer(), static_cast<std::size_t>(size2));
        }
      }
    }
  }
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}*/

packet::error_type remote_client::fuse_mkdir(const char *path, const remote::file_mode &mode) {
  packet request;
  request.encode(path);
  request.encode(mode);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_opendir(const char *path, remote::file_handle &handle) {
  packet request;
  request.encode(path);

  packet response;
  std::uint32_t service_flags = 0;
  auto ret = packet_client_.send(__FUNCTION__, request, response, service_flags);
  if (ret == 0) {
    ret = response.decode(handle);
  }
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_create(const char *path, const remote::file_mode &mode,
                                              const remote::open_flags &flags,
                                              remote::file_handle &handle) {
  packet request;
  request.encode(path);
  request.encode(mode);
  request.encode(flags);

  packet response;
  std::uint32_t service_flags = 0;
  auto ret = packet_client_.send(__FUNCTION__, request, response, service_flags);
  if (ret == 0) {
    ret = response.decode(handle);
  }
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_open(const char *path, const remote::open_flags &flags,
                                            remote::file_handle &handle) {
  packet request;
  request.encode(path);
  request.encode(flags);

  packet response;
  std::uint32_t service_flags = 0;
  auto ret = packet_client_.send(__FUNCTION__, request, response, service_flags);
  if (ret == 0) {
    ret = response.decode(handle);
  }
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_read(const char *path, char *buffer,
                                            const remote::file_size &read_size,
                                            const remote::file_offset &read_offset,
                                            const remote::file_handle &handle) {
  packet request;
  request.encode(path);
  request.encode(read_size);
  request.encode(read_offset);
  request.encode(handle);

  packet response;
  std::uint32_t service_flags = 0;
  auto ret = packet_client_.send(__FUNCTION__, request, response, service_flags);
  if (ret > 0) {
    memcpy(buffer, response.current_pointer(), ret);
  }

  if (ret < 0) {
    RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  }
  return ret;
}

packet::error_type remote_client::fuse_rename(const char *from, const char *to) {
  packet request;
  request.encode(from);
  request.encode(to);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(
      __FUNCTION__, utils::path::create_api_path(from) + "|" + utils::path::create_api_path(to),
      ret);
  return ret;
}

packet::error_type remote_client::fuse_write(const char *path, const char *buffer,
                                             const remote::file_size &write_size,
                                             const remote::file_offset &write_offset,
                                             const remote::file_handle &handle) {
  packet::error_type ret = 0;
  if (write_size > std::numeric_limits<std::size_t>::max()) {
    ret = -ERANGE;
  } else {
    packet request;
    request.encode(path);
    request.encode(write_size);
    request.encode(buffer, static_cast<std::size_t>(write_size));
    request.encode(write_offset);
    request.encode(handle);

    std::uint32_t service_flags = 0;
    ret = packet_client_.send(__FUNCTION__, request, service_flags);
  }

  if (ret < 0) {
    RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  }
  return ret;
}

packet::error_type remote_client::fuse_write_base64(const char *path, const char *buffer,
                                                    const remote::file_size &write_size,
                                                    const remote::file_offset &write_offset,
                                                    const remote::file_handle &handle) {
  packet::error_type ret = 0;
  if (write_size > std::numeric_limits<std::size_t>::max()) {
    ret = -ERANGE;
  } else {
    packet request;
    request.encode(path);
    request.encode(write_size);
    request.encode(buffer, static_cast<std::size_t>(write_size));
    request.encode(write_offset);
    request.encode(handle);

    std::uint32_t service_flags = 0;
    ret = packet_client_.send(__FUNCTION__, request, service_flags);
  }

  if (ret < 0) {
    RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  }
  return ret;
}

packet::error_type remote_client::fuse_readdir(const char *path, const remote::file_offset &offset,
                                               const remote::file_handle &handle,
                                               std::string &item_path) {
  packet request;
  request.encode(path);
  request.encode(offset);
  request.encode(handle);

  packet response;
  std::uint32_t service_flags = 0;
  auto ret = packet_client_.send(__FUNCTION__, request, response, service_flags);
  if (ret == 0) {
    DECODE_OR_IGNORE(&response, item_path);
  }

  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path),
                                 ret == -120 ? 0 : ret);
  return ret;
}

packet::error_type remote_client::fuse_release(const char *path,
                                               const remote::file_handle &handle) {
  packet request;
  request.encode(path);
  request.encode(handle);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_releasedir(const char *path,
                                                  const remote::file_handle &handle) {
  packet request;
  request.encode(path);
  request.encode(handle);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

/*packet::error_type remote_client::fuse_removexattr(const char *path, const char *name)
override { packet request; request.encode(path); request.Encode(name);

  std::uint32_t service_flags = 0;
  const auto ret = packetClient_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}*/

packet::error_type remote_client::fuse_rmdir(const char *path) {
  packet request;
  request.encode(path);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_setattr_x(const char *path, remote::setattr_x &attr) {
  packet request;
  request.encode(path);
  request.encode(attr);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_setbkuptime(const char *path,
                                                   const remote::file_time &bkuptime) {
  packet request;
  request.encode(path);
  request.encode(bkuptime);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_setchgtime(const char *path,
                                                  const remote::file_time &chgtime) {
  packet request;
  request.encode(path);
  request.encode(chgtime);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_setcrtime(const char *path,
                                                 const remote::file_time &crtime) {
  packet request;
  request.encode(path);
  request.encode(crtime);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_setvolname(const char *volname) {
  packet request;
  request.encode(volname);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, volname ? volname : "", ret);
  return ret;
}

/*packet::error_type remote_client::fuse_setxattr(const char *path, const char *name, const
char *value, const remote::file_size &size, const std::int32_t &flags) { packet::error_type ret
= 0; if (size > std::numeric_limits<std::size_t>::max()) { ret = -ERANGE; } else { packet request;
    request.encode(path);
    request.encode(name);
    request.encode(size);
    request.encode(value, static_cast<std::size_t>(size));
    request.encode(flags);

    std::uint32_t service_flags = 0;
    ret = packetClient_.send(__FUNCTION__, request, service_flags);
  }

  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_setxattr_osx(const char *path, const char *name, const
char *value, const remote::file_size &size, const std::int32_t &flags, const std::uint32_t
&position) override { packet::error_type ret = 0; if (size >
std::numeric_limits<std::size_t>::max()) { ret = -ERANGE; } else { packet request;
request.encode(path); request.Encode(name); request.Encode(size); request.encode(value,
static_cast<std::size_t>(size)); request.encode(flags); request.encode(position);

    std::uint32_t service_flags = 0;
    ret = packetClient_.send(__FUNCTION__, request, service_flags);
  }
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}*/

packet::error_type remote_client::fuse_statfs(const char *path, const std::uint64_t &frsize,
                                              remote::statfs &st) {
  packet request;
  request.encode(path);
  request.encode(frsize);

  packet response;
  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, response, service_flags);
  if (ret == 0) {
    response.decode(st);
  }
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_statfs_x(const char *path, const std::uint64_t &bsize,
                                                remote::statfs_x &st) {
  packet request;
  request.encode(path);
  request.encode(bsize);

  packet response;
  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, response, service_flags);
  if (ret == 0) {
    response.decode(st);
  }
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_truncate(const char *path, const remote::file_offset &size) {
  packet request;
  request.encode(path);
  request.encode(size);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_unlink(const char *path) {
  packet request;
  request.encode(path);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::fuse_utimens(const char *path, const remote::file_time *tv,
                                               const std::uint64_t &op0, const std::uint64_t &op1) {
  packet request;
  request.encode(path);
  request.encode(&tv[0], sizeof(remote::file_time) * 2);
  request.encode(op0);
  request.encode(op1);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);
  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, utils::path::create_api_path(path), ret);
  return ret;
}

packet::error_type remote_client::json_create_directory_snapshot(const std::string &path,
                                                                 json &json_data) {
  packet request;
  request.encode(path);

  packet response;
  std::uint32_t service_flags = 0;
  auto ret = packet_client_.send(__FUNCTION__, request, response, service_flags);
  if (ret == 0) {
    ret = packet::decode_json(response, json_data);
  }

  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, path, ret);
  return ret;
}

packet::error_type remote_client::json_read_directory_snapshot(const std::string &path,
                                                               const remote::file_handle &handle,
                                                               const std::uint32_t &page,
                                                               json &json_data) {
  packet request;
  request.encode(path);
  request.encode(handle);
  request.encode(page);

  packet response;
  std::uint32_t service_flags = 0;
  auto ret = packet_client_.send(__FUNCTION__, request, response, service_flags);
  if (ret == 0) {
    ret = packet::decode_json(response, json_data);
  }

  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, path, ret);
  return ret;
}

packet::error_type
remote_client::json_release_directory_snapshot(const std::string &path,
                                               const remote::file_handle &handle) {
  packet request;
  request.encode(path);
  request.encode(handle);

  std::uint32_t service_flags = 0;
  const auto ret = packet_client_.send(__FUNCTION__, request, service_flags);

  RAISE_REMOTE_CLIENT_FUSE_EVENT(__FUNCTION__, path, ret);
  return ret;
}

void remote_client::set_fuse_uid_gid(const remote::user_id &uid, const remote::group_id &gid) {
  uid_ = uid;
  gid_ = gid;
}
} // namespace repertory::remote_fuse
