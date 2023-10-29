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

#include "utils/windows/windows_utils.hpp"

#include "types/startup_exception.hpp"
#include "utils/com_init_wrapper.hpp"
#include "utils/string_utils.hpp"

#ifndef STATUS_DEVICE_INSUFFICIENT_RESOURCES
#define STATUS_DEVICE_INSUFFICIENT_RESOURCES ((NTSTATUS)0xC0000468L)
#endif

namespace repertory::utils {
auto from_api_error(const api_error &e) -> NTSTATUS {
  switch (e) {
  case api_error::access_denied:
    return STATUS_ACCESS_DENIED;
  case api_error::bad_address:
    return STATUS_INVALID_ADDRESS;
  case api_error::buffer_too_small:
    return STATUS_BUFFER_TOO_SMALL;
  case api_error::buffer_overflow:
    return STATUS_BUFFER_OVERFLOW;
  case api_error::directory_end_of_files:
    return STATUS_NO_MORE_FILES;
  case api_error::directory_exists:
    return STATUS_OBJECT_NAME_EXISTS;
  case api_error::directory_not_empty:
    return STATUS_DIRECTORY_NOT_EMPTY;
  case api_error::directory_not_found:
    return STATUS_OBJECT_NAME_NOT_FOUND;
  case api_error::download_failed:
    return FspNtStatusFromWin32(ERROR_INTERNAL_ERROR);
  case api_error::error:
    return FspNtStatusFromWin32(ERROR_INTERNAL_ERROR);
  case api_error::invalid_handle:
    return STATUS_INVALID_HANDLE;
  case api_error::invalid_operation:
    return STATUS_INVALID_PARAMETER;
  case api_error::item_exists:
    return FspNtStatusFromWin32(ERROR_FILE_EXISTS);
  case api_error::file_in_use:
    return STATUS_DEVICE_BUSY;
  case api_error::incompatible_version:
    return STATUS_CLIENT_SERVER_PARAMETERS_INVALID;
  case api_error::item_not_found:
    return STATUS_OBJECT_NAME_NOT_FOUND;
  case api_error::no_disk_space:
    return STATUS_DEVICE_INSUFFICIENT_RESOURCES;
  case api_error::os_error:
    return FspNtStatusFromWin32(::GetLastError());
  case api_error::out_of_memory:
    return STATUS_NO_MEMORY;
  case api_error::permission_denied:
    return FspNtStatusFromWin32(ERROR_ACCESS_DENIED);
  case api_error::success:
    return 0;
  case api_error::not_supported:
    return FspNtStatusFromWin32(ERROR_INTERNAL_ERROR);
  case api_error::not_implemented:
    return STATUS_NOT_IMPLEMENTED;
  case api_error::upload_failed:
    return FspNtStatusFromWin32(ERROR_INTERNAL_ERROR);
  default:
    return FspNtStatusFromWin32(ERROR_INTERNAL_ERROR);
  }
}

auto get_last_error_code() -> DWORD { return ::GetLastError(); }

auto get_local_app_data_directory() -> const std::string & {
  static std::string app_data = ([]() -> std::string {
    com_init_wrapper cw;
    PWSTR local_app_data{};
    if (SUCCEEDED(::SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr,
                                         &local_app_data))) {
      auto app_data = utils::string::to_utf8(local_app_data);
      ::CoTaskMemFree(local_app_data);
      return app_data;
    }

    throw startup_exception("unable to detect local application data folder");
  })();

  return app_data;
}

auto get_accessed_time_from_meta(const api_meta_map &meta) -> std::uint64_t {
  return utils::string::to_uint64(meta.at(META_ACCESSED));
}

auto get_changed_time_from_meta(const api_meta_map &meta) -> std::uint64_t {
  return utils::string::to_uint64(meta.at(META_MODIFIED));
}

auto get_creation_time_from_meta(const api_meta_map &meta) -> std::uint64_t {
  return utils::string::to_uint64(meta.at(META_CREATION));
}

