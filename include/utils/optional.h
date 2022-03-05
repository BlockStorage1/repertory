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
#ifndef INCLUDE_UTILS_OPTIONAL_HPP_
#define INCLUDE_UTILS_OPTIONAL_HPP_
#if IS_DEBIAN9_DISTRO || !HAS_STD_OPTIONAL

#include <memory>
namespace std {
class bad_optional_access : std::runtime_error {
public:
  bad_optional_access() : std::runtime_error("bad optional access") {}
};

template <typename t> class optional {
public:
  optional() = default;

  explicit optional(const t &v) : val_(new t(v)) {}

  explicit optional(t &&v) noexcept : val_(new t(std::move(v))) {}

  optional(const optional &o) noexcept : val_(o.has_value() ? new t(o.val_) : nullptr) {}

  optional(optional &&o) noexcept : val_(o.has_value() ? std::move(o.val_) : nullptr) {}

private:
  std::unique_ptr<t> val_;

public:
  bool has_value() const { return (val_ != nullptr); }

  t &value() {
    if (not has_value()) {
      throw bad_optional_access();
    }
    return *val_;
  }

  const t &value() const {
    if (not has_value()) {
      throw bad_optional_access();
    }
    return *val_;
  }

  optional &operator=(const t &v) {
    if (&v != val_.get()) {
      val_.reset(new t(v));
    }
    return *this;
  }

  optional &operator=(const optional &v) {
    if (&v != this) {
      val_.reset(new t(v.has_value() ? v.value() : nullptr));
    }
    return *this;
  }

  optional &operator=(t &&v) noexcept {
    if (&v != val_.get()) {
      val_.reset(new t(std::move(v)));
    }
    return *this;
  }

  optional &operator=(optional &&v) noexcept {
    if (&v != this) {
      val_ = std::move(v);
    }
    return *this;
  }

  bool operator==(const optional &v) {
    return (&v == this) || ((has_value() && v.has_value()) && (*val_ == *v.val_));
  }

  bool operator==(const t &v) { return has_value() && (v == *val_); }

  bool operator!=(const optional &v) {
    return (has_value() != v.has_value) || ((has_value() && v.has_value()) && (*val_ != *v.val_));
  }

  bool operator!=(const t &v) { return not has_value() || (v != *val_); }
};
} // namespace std

#endif // IS_DEBIAN9_DISTRO || !HAS_STD_OPTIONAL
#endif // INCLUDE_UTILS_OPTIONAL_HPP_
