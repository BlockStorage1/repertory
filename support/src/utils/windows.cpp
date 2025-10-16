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
#if defined(_WIN32)

#include "utils/windows.hpp"

#include "utils/com_init_wrapper.hpp"
#include "utils/error.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"

namespace {
constexpr std::array<std::string_view, 26U> drive_letters{
    "a:", "b:", "c:", "d:", "e:", "f:", "g:", "h:", "i:",
    "j:", "k:", "l:", "m:", "n:", "o:", "p:", "q:", "r:",
    "s:", "t:", "u:", "v:", "w:", "x:", "y:", "z:",
};
}

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

auto get_available_drive_letter(char first) -> std::optional<std::string_view> {
  const auto *begin = std::ranges::find_if(
      drive_letters, [first](auto &&val) { return val.at(0U) == first; });
  if (begin == drive_letters.end()) {
    begin = drive_letters.begin();
  }

  auto available =
      std::ranges::find_if(begin, drive_letters.end(), [](auto &&val) -> bool {
        return not utils::file::directory{utils::path::combine(val, {"\\"})}
                       .exists();
      });
  if (available == drive_letters.end()) {
    return std::nullopt;
  }

  return *available;
}

auto get_available_drive_letters(char first) -> std::vector<std::string_view> {
  const auto *begin =
      std::ranges::find_if(drive_letters, [first](auto &&val) -> bool {
        return val.at(0U) == first;
      });
  if (begin == drive_letters.end()) {
    begin = drive_letters.begin();
  }

  return std::accumulate(
      begin, drive_letters.end(), std::vector<std::string_view>(),
      [](auto &&vec, auto &&letter) -> auto {
        if (utils::file::directory{utils::path::combine(letter, {"\\"})}
                .exists()) {
          return vec;
        }

        vec.emplace_back(letter);
        return vec;
      });
}

auto get_last_error_code() -> DWORD { return ::GetLastError(); }

auto get_local_app_data_directory() -> const std::string & {
  REPERTORY_USES_FUNCTION_NAME();

  [[maybe_unused]] thread_local const utils::com_init_wrapper wrapper;

  static std::string app_data = ([]() -> std::string {
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
    TOKEN_ELEVATION token_elevation{};
    DWORD size = sizeof(token_elevation);
    if (::GetTokenInformation(token, TokenElevation, &token_elevation,
                              sizeof(token_elevation), &size)) {
      ret = (token_elevation.TokenIsElevated != 0);
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

auto get_startup_folder() -> std::wstring {
  [[maybe_unused]] thread_local const utils::com_init_wrapper wrapper;

  PWSTR raw{nullptr};
  auto result = ::SHGetKnownFolderPath(FOLDERID_Startup, 0, nullptr, &raw);
  if (FAILED(result)) {
    if (raw != nullptr) {
      ::CoTaskMemFree(raw);
    }

    return {};
  }

  std::wstring str{raw};
  ::CoTaskMemFree(raw);
  return str;
}

auto create_shortcut(const shortcut_cfg &cfg, bool overwrite_existing) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  [[maybe_unused]] thread_local const utils::com_init_wrapper wrapper;

  const auto hr_hex = [](HRESULT result) -> std::string {
    std::ostringstream oss;
    oss << "0x" << std::uppercase << std::hex << std::setw(8)
        << std::setfill('0') << static_cast<std::uint32_t>(result);
    return oss.str();
  };

  if (cfg.location.empty()) {
    utils::error::handle_error(function_name, "shortcut location was empty");
    return false;
  }

  if (not utils::file::directory{cfg.location}.create_directory()) {
    utils::error::handle_error(function_name,
                               "failed to create shortcut directory|path|" +
                                   utils::string::to_utf8(cfg.location));
    return false;
  }

  auto final_name = cfg.shortcut_name.empty()
                        ? utils::path::strip_to_file_name(cfg.exe_path)
                        : cfg.shortcut_name;
  if (not final_name.ends_with(L".lnk")) {
    final_name += L".lnk";
  }

  auto lnk_path = utils::path::combine(cfg.location, {final_name});
  if (utils::file::file{lnk_path}.exists() && not overwrite_existing) {
    return true;
  }

  IShellLinkW *psl{nullptr};
  auto result = ::CoCreateInstance(CLSID_ShellLink, nullptr,
                                   CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
  if (FAILED(result)) {
    utils::error::handle_error(
        function_name,
        std::string("CoCreateInstance(CLSID_ShellLink) failed: ") +
            hr_hex(result));
    return false;
  }

  result = psl->SetPath(cfg.exe_path.c_str());
  if (FAILED(result)) {
    utils::error::handle_error(function_name,
                               std::string("IShellLink::SetPath failed: ") +
                                   hr_hex(result));
    psl->Release();
    return false;
  }

  if (not cfg.arguments.empty()) {
    result = psl->SetArguments(cfg.arguments.c_str());
    if (FAILED(result)) {
      utils::error::handle_error(
          function_name,
          std::string("IShellLink::SetArguments failed: ") + hr_hex(result));
      psl->Release();
      return false;
    }
  }

  if (not cfg.working_directory.empty()) {
    result = psl->SetWorkingDirectory(cfg.working_directory.c_str());
    if (FAILED(result)) {
      utils::error::handle_error(
          function_name,
          std::string("IShellLink::SetWorkingDirectory failed: ") +
              hr_hex(result));
      psl->Release();
      return false;
    }
  }

  result = psl->SetShowCmd(SW_SHOWNORMAL);
  if (FAILED(result)) {
    utils::error::handle_error(function_name,
                               std::string("IShellLink::SetShowCmd failed: ") +
                                   hr_hex(result));
    psl->Release();
    return false;
  }

  if (not cfg.icon_path.empty()) {
    result = psl->SetIconLocation(cfg.icon_path.c_str(), 0);
    if (FAILED(result)) {
      utils::error::handle_error(
          function_name,
          std::string("IShellLink::SetIconLocation failed: ") + hr_hex(result));
      psl->Release();
      return false;
    }
  }

  if (not utils::file::file{lnk_path}.remove()) {
    utils::error::handle_error(function_name,
                               "failed to remove existing shortcut|path|" +
                                   utils::string::to_utf8(lnk_path));
    return false;
  }

  IPersistFile *ppf{nullptr};
  result = psl->QueryInterface(IID_PPV_ARGS(&ppf));
  if (FAILED(result)) {
    utils::error::handle_error(
        function_name,
        std::string("QueryInterface(IPersistFile) failed: ") + hr_hex(result));
    psl->Release();
    return false;
  }

  result = ppf->Save(lnk_path.c_str(), TRUE);
  ppf->SaveCompleted(lnk_path.c_str());

  ppf->Release();
  psl->Release();

  if (FAILED(result)) {
    utils::error::handle_error(function_name,
                               std::string("IPersistFile::Save failed: ") +
                                   hr_hex(result));
    return false;
  }

  return true;
}

auto remove_shortcut(std::wstring shortcut_name, const std::wstring &location)
    -> bool {
  if (not shortcut_name.ends_with(L".lnk")) {
    shortcut_name += L".lnk";
  }

  auto file = utils::path::combine(location, {shortcut_name});
  if (not utils::file::file{file}.exists()) {
    return true;
  }

  return utils::file::file{file}.remove();
}
} // namespace repertory::utils

#endif // defined(_WIN32)
