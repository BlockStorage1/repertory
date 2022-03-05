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
#ifdef _WIN32
#include "drives/winfsp/remotewinfsp/remote_winfsp_drive.hpp"
#include "app_config.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/consumers/logging_consumer.hpp"
#include "events/events.hpp"
#include "platform/platform.hpp"
#include "rpc/server/server.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/utils.hpp"

namespace repertory::remote_winfsp {
remote_winfsp_drive::winfsp_service::winfsp_service(lock_data &lock, remote_winfsp_drive &drive,
                                                    std::vector<std::string> drive_args,
                                                    app_config &config)
    : Service(&(L"RepertoryRemote_" + utils::string::from_utf8(lock.get_unique_id()))[0u]),
      config_(config),
      lock_(lock),
      drive_(drive),
      drive_args_(std::move(drive_args)),
      host_(drive) {}

NTSTATUS remote_winfsp_drive::winfsp_service::OnStart(ULONG, PWSTR *) {
  const auto mount_location =
      utils::path::absolute((drive_args_.size() > 1u) ? drive_args_[1u] : "");
  const auto drive_letter = ((mount_location.size() == 2u) ||
                             ((mount_location.size() == 3u) && (mount_location[2u] == '\\'))) &&
                            (mount_location[1u] == ':');

  auto ret = drive_letter ? STATUS_DEVICE_BUSY : STATUS_NOT_SUPPORTED;
  if ((drive_letter && not utils::file::is_directory(mount_location))) {
    auto unicode_mount_location = utils::string::from_utf8(mount_location);
    host_.SetFileSystemName(&unicode_mount_location[0u]);
    ret = host_.Mount(&unicode_mount_location[0u]);
  } else {
    std::cerr << (drive_letter ? "Mount location in use: " : "Mount location not supported: ")
              << mount_location << std::endl;
  }

  if (ret != STATUS_SUCCESS) {
    event_system::instance().raise<drive_mount_failed>(mount_location, std::to_string(ret));
    lock_.set_mount_state(false, "", -1);
  }

  return ret;
}

NTSTATUS remote_winfsp_drive::winfsp_service::OnStop() {
  host_.Unmount();
  lock_.set_mount_state(false, "", -1);

  return STATUS_SUCCESS;
}

remote_winfsp_drive::remote_winfsp_drive(app_config &config, lock_data &lockData,
                                         remote_instance_factory factory)
    : FileSystemBase(), config_(config), lock_(lockData), factory_(factory) {
  E_SUBSCRIBE_EXACT(unmount_requested, [this](const unmount_requested &e) {
    std::thread([this]() { this->shutdown(); }).detach();
  });
}

NTSTATUS remote_winfsp_drive::CanDelete(PVOID file_node, PVOID file_desc, PWSTR file_name) {
  return remote_instance_->winfsp_can_delete(file_desc, file_name);
}

VOID remote_winfsp_drive::Cleanup(PVOID file_node, PVOID file_desc, PWSTR file_name, ULONG flags) {
  BOOLEAN was_closed;
  remote_instance_->winfsp_cleanup(file_desc, file_name, flags, was_closed);
}

VOID remote_winfsp_drive::Close(PVOID file_node, PVOID file_desc) {
  remote_instance_->winfsp_close(file_desc);
}

NTSTATUS remote_winfsp_drive::Create(PWSTR file_name, UINT32 create_options, UINT32 granted_access,
                                     UINT32 attributes, PSECURITY_DESCRIPTOR descriptor,
                                     UINT64 allocation_size, PVOID *file_node, PVOID *file_desc,
                                     OpenFileInfo *ofi) {
  remote::file_info fi{};
  std::string normalized_name;
  BOOLEAN exists = 0;
  const auto ret =
      remote_instance_->winfsp_create(file_name, create_options, granted_access, attributes,
                                      allocation_size, file_desc, &fi, normalized_name, exists);
  if (ret == STATUS_SUCCESS) {
    SetFileInfo(ofi->FileInfo, fi);
    const auto filePath = utils::string::from_utf8(normalized_name);
    wcsncpy(ofi->NormalizedName, &filePath[0], wcslen(&filePath[0]));
    ofi->NormalizedNameSize = (UINT16)(wcslen(&filePath[0]) * sizeof(WCHAR));
  }

  return ret;
}

NTSTATUS remote_winfsp_drive::Flush(PVOID file_node, PVOID file_desc, FileInfo *file_info) {
  remote::file_info fi{};
  const auto ret = remote_instance_->winfsp_flush(file_desc, &fi);
  SetFileInfo(*file_info, fi);
  return ret;
}

NTSTATUS remote_winfsp_drive::GetFileInfo(PVOID file_node, PVOID file_desc, FileInfo *file_info) {
  remote::file_info fi{};
  const auto ret = remote_instance_->winfsp_get_file_info(file_desc, &fi);
  SetFileInfo(*file_info, fi);
  return ret;
}

NTSTATUS remote_winfsp_drive::GetSecurityByName(PWSTR file_name, PUINT32 attributes,
                                                PSECURITY_DESCRIPTOR descriptor,
                                                SIZE_T *descriptor_size) {
  std::wstring string_descriptor;
  std::uint64_t sds = descriptor_size ? *descriptor_size : 0;
  auto ret = remote_instance_->winfsp_get_security_by_name(
      file_name, attributes, descriptor_size ? &sds : nullptr, string_descriptor);
  *descriptor_size = static_cast<SIZE_T>(sds);
  if ((ret == STATUS_SUCCESS) && *descriptor_size) {
    PSECURITY_DESCRIPTOR sd = nullptr;
    ULONG sz2 = 0u;
    if (::ConvertStringSecurityDescriptorToSecurityDescriptorW(&string_descriptor[0u],
                                                               SDDL_REVISION_1, &sd, &sz2)) {
      if (sz2 > *descriptor_size) {
        ret = STATUS_BUFFER_TOO_SMALL;
      } else {
        ::CopyMemory(descriptor, sd, sz2);
      }
      *descriptor_size = sz2;
      ::LocalFree(sd);
    } else {
      ret = FspNtStatusFromWin32(::GetLastError());
    }
  }
  return ret;
}

NTSTATUS remote_winfsp_drive::GetVolumeInfo(VolumeInfo *volume_info) {
  std::string volume_label;
  const auto ret = remote_instance_->winfsp_get_volume_info(volume_info->TotalSize,
                                                            volume_info->FreeSize, volume_label);
  if (ret == STATUS_SUCCESS) {
    const auto byte_size = static_cast<UINT16>(volume_label.size() * sizeof(WCHAR));
    wcscpy_s(&volume_info->VolumeLabel[0u], 32, &utils::string::from_utf8(volume_label)[0u]);
    volume_info->VolumeLabelLength = std::min(static_cast<UINT16>(64u), byte_size);
  }
  return ret;
}

NTSTATUS remote_winfsp_drive::Init(PVOID host) {
  auto *file_system_host = reinterpret_cast<FileSystemHost *>(host);
  file_system_host->SetPrefix(
      &(L"\\repertory\\" + std::wstring(file_system_host->FileSystemName()).substr(0, 1))[0]);
  file_system_host->SetFileSystemName(REPERTORY_W);
  file_system_host->SetFlushAndPurgeOnCleanup(TRUE);
  file_system_host->SetReparsePoints(FALSE);
  file_system_host->SetReparsePointsAccessCheck(FALSE);
  file_system_host->SetSectorSize(WINFSP_ALLOCATION_UNIT);
  file_system_host->SetSectorsPerAllocationUnit(1);
  file_system_host->SetFileInfoTimeout(1000);
  file_system_host->SetCaseSensitiveSearch(FALSE);
  file_system_host->SetCasePreservedNames(TRUE);
  file_system_host->SetNamedStreams(FALSE);
  file_system_host->SetUnicodeOnDisk(TRUE);
  file_system_host->SetPersistentAcls(FALSE);
  file_system_host->SetPostCleanupWhenModifiedOnly(TRUE);
  file_system_host->SetPassQueryDirectoryPattern(FALSE);
  file_system_host->SetVolumeCreationTime(utils::get_file_time_now());
  file_system_host->SetVolumeSerialNumber(0);
  return STATUS_SUCCESS;
}

int remote_winfsp_drive::mount(const std::vector<std::string> &drive_args) {
  std::vector<std::string> parsed_drive_args;

  const auto force_no_console = utils::collection_includes(drive_args, "-nc");

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

  logging_consumer l(config_.get_log_directory(), config_.get_event_level());
  std::unique_ptr<console_consumer> c;
  if (enable_console) {
    c = std::make_unique<console_consumer>();
  }
  event_system::instance().start();
  const auto ret = winfsp_service(lock_, *this, parsed_drive_args, config_).Run();
  event_system::instance().raise<drive_mount_result>(std::to_string(ret));
  event_system::instance().stop();
  c.reset();
  return static_cast<int>(ret);
}

NTSTATUS remote_winfsp_drive::Mounted(PVOID host) {
  auto *file_system_host = reinterpret_cast<FileSystemHost *>(host);
  remote_instance_ = factory_();
  server_ = std::make_unique<server>(config_);
  server_->start();
  mount_location_ = utils::string::to_utf8(file_system_host->MountPoint());
  lock_.set_mount_state(true, mount_location_, ::GetCurrentProcessId());
  return remote_instance_->winfsp_mounted(file_system_host->MountPoint());
}

NTSTATUS remote_winfsp_drive::Open(PWSTR file_name, UINT32 create_options, UINT32 granted_access,
                                   PVOID *file_node, PVOID *file_desc, OpenFileInfo *ofi) {
  remote::file_info fi{};
  std::string normalize_name;
  const auto ret = remote_instance_->winfsp_open(file_name, create_options, granted_access,
                                                 file_desc, &fi, normalize_name);
  if (ret == STATUS_SUCCESS) {
    SetFileInfo(ofi->FileInfo, fi);
    const auto filePath = utils::string::from_utf8(normalize_name);
    wcsncpy(ofi->NormalizedName, &filePath[0], wcslen(&filePath[0]));
    ofi->NormalizedNameSize = (UINT16)(wcslen(&filePath[0]) * sizeof(WCHAR));
  }

  return ret;
}

NTSTATUS remote_winfsp_drive::Overwrite(PVOID file_node, PVOID file_desc, UINT32 attributes,
                                        BOOLEAN replace_attributes, UINT64 allocation_size,
                                        FileInfo *file_info) {
  remote::file_info fi{};
  const auto ret = remote_instance_->winfsp_overwrite(file_desc, attributes, replace_attributes,
                                                      allocation_size, &fi);
  SetFileInfo(*file_info, fi);
  return ret;
}

void remote_winfsp_drive::PopulateFileInfo(const json &item, FSP_FSCTL_FILE_INFO &file_info) {
  const auto di = directory_item::from_json(item);
  file_info.FileSize = di.directory ? 0 : di.size;
  file_info.AllocationSize =
      utils::divide_with_ceiling(file_info.FileSize, WINFSP_ALLOCATION_UNIT) *
      WINFSP_ALLOCATION_UNIT;
  file_info.ChangeTime = utils::get_changed_time_from_meta(di.meta);
  file_info.CreationTime = utils::get_creation_time_from_meta(di.meta);
  file_info.FileAttributes = utils::get_attributes_from_meta(di.meta);
  file_info.HardLinks = 0;
  file_info.IndexNumber = 0;
  file_info.LastAccessTime = utils::get_accessed_time_from_meta(di.meta);
  file_info.LastWriteTime = utils::get_written_time_from_meta(di.meta);
  file_info.ReparseTag = 0;
  file_info.EaSize = 0;
}

NTSTATUS remote_winfsp_drive::Read(PVOID file_node, PVOID file_desc, PVOID buffer, UINT64 offset,
                                   ULONG length, PULONG bytes_transferred) {
  return remote_instance_->winfsp_read(file_desc, buffer, offset, length,
                                       reinterpret_cast<PUINT32>(bytes_transferred));
}

NTSTATUS remote_winfsp_drive::ReadDirectory(PVOID file_node, PVOID file_desc, PWSTR pattern,
                                            PWSTR marker, PVOID buffer, ULONG buffer_length,
                                            PULONG bytes_transferred) {
  json itemList;
  NTSTATUS ret = remote_instance_->winfsp_read_directory(file_desc, pattern, marker, itemList);
  if (ret == STATUS_SUCCESS) {
    PVOID *directory_buffer = nullptr;
    if ((ret = remote_instance_->winfsp_get_dir_buffer(file_desc, directory_buffer)) ==
        STATUS_SUCCESS) {
      if (FspFileSystemAcquireDirectoryBuffer(directory_buffer,
                                              static_cast<BOOLEAN>(nullptr == marker), &ret)) {
        auto item_found = false;
        for (const auto &item : itemList) {
          const auto item_path = item["path"].get<std::string>();
          const auto display_name =
              utils::string::from_utf8(utils::path::strip_to_file_name(item_path));
          if (not marker || (marker && item_found)) {
            if (not utils::path::is_ads_file_path(item_path)) {
              union {
                UINT8 B[FIELD_OFFSET(FSP_FSCTL_DIR_INFO, FileNameBuf) +
                        ((MAX_PATH + 1) * sizeof(WCHAR))];
                FSP_FSCTL_DIR_INFO D;
              } directory_info_buffer;

              auto *directory_info = &directory_info_buffer.D;
              ::ZeroMemory(directory_info, sizeof(*directory_info));
              directory_info->Size = static_cast<UINT16>(
                  FIELD_OFFSET(FSP_FSCTL_DIR_INFO, FileNameBuf) +
                  (std::min((size_t)MAX_PATH, display_name.size()) * sizeof(WCHAR)));

              if (not item["meta"].empty() || ((item_path != ".") && (item_path != ".."))) {
                PopulateFileInfo(item, directory_info->FileInfo);
              }
              if (ret == STATUS_SUCCESS) {
                ::wcscpy_s(&directory_info->FileNameBuf[0], MAX_PATH, &display_name[0]);

                FspFileSystemFillDirectoryBuffer(directory_buffer, directory_info, &ret);
                if (ret != STATUS_SUCCESS) {
                  break;
                }
              }
            }
          } else {
            item_found = display_name == std::wstring(marker);
          }
        }

        FspFileSystemReleaseDirectoryBuffer(directory_buffer);
      }

      if ((ret == STATUS_SUCCESS) || (ret == STATUS_NO_MORE_FILES)) {
        FspFileSystemReadDirectoryBuffer(directory_buffer, marker, buffer, buffer_length,
                                         reinterpret_cast<PULONG>(bytes_transferred));
        ret = STATUS_SUCCESS;
      }
    }
  }

  return ret;
}

NTSTATUS remote_winfsp_drive::Rename(PVOID file_node, PVOID file_desc, PWSTR file_name,
                                     PWSTR new_file_name, BOOLEAN replace_if_exists) {
  return remote_instance_->winfsp_rename(file_desc, file_name, new_file_name, replace_if_exists);
}

NTSTATUS remote_winfsp_drive::SetBasicInfo(PVOID file_node, PVOID file_desc, UINT32 attributes,
                                           UINT64 creation_time, UINT64 last_access_time,
                                           UINT64 last_write_time, UINT64 change_time,
                                           FileInfo *file_info) {
  remote::file_info fi{};
  const auto ret = remote_instance_->winfsp_set_basic_info(
      file_desc, attributes, creation_time, last_access_time, last_write_time, change_time, &fi);
  SetFileInfo(*file_info, fi);
  return ret;
}

void remote_winfsp_drive::SetFileInfo(FileInfo &dest, const remote::file_info &src) {
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

NTSTATUS remote_winfsp_drive::SetFileSize(PVOID file_node, PVOID file_desc, UINT64 new_size,
                                          BOOLEAN set_allocation_size, FileInfo *file_info) {
  remote::file_info fi{};
  const auto ret =
      remote_instance_->winfsp_set_file_size(file_desc, new_size, set_allocation_size, &fi);
  SetFileInfo(*file_info, fi);
  return ret;
}

VOID remote_winfsp_drive::Unmounted(PVOID host) {
  server_->stop();
  server_.reset();
  auto *fileSystemHost = reinterpret_cast<FileSystemHost *>(host);
  lock_.set_mount_state(false, "", -1);
  remote_instance_->winfsp_unmounted(fileSystemHost->MountPoint());
  remote_instance_.reset();
  mount_location_ = "";
}

NTSTATUS remote_winfsp_drive::Write(PVOID file_node, PVOID file_desc, PVOID buffer, UINT64 offset,
                                    ULONG length, BOOLEAN write_to_end, BOOLEAN constrained_io,
                                    PULONG bytes_transferred, FileInfo *file_info) {
  remote::file_info fi{};
  const auto ret = remote_instance_->winfsp_write(
      file_desc, buffer, offset, length, write_to_end, constrained_io,
      reinterpret_cast<PUINT32>(bytes_transferred), &fi);
  SetFileInfo(*file_info, fi);
  return ret;
}
} // namespace repertory::remote_winfsp

#endif // _WIN32
