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
#ifndef REPERTORY_INCLUDE_DRIVES_REMOTE_REMOTE_SERVER_BASE_HPP_
#define REPERTORY_INCLUDE_DRIVES_REMOTE_REMOTE_SERVER_BASE_HPP_

#include "app_config.hpp"
#include "comm/packet/client_pool.hpp"
#include "comm/packet/packet.hpp"
#include "comm/packet/packet_server.hpp"
#include "drives/fuse/remotefuse/i_remote_instance.hpp"
#include "drives/remote/remote_open_file_table.hpp"
#include "drives/winfsp/remotewinfsp/i_remote_instance.hpp"
#include "events/event_system.hpp"
#include "events/types/service_start_begin.hpp"
#include "events/types/service_start_end.hpp"
#include "events/types/service_stop_begin.hpp"
#include "events/types/service_stop_end.hpp"
#include "types/remote.hpp"
#include "types/repertory.hpp"
#include "utils/base64.hpp"
#include "utils/path.hpp"
#include "utils/polling.hpp"

namespace repertory {
template <typename drive>
class remote_server_base : public remote_open_file_table,
                           public virtual remote_winfsp::i_remote_instance,
                           public virtual remote_fuse::i_remote_instance {
public:
  using handler_callback = std::function<packet::error_type(
      std::uint32_t, std::string, std::uint64_t, std::string, packet *,
      packet &)>;

public:
  remote_server_base(const remote_server_base &) = delete;
  remote_server_base(remote_server_base &&) = delete;
  auto operator=(const remote_server_base &) -> remote_server_base & = delete;
  auto operator=(remote_server_base &&) -> remote_server_base & = delete;

public:
  remote_server_base(app_config &config, drive &drv,
                     std::string_view mount_location)
      : config_(config),
        drive_(drv),
        mount_location_(std::string(mount_location)) {
    REPERTORY_USES_FUNCTION_NAME();

    event_system::instance().raise<service_start_begin>(function_name,
                                                        "remote_server_base");
    packet_server_ = std::make_unique<packet_server>(
        config_.get_remote_mount().api_port,
        config_.get_remote_mount().encryption_token,
        config_.get_remote_mount().client_pool_size,
        [this](auto &&client_id) { return this->closed_handler(client_id); },
        [this](auto &&service_flags, auto &&client_id, auto &&thread_id,
               auto &&method, auto &&request, auto &&response,
               auto &&message_complete) {
          return this->message_handler(service_flags, client_id, thread_id,
                                       method, request, response,
                                       message_complete);
        });

    polling::instance().set_callback({
        "remote_server_expired",
        polling::frequency::high,
        [this](auto && /* stop */) { client_pool_.remove_expired(); },
    });

    event_system::instance().raise<service_start_end>(function_name,
                                                      "remote_server_base");
  }

  ~remote_server_base() override {
    REPERTORY_USES_FUNCTION_NAME();

    event_system::instance().raise<service_stop_begin>(function_name,
                                                       "remote_server_base");
    polling::instance().remove_callback("remote_server_expired");

    client_pool_.shutdown();

    packet_server_.reset();
    event_system::instance().raise<service_stop_end>(function_name,
                                                     "remote_server_base");
  }

protected:
  app_config &config_;
  drive &drive_;
  std::string mount_location_;

private:
  client_pool client_pool_;
  std::unique_ptr<packet_server> packet_server_;

private:
  const std::unordered_map<std::string, handler_callback> handler_lookup_ = {
      {
          "::check",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */,
                 auto && /* request */,
                 auto && /* response */) -> auto { return 0; },
      },
      {
          "::fuse_access",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_access(request);
          },
      },
      {
          "::fuse_chflags",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_chflags(request);
          },
      },
      {
          "::fuse_chmod",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_chmod(request);
          },
      },
      {
          "::fuse_chown",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_chown(request);
          },
      },
      {
          "::fuse_create",
          [this](auto && /* service_flags */, auto &&client_id,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_fuse_create(client_id, request, response);
          },
      },
      {
          "::fuse_destroy",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */,
                 auto && /* request */, auto && /* response */) -> auto {
            return this->handle_fuse_destroy();
          },
      },
      {
          "::fuse_fgetattr",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_fuse_fgetattr(request, response);
          },
      },
      {
          "::fuse_fsetattr_x",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_fsetattr_x(request);
          },
      },
      {
          "::fuse_fsync",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_fsync(request);
          },
      },
      {
          "::fuse_ftruncate",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_ftruncate(request);
          },
      },
      {
          "::fuse_getattr",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_fuse_getattr(request, response);
          },
      },
      {
          "::fuse_getxtimes",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_fuse_getxtimes(request, response);
          },
      },
      {
          "::fuse_init",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */,
                 auto && /* request */, auto && /* response */) -> auto {
            return this->handle_fuse_init();
          },
      },
      {
          "::fuse_mkdir",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_mkdir(request);
          },
      },
      {
          "::fuse_open",
          [this](auto && /* service_flags */, auto &&client_id,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_fuse_open(client_id, request, response);
          },
      },
      {
          "::fuse_opendir",
          [this](auto && /* service_flags */, auto &&client_id,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_fuse_opendir(client_id, request, response);
          },
      },
      {
          "::fuse_read",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_fuse_read(request, response);
          },
      },
      {
          "::fuse_readdir",
          [this](auto && /* service_flags */, auto &&client_id,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_fuse_readdir(client_id, request, response);
          },
      },
      {
          "::fuse_release",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_release(request);
          },
      },
      {
          "::fuse_releasedir",
          [this](auto && /* service_flags */, auto &&client_id,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_releasedir(client_id, request);
          },
      },
      {
          "::fuse_rename",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_rename(request);
          },
      },
      {
          "::fuse_rmdir",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_rmdir(request);
          },
      },
      {
          "::fuse_setattr_x",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_setattr_x(request);
          },
      },
      {
          "::fuse_setbkuptime",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_setbkuptime(request);
          },
      },
      {
          "::fuse_setchgtime",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_setchgtime(request);
          },
      },
      {
          "::fuse_setcrtime",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_setcrtime(request);
          },
      },
      {
          "::fuse_setvolname",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_setvolname(request);
          },
      },
      {
          "::fuse_statfs",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_fuse_statfs(request, response);
          },
      },
      {
          "::fuse_statfs_x",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_fuse_statfs_x(request, response);
          },
      },
      {
          "::fuse_truncate",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_truncate(request);
          },
      },
      {
          "::fuse_unlink",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_unlink(request);
          },
      },
      {
          "::fuse_utimens",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_utimens(request);
          },
      },
      {
          "::fuse_write",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_write(request);
          },
      },
      {
          "::fuse_write_base64",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_fuse_write_base64(request);
          },
      },
      {
          "::json_create_directory_snapshot",
          [this](auto && /* service_flags */, auto &&client_id,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_json_create_directory_snapshot(
                client_id, request, response);
          },
      },
      {
          "::json_read_directory_snapshot",
          [this](auto && /* service_flags */, auto &&client_id,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_json_read_directory_snapshot(client_id, request,
                                                             response);
          },
      },
      {
          "::json_release_directory_snapshot",
          [this](auto && /* service_flags */, auto &&client_id,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_json_release_directory_snapshot(client_id,
                                                                request);
          },
      },
      {
          "::winfsp_can_delete",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_winfsp_can_delete(request);
          },
      },
      {
          "::winfsp_cleanup",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_winfsp_cleanup(request, response);
          },
      },
      {
          "::winfsp_close",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_winfsp_close(request);
          },
      },
      {
          "::winfsp_create",
          [this](auto && /* service_flags */, auto &&client_id,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_winfsp_create(client_id, request, response);
          },
      },
      {
          "::winfsp_flush",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_winfsp_flush(request, response);
          },
      },
      {
          "::winfsp_get_file_info",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_winfsp_get_file_info(request, response);
          },
      },
      {
          "::winfsp_get_security_by_name",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_winfsp_get_security_by_name(request, response);
          },
      },
      {
          "::winfsp_get_volume_info",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */,
                 auto && /* request */, auto &&response) -> auto {
            return this->handle_winfsp_get_volume_info(response);
          },
      },
      {
          "::winfsp_mounted",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_winfsp_mounted(request);
          },
      },
      {
          "::winfsp_open",
          [this](auto && /* service_flags */, auto &&client_id,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_winfsp_open(client_id, request, response);
          },
      },
      {
          "::winfsp_overwrite",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_winfsp_overwrite(request, response);
          },
      },
      {
          "::winfsp_read",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_winfsp_read(request, response);
          },
      },
      {
          "::winfsp_read_directory",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_winfsp_read_directory(request, response);
          },
      },
      {
          "::winfsp_rename",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_winfsp_rename(request);
          },
      },
      {
          "::winfsp_set_basic_info",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_winfsp_set_basic_info(request, response);
          },
      },
      {
          "::winfsp_set_file_size",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_winfsp_set_file_size(request, response);
          },
      },
      {
          "::winfsp_unmounted",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto && /* response */) -> auto {
            return this->handle_winfsp_unmounted(request);
          },
      },
      {
          "::winfsp_write",
          [this](auto && /* service_flags */, auto && /* client_id */,
                 auto && /* thread_id */, auto && /* method */, auto &&request,
                 auto &&response) -> auto {
            return this->handle_winfsp_write(request, response);
          },
      },
  };

