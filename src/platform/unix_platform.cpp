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
#ifndef _WIN32
#include "platform/unix_platform.hpp"
#include "app_config.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
lock_data::lock_data(const provider_type &pt, std::string unique_id /*= ""*/)
    : pt_(pt),
      unique_id_(std::move(unique_id)),
      mutex_id_("repertory2_" + app_config::get_provider_name(pt) + "_" + unique_id_) {
  lock_fd_ = open(get_lock_file().c_str(), O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
}

lock_data::lock_data() : pt_(provider_type::sia), unique_id_(""), mutex_id_(""), lock_fd_(-1) {}

lock_data::~lock_data() {
  if (lock_fd_ != -1) {
    if (lock_status_ == 0) {
      unlink(get_lock_file().c_str());
      flock(lock_fd_, LOCK_UN);
    }

    close(lock_fd_);
  }
}

std::string lock_data::get_lock_data_file() {
  const auto dir = get_state_directory();
  utils::file::create_full_directory_path(dir);
  return utils::path::combine(dir, {"mountstate_" + std::to_string(getuid()) + ".json"});
}

std::string lock_data::get_lock_file() {
  const auto dir = get_state_directory();
  utils::file::create_full_directory_path(dir);
  return utils::path::combine(dir, {mutex_id_ + "_" + std::to_string(getuid())});
}

bool lock_data::get_mount_state(json &mount_state) {
  auto ret = false;
  auto fd = open(get_lock_data_file().c_str(), O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
  if (fd != -1) {
    if (wait_for_lock(fd) == 0) {
      ret = utils::file::read_json_file(get_lock_data_file(), mount_state);
      flock(fd, LOCK_UN);
    }

    close(fd);
  }
  return ret;
}

std::string lock_data::get_state_directory() {
#ifdef __APPLE__
  return utils::path::resolve("~/Library/Application Support/" + std::string(REPERTORY_DATA_NAME) +
                              "/state");
#else
  return utils::path::resolve("~/.local/" + std::string(REPERTORY_DATA_NAME) + "/state");
#endif
}

lock_result lock_data::grab_lock(const std::uint8_t &retry_count) {
  if (lock_fd_ == -1) {
    return lock_result::failure;
  }

  lock_status_ = wait_for_lock(lock_fd_, retry_count);
  switch (lock_status_) {
  case 0:
    set_mount_state(false, "", -1);
    return lock_result::success;
  case EWOULDBLOCK:
    return lock_result::locked;
  default:
    return lock_result::failure;
  }
}

bool lock_data::set_mount_state(const bool &active, const std::string &mount_location,
                                const int &pid) {
  auto ret = false;
  auto fd = open(get_lock_data_file().c_str(), O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
  if (fd != -1) {
    if (wait_for_lock(fd) == 0) {
      const auto mount_id = app_config::get_provider_display_name(pt_) + unique_id_;
      json mount_state;
      utils::file::read_json_file(get_lock_data_file(), mount_state);
      if ((mount_state.find(mount_id) == mount_state.end()) ||
          (mount_state[mount_id].find("Active") == mount_state[mount_id].end()) ||
          (mount_state[mount_id]["Active"].get<bool>() != active) ||
          (active && ((mount_state[mount_id].find("Location") == mount_state[mount_id].end()) ||
                      (mount_state[mount_id]["Location"].get<std::string>() != mount_location)))) {
        const auto lines = utils::file::read_file_lines(get_lock_data_file());
        const auto txt =
            std::accumulate(lines.begin(), lines.end(), std::string(),
                            [](std::string s, const std::string &s2) { return std::move(s) + s2; });
        auto json = json::parse(txt.empty() ? "{}" : txt);
        json[mount_id] = {{"Active", active},
                          {"Location", active ? mount_location : ""},
                          {"PID", active ? pid : -1}};
        ret = utils::file::write_json_file(get_lock_data_file(), json);
      }
      flock(fd, LOCK_UN);
    }

    close(fd);
  }
  return ret;
}

int lock_data::wait_for_lock(const int &fd, const std::uint8_t &retry_count) {
  auto lock_status = EWOULDBLOCK;
  std::int16_t remain = retry_count * 100u;
  while ((remain > 0) && (lock_status == EWOULDBLOCK)) {
    if ((lock_status = flock(fd, LOCK_EX | LOCK_NB)) == -1) {
      if ((lock_status = errno) == EWOULDBLOCK) {
        const auto sleep_ms =
            utils::random_between(std::int16_t(1), std::min(remain, std::int16_t(100)));
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        remain -= sleep_ms;
      }
    }
  }

  return lock_status;
}
} // namespace repertory

#endif //_WIN32
