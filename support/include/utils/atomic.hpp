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
#ifndef REPERTORY_INCLUDE_UTILS_ATOMIC_HPP_
#define REPERTORY_INCLUDE_UTILS_ATOMIC_HPP_

#include "utils/config.hpp"

namespace repertory::utils {
template <typename data_t> class atomic final {
public:
  atomic() : mtx_(std::make_shared<std::mutex>()) {}

  atomic(const atomic &at_data)
      : data_(at_data.load()), mtx_(std::make_shared<std::mutex>()) {}

  atomic(data_t data)
      : data_(std::move(data)), mtx_(std::make_shared<std::mutex>()) {}

  atomic(atomic &&) = default;

  ~atomic() = default;

private:
  data_t data_;
  std::shared_ptr<std::mutex> mtx_;

public:
  [[nodiscard]] auto load() const -> data_t {
    mutex_lock lock(*mtx_);
    return data_;
  }

  auto store(data_t data) -> data_t {
    mutex_lock lock(*mtx_);
    data_ = std::move(data);
    return data_;
  }

  auto operator=(const atomic &at_data) -> atomic & {
    if (&at_data == this) {
      return *this;
    }

    store(at_data.load());
    return *this;
  }

  auto operator=(atomic &&) -> atomic & = default;

  auto operator=(data_t data) -> atomic & {
    if (&data == &data_) {
      return *this;
    }

    store(std::move(data));
    return *this;
  }

  [[nodiscard]] auto operator==(const atomic &at_data) const -> bool {
    if (&at_data == this) {
      return true;
    }

    mutex_lock lock(*mtx_);
    return at_data.load() == data_;
  }

  [[nodiscard]] auto operator==(const data_t &data) const -> bool {
    if (&data == &data_) {
      return true;
    }

    mutex_lock lock(*mtx_);
    return data == data_;
  }

  [[nodiscard]] auto operator!=(const atomic &at_data) const -> bool {
    if (&at_data == this) {
      return false;
    }

    mutex_lock lock(*mtx_);
    return at_data.load() != data_;
  }

  [[nodiscard]] auto operator!=(const data_t &data) const -> bool {
    if (&data == &data_) {
      return false;
    }

    mutex_lock lock(*mtx_);
    return data != data_;
  }

  [[nodiscard]] operator data_t() const { return load(); }
};
} // namespace repertory::utils

#endif // REPERTORY_INCLUDE_UTILS_ATOMIC_HPP_
