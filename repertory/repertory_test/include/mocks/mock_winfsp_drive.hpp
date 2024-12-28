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
#ifndef REPERTORY_TEST_INCLUDE_MOCKS_MOCK_WINFSP_DRIVE_HPP_
#define REPERTORY_TEST_INCLUDE_MOCKS_MOCK_WINFSP_DRIVE_HPP_
#if defined(_WIN32)

#include "test_common.hpp"

#include "drives/winfsp/i_winfsp_drive.hpp"
#include "utils/common.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"
#include "utils/time.hpp"
#include "utils/utils.hpp"

namespace repertory {
class mock_winfsp_drive final : public virtual i_winfsp_drive {
public:
  explicit mock_winfsp_drive(std::string mount_location)
      : mount_location_(std::move(mount_location)) {}

private:
  std::string mount_location_;

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
    di.size = 0U;
    di.meta = {{META_ATTRIBUTES, "16"},
               {META_MODIFIED, std::to_string(utils::time::get_time_now())},
               {META_WRITTEN, std::to_string(utils::time::get_time_now())},
               {META_ACCESSED, std::to_string(utils::time::get_time_now())},
               {META_CREATION, std::to_string(utils::time::get_time_now())}};
    list.emplace_back(di);

    di.api_path = "..";
    list.emplace_back(di);

    return list;
  }

  [[nodiscard]] auto get_file_size(const std::string & /*api_path*/) const
      -> std::uint64_t override {
    return 0;
  }

  auto get_item_meta(const std::string & /*api_path*/,
                     api_meta_map & /* meta */) const -> api_error override {
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

    if (attributes != nullptr) {
      *attributes = FILE_ATTRIBUTE_NORMAL;
    }

    if (descriptor_size) {
      ULONG sz{0U};
      PSECURITY_DESCRIPTOR sd{nullptr};
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
    return 100ULL * 1024ULL * 1024ULL;
  }

  [[nodiscard]] auto get_total_item_count() const -> std::uint64_t override {
    return 0U;
  }

  [[nodiscard]] auto get_used_drive_space() const -> std::uint64_t override {
    return 0U;
  }

  void get_volume_info(UINT64 &total_size, UINT64 &free_size,
                       std::string &volume_label) const override {
    free_size = 100U;
    total_size = 200U;
    volume_label = "TestVolumeLabel";
  }

  auto populate_file_info(const std::string &api_path,
                          remote::file_info &file_info) const
      -> api_error override {
    auto file_path = utils::path::combine(mount_location_, {api_path});
    auto directory = utils::file::directory(file_path).exists();
    auto attributes =
        FILE_FLAG_BACKUP_SEMANTICS |
        (directory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_ARCHIVE);
    auto share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    auto handle = ::CreateFileA(
        file_path.c_str(), GENERIC_READ, static_cast<DWORD>(share_mode),
        nullptr, OPEN_EXISTING, static_cast<DWORD>(attributes), nullptr);
    FILE_BASIC_INFO basic_info{};
    ::GetFileInformationByHandleEx(handle, FileBasicInfo, &basic_info,
                                   sizeof(basic_info));
    if (not directory) {
      auto opt_size = utils::file::file{file_path}.size();
      if (not opt_size.has_value()) {
        return api_error::os_error;
      }

      file_info.FileSize = opt_size.value();
    }

    file_info.AllocationSize =
        directory ? 0
                  : utils::divide_with_ceiling(file_info.FileSize,
                                               WINFSP_ALLOCATION_UNIT) *
                        WINFSP_ALLOCATION_UNIT;
    file_info.FileAttributes = basic_info.FileAttributes;
    file_info.ChangeTime = static_cast<UINT64>(basic_info.ChangeTime.QuadPart);
    file_info.CreationTime =
        static_cast<UINT64>(basic_info.CreationTime.QuadPart);
    file_info.LastAccessTime =
        static_cast<UINT64>(basic_info.LastAccessTime.QuadPart);
    file_info.LastWriteTime =
        static_cast<UINT64>(basic_info.LastWriteTime.QuadPart);
    ::CloseHandle(handle);
    return api_error::success;
  }
};
} // namespace repertory

#endif // _WIN32
#endif // REPERTORY_TEST_INCLUDE_MOCKS_MOCK_WINFSP_DRIVE_HPP_
