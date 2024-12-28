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

#if defined(_WIN32)
namespace {
static const auto test_path = [](std::string str) -> std::string {
#if defined(PROJECT_ENABLE_WIN32_LONG_PATH_NAMES)
  if (repertory::utils::string::begins_with(str, "\\")) {
    str = repertory::utils::string::to_lower(
              std::filesystem::current_path().string().substr(0U, 2U)) +
          str;
  }

  str = std::string{repertory::utils::path::long_notation} + str;
#else  // !defined(PROJECT_ENABLE_WIN32_LONG_PATH_NAMES)
  if (repertory::utils::string::begins_with(str, "\\")) {
    str = repertory::utils::string::to_lower(
              std::filesystem::current_path().string().substr(0U, 2U)) +
          str;
  }
#endif // defined(PROJECT_ENABLE_WIN32_LONG_PATH_NAMES)

  return repertory::utils::string::right_trim(str, '\\');
};
} // namespace
#endif // defined(_WIN32)

namespace repertory {
TEST(utils_path, constants) {
  EXPECT_EQ(std::string_view{"\\"}, utils::path::backslash);
  EXPECT_EQ(std::wstring_view{L"\\"}, utils::path::backslash_w);
  EXPECT_EQ(std::string_view{"."}, utils::path::dot);
  EXPECT_EQ(std::wstring_view{L"."}, utils::path::dot_w);
  EXPECT_EQ(std::string_view{".\\"}, utils::path::dot_backslash);
  EXPECT_EQ(std::wstring_view{L".\\"}, utils::path::dot_backslash_w);
  EXPECT_EQ(std::string_view{"./"}, utils::path::dot_slash);
  EXPECT_EQ(std::wstring_view{L"./"}, utils::path::dot_slash_w);
  EXPECT_EQ(std::string_view{"/"}, utils::path::slash);
  EXPECT_EQ(std::wstring_view{L"/"}, utils::path::slash_w);

#if defined(_WIN32)
  EXPECT_EQ(std::string_view{"\\\\"}, utils::path::unc_notation);
  EXPECT_EQ(std::wstring_view{L"\\\\"}, utils::path::unc_notation_w);
#endif // defined(_WIN32)
}

TEST(utils_path, directory_seperator) {
#if defined(_WIN32)
  EXPECT_EQ(utils::path::backslash, utils::path::directory_seperator);
  EXPECT_EQ(utils::path::backslash_w, utils::path::directory_seperator_w);

  EXPECT_EQ(utils::path::slash, utils::path::not_directory_seperator);
  EXPECT_EQ(utils::path::slash_w, utils::path::not_directory_seperator_w);
#else  // !defined(_WIN32)
  EXPECT_EQ(utils::path::slash, utils::path::directory_seperator);
  EXPECT_EQ(utils::path::slash_w, utils::path::directory_seperator_w);

  EXPECT_EQ(utils::path::backslash, utils::path::not_directory_seperator);
  EXPECT_EQ(utils::path::backslash_w, utils::path::not_directory_seperator_w);
#endif // defined(_WIN32)
}

TEST(utils_path, get_directory_seperator) {
#if defined(_WIN32)
  EXPECT_EQ(utils::path::backslash,
            utils::path::get_directory_seperator<char>());
  EXPECT_EQ(utils::path::backslash_w,
            utils::path::get_directory_seperator<wchar_t>());

  EXPECT_EQ(utils::path::slash,
            utils::path::get_not_directory_seperator<char>());
  EXPECT_EQ(utils::path::slash_w,
            utils::path::get_not_directory_seperator<wchar_t>());
#else  // !defined(_WIN32)
  EXPECT_EQ(utils::path::slash, utils::path::get_directory_seperator<char>());
  EXPECT_EQ(utils::path::slash_w,
            utils::path::get_directory_seperator<wchar_t>());

  EXPECT_EQ(utils::path::backslash,
            utils::path::get_not_directory_seperator<char>());
  EXPECT_EQ(utils::path::backslash_w,
            utils::path::get_not_directory_seperator<wchar_t>());
#endif // defined(_WIN32)
}

TEST(utils_path, get_backslash) {
  EXPECT_EQ(utils::path::backslash, utils::path::get_backslash<char>());
  EXPECT_EQ(utils::path::backslash_w, utils::path::get_backslash<wchar_t>());
}

TEST(utils_path, get_dot) {
  EXPECT_EQ(utils::path::dot, utils::path::get_dot<char>());
  EXPECT_EQ(utils::path::dot_w, utils::path::get_dot<wchar_t>());
}

TEST(utils_path, get_dot_backslash) {
  EXPECT_EQ(utils::path::dot_backslash, utils::path::get_dot_backslash<char>());
  EXPECT_EQ(utils::path::dot_backslash_w,
            utils::path::get_dot_backslash<wchar_t>());
}

TEST(utils_path, get_dot_slash) {
  EXPECT_EQ(utils::path::dot_slash, utils::path::get_dot_slash<char>());
  EXPECT_EQ(utils::path::dot_slash_w, utils::path::get_dot_slash<wchar_t>());
}

TEST(utils_path, get_slash) {
  EXPECT_EQ(utils::path::slash, utils::path::get_slash<char>());
  EXPECT_EQ(utils::path::slash_w, utils::path::get_slash<wchar_t>());
}

TEST(utils_path, get_long_notation) {
  EXPECT_EQ(utils::path::long_notation, utils::path::get_long_notation<char>());
  EXPECT_EQ(utils::path::long_notation_w,
            utils::path::get_long_notation<wchar_t>());
}

TEST(utils_path, combine) {
  auto s = utils::path::combine(R"(\test\path)", {});
#if defined(_WIN32)
  EXPECT_STREQ(test_path(R"(\test\path)").c_str(), s.c_str());
#else
  EXPECT_STREQ("/test/path", s.c_str());
#endif

  s = utils::path::combine(R"(\test)", {R"(\path)"});
#if defined(_WIN32)
  EXPECT_STREQ(test_path(R"(\test\path)").c_str(), s.c_str());
#else
  EXPECT_STREQ("/test/path", s.c_str());
#endif

  s = utils::path::combine(R"(\test)", {R"(\path)", R"(\again\)"});
#if defined(_WIN32)
  EXPECT_STREQ(test_path(R"(\test\path\again)").c_str(), s.c_str());
#else
  EXPECT_STREQ("/test/path/again", s.c_str());
#endif

  s = utils::path::combine("/home/test/.dest", {".state"});
#if defined(_WIN32)
  EXPECT_STREQ(test_path(R"(\home\test\.dest\.state)").c_str(), s.c_str());
#else
  EXPECT_STREQ("/home/test/.dest/.state", s.c_str());
#endif

#if defined(_WIN32)
  s = utils::path::combine(R"(R:\test)", {R"(\path)", R"(\again\)"});
  EXPECT_STREQ(test_path(R"(r:\test\path\again)").c_str(), s.c_str());

  s = utils::path::combine("R:", {R"(\path)", R"(\again\)"});
  EXPECT_STREQ(test_path(R"(r:\path\again)").c_str(), s.c_str());

  s = utils::path::combine("R:", {});
  EXPECT_STREQ(test_path("r:").c_str(), s.c_str());

  s = utils::path::combine("R:", {"\\"});
  EXPECT_STREQ(test_path("r:").c_str(), s.c_str());

  s = utils::path::combine("\\\\moose", {"cow"});
  EXPECT_STREQ("\\\\moose\\cow", s.c_str());
#endif
}

TEST(utils_path, format_path) {
  std::string path{"./"};
  utils::path::format_path(path, utils::path::slash, utils::path::backslash);
  EXPECT_STREQ(".", path.c_str());

  path = "~/.test";
  utils::path::format_path(path, utils::path::slash, utils::path::backslash);
  EXPECT_STREQ("~/.test", path.c_str());

  path = "\\";
  utils::path::format_path(path, utils::path::slash, utils::path::backslash);
  EXPECT_STREQ("/", path.c_str());

  path = "\\\\";
  utils::path::format_path(path, utils::path::slash, utils::path::backslash);
  EXPECT_STREQ("/", path.c_str());

  path = "\\\\\\";
  utils::path::format_path(path, utils::path::slash, utils::path::backslash);
  EXPECT_STREQ("/", path.c_str());

  path = "\\\\\\\\";
  utils::path::format_path(path, utils::path::slash, utils::path::backslash);
  EXPECT_STREQ("/", path.c_str());

  path = "/";
  utils::path::format_path(path, utils::path::slash, utils::path::backslash);
  EXPECT_STREQ("/", path.c_str());
  path = "//";
  utils::path::format_path(path, utils::path::slash, utils::path::backslash);
  EXPECT_STREQ("/", path.c_str());

  path = "///";
  utils::path::format_path(path, utils::path::slash, utils::path::backslash);
  EXPECT_STREQ("/", path.c_str());

  path = "////";
  utils::path::format_path(path, utils::path::slash, utils::path::backslash);
  EXPECT_STREQ("/", path.c_str());
}

TEST(utils_path, create_api_path) {
  auto s = utils::path::create_api_path("");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::create_api_path(R"(\)");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::create_api_path("/");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::create_api_path(".");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::create_api_path("./");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::create_api_path(R"(\\)");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::create_api_path("//");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::create_api_path("/cow///moose/////dog/chicken");
  EXPECT_STREQ("/cow/moose/dog/chicken", s.c_str());

  s = utils::path::create_api_path("\\cow\\\\\\moose\\\\\\\\dog\\chicken/");
  EXPECT_STREQ("/cow/moose/dog/chicken", s.c_str());

  s = utils::path::create_api_path("/cow\\\\/moose\\\\/\\dog\\chicken\\");
  EXPECT_STREQ("/cow/moose/dog/chicken", s.c_str());

  s = utils::path::create_api_path(".state");
  EXPECT_STREQ("/.state", s.c_str());

  s = utils::path::create_api_path("/.state/.local");
  EXPECT_STREQ("/.state/.local", s.c_str());

  s = utils::path::create_api_path("./.state/.local");
  EXPECT_STREQ("/.state/.local", s.c_str());
}

TEST(utils_path, get_parent_api_path) {
  auto s = utils::path::get_parent_api_path("");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::get_parent_api_path("/");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::get_parent_api_path("/moose");
  EXPECT_STREQ("/", s.c_str());

  s = utils::path::get_parent_api_path("/moose/cow");
  EXPECT_STREQ("/moose", s.c_str());

  s = utils::path::get_parent_api_path("/moose/cow/");
  EXPECT_STREQ("/moose", s.c_str());
}

TEST(utils_path, finalize) {
  auto s = utils::path::finalize("");
  EXPECT_STREQ("", s.c_str());

  s = utils::path::finalize(R"(\)");
#if defined(_WIN32)
  EXPECT_STREQ(test_path(R"(\)").c_str(), s.c_str());
#else
  EXPECT_STREQ("/", s.c_str());
#endif

  s = utils::path::finalize("/");
#if defined(_WIN32)
  EXPECT_STREQ(test_path(R"(\)").c_str(), s.c_str());
#else
  EXPECT_STREQ("/", s.c_str());
#endif

  s = utils::path::finalize(R"(\\)");
#if defined(_WIN32)
  EXPECT_STREQ("\\\\", s.c_str());
#else
  EXPECT_STREQ("/", s.c_str());
#endif

  s = utils::path::finalize("//");
#if defined(_WIN32)
  EXPECT_STREQ("\\\\", s.c_str());
#else
  EXPECT_STREQ("/", s.c_str());
#endif

  s = utils::path::finalize("/cow///moose/////dog/chicken");
#if defined(_WIN32)
  EXPECT_STREQ(test_path(R"(\cow\moose\dog\chicken)").c_str(), s.c_str());
#else
  EXPECT_STREQ("/cow/moose/dog/chicken", s.c_str());
#endif

  s = utils::path::finalize("\\cow\\\\\\moose\\\\\\\\dog\\chicken/");
#if defined(_WIN32)
  EXPECT_STREQ(test_path(R"(\cow\moose\dog\chicken)").c_str(), s.c_str());
#else
  EXPECT_STREQ("/cow/moose/dog/chicken", s.c_str());
#endif

  s = utils::path::finalize("/cow\\\\/moose\\\\/\\dog\\chicken\\");
#if defined(_WIN32)
  EXPECT_STREQ(test_path(R"(\cow\moose\dog\chicken)").c_str(), s.c_str());
#else
  EXPECT_STREQ("/cow/moose/dog/chicken", s.c_str());
#endif

#if defined(_WIN32)
  s = utils::path::finalize("D:");
  EXPECT_STREQ(test_path("d:").c_str(), s.c_str());

  s = utils::path::finalize("D:\\");
  EXPECT_STREQ(test_path("d:").c_str(), s.c_str());

  s = utils::path::finalize("D:\\moose");
  EXPECT_STREQ(test_path("d:\\moose").c_str(), s.c_str());

  s = utils::path::finalize("D:\\moose\\");
  EXPECT_STREQ(test_path("d:\\moose").c_str(), s.c_str());

  s = utils::path::finalize("D:/");
  EXPECT_STREQ(test_path("d:").c_str(), s.c_str());

  s = utils::path::finalize("D:/moose");
  EXPECT_STREQ(test_path("d:\\moose").c_str(), s.c_str());

  s = utils::path::finalize("D:/moose/");
  EXPECT_STREQ(test_path("d:\\moose").c_str(), s.c_str());

  s = utils::path::finalize("\\\\moose\\cow");
  EXPECT_STREQ("\\\\moose\\cow", s.c_str());

  s = utils::path::finalize("//moose/cow");
  EXPECT_STREQ("\\\\moose\\cow", s.c_str());
#else  // !defined(_WIN32)
  s = utils::path::finalize("\\\\moose\\cow");
  EXPECT_STREQ("/moose/cow", s.c_str());

  s = utils::path::finalize("//moose/cow");
  EXPECT_STREQ("/moose/cow", s.c_str());
#endif // defined(_WIN32)
}

TEST(utils_path, absolute) {
  auto dir = utils::path::get_current_path<std::string>();
  auto path = utils::path::absolute(".");
  EXPECT_STREQ(dir.c_str(), path.c_str());

  path = utils::path::absolute("./");
  EXPECT_STREQ(dir.c_str(), path.c_str());

  path = utils::path::absolute(R"(.\)");
  EXPECT_STREQ(dir.c_str(), path.c_str());

#if defined(_WIN32)
  path = utils::path::absolute(R"(.\moose)");
  EXPECT_STREQ((dir + R"(\moose)").c_str(), path.c_str());

  path = utils::path::absolute(R"(./moose)");
  EXPECT_STREQ((dir + R"(\moose)").c_str(), path.c_str());

  path = utils::path::absolute(R"(\\server\share)");
  EXPECT_STREQ(R"(\\server\share)", path.c_str());

  path = utils::path::absolute(R"(//server/share)");
  EXPECT_STREQ(R"(\\server\share)", path.c_str());

  auto home_env = utils::get_environment_variable("USERPROFILE");
#else  // !defined(_WIN32)
  path = utils::path::absolute(R"(.\moose)");
  EXPECT_STREQ((dir + R"(/moose)").c_str(), path.c_str());

  path = utils::path::absolute(R"(./moose)");
  EXPECT_STREQ((dir + R"(/moose)").c_str(), path.c_str());

  path = utils::path::absolute(R"(\\server\share)");
  EXPECT_STREQ(R"(/server/share)", path.c_str());
#endif // defined(_WIN32)
}

TEST(utils_path, absolute_can_resolve_path_variables) {
#if defined(_WIN32)
  auto home =
      utils::path::absolute(utils::get_environment_variable("USERPROFILE"));
  EXPECT_STREQ(home.c_str(), utils::path::absolute("%USERPROFILE%").c_str());
#else  // !defined(_WIN32)
  auto home = utils::path::absolute(utils::get_environment_variable("HOME"));
#endif // defined(_WIN32)

  auto expanded_str = utils::path::absolute("~\\");
  EXPECT_STREQ(home.c_str(), expanded_str.c_str());

  expanded_str = utils::path::absolute("~/");
  EXPECT_STREQ(home.c_str(), expanded_str.c_str());

  expanded_str = utils::path::absolute("~");
  EXPECT_STREQ("~", expanded_str.c_str());
}

TEST(utils_path, get_parent_path) {
#if defined(_WIN32)
  {
    auto dir = R"(c:\test)";
    auto parent = utils::path::get_parent_path(dir);
    EXPECT_STREQ("c:", parent.c_str());

    dir = R"(c:\test\file.txt)";
    parent = utils::path::get_parent_path(dir);
    EXPECT_STREQ(R"(c:\test)", parent.c_str());

    dir = "c:";
    parent = utils::path::get_parent_path(dir);
    EXPECT_STREQ("c:", parent.c_str());
  }

  {
    auto dir = LR"(c:\test)";
    auto parent = utils::path::get_parent_path(dir);
    EXPECT_STREQ(L"c:", parent.c_str());

    dir = LR"(c:\test\file.txt)";
    parent = utils::path::get_parent_path(dir);
    EXPECT_STREQ(LR"(c:\test)", parent.c_str());

    dir = L"c:";
    parent = utils::path::get_parent_path(dir);
    EXPECT_STREQ(L"c:", parent.c_str());
  }
#else  // !defined(_WIN32)
  {
    auto dir = "/test";
    auto parent = utils::path::get_parent_path(dir);
    EXPECT_STREQ("/", parent.c_str());

    dir = "/test/test";
    parent = utils::path::get_parent_path(dir);
    EXPECT_STREQ("/test", parent.c_str());
  }

  {
    auto dir = L"/test";
    auto parent = utils::path::get_parent_path(dir);
    EXPECT_STREQ(L"/", parent.c_str());

    dir = L"/test/test";
    parent = utils::path::get_parent_path(dir);
    EXPECT_STREQ(L"/test", parent.c_str());
  }
#endif // defined(_WIN32)
}

TEST(utils_path, contains_trash_directory) {
#if defined(_WIN32)
  {
    auto dir = R"(c:\$recycle.bin)";
    EXPECT_TRUE(utils::path::contains_trash_directory(dir));

    dir = R"(c:\$recycle.bin\moose.txt)";
    EXPECT_TRUE(utils::path::contains_trash_directory(dir));
  }

  {
    auto dir = LR"(c:\$recycle.bin)";
    EXPECT_TRUE(utils::path::contains_trash_directory(dir));

    dir = LR"(c:\$recycle.bin\moose.txt)";
    EXPECT_TRUE(utils::path::contains_trash_directory(dir));
  }
#else  // !defined(_WIN32)
  {
    auto dir = "/$recycle.bin";
    EXPECT_TRUE(utils::path::contains_trash_directory(dir));

    dir = "/$recycle.bin/moose.txt";
    EXPECT_TRUE(utils::path::contains_trash_directory(dir));
  }

  {
    auto dir = L"/$recycle.bin";
    EXPECT_TRUE(utils::path::contains_trash_directory(dir));

    dir = L"/$recycle.bin/moose.txt";
    EXPECT_TRUE(utils::path::contains_trash_directory(dir));
  }
#endif // defined(_WIN32)
}

TEST(utils_path, does_not_contain_trash_directory) {
#if defined(_WIN32)
  {
    auto dir = R"(c:\recycle.bin)";
    EXPECT_FALSE(utils::path::contains_trash_directory(dir));

    dir = R"(c:\recycle.bin\moose.txt)";
    EXPECT_FALSE(utils::path::contains_trash_directory(dir));
  }

  {
    auto dir = LR"(c:\recycle.bin)";
    EXPECT_FALSE(utils::path::contains_trash_directory(dir));

    dir = LR"(c:\recycle.bin\moose.txt)";
    EXPECT_FALSE(utils::path::contains_trash_directory(dir));
  }
#else  // !defined(_WIN32)
  {
    auto dir = "/recycle.bin";
    EXPECT_FALSE(utils::path::contains_trash_directory(dir));

    dir = "/recycle.bin/moose.txt)";
    EXPECT_FALSE(utils::path::contains_trash_directory(dir));
  }

  {
    auto dir = L"/recycle.bin";
    EXPECT_FALSE(utils::path::contains_trash_directory(dir));

    dir = L"/recycle.bin/moose.txt)";
    EXPECT_FALSE(utils::path::contains_trash_directory(dir));
  }
#endif // defined(_WIN32)
}
} // namespace repertory
