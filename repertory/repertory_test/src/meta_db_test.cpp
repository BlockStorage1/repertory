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

#include "fixtures/meta_db_fixture.hpp"

namespace {
[[nodiscard]] auto create_test_file() -> std::string {
  static std::atomic<std::uint64_t> idx{};
  return "/test" + std::to_string(++idx);
}
} // namespace

namespace repertory {
TYPED_TEST_CASE(meta_db_test, meta_db_types);

TYPED_TEST(meta_db_test, can_get_api_path_from_source_path) {
  auto test_file = create_test_file();
  auto test_source = create_test_file();
  EXPECT_EQ(
      api_error::success,
      this->meta_db->set_item_meta(
          test_file, {
                         {META_DIRECTORY, utils::string::from_bool(false)},
                         {META_SOURCE, test_source},
                     }));

  std::string api_path;
  EXPECT_EQ(api_error::success,
            this->meta_db->get_api_path(test_source, api_path));
  EXPECT_STREQ(test_file.c_str(), api_path.c_str());
}

TYPED_TEST(meta_db_test, can_change_source_path) {
  auto test_file = create_test_file();
  auto test_source = create_test_file();
  auto test_source2 = create_test_file();
  EXPECT_EQ(
      api_error::success,
      this->meta_db->set_item_meta(
          test_file, {
                         {META_DIRECTORY, utils::string::from_bool(false)},
                         {META_SOURCE, test_source},
                     }));

  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(test_file, META_SOURCE, test_source2));

  std::string api_path;
  EXPECT_EQ(api_error::success,
            this->meta_db->get_api_path(test_source2, api_path));
  EXPECT_STREQ(test_file.c_str(), api_path.c_str());

  std::string api_path2;
  EXPECT_EQ(api_error::item_not_found,
            this->meta_db->get_api_path(test_source, api_path2));
  EXPECT_TRUE(api_path2.empty());
}

TYPED_TEST(meta_db_test,
           get_api_path_returns_item_not_found_if_source_does_not_exist) {
  std::string api_path;
  EXPECT_EQ(api_error::item_not_found,
            this->meta_db->get_api_path(create_test_file(), api_path));
  EXPECT_TRUE(api_path.empty());
}

TYPED_TEST(meta_db_test, can_get_api_file_list) {
  std::vector<std::string> directories{};
  for (auto idx = 0U; idx < 5U; ++idx) {
    auto test_dir = create_test_file();
    directories.push_back(test_dir);
    EXPECT_EQ(
        api_error::success,
        this->meta_db->set_item_meta(
            test_dir, {
                          {META_DIRECTORY, utils::string::from_bool(true)},
                      }));
  }

  std::vector<std::string> files{};
  for (auto idx = 0U; idx < 5U; ++idx) {
    auto test_file = create_test_file();
    files.push_back(test_file);
    EXPECT_EQ(
        api_error::success,
        this->meta_db->set_item_meta(
            test_file, {
                           {META_DIRECTORY, utils::string::from_bool(false)},
                       }));
  }

  auto file_list = this->meta_db->get_api_path_list();
  for (const auto &api_path : directories) {
    EXPECT_TRUE(utils::collection::includes(file_list, api_path));
  }

  for (const auto &api_path : files) {
    EXPECT_TRUE(utils::collection::includes(file_list, api_path));
  }
}

TYPED_TEST(meta_db_test,
           full_get_item_meta_returns_item_not_found_if_item_does_not_exist) {
  auto api_path = create_test_file();

  api_meta_map meta;
  EXPECT_EQ(api_error::item_not_found,
            this->meta_db->get_item_meta(api_path, meta));
  EXPECT_TRUE(meta.empty());
}

TYPED_TEST(
    meta_db_test,
    individual_get_item_meta_returns_item_not_found_if_item_does_not_exist) {
  auto api_path = create_test_file();

  std::string value;
  EXPECT_EQ(api_error::item_not_found,
            this->meta_db->get_item_meta(api_path, META_DIRECTORY, value));
  EXPECT_TRUE(value.empty());
}

