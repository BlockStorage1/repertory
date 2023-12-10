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
#ifndef INCLUDE_PLATFORM_UNIXPLATFORM_HPP_
#define INCLUDE_PLATFORM_UNIXPLATFORM_HPP_
#ifndef _WIN32

#include "types/repertory.hpp"

namespace repertory {
class i_provider;

class lock_data final {
public:
  explicit lock_data(const provider_type &pt, std::string unique_id /*= ""*/);

  lock_data();

  ~lock_data();

private:
  const provider_type pt_;
  const std::string unique_id_;
  const std::string mutex_id_;
  int lock_fd_;
  int lock_status_ = EWOULDBLOCK;

private:
  [[nodiscard]] static auto get_state_directory() -> std::string;

  [[nodiscard]] static auto get_lock_data_file() -> std::string;

  [[nodiscard]] auto get_lock_file() -> std::string;

private:
  [[nodiscard]] static auto wait_for_lock(int fd,
                                          std::uint8_t retry_count = 30u)
      -> int;

public:
  [[nodiscard]] auto get_mount_state(json &mount_state) -> bool;

  [[nodiscard]] auto grab_lock(std::uint8_t retry_count = 30u) -> lock_result;

  [[nodiscard]] auto set_mount_state(bool active,
                                     const std::string &mount_location, int pid)
      -> bool;
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
#endif // INCLUDE_PLATFORM_UNIXPLATFORM_HPP_
