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

#include "types/repertory.hpp"

namespace repertory {
TEST(atomic_test, atomic_primitive) {
  atomic<std::uint16_t> value;
  value = 5U;
  EXPECT_EQ(5U, static_cast<std::uint16_t>(value));
  EXPECT_EQ(5U, value.load());

  value.store(6U);
  EXPECT_EQ(6U, static_cast<std::uint16_t>(value));
  EXPECT_EQ(6U, value.load());
}

TEST(atomic_test, atomic_primitive_equality) {
  atomic<std::uint16_t> value1{5U};
  atomic<std::uint16_t> value2{5U};
  EXPECT_EQ(value1, value1);
  EXPECT_EQ(value2, value2);
  EXPECT_EQ(value1, value2);
  EXPECT_EQ(static_cast<std::uint16_t>(value1), 5U);
  EXPECT_EQ(static_cast<std::uint16_t>(value2), 5U);
}

TEST(atomic_test, atomic_primitive_inequality) {
  atomic<std::uint16_t> value1{5U};
  atomic<std::uint16_t> value2{6U};
  EXPECT_NE(value1, value2);
  EXPECT_NE(static_cast<std::uint16_t>(value1), 6U);
  EXPECT_NE(static_cast<std::uint16_t>(value2), 5U);
}

TEST(atomic_test, atomic_struct) {
  atomic<encrypt_config> value{
      encrypt_config{
          .encryption_token = "token",
          .path = "path",
      },
  };

  auto data = static_cast<encrypt_config>(value);
  EXPECT_STREQ("token", data.encryption_token.c_str());
  EXPECT_STREQ("path", data.path.c_str());
}
} // namespace repertory
