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
#ifdef _WIN32

#include "drives/winfsp/winfsp_drive.hpp"

#include "app_config.hpp"
#include "drives/directory_iterator.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/consumers/logging_consumer.hpp"
#include "events/events.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "types/startup_exception.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/polling.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
// clang-format off
E_SIMPLE3(winfsp_event, debug, true,
  std::string, function, func, E_STRING,
  std::string, api_path, ap, E_STRING,
  NTSTATUS, result, res, E_FROM_INT32
);
// clang-format on

#define RAISE_WINFSP_EVENT(func, file, ret)                                    \
  if (config_.get_enable_drive_events() &&                                     \
      (((config_.get_event_level() >= winfsp_event::level) &&                  \
        (ret != STATUS_SUCCESS)) ||                                            \
       (config_.get_event_level() >= event_level::verbose)))                   \
  event_system::instance().raise<winfsp_event>(func, file, ret)

winfsp_drive::winfsp_service::winfsp_service(
    lock_data &lock, winfsp_drive &drive, std::vector<std::string> drive_args,
    app_config &config)
    : Service((PWSTR)L"Repertory"),
      lock_(lock),
      drive_(drive),
      drive_args_(std::move(drive_args)),
      host_(drive),
      config_(config) {}

auto winfsp_drive::winfsp_service::OnStart(ULONG, PWSTR *) -> NTSTATUS {
  const auto mount_location = utils::string::to_lower(
      utils::path::absolute((drive_args_.size() > 1u) ? drive_args_[1u] : ""));
  const auto drive_letter =
      ((mount_location.size() == 2u) ||
       ((mount_location.size() == 3u) && (mount_location[2u] == '\\'))) &&
      (mount_location[1u] == ':');

  auto ret = drive_letter ? STATUS_DEVICE_BUSY : STATUS_NOT_SUPPORTED;
  if ((drive_letter && not utils::file::is_directory(mount_location))) {
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
    if (not lock_.set_mount_state(false, "", -1)) {
      utils::error::raise_error(__FUNCTION__, ret, "failed to set mount state");
    }
    event_system::instance().raise<drive_mount_failed>(mount_location,
                                                       std::to_string(ret));
  }

  return ret;
}

auto winfsp_drive::winfsp_service::OnStop() -> NTSTATUS {
  host_.Unmount();

  if (not lock_.set_mount_state(false, "", -1)) {
    utils::error::raise_error(__FUNCTION__, "failed to set mount state");
  }

  return STATUS_SUCCESS;
}

winfsp_drive::winfsp_drive(app_config &config, lock_data &lock,
                           i_provider &provider)
    : FileSystemBase(), provider_(provider), config_(config), lock_(lock) {
  E_SUBSCRIBE_EXACT(unmount_requested, [this](const unmount_requested & /*e*/) {
    std::thread([this]() { this->shutdown(); }).detach();
  });
}

void winfsp_drive::shutdown() { ::GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0); }

auto winfsp_drive::CanDelete(PVOID /*file_node*/, PVOID file_desc,
                             PWSTR /*file_name*/) -> NTSTATUS {
  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle =
      static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    std::shared_ptr<i_open_file> f;
    if (fm_->get_open_file(handle, false, f)) {
      api_path = f->get_api_path();
      if (f->is_directory()) {
        error = provider_.get_directory_item_count(api_path)
                    ? api_error::directory_not_empty
                    : api_error::success;
      } else {
        error = fm_->is_processing(api_path) ? api_error::file_in_use
                                             : api_error::success;
      }
    }
  }

  auto ret = utils::from_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

