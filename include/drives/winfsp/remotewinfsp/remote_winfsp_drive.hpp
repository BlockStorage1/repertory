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
#ifndef INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_REMOTE_WINFSP_DRIVE_HPP_
#define INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_REMOTE_WINFSP_DRIVE_HPP_
#ifdef _WIN32

#include "drives/winfsp/remotewinfsp/i_remote_instance.hpp"
#include "events/event_system.hpp"

namespace repertory {
class app_config;
class lock_data;
class server;
namespace remote_winfsp {
class remote_winfsp_drive final : public virtual FileSystemBase {
  E_CONSUMER();

public:
  remote_winfsp_drive(app_config &config, remote_instance_factory factory,
                      lock_data &lock);

  ~remote_winfsp_drive() override { E_CONSUMER_RELEASE(); }

public:
  class winfsp_service : virtual public Service {
  public:
    winfsp_service(lock_data &lock, remote_winfsp_drive &drive,
                   std::vector<std::string> drive_args, app_config &config);

    ~winfsp_service() override = default;

  private:
    app_config &config_;
    lock_data &lock_;
    remote_winfsp_drive &drive_;
    const std::vector<std::string> drive_args_;
    FileSystemHost host_;

  protected:
    auto OnStart(ULONG, PWSTR *) -> NTSTATUS override;

    auto OnStop() -> NTSTATUS override;
  };

private:
  app_config &config_;
  lock_data &lock_;
  remote_instance_factory factory_;
  std::unique_ptr<i_remote_instance> remote_instance_;
  std::unique_ptr<server> server_;
  std::string mount_location_;

private:
  void populate_file_info(const json &item, FSP_FSCTL_FILE_INFO &file_info);

  static void set_file_info(FileInfo &dest, const remote::file_info &src);

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

  auto Flush(PVOID file_node, PVOID file_desc, FileInfo *file_info)
      -> NTSTATUS override;

  auto GetFileInfo(PVOID file_node, PVOID file_desc, FileInfo *file_info)
      -> NTSTATUS override;

  auto GetSecurityByName(PWSTR file_name, PUINT32 attributes,
                         PSECURITY_DESCRIPTOR descriptor,
                         SIZE_T *descriptor_size) -> NTSTATUS override;

  auto GetVolumeInfo(VolumeInfo *volume_info) -> NTSTATUS override;

  auto Init(PVOID host) -> NTSTATUS override;

  [[nodiscard]] auto mount(const std::vector<std::string> &drive_args) -> int;

  auto Mounted(PVOID host) -> NTSTATUS override;

  auto Open(PWSTR file_name, UINT32 create_options, UINT32 granted_access,
            PVOID *file_node, PVOID *file_desc, OpenFileInfo *ofi)
      -> NTSTATUS override;

  auto Overwrite(PVOID file_node, PVOID file_desc, UINT32 attributes,
                 BOOLEAN replace_attributes, UINT64 allocation_size,
                 FileInfo *file_info) -> NTSTATUS override;

  auto Read(PVOID file_node, PVOID file_desc, PVOID buffer, UINT64 offset,
            ULONG length, PULONG bytes_transferred) -> NTSTATUS override;

  auto ReadDirectory(PVOID file_node, PVOID file_desc, PWSTR pattern,
                     PWSTR marker, PVOID buffer, ULONG buffer_length,
                     PULONG bytes_transferred) -> NTSTATUS override;

  auto Rename(PVOID file_node, PVOID file_desc, PWSTR file_name,
              PWSTR new_file_name, BOOLEAN replace_if_exists)
      -> NTSTATUS override;

  auto SetBasicInfo(PVOID file_node, PVOID file_desc, UINT32 attributes,
                    UINT64 creation_time, UINT64 last_access_time,
                    UINT64 last_write_time, UINT64 change_time,
                    FileInfo *file_info) -> NTSTATUS override;

  auto SetFileSize(PVOID file_node, PVOID file_desc, UINT64 new_size,
                   BOOLEAN set_allocation_size, FileInfo *file_info)
      -> NTSTATUS override;

  VOID Unmounted(PVOID host) override;

  auto Write(PVOID file_node, PVOID file_desc, PVOID buffer, UINT64 offset,
             ULONG length, BOOLEAN write_to_end, BOOLEAN constrained_io,
             PULONG bytes_transferred, FileInfo *file_info)
      -> NTSTATUS override;

  void shutdown() { ::GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0); }

  static void display_options(int argc, char *argv[]) {}

  static void display_version_information(int argc, char *argv[]) {}
};
} // namespace remote_winfsp
} // namespace repertory

#endif // _WIN32
#endif // INCLUDE_DRIVES_WINFSP_REMOTEWINFSP_REMOTE_WINFSP_DRIVE_HPP_
