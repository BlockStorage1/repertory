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
#ifndef REPERTORY_INCLUDE_FILE_MANAGER_UPLOAD_HPP_
#define REPERTORY_INCLUDE_FILE_MANAGER_UPLOAD_HPP_

#include "types/repertory.hpp"

namespace repertory {
class i_provider;

class upload final {
public:
  upload(filesystem_item fsi, i_provider &provider);

  ~upload();

public:
  upload() = delete;
  upload(const upload &) noexcept = delete;
  upload(upload &&) noexcept = delete;
  auto operator=(upload &&) noexcept -> upload & = delete;
  auto operator=(const upload &) noexcept -> upload & = delete;

private:
  filesystem_item fsi_;
  i_provider &provider_;

private:
  bool cancelled_{false};
  api_error error_{api_error::success};
  stop_type stop_requested_{false};
  std::unique_ptr<std::thread> thread_;

private:
  void upload_thread();

public:
  void cancel();

  [[nodiscard]] auto get_api_error() const -> api_error { return error_; }

  [[nodiscard]] auto get_api_path() const -> std::string {
    return fsi_.api_path;
  }

  [[nodiscard]] auto get_source_path() const -> std::string {
    return fsi_.source_path;
  }

  [[nodiscard]] auto is_cancelled() const -> bool { return cancelled_; }

  void stop();
};
} // namespace repertory

#endif //  REPERTORY_INCLUDE_FILE_MANAGER_UPLOAD_HPP_
