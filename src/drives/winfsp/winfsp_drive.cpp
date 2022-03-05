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

#include "drives/winfsp/winfsp_drive.hpp"
#include "app_config.hpp"
#include "drives/directory_iterator.hpp"
#include "events/events.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/consumers/logging_consumer.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "types/startup_exception.hpp"
#include "utils/global_data.hpp"
#include "utils/polling.hpp"
#include "utils/utils.hpp"

namespace repertory {
// clang-format off
E_SIMPLE3(winfsp_event, debug, true,
  std::string, function, func, E_STRING,
  std::string, api_path, ap, E_STRING,
  NTSTATUS, result, res, E_FROM_INT32
);
// clang-format on

#define RAISE_WINFSP_EVENT(func, file, ret)                                                        \
  if (config_.get_enable_drive_events() &&                                                         \
      (((config_.get_event_level() >= winfsp_event::level) && (ret != STATUS_SUCCESS)) ||          \
       (config_.get_event_level() >= event_level::verbose)))                                       \
  event_system::instance().raise<winfsp_event>(func, file, ret)

winfsp_drive::winfsp_service::winfsp_service(lock_data &lock, winfsp_drive &drive,
                                             std::vector<std::string> drive_args,
                                             app_config &config)
    : Service((PWSTR)L"Repertory"),
      lock_(lock),
      drive_(drive),
      drive_args_(std::move(drive_args)),
      host_(drive),
      config_(config) {}

NTSTATUS winfsp_drive::winfsp_service::OnStart(ULONG, PWSTR *) {
  const auto mount_location =
      utils::path::absolute((drive_args_.size() > 1u) ? drive_args_[1u] : "");
  const auto drive_letter = ((mount_location.size() == 2u) ||
                             ((mount_location.size() == 3u) && (mount_location[2u] == '\\'))) &&
                            (mount_location[1u] == ':');

  auto ret = drive_letter ? STATUS_DEVICE_BUSY : STATUS_NOT_SUPPORTED;
  if ((drive_letter && not utils::file::is_directory(mount_location))) {
    auto unicode_mount_location = utils::string::from_utf8(mount_location);
    host_.SetFileSystemName(&unicode_mount_location[0u]);
    if (config_.get_enable_mount_manager()) {
      unicode_mount_location = std::wstring(L"\\\\.\\") + unicode_mount_location[0u] + L":";
    }
    ret = host_.Mount(&unicode_mount_location[0]);
  } else {
    std::cerr << (drive_letter ? "Mount location in use: " : "Mount location not supported: ")
              << mount_location << std::endl;
  }

  if (ret != STATUS_SUCCESS) {
    lock_.set_mount_state(false, "", -1);
    event_system::instance().raise<drive_mount_failed>(mount_location, std::to_string(ret));
  }

  return ret;
}

NTSTATUS winfsp_drive::winfsp_service::OnStop() {
  if (drive_.download_manager_) {
    drive_.download_manager_->stop();
  }
  host_.Unmount();

  lock_.set_mount_state(false, "", -1);

  return STATUS_SUCCESS;
}

winfsp_drive::winfsp_drive(app_config &config, lock_data &lock, i_provider &provider)
    : FileSystemBase(), provider_(provider), config_(config), lock_(lock) {
  E_SUBSCRIBE_EXACT(unmount_requested, [this](const unmount_requested &e) {
    std::thread([this]() { this->shutdown(); }).detach();
  });
}

NTSTATUS winfsp_drive::CanDelete(PVOID file_node, PVOID file_desc, PWSTR file_name) {
  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    filesystem_item *fsi = nullptr;
    if (oft_->get_open_file(handle, fsi)) {
      api_path = fsi->api_path;
      if (fsi->directory) {
        error = provider_.get_directory_item_count(fsi->api_path) ? api_error::directory_not_empty
                                                                  : api_error::success;
      } else {
        error = download_manager_->is_processing(fsi->api_path) ? api_error::file_in_use
                                                                : api_error::success;
      }
    }
  }

