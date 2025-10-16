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

#include "file_manager/ring_buffer_open_file.hpp"
#include "mocks/mock_provider.hpp"
#include "mocks/mock_upload_manager.hpp"
#include "platform/platform.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"

namespace {
constexpr std::size_t test_chunk_size{1024U};

const auto ring_buffer_dir{
    repertory::utils::path::combine(
        repertory::test::get_test_output_dir(),
        {"file_manager_ring_buffer_open_file_test"}),
};
} // namespace

namespace repertory {
class ring_buffer_open_file_test : public ::testing::Test {
public:
  console_consumer con_consumer;
  mock_provider provider;

protected:
  void SetUp() override { event_system::instance().start(); }

  void TearDown() override { event_system::instance().stop(); }
};

TEST_F(ring_buffer_open_file_test, can_forward_to_last_chunk) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);

    file.set(0U, 3U);
    file.forward(4U);

    EXPECT_EQ(std::size_t(7U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(3U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(10U), file.get_last_chunk());

    for (std::size_t chunk = 3U; chunk <= 7U; ++chunk) {
      EXPECT_TRUE(file.get_read_state(chunk));
    }

    for (std::size_t chunk = 8U; chunk <= 10U; ++chunk) {
      EXPECT_FALSE(file.get_read_state(chunk));
    }
  }
}

TEST_F(ring_buffer_open_file_test,
       can_forward_to_last_chunk_if_count_is_greater_than_remaining) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);
    file.set(0U, 3U);
    file.forward(100U);

    EXPECT_EQ(std::size_t(15U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(8U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(15U), file.get_last_chunk());
    for (std::size_t chunk = 8U; chunk <= 15U; chunk++) {
      EXPECT_FALSE(file.get_read_state(chunk));
    }
  }
}

TEST_F(ring_buffer_open_file_test, can_forward_after_last_chunk) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);

    file.set(0U, 3U);
    file.forward(5U);

    EXPECT_EQ(std::size_t(8U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(4U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(11U), file.get_last_chunk());

    EXPECT_TRUE(file.get_read_state(4U));
    EXPECT_TRUE(file.get_read_state(5U));
    EXPECT_TRUE(file.get_read_state(6U));
    EXPECT_TRUE(file.get_read_state(7U));
    EXPECT_FALSE(file.get_read_state(8U));
    EXPECT_FALSE(file.get_read_state(9U));
    EXPECT_FALSE(file.get_read_state(10U));
    EXPECT_FALSE(file.get_read_state(11U));
  }
}

TEST_F(ring_buffer_open_file_test, can_forward_and_rollover_after_last_chunk) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);

    file.set(16U, 20U);
    file.forward(8U);

    EXPECT_EQ(std::size_t(28U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(24U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(31U), file.get_last_chunk());
  }
}

TEST_F(ring_buffer_open_file_test, can_reverse_to_first_chunk) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);
    file.set(0U, 3U);
    file.reverse(3U);

    EXPECT_EQ(std::size_t(0U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(0U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(7U), file.get_last_chunk());
    for (std::size_t chunk = 0U; chunk < 8U; chunk++) {
      EXPECT_TRUE(file.get_read_state(chunk));
    }
  }
}

TEST_F(ring_buffer_open_file_test,
       can_reverse_to_first_chunk_if_count_is_greater_than_remaining) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);
    file.set(0U, 3U);
    file.reverse(13U);

    EXPECT_EQ(std::size_t(0U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(0U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(7U), file.get_last_chunk());
    for (std::size_t chunk = 0U; chunk < 8U; chunk++) {
      EXPECT_TRUE(file.get_read_state(chunk));
    }
  }
}

TEST_F(ring_buffer_open_file_test, can_reverse_before_first_chunk) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);
    file.set(1U, 3U);
    file.reverse(3U);

    EXPECT_EQ(std::size_t(0U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(0U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(7U), file.get_last_chunk());
    EXPECT_FALSE(file.get_read_state(0U));
    for (std::size_t chunk = 1U; chunk < 8U; chunk++) {
      EXPECT_TRUE(file.get_read_state(chunk));
    }
  }
}

