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

#include "fixtures/meta_db_fixture.hpp"

namespace repertory {
TEST_F(meta_db_test, get_and_set_item_meta) {
  api_meta_map meta{
      {"test", "test_value"},
      {"test2", "test_value2"},
      {"test3", "test_value3"},
  };
  EXPECT_EQ(api_error::success, db_->set_item_meta("/test/item", meta));

  api_meta_map meta2{};
  EXPECT_EQ(api_error::success, db_->get_item_meta("/test/item", meta2));

  EXPECT_EQ(meta, meta2);
}

TEST_F(meta_db_test, get_and_set_item_meta_single_key) {
  EXPECT_EQ(api_error::success,
            db_->set_item_meta("/test/item", "test", "moose"));

  std::string value;
  EXPECT_EQ(api_error::success,
            db_->get_item_meta("/test/item", "test", value));

  EXPECT_STREQ("moose", value.c_str());
}

TEST_F(meta_db_test,
       get_item_meta_fails_with_not_found_for_items_that_dont_exist) {
  api_meta_map meta{};
  EXPECT_EQ(api_error::item_not_found, db_->get_item_meta("/test/item", meta));

  std::string value;
  EXPECT_EQ(api_error::item_not_found,
            db_->get_item_meta("/test/item", "test", value));
  EXPECT_TRUE(value.empty());
}

TEST_F(meta_db_test, get_item_meta_exists) {
  EXPECT_EQ(api_error::success,
            db_->set_item_meta("/test/item", "test", "value"));
  EXPECT_TRUE(db_->get_item_meta_exists("/test/item"));
}

TEST_F(meta_db_test, get_item_meta_exists_is_false_if_not_found) {
  EXPECT_FALSE(db_->get_item_meta_exists("/test/item"));
}

TEST_F(meta_db_test, remove_item_meta) {
  api_meta_map meta{
      {"test", "test_value"},
      {"test2", "test_value2"},
      {"test3", "test_value3"},
  };
  EXPECT_EQ(api_error::success, db_->set_item_meta("/test/item", meta));

  EXPECT_EQ(api_error::success, db_->remove_item_meta("/test/item"));

  api_meta_map meta2{};
  EXPECT_EQ(api_error::item_not_found, db_->get_item_meta("/test/item", meta2));

  EXPECT_TRUE(meta2.empty());
}

TEST_F(meta_db_test, remove_item_meta_single_key) {
  api_meta_map meta{
      {"test", "test_value"},
      {"test2", "test_value2"},
      {"test3", "test_value3"},
  };
  EXPECT_EQ(api_error::success, db_->set_item_meta("/test/item", meta));

  EXPECT_EQ(api_error::success, db_->remove_item_meta("/test/item", "test"));

  api_meta_map meta2{};
  EXPECT_EQ(api_error::success, db_->get_item_meta("/test/item", meta2));

  meta.erase("test");
  EXPECT_EQ(meta, meta2);
}

TEST_F(meta_db_test, get_and_set_source_path) {
  EXPECT_EQ(api_error::success,
            db_->set_source_path("/test/item", "/test/path"));

  std::string value;
  EXPECT_EQ(api_error::success,
            db_->get_item_meta("/test/item", META_SOURCE, value));
  EXPECT_STREQ("/test/path", value.c_str());
}

// TEST_F(meta_db_test, set_source_path_fails_on_empty_path) {
//   EXPECT_EQ(api_error::invalid_operation, db_->set_source_path("/test/item",
//   ""));
//
//   std::string value;
//   EXPECT_EQ(api_error::item_not_found, db_->get_item_meta("/test/item",
//   META_SOURCE, value)); EXPECT_TRUE(value.empty());
// }

TEST_F(meta_db_test, get_api_path_from_source) {
  EXPECT_EQ(api_error::success,
            db_->set_source_path("/test/item", "/test/path"));

  std::string value;
  EXPECT_EQ(api_error::success,
            db_->get_api_path_from_source("/test/path", value));
  EXPECT_STREQ("/test/item", value.c_str());
}

TEST_F(meta_db_test, get_api_path_from_source_succeeds_after_change) {
  EXPECT_EQ(api_error::success,
            db_->set_source_path("/test/item", "/test/path"));
  EXPECT_EQ(api_error::success,
            db_->set_source_path("/test/item", "/test/path2"));

  std::string value;
  EXPECT_EQ(api_error::success,
            db_->get_api_path_from_source("/test/path2", value));
  EXPECT_STREQ("/test/item", value.c_str());

  value.clear();
  EXPECT_EQ(api_error::item_not_found,
            db_->get_api_path_from_source("/test/path", value));
  EXPECT_TRUE(value.empty());
}

TEST_F(meta_db_test, get_api_path_from_source_fails_after_remove_all_meta) {
  EXPECT_EQ(api_error::success,
            db_->set_source_path("/test/item", "/test/path"));
  EXPECT_EQ(api_error::success, db_->remove_item_meta("/test/item"));

  std::string value;
  EXPECT_EQ(api_error::item_not_found,
            db_->get_api_path_from_source("/test/path", value));
  EXPECT_TRUE(value.empty());
}

TEST_F(meta_db_test, get_api_path_from_source_fails_after_remove_source_key) {
  EXPECT_EQ(api_error::success,
            db_->set_source_path("/test/item", "/test/path"));
  EXPECT_EQ(api_error::success,
            db_->remove_item_meta("/test/item", META_SOURCE));

  std::string value;
  EXPECT_EQ(api_error::item_not_found,
            db_->get_api_path_from_source("/test/path", value));
  EXPECT_TRUE(value.empty());
}

TEST_F(meta_db_test, get_source_path_exists) {
  EXPECT_EQ(api_error::success,
            db_->set_source_path("/test/item", "/test/path"));
  EXPECT_TRUE(db_->get_source_path_exists("/test/path"));
}

TEST_F(meta_db_test, get_source_path_exists_is_false_if_not_found) {
  EXPECT_FALSE(db_->get_source_path_exists("/test/item"));
}

TEST_F(meta_db_test, get_api_path_from_key) {
  EXPECT_EQ(api_error::success,
            db_->set_item_meta("/test/item", META_KEY, "key"));

  std::string value;
  EXPECT_EQ(api_error::success, db_->get_api_path_from_key("key", value));
  EXPECT_STREQ("/test/item", value.c_str());
}

TEST_F(meta_db_test, remove_item_meta_succeeds_for_items_that_dont_exist) {
  EXPECT_EQ(api_error::success, db_->remove_item_meta("/test/item"));
  EXPECT_EQ(api_error::success, db_->remove_item_meta("/test/item", "test"));
}

TEST_F(meta_db_test, remove_item_meta_removes_source_path) {
  EXPECT_EQ(api_error::success,
            db_->set_source_path("/test/item", "/source/path"));
  EXPECT_EQ(api_error::success, db_->remove_item_meta("/test/item"));

  std::string api_path;
  EXPECT_EQ(api_error::item_not_found,
            db_->get_api_path_from_source("/source/path", api_path));
  EXPECT_TRUE(api_path.empty());
}

TEST_F(meta_db_test, remove_item_meta_by_key_removes_source_path) {
  EXPECT_EQ(api_error::success,
            db_->set_source_path("/test/item", "/source/path"));
  EXPECT_EQ(api_error::success,
            db_->remove_item_meta("/test/item", META_SOURCE));

  std::string api_path;
  EXPECT_EQ(api_error::item_not_found,
            db_->get_api_path_from_source("/source/path", api_path));
  EXPECT_TRUE(api_path.empty());
}

TEST_F(meta_db_test, remove_item_meta_removes_key) {
  EXPECT_EQ(api_error::success,
            db_->set_item_meta("/test/item", META_KEY, "key"));
  EXPECT_EQ(api_error::success, db_->remove_item_meta("/test/item"));

  std::string api_path;
  EXPECT_EQ(api_error::item_not_found,
            db_->get_api_path_from_key("key", api_path));
  EXPECT_TRUE(api_path.empty());
}

TEST_F(meta_db_test, remove_item_meta_by_key_removes_key) {
  EXPECT_EQ(api_error::success,
            db_->set_item_meta("/test/item", META_KEY, "key"));
  EXPECT_EQ(api_error::success, db_->remove_item_meta("/test/item", META_KEY));

  std::string api_path;
  EXPECT_EQ(api_error::item_not_found,
            db_->get_api_path_from_key("/source/path", api_path));
  EXPECT_TRUE(api_path.empty());
}

TEST_F(meta_db_test, rename_item_meta) {
  api_meta_map meta{
      {"test", "test_value"},
      {"test2", "test_value2"},
      {"test3", "test_value3"},
  };
  EXPECT_EQ(api_error::success, db_->set_item_meta("/test/item", meta));
  EXPECT_EQ(api_error::success,
            db_->rename_item_meta("", "/test/item", "/test/item2"));

  api_meta_map meta2{};
  EXPECT_EQ(api_error::item_not_found, db_->get_item_meta("/test/item", meta2));

  EXPECT_EQ(api_error::success, db_->get_item_meta("/test/item2", meta2));
  EXPECT_EQ(meta, meta2);
}

TEST_F(meta_db_test, rename_item_meta_with_key) {
  api_meta_map meta{
      {META_KEY, "test_key"},
      {"test2", "test_value2"},
      {"test3", "test_value3"},
  };
  EXPECT_EQ(api_error::success, db_->set_item_meta("/test/item", meta));
  EXPECT_EQ(api_error::success,
            db_->rename_item_meta("", "/test/item", "/test/item2"));

  std::string api_path;
  EXPECT_EQ(api_error::success,
            db_->get_api_path_from_key("test_key", api_path));
  EXPECT_STREQ("/test/item2", api_path.c_str());
}

TEST_F(meta_db_test, rename_item_meta_with_source_path) {
  api_meta_map meta{
      {META_SOURCE, "/test/source"},
      {"test2", "test_value2"},
      {"test3", "test_value3"},
  };
  EXPECT_EQ(api_error::success, db_->set_item_meta("/test/item", meta));
  EXPECT_EQ(api_error::success,
            db_->rename_item_meta("/test/source", "/test/item", "/test/item2"));

  std::string api_path;
  EXPECT_EQ(api_error::success,
            db_->get_api_path_from_source("/test/source", api_path));
  EXPECT_STREQ("/test/item2", api_path.c_str());
}
} // namespace repertory