auto get_written_time_from_meta(const api_meta_map &meta) -> std::uint64_t {
  return utils::string::to_uint64(meta.at(META_WRITTEN));
}

auto get_thread_id() -> std::uint64_t {
  return static_cast<std::uint64_t>(::GetCurrentThreadId());
}

auto is_process_elevated() -> bool {
  auto ret = false;
  HANDLE token = INVALID_HANDLE_VALUE;
  if (::OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
    TOKEN_ELEVATION te{};
    DWORD sz = sizeof(te);
    if (::GetTokenInformation(token, TokenElevation, &te, sizeof(te), &sz)) {
      ret = (te.TokenIsElevated != 0);
    }
  }

  if (token != INVALID_HANDLE_VALUE) {
    ::CloseHandle(token);
  }

  return ret;
}

auto run_process_elevated(int argc, char *argv[]) -> int {
  std::cout << "Elevating Process" << std::endl;
  std::string parameters = "-hidden";
  for (int i = 1; i < argc; i++) {
    parameters += (parameters.empty() ? argv[i] : " " + std::string(argv[i]));
  }

  char full_path[MAX_PATH] = {0};
  if (::GetModuleFileName(nullptr, full_path, MAX_PATH)) {
    SHELLEXECUTEINFO sei{};
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.cbSize = sizeof(sei);
    sei.lpVerb = "runas";
    sei.lpFile = &full_path[0];
    sei.lpParameters = &parameters[0];
    sei.hwnd = nullptr;
    sei.nShow = SW_NORMAL;
    if (::ShellExecuteEx(&sei)) {
      ::WaitForSingleObject(sei.hProcess, INFINITE);
      DWORD exit_code = 0u;
      ::GetExitCodeProcess(sei.hProcess, &exit_code);
      ::CloseHandle(sei.hProcess);
      return exit_code;
    }
  }

  return ::GetLastError();
}

void set_last_error_code(DWORD error_code) { ::SetLastError(error_code); }

// https://stackoverflow.com/questions/321849/strptime-equivalent-on-windows
auto strptime(const char *s, const char *f, struct tm *tm) -> const char * {
  std::istringstream input(s);
  input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
  input >> std::get_time(tm, f);
  if (input.fail()) {
    return nullptr;
  }

  return reinterpret_cast<const char *>(s + input.tellg());
}

auto unix_access_mask_to_windows(std::int32_t mask) -> int {
  if (mask & 1) {
    mask -= 1;
    if (not mask) {
      mask = 4;
    }
  }

  return mask & 6;
}

auto unix_open_flags_to_flags_and_perms(const remote::file_mode & /*mode*/,
                                        const remote::open_flags &flags,
                                        std::int32_t &perms) -> int {
  auto ret = _O_BINARY | _O_RANDOM;
  ret |= (((flags & remote::open_flags::create) == remote::open_flags::create)
              ? _O_CREAT
              : 0);
  ret |= (((flags & remote::open_flags::excl) == remote::open_flags::excl)
              ? _O_EXCL
              : 0);
  ret |=
      (((flags & remote::open_flags::truncate) == remote::open_flags::truncate)
           ? _O_TRUNC
           : 0);
  ret |= (((flags & remote::open_flags::append) == remote::open_flags::append)
              ? _O_APPEND
              : 0);
  ret |= (((flags & remote::open_flags::temp_file) ==
           remote::open_flags::temp_file)
              ? _O_TEMPORARY
              : 0);
  ret |= ((flags & remote::open_flags::write_only) ==
          remote::open_flags::write_only)
             ? _O_WRONLY
         : ((flags & remote::open_flags::read_write) ==
            remote::open_flags::read_write)
             ? _O_RDWR
             : _O_RDONLY;

  perms = _S_IREAD | _S_IWRITE;

  return ret;
}

auto time64_to_unix_time(const __time64_t &t) -> remote::file_time {
  return t * NANOS_PER_SECOND;
}
} // namespace repertory::utils

#endif // _WIN32