TEST_F(ring_buffer_open_file_test,
       can_reverse_and_rollover_before_first_chunk) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);
    file.set(16U, 20U);
    file.reverse(8U);

    EXPECT_EQ(std::size_t(12U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(12U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(19U), file.get_last_chunk());

    EXPECT_FALSE(file.get_read_state(12U));
    EXPECT_FALSE(file.get_read_state(13U));
    EXPECT_FALSE(file.get_read_state(14U));
    EXPECT_FALSE(file.get_read_state(15U));
    for (std::size_t chunk = 16U; chunk <= file.get_last_chunk(); chunk++) {
      EXPECT_TRUE(file.get_read_state(chunk));
    }
  }
}

TEST_F(ring_buffer_open_file_test, can_reverse_full_ring) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);
    file.set(8U, 15U);
    file.reverse(16U);

    EXPECT_EQ(std::size_t(0U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(0U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(7U), file.get_last_chunk());

    for (std::size_t chunk = 0U; chunk <= file.get_last_chunk(); chunk++) {
      EXPECT_FALSE(file.get_read_state(chunk));
    }
  }
}

TEST_F(ring_buffer_open_file_test, read_full_file) {
  auto &nf = test::create_random_file(test_chunk_size * 33u + 11u);
  auto download_source_path = nf.get_path();

  auto dest_path = test::generate_test_file_name("ring_buffer_open_file");

  mock_provider mp;

  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 33u + 11u;
  fsi.source_path = test::generate_test_file_name("ring_buffer_open_file");

  std::mutex read_mtx;
  EXPECT_CALL(mp, read_file_bytes)
      .WillRepeatedly([&read_mtx, &nf](std::string_view /* api_path */,
                                       std::size_t size, std::uint64_t offset,
                                       data_buffer &data,
                                       stop_type &stop_requested) -> api_error {
        mutex_lock lock(read_mtx);

        EXPECT_FALSE(stop_requested);
        std::size_t bytes_read{};
        data.resize(size);
        auto ret = nf.read(data, offset, &bytes_read) ? api_error::success
                                                      : api_error::os_error;
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      });
  {
    ring_buffer_open_file rb(ring_buffer_dir, test_chunk_size, 30U, fsi, mp,
                             8u);

    auto ptr = utils::file::file::open_or_create_file(dest_path);
    auto &nf2 = *ptr;
    EXPECT_TRUE(nf2);

    auto to_read = fsi.size;
    std::size_t chunk = 0u;
    while (to_read) {
      data_buffer data{};
      EXPECT_EQ(api_error::success,
                rb.read(test_chunk_size, chunk * test_chunk_size, data));

      std::size_t bytes_written{};
      EXPECT_TRUE(nf2.write(data, chunk * test_chunk_size, &bytes_written));
      chunk++;
      to_read -= data.size();
    }
    nf2.close();
    nf.close();

    auto hash1 = utils::file::file(download_source_path).sha256();
    auto hash2 = utils::file::file(dest_path).sha256();

    EXPECT_TRUE(hash1.has_value());
    EXPECT_TRUE(hash2.has_value());
    if (hash1.has_value() && hash2.has_value()) {
      EXPECT_STREQ(hash1.value().c_str(), hash2.value().c_str());
    }
  }
}

TEST_F(ring_buffer_open_file_test, read_full_file_in_reverse) {
  auto &nf = test::create_random_file(test_chunk_size * 32u);
  auto download_source_path = nf.get_path();

  auto dest_path = test::generate_test_file_name("ring_buffer_open_file");

  mock_provider mp;

  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32u;
  fsi.source_path = test::generate_test_file_name("ring_buffer_open_file");

  std::mutex read_mtx;
  EXPECT_CALL(mp, read_file_bytes)
      .WillRepeatedly([&read_mtx, &nf](std::string_view /* api_path */,
                                       std::size_t size, std::uint64_t offset,
                                       data_buffer &data,
                                       stop_type &stop_requested) -> api_error {
        mutex_lock lock(read_mtx);

        EXPECT_FALSE(stop_requested);
        std::size_t bytes_read{};
        data.resize(size);
        auto ret = nf.read(data, offset, &bytes_read) ? api_error::success
                                                      : api_error::os_error;
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      });
  {
    ring_buffer_open_file rb(ring_buffer_dir, test_chunk_size, 30U, fsi, mp,
                             8u);

    auto ptr = utils::file::file::open_or_create_file(dest_path);
    auto &nf2 = *ptr;
    EXPECT_TRUE(nf2);

    auto to_read = fsi.size;
    std::size_t chunk = rb.get_total_chunks() - 1U;
    while (to_read > 0U) {
      data_buffer data{};
      EXPECT_EQ(api_error::success,
                rb.read(test_chunk_size, chunk * test_chunk_size, data));

      std::size_t bytes_written{};
      EXPECT_TRUE(nf2.write(data, chunk * test_chunk_size, &bytes_written));
      --chunk;
      to_read -= data.size();
    }
    nf2.close();
    nf.close();

    auto hash1 = utils::file::file(download_source_path).sha256();
    auto hash2 = utils::file::file(dest_path).sha256();

    EXPECT_TRUE(hash1.has_value());
    EXPECT_TRUE(hash2.has_value());
    if (hash1.has_value() && hash2.has_value()) {
      EXPECT_STREQ(hash1.value().c_str(), hash2.value().c_str());
    }
  }
}

