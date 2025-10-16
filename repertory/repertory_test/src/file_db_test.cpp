// /*
//   Copyright <2018-2025> <scott.e.graves@protonmail.com>
//
//   Permission is hereby granted, free of charge, to any person obtaining a
//   copy of this software and associated documentation files (the "Software"),
//   to deal in the Software without restriction, including without limitation
//   the rights to use, copy, modify, merge, publish, distribute, sublicense,
//   and/or sell copies of the Software, and to permit persons to whom the
//   Software is furnished to do so, subject to the following conditions:
//
//   The above copyright notice and this permission notice shall be included in
//   all copies or substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//   DEALINGS IN THE SOFTWARE.
// */
//
// #include "fixtures/file_db_fixture.hpp"
//
// namespace {
// const auto get_stop_requested = []() -> bool { return false; };
// } // namespace
//
// namespace repertory {
// TYPED_TEST_SUITE(file_db_test, file_db_types);
//
// TYPED_TEST(file_db_test, can_add_and_remove_directory) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success, this->file_db->add_directory("/",
//   "c:\\test"));
//
//   auto list = this->file_db->get_item_list(get_stop_requested);
//   EXPECT_EQ(1U, list.size());
//   EXPECT_STREQ("/", list.at(0U).api_path.c_str());
//   EXPECT_TRUE(list.at(0U).directory);
//   EXPECT_STREQ("c:\\test", list.at(0U).source_path.c_str());
//
//   EXPECT_EQ(api_error::success, this->file_db->remove_item("/"));
//
//   list = this->file_db->get_item_list(get_stop_requested);
//   EXPECT_EQ(0U, list.size());
// }
//
// TYPED_TEST(file_db_test, can_add_and_remove_file) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success, this->file_db->add_or_update_file({
//                                     "/file",
//                                     0U,
//                                     {},
//                                     "c:\\test\\file.txt",
//                                 }));
//
//   auto list = this->file_db->get_item_list(get_stop_requested);
//   EXPECT_EQ(1U, list.size());
//   EXPECT_STREQ("/file", list.at(0U).api_path.c_str());
//   EXPECT_FALSE(list.at(0U).directory);
//   EXPECT_STREQ("c:\\test\\file.txt", list.at(0U).source_path.c_str());
//
//   EXPECT_EQ(api_error::success, this->file_db->remove_item("/file"));
//
//   list = this->file_db->get_item_list(get_stop_requested);
//   EXPECT_EQ(0U, list.size());
// }
//
// TYPED_TEST(file_db_test, can_get_api_path_for_directory) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success, this->file_db->add_directory("/",
//   "c:\\test")); std::string api_path; EXPECT_EQ(api_error::success,
//             this->file_db->get_api_path("c:\\test", api_path));
//   EXPECT_STREQ("/", api_path.c_str());
// }
//
// TYPED_TEST(file_db_test, can_get_api_path_for_file) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success, this->file_db->add_or_update_file({
//                                     "/file",
//                                     0U,
//                                     {},
//                                     "c:\\test\\file.txt",
//                                 }));
//   std::string api_path;
//   EXPECT_EQ(api_error::success,
//             this->file_db->get_api_path("c:\\test\\file.txt", api_path));
//   EXPECT_STREQ("/file", api_path.c_str());
// }
//
// TYPED_TEST(file_db_test,
//            item_not_found_is_returned_for_non_existing_source_path) {
//   this->file_db->clear();
//
//   std::string api_path;
//   EXPECT_EQ(api_error::item_not_found,
//             this->file_db->get_api_path("c:\\test", api_path));
//   EXPECT_TRUE(api_path.empty());
// }
//
// TYPED_TEST(file_db_test, can_get_directory_api_path) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success, this->file_db->add_directory("/",
//   "c:\\test")); std::string api_path; EXPECT_EQ(api_error::success,
//             this->file_db->get_directory_api_path("c:\\test", api_path));
//   EXPECT_STREQ("/", api_path.c_str());
// }
//
// TYPED_TEST(
//     file_db_test,
//     directory_not_found_is_returned_for_non_existing_directory_source_path) {
//   this->file_db->clear();
//
//   std::string api_path;
//   EXPECT_EQ(api_error::directory_not_found,
//             this->file_db->get_directory_api_path("c:\\test", api_path));
//   EXPECT_TRUE(api_path.empty());
// }
//
// TYPED_TEST(file_db_test, can_get_file_api_path) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success, this->file_db->add_or_update_file({
//                                     "/file",
//                                     0U,
//                                     {},
//                                     "c:\\test\\file.txt",
//                                 }));
//
//   std::string api_path;
//   EXPECT_EQ(api_error::success,
//             this->file_db->get_file_api_path("c:\\test\\file.txt",
//             api_path));
//   EXPECT_STREQ("/file", api_path.c_str());
// }
//
// TYPED_TEST(file_db_test,
//            item_not_found_is_returned_for_non_existing_file_source_path) {
//   this->file_db->clear();
//
//   std::string api_path;
//   EXPECT_EQ(api_error::item_not_found,
//             this->file_db->get_file_api_path("c:\\test", api_path));
//   EXPECT_TRUE(api_path.empty());
// }
//
// TYPED_TEST(file_db_test, can_get_directory_source_path) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success, this->file_db->add_directory("/",
//   "c:\\test"));
//
//   std::string source_path;
//   EXPECT_EQ(api_error::success,
//             this->file_db->get_directory_source_path("/", source_path));
//   EXPECT_STREQ("c:\\test", source_path.c_str());
// }
//
// TYPED_TEST(
//     file_db_test,
//     directory_not_found_is_returned_for_non_existing_directory_api_path) {
//   this->file_db->clear();
//
//   std::string source_path;
//   EXPECT_EQ(api_error::directory_not_found,
//             this->file_db->get_directory_source_path("/", source_path));
//   EXPECT_TRUE(source_path.empty());
// }
//
// TYPED_TEST(file_db_test, can_get_file_source_path) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success, this->file_db->add_or_update_file({
//                                     "/file",
//                                     0U,
//                                     {},
//                                     "c:\\test\\file.txt",
//                                 }));
//
//   std::string source_path;
//   EXPECT_EQ(api_error::success,
//             this->file_db->get_file_source_path("/file", source_path));
//   EXPECT_STREQ("c:\\test\\file.txt", source_path.c_str());
// }
//
// TYPED_TEST(file_db_test,
//            item_not_found_is_returned_for_non_existing_file_api_path) {
//   this->file_db->clear();
//
//   std::string source_path;
//   EXPECT_EQ(api_error::item_not_found,
//             this->file_db->get_file_source_path("/file.txt", source_path));
//   EXPECT_TRUE(source_path.empty());
// }
//
// TYPED_TEST(file_db_test, can_get_file_data) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success, this->file_db->add_or_update_file({
//                                     "/file",
//                                     1U,
//                                     {{}, {}},
//                                     "c:\\test\\file.txt",
//                                 }));
//
//   i_file_db::file_data data{};
//   EXPECT_EQ(api_error::success, this->file_db->get_file_data("/file", data));
//   EXPECT_STREQ("/file", data.api_path.c_str());
//   EXPECT_EQ(1U, data.file_size);
//   EXPECT_EQ(2U, data.iv_list.size());
//   EXPECT_STREQ("c:\\test\\file.txt", data.source_path.c_str());
// }
//
// TYPED_TEST(file_db_test,
//            item_not_found_is_returned_for_non_existing_file_data_api_path) {
//   this->file_db->clear();
//
//   i_file_db::file_data data{};
//   EXPECT_EQ(api_error::item_not_found,
//             this->file_db->get_file_data("/file", data));
// }
//
// TYPED_TEST(file_db_test, can_update_existing_file_iv) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success, this->file_db->add_or_update_file({
//                                     "/file",
//                                     1U,
//                                     {{}, {}},
//                                     "c:\\test\\file.txt",
//                                 }));
//
//   EXPECT_EQ(api_error::success, this->file_db->add_or_update_file({
//                                     "/file",
//                                     1U,
//                                     {{}, {}, {}},
//                                     "c:\\test\\file.txt",
//                                 }));
//
//   i_file_db::file_data data{};
//   EXPECT_EQ(api_error::success, this->file_db->get_file_data("/file", data));
//   EXPECT_STREQ("/file", data.api_path.c_str());
//   EXPECT_EQ(1U, data.file_size);
//   EXPECT_EQ(3U, data.iv_list.size());
//   EXPECT_STREQ("c:\\test\\file.txt", data.source_path.c_str());
//
//   EXPECT_EQ(1U, this->file_db->count());
// }
//
// TYPED_TEST(file_db_test, can_update_existing_file_size) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success, this->file_db->add_or_update_file({
//                                     "/file",
//                                     1U,
//                                     {{}, {}},
//                                     "c:\\test\\file.txt",
//                                 }));
//
//   EXPECT_EQ(api_error::success, this->file_db->add_or_update_file({
//                                     "/file",
//                                     2U,
//                                     {{}, {}},
//                                     "c:\\test\\file.txt",
//                                 }));
//
//   i_file_db::file_data data{};
//   EXPECT_EQ(api_error::success, this->file_db->get_file_data("/file", data));
//   EXPECT_STREQ("/file", data.api_path.c_str());
//   EXPECT_EQ(2U, data.file_size);
//   EXPECT_EQ(2U, data.iv_list.size());
//   EXPECT_STREQ("c:\\test\\file.txt", data.source_path.c_str());
//
//   EXPECT_EQ(1U, this->file_db->count());
// }
//
// TYPED_TEST(file_db_test, can_update_existing_file_source_path) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success, this->file_db->add_or_update_file({
//                                     "/file",
//                                     1U,
//                                     {{}, {}},
//                                     "c:\\test\\file.txt",
//                                 }));
//
//   EXPECT_EQ(api_error::success, this->file_db->add_or_update_file({
//                                     "/file",
//                                     1U,
//                                     {{}, {}},
//                                     "c:\\test\\file2.txt",
//                                 }));
//
//   i_file_db::file_data data{};
//   EXPECT_EQ(api_error::success, this->file_db->get_file_data("/file", data));
//   EXPECT_STREQ("/file", data.api_path.c_str());
//   EXPECT_EQ(1U, data.file_size);
//   EXPECT_EQ(2U, data.iv_list.size());
//   EXPECT_STREQ("c:\\test\\file2.txt", data.source_path.c_str());
//
//   EXPECT_EQ(1U, this->file_db->count());
// }
//
// TYPED_TEST(file_db_test, can_get_source_path_for_directory) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success, this->file_db->add_directory("/",
//   "c:\\test")); std::string source_path; EXPECT_EQ(api_error::success,
//             this->file_db->get_source_path("/", source_path));
//   EXPECT_STREQ("c:\\test", source_path.c_str());
// }
//
// TYPED_TEST(file_db_test, can_get_source_path_for_file) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success, this->file_db->add_or_update_file({
//                                     "/file",
//                                     0U,
//                                     {},
//                                     "c:\\test\\file.txt",
//                                 }));
//   std::string source_path;
//   EXPECT_EQ(api_error::success,
//             this->file_db->get_source_path("/file", source_path));
//   EXPECT_STREQ("c:\\test\\file.txt", source_path.c_str());
// }
//
// TYPED_TEST(file_db_test,
// item_not_found_is_returned_for_non_existing_api_path) {
//   this->file_db->clear();
//
//   std::string source_path;
//   EXPECT_EQ(api_error::item_not_found,
//             this->file_db->get_source_path("/file", source_path));
//   EXPECT_TRUE(source_path.empty());
// }
//
// TYPED_TEST(file_db_test, can_enumerate_item_list) {
//   this->file_db->clear();
//
//   EXPECT_EQ(api_error::success,
//             this->file_db->add_directory("/",
//             std::filesystem::current_path().string()));
//   EXPECT_EQ(api_error::success, this->file_db->add_or_update_file({
//                                     "/file",
//                                     0U,
//                                     {},
//                                     "c:\\test\\file.txt",
//                                 }));
//
//   auto call_count{0U};
//   const auto get_stop_requested = []() -> bool { return false; };
//
//   this->file_db->enumerate_item_list(
//       [&call_count](auto &&list) {
//         EXPECT_EQ(std::size_t(2U), list.size());
//         ++call_count;
//       },
//       get_stop_requested);
//
//   EXPECT_EQ(std::size_t(1U), call_count);
// }
//
// } // namespace repertory
