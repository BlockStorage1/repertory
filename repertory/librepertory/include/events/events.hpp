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
#ifndef REPERTORY_INCLUDE_EVENTS_EVENTS_HPP_
#define REPERTORY_INCLUDE_EVENTS_EVENTS_HPP_

#include "events/event_system.hpp"
#include "types/repertory.hpp"
#include "utils/utils.hpp"

namespace repertory {
// clang-format off
E_SIMPLE2(curl_error, error, true,
  std::string, url, url, E_FROM_STRING,
  CURLcode, res, res, E_FROM_CURL_CODE 
);

E_SIMPLE3(debug_log, debug, true,
  std::string, function, func, E_FROM_STRING,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, data, data, E_FROM_STRING
);

E_SIMPLE1(directory_removed, info, true,
  std::string, api_path, ap, E_FROM_STRING
);

E_SIMPLE2(directory_removed_externally, warn, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, source, src, E_FROM_STRING
);

E_SIMPLE2(directory_remove_failed, error, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, error, err, E_FROM_STRING
);

E_SIMPLE2(drive_mount_failed, error, true,
  std::string, location, loc, E_FROM_STRING,
  std::string, result, res, E_FROM_STRING
);

E_SIMPLE1(drive_mounted, info, true,
  std::string, location, loc, E_FROM_STRING
);

E_SIMPLE1(drive_mount_result, info, true,
  std::string, result, res, E_FROM_STRING
);

E_SIMPLE1(drive_unmount_pending, info, true,
  std::string, location, loc, E_FROM_STRING
);

E_SIMPLE1(drive_unmounted, info, true,
  std::string, location, loc, E_FROM_STRING
);

E_SIMPLE1(event_level_changed, info, true,
  std::string, new_event_level, level, E_FROM_STRING
);

E_SIMPLE1(failed_upload_queued, error, true,
  std::string, api_path, ap, E_FROM_STRING
);

E_SIMPLE1(failed_upload_removed, warn, true,
  std::string, api_path, ap, E_FROM_STRING
);

E_SIMPLE1(failed_upload_retry, info, true,
  std::string, api_path, ap, E_FROM_STRING
);

E_SIMPLE2(file_get_failed, error, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, error, err, E_FROM_STRING
);

E_SIMPLE1(file_get_api_list_failed, error, true,
  std::string, error, err, E_FROM_STRING
);

E_SIMPLE1(file_pinned, info, true,
  std::string, api_path, ap, E_FROM_STRING
);

E_SIMPLE3(file_read_bytes_failed, error, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, error, err, E_FROM_STRING,
  std::size_t, retry, retry, E_FROM_SIZE_T
);

E_SIMPLE1(file_removed, debug, true,
  std::string, api_path, ap, E_FROM_STRING
);

E_SIMPLE2(file_removed_externally, warn, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, source, src, E_FROM_STRING
);

E_SIMPLE2(file_remove_failed, error, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, error, err, E_FROM_STRING
);

E_SIMPLE3(file_rename_failed, error, true,
  std::string, from_api_path, FROM, E_FROM_STRING,
  std::string, to_api_path, TO, E_FROM_STRING,
  std::string, error, err, E_FROM_STRING
);

E_SIMPLE2(file_get_size_failed, error, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, error, err, E_FROM_STRING
);

E_SIMPLE3(filesystem_item_added, debug, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, parent, parent, E_FROM_STRING,
  bool, directory, dir, E_FROM_BOOL
);

E_SIMPLE4(filesystem_item_closed, trace, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, source, src, E_FROM_STRING,
  bool, directory, dir, E_FROM_BOOL,
  bool, changed, changed, E_FROM_BOOL
);

E_SIMPLE5(filesystem_item_handle_closed, trace, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::uint64_t, handle, handle, E_FROM_UINT64,
  std::string, source, src, E_FROM_STRING,
  bool, directory, dir, E_FROM_BOOL,
  bool, changed, changed, E_FROM_BOOL
);

E_SIMPLE4(filesystem_item_handle_opened, trace, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::uint64_t, handle, handle, E_FROM_UINT64,
  std::string, source, src, E_FROM_STRING,
  bool, directory, dir, E_FROM_BOOL
);

E_SIMPLE2(filesystem_item_evicted, info, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, source, src, E_FROM_STRING
);

E_SIMPLE3(filesystem_item_opened, trace, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, source, src, E_FROM_STRING,
  bool, directory, dir, E_FROM_BOOL
);

E_SIMPLE1(file_unpinned, info, true,
  std::string, api_path, ap, E_FROM_STRING
);

E_SIMPLE4(file_upload_completed, info, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, source, src, E_FROM_STRING,
  api_error, result, res, E_FROM_API_FILE_ERROR,
  bool, cancelled, cancel, E_FROM_BOOL
);

E_SIMPLE3(file_upload_failed, error, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, source, src, E_FROM_STRING,
  std::string, error, err, E_FROM_STRING
);

E_SIMPLE2(file_upload_not_found, warn, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, source, src, E_FROM_STRING
);

E_SIMPLE2(file_upload_queued, info, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, source, src, E_FROM_STRING
);

E_SIMPLE1(file_upload_removed, debug, true,
  std::string, api_path, ap, E_FROM_STRING
);

E_SIMPLE3(file_upload_retry, info, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, source, src, E_FROM_STRING,
  api_error, result, res, E_FROM_API_FILE_ERROR
);

E_SIMPLE2(file_upload_started, info, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, source, src, E_FROM_STRING
);

E_SIMPLE1(orphaned_file_deleted, warn, true,
  std::string, source, src, E_FROM_STRING
);

E_SIMPLE1(orphaned_file_detected, warn, true,
  std::string, source, src, E_FROM_STRING
);

E_SIMPLE3(orphaned_file_processing_failed, error, true,
  std::string, source, src, E_FROM_STRING,
  std::string, dest, dest, E_FROM_STRING,
  std::string, result, res, E_FROM_STRING
);

E_SIMPLE1(orphaned_source_file_detected, info, true,
  std::string, source, src, E_FROM_STRING
);

E_SIMPLE1(orphaned_source_file_removed, info, true,
  std::string, source, src, E_FROM_STRING
);

E_SIMPLE1(polling_item_begin, debug, true,
  std::string, item_name, item, E_FROM_STRING
);

E_SIMPLE1(polling_item_end, debug, true,
  std::string, item_name, item, E_FROM_STRING
);

E_SIMPLE2(provider_offline, error, true,
  std::string, host_name_or_ip, host, E_FROM_STRING,
  std::uint16_t, port, port, E_FROM_UINT16
);

E_SIMPLE2(provider_upload_begin, info, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, source, src, E_FROM_STRING
);

E_SIMPLE3(provider_upload_end, info, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, source, src, E_FROM_STRING,
  api_error, result, res, E_FROM_API_FILE_ERROR
);

E_SIMPLE2(repertory_exception, error, true,
  std::string, function, func, E_FROM_STRING,
  std::string, message, msg, E_FROM_STRING
);

E_SIMPLE1(rpc_server_exception, error, true,
  std::string, exception, exception, E_FROM_STRING
);

E_SIMPLE1(service_shutdown_begin, debug, true,
  std::string, service, svc, E_FROM_STRING
);

E_SIMPLE1(service_shutdown_end, debug, true,
  std::string, service, svc, E_FROM_STRING
);

E_SIMPLE1(service_started, debug, true,
  std::string, service, svc, E_FROM_STRING
);

E_SIMPLE(unmount_requested, info, true);
#if !defined(_WIN32)
E_SIMPLE2(unmount_result, info, true,
  std::string, location, loc, E_FROM_STRING,
  std::string, result, res, E_FROM_STRING
);
#endif
// clang-format on
} // namespace repertory

#endif // REPERTORY_INCLUDE_EVENTS_EVENTS_HPP_
