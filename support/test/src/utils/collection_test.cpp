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
#include "test.hpp"

namespace repertory {
TEST(utils_collection, excludes) {
  auto data = {"cow", "moose", "dog", "chicken"};
  EXPECT_FALSE(utils::collection::excludes(data, "chicken"));
  EXPECT_FALSE(utils::collection::excludes(data, "cow"));
  EXPECT_FALSE(utils::collection::excludes(data, "dog"));
  EXPECT_FALSE(utils::collection::excludes(data, "moose"));
  EXPECT_TRUE(utils::collection::excludes(data, "mouse"));
}

TEST(utils_collection, includes) {
  auto data = {"cow", "moose", "dog", "chicken"};
  EXPECT_FALSE(utils::collection::includes(data, "mice"));
  EXPECT_TRUE(utils::collection::includes(data, "chicken"));
  EXPECT_TRUE(utils::collection::includes(data, "cow"));
  EXPECT_TRUE(utils::collection::includes(data, "dog"));
  EXPECT_TRUE(utils::collection::includes(data, "moose"));
}

TEST(utils_collection, from_hex_string) {
  {
    auto data = "0xABCDEF10";
    std::vector<std::uint8_t> val{};
    EXPECT_TRUE(utils::collection::from_hex_string(data, val));
    EXPECT_EQ(4U, val.size());
  }

  {
    auto data = " 0xABCDEF10 ";
    std::vector<std::uint8_t> val{};
    EXPECT_TRUE(utils::collection::from_hex_string(data, val));
    EXPECT_EQ(4U, val.size());
  }

  {
    auto data = "ABCDEF10";
    std::vector<std::uint8_t> val{};
    EXPECT_TRUE(utils::collection::from_hex_string(data, val));
    EXPECT_EQ(4U, val.size());
  }

  {
    auto data = "ACDEF";
    std::vector<std::uint8_t> val{};
    EXPECT_TRUE(utils::collection::from_hex_string(data, val));
    EXPECT_EQ(3U, val.size());
  }

  {
    auto data = " ACDEF ";
    std::vector<std::uint8_t> val{};
    EXPECT_TRUE(utils::collection::from_hex_string(data, val));
    EXPECT_EQ(3U, val.size());
  }

  {
    auto data = "";
    std::vector<std::uint8_t> val{};
    EXPECT_TRUE(utils::collection::from_hex_string(data, val));
    EXPECT_TRUE(val.empty());
  }

  {
    auto data = L"0xABCDEF10";
    std::vector<std::uint8_t> val{};
    EXPECT_TRUE(utils::collection::from_hex_string(data, val));
    EXPECT_EQ(4U, val.size());
  }

  {
    auto data = L" 0xABCDEF10 ";
    std::vector<std::uint8_t> val{};
    EXPECT_TRUE(utils::collection::from_hex_string(data, val));
    EXPECT_EQ(4U, val.size());
  }

  {
    auto data = L"ABCDEF10";
    std::vector<std::uint8_t> val{};
    EXPECT_TRUE(utils::collection::from_hex_string(data, val));
    EXPECT_EQ(4U, val.size());
  }

  {
    auto data = L"ACDEF";
    std::vector<std::uint8_t> val{};
    EXPECT_TRUE(utils::collection::from_hex_string(data, val));
    EXPECT_EQ(3U, val.size());
  }

  {
    auto data = L" ACDEF ";
    std::vector<std::uint8_t> val{};
    EXPECT_TRUE(utils::collection::from_hex_string(data, val));
    EXPECT_EQ(3U, val.size());
  }

  {
    auto data = L"";
    std::vector<std::uint8_t> val{};
    EXPECT_TRUE(utils::collection::from_hex_string(data, val));
    EXPECT_TRUE(val.empty());
  }
}

TEST(utils_collection, from_hex_string_fails) {
  {
    auto data = "ABCDEF1Z";
    std::vector<std::uint8_t> val{};
    EXPECT_FALSE(utils::collection::from_hex_string(data, val));
    EXPECT_TRUE(val.empty());
  }

  {
    auto data = "ABC DEF1";
    std::vector<std::uint8_t> val{};
    EXPECT_FALSE(utils::collection::from_hex_string(data, val));
    EXPECT_TRUE(val.empty());
  }

  {
    auto data = "0x";
    std::vector<std::uint8_t> val{};
    EXPECT_FALSE(utils::collection::from_hex_string(data, val));
    EXPECT_TRUE(val.empty());
  }

  {
    auto data = " 0x ";
    std::vector<std::uint8_t> val{};
    EXPECT_FALSE(utils::collection::from_hex_string(data, val));
    EXPECT_TRUE(val.empty());
  }

  {
    auto data = L"ABCDEF1Z";
    std::vector<std::uint8_t> val{};
    EXPECT_FALSE(utils::collection::from_hex_string(data, val));
    EXPECT_TRUE(val.empty());
  }

  {
    auto data = L"ABC DEF1";
    std::vector<std::uint8_t> val{};
    EXPECT_FALSE(utils::collection::from_hex_string(data, val));
    EXPECT_TRUE(val.empty());
  }

  {
    auto data = L"0x";
    std::vector<std::uint8_t> val{};
    EXPECT_FALSE(utils::collection::from_hex_string(data, val));
    EXPECT_TRUE(val.empty());
  }

  {
    auto data = L" 0x";
    std::vector<std::uint8_t> val{};
    EXPECT_FALSE(utils::collection::from_hex_string(data, val));
    EXPECT_TRUE(val.empty());
  }
}

TEST(utils_collection, to_hex_string) {
  {
    std::array<std::int8_t, 2U> col{
        static_cast<std::int8_t>(0xFF),
        static_cast<std::int8_t>(0xEE),
    };

    auto str = utils::collection::to_hex_string(col);
    EXPECT_STREQ("ffee", str.c_str());

    auto w_str = utils::collection::to_hex_wstring(col);
    EXPECT_STREQ(L"ffee", w_str.c_str());
  }

  {
    std::array<std::uint8_t, 2U> col{
        static_cast<std::uint8_t>(0xFF),
        static_cast<std::uint8_t>(0xEE),
    };

    auto str = utils::collection::to_hex_string(col);
    EXPECT_STREQ("ffee", str.c_str());

    auto w_str = utils::collection::to_hex_wstring(col);
    EXPECT_STREQ(L"ffee", w_str.c_str());
  }
}

TEST(utils_collection, remove_element) {
  {
    std::vector<std::uint8_t> col{
        static_cast<std::uint8_t>(0xFF),
        static_cast<std::uint8_t>(0xEE),
    };

    utils::collection::remove_element(col, 0xFF);
    EXPECT_EQ(1U, col.size());
    EXPECT_EQ(static_cast<std::uint8_t>(0xEE), col.at(0U));
  }

  {
    std::vector<std::uint8_t> col{
        static_cast<std::uint8_t>(0xFF),
        static_cast<std::uint8_t>(0xEE),
    };

    utils::collection::remove_element(col, 0xEE);
    EXPECT_EQ(1U, col.size());
    EXPECT_EQ(static_cast<std::uint8_t>(0xFF), col.at(0U));
  }

  {
    std::vector<std::uint8_t> col{
        static_cast<std::uint8_t>(0xFF),
        static_cast<std::uint8_t>(0xEE),
    };

    utils::collection::remove_element(col, 0xEF);
    EXPECT_EQ(2U, col.size());
    EXPECT_EQ(static_cast<std::uint8_t>(0xFF), col.at(0U));
    EXPECT_EQ(static_cast<std::uint8_t>(0xEE), col.at(1U));
  }
}
} // namespace repertory
