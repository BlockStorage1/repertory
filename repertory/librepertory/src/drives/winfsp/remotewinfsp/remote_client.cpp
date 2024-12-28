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
#include "drives/winfsp/remotewinfsp/remote_client.hpp"

#include "app_config.hpp"
#include "drives/winfsp/remotewinfsp/i_remote_instance.hpp"
#include "events/event_system.hpp"
#include "events/events.hpp"
#include "types/repertory.hpp"
#include "utils/path.hpp"
#include "version.hpp"

namespace repertory::remote_winfsp {
#define RAISE_REMOTE_WINFSP_CLIENT_EVENT(func, file, ret)                      \
  if (config_.get_enable_drive_events() &&                                     \
      (((config_.get_event_level() >= remote_winfsp_client_event::level) &&    \
        (ret != STATUS_SUCCESS)) ||                                            \
       (config_.get_event_level() >= event_level::trace)))                     \
  event_system::instance().raise<remote_winfsp_client_event>(                  \
      std::string{func}, file, ret)

// clang-format off
E_SIMPLE3(remote_winfsp_client_event, debug, true,
  std::string, function, func, E_FROM_STRING,
  std::string, api_path, ap, E_FROM_STRING,
  packet::error_type, result, res, E_FROM_INT32
);
// clang-format on

remote_client::remote_client(const app_config &config)
    : config_(config), packet_client_(config.get_remote_config()) {}

auto remote_client::winfsp_can_delete(PVOID file_desc, PWSTR file_name)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(file_desc);
  request.encode(file_name);

  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, service_flags),
  };

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(
      function_name,
      utils::path::create_api_path(utils::string::to_utf8(file_name)), ret);
  return ret;
}

auto remote_client::json_create_directory_snapshot(const std::string &path,
                                                   json &json_data)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(path);

  packet response;
  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };
  if (ret == 0) {
    ret = packet::decode_json(response, json_data);
  }

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(function_name, path, ret);
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
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };
  if (ret == 0) {
    ret = packet::decode_json(response, json_data);
  }

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(function_name, path, ret);
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
  auto ret{
      packet_client_.send(function_name, request, service_flags),
  };

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(function_name, path, ret);
  return ret;
}

auto remote_client::winfsp_cleanup(PVOID file_desc, PWSTR file_name,
                                   UINT32 flags, BOOLEAN &was_deleted)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto handle{
      to_handle(file_desc),
  };

  auto file_path{
      get_open_file_path(handle),
  };

  was_deleted = 0U;

  packet request;
  request.encode(file_desc);
  request.encode(file_name);
  request.encode(flags);

  packet response;
  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };

  DECODE_OR_IGNORE(&response, was_deleted);
  if (was_deleted != 0U) {
    remove_all(file_path);
  }

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(function_name, file_path, ret);
  return ret;
}

auto remote_client::winfsp_close(PVOID file_desc) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto handle{
      to_handle(file_desc),
  };
  if (has_open_info(handle, STATUS_INVALID_HANDLE) == STATUS_SUCCESS) {
    auto file_path{
        get_open_file_path(handle),
    };

    packet request;
    request.encode(file_desc);

    std::uint32_t service_flags{};
    auto ret{
        packet_client_.send(function_name, request, service_flags),
    };
    if ((ret == STATUS_SUCCESS) ||
        (ret == static_cast<packet::error_type>(STATUS_INVALID_HANDLE))) {
      remove_open_info(handle);
      RAISE_REMOTE_WINFSP_CLIENT_EVENT(function_name, file_path, ret);
    }
  }

  return 0;
}

auto remote_client::winfsp_create(PWSTR file_name, UINT32 create_options,
                                  UINT32 granted_access, UINT32 attributes,
                                  UINT64 allocation_size, PVOID *file_desc,
                                  remote::file_info *file_info,
                                  std::string &normalized_name, BOOLEAN &exists)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(file_name);
  request.encode(create_options);
  request.encode(granted_access);
  request.encode(attributes);
  request.encode(allocation_size);

  packet response;
  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };
  if (ret == STATUS_SUCCESS) {
    HANDLE handle{};
    DECODE_OR_IGNORE(&response, handle);
    DECODE_OR_IGNORE(&response, *file_info);
    DECODE_OR_IGNORE(&response, normalized_name);
    DECODE_OR_IGNORE(&response, exists);

    if (ret == STATUS_SUCCESS) {
      *file_desc = reinterpret_cast<PVOID>(handle);
      set_open_info(to_handle(*file_desc),
                    open_info{
                        "",
                        nullptr,
                        {},
                        utils::string::to_utf8(file_name),
                    });
#if defined(_WIN32)
      if (exists) {
        ::SetLastError(ERROR_ALREADY_EXISTS);
      }
#endif
    }
  }

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(
      function_name, get_open_file_path(to_handle(*file_desc)), ret);
  return ret;
}