private:
  void closed_handler(std::string client_id) {
    client_pool_.remove_client(client_id);
    close_all(client_id);
  }

  [[nodiscard]] auto handle_fuse_access(packet *request) -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    std::int32_t mask{};
    DECODE_OR_RETURN(request, mask);

    return this->fuse_access(path.data(), mask);
  }

  [[nodiscard]] auto handle_fuse_chflags(packet *request)
      -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    std::uint32_t flags{};
    DECODE_OR_RETURN(request, flags);

    return this->fuse_chflags(path.data(), flags);
  }

  [[nodiscard]] auto handle_fuse_chmod(packet *request) -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_mode mode;
    DECODE_OR_RETURN(request, mode);

    return this->fuse_chmod(path.c_str(), mode);
  }

  [[nodiscard]] auto handle_fuse_chown(packet *request) -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::user_id uid{};
    DECODE_OR_RETURN(request, uid);

    remote::group_id gid{};
    DECODE_OR_RETURN(request, gid);

    return this->fuse_chown(path.c_str(), uid, gid);
  }

  [[nodiscard]] auto handle_fuse_create(std::string_view client_id,
                                        packet *request, packet &response)
      -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_mode mode{};
    DECODE_OR_RETURN(request, mode);

    remote::open_flags flags{};
    DECODE_OR_RETURN(request, flags);

    remote::file_handle handle{};

    ret = this->fuse_create(path.data(), mode, flags, handle);
    if (ret >= 0) {
#if defined(_WIN32)
      this->set_compat_client_id(handle, client_id);
#else  // !defined(_WIN32)
      this->set_client_id(static_cast<native_handle>(handle), client_id);
#endif // defined(_WIN32)
      response.encode(handle);
    }
    return ret;
  }

  [[nodiscard]] auto handle_fuse_destroy() -> packet::error_type {
    return this->fuse_destroy();
  }

  [[nodiscard]] auto handle_fuse_fgetattr(packet *request, packet &response) {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_handle handle{};
    DECODE_OR_RETURN(request, handle);

    remote::user_id uid{};
    DECODE_OR_RETURN(request, uid);

    remote::group_id gid{};
    DECODE_OR_RETURN(request, gid);

    remote::stat r_stat{};
    bool directory = false;
    ret = this->fuse_fgetattr(path.data(), r_stat, directory, handle);
    if (ret == 0) {
      r_stat.st_uid = uid;
      r_stat.st_gid = gid;
      response.encode(r_stat);
      response.encode(static_cast<std::uint8_t>(directory));
    }

    return ret;
  }

  [[nodiscard]] auto handle_fuse_fsetattr_x(packet *request)
      -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::setattr_x attr{};
    DECODE_OR_RETURN(request, attr);

    remote::file_handle handle{};
    DECODE_OR_RETURN(request, handle);

    return this->fuse_fsetattr_x(path.data(), attr, handle);
  }

  [[nodiscard]] auto handle_fuse_fsync(packet *request) -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    std::int32_t dataSync;
    DECODE_OR_RETURN(request, dataSync);

    remote::file_handle handle;
    DECODE_OR_RETURN(request, handle);

    return this->fuse_fsync(path.c_str(), dataSync, handle);
  }

  [[nodiscard]] auto handle_fuse_ftruncate(packet *request)
      -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_offset size;
    DECODE_OR_RETURN(request, size);

    remote::file_handle handle;
    DECODE_OR_RETURN(request, handle);

    return this->fuse_ftruncate(path.c_str(), size, handle);
  }

  [[nodiscard]] auto handle_fuse_getattr(packet *request, packet &response)
      -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::user_id uid;
    DECODE_OR_RETURN(request, uid);

    remote::group_id gid;
    DECODE_OR_RETURN(request, gid);

    remote::stat r_stat{};
    bool directory = false;
    ret = this->fuse_getattr(path.c_str(), r_stat, directory);
    if (ret == 0) {
      r_stat.st_uid = uid;
      r_stat.st_gid = gid;
      response.encode(r_stat);
      response.encode(static_cast<std::uint8_t>(directory));
    }
    return ret;
  }

  [[nodiscard]] auto handle_fuse_getxtimes(packet *request, packet &response)
      -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_time bkuptime{};
    remote::file_time crtime{};
    ret = this->fuse_getxtimes(path.c_str(), bkuptime, crtime);
    if (ret == 0) {
      response.encode(bkuptime);
      response.encode(crtime);
    }

    return ret;
  }

  [[nodiscard]] auto handle_fuse_init() -> packet::error_type {
    return this->fuse_init();
  }

  [[nodiscard]] auto handle_fuse_mkdir(packet *request) -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_mode mode;
    DECODE_OR_RETURN(request, mode);

    return this->fuse_mkdir(path.c_str(), mode);
  }

  [[nodiscard]] auto handle_fuse_open(std::string_view client_id,
                                      packet *request, packet &response)
      -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::open_flags flags;
    DECODE_OR_RETURN(request, flags);

    remote::file_handle handle;
    ret = this->fuse_open(path.c_str(), flags, handle);
    if (ret >= 0) {
#if defined(_WIN32)
      this->set_compat_client_id(handle, client_id);
#else  // !defined(_WIN32)
      this->set_client_id(static_cast<native_handle>(handle), client_id);
#endif // defined(_WIN32)
      response.encode(handle);
    }
    return ret;
  }

  [[nodiscard]] auto handle_fuse_opendir(std::string_view client_id,
                                         packet *request, packet &response)
      -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_handle handle{0};
    ret = this->fuse_opendir(path.c_str(), handle);
    if (ret >= 0) {
      this->add_directory(client_id, handle);
      response.encode(handle);
    }

    return ret;
  }

  [[nodiscard]] auto handle_fuse_read(packet *request, packet &response)
      -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_size read_size;
    DECODE_OR_RETURN(request, read_size);

    remote::file_offset read_offset;
    DECODE_OR_RETURN(request, read_offset);

    remote::file_handle handle;
    DECODE_OR_RETURN(request, handle);

    data_buffer buffer;
    ret = this->fuse_read(path.c_str(), reinterpret_cast<char *>(&buffer),
                          read_size, read_offset, handle);
    if (ret > 0) {
      response.encode(buffer.data(), buffer.size());
    }
    return ret;
  }

  [[nodiscard]] auto handle_fuse_readdir(std::string_view client_id,
                                         packet *request, packet &response)
      -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_offset offset;
    DECODE_OR_RETURN(request, offset);

    remote::file_handle handle;
    DECODE_OR_RETURN(request, handle);

    if (this->has_open_directory(client_id, handle)) {
      std::string file_path;
      ret = this->fuse_readdir(path.c_str(), offset, handle, file_path);
      if (ret == 0) {
        response.encode(file_path);
      }
    } else {
      ret = -EBADF;
    }

    return ret;
  }

  [[nodiscard]] auto handle_fuse_release(packet *request)
      -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_handle handle;
    DECODE_OR_RETURN(request, handle);

    return this->fuse_release(path.c_str(), handle);
  }

  [[nodiscard]] auto handle_fuse_releasedir(std::string_view client_id,
                                            packet *request)
      -> packet::error_type {
    auto ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_handle handle;
    DECODE_OR_RETURN(request, handle);

    ret = this->fuse_releasedir(path.c_str(), handle);
    if (this->remove_directory(client_id, handle)) {
      return ret;
    }
    return -EBADF;
  }

  [[nodiscard]] auto handle_fuse_rename(packet *request) -> packet::error_type {
    std::int32_t ret{0};

    std::string from;
    DECODE_OR_RETURN(request, from);

    std::string to;
    DECODE_OR_RETURN(request, to);

    return this->fuse_rename(from.data(), to.data());
  }

  [[nodiscard]] auto handle_fuse_rmdir(packet *request) -> packet::error_type {
    std::int32_t ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    return this->fuse_rmdir(path.data());
  }

  [[nodiscard]] auto handle_fuse_setattr_x(packet *request)
      -> packet::error_type {
    std::int32_t ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::setattr_x attr{};
    DECODE_OR_RETURN(request, attr);

    return this->fuse_setattr_x(path.data(), attr);
  }

  [[nodiscard]] auto handle_fuse_setbkuptime(packet *request)
      -> packet::error_type {
    std::int32_t ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_time bkuptime{};
    DECODE_OR_RETURN(request, bkuptime);

    return this->fuse_setbkuptime(path.data(), bkuptime);
  }

  [[nodiscard]] auto handle_fuse_setchgtime(packet *request)
      -> packet::error_type {
    std::int32_t ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_time chgtime{};
    DECODE_OR_RETURN(request, chgtime);

    return this->fuse_setchgtime(path.data(), chgtime);
  }

  [[nodiscard]] auto handle_fuse_setcrtime(packet *request)
      -> packet::error_type {
    std::int32_t ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_time crtime{};
    DECODE_OR_RETURN(request, crtime);

    return this->fuse_setcrtime(path.data(), crtime);
  }

  [[nodiscard]] auto handle_fuse_setvolname(packet *request)
      -> packet::error_type {
    std::int32_t ret{0};

    std::string name;
    DECODE_OR_RETURN(request, name);

    return this->fuse_setvolname(name.data());
  }

  [[nodiscard]] auto handle_fuse_statfs(packet *request, packet &response)
      -> packet::error_type {
    std::int32_t ret{-1};

    std::string path;
    DECODE_OR_RETURN(request, path);

    std::uint64_t frsize{};
    DECODE_OR_RETURN(request, frsize);

    remote::statfs r_stat{};
    ret = this->fuse_statfs(path.data(), frsize, r_stat);
    if (ret == 0) {
      response.encode(r_stat);
    }

    return ret;
  }

  [[nodiscard]] auto handle_fuse_statfs_x(packet *request, packet &response)
      -> packet::error_type {
    std::int32_t ret{-1};

    std::string path;
    DECODE_OR_RETURN(request, path);

    std::uint64_t bsize{};
    DECODE_OR_RETURN(request, bsize);

    remote::statfs_x r_stat{};
    ret = this->fuse_statfs_x(path.data(), bsize, r_stat);
    if (ret == 0) {
      response.encode(r_stat);
    }
    return ret;
  }

  [[nodiscard]] auto handle_fuse_truncate(packet *request)
      -> packet::error_type {
    std::int32_t ret{};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_offset size{};
    DECODE_OR_IGNORE(request, size);

    return this->fuse_truncate(path.data(), size);
  }

  [[nodiscard]] auto handle_fuse_unlink(packet *request) -> packet::error_type {
    std::int32_t ret{};

    std::string path;
    DECODE_OR_RETURN(request, path);

    return this->fuse_unlink(path.data());
  }

  [[nodiscard]] auto handle_fuse_utimens(packet *request)
      -> packet::error_type {
    std::int32_t ret{};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_time tv[2] = {0};
    ret = request->decode(&tv[0], sizeof(remote::file_time) * 2);
    if (ret == 0) {
      std::uint64_t op0{};
      DECODE_OR_RETURN(request, op0);

      std::uint64_t op1{};
      DECODE_OR_RETURN(request, op1);

      ret = this->fuse_utimens(path.data(), tv, op0, op1);
    }
    return ret;
  }

  [[nodiscard]] auto handle_fuse_write(packet *request) -> packet::error_type {
    std::int32_t ret{};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_size write_size{};
    DECODE_OR_RETURN(request, write_size);

    if (write_size > std::numeric_limits<std::size_t>::max()) {
      return -ERANGE;
    }

    data_buffer buffer(write_size);
    ret = request->decode(buffer.data(), buffer.size());
    if (ret == 0) {
      remote::file_offset write_offset{};
      DECODE_OR_RETURN(request, write_offset);

      remote::file_handle handle{};
      DECODE_OR_RETURN(request, handle);

      ret = this->fuse_write(path.data(),
                             reinterpret_cast<const char *>(buffer.data()),
                             write_size, write_offset, handle);
    }
    return ret;
  }

  [[nodiscard]] auto handle_fuse_write_base64(packet *request)
      -> packet::error_type {
    std::int32_t ret{};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_size write_size{};
    DECODE_OR_RETURN(request, write_size);

    if (write_size > std::numeric_limits<std::size_t>::max()) {
      return -ERANGE;
    }

    data_buffer buffer(write_size);
    ret = request->decode(buffer.data(), buffer.size());
    if (ret == 0) {
      buffer =
          macaron::Base64::Decode(std::string(buffer.begin(), buffer.end()));
      write_size = buffer.size();

      remote::file_offset write_offset{};
      DECODE_OR_RETURN(request, write_offset);

      remote::file_handle handle{};
      DECODE_OR_RETURN(request, handle);

      ret =
          this->fuse_write(path.data(), reinterpret_cast<char *>(buffer.data()),
                           write_size, write_offset, handle);
    }
    return ret;
  }

  [[nodiscard]] auto
  handle_json_create_directory_snapshot(std::string_view client_id,
                                        packet *request, packet &response)
      -> packet::error_type {
    std::int32_t ret{};

    std::string path;
    DECODE_OR_RETURN(request, path);

    json json_data;
    json_data["handle"] = -1;
    json_data["page_count"] = 0;
    json_data["path"] = path;
    ret = this->json_create_directory_snapshot(path.data(), json_data);
    if (ret == 0) {
      this->add_directory(client_id, json_data["handle"].get<std::uint64_t>());
      response.encode(json_data.dump(0));
    }
    return ret;
  }

  [[nodiscard]] auto
  handle_json_read_directory_snapshot(std::string_view client_id,
                                      packet *request, packet &response)
      -> packet::error_type {
    std::int32_t ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_handle handle{};
    DECODE_OR_RETURN(request, handle);

    std::uint32_t page{};
    DECODE_OR_RETURN(request, page);

    ret = -EBADF;
    if (this->has_open_directory(client_id, handle)) {
      json json_data;
      json_data["directory_list"] = std::vector<json>();
      json_data["page"] = page;
      ret = this->json_read_directory_snapshot(path, handle, page, json_data);
      if ((ret == 0) || (ret == -120)) {
        response.encode(json_data.dump(0));
      }
    }

    return ret;
  }

  [[nodiscard]] auto handle_json_release_directory_snapshot(
      std::string_view client_id, packet *request) -> packet::error_type {
    std::int32_t ret{0};

    std::string path;
    DECODE_OR_RETURN(request, path);

    remote::file_handle handle{};
    DECODE_OR_RETURN(request, handle);

    ret = this->json_release_directory_snapshot(path.data(), handle);
    if (this->remove_directory(client_id, handle)) {
      return ret;
    }
    return -EBADF;
  }

  [[nodiscard]] auto handle_winfsp_can_delete(packet *request)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    HANDLE file_desc{};
    DECODE_OR_RETURN(request, file_desc);

    std::wstring file_name;
    DECODE_OR_RETURN(request, file_name);

    return this->winfsp_can_delete(file_desc, file_name.data());
  }

  [[nodiscard]] auto handle_winfsp_cleanup(packet *request, packet &response)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    HANDLE file_desc{};
    DECODE_OR_RETURN(request, file_desc);

    std::wstring file_name;
    DECODE_OR_RETURN(request, file_name);

    UINT32 flags{};
    DECODE_OR_RETURN(request, flags);

    BOOLEAN was_deleted{};
    ret = this->winfsp_cleanup(file_desc, file_name.data(), flags, was_deleted);

    response.encode(was_deleted);
    return ret;
  }

  [[nodiscard]] auto handle_winfsp_close(packet *request)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    HANDLE file_desc{};
    DECODE_OR_RETURN(request, file_desc);

    return this->winfsp_close(file_desc);
  }

  [[nodiscard]] auto handle_winfsp_create(std::string_view client_id,
                                          packet *request, packet &response)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    std::wstring file_name;
    DECODE_OR_RETURN(request, file_name);

    UINT32 create_options{};
    DECODE_OR_RETURN(request, create_options);

    UINT32 granted_access{};
    DECODE_OR_RETURN(request, granted_access);

    UINT32 attributes{};
    DECODE_OR_RETURN(request, attributes);

    UINT64 allocation_size{};
    DECODE_OR_RETURN(request, allocation_size);

    BOOLEAN exists{0};
    remote::file_info r_info{};
    std::string normalized_name;
    PVOID file_desc{};
    ret = this->winfsp_create(file_name.data(), create_options, granted_access,
                              attributes, allocation_size, &file_desc, &r_info,
                              normalized_name, exists);
    if (ret == STATUS_SUCCESS) {
      if (exists == 0U) {
#if defined(_WIN32)
        this->set_client_id(file_desc, client_id);
#else  // !defined(_WIN32)
        this->set_client_id(static_cast<native_handle>(
                                reinterpret_cast<std::uintptr_t>(file_desc)),
                            client_id);
#endif // defined(_WIN32)
      }

      response.encode(file_desc);
      response.encode(r_info);
      response.encode(normalized_name);
      response.encode(exists);
    }

    return ret;
  }

  [[nodiscard]] auto handle_winfsp_flush(packet *request, packet &response)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    HANDLE file_desc{};
    DECODE_OR_RETURN(request, file_desc);

    remote::file_info r_info{};
    ret = this->winfsp_flush(file_desc, &r_info);
    if (ret == STATUS_SUCCESS) {
      response.encode(r_info);
    }

    return ret;
  }

  [[nodiscard]] auto handle_winfsp_get_file_info(packet *request,
                                                 packet &response)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    HANDLE file_desc{};
    DECODE_OR_RETURN(request, file_desc);

    remote::file_info r_info{};
    ret = this->winfsp_get_file_info(file_desc, &r_info);
    if (ret == STATUS_SUCCESS) {
      response.encode(r_info);
    }
    return ret;
  }

  [[nodiscard]] auto handle_winfsp_get_security_by_name(packet *request,
                                                        packet &response)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    std::wstring file_name;
    DECODE_OR_RETURN(request, file_name);

    std::uint64_t descriptor_size{};
    DECODE_OR_RETURN(request, descriptor_size);

    std::uint8_t get_attributes{};
    DECODE_OR_RETURN(request, get_attributes);

    UINT32 attributes{};
    auto *attr_ptr = get_attributes == 0U ? nullptr : &attributes;
    std::wstring string_descriptor;
    ret = this->winfsp_get_security_by_name(
        file_name.data(), attr_ptr,
        descriptor_size == 0U ? nullptr : &descriptor_size, string_descriptor);
    if (ret == STATUS_SUCCESS) {
      response.encode(string_descriptor);
      if (get_attributes != 0U) {
        response.encode(attributes);
      }
    }

    return ret;
  }

  [[nodiscard]] auto handle_winfsp_get_volume_info(packet &response)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    UINT64 total_size{};
    UINT64 free_size{};
    std::string volume_label;
    ret = this->winfsp_get_volume_info(total_size, free_size, volume_label);
    if (ret == STATUS_SUCCESS) {
      response.encode(total_size);
      response.encode(free_size);
      response.encode(volume_label);
    }

    return ret;
  }

  [[nodiscard]] auto handle_winfsp_mounted(packet *request)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    std::string version;
    DECODE_OR_RETURN(request, version);

    std::wstring location;
    DECODE_OR_RETURN(request, location);

    ret = this->winfsp_mounted(location);
    return ret;
  }

  [[nodiscard]] auto handle_winfsp_open(std::string_view client_id,
                                        packet *request, packet &response)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    std::wstring file_name;
    DECODE_OR_RETURN(request, file_name);

    UINT32 create_options{};
    DECODE_OR_RETURN(request, create_options);

    UINT32 granted_access{};
    DECODE_OR_RETURN(request, granted_access);

    remote::file_info r_info{};
    std::string normalized_name;
    PVOID file_desc{};
    ret = this->winfsp_open(file_name.data(), create_options, granted_access,
                            &file_desc, &r_info, normalized_name);
    if (ret == STATUS_SUCCESS) {
#if defined(_WIN32)
      this->set_client_id(file_desc, client_id);
#else  // !defined(_WIN32)
      this->set_client_id(static_cast<native_handle>(
                              reinterpret_cast<std::uintptr_t>(file_desc)),
                          client_id);
#endif // defined(_WIN32)
      response.encode(file_desc);
      response.encode(r_info);
      response.encode(normalized_name);
    }

    return ret;
  }

  [[nodiscard]] auto handle_winfsp_overwrite(packet *request, packet &response)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    HANDLE file_desc{};
    DECODE_OR_RETURN(request, file_desc);

    UINT32 attributes{};
    DECODE_OR_RETURN(request, attributes);

    BOOLEAN replace_attributes{};
    DECODE_OR_RETURN(request, replace_attributes);

    UINT64 allocation_size{};
    DECODE_OR_RETURN(request, allocation_size);

    remote::file_info r_info{};
    ret = this->winfsp_overwrite(file_desc, attributes, replace_attributes,
                                 allocation_size, &r_info);
    if (ret == STATUS_SUCCESS) {
      response.encode(r_info);
    }
    return ret;
  }

  [[nodiscard]] auto handle_winfsp_read(packet *request, packet &response)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    HANDLE file_desc{};
    DECODE_OR_RETURN(request, file_desc);

    UINT64 offset{};
    DECODE_OR_RETURN(request, offset);

    UINT32 length{};
    DECODE_OR_RETURN(request, length);

    data_buffer buffer(length);
    UINT32 bytes_transferred{0U};
    ret = this->winfsp_read(file_desc, buffer.data(), offset, length,
                            &bytes_transferred);
    response.encode(bytes_transferred);
    buffer.resize(bytes_transferred);

    if ((ret == STATUS_SUCCESS) && (bytes_transferred != 0U)) {
      response.encode(buffer.data(), bytes_transferred);
    }

    return ret;
  }

  [[nodiscard]] auto handle_winfsp_read_directory(packet *request,
                                                  packet &response)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    HANDLE file_desc{};
    DECODE_OR_RETURN(request, file_desc);

    std::wstring pattern;
    DECODE_OR_IGNORE(request, pattern);

    std::wstring marker;
    DECODE_OR_IGNORE(request, marker);

    json item_list;
    ret = this->winfsp_read_directory(
        file_desc, pattern.data(),
        wcsnlen(marker.data(), marker.size()) == 0U ? nullptr : marker.data(),
        item_list);
    if (ret == STATUS_SUCCESS) {
      response.encode(item_list.dump(0));
    }
    return ret;
  }

  [[nodiscard]] auto handle_winfsp_rename(packet *request)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    HANDLE file_desc{};
    DECODE_OR_RETURN(request, file_desc);

    std::wstring file_name;
    DECODE_OR_RETURN(request, file_name);

    std::wstring new_file_name;
    DECODE_OR_RETURN(request, new_file_name);

    BOOLEAN replace_if_exists{};
    DECODE_OR_RETURN(request, replace_if_exists);

    ret = this->winfsp_rename(file_desc, file_name.data(), new_file_name.data(),
                              replace_if_exists);
    return ret;
  }

  [[nodiscard]] auto handle_winfsp_set_basic_info(packet *request,
                                                  packet &response)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    HANDLE file_desc{};
    DECODE_OR_RETURN(request, file_desc);

    UINT32 attributes{};
    DECODE_OR_RETURN(request, attributes);

    UINT64 creation_time{};
    DECODE_OR_RETURN(request, creation_time);

    UINT64 last_access_time{};
    DECODE_OR_RETURN(request, last_access_time);

    UINT64 last_write_time{};
    DECODE_OR_RETURN(request, last_write_time);

    UINT64 change_time{};
    DECODE_OR_RETURN(request, change_time);

    remote::file_info r_info{};
    ret = this->winfsp_set_basic_info(file_desc, attributes, creation_time,
                                      last_access_time, last_write_time,
                                      change_time, &r_info);
    if (ret == STATUS_SUCCESS) {
      response.encode(r_info);
    }
    return ret;
  }

  [[nodiscard]] auto handle_winfsp_set_file_size(packet *request,
                                                 packet &response)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    HANDLE file_desc{};
    DECODE_OR_RETURN(request, file_desc);

    UINT64 new_size{};
    DECODE_OR_RETURN(request, new_size);

    BOOLEAN set_allocation_size{};
    DECODE_OR_RETURN(request, set_allocation_size);

    remote::file_info r_info{};
    ret = this->winfsp_set_file_size(file_desc, new_size, set_allocation_size,
                                     &r_info);
    if (ret == STATUS_SUCCESS) {
      response.encode(r_info);
    }
    return ret;
  }

  [[nodiscard]] auto handle_winfsp_unmounted(packet *request)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    std::wstring location;
    DECODE_OR_RETURN(request, location);

    return this->winfsp_unmounted(location);
  }

  [[nodiscard]] auto handle_winfsp_write(packet *request, packet &response)
      -> packet::error_type {
    auto ret = static_cast<packet::error_type>(STATUS_SUCCESS);

    HANDLE file_desc{};
    DECODE_OR_RETURN(request, file_desc);

    UINT32 length{};
    DECODE_OR_RETURN(request, length);

    UINT64 offset{};
    DECODE_OR_RETURN(request, offset);

    BOOLEAN write_to_end{};
    DECODE_OR_RETURN(request, write_to_end);

    BOOLEAN constrained_io{};
    DECODE_OR_RETURN(request, constrained_io);

    auto *buffer = request->current_pointer();

    UINT32 bytes_transferred{0U};
    remote::file_info r_info{};
    ret = this->winfsp_write(file_desc, buffer, offset, length, write_to_end,
                             constrained_io, &bytes_transferred, &r_info);
    if (ret == STATUS_SUCCESS) {
      response.encode(bytes_transferred);
      response.encode(r_info);
    }
    return ret;
  }

  void
  message_handler(std::uint32_t service_flags, std::string client_id,
                  std::uint64_t thread_id, std::string method, packet *request,
                  packet &response,
                  packet_server::message_complete_callback message_complete) {
    auto idx{method.find_last_of("::")};
    auto lookup_method_name{
        ((idx == std::string::npos) ? "::" + method : method.substr(idx - 1)),
    };
    if (handler_lookup_.find(lookup_method_name) == handler_lookup_.end()) {
      message_complete(static_cast<packet::error_type>(STATUS_NOT_IMPLEMENTED));
      return;
    }

    client_pool_.execute(
        client_id, thread_id,
        [this, lookup_method_name, service_flags, client_id, thread_id, method,
         request, &response]() -> packet::error_type {
          return this->handler_lookup_.at(lookup_method_name)(
              service_flags, client_id, thread_id, method, request, response);
        },
        std::move(message_complete));
  }

