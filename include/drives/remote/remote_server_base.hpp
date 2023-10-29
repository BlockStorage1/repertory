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
#ifndef INCLUDE_DRIVES_REMOTE_REMOTE_SERVER_BASE_HPP_
#define INCLUDE_DRIVES_REMOTE_REMOTE_SERVER_BASE_HPP_

#include "app_config.hpp"
#include "comm/packet/client_pool.hpp"
#include "comm/packet/packet.hpp"
#include "comm/packet/packet_server.hpp"
#include "drives/directory_iterator.hpp"
#include "drives/fuse/remotefuse/i_remote_instance.hpp"
#include "drives/remote/i_remote_json.hpp"
#include "drives/remote/remote_open_file_table.hpp"
#include "drives/winfsp/remotewinfsp/i_remote_instance.hpp"
#include "types/remote.hpp"
#include "types/repertory.hpp"
#include "utils/Base64.hpp"
#include "utils/path_utils.hpp"

#define REPERTORY_DIRECTORY_PAGE_SIZE std::size_t(100u)

namespace repertory {
template <typename drive>
class remote_server_base : public remote_open_file_table,
                           public virtual remote_winfsp::i_remote_instance,
                           public virtual remote_fuse::i_remote_instance {
public:
  remote_server_base(app_config &config, drive &d, std::string mount_location)
      : config_(config),
        drive_(d),
        mount_location_(std::move(mount_location)),
        client_pool_(config.get_remote_client_pool_size()) {
    event_system::instance().raise<service_started>("remote_server_base");
    handler_lookup_.insert(
        {"::winfsp_can_delete",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           HANDLE file_desc;
           DECODE_OR_RETURN(request, file_desc);

           std::wstring file_name;
           DECODE_OR_RETURN(request, file_name);

           ret = this->winfsp_can_delete(file_desc, &file_name[0]);
           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_cleanup",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           HANDLE file_desc;
           DECODE_OR_RETURN(request, file_desc);

           std::wstring file_name;
           DECODE_OR_RETURN(request, file_name);

           UINT32 flags;
           DECODE_OR_RETURN(request, flags);

           BOOLEAN was_closed;
           ret = this->winfsp_cleanup(file_desc, &file_name[0], flags,
                                      was_closed);
           response.encode(was_closed);

           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_close",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           HANDLE file_desc;
           DECODE_OR_RETURN(request, file_desc);

           ret = this->winfsp_close(file_desc);
           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_create",
         [this](std::uint32_t, const std::string &client_id, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           std::wstring file_name;
           DECODE_OR_RETURN(request, file_name);

           UINT32 create_options;
           DECODE_OR_RETURN(request, create_options);

           UINT32 granted_access;
           DECODE_OR_RETURN(request, granted_access);

           UINT32 attributes;
           DECODE_OR_RETURN(request, attributes);

           UINT64 allocation_size;
           DECODE_OR_RETURN(request, allocation_size);

           BOOLEAN exists = 0;
           remote::file_info file_info{};
           std::string normalized_name;
           PVOID file_desc;
           ret = this->winfsp_create(&file_name[0], create_options,
                                     granted_access, attributes,
                                     allocation_size, &file_desc, &file_info,
                                     normalized_name, exists);
           if (ret == STATUS_SUCCESS) {
#ifdef _WIN32
             this->set_client_id(file_desc, client_id);
#else
             this->set_client_id(reinterpret_cast<std::uintptr_t>(file_desc),
                                 client_id);
#endif
             response.encode(file_desc);
             response.encode(file_info);
             response.encode(normalized_name);
             response.encode(exists);
           }

           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_flush",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           HANDLE file_desc;
           DECODE_OR_RETURN(request, file_desc);

           remote::file_info file_info{};
           ret = this->winfsp_flush(file_desc, &file_info);
           if (ret == STATUS_SUCCESS) {
             response.encode(file_info);
           }

           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_get_file_info",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           HANDLE file_desc;
           DECODE_OR_RETURN(request, file_desc);

           remote::file_info file_info{};
           ret = this->winfsp_get_file_info(file_desc, &file_info);
           if (ret == STATUS_SUCCESS) {
             response.encode(file_info);
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_get_security_by_name",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           std::wstring file_name;
           DECODE_OR_RETURN(request, file_name);

           std::uint64_t descriptor_size;
           DECODE_OR_RETURN(request, descriptor_size);

           std::uint8_t get_attributes;
           DECODE_OR_RETURN(request, get_attributes);

           UINT32 attributes;
           auto *attrPtr = get_attributes ? &attributes : nullptr;
           std::wstring string_descriptor;
           ret = this->winfsp_get_security_by_name(
               &file_name[0], attrPtr,
               descriptor_size ? &descriptor_size : nullptr, string_descriptor);
           if (ret == STATUS_SUCCESS) {
             response.encode(string_descriptor);
             if (get_attributes) {
               response.encode(attributes);
             }
           }

           return ret;
         }});
    handler_lookup_.insert({"::winfsp_get_volume_info",
                            [this](std::uint32_t, const std::string &,
                                   std::uint64_t, const std::string &, packet *,
                                   packet &response) -> packet::error_type {
                              auto ret = STATUS_SUCCESS;

                              UINT64 total_size = 0u;
                              UINT64 free_size = 0u;
                              std::string volume_label;
                              if ((ret = this->winfsp_get_volume_info(
                                       total_size, free_size, volume_label)) ==
                                  STATUS_SUCCESS) {
                                response.encode(total_size);
                                response.encode(free_size);
                                response.encode(volume_label);
                              }
                              return ret;
                            }});
    handler_lookup_.insert(
        {"::winfsp_mounted",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           std::string version;
           DECODE_OR_RETURN(request, version);

           std::wstring location;
           DECODE_OR_RETURN(request, location);

           ret = this->winfsp_mounted(location);
           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_open",
         [this](std::uint32_t, const std::string &client_id, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           std::wstring file_name;
           DECODE_OR_RETURN(request, file_name);

           UINT32 create_options;
           DECODE_OR_RETURN(request, create_options);

           UINT32 granted_access;
           DECODE_OR_RETURN(request, granted_access);

           remote::file_info file_info{};
           std::string normalized_name;
           PVOID file_desc;
           ret =
               this->winfsp_open(&file_name[0], create_options, granted_access,
                                 &file_desc, &file_info, normalized_name);
           if (ret == STATUS_SUCCESS) {
#ifdef _WIN32
             this->set_client_id(file_desc, client_id);
#else
             this->set_client_id(reinterpret_cast<std::uintptr_t>(file_desc),
                                 client_id);
#endif
             response.encode(file_desc);
             response.encode(file_info);
             response.encode(normalized_name);
           }

           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_overwrite",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           HANDLE file_desc;
           DECODE_OR_RETURN(request, file_desc);

           UINT32 attributes;
           DECODE_OR_RETURN(request, attributes);

           BOOLEAN replace_attributes;
           DECODE_OR_RETURN(request, replace_attributes);

           UINT64 allocation_size;
           DECODE_OR_RETURN(request, allocation_size);

           remote::file_info file_info{};
           ret =
               this->winfsp_overwrite(file_desc, attributes, replace_attributes,
                                      allocation_size, &file_info);
           if (ret == STATUS_SUCCESS) {
             response.encode(file_info);
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_read",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           HANDLE file_desc;
           DECODE_OR_RETURN(request, file_desc);

           UINT64 offset;
           DECODE_OR_RETURN(request, offset);

           UINT32 length;
           DECODE_OR_RETURN(request, length);

           data_buffer buffer(length);
           UINT32 bytes_transferred = 0;
           ret = this->winfsp_read(file_desc, &buffer[0], offset, length,
                                   &bytes_transferred);
           if (ret == STATUS_SUCCESS) {
             response.encode(static_cast<UINT32>(bytes_transferred));
             if (bytes_transferred) {
               response.encode(&buffer[0], bytes_transferred);
             }
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_read_directory",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           HANDLE file_desc;
           DECODE_OR_RETURN(request, file_desc);

           std::wstring pattern;
           DECODE_OR_IGNORE(request, pattern);

           std::wstring marker;
           DECODE_OR_IGNORE(request, marker);

           json itemList;
           ret = this->winfsp_read_directory(
               file_desc, &pattern[0],
               wcsnlen(&marker[0], marker.size()) ? &marker[0] : nullptr,
               itemList);
           if (ret == STATUS_SUCCESS) {
             response.encode(itemList.dump(0));
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_rename",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           HANDLE file_desc;
           DECODE_OR_RETURN(request, file_desc);

           std::wstring file_name;
           DECODE_OR_RETURN(request, file_name);

           std::wstring new_file_name;
           DECODE_OR_RETURN(request, new_file_name);

           BOOLEAN replace_if_exists;
           DECODE_OR_RETURN(request, replace_if_exists);

           ret = this->winfsp_rename(file_desc, &file_name[0],
                                     &new_file_name[0], replace_if_exists);
           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_set_basic_info",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           HANDLE file_desc;
           DECODE_OR_RETURN(request, file_desc);

           UINT32 attributes;
           DECODE_OR_RETURN(request, attributes);

           UINT64 creation_time;
           DECODE_OR_RETURN(request, creation_time);

           UINT64 last_access_time;
           DECODE_OR_RETURN(request, last_access_time);

           UINT64 last_write_time;
           DECODE_OR_RETURN(request, last_write_time);

           UINT64 change_time;
           DECODE_OR_RETURN(request, change_time);

           remote::file_info file_info{};
           ret = this->winfsp_set_basic_info(
               file_desc, attributes, creation_time, last_access_time,
               last_write_time, change_time, &file_info);
           if (ret == STATUS_SUCCESS) {
             response.encode(file_info);
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_set_file_size",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           HANDLE file_desc;
           DECODE_OR_RETURN(request, file_desc);

           UINT64 new_size;
           DECODE_OR_RETURN(request, new_size);

           BOOLEAN set_allocation_size;
           DECODE_OR_RETURN(request, set_allocation_size);

           remote::file_info file_info{};
           ret = this->winfsp_set_file_size(file_desc, new_size,
                                            set_allocation_size, &file_info);
           if (ret == STATUS_SUCCESS) {
             response.encode(file_info);
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_unmounted",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           std::wstring location;
           DECODE_OR_RETURN(request, location);

           ret = this->winfsp_unmounted(location);
           return ret;
         }});
    handler_lookup_.insert(
        {"::winfsp_write",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = STATUS_SUCCESS;

           HANDLE file_desc;
           DECODE_OR_RETURN(request, file_desc);

           UINT32 length;
           DECODE_OR_RETURN(request, length);

           UINT64 offset;
           DECODE_OR_RETURN(request, offset);

           BOOLEAN write_to_end;
           DECODE_OR_RETURN(request, write_to_end);

           BOOLEAN constrained_io;
           DECODE_OR_RETURN(request, constrained_io);

           auto *buffer = request->current_pointer();

           UINT32 bytes_transferred = 0;
           remote::file_info file_info{};
           ret = this->winfsp_write(file_desc, buffer, offset, length,
                                    write_to_end, constrained_io,
                                    &bytes_transferred, &file_info);
           if (ret == STATUS_SUCCESS) {
             response.encode(bytes_transferred);
             response.encode(file_info);
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::fuse_access",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           std::int32_t mask;
           DECODE_OR_RETURN(request, mask);

           return this->fuse_access(&path[0], mask);
         }});
    handler_lookup_.insert(
        {"::fuse_chflags",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           std::uint32_t flags;
           DECODE_OR_RETURN(request, flags);

           return this->fuse_chflags(&path[0], flags);
         }});
    handler_lookup_.insert(
        {"::fuse_chmod",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_mode mode;
           DECODE_OR_RETURN(request, mode);

           return this->fuse_chmod(&path[0], mode);
         }});
    handler_lookup_.insert(
        {"::fuse_chown",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::user_id uid;
           DECODE_OR_RETURN(request, uid);

           remote::group_id gid;
           DECODE_OR_RETURN(request, gid);

           return this->fuse_chown(&path[0], uid, gid);
         }});
    handler_lookup_.insert(
        {"::fuse_create",
         [this](std::uint32_t, const std::string &client_id, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_mode mode;
           DECODE_OR_RETURN(request, mode);

           remote::open_flags flags;
           DECODE_OR_RETURN(request, flags);

           remote::file_handle handle;
           if ((ret = this->fuse_create(&path[0], mode, flags, handle)) >= 0) {
#ifdef _WIN32
             this->set_compat_client_id(handle, client_id);
#else
             this->set_client_id(handle, client_id);
#endif
             response.encode(handle);
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::fuse_destroy",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *, packet &) -> packet::error_type {
           return this->fuse_destroy();
         }});
    /*handlerLookup_.insert({"::fuse_fallocate",
                           [this](std::uint32_t serviceFlags, const std::string
       &client_id, std::uint64_t threadId, const std::string &method, packet
       *request, packet &response) -> packet::error_type { auto ret = 0;

                             std::string path;
                             DECODE_OR_RETURN(request, path);

                             std::int32_t mode;
                             DECODE_OR_RETURN(request, mode);

                             remote::file_offset offset;
                             DECODE_OR_RETURN(request, offset);

                             remote::file_offset length;
                             DECODE_OR_RETURN(request, length);

                             remote::file_handle handle;
                             DECODE_OR_RETURN(request, handle);

                             return this->fuse_fallocate(&path[0], mode, offset,
       length, handle);
                           }});*/
    handler_lookup_.insert(
        {"::fuse_fgetattr",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_handle handle;
           DECODE_OR_RETURN(request, handle);

           remote::user_id uid;
           DECODE_OR_RETURN(request, uid);

           remote::group_id gid;
           DECODE_OR_RETURN(request, gid);

           remote::stat st{};
           bool directory = false;
           ret = this->fuse_fgetattr(&path[0], st, directory, handle);
           if (ret == 0) {
             st.st_uid = uid;
             st.st_gid = gid;
             response.encode(st);
             response.encode(static_cast<std::uint8_t>(directory));
           }

           return ret;
         }});
    handler_lookup_.insert(
        {"::fuse_fsetattr_x",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::setattr_x attr{};
           DECODE_OR_RETURN(request, attr);

           remote::file_handle handle;
           DECODE_OR_RETURN(request, handle);

           return this->fuse_fsetattr_x(&path[0], attr, handle);
         }});
    handler_lookup_.insert(
        {"::fuse_fsync",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           std::int32_t dataSync;
           DECODE_OR_RETURN(request, dataSync);

           remote::file_handle handle;
           DECODE_OR_RETURN(request, handle);

           return this->fuse_fsync(&path[0], dataSync, handle);
         }});
    handler_lookup_.insert(
        {"::fuse_ftruncate",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_offset size;
           DECODE_OR_RETURN(request, size);

           remote::file_handle handle;
           DECODE_OR_RETURN(request, handle);

           return this->fuse_ftruncate(&path[0], size, handle);
         }});
    handler_lookup_.insert(
        {"::fuse_getattr",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::user_id uid;
           DECODE_OR_RETURN(request, uid);

           remote::group_id gid;
           DECODE_OR_RETURN(request, gid);

           remote::stat st{};
           bool directory = false;
           ret = this->fuse_getattr(&path[0], st, directory);
           if (ret == 0) {
             st.st_uid = uid;
             st.st_gid = gid;
             response.encode(st);
             response.encode(static_cast<std::uint8_t>(directory));
           }
           return ret;
         }});
    /*handlerLookup_.insert({"::fuse_getxattr",
                           [this](std::uint32_t serviceFlags, const std::string
    &client_id, std::uint64_t threadId, const std::string &method, packet
    *request, packet &response) -> packet::error_type { auto ret = 0;

                             std::string path;
                             DECODE_OR_RETURN(request, path);

                             std::string name;
                             DECODE_OR_RETURN(request, name);

                             remote::file_size size;
                             DECODE_OR_RETURN(request, size);

                             return this->fuse_getxattr(&path[0], &name[0],
    nullptr, size);
                           }});
    handlerLookup_.insert({"::fuse_getxattr_osx",
                           [this](std::uint32_t serviceFlags, const std::string
    &client_id, std::uint64_t threadId, const std::string &method, packet
    *request, packet &response) -> packet::error_type { auto ret = 0;

                             std::string path;
                             DECODE_OR_RETURN(request, path);

                             std::string name;
                             DECODE_OR_RETURN(request, name);

                             remote::file_size size;
                             DECODE_OR_RETURN(request, size);

                             std::uint32_t position;
                             DECODE_OR_RETURN(request, position);

                             return this->fuse_getxattr_osx(&path[0], &name[0],
    nullptr, size, position);
                           }});*/
    handler_lookup_.insert(
        {"::fuse_getxtimes",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_time bkuptime{};
           remote::file_time crtime{};
           if ((ret = this->fuse_getxtimes(&path[0], bkuptime, crtime)) == 0) {
             response.encode(bkuptime);
             response.encode(crtime);
           }

           return ret;
         }});
    handler_lookup_.insert(
        {"::fuse_init",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *,
                packet &) -> packet::error_type { return this->fuse_init(); }});
    /*handlerLookup_.insert({"::remote_fuseListxattr",
                           [this](std::uint32_t serviceFlags, const std::string
       &client_id, std::uint64_t threadId, const std::string &method, packet
       *request, packet &response) -> packet::error_type { auto ret = 0;

                             std::string path;
                             DECODE_OR_RETURN(request, path);

                             remote::file_size size;
                             DECODE_OR_RETURN(request, size);

                             return this->fuse_listxattr(&path[0], nullptr,
       size);
                           }});*/
    handler_lookup_.insert(
        {"::fuse_mkdir",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_mode mode;
           DECODE_OR_RETURN(request, mode);

           return this->fuse_mkdir(&path[0], mode);
         }});
    handler_lookup_.insert(
        {"::fuse_open",
         [this](std::uint32_t, const std::string &client_id, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::open_flags flags;
           DECODE_OR_RETURN(request, flags);

           remote::file_handle handle;
           if ((ret = this->fuse_open(&path[0], flags, handle)) >= 0) {
#ifdef _WIN32
             this->set_compat_client_id(handle, client_id);
#else
             this->set_client_id(handle, client_id);
#endif
             response.encode(handle);
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::fuse_opendir",
         [this](std::uint32_t, const std::string &client_id, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_handle handle = 0;
           if ((ret = this->fuse_opendir(&path[0], handle)) >= 0) {
             this->add_directory(client_id, reinterpret_cast<void *>(handle));
             response.encode(handle);
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::fuse_read",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_size read_size;
           DECODE_OR_RETURN(request, read_size);

           remote::file_offset read_offset;
           DECODE_OR_RETURN(request, read_offset);

           remote::file_handle handle;
           DECODE_OR_RETURN(request, handle);

           data_buffer buffer;
           if ((ret =
                    this->fuse_read(&path[0], reinterpret_cast<char *>(&buffer),
                                    read_size, read_offset, handle)) > 0) {
             response.encode(&buffer[0], buffer.size());
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::fuse_readdir",
         [this](std::uint32_t, const std::string &client_id, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_offset offset;
           DECODE_OR_RETURN(request, offset);

           remote::file_handle handle;
           DECODE_OR_RETURN(request, handle);

           if (this->has_open_directory(client_id,
                                        reinterpret_cast<void *>(handle))) {
             std::string filePath;
             ret = this->fuse_readdir(&path[0], offset, handle, filePath);
             if (ret == 0) {
               response.encode(filePath);
             }
           } else {
             ret = -EBADF;
           }

           return ret;
         }});
    handler_lookup_.insert(
        {"::fuse_release",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_handle handle;
           DECODE_OR_RETURN(request, handle);

           return this->fuse_release(&path[0], handle);
         }});
    handler_lookup_.insert(
        {"::fuse_releasedir",
         [this](std::uint32_t, const std::string &client_id, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_handle handle;
           DECODE_OR_RETURN(request, handle);

           ret = this->fuse_releasedir(&path[0], handle);
           if (this->remove_directory(client_id,
                                      reinterpret_cast<void *>(handle))) {
             return ret;
           }
           return -EBADF;
         }});
    /*handlerLookup_.insert({"::fuse_removexattr",
                           [this](std::uint32_t serviceFlags, const std::string
       &client_id, std::uint64_t threadId, const std::string &method, packet
       *request, packet &response) -> packet::error_type { auto ret = 0;

                             std::string path;
                             DECODE_OR_RETURN(request, path);

                             std::string name;
                             DECODE_OR_RETURN(request, name);

                             return this->fuse_removexattr(&path[0], &name[0]);
                           }});*/
    handler_lookup_.insert(
        {"::fuse_rename",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string from;
           DECODE_OR_RETURN(request, from);

           std::string to;
           DECODE_OR_RETURN(request, to);

           return this->fuse_rename(&from[0], &to[0]);
         }});
    handler_lookup_.insert(
        {"::fuse_rmdir",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           return this->fuse_rmdir(&path[0]);
         }});
    handler_lookup_.insert(
        {"::fuse_setattr_x",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::setattr_x attr{};
           DECODE_OR_RETURN(request, attr);

           return this->fuse_setattr_x(&path[0], attr);
         }});
    handler_lookup_.insert(
        {"::fuse_setbkuptime",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_time bkuptime;
           DECODE_OR_RETURN(request, bkuptime);

           return this->fuse_setbkuptime(&path[0], bkuptime);
         }});
    handler_lookup_.insert(
        {"::fuse_setchgtime",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_time chgtime;
           DECODE_OR_RETURN(request, chgtime);

           return this->fuse_setchgtime(&path[0], chgtime);
         }});
    handler_lookup_.insert(
        {"::fuse_setcrtime",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_time crtime;
           DECODE_OR_RETURN(request, crtime);

           return this->fuse_setcrtime(&path[0], crtime);
         }});
    handler_lookup_.insert(
        {"::fuse_setvolname",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string name;
           DECODE_OR_RETURN(request, name);

           return this->fuse_setvolname(&name[0]);
         }});
    /*handlerLookup_.insert({"::fuse_setxattr",
                           [this](std::uint32_t serviceFlags, const std::string
    &client_id, std::uint64_t threadId, const std::string &method, packet
    *request, packet &response) -> packet::error_type { auto ret = 0;

                             std::string path;
                             DECODE_OR_RETURN(request, path);

                             std::string name;
                             DECODE_OR_RETURN(request, name);

                             remote::file_size size;
                             DECODE_OR_RETURN(request, size);

                             if (size > std::numeric_limits<std::size_t>::max())
    { return -ERANGE;
                             }

                             data_buffer value(static_cast<std::size_t>(size));
                             ret = request->Decode(&value[0], value.size());
                             if (ret == 0) {
                               std::int32_t flags;
                               DECODE_OR_RETURN(request, flags);

                               ret = this->fuse_setxattr(&path[0], &name[0],
    &value[0], size, flags);
                             }
                             return ret;
                           }});
    handlerLookup_.insert({"::fuse_setxattr_osx",
                           [this](std::uint32_t serviceFlags, const std::string
    &client_id, std::uint64_t threadId, const std::string &method, packet
    *request, packet &response) -> packet::error_type { auto ret = 0;

                             std::string path;
                             DECODE_OR_RETURN(request, path);

                             std::string name;
                             DECODE_OR_RETURN(request, name);

                             remote::file_size size;
                             DECODE_OR_RETURN(request, size);

                             if (size > std::numeric_limits<std::size_t>::max())
    { return -ERANGE;
                             }

                             data_buffer value(static_cast<std::size_t>(size));
                             ret = request->Decode(&value[0], value.size());
                             if (ret == 0) {
                               std::int32_t flags;
                               DECODE_OR_RETURN(request, flags);

                               std::uint32_t position;
                               DECODE_OR_RETURN(request, position);

                               ret = this->fuse_setxattr_osx(&path[0], &name[0],
    &value[0], size, flags, position);
                             }
                             return ret;
                           }});*/
    handler_lookup_.insert(
        {"::fuse_statfs",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = -1;

           std::string path;
           DECODE_OR_RETURN(request, path);

           std::uint64_t frsize;
           DECODE_OR_RETURN(request, frsize);

           remote::statfs st{};
           ret = this->fuse_statfs(&path[0], frsize, st);
           if (ret == 0) {
             response.encode(st);
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::fuse_statfs_x",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           auto ret = -1;

           std::string path;
           DECODE_OR_RETURN(request, path);

           std::uint64_t bsize;
           DECODE_OR_RETURN(request, bsize);

           remote::statfs_x st{};
           ret = this->fuse_statfs_x(&path[0], bsize, st);
           if (ret == 0) {
             response.encode(st);
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::fuse_truncate",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_offset size;
           DECODE_OR_IGNORE(request, size);

           return this->fuse_truncate(&path[0], size);
         }});
    handler_lookup_.insert(
        {"::fuse_unlink",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           return this->fuse_unlink(&path[0]);
         }});
    handler_lookup_.insert(
        {"::fuse_utimens",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_time tv[2] = {0};
           if ((ret = request->decode(&tv[0], sizeof(remote::file_time) * 2)) ==
               0) {
             std::uint64_t op0;
             DECODE_OR_RETURN(request, op0);

             std::uint64_t op1;
             DECODE_OR_RETURN(request, op1);

             ret = this->fuse_utimens(&path[0], tv, op0, op1);
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::fuse_write",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           std::int32_t ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_size writeSize;
           DECODE_OR_RETURN(request, writeSize);

           if (writeSize > std::numeric_limits<std::size_t>::max()) {
             return -ERANGE;
           }

           data_buffer buffer(static_cast<std::size_t>(writeSize));
           if ((ret = request->decode(&buffer[0], buffer.size())) == 0) {
             remote::file_offset write_offset;
             DECODE_OR_RETURN(request, write_offset);

             remote::file_handle handle;
             DECODE_OR_RETURN(request, handle);

             ret = this->fuse_write(&path[0], &buffer[0], writeSize,
                                    write_offset, handle);
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::fuse_write_base64",
         [this](std::uint32_t, const std::string &, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           std::int32_t ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_size write_size;
           DECODE_OR_RETURN(request, write_size);

           if (write_size > std::numeric_limits<std::size_t>::max()) {
             return -ERANGE;
           }

           data_buffer buffer(static_cast<std::size_t>(write_size));
           if ((ret = request->decode(&buffer[0], buffer.size())) == 0) {
             buffer = macaron::Base64::Decode(
                 std::string(buffer.begin(), buffer.end()));
             write_size = buffer.size();

             remote::file_offset write_offset;
             DECODE_OR_RETURN(request, write_offset);

             remote::file_handle handle;
             DECODE_OR_RETURN(request, handle);

             ret = this->fuse_write(&path[0], &buffer[0], write_size,
                                    write_offset, handle);
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::json_create_directory_snapshot",
         [this](std::uint32_t, const std::string &client_id, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           std::int32_t ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           json json_data;
           json_data["handle"] = -1;
           json_data["page_count"] = 0;
           json_data["path"] = path;
           if ((ret = this->json_create_directory_snapshot(&path[0],
                                                           json_data)) == 0) {
             this->add_directory(client_id,
                                 reinterpret_cast<void *>(
                                     json_data["handle"].get<std::uint64_t>()));
             response.encode(json_data.dump(0));
           }
           return ret;
         }});
    handler_lookup_.insert(
        {"::json_read_directory_snapshot",
         [this](std::uint32_t, const std::string &client_id, std::uint64_t,
                const std::string &, packet *request,
                packet &response) -> packet::error_type {
           std::int32_t ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_handle handle;
           DECODE_OR_RETURN(request, handle);

           std::uint32_t page;
           DECODE_OR_RETURN(request, page);

           ret = -EBADF;
           if (this->has_open_directory(client_id,
                                        reinterpret_cast<void *>(handle))) {
             json json_data;
             json_data["directory_list"] = std::vector<json>();
             json_data["page"] = page;
             ret = this->json_read_directory_snapshot(path, handle, page,
                                                      json_data);
             if ((ret == 0) || (ret == -120)) {
               response.encode(json_data.dump(0));
             }
           }

           return ret;
         }});
    handler_lookup_.insert(
        {"::json_release_directory_snapshot",
         [this](std::uint32_t, const std::string &client_id, std::uint64_t,
                const std::string &, packet *request,
                packet &) -> packet::error_type {
           auto ret = 0;

           std::string path;
           DECODE_OR_RETURN(request, path);

           remote::file_handle handle;
           DECODE_OR_RETURN(request, handle);

           ret = this->json_release_directory_snapshot(&path[0], handle);
           if (this->remove_directory(client_id,
                                      reinterpret_cast<void *>(handle))) {
             return ret;
           }
           return -EBADF;
         }});

    packet_server_ = std::make_unique<packet_server>(
        config_.get_remote_port(), config_.get_remote_token(), 10,
        [this](const std::string &client_id) {
          return this->closed_handler(client_id);
        },
        [this](std::uint32_t service_flags, const std::string &client_id,
               std::uint64_t thread_id, const std::string &method,
               packet *request, packet &response,
               packet_server::message_complete_callback message_complete) {
          return this->message_handler(service_flags, client_id, thread_id,
                                       method, request, response,
                                       message_complete);
        });
  }

  ~remote_server_base() override {
    event_system::instance().raise<service_shutdown_begin>(
        "remote_server_base");
    client_pool_.shutdown();
    packet_server_.reset();
    event_system::instance().raise<service_shutdown_end>("remote_server_base");
  }

public:
  using handler_callback = std::function<packet::error_type(
      std::uint32_t, const std::string &, std::uint64_t, const std::string &,
      packet *, packet &)>;

protected:
  app_config &config_;
  drive &drive_;
  const std::string mount_location_;

private:
  client_pool client_pool_;
  std::unique_ptr<packet_server> packet_server_;
  std::unordered_map<std::string, handler_callback> handler_lookup_;

private:
  void closed_handler(const std::string &client_id) {
    client_pool_.remove_client(client_id);
    close_all(client_id);
  }

  void
  message_handler(std::uint32_t service_flags, const std::string &client_id,
                  std::uint64_t thread_id, const std::string &method,
                  packet *request, packet &response,
                  packet_server::message_complete_callback message_complete) {
    const auto idx = method.find_last_of("::");
    const auto lookup_method_name =
        ((idx == std::string::npos) ? "::" + method : method.substr(idx - 1));
    if (handler_lookup_.find(lookup_method_name) == handler_lookup_.end()) {
      message_complete(STATUS_NOT_IMPLEMENTED);
    } else {
      client_pool_.execute(
          client_id, thread_id,
          [this, lookup_method_name, service_flags, client_id, thread_id,
           method, request, &response]() -> packet::error_type {
            return this->handler_lookup_[lookup_method_name](
                service_flags, client_id, thread_id, method, request, response);
          },
          message_complete);
    }
  }

protected:
  [[nodiscard]] auto construct_api_path(std::string path) -> std::string {
    path = utils::path::create_api_path(path.substr(mount_location_.size()));
    return path;
  }

  void delete_open_directory(void *dir) override {
    if (dir) {
      delete reinterpret_cast<directory_iterator *>(dir);
    }
  }
};
} // namespace repertory

#endif // INCLUDE_DRIVES_REMOTE_REMOTE_SERVER_BASE_HPP_
