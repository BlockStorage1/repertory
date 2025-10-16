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
#ifndef REPERTORY_INCLUDE_DRIVES_WINFSP_WINFSP_DRIVE_HPP_
#define REPERTORY_INCLUDE_DRIVES_WINFSP_WINFSP_DRIVE_HPP_
#if defined(_WIN32)

#include "drives/eviction.hpp"
#include "drives/winfsp/i_winfsp_drive.hpp"
#include "drives/winfsp/remotewinfsp/remote_server.hpp"
#include "events/event_system.hpp"
#include "file_manager/file_manager.hpp"
#include "rpc/server/full_server.hpp"

namespace repertory {
class app_config;
class lock_data;
class i_provider;

class winfsp_drive final : public virtual i_winfsp_drive,
                           public virtual FileSystemBase {
  E_CONSUMER();

public:
  winfsp_drive(app_config &config, lock_data &lockData, i_provider &provider);

  ~winfsp_drive() override { E_CONSUMER_RELEASE(); }

private:
  class winfsp_service final : virtual public Service {
  public:
    winfsp_service(lock_data &lock, winfsp_drive &drive,
                   std::vector<std::string> drive_args, app_config &config);

    ~winfsp_service() override = default;

  private:
    app_config &config_;
    winfsp_drive &drive_;
    std::vector<std::string> drive_args_;
    FileSystemHost host_;
    lock_data &lock_;

  protected:
    auto OnStart(ULONG, PWSTR *) -> NTSTATUS override;

    auto OnStop() -> NTSTATUS override;
  };

private:
  i_provider &provider_;
  app_config &config_;
  lock_data &lock_;
  std::unique_ptr<full_server> server_;
  std::unique_ptr<file_manager> fm_;
  std::unique_ptr<eviction> eviction_;
  std::unique_ptr<remote_winfsp::remote_server> remote_server_;
  std::mutex stop_all_mtx_;

private:
  [[nodiscard]] auto handle_error(std::string_view function_name,
                                  std::string_view api_path, api_error error,
                                  FileInfo *f_info, std::uint64_t file_size,
                                  bool raise_on_failure_only = false) const
      -> NTSTATUS;

  static auto parse_mount_location(std::wstring_view mount_location)
      -> std::string;

  void populate_file_info(std::string_view api_path, std::uint64_t file_size,
                          const api_meta_map &meta,
                          FSP_FSCTL_OPEN_FILE_INFO &ofi) const;

  void populate_file_info(std::uint64_t file_size, api_meta_map meta,
                          FSP_FSCTL_FILE_INFO &fi) const;

  static void set_file_info(remote::file_info &r_info,
                            const FSP_FSCTL_FILE_INFO &f_info);

  void stop_all();

public:
  auto CanDelete(PVOID file_node, PVOID file_desc, PWSTR file_name)
      -> NTSTATUS override;

  VOID Cleanup(PVOID file_node, PVOID file_desc, PWSTR file_name,
               ULONG flags) override;

  VOID Close(PVOID file_node, PVOID file_desc) override;

  auto Create(PWSTR file_name, UINT32 create_options, UINT32 granted_access,
              UINT32 attributes, PSECURITY_DESCRIPTOR descriptor,
              UINT64 allocation_size, PVOID *file_node, PVOID *file_desc,
              OpenFileInfo *ofi) -> NTSTATUS override;

  auto Flush(PVOID file_node, PVOID file_desc, FileInfo *f_info)
      -> NTSTATUS override;

  [[nodiscard]] auto get_directory_item_count(std::string_view api_path) const
      -> std::uint64_t override;

  [[nodiscard]] auto get_directory_items(std::string_view api_path) const
      -> directory_item_list override;

  auto GetFileInfo(PVOID file_node, PVOID file_desc, FileInfo *f_info)
      -> NTSTATUS override;

  [[nodiscard]] auto get_file_size(std::string_view api_path) const
      -> std::uint64_t override;

  [[nodiscard]] auto get_item_meta(std::string_view api_path,
                                   api_meta_map &meta) const
      -> api_error override;

  [[nodiscard]] auto get_item_meta(std::string_view api_path,
                                   std::string_view name,
                                   std::string &value) const
      -> api_error override;

  [[nodiscard]] auto get_security_by_name(PWSTR file_name, PUINT32 attributes,
                                          PSECURITY_DESCRIPTOR descriptor,
                                          std::uint64_t *descriptor_size)
      -> NTSTATUS override;

  auto GetSecurityByName(PWSTR file_name, PUINT32 attributes,
                         PSECURITY_DESCRIPTOR descriptor,
                         SIZE_T *descriptor_size) -> NTSTATUS override;

  [[nodiscard]] auto get_total_drive_space() const -> std::uint64_t override;

  [[nodiscard]] auto get_total_item_count() const -> std::uint64_t override;

  [[nodiscard]] auto get_used_drive_space() const -> std::uint64_t override;

  void get_volume_info(UINT64 &total_size, UINT64 &free_size,
                       std::string &volume_label) const override;

  auto GetVolumeInfo(VolumeInfo *volume_info) -> NTSTATUS override;

  auto Init(PVOID host) -> NTSTATUS override;

  [[nodiscard]] auto mount(std::vector<std::string> orig_args,
                           std::vector<std::string> drive_args,
                           provider_type prov, std::string_view unique_id)
      -> int;

  auto Mounted(PVOID host) -> NTSTATUS override;

  auto Open(PWSTR file_name, UINT32 create_options, UINT32 granted_access,
            PVOID *file_node, PVOID *file_desc, OpenFileInfo *ofi)
      -> NTSTATUS override;

  auto Overwrite(PVOID file_node, PVOID file_desc, UINT32 attributes,
                 BOOLEAN replace_attributes, UINT64 allocation_size,
                 FileInfo *f_info) -> NTSTATUS override;

  [[nodiscard]] auto populate_file_info(std::string_view api_path,
                                        remote::file_info &r_info) const
      -> api_error override;

  auto Read(PVOID file_node, PVOID file_desc, PVOID buffer, UINT64 offset,
            ULONG length, PULONG bytes_transferred) -> NTSTATUS override;

  auto ReadDirectory(PVOID file_node, PVOID file_desc, PWSTR pattern,
                     PWSTR marker, PVOID buffer, ULONG bufferLength,
                     PULONG bytes_transferred) -> NTSTATUS override;

  auto Rename(PVOID file_node, PVOID file_desc, PWSTR file_name,
              PWSTR new_file_name, BOOLEAN replace_if_exists)
      -> NTSTATUS override;

  auto SetBasicInfo(PVOID file_node, PVOID file_desc, UINT32 attributes,
                    UINT64 creation_time, UINT64 last_access_time,
                    UINT64 last_write_time, UINT64 change_time,
                    FileInfo *f_info) -> NTSTATUS override;

  auto SetFileSize(PVOID file_node, PVOID file_desc, UINT64 new_size,
                   BOOLEAN set_allocation_size, FileInfo *f_info)
      -> NTSTATUS override;

  VOID Unmounted(PVOID host) override;

  auto Write(PVOID file_node, PVOID file_desc, PVOID buffer, UINT64 offset,
             ULONG length, BOOLEAN write_to_end, BOOLEAN constrained_io,
             PULONG bytes_transferred, FileInfo *f_info) -> NTSTATUS override;

  void shutdown();

  static void display_options(std::vector<const char *> /* args */) {}

  static void
  display_version_information(std::vector<const char *> /* args */) {}
};
} // namespace repertory

#endif // _WIN32
#endif // REPERTORY_INCLUDE_DRIVES_WINFSP_WINFSP_DRIVE_HPP_
