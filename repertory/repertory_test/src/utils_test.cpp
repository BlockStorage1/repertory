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
#include "test_common.hpp"

#include "providers/s3/s3_provider.hpp"
#include "utils/file.hpp"

namespace repertory {
TEST(utils, convert_api_date) {
#if defined(_WIN32)
  auto file_time = utils::time::unix_time_to_filetime(
      s3_provider::convert_api_date("2009-10-12T17:50:30.111Z"));

  SYSTEMTIME st{};
  FileTimeToSystemTime(&file_time, &st);

  EXPECT_EQ(2009, st.wYear);
  EXPECT_EQ(10, st.wMonth);
  EXPECT_EQ(12, st.wDay);

  EXPECT_EQ(17, st.wHour);
  EXPECT_EQ(50, st.wMinute);
  EXPECT_EQ(30, st.wSecond);
  EXPECT_EQ(111, st.wMilliseconds);
#else  // !defined(_WIN32)
  auto unix_time = s3_provider::convert_api_date("2009-10-12T17:50:30.111Z") /
                   utils::time::NANOS_PER_SECOND;
  auto *tm_data = gmtime(reinterpret_cast<time_t *>(&unix_time));
  EXPECT_TRUE(tm_data != nullptr);
  if (tm_data != nullptr) {
    EXPECT_EQ(2009, tm_data->tm_year + 1900);
    EXPECT_EQ(10, tm_data->tm_mon + 1);
    EXPECT_EQ(12, tm_data->tm_mday);

    EXPECT_EQ(17, tm_data->tm_hour);
    EXPECT_EQ(50, tm_data->tm_min);
    EXPECT_EQ(30, tm_data->tm_sec);

    auto millis = (s3_provider::convert_api_date("2009-10-12T17:50:30.111Z") %
                   utils::time::NANOS_PER_SECOND) /
                  1000000UL;
    EXPECT_EQ(111U, millis);
  }
#endif // defined(_WIN32)
}

TEST(utils, generate_sha256) {
  auto res = utils::file::file{__FILE__}.sha256();
  EXPECT_TRUE(res.has_value());
  if (res.has_value()) {
    std::cout << res.value() << std::endl;
  }
}
} // namespace repertory