TEST_F(ring_buffer_open_file_test, read_full_file_in_partial_chunks) {
  auto &nf = test::create_random_file(test_chunk_size * 32u);
  auto download_source_path = nf.get_path();

  auto dest_path = test::generate_test_file_name("test");

  mock_provider mp;

  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32u;
  fsi.source_path = test::generate_test_file_name("test");

  std::mutex read_mtx;
  EXPECT_CALL(mp, read_file_bytes)
      .WillRepeatedly([&read_mtx, &nf](std::string_view /* api_path */,
                                       std::size_t size, std::uint64_t offset,
                                       data_buffer &data,
                                       stop_type &stop_requested) -> api_error {
        mutex_lock lock(read_mtx);

        EXPECT_FALSE(stop_requested);
        std::size_t bytes_read{};
        data.resize(size);
        auto ret = nf.read(data, offset, &bytes_read) ? api_error::success
                                                      : api_error::os_error;
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      });
  {
    ring_buffer_open_file rb(ring_buffer_dir, test_chunk_size, 30U, fsi, mp,
                             8u);

    auto ptr = utils::file::file::open_or_create_file(dest_path);
    auto &nf2 = *ptr;
    EXPECT_TRUE(nf2);
    // EXPECT_EQ(api_error::success, native_file::create_or_open(dest_path,
    // nf2));

    auto total_read = std::uint64_t(0u);

    while (total_read < fsi.size) {
      data_buffer data{};
      EXPECT_EQ(api_error::success, rb.read(3u, total_read, data));

      std::size_t bytes_written{};
      EXPECT_TRUE(
          nf2.write(data.data(), data.size(), total_read, &bytes_written));
      total_read += data.size();
    }
    nf2.close();
    nf.close();

    auto hash1 = utils::file::file(download_source_path).sha256();
    auto hash2 = utils::file::file(dest_path).sha256();

    EXPECT_TRUE(hash1.has_value());
    EXPECT_TRUE(hash2.has_value());
    if (hash1.has_value() && hash2.has_value()) {
      EXPECT_STREQ(hash1.value().c_str(), hash2.value().c_str());
    }
  }
}