  const auto ret = utils::translate_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

VOID winfsp_drive::Cleanup(PVOID file_node, PVOID file_desc, PWSTR file_name, ULONG flags) {
  std::string api_path;
  auto handle = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    filesystem_item *fsi = nullptr;
    if (oft_->get_open_file(handle, fsi)) {
      api_path = fsi->api_path;

      const auto directory = fsi->directory;
      if ((flags & FspCleanupSetArchiveBit) && not directory) {
        api_meta_map meta;
        if (oft_->derive_item_data(api_path, meta) == api_error::success) {
          oft_->set_item_meta(
              api_path, META_ATTRIBUTES,
              std::to_string(utils::get_attributes_from_meta(meta) | FILE_ATTRIBUTE_ARCHIVE));
        }
      }

      if (flags &
          (FspCleanupSetLastAccessTime | FspCleanupSetLastWriteTime | FspCleanupSetChangeTime)) {
        const auto fileTimeNow = utils::get_file_time_now();
        if (flags & FspCleanupSetLastAccessTime) {
          oft_->set_item_meta(api_path, META_ACCESSED, std::to_string(fileTimeNow));
        }

        if (flags & FspCleanupSetLastWriteTime) {
          oft_->set_item_meta(api_path, META_WRITTEN, std::to_string(fileTimeNow));
        }

        if (flags & FspCleanupSetChangeTime) {
          oft_->set_item_meta(api_path, META_MODIFIED, std::to_string(fileTimeNow));
        }
      }

      if (flags & FspCleanupSetAllocationSize) {
        UINT64 allocation_size =
            utils::divide_with_ceiling(fsi->size, WINFSP_ALLOCATION_UNIT) * WINFSP_ALLOCATION_UNIT;
        SetFileSize(file_node, file_desc, allocation_size, TRUE, nullptr);
      }

      if (flags & FspCleanupDelete) {
        auto *directory_buffer = fsi->open_data[handle].directory_buffer;
        fsi = nullptr;

        oft_->close_all(api_path);

        if (directory_buffer) {
          FspFileSystemDeleteDirectoryBuffer(&directory_buffer);
        }

        if (directory) {
          provider_.remove_directory(api_path);
        } else {
          oft_->remove_file(api_path);
        }
      }
    }
  }
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, 0);
}

VOID winfsp_drive::Close(PVOID file_node, PVOID file_desc) {
  std::string api_path;
  auto handle = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    PVOID directory_buffer = nullptr;
    filesystem_item *fsi = nullptr;
    if (oft_->get_open_file(handle, fsi)) {
      api_path = fsi->api_path;
      directory_buffer = fsi->open_data[handle].directory_buffer;
      fsi = nullptr;
    }
    oft_->close(handle);
    if (directory_buffer) {
      FspFileSystemDeleteDirectoryBuffer(&directory_buffer);
    }
  }
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, 0);
}

NTSTATUS winfsp_drive::Create(PWSTR fileName, UINT32 create_options, UINT32 grantedAccess,
                              UINT32 attributes, PSECURITY_DESCRIPTOR descriptor,
                              UINT64 allocation_size, PVOID *file_node, PVOID *file_desc,
                              OpenFileInfo *ofi) {
  // TODO Need to revisit this
  // (ConvertSecurityDescriptorToStringSecurityDescriptor/ConvertStringSecurityDescriptorToSecurityDescriptor)
  auto error = api_error::error;

  if (create_options & FILE_DIRECTORY_FILE) {
    attributes |= FILE_ATTRIBUTE_DIRECTORY;
  } else {
    attributes &= ~FILE_ATTRIBUTE_DIRECTORY;
  }

  if (not attributes) {
    attributes = FILE_ATTRIBUTE_NORMAL;
  }

  const auto create_date = utils::get_file_time_now();

  auto meta = api_meta_map({
      {META_ATTRIBUTES, std::to_string(attributes)},
      {META_CREATION, std::to_string(create_date)},
      {META_WRITTEN, std::to_string(create_date)},
      {META_MODIFIED, std::to_string(create_date)},
      {META_ACCESSED, std::to_string(create_date)},
  });

  const auto api_path = utils::path::create_api_path(utils::string::to_utf8(fileName));
  if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
    error = provider_.create_directory(api_path, meta);
  } else {
    error = provider_.create_file(api_path, meta);
  }
  if (error == api_error::success) {
    open_file_data ofd{};
    std::uint64_t handle;
    error =
        oft_->Open(api_path, static_cast<bool>(attributes & FILE_ATTRIBUTE_DIRECTORY), ofd, handle);
    if (error == api_error::success) {
      populate_file_info(api_path, 0, meta, *ofi);
      *file_desc = reinterpret_cast<PVOID>(handle);
    }
  }

  const auto ret = utils::translate_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

