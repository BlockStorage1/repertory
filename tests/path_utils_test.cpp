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
#include "utils/path_utils.hpp"

namespace repertory {
TEST(path_utils, combine) {
  auto s = utils::path::combine(R"(\test\path)", {});

#ifdef _WIN32
  EXPECT_STREQ(R"(\test\path)", s.c_str());
#else
  EXPECT_STREQ("/test/path", s.c_str());
#endif

  s = utils::path::combine(R"(\test)", {R"(\path)"});
#ifdef _WIN32
  EXPECT_STREQ(R"(\test\path)", s.c_str());
#else
  EXPECT_STREQ("/test/path", s.c_str());
#endif

  s = utils::path::combine(R"(\test)", {R"(\path)", R"(\again\)"});
#ifdef _WIN32
  EXPECT_STREQ(R"(\test\path\again)", s.c_str());
#else
  EXPECT_STREQ("/test/path/again", s.c_str());
#endif
}

TEST(path_utils, create_api_path) {
  auto s = utils::path::create_api_path("");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::create_api_path(R"(\)");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::create_api_path("/");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::create_api_path(".");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::create_api_path(R"(\\)");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::create_api_path("//");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::create_api_path("/cow///moose/////dog/chicken");
  EXPECT_STREQ("/cow/moose/dog/chicken", s.c_str());

  s = utils::path::create_api_path("\\cow\\\\\\moose\\\\\\\\dog\\chicken/");
  EXPECT_STREQ("/cow/moose/dog/chicken/", s.c_str());

  s = utils::path::create_api_path("/cow\\\\/moose\\\\/\\dog\\chicken\\");
  EXPECT_STREQ("/cow/moose/dog/chicken/", s.c_str());
}

TEST(path_utils, finalize) {
  auto s = utils::path::finalize("");
  EXPECT_STREQ("", s.c_str());

  s = utils::path::finalize(R"(\)");
#ifdef _WIN32
  EXPECT_STREQ(R"(\)", s.c_str());
#else
  EXPECT_STREQ("/", s.c_str());
#endif

  s = utils::path::finalize("/");
#ifdef _WIN32
  EXPECT_STREQ(R"(\)", s.c_str());
#else
  EXPECT_STREQ("/", s.c_str());
#endif

  s = utils::path::finalize(R"(\\)");
#ifdef _WIN32
  EXPECT_STREQ(R"(\)", s.c_str());
#else
  EXPECT_STREQ("/", s.c_str());
#endif

  s = utils::path::finalize("//");
#ifdef _WIN32
  EXPECT_STREQ(R"(\)", s.c_str());
#else
  EXPECT_STREQ("/", s.c_str());
#endif

  s = utils::path::finalize("/cow///moose/////dog/chicken");
#ifdef _WIN32
  EXPECT_STREQ(R"(\cow\moose\dog\chicken)", s.c_str());
#else
  EXPECT_STREQ("/cow/moose/dog/chicken", s.c_str());
#endif

  s = utils::path::finalize("\\cow\\\\\\moose\\\\\\\\dog\\chicken/");
#ifdef _WIN32
  EXPECT_STREQ(R"(\cow\moose\dog\chicken)", s.c_str());
#else
  EXPECT_STREQ("/cow/moose/dog/chicken", s.c_str());
#endif

  s = utils::path::finalize("/cow\\\\/moose\\\\/\\dog\\chicken\\");
#ifdef _WIN32
  EXPECT_STREQ(R"(\cow\moose\dog\chicken)", s.c_str());
#else
  EXPECT_STREQ("/cow/moose/dog/chicken", s.c_str());
#endif
}
} // namespace repertory
