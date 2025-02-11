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
#ifndef REPERTORY_INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_I_REMOTE_INSTANCE_HPP_
#define REPERTORY_INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_I_REMOTE_INSTANCE_HPP_

#include "drives/remote/i_remote_json.hpp"

namespace repertory::remote_winfsp {
class i_remote_instance : public virtual i_remote_json {
  INTERFACE_SETUP(i_remote_instance);

public:
  virtual auto winfsp_can_delete(PVOID file_desc, PWSTR file_name)
      -> packet::error_type = 0;

  virtual auto winfsp_cleanup(PVOID file_desc, PWSTR file_name, UINT32 flags,
                              BOOLEAN &was_deleted) -> packet::error_type = 0;

  virtual auto winfsp_close(PVOID file_desc) -> packet::error_type = 0;

  virtual auto winfsp_create(PWSTR file_name, UINT32 create_options,
                             UINT32 granted_access, UINT32 file_attributes,
                             UINT64 allocation_size, PVOID *file_desc,
                             remote::file_info *file_info,
                             std::string &normalized_name, BOOLEAN &exists)
      -> packet::error_type = 0;

  virtual auto winfsp_flush(PVOID file_desc, remote::file_info *file_info)
      -> packet::error_type = 0;

  virtual auto winfsp_get_dir_buffer(PVOID file_desc, PVOID *&ptr)
      -> packet::error_type = 0;

  virtual auto winfsp_get_file_info(PVOID file_desc,
                                    remote::file_info *file_info)
      -> packet::error_type = 0;

  virtual auto
  winfsp_get_security_by_name(PWSTR file_name, PUINT32 file_attributes,
                              std::uint64_t *security_descriptor_size,
                              std::wstring &str_descriptor)
      -> packet::error_type = 0;

  virtual auto winfsp_get_volume_info(UINT64 &total_size, UINT64 &free_size,
                                      std::string &volume_label)
      -> packet::error_type = 0;

  virtual auto winfsp_mounted(const std::wstring &location)
      -> packet::error_type = 0;

  virtual auto winfsp_open(PWSTR file_name, UINT32 create_options,
                           UINT32 granted_access, PVOID *file_desc,
                           remote::file_info *file_info,
                           std::string &normalized_name)
      -> packet::error_type = 0;

  virtual auto winfsp_overwrite(PVOID file_desc, UINT32 file_attributes,
                                BOOLEAN replace_file_attributes,
                                UINT64 allocation_size,
                                remote::file_info *file_info)
      -> packet::error_type = 0;

  virtual auto winfsp_read(PVOID file_desc, PVOID buffer, UINT64 offset,
                           UINT32 length, PUINT32 bytes_transferred)
      -> packet::error_type = 0;

  virtual auto winfsp_read_directory(PVOID file_desc, PWSTR pattern,
                                     PWSTR marker, json &itemList)
      -> packet::error_type = 0;

  virtual auto winfsp_rename(PVOID file_desc, PWSTR file_name,
                             PWSTR new_file_name, BOOLEAN replace_if_exists)
      -> packet::error_type = 0;

  virtual auto winfsp_set_basic_info(PVOID file_desc, UINT32 file_attributes,
                                     UINT64 creation_time,
                                     UINT64 last_access_time,
                                     UINT64 last_write_time, UINT64 change_time,
                                     remote::file_info *file_info)
      -> packet::error_type = 0;

  virtual auto winfsp_set_file_size(PVOID file_desc, UINT64 new_size,
                                    BOOLEAN set_allocation_size,
                                    remote::file_info *file_info)
      -> packet::error_type = 0;

  virtual auto winfsp_unmounted(const std::wstring &location)
      -> packet::error_type = 0;

  virtual auto winfsp_write(PVOID file_desc, PVOID buffer, UINT64 offset,
                            UINT32 length, BOOLEAN write_to_end,
                            BOOLEAN constrained_io, PUINT32 bytes_transferred,
                            remote::file_info *file_info)
      -> packet::error_type = 0;
};

using remote_instance_factory =
    std::function<std::unique_ptr<i_remote_instance>()>;
} // namespace repertory::remote_winfsp

#endif // REPERTORY_INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_I_REMOTE_INSTANCE_HPP_