NTSTATUS winfsp_drive::Flush(PVOID file_node, PVOID file_desc, FileInfo *fileInfo) {
  std::string api_path;
  auto error = api_error::success;
  auto handle = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    error = api_error::invalid_handle;
    filesystem_item *fsi = nullptr;
    if (oft_->get_open_file(handle, fsi)) {
      api_path = fsi->api_path;
      error = api_error::success;
      if (fsi->handle != REPERTORY_INVALID_HANDLE) {
        if (not ::FlushFileBuffers(fsi->handle)) {
          error = api_error::os_error;
        }
      }

      // Populate file information
      std::uint64_t file_size = 0u;
      api_meta_map meta;
      if (oft_->derive_item_data(api_path, false, file_size, meta) == api_error::success) {
        populate_file_info(file_size, meta, *fileInfo);
      }
    }
  }

  const auto ret = utils::translate_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

std::uint64_t winfsp_drive::get_directory_item_count(const std::string &api_path) const {
  return provider_.get_directory_item_count(api_path);
}

directory_item_list winfsp_drive::get_directory_items(const std::string &api_path) const {
  directory_item_list list;
  provider_.get_directory_items(api_path, list);
  return list;
}

NTSTATUS winfsp_drive::GetFileInfo(PVOID file_node, PVOID file_desc, FileInfo *fileInfo) {
  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    filesystem_item *fsi = nullptr;
    if (oft_->get_open_file(handle, fsi)) {
      api_path = fsi->api_path;
      std::uint64_t file_size = 0u;
      api_meta_map meta;
      if ((error = oft_->derive_item_data(fsi->api_path, fsi->directory, file_size, meta)) ==
          api_error::success) {
        populate_file_info(file_size, meta, *fileInfo);
      }
    }
  }

  const auto ret = utils::translate_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

std::uint64_t winfsp_drive::get_file_size(const std::string &api_path) const {
  std::uint64_t file_size = 0u;
  oft_->derive_file_size(api_path, file_size);
  return file_size;
}

api_error winfsp_drive::get_item_meta(const std::string &api_path, api_meta_map &meta) const {
  return oft_->derive_item_data(api_path, meta);
}

api_error winfsp_drive::get_item_meta(const std::string &api_path, const std::string &name,
                                      std::string &value) const {
  api_meta_map meta{};
  const auto ret = oft_->derive_item_data(api_path, meta);
  if (ret == api_error::success) {
    value = meta[name];
  }

  return ret;
}

NTSTATUS winfsp_drive::get_security_by_name(PWSTR fileName, PUINT32 attributes,
                                            PSECURITY_DESCRIPTOR descriptor,
                                            std::uint64_t *descriptor_size) {
  const auto api_path = utils::path::create_api_path(utils::string::to_utf8(fileName));

  api_meta_map meta;
  auto error = oft_->derive_item_data(api_path, meta);
  if (error == api_error::success) {
    if (attributes) {
      *attributes = utils::get_attributes_from_meta(meta);
    }

    if (descriptor_size) {
      ULONG sz = 0;
      PSECURITY_DESCRIPTOR sd = nullptr;
      if (::ConvertStringSecurityDescriptorToSecurityDescriptor(
              "O:BAG:BAD:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;FA;;;WD)", SDDL_REVISION_1, &sd, &sz)) {
        if (sz > *descriptor_size) {
          error = api_error::buffer_too_small;
        } else {
          ::CopyMemory(descriptor, sd, sz);
        }
        *descriptor_size = sz;
        ::LocalFree(sd);
      } else {
        error = api_error::os_error;
      }
    }
  }
  return utils::translate_api_error(error);
}