TEST_F(ring_buffer_open_file_test,
       read_full_file_in_partial_chunks_in_reverse) {
  auto &nf = test::create_random_file(test_chunk_size * 32u);
  auto download_source_path = nf.get_path();

  auto dest_path = test::generate_test_file_name("ring_buffer_open_file");

  mock_provider mp;

  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32u;
  fsi.source_path = test::generate_test_file_name("ring_buffer_open_file");

  std::mutex read_mtx;
  EXPECT_CALL(mp, read_file_bytes)
      .WillRepeatedly([&read_mtx, &nf](std::string_view /* api_path */,
                                       std::size_t size, std::uint64_t offset,
                                       data_buffer &data,
                                       stop_type &stop_requested) -> api_error {
        mutex_lock lock(read_mtx);

        EXPECT_FALSE(stop_requested);
        std::size_t bytes_read{};
        data.resize(size);
        auto ret = nf.read(data, offset, &bytes_read) ? api_error::success
                                                      : api_error::os_error;
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      });
  {
    ring_buffer_open_file rb(ring_buffer_dir, test_chunk_size, 30U, fsi, mp,
                             8u);

    auto ptr = utils::file::file::open_or_create_file(dest_path);
    auto &nf2 = *ptr;
    EXPECT_TRUE(nf2);

    std::uint64_t total_read{0U};
    auto read_size{3U};

    while (total_read < fsi.size) {
      auto offset = fsi.size - total_read - read_size;
      auto remain = fsi.size - total_read;

      data_buffer data{};
      EXPECT_EQ(api_error::success,
                rb.read(static_cast<std::size_t>(
                            std::min(remain, std::uint64_t(read_size))),
                        (remain >= read_size) ? offset : 0u, data));

      std::size_t bytes_written{};
      EXPECT_TRUE(
          nf2.write(data, (remain >= read_size) ? offset : 0u, &bytes_written));
      total_read += data.size();
    }
    nf2.close();
    nf.close();

    auto hash1 = utils::file::file(download_source_path).sha256();
    auto hash2 = utils::file::file(dest_path).sha256();

    EXPECT_TRUE(hash1.has_value());
    EXPECT_TRUE(hash2.has_value());
    if (hash1.has_value() && hash2.has_value()) {
      EXPECT_STREQ(hash1.value().c_str(), hash2.value().c_str());
    }
  }
}

TEST_F(ring_buffer_open_file_test, forward_center_noop_when_within_half) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");
  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);

    file.set(0U, 3U);
    file.forward(1U);

    EXPECT_EQ(std::size_t(4U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(0U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(7U), file.get_last_chunk());

    for (std::size_t chunk = 0U; chunk <= 7U; ++chunk) {
      EXPECT_TRUE(file.get_read_state(chunk)) << "chunk " << chunk;
    }
  }
}

TEST_F(ring_buffer_open_file_test, forward_center_caps_at_file_end) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");
  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);

    file.set(6U, 9U);
    file.forward(100U);

    EXPECT_EQ(std::size_t(15U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(8U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(15U), file.get_last_chunk());
  }
}

TEST_F(ring_buffer_open_file_test,
       forward_delta_ge_ring_triggers_full_invalidation_path) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");
  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);

    file.set(0U, 0U);
    file.forward(100U);

    EXPECT_EQ(std::size_t(15U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(8U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(15U), file.get_last_chunk());

    for (std::size_t chunk = 8U; chunk <= 15U; ++chunk) {
      EXPECT_FALSE(file.get_read_state(chunk)) << "chunk " << chunk;
    }
  }
}

TEST_F(ring_buffer_open_file_test,
       forward_center_marks_only_tail_entrants_unread) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");
  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);

    file.set(0U, 3U);
    file.forward(2U);

    EXPECT_EQ(std::size_t(5U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(1U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(8U), file.get_last_chunk());

    for (std::size_t chunk = 1U; chunk <= 7U; ++chunk) {
      EXPECT_TRUE(file.get_read_state(chunk)) << "chunk " << chunk;
    }
    EXPECT_FALSE(file.get_read_state(8U));
  }
}

TEST_F(ring_buffer_open_file_test, reverse_does_not_trigger_forward_centering) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");
  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);

    file.set(8U, 12U);
    file.reverse(1U);

    EXPECT_EQ(std::size_t(11U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(8U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(15U), file.get_last_chunk());
  }
}

TEST_F(ring_buffer_open_file_test,
       forward_minimal_slide_then_center_multi_step) {
  auto source_path = test::generate_test_file_name("ring_buffer_open_file");
  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32U;
  fsi.source_path = source_path;

  {
    ring_buffer_open_file file(ring_buffer_dir, test_chunk_size, 30U, fsi,
                               provider, 8U);

    file.set(0U, 3U);
    file.forward(7U);

    EXPECT_EQ(std::size_t(10U), file.get_current_chunk());
    EXPECT_EQ(std::size_t(6U), file.get_first_chunk());
    EXPECT_EQ(std::size_t(13U), file.get_last_chunk());

    EXPECT_FALSE(file.get_read_state(11U));
    EXPECT_FALSE(file.get_read_state(12U));
    EXPECT_FALSE(file.get_read_state(13U));
  }
}
} // namespace repertory
