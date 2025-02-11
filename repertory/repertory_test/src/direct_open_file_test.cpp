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
#include "test_common.hpp"

#include "file_manager/direct_open_file.hpp"
#include "mocks/mock_provider.hpp"

namespace {
constexpr const std::size_t test_chunk_size{1024U};
} // namespace

namespace repertory {
class direct_open_file_test : public ::testing::Test {
public:
  console_consumer con_consumer;
  mock_provider provider;

protected:
  void SetUp() override {
    event_system::instance().start();
  }

  void TearDown() override {
    event_system::instance().stop();
  }
};

TEST_F(direct_open_file_test, read_full_file) {
  auto &source_file = test::create_random_file(test_chunk_size * 32U);

  auto dest_path = test::generate_test_file_name("direct_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.directory = false;
  fsi.size = test_chunk_size * 32U;

  std::mutex read_mtx;
  EXPECT_CALL(provider, read_file_bytes)
      .WillRepeatedly([&read_mtx, &source_file](
                          const std::string & /* api_path */, std::size_t size,
                          std::uint64_t offset, data_buffer &data,
                          stop_type &stop_requested) -> api_error {
        mutex_lock lock(read_mtx);

        EXPECT_FALSE(stop_requested);
        std::size_t bytes_read{};
        data.resize(size);
        auto ret = source_file.read(data, offset, &bytes_read)
                       ? api_error::success
                       : api_error::os_error;
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      });
  {
    direct_open_file file(test_chunk_size, 30U, fsi, provider);

    auto dest_file = utils::file::file::open_or_create_file(dest_path);
    EXPECT_TRUE(dest_file);

    auto to_read{fsi.size};
    std::size_t chunk{0U};
    while (to_read > 0U) {
      data_buffer data{};
      EXPECT_EQ(api_error::success,
                file.read(test_chunk_size, chunk * test_chunk_size, data));

      std::size_t bytes_written{};
      EXPECT_TRUE(
          dest_file->write(data, chunk * test_chunk_size, &bytes_written));
      ++chunk;
      to_read -= data.size();
    }
    dest_file->close();
    source_file.close();

    auto hash1 = utils::file::file(source_file.get_path()).sha256();
    auto hash2 = utils::file::file(dest_path).sha256();

    EXPECT_TRUE(hash1.has_value());
    EXPECT_TRUE(hash2.has_value());
    if (hash1.has_value() && hash2.has_value()) {
      EXPECT_STREQ(hash1.value().c_str(), hash2.value().c_str());
    }
  }
}

TEST_F(direct_open_file_test, read_full_file_in_reverse) {
  auto &source_file = test::create_random_file(test_chunk_size * 32U);

  auto dest_path = test::generate_test_file_name("direct_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.directory = false;
  fsi.size = test_chunk_size * 32U;

  std::mutex read_mtx;
  EXPECT_CALL(provider, read_file_bytes)
      .WillRepeatedly([&read_mtx, &source_file](
                          const std::string & /* api_path */, std::size_t size,
                          std::uint64_t offset, data_buffer &data,
                          stop_type &stop_requested) -> api_error {
        mutex_lock lock(read_mtx);

        EXPECT_FALSE(stop_requested);
        std::size_t bytes_read{};
        data.resize(size);
        auto ret = source_file.read(data, offset, &bytes_read)
                       ? api_error::success
                       : api_error::os_error;
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      });
  {
    direct_open_file file(test_chunk_size, 30U, fsi, provider);

    auto dest_file = utils::file::file::open_or_create_file(dest_path);
    EXPECT_TRUE(dest_file);

    auto to_read{fsi.size};
    std::size_t chunk{file.get_total_chunks() - 1U};
    while (to_read > 0U) {
      data_buffer data{};
      EXPECT_EQ(api_error::success,
                file.read(test_chunk_size, chunk * test_chunk_size, data));

      std::size_t bytes_written{};
      EXPECT_TRUE(
          dest_file->write(data, chunk * test_chunk_size, &bytes_written));
      --chunk;
      to_read -= data.size();
    }
    dest_file->close();
    source_file.close();

    auto hash1 = utils::file::file(source_file.get_path()).sha256();
    auto hash2 = utils::file::file(dest_path).sha256();

    EXPECT_TRUE(hash1.has_value());
    EXPECT_TRUE(hash2.has_value());
    if (hash1.has_value() && hash2.has_value()) {
      EXPECT_STREQ(hash1.value().c_str(), hash2.value().c_str());
    }
  }
}

TEST_F(direct_open_file_test, read_full_file_in_partial_chunks) {
  auto &source_file = test::create_random_file(test_chunk_size * 32U);

  auto dest_path = test::generate_test_file_name("test");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32U;

  std::mutex read_mtx;
  EXPECT_CALL(provider, read_file_bytes)
      .WillRepeatedly([&read_mtx, &source_file](
                          const std::string & /* api_path */, std::size_t size,
                          std::uint64_t offset, data_buffer &data,
                          stop_type &stop_requested) -> api_error {
        mutex_lock lock(read_mtx);

        EXPECT_FALSE(stop_requested);
        std::size_t bytes_read{};
        data.resize(size);
        auto ret = source_file.read(data, offset, &bytes_read)
                       ? api_error::success
                       : api_error::os_error;
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      });
  {
    direct_open_file file(test_chunk_size, 30U, fsi, provider);

    auto dest_file = utils::file::file::open_or_create_file(dest_path);
    EXPECT_TRUE(dest_file);

    auto total_read{std::uint64_t(0U)};
    while (total_read < fsi.size) {
      data_buffer data{};
      EXPECT_EQ(api_error::success, file.read(3U, total_read, data));

      std::size_t bytes_written{};
      EXPECT_TRUE(dest_file->write(data.data(), data.size(), total_read,
                                   &bytes_written));
      total_read += data.size();
    }
    dest_file->close();
    source_file.close();

    auto hash1 = utils::file::file(source_file.get_path()).sha256();
    auto hash2 = utils::file::file(dest_path).sha256();

    EXPECT_TRUE(hash1.has_value());
    EXPECT_TRUE(hash2.has_value());
    if (hash1.has_value() && hash2.has_value()) {
      EXPECT_STREQ(hash1.value().c_str(), hash2.value().c_str());
    }
  }
}

TEST_F(direct_open_file_test, read_full_file_in_partial_chunks_in_reverse) {
  auto &source_file = test::create_random_file(test_chunk_size * 32U);

  auto dest_path = test::generate_test_file_name("direct_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32U;

  std::mutex read_mtx;
  EXPECT_CALL(provider, read_file_bytes)
      .WillRepeatedly([&read_mtx, &source_file](
                          const std::string & /* api_path */, std::size_t size,
                          std::uint64_t offset, data_buffer &data,
                          stop_type &stop_requested) -> api_error {
        mutex_lock lock(read_mtx);

        EXPECT_FALSE(stop_requested);
        std::size_t bytes_read{};
        data.resize(size);
        auto ret = source_file.read(data, offset, &bytes_read)
                       ? api_error::success
                       : api_error::os_error;
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      });
  {
    direct_open_file file(test_chunk_size, 30U, fsi, provider);

    auto dest_file = utils::file::file::open_or_create_file(dest_path);
    EXPECT_TRUE(dest_file);

    std::uint64_t total_read{0U};
    auto read_size{3U};

    while (total_read < fsi.size) {
      auto offset = fsi.size - total_read - read_size;
      auto remain = fsi.size - total_read;

      data_buffer data{};
      EXPECT_EQ(api_error::success,
                file.read(static_cast<std::size_t>(
                              std::min(remain, std::uint64_t(read_size))),
                          (remain >= read_size) ? offset : 0U, data));

      std::size_t bytes_written{};
      EXPECT_TRUE(dest_file->write(data, (remain >= read_size) ? offset : 0U,
                                   &bytes_written));
      total_read += data.size();
    }
    dest_file->close();
    source_file.close();

    auto hash1 = utils::file::file(source_file.get_path()).sha256();
    auto hash2 = utils::file::file(dest_path).sha256();

    EXPECT_TRUE(hash1.has_value());
    EXPECT_TRUE(hash2.has_value());
    if (hash1.has_value() && hash2.has_value()) {
      EXPECT_STREQ(hash1.value().c_str(), hash2.value().c_str());
    }
  }
}
} // namespace repertory