protected:
  [[nodiscard]] auto construct_api_path(std::string path) -> std::string {
    return utils::path::create_api_path(path.substr(mount_location_.size()));
  }
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_DRIVES_REMOTE_REMOTE_SERVER_BASE_HPP_

/*handlerLookup_.insert({"::fuse_fallocate",
                       [this](std::uint32_t serviceFlags, const
   std::string &client_id, std::uint64_t threadId, const std::string
   &method, packet *request, packet &response) -> packet::error_type {
   auto ret{0};

                         std::string path;
                         DECODE_OR_RETURN(request, path);

                         std::int32_t mode;
                         DECODE_OR_RETURN(request, mode);

                         remote::file_offset offset;
                         DECODE_OR_RETURN(request, offset);

                         remote::file_offset length;
                         DECODE_OR_RETURN(request, length);

                         remote::file_handle handle;
                         DECODE_OR_RETURN(request, handle);

                         return this->fuse_fallocate(path.c_str(), mode,
   offset, length, handle);
                       }});*/
/*handlerLookup_.insert({"::fuse_getxattr",
                       [this](std::uint32_t serviceFlags, const
std::string &client_id, std::uint64_t threadId, const std::string
&method, packet *request, packet &response) -> packet::error_type { auto
ret{0};

                         std::string path;
                         DECODE_OR_RETURN(request, path);

                         std::string name;
                         DECODE_OR_RETURN(request, name);

                         remote::file_size size;
                         DECODE_OR_RETURN(request, size);

                         return this->fuse_getxattr(path.c_str(),
&name[0], nullptr, size);
                       }});
handlerLookup_.insert({"::fuse_getxattr_osx",
                       [this](std::uint32_t serviceFlags, const
std::string &client_id, std::uint64_t threadId, const std::string
&method, packet *request, packet &response) -> packet::error_type { auto
ret{0};

                         std::string path;
                         DECODE_OR_RETURN(request, path);

                         std::string name;
                         DECODE_OR_RETURN(request, name);

                         remote::file_size size;
                         DECODE_OR_RETURN(request, size);

                         std::uint32_t position;
                         DECODE_OR_RETURN(request, position);

                         return this->fuse_getxattr_osx(path.c_str(),
&name[0], nullptr, size, position);
                       }});*/
