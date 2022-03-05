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
#ifndef INCLUDE_PLATFORM_UNIXPLATFORM_HPP_
#define INCLUDE_PLATFORM_UNIXPLATFORM_HPP_
#ifndef _WIN32

#include "common.hpp"
#include "types/repertory.hpp"

namespace repertory {
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
  static std::string get_state_directory();

  static std::string get_lock_data_file();

  std::string get_lock_file();

private:
  static int wait_for_lock(const int &fd, const std::uint8_t &retry_count = 30u);

public:
  bool get_mount_state(json &mount_state);

  lock_result grab_lock(const std::uint8_t &retry_count = 30u);

  bool set_mount_state(const bool &active, const std::string &mount_location, const int &pid);
};
} // namespace repertory

#endif // _WIN32
#endif // INCLUDE_PLATFORM_UNIXPLATFORM_HPP_
