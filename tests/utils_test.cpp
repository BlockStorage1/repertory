/*
  Copyright <2018-2023> <scott.e.graves@protonmail.com>

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
#include "test_common.hpp"

#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
#ifdef _WIN32
TEST(utils, convert_api_date) {
  LARGE_INTEGER li{};
  li.QuadPart = utils::convert_api_date("2009-10-12T17:50:30.111Z");

  SYSTEMTIME st{};
  FileTimeToSystemTime(reinterpret_cast<FILETIME *>(&li), &st);

  SYSTEMTIME lt{};
  SystemTimeToTzSpecificLocalTime(nullptr, &st, &lt);

  EXPECT_EQ(2009, lt.wYear);
  EXPECT_EQ(10, lt.wMonth);
  EXPECT_EQ(12, lt.wDay);

  EXPECT_EQ(17, lt.wHour);
  EXPECT_EQ(50, lt.wMinute);
  EXPECT_EQ(20, lt.wSecond);
  EXPECT_EQ(111, lt.wMilliseconds);
}
#endif

TEST(utils, create_uuid_string) {
  const auto uuid1 = utils::create_uuid_string();
  const auto uuid2 = utils::create_uuid_string();
  ASSERT_EQ(36u, uuid1.size());
  ASSERT_EQ(36u, uuid2.size());
  ASSERT_STRNE(&uuid1[0], &uuid2[0]);
}

TEST(utils, generate_sha256) {
  const auto res = utils::file::generate_sha256(__FILE__);
  std::cout << res << std::endl;
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