auto remote_client::winfsp_flush(PVOID file_desc, remote::file_info *file_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(file_desc);

  packet response;
  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };
  DECODE_OR_IGNORE(&response, *file_info);

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(
      function_name, get_open_file_path(to_handle(file_desc)), ret);
  return ret;
}

auto remote_client::winfsp_get_dir_buffer([[maybe_unused]] PVOID file_desc,
                                          [[maybe_unused]] PVOID *&ptr)
    -> packet::error_type {
#if defined(_WIN32)
  if (get_directory_buffer(reinterpret_cast<native_handle>(file_desc), ptr)) {
    return static_cast<packet::error_type>(STATUS_SUCCESS);
  }
#endif
  return static_cast<packet::error_type>(STATUS_INVALID_HANDLE);
}

auto remote_client::winfsp_get_file_info(PVOID file_desc,
                                         remote::file_info *file_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(file_desc);

  packet response;
  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };
  DECODE_OR_IGNORE(&response, *file_info);

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(
      function_name, get_open_file_path(to_handle(file_desc)), ret);
  return ret;
}

auto remote_client::winfsp_get_security_by_name(PWSTR file_name,
                                                PUINT32 attributes,
                                                std::uint64_t *descriptor_size,
                                                std::wstring &string_descriptor)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(file_name);
  request.encode(static_cast<std::uint64_t>(
      descriptor_size == nullptr ? 0 : *descriptor_size));
  request.encode(static_cast<std::uint8_t>(attributes != nullptr));

  packet response;
  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };

  string_descriptor.clear();
  DECODE_OR_IGNORE(&response, string_descriptor);
  if (string_descriptor.empty()) {
    string_descriptor = L"O:BAG:BAD:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;FA;;;WD)";
  }

  if (attributes != nullptr) {
    DECODE_OR_IGNORE(&response, *attributes);
  }

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(function_name,
                                   utils::string::to_utf8(file_name), ret);
  return ret;
}

auto remote_client::winfsp_get_volume_info(UINT64 &total_size,
                                           UINT64 &free_size,
                                           std::string &volume_label)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  packet response;
  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };
  DECODE_OR_IGNORE(&response, total_size);
  DECODE_OR_IGNORE(&response, free_size);
  DECODE_OR_IGNORE(&response, volume_label);

  return ret;
}

auto remote_client::winfsp_mounted(const std::wstring &location)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(std::string{project_get_version()});
  request.encode(location);

  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, service_flags),
  };

  auto mount_location{
      utils::string::to_utf8(location),
  };
  event_system::instance().raise<drive_mounted>(mount_location);

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(function_name, mount_location, ret);
  return ret;
}

auto remote_client::winfsp_open(PWSTR file_name, UINT32 create_options,
                                UINT32 granted_access, PVOID *file_desc,
                                remote::file_info *file_info,
                                std::string &normalized_name)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(file_name);
  request.encode(create_options);
  request.encode(granted_access);

  packet response;
  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };
  if (ret == STATUS_SUCCESS) {
    HANDLE handle{};
    DECODE_OR_IGNORE(&response, handle);
    DECODE_OR_IGNORE(&response, *file_info);
    DECODE_OR_IGNORE(&response, normalized_name);

    if (ret == STATUS_SUCCESS) {
      *file_desc = reinterpret_cast<PVOID>(handle);
      set_open_info(to_handle(*file_desc),
                    open_info{
                        "",
                        nullptr,
                        {},
                        utils::string::to_utf8(file_name),
                    });
    }
  }

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(
      function_name, get_open_file_path(to_handle(*file_desc)), ret);
  return ret;
}

auto remote_client::winfsp_overwrite(PVOID file_desc, UINT32 attributes,
                                     BOOLEAN replace_attributes,
                                     UINT64 allocation_size,
                                     remote::file_info *file_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(file_desc);
  request.encode(attributes);
  request.encode(replace_attributes);
  request.encode(allocation_size);

  packet response;
  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };
  DECODE_OR_IGNORE(&response, *file_info);

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(
      function_name, get_open_file_path(to_handle(file_desc)), ret);
  return ret;
}

