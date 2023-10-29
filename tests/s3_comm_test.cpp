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
#if defined(REPERTORY_ENABLE_S3) && defined(REPERTORY_ENABLE_S3_TESTING)

#include "test_common.hpp"

#include "comm/s3/s3_comm.hpp"
#include "fixtures/s3_comm_fixture.hpp"
#include "types/repertory.hpp"

namespace repertory {
TEST_F(s3_comm_test, create_and_remove_directory) {
  auto ret = s3_comm_->create_directory("/dir");
  EXPECT_EQ(api_error::success, ret);
  EXPECT_EQ(api_error::directory_exists, s3_comm_->directory_exists("/dir"));

  ret = s3_comm_->remove_directory("/dir");
  EXPECT_EQ(api_error::success, ret);
  EXPECT_EQ(api_error::directory_not_found, s3_comm_->directory_exists("/dir"));

  ret = s3_comm_->remove_directory("/dir");
  EXPECT_TRUE(ret == api_error::success ||
              ret == api_error::directory_not_found);
}

TEST_F(s3_comm_test, upload_file) {
  stop_type stop_requested = false;
  auto ret = s3_comm_->upload_file(
      "/test.txt", __FILE__, "", []() -> std::string { return ""; },
      [](const std::string &) -> api_error { return api_error::success; },
      stop_requested);
  EXPECT_EQ(api_error::success, ret);

  ret = s3_comm_->upload_file(
      "/subdir/test2.txt", __FILE__, "", []() -> std::string { return ""; },
      [](const std::string &) -> api_error { return api_error::success; },
      stop_requested);
  EXPECT_EQ(api_error::success, ret);
}

TEST_F(s3_comm_test, get_directory_items) {
  directory_item_list list{};
  auto ret = s3_comm_->get_directory_items(
      "/subdir", [](directory_item &) {}, list);
  EXPECT_EQ(api_error::success, ret);
}

TEST_F(s3_comm_test, list_directories) {
  api_file_list list{};
  auto ret = s3_comm_->get_directory_list(list);
  EXPECT_EQ(api_error::success, ret);
}

TEST_F(s3_comm_test, list_files) {
  api_file_list list{};
  auto ret = s3_comm_->get_file_list(
      [](const std::string &) -> std::string { return ""; },
      [](const std::string &, const std::string &object_name) -> std::string {
        return object_name;
      },
      list);
  EXPECT_EQ(api_error::success, ret);
}

TEST_F(s3_comm_test, read_file_bytes) {
  stop_type stop_requested = false;
  data_buffer data;
  auto ret = s3_comm_->read_file_bytes(
      "/test.txt", 2, 0, data, []() -> std::string { return ""; },
      []() -> std::uint64_t { return 0ull; },
      []() -> std::string { return ""; }, stop_requested);
  EXPECT_EQ(api_error::success, ret);
}

TEST_F(s3_comm_test, exists) {
  EXPECT_EQ(
      api_error::item_exists,
      s3_comm_->file_exists("/test.txt", []() -> std::string { return ""; }));
  EXPECT_EQ(api_error::item_not_found,
            s3_comm_->file_exists("/subdir/test.txt",
                                  []() -> std::string { return ""; }));
}

TEST_F(s3_comm_test, get_file) {
  api_file file{};
  auto ret = s3_comm_->get_file(
      "/test.txt", []() -> std::string { return ""; },
      [](const std::string &, const std::string &object_name) -> std::string {
        return object_name;
      },
      []() -> std::string { return ""; }, file);
  EXPECT_EQ(api_error::success, ret);
}

TEST_F(s3_comm_test, remove_file) {
  auto ret =
      s3_comm_->remove_file("/test.txt", []() -> std::string { return ""; });
  EXPECT_EQ(api_error::success, ret);
  ret = s3_comm_->remove_file("/subdir/test2.txt",
                              []() -> std::string { return ""; });
  EXPECT_EQ(api_error::success, ret);
}

TEST_F(s3_comm_test, rename_file) {
  stop_type stop_requested = false;
  auto ret =
      s3_comm_->remove_file("/test_r2.txt", []() -> std::string { return ""; });

  ret = s3_comm_->upload_file(
      "/test_r1.txt", __FILE__, "", []() -> std::string { return ""; },
      [](const std::string &) -> api_error { return api_error::success; },
      stop_requested);
  EXPECT_EQ(api_error::success, ret);

  ret = s3_comm_->rename_file("/test_r1.txt", "/test_r2.txt");
  EXPECT_EQ(api_error::not_implemented, ret);
  EXPECT_EQ(api_error::item_exists,
            s3_comm_->file_exists("/test_r1.txt",
                                  []() -> std::string { return ""; }));
  EXPECT_EQ(api_error::item_not_found,
            s3_comm_->file_exists("/test_r2.txt",
                                  []() -> std::string { return ""; }));
  EXPECT_EQ(api_error::success,
            s3_comm_->remove_file("/test_r1.txt",
                                  []() -> std::string { return ""; }));
}

TEST_F(s3_comm_test, upload_file_encrypted) {
  const auto source_file_path = generate_test_file_name("./", "awscomm");
  auto file_size =
      2u * utils::encryption::encrypting_reader::get_data_chunk_size() + 3u;
  auto source_file = create_random_file(source_file_path, file_size);

  stop_type stop_requested = false;
  std::string key;
  auto ret = s3_comm_->upload_file(
      "/test.txt", source_file_path, "test", []() -> std::string { return ""; },
      [&key](const std::string &k) -> api_error {
        key = k;
        std::cout << "key:" << key << std::endl;
        return api_error::success;
      },
      stop_requested);
  EXPECT_EQ(api_error::success, ret);

  std::uint64_t offset = 0u;
  auto remain = file_size;
  while ((ret == api_error::success) && remain) {
    data_buffer data;
    ret = s3_comm_->read_file_bytes(
        "/test.txt",
        std::min(remain,
                 utils::encryption::encrypting_reader::get_data_chunk_size()),
        offset, data, [&key]() -> std::string { return key; },
        [&file_size]() -> std::uint64_t { return file_size; },
        []() -> std::string { return "test"; }, stop_requested);
    EXPECT_EQ(api_error::success, ret);

    data_buffer data2(data.size());
    std::size_t bytes_read{};
    EXPECT_TRUE(
        source_file->read_bytes(&data2[0u], data2.size(), offset, bytes_read));

    EXPECT_EQ(0, std::memcmp(&data2[0u], &data[0u], data2.size()));
    remain -= data.size();
    offset += data.size();
  }

  source_file->close();
  EXPECT_TRUE(utils::file::retry_delete_file(source_file_path));

  EXPECT_EQ(api_error::success,
            s3_comm_->remove_file("/test.txt",
                                  [&key]() -> std::string { return key; }));
}

TEST_F(s3_comm_test, get_directory_item_count) {}

TEST_F(s3_comm_test, get_object_list) {}

TEST_F(s3_comm_test, get_object_name) {}

TEST_F(s3_comm_test, is_online) {}
} // namespace repertory

#endif // REPERTORY_ENABLE_S3_TESTING
