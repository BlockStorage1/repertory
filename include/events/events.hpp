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
#ifndef INCLUDE_EVENTS_EVENTS_HPP_
#define INCLUDE_EVENTS_EVENTS_HPP_

#include "events/event_system.hpp"
#include "types/repertory.hpp"
#include "utils/utils.hpp"

namespace repertory {
// clang-format off
E_SIMPLE3(cache_file_processed, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING,
  api_error, result, res, E_FROM_API_FILE_ERROR
);

E_SIMPLE2(comm_auth_begin, normal, true,
  std::string, url, url, E_STRING,
  std::string, user, user, E_STRING
);

E_SIMPLE4(comm_auth_end, normal, true,
  std::string, url, url, E_STRING,
  std::string, user, user, E_STRING,
  CURLcode, curl, curl, E_FROM_INT32,
  int, http, http, E_FROM_INT32
);

E_SIMPLE2(comm_auth_logout_begin, normal, true,
  std::string, url, url, E_STRING,
  std::string, user, user, E_STRING
);

E_SIMPLE4(comm_auth_logout_end, normal, true,
  std::string, url, url, E_STRING,
  std::string, user, user, E_STRING,
  CURLcode, curl, curl, E_FROM_INT32,
  int, http, http, E_FROM_INT32
);

E_SIMPLE1(comm_del_begin, verbose, true,
  std::string, url, url, E_STRING
);

E_SIMPLE4(comm_del_end, verbose, true,
  std::string, url, url, E_STRING,
  CURLcode, curl, curl, E_FROM_INT32,
  int, http, http, E_FROM_INT32,
  std::string, result, res, E_STRING
);

E_SIMPLE1(comm_get_begin, verbose, true,
  std::string, url, url, E_STRING
);

E_SIMPLE4(comm_get_end, verbose, true,
  std::string, url, url, E_STRING,
  CURLcode, curl, curl, E_FROM_INT32,
  int, http, http, E_FROM_INT32,
  std::string, result, res, E_STRING
);

E_SIMPLE1(comm_get_range_begin, verbose, true,
  std::string, url, url, E_STRING
);

E_SIMPLE4(comm_get_range_end, verbose, true,
  std::string, url, url, E_STRING,
  CURLcode, curl, curl, E_FROM_INT32,
  int, http, http, E_FROM_INT32,
  std::string, error, err, E_STRING
);

E_SIMPLE1(comm_get_raw_begin, verbose, true,
  std::string, url, url, E_STRING
);

E_SIMPLE4(comm_get_raw_end, verbose, true,
  std::string, url, url, E_STRING,
  CURLcode, curl, curl, E_FROM_INT32,
  int, http, http, E_FROM_INT32,
  std::string, error, err, E_STRING
);

E_SIMPLE2(comm_post_begin, verbose, true,
  std::string, url, url, E_STRING,
  std::string, fields, fields, E_STRING
);

E_SIMPLE4(comm_post_end, verbose, true,
  std::string, url, url, E_STRING,
  CURLcode, curl, curl, E_FROM_INT32,
  int, http, http, E_FROM_INT32,
  std::string, result, res, E_STRING
);

E_SIMPLE2(comm_duration, normal, true,
  std::string, url, url, E_STRING,
  std::string, duration, duration, E_STRING
);

E_SIMPLE1(comm_post_file_begin, debug, true,
  std::string, url, url, E_STRING
);

E_SIMPLE4(comm_post_file_end, debug, true,
  std::string, url, url, E_STRING,
  CURLcode, curl, curl, E_FROM_INT32,
  int, http, http, E_FROM_INT32,
  std::string, error, err, E_STRING
);

E_SIMPLE1(comm_post_multi_part_file_begin, debug, true,
  std::string, url, url, E_STRING
);

E_SIMPLE4(comm_post_multi_part_file_end, debug, true,
  std::string, url, url, E_STRING,
  CURLcode, curl, curl, E_FROM_INT32,
  int, http, http, E_FROM_INT32,
  std::string, error, err, E_STRING
);

E_SIMPLE4(comm_tus_upload_begin, debug, true,
  std::string, file_name, fn, E_STRING,
  std::string, url, url, E_STRING,
  std::uint64_t, remain, rem, E_FROM_UINT64,
  std::uint64_t, offset, off, E_FROM_UINT64
);

E_SIMPLE6(comm_tus_upload_end, debug, true,
  std::string, file_name, fn, E_STRING,
  std::string, url, url, E_STRING,
  std::uint64_t, remain, rem, E_FROM_UINT64,
  std::uint64_t, offset, OFF, E_FROM_UINT64,
  CURLcode, curl, curl, E_FROM_INT32,
  int, http, http, E_FROM_INT32
);

E_SIMPLE2(comm_tus_upload_create_begin, debug, true,
  std::string, file_name, fn, E_STRING,
  std::string, url, url, E_STRING
);

E_SIMPLE4(comm_tus_upload_create_end, debug, true,
  std::string, file_name, fn, E_STRING,
  std::string, url, url, E_STRING,
  CURLcode, curl, curl, E_FROM_INT32,
  int, http, http, E_FROM_INT32
);

E_SIMPLE3(debug_log, debug, true,
  std::string, function, func, E_STRING,
  std::string, api_path, ap, E_STRING,
  std::string, data, data, E_STRING
);

E_SIMPLE1(directory_removed, normal, true,
  std::string, api_path, ap, E_STRING
);

E_SIMPLE2(directory_removed_externally, warn, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING
);

E_SIMPLE2(directory_remove_failed, error, true,
  std::string, api_path, ap, E_STRING,
  std::string, error, err, E_STRING
);

E_SIMPLE2(drive_mount_failed, error, true,
  std::string, location, loc, E_STRING,
  std::string, result, res, E_STRING
);

E_SIMPLE1(drive_mounted, normal, true,
  std::string, location, loc, E_STRING
);

E_SIMPLE1(drive_mount_result, normal, true,
  std::string, result, res, E_STRING
);

E_SIMPLE1(drive_unmount_pending, normal, true,
  std::string, location, loc, E_STRING
);

E_SIMPLE1(drive_unmounted, normal, true,
  std::string, location, loc, E_STRING
);

E_SIMPLE1(event_level_changed, normal, true,
  std::string, new_event_level, level, E_STRING
);

E_SIMPLE1(failed_upload_queued, error, true,
  std::string, api_path, ap, E_STRING
);

E_SIMPLE1(failed_upload_removed, warn, true,
  std::string, api_path, ap, E_STRING
);

E_SIMPLE1(failed_upload_retry, normal, true,
  std::string, api_path, ap, E_STRING
);

E_SIMPLE2(file_get_failed, error, true,
  std::string, api_path, ap, E_STRING,
  std::string, error, err, E_STRING
);

E_SIMPLE1(file_get_api_list_failed, error, true,
  std::string, error, err, E_STRING
);

E_SIMPLE1(file_pinned, normal, true,
  std::string, api_path, ap, E_STRING
);

E_SIMPLE3(file_read_bytes_failed, error, true,
  std::string, api_path, ap, E_STRING,
  std::string, error, err, E_STRING,
  std::size_t, retry, retry, E_FROM_SIZE_T
);

E_SIMPLE1(file_removed, normal, true,
  std::string, api_path, ap, E_STRING
);

E_SIMPLE2(file_removed_externally, warn, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING
);

E_SIMPLE2(file_remove_failed, error, true,
  std::string, api_path, ap, E_STRING,
  std::string, error, err, E_STRING
);

E_SIMPLE3(file_rename_failed, error, true,
  std::string, from_api_path, FROM, E_STRING,
  std::string, to_api_path, TO, E_STRING,
  std::string, error, err, E_STRING
);

E_SIMPLE2(file_get_size_failed, error, true,
  std::string, api_path, ap, E_STRING,
  std::string, error, err, E_STRING
);

E_SIMPLE3(filesystem_item_added, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, parent, parent, E_STRING,
  bool, directory, dir, E_FROM_BOOL
);

E_SIMPLE4(filesystem_item_closed, verbose, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING,
  bool, directory, dir, E_FROM_BOOL,
  bool, changed, changed, E_FROM_BOOL
);

E_SIMPLE5(filesystem_item_handle_closed, verbose, true,
  std::string, api_path, ap, E_STRING,
  std::uint64_t, handle, handle, E_FROM_UINT64,
  std::string, source, src, E_STRING,
  bool, directory, dir, E_FROM_BOOL,
  bool, changed, changed, E_FROM_BOOL
);

E_SIMPLE4(filesystem_item_handle_opened, verbose, true,
  std::string, api_path, ap, E_STRING,
  std::uint64_t, handle, handle, E_FROM_UINT64,
  std::string, source, src, E_STRING,
  bool, directory, dir, E_FROM_BOOL
);

E_SIMPLE2(filesystem_item_evicted, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING
);

E_SIMPLE2(filesystem_item_get_failed, error, true,
  std::string, api_path, ap, E_STRING,
  std::string, error, err, E_STRING
);

E_SIMPLE3(filesystem_item_opened, verbose, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING,
  bool, directory, dir, E_FROM_BOOL
);

E_SIMPLE1(file_unpinned, normal, true,
  std::string, api_path, ap, E_STRING
);

E_SIMPLE4(file_upload_completed, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING,
  api_error, result, res, E_FROM_API_FILE_ERROR,
  bool, cancelled, cancel, E_FROM_BOOL
);

E_SIMPLE3(file_upload_failed, error, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING,
  std::string, error, err, E_STRING
);

E_SIMPLE2(file_upload_not_found, warn, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING
);

E_SIMPLE2(file_upload_queued, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING
);

E_SIMPLE2(file_upload_removed, debug, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING
);

E_SIMPLE3(file_upload_retry, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING,
  api_error, result, res, E_FROM_API_FILE_ERROR
);

E_SIMPLE2(file_upload_started, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING
);

E_SIMPLE(item_scan_begin, normal, true);

E_SIMPLE(item_scan_end, normal, true);

E_SIMPLE1(orphaned_file_deleted, warn, true,
  std::string, source, src, E_STRING
);

E_SIMPLE1(orphaned_file_detected, warn, true,
  std::string, source, src, E_STRING
);

E_SIMPLE2(orphaned_file_processed, warn, true,
  std::string, source, src, E_STRING,
  std::string, dest, dest, E_STRING
);

E_SIMPLE3(orphaned_file_processing_failed, error, true,
  std::string, source, src, E_STRING,
  std::string, dest, dest, E_STRING,
  std::string, result, res, E_STRING
);

E_SIMPLE1(polling_item_begin, debug, true,
  std::string, item_name, item, E_STRING
);

E_SIMPLE1(polling_item_end, debug, true,
  std::string, item_name, item, E_STRING
);

E_SIMPLE2(provider_offline, error, true,
  std::string, host_name_or_ip, host, E_STRING,
  std::uint16_t, port, port, E_FROM_UINT16
);

E_SIMPLE2(provider_upload_begin, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING
);

E_SIMPLE3(provider_upload_end, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, source, src, E_STRING,
  api_error, result, res, E_FROM_API_FILE_ERROR
);

E_SIMPLE2(repertory_exception, error, true,
  std::string, function, func, E_STRING,
  std::string, message, msg, E_STRING
);

E_SIMPLE1(rpc_server_exception, error, true,
  std::string, exception, exception, E_STRING
);

E_SIMPLE1(service_shutdown_begin, debug, true,
  std::string, service, svc, E_STRING
);

E_SIMPLE1(service_shutdown_end, debug, true,
  std::string, service, svc, E_STRING
);

E_SIMPLE1(service_started, debug, true,
  std::string, service, svc, E_STRING
);

E_SIMPLE(unmount_requested, normal, true);
#ifndef _WIN32
E_SIMPLE2(unmount_result, normal, true,
  std::string, location, loc, E_STRING,
  std::string, result, res, E_STRING
);
#endif
// clang-format on
} // namespace repertory

#endif // INCLUDE_EVENTS_EVENTS_HPP_
