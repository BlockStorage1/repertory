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
#ifndef REPERTORY_INCLUDE_UTILS_ERROR_HPP_
#define REPERTORY_INCLUDE_UTILS_ERROR_HPP_

#include "utils/config.hpp"

namespace repertory::utils::error {
[[nodiscard]] auto
create_error_message(std::vector<std::string_view> items) -> std::string;

[[nodiscard]] auto
create_exception(std::string_view function_name,
                 std::vector<std::string_view> items) -> std::runtime_error;

struct i_exception_handler {
  virtual ~i_exception_handler() {}

  i_exception_handler(const i_exception_handler &) noexcept = delete;
  i_exception_handler(i_exception_handler &&) noexcept = delete;
  auto operator=(const i_exception_handler &) noexcept = delete;
  auto operator=(i_exception_handler &&) noexcept = delete;

  virtual void handle_error(std::string_view function_name,
                            std::string_view msg) const = 0;

  virtual void handle_exception(std::string_view function_name) const = 0;

  virtual void handle_exception(std::string_view function_name,
                                const std::exception &ex) const = 0;

protected:
  i_exception_handler() = default;
};

struct iostream_exception_handler final : i_exception_handler {
  void handle_error(std::string_view function_name,
                    std::string_view msg) const override {
    std::cerr << create_error_message({
                     function_name,
                     msg,
                 })
              << std::endl;
  }

  void handle_exception(std::string_view function_name) const override {
    std::cerr << create_error_message({
                     function_name,
                     "exception",
                     "unknown",
                 })
              << std::endl;
  }

  void handle_exception(std::string_view function_name,
                        const std::exception &ex) const override {
    std::cerr << create_error_message({
                     function_name,
                     "exception",
                     (ex.what() == nullptr ? "unknown" : ex.what()),
                 })
              << std::endl;
  }
};
inline const iostream_exception_handler default_exception_handler{};

extern std::atomic<const i_exception_handler *> exception_handler;

#if defined(PROJECT_ENABLE_TESTING)
[[nodiscard]] inline auto
get_exception_handler() -> const i_exception_handler * {
  return exception_handler;
}
#endif // defined(PROJECT_ENABLE_TESTING)

void handle_error(std::string_view function_name, std::string_view msg);

void handle_exception(std::string_view function_name);

void handle_exception(std::string_view function_name, const std::exception &ex);

void set_exception_handler(const i_exception_handler *handler);
} // namespace repertory::utils::error

#endif // REPERTORY_INCLUDE_UTILS_ERROR_HPP_
