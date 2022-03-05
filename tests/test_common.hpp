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
#ifndef TESTS_TEST_COMMON_HPP_
#define TESTS_TEST_COMMON_HPP_

#ifdef U
#undef U
#endif

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/events.hpp"
#include "events/event_system.hpp"
#include "providers/passthrough/passthroughprovider.hpp"

#ifdef _WIN32
// Disable DLL-interface warnings
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif

#include "utils/encryption.hpp"
#include "utils/file_utils.hpp"
#include "utils/native_file.hpp"

#define COMMA ,
#define RENTER_FILE_DATA(api_path, size)                                                           \
  {                                                                                                \
    {                                                                                              \
      "file" COMMA {                                                                               \
        {"filesize" COMMA size} COMMA{"siapath" COMMA api_path} COMMA{                             \
            "redundancy" COMMA 1.0} COMMA{"available" COMMA false} COMMA{                          \
            "expiration" COMMA 0} COMMA{"ondisk" COMMA true} COMMA{                                \
            "recoverable" COMMA false} COMMA{"renewing" COMMA false} COMMA{                        \
            "localpath" COMMA                                                                      \
            ".\\"} COMMA{"uploadedbytes" COMMA 0} COMMA{"uploadprogress" COMMA 0} COMMA{           \
            "accesstime" COMMA "2019-02-21T02:24:37.653091916-06:00"} COMMA{                       \
            "changetime" COMMA "2019-02-21T02:24:37.653091916-06:00"} COMMA{                       \
            "createtime" COMMA "2019-02-21T02:24:37.653091916-06:00"} COMMA{                       \
            "modtime" COMMA "2019-02-21T02:24:37.653091916-06:00"} COMMA                           \
      }                                                                                            \
    }                                                                                              \
  }

#define RENTER_DIR_DATA(api_path)                                                                  \
  {                                                                                                \
    {                                                                                              \
      "directories", {                                                                             \
        {                                                                                          \
          {"siapath", api_path}, {"numfiles", 0}, { "numsubdirs", 0 }                              \
        }                                                                                          \
      }                                                                                            \
    }                                                                                              \
  }

using ::testing::_;
using namespace ::testing;

namespace repertory {
native_file_ptr create_random_file(std::string path, const size_t &size);

std::string generate_test_file_name(const std::string &directory,
                                    const std::string &file_name_no_extension);

template <typename T, typename T2>
static void decrypt_and_verify(const T &buffer, const std::string &token, T2 &result) {
  EXPECT_TRUE(utils::encryption::decrypt_data(token, buffer, result));
}
} // namespace repertory

#endif // TESTS_TEST_COMMON_HPP_