VOID winfsp_drive::Cleanup(PVOID file_node, PVOID file_desc,
                           PWSTR /*file_name*/, ULONG flags) {
  std::string api_path;
  auto handle =
      static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    std::shared_ptr<i_open_file> f;
    if (fm_->get_open_file(handle, false, f)) {
      api_path = f->get_api_path();

      const auto directory = f->is_directory();
      if (flags & FspCleanupDelete) {
        auto *directory_buffer = f->get_open_data(handle).directory_buffer;
        f.reset();

        if (directory_buffer) {
          FspFileSystemDeleteDirectoryBuffer(&directory_buffer);
        }

        if (directory) {
          auto res = provider_.remove_directory(api_path);
          if (res != api_error::success) {
            utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                               "failed to remove directory");
          }
        } else {
          auto res = fm_->remove_file(api_path);
          if (res != api_error::success) {
            utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                               "failed to remove file");
          }
        }
      } else {
        if ((flags & FspCleanupSetArchiveBit) && not directory) {
          api_meta_map meta;
          if (provider_.get_item_meta(api_path, meta) == api_error::success) {
            auto res = provider_.set_item_meta(
                api_path, META_ATTRIBUTES,
                std::to_string(utils::get_attributes_from_meta(meta) |
                               FILE_ATTRIBUTE_ARCHIVE));
            if (res != api_error::success) {
              utils::error::raise_api_path_error(
                  __FUNCTION__, api_path, res, "failed to set meta attributes");
            }
          }
        }

        if (flags & (FspCleanupSetLastAccessTime | FspCleanupSetLastWriteTime |
                     FspCleanupSetChangeTime)) {
          const auto now = utils::get_file_time_now();
          if (flags & FspCleanupSetLastAccessTime) {
            auto res = provider_.set_item_meta(api_path, META_ACCESSED,
                                               std::to_string(now));
            if (res != api_error::success) {
              utils::error::raise_api_path_error(
                  __FUNCTION__, api_path, res,
                  "failed to set meta accessed time");
            }
          }

          if (flags & FspCleanupSetLastWriteTime) {
            auto res = provider_.set_item_meta(api_path, META_WRITTEN,
                                               std::to_string(now));
            if (res != api_error::success) {
              utils::error::raise_api_path_error(
                  __FUNCTION__, api_path, res,
                  "failed to set meta written time");
            }
          }

          if (flags & FspCleanupSetChangeTime) {
            auto res = provider_.set_item_meta(
                api_path, {{META_CHANGED, std::to_string(now)},
                           {META_MODIFIED, std::to_string(now)}});
            if (res != api_error::success) {
              utils::error::raise_api_path_error(
                  __FUNCTION__, api_path, res,
                  "failed to set meta modified time");
            }
          }
        }

        if (flags & FspCleanupSetAllocationSize) {
          UINT64 allocation_size =
              utils::divide_with_ceiling(f->get_file_size(),
                                         WINFSP_ALLOCATION_UNIT) *
              WINFSP_ALLOCATION_UNIT;
          SetFileSize(file_node, file_desc, allocation_size, TRUE, nullptr);
        }
      }
    }
  }
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, 0);
}

VOID winfsp_drive::Close(PVOID /*file_node*/, PVOID file_desc) {
  std::string api_path;
  auto handle =
      static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    PVOID directory_buffer = nullptr;

    std::shared_ptr<i_open_file> f;
    if (fm_->get_open_file(handle, false, f)) {
      api_path = f->get_api_path();
      directory_buffer = f->get_open_data(handle).directory_buffer;
      f.reset();
    }
    fm_->close(handle);
    if (directory_buffer) {
      FspFileSystemDeleteDirectoryBuffer(&directory_buffer);
    }
  }
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, 0);
}

auto winfsp_drive::Create(PWSTR fileName, UINT32 create_options,
                          UINT32 /*grantedAccess*/, UINT32 attributes,
                          PSECURITY_DESCRIPTOR /*descriptor*/,
                          UINT64 /*allocation_size*/, PVOID * /*file_node*/,
                          PVOID *file_desc, OpenFileInfo *ofi) -> NTSTATUS {
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

  const auto now = utils::get_file_time_now();
  auto meta = create_meta_attributes(
      now, attributes, now, now, attributes & FILE_ATTRIBUTE_DIRECTORY, "", 0U,
      "", 0U, now, 0U, 0U, 0U,
      (attributes & FILE_ATTRIBUTE_DIRECTORY)
          ? ""
          : utils::path::combine(config_.get_cache_directory(),
                                 {utils::create_uuid_string()}),
      0U, now);

  const auto api_path =
      utils::path::create_api_path(utils::string::to_utf8(fileName));

  open_file_data ofd{};
  std::uint64_t handle{};
  std::shared_ptr<i_open_file> f;
  if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
    error = provider_.create_directory(api_path, meta);
    if (error == api_error::success) {
      error = fm_->open(api_path, true, ofd, handle, f);
    }
  } else {
    error = fm_->create(api_path, meta, ofd, handle, f);
  }

  if (error == api_error::success) {
    populate_file_info(api_path, 0, meta, *ofi);
    *file_desc = reinterpret_cast<PVOID>(handle);
  }

  auto ret = utils::from_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

