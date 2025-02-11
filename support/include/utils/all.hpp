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
#ifndef REPERTORY_INCLUDE_UTILS_ALL_HPP_
#define REPERTORY_INCLUDE_UTILS_ALL_HPP_

#include "utils/config.hpp"

#include "utils/base64.hpp"
#include "utils/collection.hpp"
#if defined(_WIN32)
#include "utils/com_init_wrapper.hpp"
#endif // defined(_WIN32)
#include "utils/common.hpp"
#if defined(PROJECT_ENABLE_SQLITE)
#include "utils/db/sqlite/db_common.hpp"
#include "utils/db/sqlite/db_delete.hpp"
#include "utils/db/sqlite/db_insert.hpp"
#include "utils/db/sqlite/db_select.hpp"
#include "utils/db/sqlite/db_update.hpp"
#endif // defined(PROJECT_ENABLE_SQLITE)
#if defined(PROJECT_ENABLE_LIBSODIUM)
#include "utils/encrypting_reader.hpp"
#include "utils/encryption.hpp"
#endif // defined(PROJECT_ENABLE_LIBSODIUM)
#include "utils/error.hpp"
#include "utils/file.hpp"
#if defined(PROJECT_ENABLE_LIBSODIUM)
#include "utils/hash.hpp"
#endif // defined(PROJECT_ENABLE_LIBSODIUM)
#include "utils/path.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"
#if !defined(_WIN32)
#include "utils/unix.hpp"
#endif // !defined(_WIN32)
#if defined(_WIN32)
#include "utils/windows.hpp"
#endif // defined(_WIN32)

#endif // REPERTORY_INCLUDE_UTILS_ALL_HPP_