auto remote_client::winfsp_read(PVOID file_desc, PVOID buffer, UINT64 offset,
                                UINT32 length, PUINT32 bytes_transferred)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(file_desc);
  request.encode(offset);
  request.encode(length);

  packet response;
  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };
  DECODE_OR_IGNORE(&response, *bytes_transferred);
  if (ret == STATUS_SUCCESS) {
    ret = response.decode(buffer, *bytes_transferred);
#if defined(_WIN32)
    if ((ret == STATUS_SUCCESS) &&
        (not*bytes_transferred || (*bytes_transferred != length))) {
      ::SetLastError(ERROR_HANDLE_EOF);
    }
#endif
  }

  if (ret != STATUS_SUCCESS) {
    RAISE_REMOTE_WINFSP_CLIENT_EVENT(
        function_name, get_open_file_path(to_handle(file_desc)), ret);
  }

  return ret;
}

auto remote_client::winfsp_read_directory(PVOID file_desc, PWSTR pattern,
                                          PWSTR marker, json &item_list)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(file_desc);
  request.encode(pattern);
  request.encode(marker);

  packet response;
  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };
  if (ret == STATUS_SUCCESS) {
    ret = packet::decode_json(response, item_list);
  }

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(
      function_name, get_open_file_path(to_handle(file_desc)), ret);
  return ret;
}

auto remote_client::winfsp_rename(PVOID file_desc, PWSTR file_name,
                                  PWSTR new_file_name,
                                  BOOLEAN replace_if_exists)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(file_desc);
  request.encode(file_name);
  request.encode(new_file_name);
  request.encode(replace_if_exists);

  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, service_flags),
  };

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(
      function_name,
      utils::path::create_api_path(utils::string::to_utf8(file_name)) + "|" +
          utils::path::create_api_path(utils::string::to_utf8(new_file_name)),
      ret);
  return ret;
}

auto remote_client::winfsp_set_basic_info(
    PVOID file_desc, UINT32 attributes, UINT64 creation_time,
    UINT64 last_access_time, UINT64 last_write_time, UINT64 change_time,
    remote::file_info *file_info) -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(file_desc);
  request.encode(attributes);
  request.encode(creation_time);
  request.encode(last_access_time);
  request.encode(last_write_time);
  request.encode(change_time);

  packet response;
  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };
  DECODE_OR_IGNORE(&response, *file_info);

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(
      function_name, get_open_file_path(to_handle(file_desc)), ret);
  return ret;
}

auto remote_client::winfsp_set_file_size(PVOID file_desc, UINT64 new_size,
                                         BOOLEAN set_allocation_size,
                                         remote::file_info *file_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(file_desc);
  request.encode(new_size);
  request.encode(set_allocation_size);

  packet response;
  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };
  DECODE_OR_IGNORE(&response, *file_info);

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(
      function_name, get_open_file_path(to_handle(file_desc)), ret);
  return ret;
}

auto remote_client::winfsp_unmounted(const std::wstring &location)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto mount_location{
      utils::string::to_utf8(location),
  };
  event_system::instance().raise<drive_unmount_pending>(mount_location);
  packet request;
  request.encode(location);

  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, service_flags),
  };
  event_system::instance().raise<drive_unmounted>(mount_location);

  RAISE_REMOTE_WINFSP_CLIENT_EVENT(function_name, mount_location, ret);
  return ret;
}

auto remote_client::winfsp_write(PVOID file_desc, PVOID buffer, UINT64 offset,
                                 UINT32 length, BOOLEAN write_to_end,
                                 BOOLEAN constrained_io,
                                 PUINT32 bytes_transferred,
                                 remote::file_info *file_info)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  packet request;
  request.encode(file_desc);
  request.encode(length);
  request.encode(offset);
  request.encode(write_to_end);
  request.encode(constrained_io);
  if (length != 0U) {
    request.encode(buffer, length);
  }

  packet response;
  std::uint32_t service_flags{};
  auto ret{
      packet_client_.send(function_name, request, response, service_flags),
  };
  DECODE_OR_IGNORE(&response, *bytes_transferred);
  DECODE_OR_IGNORE(&response, *file_info);

  if (ret != STATUS_SUCCESS) {
    RAISE_REMOTE_WINFSP_CLIENT_EVENT(
        function_name, get_open_file_path(to_handle(file_desc)), ret);
  }
  return ret;
}

#if !defined(_WIN32)
auto remote_client::to_handle(PVOID file_desc) -> native_handle {
  return static_cast<native_handle>(reinterpret_cast<std::uint64_t>(file_desc));
}
#endif // !defined(_WIN32)
} // namespace repertory::remote_winfsp
