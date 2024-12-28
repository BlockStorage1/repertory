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
#ifndef REPERTORY_INCLUDE_PLATFORM_WINPLATFORM_HPP_
#define REPERTORY_INCLUDE_PLATFORM_WINPLATFORM_HPP_
#if defined(_WIN32)

#include "app_config.hpp"
#include "types/repertory.hpp"

namespace repertory {
class i_provider;

class lock_data final {
public:
  explicit lock_data(const provider_type &pt, std::string unique_id /*= ""*/)
      : pt_(pt),
        unique_id_(std::move(unique_id)),
        mutex_id_("repertory_" + app_config::get_provider_name(pt) + "_" +
                  unique_id_),
        mutex_handle_(::CreateMutex(nullptr, FALSE, &mutex_id_[0u])) {}

  lock_data()
      : pt_(provider_type::sia),
        unique_id_(""),
        mutex_id_(""),
        mutex_handle_(INVALID_HANDLE_VALUE) {}

  ~lock_data() { release(); }

private:
  const provider_type pt_;
  const std::string unique_id_;
  const std::string mutex_id_;
  HANDLE mutex_handle_;
  DWORD mutex_state_ = WAIT_FAILED;

public:
  [[nodiscard]] auto get_mount_state(const provider_type &pt,
                                     json &mount_state) -> bool;

  [[nodiscard]] auto get_mount_state(json &mount_state) -> bool;

  [[nodiscard]] auto get_unique_id() const -> std::string { return unique_id_; }

  [[nodiscard]] auto grab_lock(std::uint8_t retry_count = 30) -> lock_result;

  void release();

  [[nodiscard]] auto set_mount_state(bool active,
                                     const std::string &mount_location,
                                     const std::int64_t &pid) -> bool;
};

[[nodiscard]] auto create_meta_attributes(
    std::uint64_t accessed_date, std::uint32_t attributes,
    std::uint64_t changed_date, std::uint64_t creation_date, bool directory,
    std::uint32_t gid, const std::string &key, std::uint32_t mode,
    std::uint64_t modified_date, std::uint32_t osx_backup,
    std::uint32_t osx_flags, std::uint64_t size, const std::string &source_path,
    std::uint32_t uid, std::uint64_t written_date) -> api_meta_map;

[[nodiscard]] auto provider_meta_handler(i_provider &provider, bool directory,
                                         const api_file &file) -> api_error;
} // namespace repertory

#endif // _WIN32
#endif // REPERTORY_INCLUDE_PLATFORM_WINPLATFORM_HPP_
