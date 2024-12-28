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

#include "fixtures/file_mgr_db_fixture.hpp"
#include <utils/time.hpp>

namespace repertory {
TYPED_TEST_CASE(file_mgr_db_test, file_mgr_db_types);

TYPED_TEST(file_mgr_db_test, can_add_and_remove_resume) {
  this->file_mgr_db->clear();

  EXPECT_TRUE(this->file_mgr_db->add_resume({
      "/test0",
      2ULL,
      {},
      "/src/test0",
  }));
  auto list = this->file_mgr_db->get_resume_list();
  EXPECT_EQ(1U, list.size());
  EXPECT_EQ(1U, list.size());
  EXPECT_STREQ("/test0", list.at(0U).api_path.c_str());
  EXPECT_EQ(2ULL, list.at(0U).chunk_size);
  EXPECT_STREQ("/src/test0", list.at(0U).source_path.c_str());

  EXPECT_TRUE(this->file_mgr_db->remove_resume("/test0"));
  list = this->file_mgr_db->get_resume_list();
  EXPECT_TRUE(list.empty());
}

TYPED_TEST(file_mgr_db_test, can_get_resume_list) {
  this->file_mgr_db->clear();

  for (auto idx = 0U; idx < 5U; ++idx) {
    EXPECT_TRUE(this->file_mgr_db->add_resume({
        "/test1_" + std::to_string(idx),
        2ULL + idx,
        {},
        "/src/test1_" + std::to_string(idx),
    }));
  }

  auto list = this->file_mgr_db->get_resume_list();
  EXPECT_EQ(5U, list.size());
  for (auto idx = 0U; idx < list.size(); ++idx) {
    EXPECT_STREQ(("/test1_" + std::to_string(idx)).c_str(),
                 list.at(idx).api_path.c_str());
    EXPECT_EQ(2ULL + idx, list.at(idx).chunk_size);
    EXPECT_STREQ(("/src/test1_" + std::to_string(idx)).c_str(),
                 list.at(idx).source_path.c_str());
  }
}

TYPED_TEST(file_mgr_db_test, can_replace_resume) {
  this->file_mgr_db->clear();

  EXPECT_TRUE(this->file_mgr_db->add_resume({
      "/test0",
      2ULL,
      {},
      "/src/test0",
  }));
  EXPECT_TRUE(this->file_mgr_db->add_resume({
      "/test0",
      3ULL,
      {},
      "/src/test1",
  }));

  auto list = this->file_mgr_db->get_resume_list();
  EXPECT_EQ(1U, list.size());
  EXPECT_STREQ("/test0", list.at(0U).api_path.c_str());
  EXPECT_EQ(3ULL, list.at(0U).chunk_size);
  EXPECT_STREQ("/src/test1", list.at(0U).source_path.c_str());

  EXPECT_TRUE(this->file_mgr_db->remove_resume("/test0"));
}

TYPED_TEST(file_mgr_db_test, can_rename_resume) {
  this->file_mgr_db->clear();

  EXPECT_TRUE(this->file_mgr_db->add_resume({
      "/test0",
      2ULL,
      {},
      "/src/test0",
  }));
  EXPECT_TRUE(this->file_mgr_db->rename_resume("/test0", "/test1"));

  auto list = this->file_mgr_db->get_resume_list();
  EXPECT_EQ(1U, list.size());
  EXPECT_STREQ("/test1", list.at(0U).api_path.c_str());
  EXPECT_EQ(2ULL, list.at(0U).chunk_size);
  EXPECT_STREQ("/src/test0", list.at(0U).source_path.c_str());

  EXPECT_TRUE(this->file_mgr_db->remove_resume("/test1"));
}

TYPED_TEST(file_mgr_db_test, can_add_get_and_remove_upload) {
  this->file_mgr_db->clear();
  EXPECT_TRUE(this->file_mgr_db->add_upload({
      "/test0",
      "/src/test0",
  }));

  auto upload = this->file_mgr_db->get_upload("/test0");
  EXPECT_TRUE(upload.has_value());

  EXPECT_TRUE(this->file_mgr_db->remove_upload("/test0"));

  upload = this->file_mgr_db->get_next_upload();
  EXPECT_FALSE(upload.has_value());
}

TYPED_TEST(file_mgr_db_test, uploads_are_correctly_ordered) {
  this->file_mgr_db->clear();
  EXPECT_TRUE(this->file_mgr_db->add_upload({
      "/test08",
      "/src/test0",
  }));

  EXPECT_TRUE(this->file_mgr_db->add_upload({
      "/test07",
      "/src/test1",
  }));

  auto upload = this->file_mgr_db->get_next_upload();
  EXPECT_TRUE(upload.has_value());
  EXPECT_STREQ("/test08", upload->api_path.c_str());

  EXPECT_TRUE(this->file_mgr_db->remove_upload("/test08"));
  upload = this->file_mgr_db->get_next_upload();
  EXPECT_TRUE(upload.has_value());
  EXPECT_STREQ("/test07", upload->api_path.c_str());

  EXPECT_TRUE(this->file_mgr_db->remove_upload("/test07"));

  upload = this->file_mgr_db->get_next_upload();
  EXPECT_FALSE(upload.has_value());
}
} // namespace repertory
