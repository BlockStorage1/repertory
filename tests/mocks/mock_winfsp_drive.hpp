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
#ifndef TESTS_MOCKS_MOCK_WINFSP_DRIVE_HPP_
#define TESTS_MOCKS_MOCK_WINFSP_DRIVE_HPP_
#ifdef _WIN32

#include "test_common.hpp"

#include "drives/winfsp/i_winfsp_drive.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
class mock_winfsp_drive final : public virtual i_winfsp_drive {
public:
  explicit mock_winfsp_drive(std::string mount_location)
      : mount_location_(std::move(mount_location)) {}

private:
  const std::string mount_location_;

public:
  [[nodiscard]] auto
  get_directory_item_count(const std::string & /*api_path*/) const
      -> std::uint64_t override {
    return 1;
  }

  [[nodiscard]] auto get_directory_items(const std::string & /*api_path*/) const
      -> directory_item_list override {
    directory_item_list list{};

    directory_item di{};
    di.api_path = ".";
    di.directory = true;
    di.size = 0u;
    di.meta = {{META_ATTRIBUTES, "16"},
               {META_MODIFIED, std::to_string(utils::get_file_time_now())},
               {META_WRITTEN, std::to_string(utils::get_file_time_now())},
               {META_ACCESSED, std::to_string(utils::get_file_time_now())},
               {META_CREATION, std::to_string(utils::get_file_time_now())}};
    list.emplace_back(di);

    di.api_path = "..";
    list.emplace_back(di);

    return list;
  }

  [[nodiscard]] auto get_file_size(const std::string & /*api_path*/) const
      -> std::uint64_t override {
    return 0;
  }

  auto get_item_meta(const std::string & /*api_path*/, api_meta_map &meta) const
      -> api_error override {
    return api_error::error;
  }

  auto get_item_meta(const std::string & /*api_path*/,
                     const std::string & /*name*/,
                     std::string & /*value*/) const -> api_error override {
    return api_error::error;
  }

  auto get_security_by_name(PWSTR /*file_name*/, PUINT32 attributes,
                            PSECURITY_DESCRIPTOR descriptor,
                            std::uint64_t *descriptor_size)
      -> NTSTATUS override {
    auto ret = STATUS_SUCCESS;

    if (attributes) {
      *attributes = FILE_ATTRIBUTE_NORMAL;
    }

    if (descriptor_size) {
      ULONG sz = 0;
      PSECURITY_DESCRIPTOR sd = nullptr;
      if (::ConvertStringSecurityDescriptorToSecurityDescriptor(
              "O:BAG:BAD:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;FA;;;WD)",
              SDDL_REVISION_1, &sd, &sz)) {
        if (sz > *descriptor_size) {
          ret = STATUS_BUFFER_TOO_SMALL;
        } else {
          ::CopyMemory(descriptor, sd, sz);
        }
        *descriptor_size = sz;
        ::LocalFree(sd);
      } else {
        ret = FspNtStatusFromWin32(::GetLastError());
      }
    }

    return ret;
  }

  [[nodiscard]] auto get_total_drive_space() const -> std::uint64_t override {
    return 100 * 1024 * 1024;
  }

  [[nodiscard]] auto get_total_item_count() const -> std::uint64_t override {
    return 0;
  }

  [[nodiscard]] auto get_used_drive_space() const -> std::uint64_t override {
    return 0;
  }

  void get_volume_info(UINT64 &total_size, UINT64 &free_size,
                       std::string &volume_label) const override {
    free_size = 100;
    total_size = 200;
    volume_label = "TestVolumeLabel";
  }

  auto populate_file_info(const std::string &api_path,
                          remote::file_info &file_info) -> api_error override {
    const auto file_path = utils::path::combine(mount_location_, {api_path});
    const auto directory = utils::file::is_directory(file_path);
    const auto attributes =
        FILE_FLAG_BACKUP_SEMANTICS |
        (directory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL);
    const auto share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    auto handle = ::CreateFileA(&file_path[0], GENERIC_READ, share_mode,
                                nullptr, OPEN_EXISTING, attributes, nullptr);
    FILE_BASIC_INFO fi{};
    ::GetFileInformationByHandleEx(handle, FileBasicInfo, &fi, sizeof(fi));
    if (not directory) {
      utils::file::get_file_size(file_path, file_info.FileSize);
    }
    file_info.AllocationSize =
        directory ? 0
                  : utils::divide_with_ceiling(file_info.FileSize,
                                               WINFSP_ALLOCATION_UNIT) *
                        WINFSP_ALLOCATION_UNIT;
    file_info.FileAttributes = fi.FileAttributes;
    file_info.ChangeTime = fi.ChangeTime.QuadPart;
    file_info.CreationTime = fi.CreationTime.QuadPart;
    file_info.LastAccessTime = fi.LastAccessTime.QuadPart;
    file_info.LastWriteTime = fi.LastWriteTime.QuadPart;
    ::CloseHandle(handle);
    return api_error::success;
  }
};
} // namespace repertory

#endif // _WIN32
#endif // TESTS_MOCKS_MOCK_WINFSP_DRIVE_HPP_
