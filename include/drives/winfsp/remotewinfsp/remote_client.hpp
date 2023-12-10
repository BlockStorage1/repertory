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
#ifndef INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_REMOTE_CLIENT_HPP_
#define INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_REMOTE_CLIENT_HPP_

#include "comm/packet/packet.hpp"
#include "comm/packet/packet_client.hpp"
#include "drives/remote/remote_open_file_table.hpp"
#include "drives/winfsp/remotewinfsp/i_remote_instance.hpp"

namespace repertory {
class app_config;
namespace remote_winfsp {
class remote_client final : public remote_open_file_table,
                            public virtual i_remote_instance {
public:
  explicit remote_client(const app_config &config);

private:
  const app_config &config_;
  packet_client packet_client_;

private:
#ifdef _WIN32
#define to_handle(x) (x)
#else
  static auto to_handle(PVOID file_desc) -> native_handle;
#endif

protected:
  void delete_open_directory(void * /*dir*/) override {}

public:
  auto json_create_directory_snapshot(const std::string &path, json &json_data)
      -> packet::error_type override;

  auto json_read_directory_snapshot(const std::string &path,
                                    const remote::file_handle &handle,
                                    std::uint32_t page, json &json_data)
      -> packet::error_type override;

  auto json_release_directory_snapshot(const std::string &path,
                                       const remote::file_handle &handle)
      -> packet::error_type override;

  auto winfsp_can_delete(PVOID file_desc, PWSTR file_name)
      -> packet::error_type override;

  auto winfsp_cleanup(PVOID file_desc, PWSTR file_name, UINT32 flags,
                      BOOLEAN &was_closed) -> packet::error_type override;

  auto winfsp_close(PVOID file_desc) -> packet::error_type override;

  auto winfsp_create(PWSTR file_name, UINT32 create_options,
                     UINT32 granted_access, UINT32 attributes,
                     UINT64 allocation_size, PVOID *file_desc,
                     remote::file_info *file_info, std::string &normalized_name,
                     BOOLEAN &exists) -> packet::error_type override;

  auto winfsp_flush(PVOID file_desc, remote::file_info *file_info)
      -> packet::error_type override;

  auto winfsp_get_dir_buffer(PVOID file_desc, PVOID *&ptr)
      -> packet::error_type override;

  auto winfsp_get_file_info(PVOID file_desc, remote::file_info *file_info)
      -> packet::error_type override;

  auto winfsp_get_security_by_name(PWSTR file_name, PUINT32 attributes,
                                   std::uint64_t *descriptor_size,
                                   std::wstring &string_descriptor)
      -> packet::error_type override;

  auto winfsp_get_volume_info(UINT64 &total_size, UINT64 &free_size,
                              std::string &volume_label)
      -> packet::error_type override;

  auto winfsp_mounted(const std::wstring &location)
      -> packet::error_type override;

  auto winfsp_open(PWSTR file_name, UINT32 create_options,
                   UINT32 granted_access, PVOID *file_desc,
                   remote::file_info *file_info, std::string &normalized_name)
      -> packet::error_type override;

  auto winfsp_overwrite(PVOID file_desc, UINT32 attributes,
                        BOOLEAN replace_attributes, UINT64 allocation_size,
                        remote::file_info *file_info)
      -> packet::error_type override;

  auto winfsp_read(PVOID file_desc, PVOID buffer, UINT64 offset, UINT32 length,
                   PUINT32 bytes_transferred) -> packet::error_type override;

  auto winfsp_read_directory(PVOID file_desc, PWSTR pattern, PWSTR marker,
                             json &itemList) -> packet::error_type override;

  auto winfsp_rename(PVOID file_desc, PWSTR file_name, PWSTR new_file_name,
                     BOOLEAN replace_if_exists) -> packet::error_type override;

  auto winfsp_set_basic_info(PVOID file_desc, UINT32 attributes,
                             UINT64 creation_time, UINT64 last_access_time,
                             UINT64 last_write_time, UINT64 change_time,
                             remote::file_info *file_info)
      -> packet::error_type override;

  auto winfsp_set_file_size(PVOID file_desc, UINT64 new_size,
                            BOOLEAN set_allocation_size,
                            remote::file_info *file_info)
      -> packet::error_type override;

  auto winfsp_unmounted(const std::wstring &location)
      -> packet::error_type override;

  auto winfsp_write(PVOID file_desc, PVOID buffer, UINT64 offset, UINT32 length,
                    BOOLEAN write_to_end, BOOLEAN constrained_io,
                    PUINT32 bytes_transferred, remote::file_info *file_info)
      -> packet::error_type override;
};
} // namespace remote_winfsp
} // namespace repertory

#endif // INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_REMOTE_CLIENT_HPP_
