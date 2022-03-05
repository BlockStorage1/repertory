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
#ifdef _WIN32
#include "platform/win32_platform.hpp"

namespace repertory {
bool lock_data::get_mount_state(const provider_type &pt, json &mount_state) {
  const auto ret = get_mount_state(mount_state);
  if (ret) {
    const auto mount_id = app_config::get_provider_display_name(pt_) + unique_id_;
    mount_state = mount_state[mount_id].empty()
                      ? json({{"Active", false}, {"Location", ""}, {"PID", -1}})
                      : mount_state[mount_id];
  }

  return ret;
}

bool lock_data::get_mount_state(json &mount_state) {
  HKEY key;
  auto ret = !::RegCreateKeyEx(
      HKEY_CURRENT_USER, ("SOFTWARE\\" + std::string(REPERTORY_DATA_NAME) + "\\Mounts").c_str(), 0,
      nullptr, 0, KEY_ALL_ACCESS, nullptr, &key, nullptr);
  if (ret) {
    DWORD i = 0u;
    DWORD data_size = 0u;
    std::string name;
    name.resize(32767u);
    auto name_size = static_cast<DWORD>(name.size());
    while (ret && (::RegEnumValue(key, i, &name[0], &name_size, nullptr, nullptr, nullptr,
                                  &data_size) == ERROR_SUCCESS)) {
      std::string data;
      data.resize(data_size);
      name_size++;
      if ((ret = !::RegEnumValue(key, i++, &name[0], &name_size, nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(&data[0]), &data_size))) {
        mount_state[name.c_str()] = json::parse(data);
        name_size = static_cast<DWORD>(name.size());
        data_size = 0u;
      }
    }
    ::RegCloseKey(key);
  }
  return ret;
}

lock_result lock_data::grab_lock(const std::uint8_t &retryCount) {
  auto ret = lock_result::success;
  if (mutex_handle_ == INVALID_HANDLE_VALUE) {
    ret = lock_result::failure;
  } else {
    for (auto i = 0; (i <= retryCount) &&
                     ((mutex_state_ = ::WaitForSingleObject(mutex_handle_, 100)) == WAIT_TIMEOUT);
         i++) {
    }

    switch (mutex_state_) {
    case WAIT_OBJECT_0: {
      ret = lock_result::success;
      auto should_reset = true;
      json mount_state;
      if (get_mount_state(pt_, mount_state)) {
        if (mount_state["Active"].get<bool>() && mount_state["Location"] == "elevating") {
          should_reset = false;
        }
      }

      if (should_reset) {
        set_mount_state(false, "", -1);
      }
    } break;

    case WAIT_TIMEOUT:
      ret = lock_result::locked;
      break;

    default:
      ret = lock_result::failure;
      break;
    }
  }

  return ret;
}

void lock_data::release() {
  if (mutex_handle_ != INVALID_HANDLE_VALUE) {
    if ((mutex_state_ == WAIT_OBJECT_0) || (mutex_state_ == WAIT_ABANDONED)) {
      ::ReleaseMutex(mutex_handle_);
    }
    ::CloseHandle(mutex_handle_);
    mutex_handle_ = INVALID_HANDLE_VALUE;
  }
}

bool lock_data::set_mount_state(const bool &active, const std::string &mountLocation,
                                const std::int64_t &pid) {
  auto ret = false;
  if (mutex_handle_ != INVALID_HANDLE_VALUE) {
    const auto mount_id = app_config::get_provider_display_name(pt_) + unique_id_;
    json mount_state;
    get_mount_state(mount_state);
    if ((mount_state.find(mount_id) == mount_state.end()) ||
        (mount_state[mount_id].find("Active") == mount_state[mount_id].end()) ||
        (mount_state[mount_id]["Active"].get<bool>() != active) ||
        (active && ((mount_state[mount_id].find("Location") == mount_state[mount_id].end()) ||
                    (mount_state[mount_id]["Location"].get<std::string>() != mountLocation)))) {
      HKEY key;
      if ((ret = !::RegCreateKeyEx(
               HKEY_CURRENT_USER,
               ("SOFTWARE\\" + std::string(REPERTORY_DATA_NAME) + "\\Mounts").c_str(), 0, nullptr,
               0, KEY_ALL_ACCESS, nullptr, &key, nullptr))) {
        const auto str = json({{"Active", active},
                               {"Location", active ? mountLocation : ""},
                               {"PID", active ? pid : -1}})
                             .dump(0);
        ret =
            !::RegSetValueEx(key, &mount_id[0], 0, REG_SZ, reinterpret_cast<const BYTE *>(&str[0]),
                             static_cast<DWORD>(str.size()));
        ::RegCloseKey(key);
      }
    }
  }

  return ret;
}
} // namespace repertory

#endif //_WIN32
