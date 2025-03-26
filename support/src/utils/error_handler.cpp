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
#include "utils/error.hpp"

#include "utils/config.hpp"
#include "utils/string.hpp"

namespace repertory::utils::error {
std::atomic<const i_exception_handler *> exception_handler{
    &default_exception_handler,
};

#if defined(PROJECT_ENABLE_V2_ERRORS)
void iostream_exception_handler::handle_debug(std::string_view function_name,
                                              std::string_view msg) const {
  std::cout << create_error_message({
                   "debug",
                   function_name,
                   msg,
               })
            << std::endl;
}
#endif // defined(PROJECT_ENABLE_V2_ERRORS)

void iostream_exception_handler::handle_error(std::string_view function_name,
                                              std::string_view msg) const {
  std::cerr << create_error_message({
                   "error",
                   function_name,
                   msg,
               })
            << std::endl;
}

void iostream_exception_handler::handle_exception(
    std::string_view function_name) const {
  std::cerr << create_error_message({
                   "error",
                   function_name,
                   "exception",
                   "unknown",
               })
            << std::endl;
}

void iostream_exception_handler::handle_exception(
    std::string_view function_name, const std::exception &ex) const {
  std::cerr << create_error_message({
                   "error",
                   function_name,
                   "exception",
                   (ex.what() == nullptr ? "unknown" : ex.what()),
               })
            << std::endl;
}

#if defined(PROJECT_ENABLE_V2_ERRORS)
void iostream_exception_handler::handle_info(std::string_view function_name,
                                             std::string_view msg) const {
  std::cout << create_error_message({
                   "info",
                   function_name,
                   msg,
               })
            << std::endl;
}

void iostream_exception_handler::handle_trace(std::string_view function_name,
                                              std::string_view msg) const {
  std::cout << create_error_message({
                   "trace",
                   function_name,
                   msg,
               })
            << std::endl;
}

void iostream_exception_handler::handle_warn(std::string_view function_name,
                                             std::string_view msg) const {
  std::cout << create_error_message({
                   "warn",
                   function_name,
                   msg,
               })
            << std::endl;
}
#endif // defined(PROJECT_ENABLE_V2_ERRORS)

#if defined(PROJECT_ENABLE_SPDLOG) && defined(PROJECT_ENABLE_V2_ERRORS)
void spdlog_exception_handler::handle_debug(std::string_view function_name,
                                            std::string_view msg) const {
  auto console = spdlog::get("console");
  if (console) {
    console->debug(utils::error::create_error_message(function_name, {msg}));
  } else {
    fallback.handle_debug(function_name, msg);
  }

  auto file = spdlog::get("file");
  if (not file) {
    return;
  }

  file->debug(utils::error::create_error_message(function_name, {msg}));
}

void spdlog_exception_handler::handle_error(std::string_view function_name,
                                            std::string_view msg) const {
  auto console = spdlog::get("console");
  if (console) {
    console->error(utils::error::create_error_message(function_name, {msg}));
  } else {
    fallback.handle_error(function_name, msg);
  }

  auto file = spdlog::get("file");
  if (not file) {
    return;
  }

  file->error(utils::error::create_error_message(function_name, {msg}));
}

void spdlog_exception_handler::handle_exception(
    std::string_view function_name) const {
  auto console = spdlog::get("console");
  if (console) {
    console->error(utils::error::create_error_message(function_name,
                                                      {
                                                          "exception",
                                                          "unknown exception",
                                                      }));
  } else {
    fallback.handle_exception(function_name);
  }

  auto file = spdlog::get("file");
  if (not file) {
    return;
  }

  file->error(
      utils::error::create_error_message(function_name, {
                                                            "exception",
                                                            "unknown exception",
                                                        }));
}

void spdlog_exception_handler::handle_exception(
    std::string_view function_name, const std::exception &ex) const {
  auto console = spdlog::get("console");
  if (console) {
    console->error(utils::error::create_error_message(
        function_name, {
                           "exception",
                           (ex.what() == nullptr ? "unknown" : ex.what()),
                       }));
  } else {
    fallback.handle_exception(function_name, ex);
  }

  auto file = spdlog::get("file");
  if (not file) {
    return;
  }

  file->error(utils::error::create_error_message(
      function_name, {
                         "exception",
                         (ex.what() == nullptr ? "unknown" : ex.what()),
                     }));
}

void spdlog_exception_handler::handle_info(std::string_view function_name,
                                           std::string_view msg) const {
  auto console = spdlog::get("console");
  if (console) {
    console->info(utils::error::create_error_message(function_name, {msg}));
  } else {
    fallback.handle_info(function_name, msg);
  }

  auto file = spdlog::get("file");
  if (not file) {
    return;
  }

  file->info(utils::error::create_error_message(function_name, {msg}));
}

void spdlog_exception_handler::handle_trace(std::string_view function_name,
                                            std::string_view msg) const {
  auto console = spdlog::get("console");
  if (console) {
    console->trace(utils::error::create_error_message(function_name, {msg}));
  } else {
    fallback.handle_trace(function_name, msg);
  }

  auto file = spdlog::get("file");
  if (not file) {
    return;
  }

  file->trace(utils::error::create_error_message(function_name, {msg}));
}

void spdlog_exception_handler::handle_warn(std::string_view function_name,
                                           std::string_view msg) const {
  auto console = spdlog::get("console");
  if (console) {
    console->warn(utils::error::create_error_message(function_name, {msg}));
  } else {
    fallback.handle_warn(function_name, msg);
  }

  auto file = spdlog::get("file");
  if (not file) {
    return;
  }

  file->warn(utils::error::create_error_message(function_name, {msg}));
}
#endif // defined(PROJECT_ENABLE_SPDLOG) && defined(PROJECT_ENABLE_V2_ERRORS)

#if defined(PROJECT_ENABLE_V2_ERRORS)
void handle_debug(std::string_view function_name, std::string_view msg) {
  const i_exception_handler *handler{exception_handler};
  if (handler != nullptr) {
    handler->handle_debug(function_name, msg);
    return;
  }

  default_exception_handler.handle_debug(function_name, msg);
}
#endif // defined(PROJECT_ENABLE_V2_ERRORS)

void handle_error(std::string_view function_name, std::string_view msg) {
  const i_exception_handler *handler{exception_handler};
  if (handler != nullptr) {
    handler->handle_error(function_name, msg);
    return;
  }

  default_exception_handler.handle_error(function_name, msg);
}

void handle_exception(std::string_view function_name) {
  const i_exception_handler *handler{exception_handler};
  if (handler != nullptr) {
    handler->handle_exception(function_name);
    return;
  }

  default_exception_handler.handle_exception(function_name);
}

void handle_exception(std::string_view function_name,
                      const std::exception &ex) {
  const i_exception_handler *handler{exception_handler};
  if (handler != nullptr) {
    handler->handle_exception(function_name, ex);
    return;
  }

  default_exception_handler.handle_exception(function_name, ex);
}

#if defined(PROJECT_ENABLE_V2_ERRORS)
void handle_info(std::string_view function_name, std::string_view msg) {
  const i_exception_handler *handler{exception_handler};
  if (handler != nullptr) {
    handler->handle_info(function_name, msg);
    return;
  }

  default_exception_handler.handle_info(function_name, msg);
}

void handle_trace(std::string_view function_name, std::string_view msg) {
  const i_exception_handler *handler{exception_handler};
  if (handler != nullptr) {
    handler->handle_trace(function_name, msg);
    return;
  }

  default_exception_handler.handle_trace(function_name, msg);
}

void handle_warn(std::string_view function_name, std::string_view msg) {
  const i_exception_handler *handler{exception_handler};
  if (handler != nullptr) {
    handler->handle_warn(function_name, msg);
    return;
  }

  default_exception_handler.handle_warn(function_name, msg);
}
#endif // defined(PROJECT_ENABLE_V2_ERRORS)

void set_exception_handler(const i_exception_handler *handler) {
  exception_handler = handler;
}
} // namespace repertory::utils::error
