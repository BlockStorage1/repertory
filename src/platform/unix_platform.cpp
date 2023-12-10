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
#ifndef _WIN32

#include "platform/unix_platform.hpp"

#include "app_config.hpp"
#include "providers/i_provider.hpp"
#include "types/startup_exception.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/unix/unix_utils.hpp"

namespace repertory {
lock_data::lock_data(const provider_type &pt, std::string unique_id /*= ""*/)
    : pt_(pt),
      unique_id_(std::move(unique_id)),
      mutex_id_("repertory2_" + app_config::get_provider_name(pt) + "_" +
                unique_id_) {
  lock_fd_ = open(get_lock_file().c_str(), O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
}

lock_data::lock_data()
    : pt_(provider_type::sia), unique_id_(""), mutex_id_(""), lock_fd_(-1) {}

lock_data::~lock_data() {
  if (lock_fd_ != -1) {
    if (lock_status_ == 0) {
      unlink(get_lock_file().c_str());
      flock(lock_fd_, LOCK_UN);
    }

    close(lock_fd_);
  }
}

auto lock_data::get_lock_data_file() -> std::string {
  const auto dir = get_state_directory();
  if (not utils::file::create_full_directory_path(dir)) {
    throw startup_exception("failed to create directory|sp|" + dir + "|err|" +
                            std::to_string(utils::get_last_error_code()));
  }
  return utils::path::combine(
      dir, {"mountstate_" + std::to_string(getuid()) + ".json"});
}

auto lock_data::get_lock_file() -> std::string {
  const auto dir = get_state_directory();
  if (not utils::file::create_full_directory_path(dir)) {
    throw startup_exception("failed to create directory|sp|" + dir + "|err|" +
                            std::to_string(utils::get_last_error_code()));
  }

  return utils::path::combine(dir,
                              {mutex_id_ + "_" + std::to_string(getuid())});
}

auto lock_data::get_mount_state(json &mount_state) -> bool {
  auto ret = false;
  auto fd =
      open(get_lock_data_file().c_str(), O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
  if (fd != -1) {
    if (wait_for_lock(fd) == 0) {
      ret = utils::file::read_json_file(get_lock_data_file(), mount_state);
      flock(fd, LOCK_UN);
    }

    close(fd);
  }
  return ret;
}

auto lock_data::get_state_directory() -> std::string {
#ifdef __APPLE__
  return utils::path::resolve("~/Library/Application Support/" +
                              std::string(REPERTORY_DATA_NAME) + "/state");
#else
  return utils::path::resolve("~/.local/" + std::string(REPERTORY_DATA_NAME) +
                              "/state");
#endif
}

auto lock_data::grab_lock(std::uint8_t retry_count) -> lock_result {
  if (lock_fd_ == -1) {
    return lock_result::failure;
  }

  lock_status_ = wait_for_lock(lock_fd_, retry_count);
  switch (lock_status_) {
  case 0:
    if (not set_mount_state(false, "", -1)) {
      utils::error::raise_error(__FUNCTION__, "failed to set mount state");
    }
    return lock_result::success;
  case EWOULDBLOCK:
    return lock_result::locked;
  default:
    return lock_result::failure;
  }
}

auto lock_data::set_mount_state(bool active, const std::string &mount_location,
                                int pid) -> bool {
  auto ret = false;
  auto fd =
      open(get_lock_data_file().c_str(), O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
  if (fd != -1) {
    if (wait_for_lock(fd) == 0) {
      const auto mount_id =
          app_config::get_provider_display_name(pt_) + unique_id_;
      json mount_state;
      if (not utils::file::read_json_file(get_lock_data_file(), mount_state)) {
        utils::error::raise_error(__FUNCTION__,
                                  "failed to read mount state file|sp|" +
                                      get_lock_file());
      }
      if ((mount_state.find(mount_id) == mount_state.end()) ||
          (mount_state[mount_id].find("Active") ==
           mount_state[mount_id].end()) ||
          (mount_state[mount_id]["Active"].get<bool>() != active) ||
          (active && ((mount_state[mount_id].find("Location") ==
                       mount_state[mount_id].end()) ||
                      (mount_state[mount_id]["Location"].get<std::string>() !=
                       mount_location)))) {
        const auto lines = utils::file::read_file_lines(get_lock_data_file());
        const auto txt =
            std::accumulate(lines.begin(), lines.end(), std::string(),
                            [](std::string s, const std::string &s2) {
                              return std::move(s) + s2;
                            });
        auto json = json::parse(txt.empty() ? "{}" : txt);
        json[mount_id] = {{"Active", active},
                          {"Location", active ? mount_location : ""},
                          {"PID", active ? pid : -1}};
        ret = utils::file::write_json_file(get_lock_data_file(), json);
      } else {
        ret = true;
      }

      flock(fd, LOCK_UN);
    }

    close(fd);
  }
  return ret;
}

auto lock_data::wait_for_lock(int fd, std::uint8_t retry_count) -> int {
  static constexpr const std::uint32_t max_sleep = 100U;

  auto lock_status = EWOULDBLOCK;
  auto remain = static_cast<std::uint32_t>(retry_count * max_sleep);
  while ((remain > 0) && (lock_status == EWOULDBLOCK)) {
    lock_status = flock(fd, LOCK_EX | LOCK_NB);
    if (lock_status == -1) {
      lock_status = errno;
      if (lock_status == EWOULDBLOCK) {
        auto sleep_ms = utils::random_between(1U, std::min(remain, max_sleep));
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
  const auto meta = create_meta_attributes(
      file.accessed_date,
      directory ? FILE_ATTRIBUTE_DIRECTORY
                : FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE,
      file.changed_date, file.creation_date, directory, getgid(), file.key,
      directory ? S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR
                : S_IFREG | S_IRUSR | S_IWUSR,
      file.modified_date, 0u, 0u, file.file_size, file.source_path, getuid(),
      file.modified_date);
  auto res = provider.set_item_meta(file.api_path, meta);
  if (res == api_error::success) {
    event_system::instance().raise<filesystem_item_added>(
        file.api_path, file.api_parent, directory);
  }

  return res;
}
} // namespace repertory

#endif //_WIN32
