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
#include "utils/error_utils.hpp"

#include "events/event_system.hpp"
#include "events/events.hpp"
#include "types/repertory.hpp"
#include "utils/error.hpp"

namespace {
struct repertory_exception_handler final
    : repertory::utils::error::i_exception_handler {
  void handle_error(std::string_view function_name,
                    std::string_view msg) const override {
    repertory::utils::error::raise_error(function_name, msg);
  }

  void handle_exception(std::string_view function_name) const override {
    repertory::utils::error::raise_error(function_name, "|exception|unknown");
  }

  void handle_exception(std::string_view function_name,
                        const std::exception &ex) const override {
    repertory::utils::error::raise_error(function_name, ex);
  }
};

std::unique_ptr<repertory_exception_handler> handler{([]() -> auto * {
  auto *ptr = new repertory_exception_handler{};
  repertory::utils::error::set_exception_handler(ptr);
  return ptr;
})()};
} // namespace

namespace repertory::utils::error {
void raise_error(std::string_view function, std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function), static_cast<std::string>(msg));
}

void raise_error(std::string_view function, const api_error &err,
                 std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|err|" + api_error_to_string(err));
}

void raise_error(std::string_view function, const std::exception &exception) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      "err|" + std::string(exception.what() == nullptr ? "unknown error"
                                                       : exception.what()));
}

void raise_error(std::string_view function, const std::exception &exception,
                 std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|err|" +
          (exception.what() == nullptr ? "unknown error" : exception.what()));
}

void raise_error(std::string_view function, const json &err,
                 std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|err|" + err.dump(2));
}

void raise_error(std::string_view function, std::int64_t err,
                 std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|err|" + std::to_string(err));
}

void raise_error(std::string_view function, const api_error &err,
                 std::string_view file_path, std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|sp|" +
          static_cast<std::string>(file_path) + "|err|" +
          api_error_to_string(err));
}

void raise_error(std::string_view function, std::int64_t err,
                 std::string_view file_path, std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|sp|" +
          static_cast<std::string>(file_path) + "|err|" + std::to_string(err));
}

void raise_error(std::string_view function, const std::exception &exception,
                 std::string_view file_path, std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|sp|" +
          static_cast<std::string>(file_path) + "|err|" +
          (exception.what() == nullptr ? "unknown error" : exception.what()));
}

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          const api_error &err, std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|ap|" +
          static_cast<std::string>(api_path) + "|err|" +
          api_error_to_string(err));
}

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          std::int64_t err, std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|ap|" +
          static_cast<std::string>(api_path) + "|err|" + std::to_string(err));
}

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          const std::exception &exception) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      "ap|" + static_cast<std::string>(api_path) + "|err|" +
          (exception.what() == nullptr ? "unknown error" : exception.what()));
}

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          const std::exception &exception,
                          std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|ap|" +
          static_cast<std::string>(api_path) + "|err|" +
          (exception.what() == nullptr ? "unknown error" : exception.what()));
}

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          std::string_view source_path, const api_error &err,
                          std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|ap|" +
          static_cast<std::string>(api_path) + "|sp|" +
          static_cast<std::string>(source_path) + "|err|" +
          api_error_to_string(err));
}

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          std::string_view source_path, std::int64_t err,
                          std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|ap|" +
          static_cast<std::string>(api_path) + "|sp|" +
          static_cast<std::string>(source_path) + "|err|" +
          std::to_string(err));
}

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          const json &err, std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|ap|" +
          static_cast<std::string>(api_path) + "|err|" + err.dump(2));
}

void raise_api_path_error(std::string_view function, std::string_view api_path,
                          std::string_view source_path,
                          const std::exception &exception,
                          std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|ap|" +
          static_cast<std::string>(api_path) + "|sp|" +
          static_cast<std::string>(source_path) + "|err|" +
          (exception.what() == nullptr ? "unknown error" : exception.what()));
}

void raise_url_error(std::string_view function, std::string_view url,
                     CURLcode err, std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|url|" + static_cast<std::string>(url) +
          "|err|" + curl_easy_strerror(err));
}

void raise_url_error(std::string_view function, std::string_view url,
                     std::string_view source_path,
                     const std::exception &exception) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      "url|" + static_cast<std::string>(url) + "|sp|" +
          static_cast<std::string>(source_path) + "|err|" +
          (exception.what() == nullptr ? "unknown error" : exception.what()));
}

void raise_url_error(std::string_view function, std::string_view url,
                     std::string_view source_path,
                     const std::exception &exception, std::string_view msg) {
  event_system::instance().raise<repertory_exception>(
      static_cast<std::string>(function),
      static_cast<std::string>(msg) + "|url|" + static_cast<std::string>(url) +
          "|sp|" + static_cast<std::string>(source_path) + "|err|" +
          (exception.what() == nullptr ? "unknown error" : exception.what()));
}
} // namespace repertory::utils::error