TYPED_TEST(meta_db_test, can_get_full_item_meta_for_directory) {
  auto api_path = create_test_file();
  auto source_path = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(
                api_path, {
                              {META_DIRECTORY, utils::string::from_bool(true)},
                              {META_PINNED, utils::string::from_bool(true)},
                              {META_SIZE, std::to_string(2ULL)},
                              {META_SOURCE, source_path},
                          }));
  api_meta_map meta;
  EXPECT_EQ(api_error::success, this->meta_db->get_item_meta(api_path, meta));

  EXPECT_TRUE(utils::string::to_bool(meta[META_DIRECTORY]));
  EXPECT_FALSE(utils::string::to_bool(meta[META_PINNED]));
  EXPECT_EQ(0U, utils::string::to_uint64(meta[META_SIZE]));
  EXPECT_TRUE(meta[META_SOURCE].empty());
}

TYPED_TEST(meta_db_test, can_get_full_item_meta_for_file) {
  auto api_path = create_test_file();
  auto source_path = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(
                api_path, {
                              {META_DIRECTORY, utils::string::from_bool(false)},
                              {META_PINNED, utils::string::from_bool(true)},
                              {META_SIZE, std::to_string(2ULL)},
                              {META_SOURCE, source_path},
                          }));

  api_meta_map meta;
  EXPECT_EQ(api_error::success, this->meta_db->get_item_meta(api_path, meta));

  EXPECT_FALSE(utils::string::to_bool(meta[META_DIRECTORY]));
  EXPECT_TRUE(utils::string::to_bool(meta[META_PINNED]));
  EXPECT_EQ(2ULL, utils::string::to_uint64(meta[META_SIZE]));
  EXPECT_STREQ(source_path.c_str(), meta[META_SOURCE].c_str());
}

TYPED_TEST(meta_db_test, can_get_individual_item_meta_for_directory) {
  auto api_path = create_test_file();
  auto source_path = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(
                api_path, {
                              {META_DIRECTORY, utils::string::from_bool(true)},
                              {META_PINNED, utils::string::from_bool(true)},
                              {META_SIZE, std::to_string(2ULL)},
                              {META_SOURCE, source_path},
                          }));
  {
    std::string value;
    EXPECT_EQ(api_error::success,
              this->meta_db->get_item_meta(api_path, META_DIRECTORY, value));
    EXPECT_TRUE(utils::string::to_bool(value));
  }

  {
    std::string value;
    EXPECT_EQ(api_error::success,
              this->meta_db->get_item_meta(api_path, META_PINNED, value));
    EXPECT_FALSE(utils::string::to_bool(value));
  }

  {
    std::string value;
    EXPECT_EQ(api_error::success,
              this->meta_db->get_item_meta(api_path, META_SIZE, value));
    EXPECT_EQ(0ULL, utils::string::to_uint64(value));
  }

  {
    std::string value;
    EXPECT_EQ(api_error::success,
              this->meta_db->get_item_meta(api_path, META_SOURCE, value));
    EXPECT_TRUE(value.empty());
  }
}

TYPED_TEST(meta_db_test, can_get_individual_item_meta_for_file) {
  auto api_path = create_test_file();
  auto source_path = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(
                api_path, {
                              {META_DIRECTORY, utils::string::from_bool(false)},
                              {META_PINNED, utils::string::from_bool(true)},
                              {META_SIZE, std::to_string(2ULL)},
                              {META_SOURCE, source_path},
                          }));
  {
    std::string value;
    EXPECT_EQ(api_error::success,
              this->meta_db->get_item_meta(api_path, META_DIRECTORY, value));
    EXPECT_FALSE(utils::string::to_bool(value));
  }

  {
    std::string value;
    EXPECT_EQ(api_error::success,
              this->meta_db->get_item_meta(api_path, META_PINNED, value));
    EXPECT_TRUE(utils::string::to_bool(value));
  }

  {
    std::string value;
    EXPECT_EQ(api_error::success,
              this->meta_db->get_item_meta(api_path, META_SIZE, value));
    EXPECT_EQ(2ULL, utils::string::to_uint64(value));
  }

  {
    std::string value;
    EXPECT_EQ(api_error::success,
              this->meta_db->get_item_meta(api_path, META_SOURCE, value));
    EXPECT_STREQ(source_path.c_str(), value.c_str());
  }
}

