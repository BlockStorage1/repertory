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

#include "platform/platform.hpp"

#include "events/event_system.hpp"
#include "events/types/filesystem_item_added.hpp"
#include "providers/i_provider.hpp"
#include "utils/config.hpp"
#include "utils/error_utils.hpp"
#include "utils/string.hpp"

namespace repertory {
lock_data::lock_data(provider_type prov, std::string unique_id)
    : mutex_id_(create_lock_id(prov, unique_id)),
      mutex_handle_(::CreateMutex(nullptr, FALSE,
                                  create_lock_id(prov, unique_id).c_str())) {}

lock_data::~lock_data() { release(); }

auto lock_data::get_current_mount_state(json &mount_state) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  HKEY key{};
  if (::RegOpenKeyEx(HKEY_CURRENT_USER,
                     fmt::format(R"(SOFTWARE\{}\Mounts\{})",
                                 REPERTORY_DATA_NAME, mutex_id_)
                         .c_str(),
                     0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS) {
    return true;
  }

  std::string data;
  DWORD data_size{};

  DWORD type{REG_SZ};
  ::RegGetValueA(key, nullptr, nullptr, RRF_RT_REG_SZ, &type, nullptr,
                 &data_size);

  data.resize(data_size);
  auto res = ::RegGetValueA(key, nullptr, nullptr, RRF_RT_REG_SZ, &type,
                            data.data(), &data_size);
  auto ret = res == ERROR_SUCCESS || res == ERROR_FILE_NOT_FOUND;
  if (ret && data_size != 0U) {
    try {
      mount_state = json::parse(data);
    } catch (const std::exception &e) {
      utils::error::raise_error(function_name, e, "failed to read mount state");
      ret = false;
    }
  }

  ::RegCloseKey(key);
  return ret;
}

auto lock_data::get_mount_state(json &mount_state) -> bool {
  if (not get_current_mount_state(mount_state)) {
    return false;
  }

  mount_state = mount_state.empty() ? json({
                                          {"Active", false},
                                          {"Location", ""},
                                          {"PID", -1},
                                      })
                                    : mount_state;

  return true;
}

auto lock_data::grab_lock(std::uint8_t retry_count) -> lock_result {
  static constexpr std::uint32_t max_sleep{100U};

  if (mutex_handle_ == INVALID_HANDLE_VALUE) {
    return lock_result::failure;
  }

  for (std::uint8_t idx = 0U;
       (idx <= retry_count) &&
       ((mutex_state_ = ::WaitForSingleObject(mutex_handle_, max_sleep)) ==
        WAIT_TIMEOUT);
       ++idx) {
  }

  switch (mutex_state_) {
  case WAIT_OBJECT_0:
    return lock_result::success;

  case WAIT_TIMEOUT:
    return lock_result::locked;

  default:
    return lock_result::failure;
  }
}

void lock_data::release() {
  if (mutex_handle_ == INVALID_HANDLE_VALUE) {
    return;
  }

  if ((mutex_state_ == WAIT_OBJECT_0) || (mutex_state_ == WAIT_ABANDONED)) {
    if (mutex_state_ == WAIT_OBJECT_0) {
      [[maybe_unused]] auto success{set_mount_state(false, "", -1)};
    }

    ::ReleaseMutex(mutex_handle_);
  }

  ::CloseHandle(mutex_handle_);
  mutex_handle_ = INVALID_HANDLE_VALUE;
}

auto lock_data::set_mount_state(bool active, std::string_view mount_location,
                                std::int64_t pid) -> bool {
  if (mutex_handle_ == INVALID_HANDLE_VALUE) {
    return false;
  }

  json mount_state;
  [[maybe_unused]] auto success{get_mount_state(mount_state)};
  if (not((mount_state.find("Active") == mount_state.end()) ||
          (mount_state["Active"].get<bool>() != active) ||
          (active &&
           ((mount_state.find("Location") == mount_state.end()) ||
            (mount_state["Location"].get<std::string>() != mount_location))))) {
    return true;
  }

  HKEY key{};
  if (::RegCreateKeyExA(HKEY_CURRENT_USER,
                        fmt::format(R"(SOFTWARE\{}\Mounts\{})",
                                    REPERTORY_DATA_NAME, mutex_id_)
                            .c_str(),
                        0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &key,
                        nullptr) != ERROR_SUCCESS) {
    return false;
  }

  auto ret{false};
  if (mount_location.empty() && not active) {
    ::RegCloseKey(key);

    if (::RegCreateKeyExA(
            HKEY_CURRENT_USER,
            fmt::format(R"(SOFTWARE\{}\Mounts)", REPERTORY_DATA_NAME).c_str(),
            0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &key,
            nullptr) != ERROR_SUCCESS) {
      return false;
    }

    ret = (::RegDeleteKeyA(key, mutex_id_.c_str()) == ERROR_SUCCESS);
  } else {
    auto data{
        json({
                 {"Active", active},
                 {"Location", active ? mount_location : ""},
                 {"PID", active ? pid : -1},
             })
            .dump(),
    };
    ret = (::RegSetValueEx(key, nullptr, 0, REG_SZ,
                           reinterpret_cast<const BYTE *>(data.c_str()),
                           static_cast<DWORD>(data.size())) == ERROR_SUCCESS);
  }

  ::RegCloseKey(key);
  return ret;
}

auto create_meta_attributes(
    std::uint64_t accessed_date, std::uint32_t attributes,
    std::uint64_t changed_date, std::uint64_t creation_date, bool directory,
    std::uint32_t gid, const std::string &key, std::uint32_t mode,
    std::uint64_t modified_date, std::uint32_t osx_backup,
    std::uint32_t osx_flags, std::uint64_t size, const std::string &source_path,
    std::uint32_t uid, std::uint64_t written_date) -> api_meta_map {
  return {
      {META_ACCESSED, std::to_string(accessed_date)},
      {META_ATTRIBUTES, std::to_string(attributes)},
      {META_BACKUP, std::to_string(osx_backup)},
      {META_CHANGED, std::to_string(changed_date)},
      {META_CREATION, std::to_string(creation_date)},
      {META_DIRECTORY, utils::string::from_bool(directory)},
      {META_GID, std::to_string(gid)},
      {META_KEY, key},
      {META_MODE, std::to_string(mode)},
      {META_MODIFIED, std::to_string(modified_date)},
      {META_OSXFLAGS, std::to_string(osx_flags)},
      {META_PINNED, "0"},
      {META_SIZE, std::to_string(size)},
      {META_SOURCE, source_path},
      {META_UID, std::to_string(uid)},
      {META_WRITTEN, std::to_string(written_date)},
  };
}

auto provider_meta_handler(i_provider &provider, bool directory,
                           const api_file &file) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  const auto meta = create_meta_attributes(
      file.accessed_date,
      directory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_ARCHIVE,
      file.changed_date, file.creation_date, directory, 0u, file.key,
      directory ? S_IFDIR : S_IFREG, file.modified_date, 0u, 0u, file.file_size,
      file.source_path, 0u, file.modified_date);
  auto res = provider.set_item_meta(file.api_path, meta);
  if (res == api_error::success) {
    event_system::instance().raise<filesystem_item_added>(
        file.api_parent, file.api_path, directory, function_name);
  }

  return res;
}
} // namespace repertory

#endif // defined(_WIN32)
