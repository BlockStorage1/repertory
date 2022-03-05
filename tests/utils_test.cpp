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
#include "test_common.hpp"
#include "comm/curl/curl_comm.hpp"
#include "types/skynet.hpp"
#include "utils/path_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
#ifdef _WIN32
TEST(utils, convert_api_date) {
  LARGE_INTEGER li{};
  li.QuadPart = utils::convert_api_date("2019-02-21T03:24:37.653091916-06:00");

  SYSTEMTIME st{};
  FileTimeToSystemTime((FILETIME *)&li, &st);

  SYSTEMTIME lt{};
  SystemTimeToTzSpecificLocalTime(nullptr, &st, &lt);

  EXPECT_EQ(2019, lt.wYear);
  EXPECT_EQ(2, lt.wMonth);
  EXPECT_EQ(21, lt.wDay);

  EXPECT_EQ(3, lt.wHour);
  EXPECT_EQ(24, lt.wMinute);
  EXPECT_EQ(37, lt.wSecond);
  EXPECT_EQ(653, lt.wMilliseconds);
}
#endif

TEST(utils, create_uuid_string) {
  const auto uuid1 = utils::create_uuid_string();
  const auto uuid2 = utils::create_uuid_string();
  ASSERT_EQ(36, uuid1.size());
  ASSERT_EQ(36, uuid2.size());
  ASSERT_STRNE(&uuid1[0], &uuid2[0]);
}

TEST(utils, string_to_bool) {
  EXPECT_TRUE(utils::string::to_bool("1"));
  EXPECT_TRUE(utils::string::to_bool("-1"));
  EXPECT_TRUE(utils::string::to_bool("0.1"));
  EXPECT_TRUE(utils::string::to_bool("-0.1"));
  EXPECT_TRUE(utils::string::to_bool("00000.1000000"));
  EXPECT_TRUE(utils::string::to_bool("true"));

  EXPECT_FALSE(utils::string::to_bool("false"));
  EXPECT_FALSE(utils::string::to_bool("0"));
  EXPECT_FALSE(utils::string::to_bool("00000.00000"));
}
} // namespace repertory