TYPED_TEST(meta_db_test, can_get_pinned_files) {
  std::vector<std::string> pinned_files{};
  std::vector<std::string> unpinned_files{};
  for (auto idx = 0U; idx < 20U; ++idx) {
    auto test_file = create_test_file();
    auto pinned = idx % 2U == 0U;
    EXPECT_EQ(
        api_error::success,
        this->meta_db->set_item_meta(
            test_file, {
                           {META_DIRECTORY, utils::string::from_bool(false)},
                           {META_PINNED, utils::string::from_bool(pinned)},
                       }));
    if (pinned) {
      pinned_files.push_back(test_file);
      continue;
    }
    unpinned_files.push_back(test_file);
  }

  auto pinned = this->meta_db->get_pinned_files();
  EXPECT_GE(pinned.size(), pinned_files.size());
  for (const auto &api_path : pinned_files) {
    EXPECT_TRUE(utils::collection::includes(pinned, api_path));
  }

  for (const auto &api_path : unpinned_files) {
    EXPECT_TRUE(utils::collection::excludes(pinned, api_path));
  }
}

TYPED_TEST(meta_db_test, can_get_total_item_count) {
  this->meta_db->clear();
  EXPECT_EQ(0U, this->meta_db->get_total_item_count());

  auto test_file = create_test_file();
  auto test_source = create_test_file();
  EXPECT_EQ(
      api_error::success,
      this->meta_db->set_item_meta(
          test_file, {
                         {META_DIRECTORY, utils::string::from_bool(false)},
                         {META_SOURCE, test_source},
                     }));

  auto test_dir = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(
                test_dir, {
                              {META_DIRECTORY, utils::string::from_bool(true)},
                          }));

  EXPECT_EQ(2U, this->meta_db->get_total_item_count());
}

TYPED_TEST(meta_db_test,
           get_total_item_count_decreases_after_directory_is_removed) {
  this->meta_db->clear();
  EXPECT_EQ(0U, this->meta_db->get_total_item_count());

  auto test_file = create_test_file();
  auto test_source = create_test_file();
  EXPECT_EQ(
      api_error::success,
      this->meta_db->set_item_meta(
          test_file, {
                         {META_DIRECTORY, utils::string::from_bool(false)},
                         {META_SOURCE, test_source},
                     }));

  auto test_dir = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(
                test_dir, {
                              {META_DIRECTORY, utils::string::from_bool(true)},
                          }));

  this->meta_db->remove_api_path(test_dir);
  EXPECT_EQ(1U, this->meta_db->get_total_item_count());
}

TYPED_TEST(meta_db_test, get_total_item_count_decreases_after_file_is_removed) {
  this->meta_db->clear();
  EXPECT_EQ(0U, this->meta_db->get_total_item_count());

  auto test_file = create_test_file();
  auto test_source = create_test_file();
  EXPECT_EQ(
      api_error::success,
      this->meta_db->set_item_meta(
          test_file, {
                         {META_DIRECTORY, utils::string::from_bool(false)},
                         {META_SOURCE, test_source},
                     }));

  auto test_dir = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(
                test_dir, {
                              {META_DIRECTORY, utils::string::from_bool(true)},
                          }));

  this->meta_db->remove_api_path(test_file);
  EXPECT_EQ(1U, this->meta_db->get_total_item_count());
}

