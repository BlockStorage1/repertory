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
#if !defined(_WIN32)

#include "platform/platform.hpp"

#include "events/event_system.hpp"
#include "events/types/filesystem_item_added.hpp"
#include "providers/i_provider.hpp"
#include "types/startup_exception.hpp"
#include "utils/common.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"
#include "utils/unix.hpp"

namespace repertory {
lock_data::lock_data(provider_type prov, std::string_view unique_id)
    : mutex_id_(create_lock_id(prov, unique_id)) {
  handle_ = open(get_lock_file().c_str(), O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
}

lock_data::~lock_data() { release(); }

auto lock_data::get_lock_data_file() const -> std::string {
  auto dir = get_state_directory();
  if (not utils::file::directory(dir).create_directory()) {
    throw startup_exception("failed to create directory|sp|" + dir + "|err|" +
                            std::to_string(utils::get_last_error_code()));
  }

  return utils::path::combine(
      dir, {
               fmt::format("{}_{}.json", mutex_id_, getuid()),
           });
}

auto lock_data::get_lock_file() const -> std::string {
  auto dir = get_state_directory();
  if (not utils::file::directory(dir).create_directory()) {
    throw startup_exception("failed to create directory|sp|" + dir + "|err|" +
                            std::to_string(utils::get_last_error_code()));
  }

  return utils::path::combine(
      dir, {
               fmt::format("{}_{}.lock", mutex_id_, getuid()),
           });
}

auto lock_data::get_mount_state(json &mount_state) -> bool {
  auto handle = open(get_lock_data_file().c_str(), O_RDWR, S_IWUSR | S_IRUSR);
  if (handle == -1) {
    mount_state = {
        {"Active", false},
        {"Location", ""},
        {"PID", -1},
    };

    return true;
  }

  auto ret{false};
  if (wait_for_lock(handle) == 0) {
    ret = utils::file::read_json_file(get_lock_data_file(), mount_state);
    if (ret && mount_state.empty()) {
      mount_state = {
          {"Active", false},
          {"Location", ""},
          {"PID", -1},
      };
    }
    flock(handle, LOCK_UN);
  }

  close(handle);
  return ret;
}

auto lock_data::get_state_directory() -> std::string {
#if defined(__APPLE__)
  return utils::path::absolute("~/Library/Application Support/" +
                               std::string{REPERTORY_DATA_NAME} + "/state");
#else  // !defined(__APPLE__)
  return utils::path::absolute("~/.local/" + std::string{REPERTORY_DATA_NAME} +
                               "/state");
#endif // defined(__APPLE__)
}

auto lock_data::grab_lock(std::uint8_t retry_count) -> lock_result {
  if (handle_ == -1) {
    return lock_result::failure;
  }

  lock_status_ = wait_for_lock(handle_, retry_count);
  switch (lock_status_) {
  case 0:
    return lock_result::success;
  case EWOULDBLOCK:
    return lock_result::locked;
  default:
    return lock_result::failure;
  }
}

void lock_data::release() {
  if (handle_ == -1) {
    return;
  }

  if (lock_status_ == 0) {
    [[maybe_unused]] auto success{utils::file::file{get_lock_file()}.remove()};
    flock(handle_, LOCK_UN);
  }

  close(handle_);
  handle_ = -1;
}

auto lock_data::set_mount_state(bool active, std::string_view mount_location,
                                int pid) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  auto handle =
      open(get_lock_data_file().c_str(), O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
  if (handle == -1) {
    return false;
  }

  auto ret{false};
  if (wait_for_lock(handle) == 0) {
    json mount_state;
    if (not utils::file::read_json_file(get_lock_data_file(), mount_state)) {
      utils::error::raise_error(function_name,
                                "failed to read mount state file|sp|" +
                                    get_lock_file());
    }
    if ((mount_state.find("Active") == mount_state.end()) ||
        (mount_state["Active"].get<bool>() != active) ||
        (active &&
         ((mount_state.find("Location") == mount_state.end()) ||
          (mount_state["Location"].get<std::string>() != mount_location)))) {
      if (mount_location.empty() && not active) {
        ret = utils::file::file{get_lock_data_file()}.remove();
      } else {
        ret = utils::file::write_json_file(
            get_lock_data_file(),
            {
                {"Active", active},
                {"Location", active ? mount_location : ""},
                {"PID", active ? pid : -1},
            });
      }
    } else {
      ret = true;
    }

    flock(handle, LOCK_UN);
  }

  close(handle);
  return ret;
}

auto lock_data::wait_for_lock(int handle, std::uint8_t retry_count) -> int {
  static constexpr const std::uint32_t max_sleep{100U};

  auto lock_status{EWOULDBLOCK};
  auto remain{static_cast<std::uint32_t>(retry_count * max_sleep)};
  while ((remain > 0) && (lock_status == EWOULDBLOCK)) {
    lock_status = flock(handle, LOCK_EX | LOCK_NB);
    if (lock_status == -1) {
      lock_status = errno;
      if (lock_status == EWOULDBLOCK) {
        auto sleep_ms = std::min(remain, max_sleep);
        if (sleep_ms > 1U) {
          sleep_ms = utils::generate_random_between(1U, sleep_ms);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        remain -= sleep_ms;
      }
    }
  }

  return lock_status;
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
      {META_SOURCE, source_path},
      {META_SIZE, std::to_string(size)},
      {META_UID, std::to_string(uid)},
      {META_WRITTEN, std::to_string(written_date)},
  };
}

auto provider_meta_handler(i_provider &provider, bool directory,
                           const api_file &file) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  auto meta = create_meta_attributes(
      file.accessed_date,
      directory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_ARCHIVE,
      file.changed_date, file.creation_date, directory, getgid(), file.key,
      directory ? S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR
                : S_IFREG | S_IRUSR | S_IWUSR,
      file.modified_date, 0U, 0U, file.file_size, file.source_path, getuid(),
      file.modified_date);
  auto res = provider.set_item_meta(file.api_path, meta);
  if (res == api_error::success) {
    event_system::instance().raise<filesystem_item_added>(
        file.api_parent, file.api_path, directory, function_name);
  }

  return res;
}
} // namespace repertory

#endif //_WIN32
