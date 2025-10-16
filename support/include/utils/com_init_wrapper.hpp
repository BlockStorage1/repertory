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
#ifndef REPERTORY_INCLUDE_UTILS_COM_INIT_WRAPPER_HPP_
#define REPERTORY_INCLUDE_UTILS_COM_INIT_WRAPPER_HPP_
#if defined(_WIN32)

#include "utils/config.hpp"

namespace repertory::utils {
struct com_init_wrapper final {
  com_init_wrapper()
      : initialized_(
            SUCCEEDED(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {}

  com_init_wrapper(const com_init_wrapper &) = delete;
  com_init_wrapper(com_init_wrapper &&) = delete;

  ~com_init_wrapper() {
    if (initialized_) {
      ::CoUninitialize();
    }
  }

  auto operator=(const com_init_wrapper &) -> com_init_wrapper & = delete;
  auto operator=(com_init_wrapper &&) -> com_init_wrapper & = delete;

  [[nodiscard]] auto is_initialized() const -> bool { return initialized_; }

private:
  BOOL initialized_{};
};
} // namespace repertory::utils

#endif // defined(_WIN32)
#endif // REPERTORY_INCLUDE_UTILS_COM_INIT_WRAPPER_HPP_