NTSTATUS winfsp_drive::GetSecurityByName(PWSTR fileName, PUINT32 attributes,
                                         PSECURITY_DESCRIPTOR descriptor, SIZE_T *descriptor_size) {
  const auto api_path = utils::path::create_api_path(utils::string::to_utf8(fileName));
  std::uint64_t sds = descriptor_size ? *descriptor_size : 0;
  const auto ret =
      get_security_by_name(fileName, attributes, descriptor, descriptor_size ? &sds : nullptr);
  if (descriptor_size) {
    *descriptor_size = static_cast<SIZE_T>(sds);
  }
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

std::uint64_t winfsp_drive::get_total_drive_space() const {
  return provider_.get_total_drive_space();
}

std::uint64_t winfsp_drive::get_total_item_count() const {
  return provider_.get_total_item_count();
}

std::uint64_t winfsp_drive::get_used_drive_space() const {
  return provider_.get_used_drive_space();
}

void winfsp_drive::get_volume_info(UINT64 &total_size, UINT64 &free_size,
                                   std::string &volume_label) const {
  const auto total_bytes = provider_.get_total_drive_space();
  const auto total_used = provider_.get_used_drive_space();
  free_size = total_bytes - total_used;
  total_size = total_bytes;
  volume_label = utils::create_volume_label(config_.get_provider_type());
}

NTSTATUS winfsp_drive::GetVolumeInfo(VolumeInfo *volume_info) {
  const std::wstring volume_label =
      utils::string::from_utf8(utils::create_volume_label(config_.get_provider_type()));
  const auto total_bytes = provider_.get_total_drive_space();
  const auto total_used = provider_.get_used_drive_space();
  volume_info->FreeSize = total_bytes - total_used;
  volume_info->TotalSize = total_bytes;
  wcscpy_s(&volume_info->VolumeLabel[0], 32, &volume_label[0]);
  volume_info->VolumeLabelLength =
      std::min(static_cast<UINT16>(64), static_cast<UINT16>(volume_label.size() * sizeof(WCHAR)));

  RAISE_WINFSP_EVENT(__FUNCTION__, utils::string::to_utf8(volume_label), 0);
  return STATUS_SUCCESS;
}

NTSTATUS winfsp_drive::Init(PVOID host) {
  auto *fileSystemHost = reinterpret_cast<FileSystemHost *>(host);
  if (not config_.get_enable_mount_manager()) {
    fileSystemHost->SetPrefix(
        &(L"\\repertory\\" + std::wstring(fileSystemHost->FileSystemName()).substr(0, 1))[0u]);
  }
  fileSystemHost->SetFileSystemName(REPERTORY_W);
  fileSystemHost->SetFlushAndPurgeOnCleanup(TRUE);
  fileSystemHost->SetReparsePoints(FALSE);
  fileSystemHost->SetReparsePointsAccessCheck(FALSE);
  fileSystemHost->SetSectorSize(WINFSP_ALLOCATION_UNIT);
  fileSystemHost->SetSectorsPerAllocationUnit(1);
  fileSystemHost->SetFileInfoTimeout(1000);
  fileSystemHost->SetCaseSensitiveSearch(TRUE);
  fileSystemHost->SetCasePreservedNames(TRUE);
  fileSystemHost->SetNamedStreams(FALSE);
  fileSystemHost->SetUnicodeOnDisk(TRUE);
  fileSystemHost->SetPersistentAcls(FALSE);
  fileSystemHost->SetPostCleanupWhenModifiedOnly(TRUE);
  fileSystemHost->SetPassQueryDirectoryPattern(FALSE);
  fileSystemHost->SetVolumeCreationTime(utils::get_file_time_now());
  fileSystemHost->SetVolumeSerialNumber(0);
  return STATUS_SUCCESS;
}

int winfsp_drive::mount(const std::vector<std::string> &drive_args) {
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

NTSTATUS winfsp_drive::Mounted(PVOID host) {
  auto ret = STATUS_SUCCESS;
  utils::file::change_to_process_directory();

  auto *file_system_host = reinterpret_cast<FileSystemHost *>(host);
  polling::instance().start(&config_);
  download_manager_ = std::make_unique<download_manager>(
      config_,
      [this](const std::string &path, const std::size_t &size, const std::uint64_t &offset,
             std::vector<char> &data, const bool &stop_requested) -> api_error {
        return provider_.read_file_bytes(path, size, offset, data, stop_requested);
      });
  oft_ = std::make_unique<open_file_table<open_file_data>>(provider_, config_, *download_manager_);
  server_ = std::make_unique<full_server>(config_, provider_, *oft_);
  eviction_ = std::make_unique<eviction>(provider_, config_, *oft_);
  try {
    server_->start();
    if (provider_.start(
            [this](const std::string &api_path, const std::string &api_parent,
                   const std::string &source, const bool &directory,
                   const std::uint64_t &createDate, const std::uint64_t &accessDate,
                   const std::uint64_t &modifiedDate, const std::uint64_t &changedDate) -> void {
              event_system::instance().raise<filesystem_item_added>(api_path, api_parent,
                                                                    directory);
              provider_.set_item_meta(
                  api_path,
                  {{META_ATTRIBUTES,
                    std::to_string(directory ? FILE_ATTRIBUTE_DIRECTORY
                                             : FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE)},
                   {META_CREATION, std::to_string(createDate)},
                   {META_WRITTEN, std::to_string(modifiedDate)},
                   {META_MODIFIED, std::to_string(modifiedDate)},
                   {META_ACCESSED, std::to_string(accessDate)}});
              if (not directory && not source.empty()) {
                provider_.set_source_path(api_path, source);
              }
            },
            oft_.get())) {
      throw startup_exception("provider is offline");
    }
    download_manager_->start(oft_.get());
    oft_->start();
    eviction_->start();

    // Force root creation
    api_meta_map meta{};
    provider_.get_item_meta("/", meta);

    const auto mount_location = parse_mount_location(file_system_host->MountPoint());
    if (config_.get_enable_remote_mount()) {
      remote_server_ =
          std::make_unique<remote_winfsp::remote_server>(config_, *this, mount_location);
    }

    lock_.set_mount_state(true, mount_location, ::GetCurrentProcessId());
    event_system::instance().raise<drive_mounted>(mount_location);
  } catch (const std::exception &e) {
    event_system::instance().raise<repertory_exception>(__FUNCTION__, e.what());
    if (remote_server_) {
      remote_server_.reset();
    }
    server_->stop();
    polling::instance().stop();
    eviction_->stop();
    oft_->stop();
    download_manager_->stop();
    provider_.stop();
    ret = STATUS_INTERNAL_ERROR;
  }

  return ret;
}

NTSTATUS winfsp_drive::Open(PWSTR file_name, UINT32 create_options, UINT32 granted_access,
                            PVOID *file_node, PVOID *file_desc, OpenFileInfo *ofi) {
  const auto api_path = utils::path::create_api_path(utils::string::to_utf8(file_name));
  const auto directory = provider_.is_directory(api_path);

  auto error = api_error::item_is_file;
  if ((create_options & FILE_DIRECTORY_FILE) && not directory) {
  } else {
    error = api_error::success;
    if (not directory && (((granted_access & FILE_WRITE_DATA) == FILE_WRITE_DATA) ||
                          ((granted_access & GENERIC_WRITE) == GENERIC_WRITE))) {
      error =
          provider_.is_file_writeable(api_path) ? api_error::success : api_error::permission_denied;
    }

    if (error == api_error::success) {
      open_file_data ofd{};
      std::uint64_t handle;
      error = oft_->Open(api_path, directory, ofd, handle);
      if (error == api_error::success) {
        std::uint64_t file_size = 0u;
        api_meta_map meta;
        if ((error = oft_->derive_item_data(api_path, directory, file_size, meta)) ==
            api_error::success) {
          populate_file_info(api_path, file_size, meta, *ofi);
          *file_desc = reinterpret_cast<PVOID>(handle);
        }
      }
    }
  }

  const auto ret = utils::translate_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

NTSTATUS winfsp_drive::Overwrite(PVOID file_node, PVOID file_desc, UINT32 attributes,
                                 BOOLEAN replace_attributes, UINT64 allocation_size,
                                 FileInfo *file_info) {
  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    filesystem_item *fsi = nullptr;
    if (oft_->get_open_file(handle, fsi)) {
      api_path = fsi->api_path;
      std::uint64_t file_size;
      api_meta_map meta{};
      if ((error = oft_->derive_item_data(api_path, false, file_size, meta)) ==
          api_error::success) {
        error = api_error::file_in_use;
        if (not download_manager_->is_processing(api_path)) {
          error = api_error::success;
          auto update_cache_space = true;
          if (fsi->handle != REPERTORY_INVALID_HANDLE) {
            recur_mutex_lock l(*fsi->lock);
            auto nf = native_file::attach(fsi->handle);
            if (nf->truncate(0)) {
              global_data::instance().update_used_space(file_size, 0, true);
              nf->close();
              fsi->handle = REPERTORY_INVALID_HANDLE;
              update_cache_space = false;
            } else {
              error = api_error::os_error;
            }
          }

          utils::file::delete_file(fsi->source_path);

          if ((error == api_error::success) &&
              ((error = provider_.remove_file(api_path)) == api_error::success)) {
            if (update_cache_space) {
              global_data::instance().update_used_space(file_size, 0, false);
            } else {
              global_data::instance().decrement_used_drive_space(file_size);
            }

            fsi->changed = false;
            file_size = fsi->size = 0u;
            if ((error = provider_.create_file(api_path, meta)) == api_error::success) {
              filesystem_item existing_fsi{};
              if ((error = provider_.get_filesystem_item(api_path, false, existing_fsi)) ==
                  api_error::success) {
                fsi->source_path = existing_fsi.source_path;
                fsi->source_path_changed = false;
                utils::file::delete_file(fsi->source_path);

                // Handle replace attributes
                if (replace_attributes) {
                  if (not attributes) {
                    attributes = FILE_ATTRIBUTE_NORMAL;
                  }
                  meta[META_ATTRIBUTES] = std::to_string(attributes);
                  error = oft_->set_item_meta(api_path, meta);
                } else if (attributes) { // Handle merge attributes
                  const auto current_attributes = utils::get_attributes_from_meta(meta);
                  const auto merged_attributes = attributes | current_attributes;
                  if (merged_attributes != current_attributes) {
                    meta[META_ATTRIBUTES] = std::to_string(merged_attributes);
                    error = oft_->set_item_meta(api_path, meta);
                  }
                }
              }
            }
          }
        }

        // Populate file information
        populate_file_info(file_size, meta, *file_info);
      }
    }
  }

  const auto ret = utils::translate_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

std::string winfsp_drive::parse_mount_location(const std::wstring &mount_location) {
  return utils::string::to_utf8(
      (mount_location.substr(0, 4) == L"\\\\.\\") ? mount_location.substr(4) : mount_location);
}

void winfsp_drive::populate_file_info(const std::string &api_path, const std::uint64_t &file_size,
                                      const api_meta_map &meta, FSP_FSCTL_OPEN_FILE_INFO &ofi) {
  const auto filePath = utils::string::from_utf8(utils::string::replace_copy(api_path, '/', '\\'));

  wcscpy_s(ofi.NormalizedName, ofi.NormalizedNameSize / sizeof(WCHAR), &filePath[0]);
  ofi.NormalizedNameSize = (UINT16)(wcslen(&filePath[0]) * sizeof(WCHAR));
  populate_file_info(file_size, meta, ofi.FileInfo);
}

api_error winfsp_drive::populate_file_info(const std::string &api_path,
                                           remote::file_info &fileInfo) {
  std::uint64_t file_size = 0u;
  api_meta_map meta{};
  const auto ret =
      oft_->derive_item_data(api_path, provider_.is_directory(api_path), file_size, meta);
  if (ret == api_error::success) {
    FSP_FSCTL_FILE_INFO fi{};
    populate_file_info(file_size, meta, fi);
    set_file_info(fileInfo, fi);
  }

  return ret;
}

void winfsp_drive::populate_file_info(const std::uint64_t &file_size, api_meta_map meta,
                                      FSP_FSCTL_FILE_INFO &file_info) {
  file_info.FileSize = file_size;
  file_info.AllocationSize =
      utils::divide_with_ceiling(file_size ? file_size : WINFSP_ALLOCATION_UNIT,
                                 WINFSP_ALLOCATION_UNIT) *
      WINFSP_ALLOCATION_UNIT;
  file_info.ChangeTime = utils::get_changed_time_from_meta(meta);
  file_info.CreationTime = utils::get_creation_time_from_meta(meta);
  file_info.FileAttributes = utils::get_attributes_from_meta(meta);
  file_info.HardLinks = 0;
  file_info.IndexNumber = 0;
  file_info.LastAccessTime = utils::get_accessed_time_from_meta(meta);
  file_info.LastWriteTime = utils::get_written_time_from_meta(meta);
  file_info.ReparseTag = 0;
  file_info.EaSize = 0;
}

NTSTATUS winfsp_drive::Read(PVOID file_node, PVOID file_desc, PVOID buffer, UINT64 offset,
                            ULONG length, PULONG bytes_transferred) {
  *bytes_transferred = 0u;

  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    if (length > 0u) {
      filesystem_item *fsi = nullptr;
      if (oft_->get_open_file(handle, fsi)) {
        api_path = fsi->api_path;
        std::vector<char> data;
        if ((error = download_manager_->read_bytes(handle, *fsi, length, offset, data)) ==
            api_error::success) {
          *bytes_transferred = static_cast<ULONG>(data.size());
          if ((length > 0) && (data.size() != length)) {
            ::SetLastError(ERROR_HANDLE_EOF);
          }

          if (not data.empty()) {
            ::CopyMemory(buffer, &data[0u], data.size());
            data.clear();
            oft_->set_item_meta(api_path, META_ACCESSED,
                                std::to_string(utils::get_file_time_now()));
          }
        }
      }
    } else {
      error = api_error::success;
    }
  }

  const auto ret = utils::translate_api_error(error);
  if (ret != STATUS_SUCCESS) {
    RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  }
  return ret;
}

NTSTATUS winfsp_drive::ReadDirectory(PVOID file_node, PVOID file_desc, PWSTR pattern, PWSTR marker,
                                     PVOID buffer, ULONG buffer_length, PULONG bytes_transferred) {
  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    filesystem_item *fsi = nullptr;
    if (oft_->get_open_file(handle, fsi)) {
      api_path = fsi->api_path;
      if (provider_.is_directory(api_path)) {
        directory_item_list list{};
        if ((error = provider_.get_directory_items(api_path, list)) == api_error::success) {
          directory_iterator iterator(std::move(list));
          auto status_result = STATUS_SUCCESS;
          auto *directory_buffer = fsi->open_data[handle].directory_buffer;
          if (FspFileSystemAcquireDirectoryBuffer(
                  &directory_buffer, static_cast<BOOLEAN>(nullptr == marker), &status_result)) {
            directory_item di{};
            auto offset =
                marker ? iterator.get_next_directory_offset(utils::path::create_api_path(
                             utils::path::combine(api_path, {utils::string::to_utf8(marker)})))
                       : 0u;
            while ((error = iterator.get_directory_item(offset++, di)) == api_error::success) {
              if (not utils::path::is_ads_file_path(di.api_path)) {
                const auto display_name =
                    utils::string::from_utf8(utils::path::strip_to_file_name(di.api_path));
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

                if (not di.meta.empty() || ((di.api_path != ".") && (di.api_path != ".."))) {
                  populate_file_info(di.size, di.meta, directory_info->FileInfo);
                }
                ::wcscpy_s(&directory_info->FileNameBuf[0], MAX_PATH, &display_name[0]);

                FspFileSystemFillDirectoryBuffer(&directory_buffer, directory_info, &status_result);
              }
            }

            FspFileSystemReleaseDirectoryBuffer(&directory_buffer);
          }

          if (status_result == STATUS_SUCCESS) {
            FspFileSystemReadDirectoryBuffer(&directory_buffer, marker, buffer, buffer_length,
                                             bytes_transferred);
            if (error == api_error::directory_end_of_files) {
              error = api_error::success;
            }
          } else {
            RAISE_WINFSP_EVENT(__FUNCTION__, api_path, status_result);
            return status_result;
          }
        }
      } else {
        error = api_error::directory_not_found;
      }
    }
  }

  const auto ret = utils::translate_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

NTSTATUS winfsp_drive::Rename(PVOID file_node, PVOID file_desc, PWSTR file_name,
                              PWSTR new_file_name, BOOLEAN replace_if_exists) {
  const auto from_api_path = utils::path::create_api_path(utils::string::to_utf8(file_name));
  const auto to_api_path = utils::path::create_api_path(utils::string::to_utf8(new_file_name));

  auto error = api_error::item_not_found;
  if (provider_.is_file(from_api_path)) {
    error = oft_->rename_file(from_api_path, to_api_path, replace_if_exists);
  } else if (provider_.is_directory(from_api_path)) {
    error = oft_->rename_directory(from_api_path, to_api_path);
  }

  const auto ret = utils::translate_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, from_api_path + "|" + to_api_path, ret);
  return ret;
}

NTSTATUS winfsp_drive::SetBasicInfo(PVOID file_node, PVOID file_desc, UINT32 attributes,
                                    UINT64 creation_time, UINT64 last_access_time,
                                    UINT64 last_write_time, UINT64 change_time,
                                    FileInfo *fileInfo) {
  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    filesystem_item *fsi = nullptr;
    if (oft_->get_open_file(handle, fsi)) {
      api_path = fsi->api_path;
      if (attributes == INVALID_FILE_ATTRIBUTES) {
        attributes = 0;
      } else if (attributes == 0) {
        attributes = fsi->directory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
      }

      api_meta_map meta;
      if (attributes) {
        if ((attributes & FILE_ATTRIBUTE_NORMAL) && (attributes != FILE_ATTRIBUTE_NORMAL)) {
          attributes &= ~(FILE_ATTRIBUTE_NORMAL);
        }
        meta[META_ATTRIBUTES] = std::to_string(attributes);
      }
      if (creation_time && (creation_time != 0xFFFFFFFFFFFFFFFF)) {
        meta[META_CREATION] = std::to_string(creation_time);
      }
      if (last_access_time && (last_access_time != 0xFFFFFFFFFFFFFFFF)) {
        meta[META_ACCESSED] = std::to_string(last_access_time);
      }
      if (last_write_time && (last_write_time != 0xFFFFFFFFFFFFFFFF)) {
        meta[META_WRITTEN] = std::to_string(last_write_time);
      }
      if (change_time && (change_time != 0xFFFFFFFFFFFFFFFF)) {
        meta[META_MODIFIED] = std::to_string(change_time);
      }

      error = oft_->set_item_meta(api_path, meta);

      // Populate file information
      std::uint64_t file_size = 0;
      if (oft_->derive_item_data(api_path, false, file_size, meta) == api_error::success) {
        populate_file_info(file_size, meta, *fileInfo);
      }
    }
  }

  const auto ret = utils::translate_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

void winfsp_drive::set_file_info(remote::file_info &dest, const FSP_FSCTL_FILE_INFO &src) {
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

NTSTATUS winfsp_drive::SetFileSize(PVOID file_node, PVOID file_desc, UINT64 new_size,
                                   BOOLEAN set_allocation_size, FileInfo *file_info) {
  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    filesystem_item *fsi = nullptr;
    if (oft_->get_open_file(handle, fsi)) {
      api_path = fsi->api_path;
      i_download::allocator_callback allocator;
      if (set_allocation_size) {
        allocator = [&]() -> api_error {
          std::string meta_allocation_size;
          utils::calculate_allocation_size(false, 0, new_size, meta_allocation_size);

          FILE_ALLOCATION_INFO allocation_info{};
          allocation_info.AllocationSize.QuadPart = new_size;
          return ::SetFileInformationByHandle(fsi->handle, FileAllocationInfo, &allocation_info,
                                              sizeof(allocation_info))
                     ? api_error::success
                     : api_error::os_error;
        };
      } else {
        allocator = [&]() -> api_error {
          FILE_END_OF_FILE_INFO end_of_file_info{};
          end_of_file_info.EndOfFile.QuadPart = new_size;
          return ::SetFileInformationByHandle(fsi->handle, FileEndOfFileInfo, &end_of_file_info,
                                              sizeof(end_of_file_info))
                     ? api_error::success
                     : api_error::os_error;
        };
      }

      error = download_manager_->allocate(handle, *fsi, new_size, allocator);
      if (file_info) {
        // Populate file information
        std::uint64_t file_size = 0u;
        api_meta_map meta;
        if (oft_->derive_item_data(api_path, false, file_size, meta) == api_error::success) {
          populate_file_info(file_size, meta, *file_info);
        }
      }
    }
  }

  const auto ret = utils::translate_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

VOID winfsp_drive::Unmounted(PVOID host) {
  auto *file_system_host = reinterpret_cast<FileSystemHost *>(host);
  const auto mount_location = parse_mount_location(file_system_host->MountPoint());
  event_system::instance().raise<drive_unmount_pending>(mount_location);
  if (remote_server_) {
    remote_server_.reset();
  }
  server_->stop();
  polling::instance().stop();
  eviction_->stop();
  oft_->stop();
  download_manager_->stop();
  provider_.stop();
  server_.reset();
  download_manager_.reset();
  oft_.reset();
  eviction_.reset();

  lock_.set_mount_state(false, "", -1);
  event_system::instance().raise<drive_unmounted>(mount_location);
  config_.save();
}

NTSTATUS winfsp_drive::Write(PVOID file_node, PVOID file_desc, PVOID buffer, UINT64 offset,
                             ULONG length, BOOLEAN write_to_end, BOOLEAN constrained_io,
                             PULONG bytes_transferred, FileInfo *file_info) {
  *bytes_transferred = 0;

  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    filesystem_item *fsi = nullptr;
    if (oft_->get_open_file(handle, fsi)) {
      api_path = fsi->api_path;
      if (write_to_end) {
        offset = fsi->size;
      }

      auto should_write = true;
      if (constrained_io) {
        if (offset >= fsi->size) {
          error = api_error::success;
          should_write = false;
        } else if (offset + length > fsi->size) {
          length = static_cast<ULONG>(fsi->size - offset);
        }
      }

      if (should_write) {
        if (length > 0u) {
          std::size_t bytes_written = 0u;
          std::vector<char> data(length);
          std::memcpy(&data[0], buffer, length);
          if ((error = download_manager_->write_bytes(handle, *fsi, offset, data, bytes_written)) ==
              api_error::success) {
            *bytes_transferred = static_cast<ULONG>(bytes_written);

            // Populate file information
            api_meta_map meta;
            if (oft_->derive_item_data(api_path, meta) == api_error::success) {
              populate_file_info(fsi->size, meta, *file_info);
            }
          }
        }
      } else {
        error = api_error::success;
      }
    }
  }

  const auto ret = utils::translate_api_error(error);
  if (ret != STATUS_SUCCESS) {
    RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  }
  return ret;
}
} // namespace repertory

#endif // _WIN32
