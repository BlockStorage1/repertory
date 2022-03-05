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
#ifndef INCLUDE_UTILS_GLOBAL_DATA_HPP_
#define INCLUDE_UTILS_GLOBAL_DATA_HPP_

#include "common.hpp"

namespace repertory {
class global_data final {
public:
  global_data(const global_data &) = delete;

  global_data(global_data &&) = delete;

  global_data &operator=(const global_data &) = delete;

  global_data &operator=(global_data &&) = delete;

private:
  global_data() : used_cache_space_(0u), used_drive_space_(0u) {}

  ~global_data() = default;

private:
  static global_data instance_;

public:
  static global_data &instance() { return instance_; }

private:
  std::atomic<std::uint64_t> used_cache_space_;
  std::atomic<std::uint64_t> used_drive_space_;

public:
  void decrement_used_drive_space(const std::uint64_t &val) { used_drive_space_ -= val; }

  std::uint64_t get_used_cache_space() const { return used_cache_space_; }

  std::uint64_t get_used_drive_space() const { return used_drive_space_; }

  void increment_used_drive_space(const std::uint64_t &val) { used_drive_space_ += val; }

  void initialize_used_cache_space(const std::uint64_t &val) { used_cache_space_ = val; }

  void initialize_used_drive_space(const std::uint64_t &val) { used_drive_space_ = val; }

  void update_used_space(const std::uint64_t &file_size, const std::uint64_t &new_file_size,
                         const bool &cache_only);
};
} // namespace repertory

#endif // INCLUDE_UTILS_GLOBAL_DATA_HPP_
