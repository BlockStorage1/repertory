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
TEST(utils_string, begins_with) {
  std::string str{"moose"};
  EXPECT_TRUE(utils::string::begins_with(str, "m"));
  EXPECT_TRUE(utils::string::begins_with(str, "mo"));
  EXPECT_TRUE(utils::string::begins_with(str, "moo"));
  EXPECT_TRUE(utils::string::begins_with(str, "moos"));
  EXPECT_TRUE(utils::string::begins_with(str, "moose"));

  EXPECT_FALSE(utils::string::begins_with(str, "a"));
  EXPECT_FALSE(utils::string::begins_with(str, "ma"));
  EXPECT_FALSE(utils::string::begins_with(str, "moose1"));

  std::wstring str_w{L"moose"};
  EXPECT_TRUE(utils::string::begins_with(str_w, L"m"));
  EXPECT_TRUE(utils::string::begins_with(str_w, L"mo"));
  EXPECT_TRUE(utils::string::begins_with(str_w, L"moo"));
  EXPECT_TRUE(utils::string::begins_with(str_w, L"moos"));
  EXPECT_TRUE(utils::string::begins_with(str_w, L"moose"));

  EXPECT_FALSE(utils::string::begins_with(str_w, L"a"));
  EXPECT_FALSE(utils::string::begins_with(str_w, L"ma"));
  EXPECT_FALSE(utils::string::begins_with(str_w, L"moose1"));
}

TEST(utils_string, contains) {
  std::string str{R"(\\)"};
  EXPECT_TRUE(utils::string::contains(str, "\\"));
  EXPECT_FALSE(utils::string::contains(str, "/"));

  std::wstring str_w{LR"(\\)"};
  EXPECT_TRUE(utils::string::contains(str_w, L"\\"));
  EXPECT_FALSE(utils::string::contains(str_w, L"/"));
}

TEST(utils_string, replace) {
  std::string str{"moose"};
  utils::string::replace(str, 'o', '0');
  EXPECT_STREQ("m00se", str.c_str());

  std::wstring str_w{L"moose"};
  utils::string::replace(str_w, 'o', '0');
  EXPECT_STREQ(L"m00se", str_w.c_str());

  std::string str2{"\\\\\\"};
  utils::string::replace(str2, '\\', '/');
  EXPECT_STREQ("///", str2.c_str());

  std::wstring str_w2{L"\\\\\\"};
  utils::string::replace(str_w2, '\\', '/');
  EXPECT_STREQ(L"///", str_w2.c_str());

  std::string str3{"///"};
  utils::string::replace(str3, '/', '\\');
  EXPECT_STREQ("\\\\\\", str3.c_str());

  std::wstring str_w3{L"///"};
  utils::string::replace(str_w3, '/', '\\');
  EXPECT_STREQ(L"\\\\\\", str_w3.c_str());

  str.clear();
  utils::string::replace(str, '/', '\\');
  EXPECT_STREQ("", str.c_str());

  str_w.clear();
  utils::string::replace(str_w, '/', '\\');
  EXPECT_STREQ(L"", str_w.c_str());
}

TEST(utils_string, replace_string) {
  std::string str{"moose"};
  utils::string::replace(str, "o", "0");
  EXPECT_STREQ("m00se", str.c_str());

  std::wstring str_w{L"moose"};
  utils::string::replace(str_w, L"o", L"0");
  EXPECT_STREQ(L"m00se", str_w.c_str());
}

TEST(utils_string, is_numeric) {
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

TEST(utils_string, to_bool) {
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
