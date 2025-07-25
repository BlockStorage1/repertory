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
#if defined(PROJECT_ENABLE_OPENSSL)
#include <openssl/ssl.h>
#endif // defined(PROJECT_ENABLE_OPENSSL)

#if defined(PROJECT_REQUIRE_ALPINE) && !defined(PROJECT_IS_MINGW)
#include <cstdlib>
#include <pthread.h>
#endif // defined(PROJECT_REQUIRE_ALPINE) && !defined (PROJECT_IS_MINGW)

#if defined(PROJECT_ENABLE_LIBSODIUM)
#include <sodium.h>
#endif // defined(PROJECT_ENABLE_LIBSODIUM)

#if defined(PROJECT_ENABLE_SQLITE)
#include <sqlite3.h>
#endif // defined(PROJECT_ENABLE_SQLITE)

#include "spdlog/spdlog.h"

#include "initialize.hpp"
#include "utils/error.hpp"

#if defined(PROJECT_REQUIRE_ALPINE) && !defined(PROJECT_IS_MINGW)
#include "utils/path.hpp"
#endif // defined(PROJECT_REQUIRE_ALPINE) && !defined (PROJECT_IS_MINGW)

#if defined(PROJECT_ENABLE_CURL)
#include "comm/curl/curl_shared.hpp"
#endif // defined(PROJECT_ENABLE_CURL)

namespace {
bool curl_initialized{false};
bool sqlite3_initialized{false};
} // namespace

namespace repertory {
auto project_initialize() -> bool {
  REPERTORY_USES_FUNCTION_NAME();

#if defined(PROJECT_REQUIRE_ALPINE) && !defined(PROJECT_IS_MINGW)
  {
    static constexpr auto guard_size{4096U};
    static constexpr auto stack_size{8U * 1024U * 1024U};

    pthread_attr_t attr{};
    pthread_attr_setstacksize(&attr, stack_size);
    pthread_attr_setguardsize(&attr, guard_size);
    pthread_setattr_default_np(&attr);

    setenv("ICU_DATA", utils::path::combine(".", {"/icu"}).c_str(), 1);
  }
#endif // defined(PROJECT_REQUIRE_ALPINE) && !defined (PROJECT_IS_MINGW)

  spdlog::drop_all();
  spdlog::flush_every(std::chrono::seconds(5));
  spdlog::set_pattern("%Y-%m-%d|%T.%e|%^%l%$|%v");

#if defined(PROJECT_ENABLE_LIBSODIUM)
  if (sodium_init() == -1) {
    utils::error::handle_error(function_name, "failed to initialize sodium");
    return false;
  }
#endif // defined(PROJECT_ENABLE_LIBSODIUM)

#if defined(PROJECT_ENABLE_OPENSSL)
  SSL_library_init();
#endif // defined(PROJECT_ENABLE_OPENSSL)

#if defined(PROJECT_ENABLE_CURL)
  if (not curl_shared::init()) {
    return false;
  }

  curl_initialized = true;
#endif // defined(PROJECT_ENABLE_CURL)

#if defined(PROJECT_ENABLE_SQLITE)
  {
    auto res = sqlite3_initialize();
    if (res != SQLITE_OK) {
      utils::error::handle_error(function_name,
                                 "failed to initialize sqlite3|result|" +
                                     std::to_string(res));
      return false;
    }

    sqlite3_initialized = true;
  }
#endif // defined(PROJECT_ENABLE_SQLITE)

  return true;
}

void project_cleanup() {
#if defined(PROJECT_ENABLE_CURL)
  if (curl_initialized) {
    curl_shared::cleanup();
  }
#endif // defined(PROJECT_ENABLE_CURL)

#if defined(PROJECT_ENABLE_SQLITE)
  if (sqlite3_initialized) {
    sqlite3_shutdown();
  }
#endif // defined(PROJECT_ENABLE_SQLITE)

  spdlog::shutdown();
}
} // namespace repertory
