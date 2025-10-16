/*
 Copyright <2018-2025> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

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

namespace {
#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
#include "utils/file_enc_file.hpp"
constexpr auto file_type_count{3U};
#else
constexpr auto file_type_count{2U};
#endif

[[nodiscard]] auto create_file(auto idx, auto path, bool read_only = false)
    -> auto {
  switch (idx) {
  case 0U:
    return repertory::utils::file::file::open_or_create_file(path, read_only);
  case 1U:
    return repertory::utils::file::thread_file::open_or_create_file(path,
                                                                    read_only);
#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
  case 2U:
    return repertory::utils::file::enc_file::attach_file(
        repertory::utils::file::file::open_or_create_file(path, read_only));
#endif
  default:
    throw std::runtime_error("not supported");
  }
}

[[nodiscard]] auto open_file(auto idx, auto path, bool read_only = false)
    -> auto {
  switch (idx) {
  case 0U:
    return repertory::utils::file::file::open_file(path, read_only);
  case 1U:
    return repertory::utils::file::thread_file::open_file(path, read_only);
#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
  case 2U:
    return repertory::utils::file::enc_file::attach_file(
        repertory::utils::file::file::open_file(path, read_only));
#endif
  default:
    throw std::runtime_error("not supported");
  }
}
} // namespace

namespace repertory {
TEST(utils_file, can_create_and_remove_file) {
  for (auto idx = 0U; idx < file_type_count; ++idx) {
    auto path = test::generate_test_file_name("utils_file");
    EXPECT_FALSE(utils::file::file(path).exists() ||
                 utils::file::directory(path).exists());

    auto file{create_file(idx, path)};
    EXPECT_TRUE(*file);

    EXPECT_TRUE(utils::file::file(path).exists());
    EXPECT_TRUE(file->exists());

    EXPECT_TRUE(file->remove());

    EXPECT_FALSE(utils::file::file(path).exists());
    EXPECT_FALSE(file->exists());
  }
}

TEST(utils_file, can_open_file) {
  for (auto idx = 0U; idx < file_type_count; ++idx) {
    auto path = test::generate_test_file_name("utils_file");

    {
      auto file{create_file(idx, path)};
      EXPECT_TRUE(*file);
    }

    {
      auto file{create_file(idx, path)};
      EXPECT_TRUE(*file);
    }
  }
}

TEST(utils_file, open_file_fails_if_not_found) {
  for (auto idx = 0U; idx < file_type_count; ++idx) {
    auto path = test::generate_test_file_name("utils_file");

    auto file{open_file(idx, path)};
    EXPECT_FALSE(*file);
  }
}

TEST(utils_file, write_fails_for_read_only_file) {
  for (auto idx = 0U; idx < file_type_count; ++idx) {
    auto path = test::generate_test_file_name("utils_file");

    auto file{create_file(idx, path, true)};
    EXPECT_TRUE(utils::file::file(path).exists());
    EXPECT_TRUE(*file);
    std::size_t bytes_written{};
    EXPECT_FALSE(file->write(reinterpret_cast<const unsigned char *>("0"), 1U,
                             0U, &bytes_written));
    EXPECT_EQ(0U, bytes_written);
  }
}

// TEST(utils_file, can_attach_file) {
//   for (auto idx = 0U; idx < file_type_count; ++idx) {
//     auto path = test::generate_test_file_name("utils_file");
//     auto file = idx == 0U ? utils::file::file::open_or_create_file(path)
//                           :
//                           utils::file::thread_file::open_or_create_file(path);
//     auto file2 =
//         idx == 0U ? utils::file::file::attach_file(file->get_handle())
//                   :
//                   utils::file::thread_file::attach_file(file->get_handle());
//     EXPECT_TRUE(*file);
//     EXPECT_TRUE(*file2);
//     EXPECT_EQ(file->get_path(), file2->get_path());
//   }
// }

#if defined(PROJECT_ENABLE_JSON)
TEST(utils_file, read_and_write_json_file) {
  auto path = test::generate_test_file_name("utils_file");

  auto json_data = nlohmann::json({{"moose", "cow"}});
  EXPECT_TRUE(utils::file::write_json_file(path, json_data));

  {
    nlohmann::json result_data{};
    EXPECT_TRUE(utils::file::read_json_file(path, result_data));
    EXPECT_STREQ(json_data.dump().c_str(), result_data.dump().c_str());
  }

  {
    nlohmann::json result_data{};
    EXPECT_TRUE(utils::file::read_json_file(utils::string::from_utf8(path),
                                            result_data));
    EXPECT_STREQ(json_data.dump().c_str(), result_data.dump().c_str());
  }
}

#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
TEST(utils_file, read_and_write_json_file_encrypted) {
  {
    auto path = test::generate_test_file_name("utils_file");

    auto json_data = nlohmann::json({{"moose", "cow"}});
    EXPECT_TRUE(utils::file::write_json_file(path, json_data, "moose"));

    nlohmann::json result_data{};
    EXPECT_TRUE(utils::file::read_json_file(path, result_data, "moose"));
    EXPECT_STREQ(json_data.dump().c_str(), result_data.dump().c_str());

    {
      auto file = utils::file::file::open_file(path);
      data_buffer encrypted_data{};
      EXPECT_TRUE(file->read_all(encrypted_data, 0U));

      data_buffer decrypted_data{};
      EXPECT_TRUE(utils::encryption::decrypt_data("moose", encrypted_data,
                                                  decrypted_data));
      EXPECT_STREQ(json_data.dump().c_str(),
                   nlohmann::json::parse(std::string(decrypted_data.begin(),
                                                     decrypted_data.end()))
                       .dump()
                       .c_str());
    }
  }

  {
    auto path =
        utils::string::from_utf8(test::generate_test_file_name("utils_file"));

    auto json_data = nlohmann::json({{"moose", "cow"}});
    EXPECT_TRUE(utils::file::write_json_file(path, json_data, L"moose"));

    nlohmann::json result_data{};
    EXPECT_TRUE(utils::file::read_json_file(path, result_data, L"moose"));
    EXPECT_STREQ(json_data.dump().c_str(), result_data.dump().c_str());

    {
      auto file = utils::file::file::open_file(path);
      data_buffer encrypted_data{};
      EXPECT_TRUE(file->read_all(encrypted_data, 0U));

      data_buffer decrypted_data{};
      EXPECT_TRUE(utils::encryption::decrypt_data("moose", encrypted_data,
                                                  decrypted_data));
      EXPECT_STREQ(json_data.dump().c_str(),
                   nlohmann::json::parse(std::string(decrypted_data.begin(),
                                                     decrypted_data.end()))
                       .dump()
                       .c_str());
    }
  }
}
#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
#endif // defined(PROJECT_ENABLE_JSON)

#if defined(PROJECT_ENABLE_LIBDSM)
TEST(utils_file, smb_create_smb_path) {
  const auto *path = "//server/share";
  const auto *rel_path = "test/test.txt";
  auto smb_path = utils::file::smb_create_smb_path(path, rel_path);
  EXPECT_STREQ("//server/share/test/test.txt", smb_path.c_str());

  rel_path = "/test/test.txt";
  smb_path = utils::file::smb_create_smb_path(path, rel_path);
  EXPECT_STREQ("//server/share/test/test.txt", smb_path.c_str());

  rel_path = "test\\test.txt";
  smb_path = utils::file::smb_create_smb_path(path, rel_path);
  EXPECT_STREQ("//server/share/test/test.txt", smb_path.c_str());

  rel_path = "\\test\\test.txt";
  smb_path = utils::file::smb_create_smb_path(path, rel_path);
  EXPECT_STREQ("//server/share/test/test.txt", smb_path.c_str());
}

TEST(utils_file, smb_create_relative_path) {
  const auto *path = "//server/share/test.txt";
  auto rel_path = utils::file::smb_create_relative_path(path);
  EXPECT_STREQ("\\test.txt", rel_path.c_str());

  path = "//server/share/test";
  rel_path = utils::file::smb_create_relative_path(path);
  EXPECT_STREQ("\\test", rel_path.c_str());

  path = "//server/share/test/";
  rel_path = utils::file::smb_create_relative_path(path);
  EXPECT_STREQ("\\test", rel_path.c_str());

  path = "//server/share/test/";
  rel_path = utils::file::smb_create_relative_path(path);
  EXPECT_STREQ("\\test", rel_path.c_str());
}

TEST(utils_file, smb_create_search_path) {
  const auto *path = "//server/share";
  auto search_path = utils::file::smb_create_search_path(path);
  EXPECT_STREQ("\\*", search_path.c_str());

  path = "//server/share/";
  search_path = utils::file::smb_create_search_path(path);
  EXPECT_STREQ("\\*", search_path.c_str());

  path = "//server/share/folder";
  search_path = utils::file::smb_create_search_path(path);
  EXPECT_STREQ("\\folder\\*", search_path.c_str());

  path = "//server/share/folder/";
  search_path = utils::file::smb_create_search_path(path);
  EXPECT_STREQ("\\folder\\*", search_path.c_str());

  path = "//server/share/folder/next";
  search_path = utils::file::smb_create_search_path(path);
  EXPECT_STREQ("\\folder\\next\\*", search_path.c_str());

  path = "//server/share/folder/next/";
  search_path = utils::file::smb_create_search_path(path);
  EXPECT_STREQ("\\folder\\next\\*", search_path.c_str());
}

TEST(utils_file, smb_parent_is_same) {
  const auto *path1 = "//server/share";
  const auto *path2 = "//server/share";
  EXPECT_TRUE(utils::file::smb_parent_is_same(path1, path2));

  path1 = "//server/share/";
  path2 = "//server/share/";
  EXPECT_TRUE(utils::file::smb_parent_is_same(path1, path2));

  path1 = "//server/share/one";
  path2 = "//server/share/two";
  EXPECT_TRUE(utils::file::smb_parent_is_same(path1, path2));

  path1 = "// server/cow";
  path2 = "// server/cow";
  EXPECT_TRUE(utils::file::smb_parent_is_same(path1, path2));
}

TEST(utils_file, smb_parent_is_not_same) {
  const auto *path1 = "server/share";
  const auto *path2 = "//server/share";
  EXPECT_FALSE(utils::file::smb_parent_is_same(path1, path2));

  path1 = "server/share/";
  path2 = "server/share/";
  EXPECT_FALSE(utils::file::smb_parent_is_same(path1, path2));

  path1 = "//server1/share/one";
  path2 = "//server/share/two";
  EXPECT_FALSE(utils::file::smb_parent_is_same(path1, path2));

  path1 = "//server/share";
  path2 = "//server/share2";
  EXPECT_FALSE(utils::file::smb_parent_is_same(path1, path2));

  path1 = "//server/share/";
  path2 = "//server/share2/";
  EXPECT_FALSE(utils::file::smb_parent_is_same(path1, path2));

  path1 = "//server/share/one";
  path2 = "//server/share2/two";
  EXPECT_FALSE(utils::file::smb_parent_is_same(path1, path2));

  path1 = "//server";
  path2 = "//server/share/two";
  EXPECT_FALSE(utils::file::smb_parent_is_same(path1, path2));

  path1 = "//server/";
  path2 = "//server/";
  EXPECT_FALSE(utils::file::smb_parent_is_same(path1, path2));

  path1 = "//server";
  path2 = "//server";
  EXPECT_FALSE(utils::file::smb_parent_is_same(path1, path2));
}
#endif // defined(PROJECT_ENABLE_LIBDSM)

TEST(utils_file, directory_exists_in_path) {
  auto &test_dir = test::generate_test_directory();
  EXPECT_FALSE(
      utils::file::directory_exists_in_path(test_dir.get_path(), "moose"));

  EXPECT_FALSE(utils::file::directory_exists_in_path(
      utils::string::from_utf8(test_dir.get_path()), L"moose"));

  EXPECT_FALSE(utils::file::file_exists_in_path(test_dir.get_path(), "moose"));

  EXPECT_FALSE(utils::file::file_exists_in_path(
      utils::string::from_utf8(test_dir.get_path()), L"moose"));

  auto sub_dir = test_dir.create_directory("moose");
  EXPECT_TRUE(sub_dir != nullptr);
  if (sub_dir) {
    EXPECT_TRUE(
        utils::file::directory_exists_in_path(test_dir.get_path(), "moose"));

    EXPECT_FALSE(
        utils::file::file_exists_in_path(test_dir.get_path(), "moose"));

    EXPECT_TRUE(utils::file::directory_exists_in_path(
        utils::string::from_utf8(test_dir.get_path()), L"moose"));

    EXPECT_FALSE(utils::file::file_exists_in_path(
        utils::string::from_utf8(test_dir.get_path()), L"moose"));
  }
}

TEST(utils_file, directory_can_get_empty_directory_count) {
  auto &test_dir = test::generate_test_directory();
  EXPECT_EQ(0U, test_dir.count());
  EXPECT_EQ(0U, test_dir.count(false));
}

TEST(utils_file, directory_can_get_empty_directory_count_recursively) {
  auto &test_dir = test::generate_test_directory();
  EXPECT_EQ(0U, test_dir.count(true));
}

TEST(utils_file, directory_can_get_non_empty_directory_count) {
  auto &test_dir = test::generate_test_directory();
  auto sub_dir = test_dir.create_directory("sub_dir");
  EXPECT_TRUE(sub_dir != nullptr);
  if (sub_dir) {
    sub_dir->create_directory("sub_dir");

    EXPECT_EQ(1U, test_dir.count());
    EXPECT_EQ(1U, test_dir.count(false));
  }
}

TEST(utils_file, directory_can_get_non_empty_directory_count_recursively) {
  auto &test_dir = test::generate_test_directory();
  auto sub_dir = test_dir.create_directory("sub_dir");
  EXPECT_TRUE(sub_dir != nullptr);
  if (sub_dir) {
    sub_dir = sub_dir->create_directory("sub_dir");
    EXPECT_TRUE(sub_dir != nullptr);
  }

  EXPECT_EQ(2U, test_dir.count(true));
}

TEST(utils_file, file_exists_in_path) {
  auto &test_dir = test::generate_test_directory();
  EXPECT_FALSE(
      utils::file::file_exists_in_path(test_dir.get_path(), "moose.txt"));

  EXPECT_FALSE(utils::file::file_exists_in_path(
      utils::string::from_utf8(test_dir.get_path()), L"moose.txt"));

  EXPECT_FALSE(
      utils::file::directory_exists_in_path(test_dir.get_path(), "moose.txt"));

  EXPECT_FALSE(utils::file::directory_exists_in_path(
      utils::string::from_utf8(test_dir.get_path()), L"moose.txt"));

  auto sub_file = test_dir.create_file("moose.txt", false);
  EXPECT_TRUE(sub_file != nullptr);
  if (sub_file) {
    EXPECT_TRUE(
        utils::file::file_exists_in_path(test_dir.get_path(), "moose.txt"));

    EXPECT_FALSE(utils::file::directory_exists_in_path(test_dir.get_path(),
                                                       "moose.txt"));

    EXPECT_TRUE(utils::file::file_exists_in_path(
        utils::string::from_utf8(test_dir.get_path()), L"moose.txt"));

    EXPECT_FALSE(utils::file::directory_exists_in_path(
        utils::string::from_utf8(test_dir.get_path()), L"moose.txt"));
  }
}

TEST(utils_file, get_free_drive_space) {
#if defined(_WIN32)
  auto space = utils::file::get_free_drive_space("C:");
  auto space2 = utils::file::get_free_drive_space(L"C:");
#else  // defined(_WIN32)
  auto space = utils::file::get_free_drive_space("/");
  auto space2 = utils::file::get_free_drive_space(L"/");
#endif // !defined(_WIN32)

  EXPECT_TRUE(space.has_value());
  EXPECT_LT(0U, space.value());

  EXPECT_TRUE(space2.has_value());
  EXPECT_EQ(space.value(), space2.value());
}

TEST(utils_file, get_free_drive_space_fails_for_bad_path) {
  std::string name{"free_drive_space_test_XXXXXX"};
  auto temp = utils::file::create_temp_name("free_drive_space_test");

  auto space = utils::file::get_free_drive_space(temp);
  EXPECT_FALSE(space.has_value());
}

TEST(utils_file, get_total_drive_space) {
#if defined(_WIN32)
  auto space = utils::file::get_total_drive_space("C:");
  auto space2 = utils::file::get_total_drive_space(L"C:");
#else  // defined(_WIN32)
  auto space = utils::file::get_total_drive_space("/");
  auto space2 = utils::file::get_total_drive_space(L"/");
#endif // !defined(_WIN32)

  EXPECT_TRUE(space.has_value());
  EXPECT_LT(0U, space.value());

  EXPECT_TRUE(space2.has_value());
  EXPECT_EQ(space.value(), space2.value());
}

TEST(utils_file, create_temp_name) {
  {
    auto temp = utils::file::create_temp_name("test_temp");
    EXPECT_EQ(18U, temp.size());

    auto temp2 = utils::file::create_temp_name("test_temp");
    EXPECT_STRNE(temp.c_str(), temp2.c_str());

    EXPECT_TRUE(utils::string::begins_with(temp, "test_temp_"));
  }

  {
    auto temp = utils::file::create_temp_name(L"test_temp");
    EXPECT_EQ(18U, temp.size());

    auto temp2 = utils::file::create_temp_name(L"test_temp");
    EXPECT_STRNE(temp.c_str(), temp2.c_str());

    EXPECT_TRUE(utils::string::begins_with(temp, L"test_temp_"));
  }
}

TEST(utils_file, get_total_drive_space_fails_for_bad_path) {
  auto temp = utils::file::create_temp_name("total_drive_space_test");
  auto space = utils::file::get_total_drive_space(temp);
  EXPECT_FALSE(space.has_value());
}

TEST(utils_file, get_times) {
  {
    auto times =
        utils::file::get_times(test::create_random_file(1U).get_path());
    EXPECT_TRUE(times.has_value());
    EXPECT_LT(0U, times->get(utils::file::time_type::accessed));
    EXPECT_LT(0U, times->get(utils::file::time_type::changed));
    EXPECT_LT(0U, times->get(utils::file::time_type::created));
    EXPECT_LT(0U, times->get(utils::file::time_type::modified));
    EXPECT_LT(0U, times->get(utils::file::time_type::written));
  }

  {
    auto times = utils::file::get_times(
        utils::string::from_utf8(test::create_random_file(1U).get_path()));
    EXPECT_TRUE(times.has_value());
    EXPECT_LT(0U, times->get(utils::file::time_type::accessed));
    EXPECT_LT(0U, times->get(utils::file::time_type::changed));
    EXPECT_LT(0U, times->get(utils::file::time_type::created));
    EXPECT_LT(0U, times->get(utils::file::time_type::modified));
    EXPECT_LT(0U, times->get(utils::file::time_type::written));
  }
}

TEST(utils_file, get_times_fails_if_not_found) {
  auto temp = utils::path::combine(".", {"get_times_test"});
  auto times = utils::file::get_times(temp);
  EXPECT_FALSE(times.has_value());
}

TEST(utils_file, get_time) {
  {
    auto file_path = test::create_random_file(1U).get_path();
    auto file_time =
        utils::file::get_time(file_path, utils::file::time_type::accessed);
    EXPECT_TRUE(file_time.has_value());
    EXPECT_LT(0U, file_time.value());

    file_time =
        utils::file::get_time(file_path, utils::file::time_type::changed);
    EXPECT_TRUE(file_time.has_value());
    EXPECT_LT(0U, file_time.value());

    file_time =
        utils::file::get_time(file_path, utils::file::time_type::created);
    EXPECT_TRUE(file_time.has_value());
    EXPECT_LT(0U, file_time.value());

    file_time =
        utils::file::get_time(file_path, utils::file::time_type::modified);
    EXPECT_TRUE(file_time.has_value());
    EXPECT_LT(0U, file_time.value());

    file_time =
        utils::file::get_time(file_path, utils::file::time_type::written);
    EXPECT_TRUE(file_time.has_value());
    EXPECT_LT(0U, file_time.value());
  }

  {
    auto file_path =
        utils::string::from_utf8(test::create_random_file(1U).get_path());

    auto file_time =
        utils::file::get_time(file_path, utils::file::time_type::accessed);
    EXPECT_TRUE(file_time.has_value());
    EXPECT_LT(0U, file_time.value());

    file_time =
        utils::file::get_time(file_path, utils::file::time_type::changed);
    EXPECT_TRUE(file_time.has_value());
    EXPECT_LT(0U, file_time.value());

    file_time =
        utils::file::get_time(file_path, utils::file::time_type::created);
    EXPECT_TRUE(file_time.has_value());
    EXPECT_LT(0U, file_time.value());

    file_time =
        utils::file::get_time(file_path, utils::file::time_type::modified);
    EXPECT_TRUE(file_time.has_value());
    EXPECT_LT(0U, file_time.value());

    file_time =
        utils::file::get_time(file_path, utils::file::time_type::written);
    EXPECT_TRUE(file_time.has_value());
    EXPECT_LT(0U, file_time.value());
  }
}

TEST(utils_file, get_time_fails_if_not_found) {
  auto temp = utils::path::combine(".", {"get_times_test"});
  auto file_time =
      utils::file::get_time(temp, utils::file::time_type::accessed);
  EXPECT_FALSE(file_time.has_value());
}
} // namespace repertory