/*handlerLookup_.insert({"::remote_fuseListxattr",
                       [this](std::uint32_t serviceFlags, const
   std::string &client_id, std::uint64_t threadId, const std::string
   &method, packet *request, packet &response) -> packet::error_type {
   auto ret{0};

                         std::string path;
                         DECODE_OR_RETURN(request, path);

                         remote::file_size size;
                         DECODE_OR_RETURN(request, size);

                         return this->fuse_listxattr(path.c_str(),
   nullptr, size);
                       }});*/
/*handlerLookup_.insert({"::fuse_removexattr",
                       [this](std::uint32_t serviceFlags, const
   std::string &client_id, std::uint64_t threadId, const std::string
   &method, packet *request, packet &response) -> packet::error_type {
   auto ret{0};

                         std::string path;
                         DECODE_OR_RETURN(request, path);

                         std::string name;
                         DECODE_OR_RETURN(request, name);

                         return this->fuse_removexattr(path.c_str(),
   &name[0]);
                       }});*/
/*handlerLookup_.insert({"::fuse_setxattr",
                       [this](std::uint32_t serviceFlags, const
std::string &client_id, std::uint64_t threadId, const std::string
&method, packet *request, packet &response) -> packet::error_type { auto
ret{0};

                         std::string path;
                         DECODE_OR_RETURN(request, path);

                         std::string name;
                         DECODE_OR_RETURN(request, name);

                         remote::file_size size;
                         DECODE_OR_RETURN(request, size);

                         if (size >
std::numeric_limits<std::size_t>::max()) { return -ERANGE;
                         }

                         data_buffer
value(static_cast<std::size_t>(size)); ret = request->Decode(&value[0],
value.size()); if (ret == 0) { std::int32_t flags;
                           DECODE_OR_RETURN(request, flags);

                           ret = this->fuse_setxattr(path.c_str(),
&name[0], &value[0], size, flags);
                         }
                         return ret;
                       }});
handlerLookup_.insert({"::fuse_setxattr_osx",
                       [this](std::uint32_t serviceFlags, const
std::string &client_id, std::uint64_t threadId, const std::string
&method, packet *request, packet &response) -> packet::error_type { auto
ret{0};

                         std::string path;
                         DECODE_OR_RETURN(request, path);

                         std::string name;
                         DECODE_OR_RETURN(request, name);

                         remote::file_size size;
                         DECODE_OR_RETURN(request, size);

                         if (size >
std::numeric_limits<std::size_t>::max()) { return -ERANGE;
                         }

                         data_buffer
value(static_cast<std::size_t>(size)); ret = request->Decode(&value[0],
value.size()); if (ret == 0) { std::int32_t flags;
                           DECODE_OR_RETURN(request, flags);

                           std::uint32_t position;
                           DECODE_OR_RETURN(request, position);

                           ret = this->fuse_setxattr_osx(path.c_str(),
&name[0], &value[0], size, flags, position);
                         }
                         return ret;
                       }});*/
