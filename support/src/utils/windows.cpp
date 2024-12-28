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
#if defined(_WIN32)

#include "utils/windows.hpp"

#include "utils/com_init_wrapper.hpp"
#include "utils/error.hpp"
#include "utils/string.hpp"

namespace repertory::utils {
void create_console() {
  if (::AllocConsole() == 0) {
    return;
  }

  FILE *dummy{nullptr};
  freopen_s(&dummy, "CONOUT$", "w", stdout);
  freopen_s(&dummy, "CONOUT$", "w", stderr);
  freopen_s(&dummy, "CONIN$", "r", stdin);
  std::cout.clear();
  std::clog.clear();
  std::cerr.clear();
  std::cin.clear();

  auto *out_w = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  auto *in_w = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  SetStdHandle(STD_OUTPUT_HANDLE, out_w);
  SetStdHandle(STD_ERROR_HANDLE, out_w);
  SetStdHandle(STD_INPUT_HANDLE, in_w);
  std::wcout.clear();
  std::wclog.clear();
  std::wcerr.clear();
  std::wcin.clear();
}

void free_console() { ::FreeConsole(); }

auto get_last_error_code() -> DWORD { return ::GetLastError(); }

auto get_local_app_data_directory() -> const std::string & {
  REPERTORY_USES_FUNCTION_NAME();

  static std::string app_data = ([]() -> std::string {
    com_init_wrapper cw;
    PWSTR local_app_data{};
    if (SUCCEEDED(::SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr,
                                         &local_app_data))) {
      auto ret = utils::string::to_utf8(local_app_data);
      ::CoTaskMemFree(local_app_data);
      return ret;
    }

    throw utils::error::create_exception(
        function_name, {
                           "unable to detect local application data folder",
                       });
  })();

  return app_data;
}

auto get_thread_id() -> std::uint64_t {
  return static_cast<std::uint64_t>(::GetCurrentThreadId());
}

auto is_process_elevated() -> bool {
  auto ret{false};
  HANDLE token{INVALID_HANDLE_VALUE};
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

auto run_process_elevated(std::vector<const char *> args) -> int {
  std::cout << "Elevating Process" << std::endl;
  std::string parameters{"-hidden"};
  for (std::size_t idx = 1U; idx < args.size(); idx++) {
    parameters +=
        (parameters.empty() ? args.at(idx) : " " + std::string(args.at(idx)));
  }

  std::string full_path;
  full_path.resize(repertory::max_path_length + 1);

  if (::GetModuleFileNameA(nullptr, full_path.data(),
                           repertory::max_path_length)) {
    SHELLEXECUTEINFOA sei{};
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.cbSize = sizeof(sei);
    sei.lpVerb = "runas";
    sei.lpFile = full_path.c_str();
    sei.lpParameters = parameters.c_str();
    sei.hwnd = nullptr;
    sei.nShow = SW_NORMAL;
    if (::ShellExecuteExA(&sei)) {
      ::WaitForSingleObject(sei.hProcess, INFINITE);
      DWORD exit_code{};
      ::GetExitCodeProcess(sei.hProcess, &exit_code);
      ::CloseHandle(sei.hProcess);
      return static_cast<int>(exit_code);
    }
  }

  return static_cast<int>(::GetLastError());
}

void set_last_error_code(DWORD error_code) { ::SetLastError(error_code); }
} // namespace repertory::utils

#endif // defined(_WIN32)
