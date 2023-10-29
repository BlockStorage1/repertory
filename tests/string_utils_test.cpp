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

#include "utils/string_utils.hpp"

namespace repertory {
TEST(string_utils, is_numeric) {
  EXPECT_TRUE(utils::string::is_numeric("100"));
  EXPECT_TRUE(utils::string::is_numeric("+100"));
  EXPECT_TRUE(utils::string::is_numeric("-100"));

  EXPECT_TRUE(utils::string::is_numeric("100.00"));
  EXPECT_TRUE(utils::string::is_numeric("+100.00"));
  EXPECT_TRUE(utils::string::is_numeric("-100.00"));

  EXPECT_FALSE(utils::string::is_numeric("1.00.00"));
  EXPECT_FALSE(utils::string::is_numeric("+1.00.00"));
  EXPECT_FALSE(utils::string::is_numeric("-1.00.00"));

  EXPECT_FALSE(utils::string::is_numeric("a1"));
  EXPECT_FALSE(utils::string::is_numeric("1a"));

  EXPECT_FALSE(utils::string::is_numeric("+"));
  EXPECT_FALSE(utils::string::is_numeric("-"));

  EXPECT_FALSE(utils::string::is_numeric(""));
}
} // namespace repertory
