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
#if defined(REPERTORY_ENABLE_S3_TESTING)

#include "comm/aws_s3/aws_s3_comm.hpp"
#include "common.hpp"
#include "fixtures/aws_s3_comm_fixture.hpp"
#include "test_common.hpp"

namespace repertory {
TEST_F(aws_s3_comm_test, upload_file) {
  auto ret = s3_comm_->upload_file(
      "/repertory/test.txt", __FILE__, "", []() -> std::string { return ""; },
      [](const std::string &) -> api_error { return api_error::success; }, false);
  EXPECT_EQ(api_error::success, ret);

  ret = s3_comm_->upload_file(
      "/repertory/subdir/test2.txt", __FILE__, "", []() -> std::string { return ""; },
      [](const std::string &) -> api_error { return api_error::success; }, false);
  EXPECT_EQ(api_error::success, ret);
}

TEST_F(aws_s3_comm_test, get_directory_items) {
  directory_item_list list{};
  auto ret = s3_comm_->get_directory_items(
      "/repertory/subdir", [](directory_item &, const bool &) {}, list);
  EXPECT_EQ(api_error::success, ret);
}

TEST_F(aws_s3_comm_test, list_files) {
  api_file_list list{};
  auto ret = s3_comm_->get_file_list(
      [](const std::string &) -> std::string { return ""; },
      [](const std::string &, const std::string &object_name) -> std::string {
        return object_name;
      },
      list);
  EXPECT_EQ(api_error::success, ret);
}

TEST_F(aws_s3_comm_test, read_file_bytes) {
  bool stop_requested = false;
  std::vector<char> data;
  auto ret = s3_comm_->read_file_bytes(
      "/repertory/test.txt", 2, 0, data, []() -> std::string { return ""; },
      []() -> std::uint64_t { return 0ull; }, []() -> std::string { return ""; }, stop_requested);
  EXPECT_EQ(api_error::success, ret);
}

TEST_F(aws_s3_comm_test, exists) {
  EXPECT_TRUE(s3_comm_->exists("/repertory/test.txt", []() -> std::string { return ""; }));
  EXPECT_FALSE(s3_comm_->exists("/repertory/subdir/test.txt", []() -> std::string { return ""; }));
}

TEST_F(aws_s3_comm_test, get_file) {
  api_file file{};
  auto ret = s3_comm_->get_file(
      "/repertory/test.txt", []() -> std::string { return ""; },
      [](const std::string &, const std::string &object_name) -> std::string {
        return object_name;
      },
      []() -> std::string { return ""; }, file);
  EXPECT_EQ(api_error::success, ret);
}

TEST_F(aws_s3_comm_test, remove_file) {
  auto ret = s3_comm_->remove_file("/repertory/test.txt", []() -> std::string { return ""; });
  EXPECT_EQ(api_error::success, ret);
  ret = s3_comm_->remove_file("/repertory/subdir/test2.txt", []() -> std::string { return ""; });
  EXPECT_EQ(api_error::success, ret);
}

TEST_F(aws_s3_comm_test, rename_file) {
  auto ret = s3_comm_->upload_file(
      "/repertory/test_r1.txt", __FILE__, "", []() -> std::string { return ""; },
      [](const std::string &) -> api_error { return api_error::success; }, false);
  EXPECT_EQ(api_error::success, ret);
  s3_comm_->remove_file("/repertory/test_r2.txt", []() -> std::string { return ""; });

  ret = s3_comm_->rename_file("/repertory/test_r1.txt", "/repertory/test_r2.txt");
  EXPECT_EQ(api_error::not_implemented, ret);
  EXPECT_TRUE(s3_comm_->exists("/repertory/test_r1.txt", []() -> std::string { return ""; }));
  EXPECT_FALSE(s3_comm_->exists("/repertory/test_r2.txt", []() -> std::string { return ""; }));
}

TEST_F(aws_s3_comm_test, create_bucket_and_remove_bucket) {
  auto ret = s3_comm_->create_bucket("/repertory2");
  EXPECT_EQ(api_error::success, ret);

  ret = s3_comm_->remove_bucket("/repertory2");
  EXPECT_EQ(api_error::success, ret);
}

TEST_F(aws_s3_comm_test, upload_file_encrypted) {
  const auto source_file_path = generate_test_file_name("./", "awscomm");
  auto file_size = 2u * utils::encryption::encrypting_reader::get_data_chunk_size() + 3u;
  auto source_file = create_random_file(source_file_path, file_size);

  auto stop_requested = false;
  std::string key;
  auto ret = s3_comm_->upload_file(
      "/repertory/test.txt", source_file_path, "test", []() -> std::string { return ""; },
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
    std::vector<char> data;
    ret = s3_comm_->read_file_bytes(
        "/repertory/test.txt",
        std::min(remain, utils::encryption::encrypting_reader::get_data_chunk_size()), offset, data,
        [&key]() -> std::string { return key; },
        [&file_size]() -> std::uint64_t { return file_size; },
        []() -> std::string { return "test"; }, stop_requested);
    EXPECT_EQ(api_error::success, ret);

    std::vector<char> data2(data.size());
    std::size_t bytes_read{};
    source_file->read_bytes(&data2[0u], data2.size(), offset, bytes_read);

    EXPECT_EQ(0, std::memcmp(&data2[0u], &data[0u], data2.size()));
    remain -= data.size();
    offset += data.size();
  }

  source_file->close();
  utils::file::delete_file(source_file_path);

  s3_comm_->remove_file("/repertory/test.txt", [&key]() -> std::string { return key; });
}
} // namespace repertory

#endif // REPERTORY_ENABLE_S3_TESTING
