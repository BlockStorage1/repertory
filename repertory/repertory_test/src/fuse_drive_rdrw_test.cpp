/*
  Copyright <2018-2025> <scott.e.graves@protonmail.com>

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
#if !defined(_WIN32)

#include "fixtures/drive_fixture.hpp"

namespace repertory {
TYPED_TEST_SUITE(fuse_test, platform_provider_types);

TYPED_TEST(fuse_test, rdrw_can_read_and_write_file) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);

  auto handle = open(file_path.c_str(), O_RDWR);
  ASSERT_GT(handle, -1);

  auto write_buffer = utils::generate_secure_random<data_buffer>(8096U);
  auto bytes_written =
      pwrite64(handle, write_buffer.data(), write_buffer.size(), 0U);
  EXPECT_EQ(write_buffer.size(), bytes_written);

  data_buffer read_buffer(write_buffer.size());
  auto bytes_read = pread64(handle, read_buffer.data(), read_buffer.size(), 0U);
  EXPECT_EQ(bytes_written, bytes_read);

  EXPECT_EQ(0, std::memcmp(write_buffer.data(), read_buffer.data(),
                           write_buffer.size()));
  close(handle);

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test, rdrw_can_read_from_offset) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);

  auto handle = open(file_path.c_str(), O_RDWR);
  ASSERT_GT(handle, -1);

  auto write_buffer = utils::generate_secure_random<data_buffer>(8096U);
  auto bytes_written =
      pwrite64(handle, write_buffer.data(), write_buffer.size(), 0U);
  EXPECT_EQ(write_buffer.size(), bytes_written);

  data_buffer read_buffer(1U);
  for (std::size_t idx = 0U; idx < write_buffer.size(); ++idx) {
    auto bytes_read = pread64(handle, read_buffer.data(), read_buffer.size(),
                              static_cast<off64_t>(idx));
    EXPECT_EQ(1U, bytes_read);

    EXPECT_EQ(write_buffer.at(idx), read_buffer.at(0U));
  }

  close(handle);

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test, rdrw_can_read_from_offset_after_eof) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);

  auto handle = open(file_path.c_str(), O_RDWR);
  ASSERT_GT(handle, -1);

  auto write_buffer = utils::generate_secure_random<data_buffer>(8096U);
  auto bytes_written =
      pwrite64(handle, write_buffer.data(), write_buffer.size(), 0U);
  EXPECT_EQ(write_buffer.size(), bytes_written);

  data_buffer read_buffer(1U);
  for (std::size_t idx = 0U; idx < write_buffer.size() + 1U; ++idx) {
    auto bytes_read = pread64(handle, read_buffer.data(), read_buffer.size(),
                              static_cast<off64_t>(idx));
    if (idx == write_buffer.size()) {
      EXPECT_EQ(0U, bytes_read);
    } else {
      EXPECT_EQ(1U, bytes_read);

      EXPECT_EQ(write_buffer.at(idx), read_buffer.at(0U));
    }
  }

  close(handle);

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test, rdrw_can_not_write_to_ro_file) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);

  auto handle = open(file_path.c_str(), O_RDONLY);
  ASSERT_GT(handle, -1);

  auto write_buffer = utils::generate_secure_random<data_buffer>(8096U);
  auto bytes_written =
      pwrite64(handle, write_buffer.data(), write_buffer.size(), 0U);
  EXPECT_EQ(-1, bytes_written);
  EXPECT_EQ(EBADF, errno);

  auto file_size = utils::file::file{file_path}.size();
  EXPECT_TRUE(file_size.has_value());
  EXPECT_EQ(0U, *file_size);

  close(handle);

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test, rdrw_can_not_read_from_wo_file) {
  std::string file_name{"create_test"};
  auto file_path = this->create_file_and_test(file_name);

  auto handle = open(file_path.c_str(), O_WRONLY);
  ASSERT_GT(handle, -1);

  auto write_buffer = utils::generate_secure_random<data_buffer>(8096U);
  auto bytes_written =
      pwrite64(handle, write_buffer.data(), write_buffer.size(), 0U);
  EXPECT_EQ(write_buffer.size(), bytes_written);

  data_buffer read_buffer(1U);
  auto bytes_read = pread64(handle, read_buffer.data(), read_buffer.size(), 0);
  EXPECT_EQ(-1, bytes_read);
  EXPECT_EQ(EBADF, errno);

  close(handle);

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test, rdrw_can_not_read_or_write_to_directory) {
  std::string dir_name{"create_test"};
  auto dir_path = this->create_directory_and_test(dir_name);

  auto handle = open(dir_path.c_str(), O_DIRECTORY);
  ASSERT_GT(handle, -1);

  auto write_buffer = utils::generate_secure_random<data_buffer>(8096U);
  auto bytes_written =
      pwrite64(handle, write_buffer.data(), write_buffer.size(), 0U);
  EXPECT_EQ(-1, bytes_written);
  EXPECT_EQ(EBADF, errno);

  data_buffer read_buffer(1U);
  auto bytes_read = pread64(handle, read_buffer.data(), read_buffer.size(), 0);
  EXPECT_EQ(-1, bytes_read);
  EXPECT_EQ(EISDIR, errno);

  close(handle);

  this->rmdir_and_test(dir_path);
}

TYPED_TEST(fuse_test, rdrw_can_append_to_file) {
  std::string file_name{"append_test"};
  auto file_path = this->create_file_and_test(file_name);

  auto handle = open(file_path.c_str(), O_WRONLY);
  ASSERT_GT(handle, -1);
  auto bytes_written = pwrite64(handle, "test_", 5U, 0);
  EXPECT_EQ(5U, bytes_written);
  close(handle);

  handle = open(file_path.c_str(), O_WRONLY | O_APPEND);
  ASSERT_GT(handle, -1);
  bytes_written = write(handle, "cow_", 4U);
  EXPECT_EQ(4U, bytes_written);
  close(handle);

  handle = open(file_path.c_str(), O_WRONLY | O_APPEND);
  ASSERT_GT(handle, -1);
  bytes_written = write(handle, "dog", 3U);
  EXPECT_EQ(3U, bytes_written);
  close(handle);

  handle = open(file_path.c_str(), O_RDONLY);
  ASSERT_GT(handle, -1);
  std::string read_buffer;
  read_buffer.resize(12U);
  auto bytes_read = pread64(handle, read_buffer.data(), read_buffer.size(), 0);
  EXPECT_EQ(12U, bytes_read);
  EXPECT_STREQ("test_cow_dog", read_buffer.c_str());
  close(handle);

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test, rdrw_open_with_o_trunc_resets_size) {
  std::string name{"trunc_test"};
  auto path = this->create_file_and_test(name, 0644);

  this->overwrite_text(path, "ABCDEFG");
  EXPECT_GT(this->stat_size(path), 0);

  auto desc = ::open(path.c_str(), O_WRONLY | O_TRUNC);
  ASSERT_NE(desc, -1);
  ::close(desc);

  EXPECT_EQ(0, this->stat_size(path));
  this->unlink_file_and_test(path);
}

TYPED_TEST(fuse_test, rdrw_o_append_writes_at_eof) {
  std::string name{"append_test"};
  auto path = this->create_file_and_test(name, 0644);

  this->overwrite_text(path, "HEAD");

  auto desc = ::open(path.c_str(), O_WRONLY | O_APPEND);
  ASSERT_NE(desc, -1);

  ASSERT_NE(-1, ::lseek(desc, 0, SEEK_SET));
  this->write_all(desc, "TAIL");
  ::close(desc);

  EXPECT_EQ("HEADTAIL", this->slurp(path));
  this->unlink_file_and_test(path);
}
} // namespace repertory

#endif // !defined(_WIN32)