auto winfsp_drive::Flush(PVOID /*file_node*/, PVOID file_desc,
                         FileInfo *fileInfo) -> NTSTATUS {
  std::string api_path;
  auto error = api_error::success;
  auto handle =
      static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    error = api_error::invalid_handle;
    std::shared_ptr<i_open_file> f;
    if (fm_->get_open_file(handle, false, f)) {
      api_path = f->get_api_path();
      error = f->native_operation([&](native_handle handle) {
        if (not ::FlushFileBuffers(handle)) {
          return api_error::os_error;
        }

        return api_error::success;
      });

      // Populate file information
      api_meta_map meta;
      if (provider_.get_item_meta(api_path, meta) == api_error::success) {
        populate_file_info(f->get_file_size(), meta, *fileInfo);
      }
    }
  }

  auto ret = utils::from_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

auto winfsp_drive::get_directory_item_count(const std::string &api_path) const
    -> std::uint64_t {
  return provider_.get_directory_item_count(api_path);
}

auto winfsp_drive::get_directory_items(const std::string &api_path) const
    -> directory_item_list {
  directory_item_list list;
  auto res = provider_.get_directory_items(api_path, list);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                       "failed to get directory list");
  }
  return list;
}

auto winfsp_drive::GetFileInfo(PVOID /*file_node*/, PVOID file_desc,
                               FileInfo *fileInfo) -> NTSTATUS {
  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle =
      static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    std::shared_ptr<i_open_file> f;
    if (fm_->get_open_file(handle, false, f)) {
      api_path = f->get_api_path();
      api_meta_map meta;
      if ((error = provider_.get_item_meta(api_path, meta)) ==
          api_error::success) {
        populate_file_info(f->get_file_size(), meta, *fileInfo);
      }
    }
  }

  auto ret = utils::from_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

auto winfsp_drive::get_file_size(const std::string &api_path) const
    -> std::uint64_t {
  std::uint64_t file_size{};
  auto res = provider_.get_file_size(api_path, file_size);
  if (res != api_error::success) {
    utils::error::raise_api_path_error(__FUNCTION__, api_path, res,
                                       "failed to get file size");
  }

  return file_size;
}

auto winfsp_drive::get_item_meta(const std::string &api_path,
                                 api_meta_map &meta) const -> api_error {
  return provider_.get_item_meta(api_path, meta);
}

auto winfsp_drive::get_item_meta(const std::string &api_path,
                                 const std::string &name,
                                 std::string &value) const -> api_error {
  api_meta_map meta{};
  auto ret = provider_.get_item_meta(api_path, meta);
  if (ret == api_error::success) {
    value = meta[name];
  }

  return ret;
}