TYPED_TEST(meta_db_test, can_get_total_size) {
  this->meta_db->clear();
  EXPECT_EQ(0U, this->meta_db->get_total_item_count());

  auto test_file = create_test_file();
  auto test_source = create_test_file();
  EXPECT_EQ(
      api_error::success,
      this->meta_db->set_item_meta(
          test_file, {
                         {META_DIRECTORY, utils::string::from_bool(false)},
                         {META_SOURCE, test_source},
                         {META_SIZE, "2"},
                     }));

  test_file = create_test_file();
  test_source = create_test_file();
  EXPECT_EQ(
      api_error::success,
      this->meta_db->set_item_meta(
          test_file, {
                         {META_DIRECTORY, utils::string::from_bool(false)},
                         {META_SOURCE, test_source},
                         {META_SIZE, "1"},
                     }));

  auto test_dir = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(
                test_dir, {
                              {META_DIRECTORY, utils::string::from_bool(true)},
                              {META_SIZE, "7"},
                          }));

  EXPECT_EQ(3U, this->meta_db->get_total_size());
}

TYPED_TEST(meta_db_test,
           total_size_does_not_decrease_after_directory_is_removed) {
  this->meta_db->clear();
  EXPECT_EQ(0U, this->meta_db->get_total_item_count());

  auto test_file = create_test_file();
  auto test_source = create_test_file();
  EXPECT_EQ(
      api_error::success,
      this->meta_db->set_item_meta(
          test_file, {
                         {META_DIRECTORY, utils::string::from_bool(false)},
                         {META_SOURCE, test_source},
                         {META_SIZE, "2"},
                     }));

  test_file = create_test_file();
  test_source = create_test_file();
  EXPECT_EQ(
      api_error::success,
      this->meta_db->set_item_meta(
          test_file, {
                         {META_DIRECTORY, utils::string::from_bool(false)},
                         {META_SOURCE, test_source},
                         {META_SIZE, "1"},
                     }));

  auto test_dir = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(
                test_dir, {
                              {META_DIRECTORY, utils::string::from_bool(true)},
                              {META_SIZE, "7"},
                          }));
  this->meta_db->remove_api_path(test_dir);

  EXPECT_EQ(3U, this->meta_db->get_total_size());
}

TYPED_TEST(meta_db_test, total_size_decreases_after_file_is_removed) {
  this->meta_db->clear();
  EXPECT_EQ(0U, this->meta_db->get_total_item_count());

  auto test_file = create_test_file();
  auto test_source = create_test_file();
  EXPECT_EQ(
      api_error::success,
      this->meta_db->set_item_meta(
          test_file, {
                         {META_DIRECTORY, utils::string::from_bool(false)},
                         {META_SOURCE, test_source},
                         {META_SIZE, "2"},
                     }));

  test_file = create_test_file();
  test_source = create_test_file();
  EXPECT_EQ(
      api_error::success,
      this->meta_db->set_item_meta(
          test_file, {
                         {META_DIRECTORY, utils::string::from_bool(false)},
                         {META_SOURCE, test_source},
                         {META_SIZE, "1"},
                     }));

  auto test_dir = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(
                test_dir, {
                              {META_DIRECTORY, utils::string::from_bool(true)},
                              {META_SIZE, "7"},
                          }));
  this->meta_db->remove_api_path(test_file);

  EXPECT_EQ(2U, this->meta_db->get_total_size());
}

TYPED_TEST(meta_db_test, can_remove_api_path) {
  auto test_file = create_test_file();
  auto test_source = create_test_file();
  EXPECT_EQ(
      api_error::success,
      this->meta_db->set_item_meta(
          test_file, {
                         {META_DIRECTORY, utils::string::from_bool(false)},
                         {META_SOURCE, test_source},
                         {META_SIZE, "2"},
                     }));
  this->meta_db->remove_api_path(test_file);

  api_meta_map meta;
  EXPECT_EQ(api_error::item_not_found,
            this->meta_db->get_item_meta(test_file, meta));
}

TYPED_TEST(meta_db_test, can_rename_item_meta) {
  auto test_file = create_test_file();
  auto test_source = create_test_file();
  EXPECT_EQ(
      api_error::success,
      this->meta_db->set_item_meta(
          test_file, {
                         {META_DIRECTORY, utils::string::from_bool(false)},
                         {META_SOURCE, test_source},
                         {META_SIZE, "2"},
                     }));
  auto test_file2 = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->rename_item_meta(test_file, test_file2));

  api_meta_map meta;
  EXPECT_EQ(api_error::item_not_found,
            this->meta_db->get_item_meta(test_file, meta));

  EXPECT_EQ(api_error::success, this->meta_db->get_item_meta(test_file2, meta));
}

