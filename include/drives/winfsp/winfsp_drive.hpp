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
#ifndef INCLUDE_DRIVES_WINFSP_WINFSP_DRIVE_HPP_
#define INCLUDE_DRIVES_WINFSP_WINFSP_DRIVE_HPP_
#ifdef _WIN32

#include "common.hpp"
#include "download/download_manager.hpp"
#include "drives/eviction.hpp"
#include "drives/i_open_file_table.hpp"
#include "drives/open_file_table.hpp"
#include "drives/winfsp/i_winfsp_drive.hpp"
#include "drives/winfsp/remotewinfsp/remote_server.hpp"
#include "events/event_system.hpp"
#include "rpc/server/full_server.hpp"

namespace repertory {
class app_config;
class lock_data;
class i_provider;
class winfsp_drive final : public virtual i_winfsp_drive, public virtual FileSystemBase {
  E_CONSUMER();

public:
  winfsp_drive(app_config &config, lock_data &lockData, i_provider &provider);

  ~winfsp_drive() override { E_CONSUMER_RELEASE(); }

private:
  class winfsp_service final : virtual public Service {
  public:
    winfsp_service(lock_data &lock, winfsp_drive &drive, std::vector<std::string> drive_args,
                   app_config &config);

    ~winfsp_service() override = default;

  private:
    lock_data &lock_;
    winfsp_drive &drive_;
    const std::vector<std::string> drive_args_;
    FileSystemHost host_;
    app_config &config_;

  protected:
    NTSTATUS OnStart(ULONG, PWSTR *) override;

    NTSTATUS OnStop() override;
  };

private:
  i_provider &provider_;
  app_config &config_;
  lock_data &lock_;
  std::unique_ptr<full_server> server_;
  std::unique_ptr<open_file_table<open_file_data>> oft_;
  std::unique_ptr<download_manager> download_manager_;
  std::unique_ptr<eviction> eviction_;
  std::unique_ptr<remote_winfsp::remote_server> remote_server_;

private:
  static std::string parse_mount_location(const std::wstring &mount_location);

  void populate_file_info(const std::string &api_path, const std::uint64_t &file_size,
                          const api_meta_map &meta, FSP_FSCTL_OPEN_FILE_INFO &ofi);

  void populate_file_info(const std::uint64_t &file_size, api_meta_map meta,
                          FSP_FSCTL_FILE_INFO &fi);

  static void set_file_info(remote::file_info &dest, const FSP_FSCTL_FILE_INFO &src);

public:
  NTSTATUS CanDelete(PVOID file_node, PVOID file_desc, PWSTR file_name) override;

  VOID Cleanup(PVOID file_node, PVOID file_desc, PWSTR file_name, ULONG flags) override;

  VOID Close(PVOID file_node, PVOID file_desc) override;

  NTSTATUS Create(PWSTR file_name, UINT32 create_options, UINT32 granted_access, UINT32 attributes,
                  PSECURITY_DESCRIPTOR descriptor, UINT64 allocation_size, PVOID *file_node,
                  PVOID *file_desc, OpenFileInfo *ofi) override;

  NTSTATUS Flush(PVOID file_node, PVOID file_desc, FileInfo *file_info) override;

  std::uint64_t get_directory_item_count(const std::string &api_path) const override;

  directory_item_list get_directory_items(const std::string &api_path) const override;

  NTSTATUS GetFileInfo(PVOID file_node, PVOID file_desc, FileInfo *file_info) override;

  std::uint64_t get_file_size(const std::string &api_path) const override;

  api_error get_item_meta(const std::string &api_path, api_meta_map &meta) const override;

  api_error get_item_meta(const std::string &api_path, const std::string &name,
                          std::string &value) const override;

  NTSTATUS get_security_by_name(PWSTR file_name, PUINT32 attributes,
                                PSECURITY_DESCRIPTOR descriptor,
                                std::uint64_t *descriptor_size) override;

  NTSTATUS GetSecurityByName(PWSTR file_name, PUINT32 attributes, PSECURITY_DESCRIPTOR descriptor,
                             SIZE_T *descriptor_size) override;

  std::uint64_t get_total_drive_space() const override;

  std::uint64_t get_total_item_count() const override;

  std::uint64_t get_used_drive_space() const override;

  void get_volume_info(UINT64 &total_size, UINT64 &free_size,
                       std::string &volume_label) const override;

  NTSTATUS GetVolumeInfo(VolumeInfo *volume_info) override;

  NTSTATUS Init(PVOID host) override;

  int mount(const std::vector<std::string> &drive_args);

  NTSTATUS Mounted(PVOID host) override;

  NTSTATUS Open(PWSTR file_name, UINT32 create_options, UINT32 granted_access, PVOID *file_node,
                PVOID *file_desc, OpenFileInfo *ofi) override;

  NTSTATUS Overwrite(PVOID file_node, PVOID file_desc, UINT32 attributes,
                     BOOLEAN replace_attributes, UINT64 allocation_size,
                     FileInfo *file_info) override;

  api_error populate_file_info(const std::string &api_path, remote::file_info &file_info) override;

  NTSTATUS Read(PVOID file_node, PVOID file_desc, PVOID buffer, UINT64 offset, ULONG length,
                PULONG bytes_transferred) override;

  NTSTATUS ReadDirectory(PVOID file_node, PVOID file_desc, PWSTR pattern, PWSTR marker,
                         PVOID buffer, ULONG bufferLength, PULONG bytes_transferred) override;

  NTSTATUS Rename(PVOID file_node, PVOID file_desc, PWSTR file_name, PWSTR newFileName,
                  BOOLEAN replace_if_exists) override;

  NTSTATUS SetBasicInfo(PVOID file_node, PVOID file_desc, UINT32 attributes, UINT64 creation_time,
                        UINT64 last_access_time, UINT64 last_write_time, UINT64 change_time,
                        FileInfo *file_info) override;

  NTSTATUS SetFileSize(PVOID file_node, PVOID file_desc, UINT64 new_size,
                       BOOLEAN set_allocation_size, FileInfo *file_info) override;

  VOID Unmounted(PVOID host) override;

  NTSTATUS Write(PVOID file_node, PVOID file_desc, PVOID buffer, UINT64 offset, ULONG length,
                 BOOLEAN write_to_end, BOOLEAN constrained_io, PULONG bytes_transferred,
                 FileInfo *file_info) override;

  void shutdown() { ::GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0); }

  static void display_options(int argc, char *argv[]) {}

  static void display_version_information(int argc, char *argv[]) {}
};
} // namespace repertory

#endif // _WIN32
#endif // INCLUDE_DRIVES_WINFSP_WINFSP_DRIVE_HPP_