auto winfsp_drive::get_security_by_name(PWSTR fileName, PUINT32 attributes,
                                        PSECURITY_DESCRIPTOR descriptor,
                                        std::uint64_t *descriptor_size)
    -> NTSTATUS {
  const auto api_path =
      utils::path::create_api_path(utils::string::to_utf8(fileName));

  api_meta_map meta{};
  auto error = provider_.get_item_meta(api_path, meta);
  if (error == api_error::success) {
    if (attributes) {
      *attributes = utils::get_attributes_from_meta(meta);
    }

    if (descriptor_size) {
      ULONG sz{};
      PSECURITY_DESCRIPTOR sd{};

      if (::ConvertStringSecurityDescriptorToSecurityDescriptor(
              "O:BAG:BAD:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;FA;;;WD)",
              SDDL_REVISION_1, &sd, &sz)) {
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

  return utils::from_api_error(error);
}

auto winfsp_drive::GetSecurityByName(PWSTR fileName, PUINT32 attributes,
                                     PSECURITY_DESCRIPTOR descriptor,
                                     SIZE_T *descriptor_size) -> NTSTATUS {
  const auto api_path =
      utils::path::create_api_path(utils::string::to_utf8(fileName));
  std::uint64_t sds = descriptor_size ? *descriptor_size : 0U;
  auto ret = get_security_by_name(fileName, attributes, descriptor,
                                  sds > 0U ? &sds : nullptr);
  if (sds) {
    *descriptor_size = static_cast<SIZE_T>(sds);
  }
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

auto winfsp_drive::get_total_drive_space() const -> std::uint64_t {
  return provider_.get_total_drive_space();
}

auto winfsp_drive::get_total_item_count() const -> std::uint64_t {
  return provider_.get_total_item_count();
}

auto winfsp_drive::get_used_drive_space() const -> std::uint64_t {
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

auto winfsp_drive::GetVolumeInfo(VolumeInfo *volume_info) -> NTSTATUS {
  const std::wstring volume_label = utils::string::from_utf8(
      utils::create_volume_label(config_.get_provider_type()));
  const auto total_bytes = provider_.get_total_drive_space();
  const auto total_used = provider_.get_used_drive_space();
  volume_info->FreeSize = total_bytes - total_used;
  volume_info->TotalSize = total_bytes;
  wcscpy_s(&volume_info->VolumeLabel[0U], 32, &volume_label[0U]);
  volume_info->VolumeLabelLength =
      std::min(static_cast<UINT16>(64),
               static_cast<UINT16>(volume_label.size() * sizeof(WCHAR)));

  RAISE_WINFSP_EVENT(__FUNCTION__, utils::string::to_utf8(volume_label), 0);
  return STATUS_SUCCESS;
}

auto winfsp_drive::Init(PVOID host) -> NTSTATUS {
  auto *file_system_host = reinterpret_cast<FileSystemHost *>(host);
  if (not config_.get_enable_mount_manager()) {
    file_system_host->SetPrefix(
        &(L"\\repertory\\" +
          std::wstring(file_system_host->FileSystemName()).substr(0, 1))[0u]);
  }
  file_system_host->SetFileSystemName(REPERTORY_W);
  file_system_host->SetFlushAndPurgeOnCleanup(TRUE);
  file_system_host->SetReparsePoints(FALSE);
  file_system_host->SetReparsePointsAccessCheck(FALSE);
  file_system_host->SetSectorSize(WINFSP_ALLOCATION_UNIT);
  file_system_host->SetSectorsPerAllocationUnit(1);
  file_system_host->SetFileInfoTimeout(1000);
  file_system_host->SetCaseSensitiveSearch(TRUE);
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

auto winfsp_drive::mount(const std::vector<std::string> &drive_args) -> int {
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
  auto svc = winfsp_service(lock_, *this, parsed_drive_args, config_);
  auto ret = svc.Run();

  event_system::instance().raise<drive_mount_result>(std::to_string(ret));
  event_system::instance().stop();
  c.reset();
  return static_cast<int>(ret);
}

auto winfsp_drive::Mounted(PVOID host) -> NTSTATUS {
  auto ret = STATUS_SUCCESS;
  utils::file::change_to_process_directory();

  auto *file_system_host = reinterpret_cast<FileSystemHost *>(host);
  polling::instance().start(&config_);
  fm_ = std::make_unique<file_manager>(config_, provider_);
  server_ = std::make_unique<full_server>(config_, provider_, *fm_);
  if (not provider_.is_direct_only()) {
    eviction_ = std::make_unique<eviction>(provider_, config_, *fm_);
  }

  try {
    server_->start();
    if (not provider_.start(
            [this](bool directory, api_file &file) -> api_error {
              return provider_meta_handler(provider_, directory, file);
            },
            fm_.get())) {
      throw startup_exception("provider is offline");
    }

    fm_->start();
    if (eviction_) {
      eviction_->start();
    }

    const auto mount_location =
        parse_mount_location(file_system_host->MountPoint());
    if (config_.get_enable_remote_mount()) {
      remote_server_ = std::make_unique<remote_winfsp::remote_server>(
          config_, *this, mount_location);
    }

    if (not lock_.set_mount_state(true, mount_location,
                                  ::GetCurrentProcessId())) {
      utils::error::raise_error(__FUNCTION__, "failed to set mount state");
    }

    event_system::instance().raise<drive_mounted>(mount_location);
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
    if (remote_server_) {
      remote_server_.reset();
    }
    server_->stop();
    polling::instance().stop();
    if (eviction_) {
      eviction_->stop();
    }
    fm_->stop();
    provider_.stop();
    ret = STATUS_INTERNAL_ERROR;
  }

  return ret;
}

auto winfsp_drive::Open(PWSTR file_name, UINT32 create_options,
                        UINT32 granted_access, PVOID * /*file_node*/,
                        PVOID *file_desc, OpenFileInfo *ofi) -> NTSTATUS {
  const auto api_path =
      utils::path::create_api_path(utils::string::to_utf8(file_name));

  bool directory{};
  auto error = provider_.is_directory(api_path, directory);
  if (error == api_error::success) {
    error = api_error::directory_not_found;
    if ((create_options & FILE_DIRECTORY_FILE) && not directory) {
    } else {
      error = api_error::success;
      if (not directory &&
          (((granted_access & FILE_WRITE_DATA) == FILE_WRITE_DATA) ||
           ((granted_access & GENERIC_WRITE) == GENERIC_WRITE))) {
        error = provider_.is_file_writeable(api_path)
                    ? api_error::success
                    : api_error::permission_denied;
      }

      if (error == api_error::success) {
        open_file_data ofd{};
        std::uint64_t handle{};
        std::shared_ptr<i_open_file> f;
        error = fm_->open(api_path, directory, ofd, handle, f);
        if (error == api_error::success) {
          api_meta_map meta;
          if ((error = provider_.get_item_meta(api_path, meta)) ==
              api_error::success) {
            populate_file_info(f->get_api_path(), f->get_file_size(), meta,
                               *ofi);
            *file_desc = reinterpret_cast<PVOID>(handle);
          }
        }
      }
    }
  }

  auto ret = utils::from_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

auto winfsp_drive::Overwrite(PVOID /*file_node*/, PVOID file_desc,
                             UINT32 attributes, BOOLEAN replace_attributes,
                             UINT64 /*allocation_size*/, FileInfo *file_info)
    -> NTSTATUS {
  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle =
      static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    std::shared_ptr<i_open_file> f;
    if (fm_->get_open_file(handle, true, f)) {
      api_path = f->get_api_path();
      api_meta_map meta{};
      if ((error = provider_.get_item_meta(api_path, meta)) ==
          api_error::success) {
        error = api_error::file_in_use;
        if (not fm_->is_processing(api_path)) {
          if ((error = f->resize(0)) == api_error::success) {
            filesystem_item existing_fsi{};
            if ((error = provider_.get_filesystem_item(
                     api_path, false, existing_fsi)) == api_error::success) {
              // Handle replace attributes
              if (replace_attributes) {
                if (not attributes) {
                  attributes = FILE_ATTRIBUTE_NORMAL;
                }
                meta[META_ATTRIBUTES] = std::to_string(attributes);
                error = provider_.set_item_meta(api_path, meta);
              } else if (attributes) { // Handle merge attributes
                const auto current_attributes =
                    utils::get_attributes_from_meta(meta);
                const auto merged_attributes = attributes | current_attributes;
                if (merged_attributes != current_attributes) {
                  meta[META_ATTRIBUTES] = std::to_string(merged_attributes);
                  error = provider_.set_item_meta(api_path, meta);
                }
              }
            }
          }
        }

        // Populate file information
        populate_file_info(f->get_file_size(), meta, *file_info);
      }
    }
  }

  auto ret = utils::from_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

auto winfsp_drive::parse_mount_location(const std::wstring &mount_location)
    -> std::string {
  return utils::string::to_utf8((mount_location.substr(0, 4) == L"\\\\.\\")
                                    ? mount_location.substr(4)
                                    : mount_location);
}

void winfsp_drive::populate_file_info(const std::string &api_path,
                                      std::uint64_t file_size,
                                      const api_meta_map &meta,
                                      FSP_FSCTL_OPEN_FILE_INFO &ofi) {
  const auto file_path = utils::string::from_utf8(
      utils::string::replace_copy(api_path, '/', '\\'));

  wcscpy_s(ofi.NormalizedName, ofi.NormalizedNameSize / sizeof(WCHAR),
           &file_path[0U]);
  ofi.NormalizedNameSize = (UINT16)(wcslen(&file_path[0U]) * sizeof(WCHAR));
  populate_file_info(file_size, meta, ofi.FileInfo);
}

auto winfsp_drive::populate_file_info(const std::string &api_path,
                                      remote::file_info &file_info)
    -> api_error {
  api_meta_map meta{};
  auto ret = provider_.get_item_meta(api_path, meta);
  if (ret == api_error::success) {
    FSP_FSCTL_FILE_INFO fi{};
    populate_file_info(utils::string::to_uint64(meta[META_SIZE]), meta, fi);
    set_file_info(file_info, fi);
  }

  return ret;
}

void winfsp_drive::populate_file_info(std::uint64_t file_size,
                                      api_meta_map meta,
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

auto winfsp_drive::Read(PVOID /*file_node*/, PVOID file_desc, PVOID buffer,
                        UINT64 offset, ULONG length, PULONG bytes_transferred)
    -> NTSTATUS {
  *bytes_transferred = 0U;

  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle =
      static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    if (length > 0U) {
      std::shared_ptr<i_open_file> f;
      if (fm_->get_open_file(handle, false, f)) {
        api_path = f->get_api_path();
        data_buffer data;
        if ((error = f->read(length, offset, data)) == api_error::success) {
          *bytes_transferred = static_cast<ULONG>(data.size());
          if ((length > 0) && (data.size() != length)) {
            ::SetLastError(ERROR_HANDLE_EOF);
          }

          if (not data.empty()) {
            ::CopyMemory(buffer, &data[0U], data.size());
            data.clear();
            auto res = provider_.set_item_meta(
                api_path, META_ACCESSED,
                std::to_string(utils::get_file_time_now()));
            if (res != api_error::success) {
              utils::error::raise_api_path_error(
                  __FUNCTION__, api_path, res,
                  "failed to set meta accessed time");
            }
          }
        }
      }
    } else {
      error = api_error::success;
    }
  }

  auto ret = utils::from_api_error(error);
  if (ret != STATUS_SUCCESS) {
    RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  }
  return ret;
}

auto winfsp_drive::ReadDirectory(PVOID /*file_node*/, PVOID file_desc,
                                 PWSTR /*pattern*/, PWSTR marker, PVOID buffer,
                                 ULONG buffer_length, PULONG bytes_transferred)
    -> NTSTATUS {
  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle =
      static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    std::shared_ptr<i_open_file> f;
    if (fm_->get_open_file(handle, false, f)) {
      api_path = f->get_api_path();
      bool exists{};
      error = provider_.is_directory(api_path, exists);
      if (error == api_error::success) {
        if (exists) {
          directory_item_list list{};
          if ((error = provider_.get_directory_items(api_path, list)) ==
              api_error::success) {
            directory_iterator iterator(std::move(list));
            auto status_result = STATUS_SUCCESS;
            auto *directory_buffer = f->get_open_data(handle).directory_buffer;
            if (FspFileSystemAcquireDirectoryBuffer(
                    &directory_buffer, static_cast<BOOLEAN>(nullptr == marker),
                    &status_result)) {
              directory_item di{};
              auto offset =
                  marker
                      ? iterator.get_next_directory_offset(
                            utils::path::create_api_path(utils::path::combine(
                                api_path, {utils::string::to_utf8(marker)})))
                      : 0U;
              while ((error = iterator.get_directory_item(offset++, di)) ==
                     api_error::success) {
                if (utils::path::is_ads_file_path(di.api_path) ||
                    di.api_path == "." || di.api_path == "..") {
                  continue;
                }

                if (di.meta.empty()) {
                  utils::error::raise_api_path_error(__FUNCTION__, di.api_path,
                                                     api_error::error,
                                                     "item meta is empty");
                  continue;
                }

                const auto display_name = utils::string::from_utf8(
                    utils::path::strip_to_file_name(di.api_path));
                union {
                  UINT8 B[FIELD_OFFSET(FSP_FSCTL_DIR_INFO, FileNameBuf) +
                          ((MAX_PATH + 1) * sizeof(WCHAR))];
                  FSP_FSCTL_DIR_INFO D;
                } directory_info_buffer;

                auto *directory_info = &directory_info_buffer.D;
                ::ZeroMemory(directory_info, sizeof(*directory_info));
                directory_info->Size = static_cast<UINT16>(
                    FIELD_OFFSET(FSP_FSCTL_DIR_INFO, FileNameBuf) +
                    (std::min((size_t)MAX_PATH, display_name.size()) *
                     sizeof(WCHAR)));

                populate_file_info(di.size, di.meta, directory_info->FileInfo);
                ::wcscpy_s(&directory_info->FileNameBuf[0U], MAX_PATH,
                           &display_name[0U]);

                FspFileSystemFillDirectoryBuffer(
                    &directory_buffer, directory_info, &status_result);
              }

              FspFileSystemReleaseDirectoryBuffer(&directory_buffer);
            }

            if (status_result == STATUS_SUCCESS) {
              FspFileSystemReadDirectoryBuffer(&directory_buffer, marker,
                                               buffer, buffer_length,
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
  }

  auto ret = utils::from_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

auto winfsp_drive::Rename(PVOID /*file_node*/, PVOID /*file_desc*/,
                          PWSTR file_name, PWSTR new_file_name,
                          BOOLEAN replace_if_exists) -> NTSTATUS {
  const auto from_api_path =
      utils::path::create_api_path(utils::string::to_utf8(file_name));
  const auto to_api_path =
      utils::path::create_api_path(utils::string::to_utf8(new_file_name));

  auto error = api_error::item_not_found;
  bool exists{};
  error = provider_.is_file(from_api_path, exists);
  if (error == api_error::success) {
    if (exists) {
      error = fm_->rename_file(from_api_path, to_api_path, replace_if_exists);
    } else {
      error = provider_.is_directory(from_api_path, exists);
      if (error == api_error::success && exists)
        error = fm_->rename_directory(from_api_path, to_api_path);
    }
  }

  auto ret = utils::from_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, from_api_path + "|" + to_api_path, ret);
  return ret;
}

auto winfsp_drive::SetBasicInfo(PVOID /*file_node*/, PVOID file_desc,
                                UINT32 attributes, UINT64 creation_time,
                                UINT64 last_access_time, UINT64 last_write_time,
                                UINT64 change_time, FileInfo *fileInfo)
    -> NTSTATUS {
  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle =
      static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    std::shared_ptr<i_open_file> f;
    if (fm_->get_open_file(handle, false, f)) {
      api_path = f->get_api_path();
      if (attributes == INVALID_FILE_ATTRIBUTES) {
        attributes = 0;
      } else if (attributes == 0) {
        attributes = f->is_directory() ? FILE_ATTRIBUTE_DIRECTORY
                                       : FILE_ATTRIBUTE_NORMAL;
      }

      api_meta_map meta;
      if (attributes) {
        if ((attributes & FILE_ATTRIBUTE_NORMAL) &&
            (attributes != FILE_ATTRIBUTE_NORMAL)) {
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
        meta[META_CHANGED] = std::to_string(change_time);
        meta[META_MODIFIED] = std::to_string(change_time);
      }

      error = provider_.set_item_meta(api_path, meta);

      // Populate file information
      if (provider_.get_item_meta(api_path, meta) == api_error::success) {
        populate_file_info(utils::string::to_uint64(meta[META_SIZE]), meta,
                           *fileInfo);
      }
    }
  }

  auto ret = utils::from_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

void winfsp_drive::set_file_info(remote::file_info &dest,
                                 const FSP_FSCTL_FILE_INFO &src) {
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

auto winfsp_drive::SetFileSize(PVOID /*file_node*/, PVOID file_desc,
                               UINT64 new_size, BOOLEAN set_allocation_size,
                               FileInfo *file_info) -> NTSTATUS {
  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle =
      static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    std::shared_ptr<i_open_file> f;
    if (fm_->get_open_file(handle, true, f)) {
      api_path = f->get_api_path();
      i_open_file::native_operation_callback allocator;
      if (set_allocation_size) {
        allocator = [&](native_handle handle) -> api_error {
          std::string meta_allocation_size;
          utils::calculate_allocation_size(false, 0, new_size,
                                           meta_allocation_size);

          FILE_ALLOCATION_INFO allocation_info{};
          allocation_info.AllocationSize.QuadPart = new_size;
          return ::SetFileInformationByHandle(handle, FileAllocationInfo,
                                              &allocation_info,
                                              sizeof(allocation_info))
                     ? api_error::success
                     : api_error::os_error;
        };
      } else {
        allocator = [&](native_handle handle) -> api_error {
          FILE_END_OF_FILE_INFO end_of_file_info{};
          end_of_file_info.EndOfFile.QuadPart = new_size;
          return ::SetFileInformationByHandle(handle, FileEndOfFileInfo,
                                              &end_of_file_info,
                                              sizeof(end_of_file_info))
                     ? api_error::success
                     : api_error::os_error;
        };
      }

      error = f->native_operation(new_size, allocator);
      if (file_info) {
        // Populate file information
        api_meta_map meta;
        if (provider_.get_item_meta(api_path, meta) == api_error::success) {
          populate_file_info(f->get_file_size(), meta, *file_info);
        }
      }
    }
  }

  auto ret = utils::from_api_error(error);
  RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  return ret;
}

VOID winfsp_drive::Unmounted(PVOID host) {
  auto *file_system_host = reinterpret_cast<FileSystemHost *>(host);
  const auto mount_location =
      parse_mount_location(file_system_host->MountPoint());
  event_system::instance().raise<drive_unmount_pending>(mount_location);
  if (remote_server_) {
    remote_server_.reset();
  }
  server_->stop();
  polling::instance().stop();
  if (eviction_) {
    eviction_->stop();
  }
  fm_->stop();
  provider_.stop();
  server_.reset();
  fm_.reset();
  eviction_.reset();

  if (not lock_.set_mount_state(false, "", -1)) {
    utils::error::raise_error(__FUNCTION__, "failed to set mount state");
  }

  event_system::instance().raise<drive_unmounted>(mount_location);
  config_.save();
}

auto winfsp_drive::Write(PVOID /*file_node*/, PVOID file_desc, PVOID buffer,
                         UINT64 offset, ULONG length, BOOLEAN write_to_end,
                         BOOLEAN constrained_io, PULONG bytes_transferred,
                         FileInfo *file_info) -> NTSTATUS {
  *bytes_transferred = 0;

  std::string api_path;
  auto error = api_error::invalid_handle;
  auto handle =
      static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(file_desc));
  if (handle) {
    std::shared_ptr<i_open_file> f;
    if (fm_->get_open_file(handle, true, f)) {
      api_path = f->get_api_path();
      if (write_to_end) {
        offset = f->get_file_size();
      }

      auto should_write = true;
      if (constrained_io) {
        if (offset >= f->get_file_size()) {
          error = api_error::success;
          should_write = false;
        } else if (offset + length > f->get_file_size()) {
          length = static_cast<ULONG>(f->get_file_size() - offset);
        }
      }

      if (should_write) {
        if (length > 0U) {
          std::size_t bytes_written{};
          data_buffer data(length);
          std::memcpy(&data[0U], buffer, length);
          if ((error = f->write(offset, data, bytes_written)) ==
              api_error::success) {
            *bytes_transferred = static_cast<ULONG>(bytes_written);

            // Populate file information
            api_meta_map meta;
            if (provider_.get_item_meta(api_path, meta) == api_error::success) {
              populate_file_info(f->get_file_size(), meta, *file_info);
            }
          }
        }
      } else {
        error = api_error::success;
      }
    }
  }

  auto ret = utils::from_api_error(error);
  if (ret != STATUS_SUCCESS) {
    RAISE_WINFSP_EVENT(__FUNCTION__, api_path, ret);
  }
  return ret;
}
} // namespace repertory

#endif // _WIN32