TYPED_TEST(meta_db_test, rename_item_meta_fails_if_not_found) {
  auto test_file = create_test_file();
  auto test_file2 = create_test_file();

  EXPECT_EQ(api_error::item_not_found,
            this->meta_db->rename_item_meta(test_file, test_file2));
}

TYPED_TEST(meta_db_test, set_item_meta_fails_with_missing_directory_meta) {
  auto test_file = create_test_file();
  auto test_source = create_test_file();
  EXPECT_EQ(api_error::error, this->meta_db->set_item_meta(
                                  test_file, {
                                                 {META_SOURCE, test_source},
                                             }));
  EXPECT_EQ(api_error::error,
            this->meta_db->set_item_meta(test_file, META_SOURCE, test_source));
}

TYPED_TEST(meta_db_test, check_size_is_ignored_for_directory) {
  auto test_dir = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(
                test_dir, {
                              {META_DIRECTORY, utils::string::from_bool(true)},
                              {META_SIZE, "2"},
                          }));

  api_meta_map meta;
  EXPECT_EQ(api_error::success, this->meta_db->get_item_meta(test_dir, meta));
  EXPECT_EQ(0U, utils::string::to_uint64(meta[META_SIZE]));
}

TYPED_TEST(meta_db_test, check_pinned_is_ignored_for_directory) {
  auto test_dir = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(
                test_dir, {
                              {META_DIRECTORY, utils::string::from_bool(true)},
                              {META_PINNED, utils::string::from_bool(true)},
                          }));

  api_meta_map meta;
  EXPECT_EQ(api_error::success, this->meta_db->get_item_meta(test_dir, meta));
  EXPECT_FALSE(utils::string::to_bool(meta[META_PINNED]));
}

TYPED_TEST(meta_db_test, check_source_is_ignored_for_directory) {
  auto test_dir = create_test_file();
  auto test_source = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(
                test_dir, {
                              {META_DIRECTORY, utils::string::from_bool(true)},
                              {META_SOURCE, test_source},
                          }));

  api_meta_map meta;
  EXPECT_EQ(api_error::success, this->meta_db->get_item_meta(test_dir, meta));
  EXPECT_TRUE(meta[META_SOURCE].empty());
}

TYPED_TEST(meta_db_test, check_set_item_meta_directory_defaults) {
  auto test_dir = create_test_file();
  EXPECT_EQ(api_error::success,
            this->meta_db->set_item_meta(
                test_dir, {
                              {META_DIRECTORY, utils::string::from_bool(true)},
                          }));

  api_meta_map meta;
  EXPECT_EQ(api_error::success, this->meta_db->get_item_meta(test_dir, meta));

  EXPECT_TRUE(utils::string::to_bool(meta[META_DIRECTORY]));
  EXPECT_FALSE(utils::string::to_bool(meta[META_PINNED]));
  EXPECT_EQ(0U, utils::string::to_uint64(meta[META_SIZE]));
  EXPECT_TRUE(meta[META_SOURCE].empty());
}

TYPED_TEST(meta_db_test, check_set_item_meta_file_defaults) {
  auto test_file = create_test_file();
  EXPECT_EQ(
      api_error::success,
      this->meta_db->set_item_meta(
          test_file, {
                         {META_DIRECTORY, utils::string::from_bool(false)},
                     }));
  api_meta_map meta;
  EXPECT_EQ(api_error::success, this->meta_db->get_item_meta(test_file, meta));

  EXPECT_FALSE(utils::string::to_bool(meta[META_DIRECTORY]));
  EXPECT_FALSE(utils::string::to_bool(meta[META_PINNED]));
  EXPECT_EQ(0U, utils::string::to_uint64(meta[META_SIZE]));
  EXPECT_TRUE(meta[META_SOURCE].empty());
}
} // namespace repertory
