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
#if defined(_WIN32)

#include "drives/winfsp/remotewinfsp/remote_winfsp_drive.hpp"

#include "app_config.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/consumers/logging_consumer.hpp"
#include "events/event_system.hpp"
#include "events/types/drive_mount_failed.hpp"
#include "events/types/drive_mount_result.hpp"
#include "events/types/unmount_requested.hpp"
#include "platform/platform.hpp"
#include "rpc/server/server.hpp"
#include "utils/collection.hpp"
#include "utils/common.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"
#include "utils/time.hpp"
#include "utils/utils.hpp"

namespace repertory::remote_winfsp {
remote_winfsp_drive::winfsp_service::winfsp_service(
    lock_data &lock, remote_winfsp_drive &drive,
    std::vector<std::string> drive_args, app_config &config)
    : Service(std::wstring{REPERTORY_W}.data()),
      config_(config),
      drive_(drive),
      drive_args_(std::move(drive_args)),
      host_(drive),
      lock_(lock) {}

auto remote_winfsp_drive::winfsp_service::OnStart(ULONG, PWSTR *) -> NTSTATUS {
  REPERTORY_USES_FUNCTION_NAME();

  auto mount_location = utils::string::to_lower(utils::path::absolute(
      (drive_args_.size() > 1U) ? drive_args_.at(1U) : ""));
  auto drive_letter =
      ((mount_location.size() == 2U) ||
       ((mount_location.size() == 3U) && (mount_location.at(2U) == '\\'))) &&
      (mount_location.at(1U) == ':');

  auto ret = drive_letter ? STATUS_DEVICE_BUSY : STATUS_NOT_SUPPORTED;
  if ((drive_letter && not utils::file::directory(mount_location).exists())) {
    auto unicode_mount_location = utils::string::from_utf8(mount_location);
    host_.SetFileSystemName(&unicode_mount_location[0U]);
    if (config_.get_enable_mount_manager()) {
      unicode_mount_location =
          std::wstring(L"\\\\.\\") + unicode_mount_location[0U] + L":";
    }
    ret = host_.Mount(&unicode_mount_location[0U]);
  } else {
    std::cerr << (drive_letter ? "Mount location in use: "
                               : "Mount location not supported: ")
              << mount_location << std::endl;
  }

  if (ret != STATUS_SUCCESS) {
    event_system::instance().raise<drive_mount_failed>(ret, function_name,
                                                       mount_location);
    if (not lock_.set_mount_state(false, "", -1)) {
      utils::error::raise_error(function_name, "failed to set mount state");
    }
  }

  return ret;
}

auto remote_winfsp_drive::winfsp_service::OnStop() -> NTSTATUS {
  REPERTORY_USES_FUNCTION_NAME();

  host_.Unmount();
  if (not lock_.set_mount_state(false, "", -1)) {
    utils::error::raise_error(function_name, "failed to set mount state");
  }

  return STATUS_SUCCESS;
}

remote_winfsp_drive::remote_winfsp_drive(app_config &config,
                                         remote_instance_factory factory,
                                         lock_data &lock)
    : FileSystemBase(),
      config_(config),
      lock_(lock),
      factory_(std::move(factory)) {
  E_SUBSCRIBE(unmount_requested, [this](const unmount_requested &) {
    std::thread([this]() { this->shutdown(); }).detach();
  });
}

auto remote_winfsp_drive::CanDelete(PVOID /*file_node*/, PVOID file_desc,
                                    PWSTR file_name) -> NTSTATUS {
  return remote_instance_->winfsp_can_delete(file_desc, file_name);
}

VOID remote_winfsp_drive::Cleanup(PVOID /*file_node*/, PVOID file_desc,
                                  PWSTR file_name, ULONG flags) {
  BOOLEAN was_deleted{};
  remote_instance_->winfsp_cleanup(file_desc, file_name, flags, was_deleted);
}

VOID remote_winfsp_drive::Close(PVOID /*file_node*/, PVOID file_desc) {
  remote_instance_->winfsp_close(file_desc);
}

auto remote_winfsp_drive::Create(PWSTR file_name, UINT32 create_options,
                                 UINT32 granted_access, UINT32 attributes,
                                 PSECURITY_DESCRIPTOR /*descriptor*/,
                                 UINT64 allocation_size, PVOID * /*file_node*/,
                                 PVOID *file_desc, OpenFileInfo *ofi)
    -> NTSTATUS {
  remote::file_info f_info{};
  std::string normalized_name;
  BOOLEAN exists{0};
  auto ret = remote_instance_->winfsp_create(
      file_name, create_options, granted_access, attributes, allocation_size,
      file_desc, &f_info, normalized_name, exists);
  if (ret == STATUS_SUCCESS) {
    set_file_info(ofi->FileInfo, f_info);
    auto file_path = utils::string::from_utf8(normalized_name);
    wcsncpy(ofi->NormalizedName, file_path.data(), wcslen(file_path.c_str()));
    ofi->NormalizedNameSize =
        static_cast<UINT16>(wcslen(file_path.c_str()) * sizeof(WCHAR));
    if (exists != 0U) {
      ret = STATUS_OBJECT_NAME_COLLISION;
    }
  }

  return ret;
}

auto remote_winfsp_drive::Flush(PVOID /*file_node*/, PVOID file_desc,
                                FileInfo *file_info) -> NTSTATUS {
  remote::file_info fi{};
  auto ret = remote_instance_->winfsp_flush(file_desc, &fi);
  set_file_info(*file_info, fi);
  return ret;
}

auto remote_winfsp_drive::GetFileInfo(PVOID /*file_node*/, PVOID file_desc,
                                      FileInfo *file_info) -> NTSTATUS {
  remote::file_info fi{};
  auto ret = remote_instance_->winfsp_get_file_info(file_desc, &fi);
  set_file_info(*file_info, fi);
  return ret;
}

auto remote_winfsp_drive::GetSecurityByName(PWSTR file_name, PUINT32 attributes,
                                            PSECURITY_DESCRIPTOR descriptor,
                                            SIZE_T *descriptor_size)
    -> NTSTATUS {
  std::uint64_t sds{
      (descriptor_size == nullptr) ? 0U : *descriptor_size,
  };

  std::wstring string_descriptor;
  auto ret{
      remote_instance_->winfsp_get_security_by_name(
          file_name, attributes, descriptor_size ? &sds : nullptr,
          string_descriptor),
  };

  if ((ret == STATUS_SUCCESS) && (descriptor_size != nullptr)) {
    *descriptor_size = static_cast<SIZE_T>(sds);

    PSECURITY_DESCRIPTOR desc{nullptr};
    ULONG size{0U};
    if (::ConvertStringSecurityDescriptorToSecurityDescriptorW(
            string_descriptor.data(), SDDL_REVISION_1, &desc, &size)) {
      if (size > *descriptor_size) {
        ret = STATUS_BUFFER_TOO_SMALL;
      } else {
        ::CopyMemory(descriptor, desc, size);
      }

      *descriptor_size = size;
      ::LocalFree(desc);
    } else {
      ret = FspNtStatusFromWin32(::GetLastError());
    }
  }
  return ret;
}

auto remote_winfsp_drive::GetVolumeInfo(VolumeInfo *volume_info) -> NTSTATUS {
  std::string volume_label;
  auto ret = remote_instance_->winfsp_get_volume_info(
      volume_info->TotalSize, volume_info->FreeSize, volume_label);
  if (ret == STATUS_SUCCESS) {
    auto byte_size = static_cast<UINT16>(volume_label.size() * sizeof(WCHAR));
    wcscpy_s(&volume_info->VolumeLabel[0U], 32,
             utils::string::from_utf8(volume_label).c_str());
    volume_info->VolumeLabelLength =
        std::min(static_cast<UINT16>(64U), byte_size);
  }
  return ret;
}

auto remote_winfsp_drive::Init(PVOID host) -> NTSTATUS {
  auto *file_system_host = reinterpret_cast<FileSystemHost *>(host);
  if (not config_.get_enable_mount_manager()) {
    file_system_host->SetPrefix(
        &(L"\\repertory\\" +
          std::wstring(file_system_host->FileSystemName()).substr(0U, 1U))[0U]);
  }

  file_system_host->SetCasePreservedNames(TRUE);
  file_system_host->SetCaseSensitiveSearch(TRUE);
  file_system_host->SetFileInfoTimeout(1000);
  file_system_host->SetFlushAndPurgeOnCleanup(TRUE);
  file_system_host->SetMaxComponentLength(255U);
  file_system_host->SetNamedStreams(FALSE);
  file_system_host->SetPassQueryDirectoryPattern(FALSE);
  file_system_host->SetPersistentAcls(FALSE);
  file_system_host->SetPostCleanupWhenModifiedOnly(TRUE);
  file_system_host->SetReparsePoints(FALSE);
  file_system_host->SetReparsePointsAccessCheck(FALSE);
  file_system_host->SetSectorSize(WINFSP_ALLOCATION_UNIT);
  file_system_host->SetSectorsPerAllocationUnit(1);
  file_system_host->SetUnicodeOnDisk(TRUE);
  file_system_host->SetVolumeCreationTime(utils::time::get_time_now());
  file_system_host->SetVolumeSerialNumber(0);
  return STATUS_SUCCESS;
}

auto remote_winfsp_drive::mount(const std::vector<std::string> &drive_args)
    -> int {
  REPERTORY_USES_FUNCTION_NAME();

  std::vector<std::string> parsed_drive_args;

  auto force_no_console = utils::collection::includes(drive_args, "-nc");

  auto enable_console = false;
  for (const auto &arg : drive_args) {
    if (arg == "-f") {
      if (not force_no_console) {
        enable_console = true;
      }
    } else if (arg != "-nc") {
      parsed_drive_args.emplace_back(arg);
    }
  }

  logging_consumer l(config_.get_event_level(), config_.get_log_directory());
  std::unique_ptr<console_consumer> c;
  if (enable_console) {
    c = std::make_unique<console_consumer>(config_.get_event_level());
  }
  event_system::instance().start();

  auto ret = winfsp_service(lock_, *this, parsed_drive_args, config_).Run();

  event_system::instance().raise<drive_mount_result>(function_name, "",
                                                     std::to_string(ret));
  event_system::instance().stop();
  c.reset();
  return static_cast<int>(ret);
}

auto remote_winfsp_drive::Mounted(PVOID host) -> NTSTATUS {
  REPERTORY_USES_FUNCTION_NAME();

  auto *file_system_host = reinterpret_cast<FileSystemHost *>(host);
  remote_instance_ = factory_();
  server_ = std::make_unique<server>(config_);
  server_->start();
  mount_location_ = utils::string::to_utf8(file_system_host->MountPoint());
  if (not lock_.set_mount_state(true, mount_location_,
                                ::GetCurrentProcessId())) {
    utils::error::raise_error(function_name, "failed to set mount state");
  }

  return remote_instance_->winfsp_mounted(file_system_host->MountPoint());
}

auto remote_winfsp_drive::Open(PWSTR file_name, UINT32 create_options,
                               UINT32 granted_access, PVOID * /*file_node*/,
                               PVOID *file_desc, OpenFileInfo *ofi)
    -> NTSTATUS {
  remote::file_info fi{};
  std::string normalize_name;
  auto ret =
      remote_instance_->winfsp_open(file_name, create_options, granted_access,
                                    file_desc, &fi, normalize_name);
  if (ret == STATUS_SUCCESS) {
    set_file_info(ofi->FileInfo, fi);
    auto file_path = utils::string::from_utf8(normalize_name);
    wcsncpy(ofi->NormalizedName, file_path.data(), wcslen(file_path.c_str()));
    ofi->NormalizedNameSize =
        static_cast<UINT16>(wcslen(file_path.c_str()) * sizeof(WCHAR));
  }

  return ret;
}

auto remote_winfsp_drive::Overwrite(PVOID /*file_node*/, PVOID file_desc,
                                    UINT32 attributes,
                                    BOOLEAN replace_attributes,
                                    UINT64 allocation_size, FileInfo *file_info)
    -> NTSTATUS {
  remote::file_info info{};
  auto ret = remote_instance_->winfsp_overwrite(
      file_desc, attributes, replace_attributes, allocation_size, &info);
  set_file_info(*file_info, info);
  return ret;
}

void remote_winfsp_drive::populate_file_info(const json &item,
                                             FSP_FSCTL_FILE_INFO &file_info) {
  auto dir_item = item.get<directory_item>();
  file_info.FileSize = dir_item.directory ? 0 : dir_item.size;
  file_info.AllocationSize =
      utils::divide_with_ceiling(file_info.FileSize, WINFSP_ALLOCATION_UNIT) *
      WINFSP_ALLOCATION_UNIT;
  file_info.ChangeTime = utils::get_changed_time_from_meta(dir_item.meta);
  file_info.CreationTime = utils::get_creation_time_from_meta(dir_item.meta);
  file_info.FileAttributes = utils::get_attributes_from_meta(dir_item.meta);
  file_info.HardLinks = 0;
  file_info.IndexNumber = 0;
  file_info.LastAccessTime = utils::get_accessed_time_from_meta(dir_item.meta);
  file_info.LastWriteTime = utils::get_written_time_from_meta(dir_item.meta);
  file_info.ReparseTag = 0;
  file_info.EaSize = 0;
}

auto remote_winfsp_drive::Read(PVOID /*file_node*/, PVOID file_desc,
                               PVOID buffer, UINT64 offset, ULONG length,
                               PULONG bytes_transferred) -> NTSTATUS {
  auto ret = remote_instance_->winfsp_read(
      file_desc, buffer, offset, length,
      reinterpret_cast<PUINT32>(bytes_transferred));
  if ((ret == STATUS_SUCCESS) && (*bytes_transferred != length)) {
    ::SetLastError(ERROR_HANDLE_EOF);
  }

  return ret;
}

auto remote_winfsp_drive::ReadDirectory(PVOID /*file_node*/, PVOID file_desc,
                                        PWSTR pattern, PWSTR marker,
                                        PVOID buffer, ULONG buffer_length,
                                        PULONG bytes_transferred) -> NTSTATUS {
  json item_list;
  auto ret{
      static_cast<NTSTATUS>(remote_instance_->winfsp_read_directory(
          file_desc, pattern, marker, item_list)),
  };
  if (ret == STATUS_SUCCESS) {
    PVOID *directory_buffer{nullptr};
    if ((ret = remote_instance_->winfsp_get_dir_buffer(
             file_desc, directory_buffer)) == STATUS_SUCCESS) {
      if (FspFileSystemAcquireDirectoryBuffer(
              directory_buffer, static_cast<BOOLEAN>(nullptr == marker),
              &ret)) {
        auto item_found = false;
        for (const auto &item : item_list) {
          auto item_path = item[JSON_API_PATH].get<std::string>();
          auto display_name = utils::string::from_utf8(
              utils::path::strip_to_file_name(item_path));
          if (not marker || (marker && item_found)) {
            union {
              UINT8 B[FIELD_OFFSET(FSP_FSCTL_DIR_INFO, FileNameBuf) +
                      ((repertory::max_path_length + 1U) * sizeof(WCHAR))];
              FSP_FSCTL_DIR_INFO D;
            } directory_info_buffer;

            auto *directory_info = &directory_info_buffer.D;
            ::ZeroMemory(directory_info, sizeof(*directory_info));
            directory_info->Size = static_cast<UINT16>(
                FIELD_OFFSET(FSP_FSCTL_DIR_INFO, FileNameBuf) +
                (std::min(static_cast<size_t>(repertory::max_path_length),
                          display_name.size()) *
                 sizeof(WCHAR)));

            populate_file_info(item, directory_info->FileInfo);

            if (ret == STATUS_SUCCESS) {
              ::wcscpy_s(&directory_info->FileNameBuf[0],
                         repertory::max_path_length, &display_name[0]);

              FspFileSystemFillDirectoryBuffer(directory_buffer, directory_info,
                                               &ret);
              if (ret != STATUS_SUCCESS) {
                break;
              }
            }
          } else {
            item_found = display_name == std::wstring(marker);
          }
        }

        FspFileSystemReleaseDirectoryBuffer(directory_buffer);
      }

      if ((ret == STATUS_SUCCESS) || (ret == STATUS_NO_MORE_FILES)) {
        FspFileSystemReadDirectoryBuffer(
            directory_buffer, marker, buffer, buffer_length,
            reinterpret_cast<PULONG>(bytes_transferred));
        ret = STATUS_SUCCESS;
      }
    }
  }

  return ret;
}

auto remote_winfsp_drive::Rename(PVOID /*file_node*/, PVOID file_desc,
                                 PWSTR file_name, PWSTR new_file_name,
                                 BOOLEAN replace_if_exists) -> NTSTATUS {
  auto res = remote_instance_->winfsp_rename(file_desc, file_name,
                                             new_file_name, replace_if_exists);
  if (res == STATUS_OBJECT_NAME_EXISTS) {
    return FspNtStatusFromWin32(ERROR_ALREADY_EXISTS);
  }

  return res;
}

auto remote_winfsp_drive::SetBasicInfo(PVOID /*file_node*/, PVOID file_desc,
                                       UINT32 attributes, UINT64 creation_time,
                                       UINT64 last_access_time,
                                       UINT64 last_write_time,
                                       UINT64 change_time, FileInfo *file_info)
    -> NTSTATUS {
  remote::file_info f_info{};
  auto ret = remote_instance_->winfsp_set_basic_info(
      file_desc, attributes, creation_time, last_access_time, last_write_time,
      change_time, &f_info);
  set_file_info(*file_info, f_info);
  return ret;
}

void remote_winfsp_drive::set_file_info(FileInfo &dest,
                                        const remote::file_info &src) {
  dest.FileAttributes = src.FileAttributes;
  dest.ReparseTag = src.ReparseTag;
  dest.AllocationSize = src.AllocationSize;
  dest.FileSize = src.FileSize;
  dest.CreationTime = src.CreationTime;
  dest.LastAccessTime = src.LastAccessTime;
  dest.LastWriteTime = src.LastWriteTime;
  dest.ChangeTime = src.ChangeTime;
  dest.IndexNumber = src.IndexNumber;
  dest.HardLinks = src.HardLinks;
  dest.EaSize = src.EaSize;
}

auto remote_winfsp_drive::SetFileSize(PVOID /*file_node*/, PVOID file_desc,
                                      UINT64 new_size,
                                      BOOLEAN set_allocation_size,
                                      FileInfo *file_info) -> NTSTATUS {
  remote::file_info fi{};
  auto ret = remote_instance_->winfsp_set_file_size(file_desc, new_size,
                                                    set_allocation_size, &fi);
  set_file_info(*file_info, fi);
  return ret;
}

VOID remote_winfsp_drive::Unmounted(PVOID host) {
  REPERTORY_USES_FUNCTION_NAME();

  server_->stop();
  server_.reset();
  auto *file_system_host = reinterpret_cast<FileSystemHost *>(host);
  if (not lock_.set_mount_state(false, "", -1)) {
    utils::error::raise_error(function_name, "failed to set mount state");
  }
  remote_instance_->winfsp_unmounted(file_system_host->MountPoint());
  remote_instance_.reset();
}

auto remote_winfsp_drive::Write(PVOID /*file_node*/, PVOID file_desc,
                                PVOID buffer, UINT64 offset, ULONG length,
                                BOOLEAN write_to_end, BOOLEAN constrained_io,
                                PULONG bytes_transferred, FileInfo *file_info)
    -> NTSTATUS {
  remote::file_info fi{};
  auto ret = remote_instance_->winfsp_write(
      file_desc, buffer, offset, length, write_to_end, constrained_io,
      reinterpret_cast<PUINT32>(bytes_transferred), &fi);
  set_file_info(*file_info, fi);
  return ret;
}
} // namespace repertory::remote_winfsp

#endif // _WIN32
