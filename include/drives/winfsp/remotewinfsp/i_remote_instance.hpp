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
#ifndef INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_I_REMOTE_INSTANCE_HPP_
#define INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_I_REMOTE_INSTANCE_HPP_

#include "drives/remote/i_remote_json.hpp"

namespace repertory::remote_winfsp {
class i_remote_instance : public virtual i_remote_json {
  INTERFACE_SETUP(i_remote_instance);

public:
  virtual auto winfsp_can_delete(PVOID fileDesc, PWSTR fileName)
      -> packet::error_type = 0;

  virtual auto winfsp_cleanup(PVOID fileDesc, PWSTR fileName, UINT32 flags,
                              BOOLEAN &wasClosed) -> packet::error_type = 0;

  virtual auto winfsp_close(PVOID fileDesc) -> packet::error_type = 0;

  virtual auto winfsp_create(PWSTR fileName, UINT32 createOptions,
                             UINT32 grantedAccess, UINT32 fileAttributes,
                             UINT64 allocationSize, PVOID *fileDesc,
                             remote::file_info *fileInfo,
                             std::string &normalizedName, BOOLEAN &exists)
      -> packet::error_type = 0;

  virtual auto winfsp_flush(PVOID fileDesc, remote::file_info *fileInfo)
      -> packet::error_type = 0;

  virtual auto winfsp_get_dir_buffer(PVOID fileDesc, PVOID *&ptr)
      -> packet::error_type = 0;

  virtual auto winfsp_get_file_info(PVOID fileDesc, remote::file_info *fileInfo)
      -> packet::error_type = 0;

  virtual auto
  winfsp_get_security_by_name(PWSTR fileName, PUINT32 fileAttributes,
                              std::uint64_t *securityDescriptorSize,
                              std::wstring &strDescriptor)
      -> packet::error_type = 0;

  virtual auto winfsp_get_volume_info(UINT64 &totalSize, UINT64 &freeSize,
                                      std::string &volumeLabel)
      -> packet::error_type = 0;

  virtual auto winfsp_mounted(const std::wstring &location)
      -> packet::error_type = 0;

  virtual auto winfsp_open(PWSTR fileName, UINT32 createOptions,
                           UINT32 grantedAccess, PVOID *fileDesc,
                           remote::file_info *fileInfo,
                           std::string &normalizedName)
      -> packet::error_type = 0;

  virtual auto winfsp_overwrite(PVOID fileDesc, UINT32 fileAttributes,
                                BOOLEAN replaceFileAttributes,
                                UINT64 allocationSize,
                                remote::file_info *fileInfo)
      -> packet::error_type = 0;

  virtual auto winfsp_read(PVOID fileDesc, PVOID buffer, UINT64 offset,
                           UINT32 length, PUINT32 bytesTransferred)
      -> packet::error_type = 0;

  virtual auto winfsp_read_directory(PVOID fileDesc, PWSTR pattern,
                                     PWSTR marker, json &itemList)
      -> packet::error_type = 0;

  virtual auto winfsp_rename(PVOID fileDesc, PWSTR fileName, PWSTR newFileName,
                             BOOLEAN replaceIfExists) -> packet::error_type = 0;

  virtual auto winfsp_set_basic_info(PVOID fileDesc, UINT32 fileAttributes,
                                     UINT64 creationTime, UINT64 lastAccessTime,
                                     UINT64 lastWriteTime, UINT64 changeTime,
                                     remote::file_info *fileInfo)
      -> packet::error_type = 0;

  virtual auto winfsp_set_file_size(PVOID fileDesc, UINT64 newSize,
                                    BOOLEAN setAllocationSize,
                                    remote::file_info *fileInfo)
      -> packet::error_type = 0;

  virtual auto winfsp_unmounted(const std::wstring &location)
      -> packet::error_type = 0;

  virtual auto winfsp_write(PVOID fileDesc, PVOID buffer, UINT64 offset,
                            UINT32 length, BOOLEAN writeToEndOfFile,
                            BOOLEAN constrainedIo, PUINT32 bytesTransferred,
                            remote::file_info *fileInfo)
      -> packet::error_type = 0;
};

using remote_instance_factory =
    std::function<std::unique_ptr<i_remote_instance>()>;
} // namespace repertory::remote_winfsp

#endif // INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_I_REMOTE_INSTANCE_HPP_
