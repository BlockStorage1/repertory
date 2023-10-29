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

#include "file_manager/file_manager.hpp"
#include "mocks/mock_provider.hpp"
#include "mocks/mock_upload_manager.hpp"
#include "utils/file_utils.hpp"
#include "utils/unix/unix_utils.hpp"

namespace repertory {
static constexpr const std::size_t test_chunk_size = 1024u;

TEST(ring_buffer_open_file, can_forward_to_last_chunk) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16u;
  fsi.source_path = source_path;

  {
    file_manager::ring_buffer_open_file rb("./ring_buffer_directory",
                                           test_chunk_size, 30U, fsi, mp, 8u);
    rb.set(0u, 3u);
    rb.forward(4u);

    EXPECT_EQ(std::size_t(7u), rb.get_current_chunk());
    EXPECT_EQ(std::size_t(0u), rb.get_first_chunk());
    EXPECT_EQ(std::size_t(7u), rb.get_last_chunk());
    for (std::size_t chunk = 0u; chunk < 8u; chunk++) {
      EXPECT_TRUE(rb.get_read_state(chunk));
    }
  }

  EXPECT_TRUE(
      utils::file::delete_directory_recursively("./ring_buffer_directory"));
}

TEST(ring_buffer_open_file,
     can_forward_to_last_chunk_if_count_is_greater_than_remaining) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16u;
  fsi.source_path = source_path;

  {
    file_manager::ring_buffer_open_file rb("./ring_buffer_directory",
                                           test_chunk_size, 30U, fsi, mp, 8u);
    rb.set(0u, 3u);
    rb.forward(100u);

    EXPECT_EQ(std::size_t(15u), rb.get_current_chunk());
    EXPECT_EQ(std::size_t(8u), rb.get_first_chunk());
    EXPECT_EQ(std::size_t(15u), rb.get_last_chunk());
    for (std::size_t chunk = 8u; chunk <= 15u; chunk++) {
      EXPECT_FALSE(rb.get_read_state(chunk));
    }
  }

  EXPECT_TRUE(
      utils::file::delete_directory_recursively("./ring_buffer_directory"));
}

TEST(ring_buffer_open_file, can_forward_after_last_chunk) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16u;
  fsi.source_path = source_path;

  {
    file_manager::ring_buffer_open_file rb("./ring_buffer_directory",
                                           test_chunk_size, 30U, fsi, mp, 8u);
    rb.set(0u, 3u);
    rb.forward(5u);

    EXPECT_EQ(std::size_t(8u), rb.get_current_chunk());
    EXPECT_EQ(std::size_t(1u), rb.get_first_chunk());
    EXPECT_EQ(std::size_t(8u), rb.get_last_chunk());
    EXPECT_FALSE(rb.get_read_state(8u));
    for (std::size_t chunk = 1u; chunk < 8u; chunk++) {
      EXPECT_TRUE(rb.get_read_state(chunk));
    }
  }

  EXPECT_TRUE(
      utils::file::delete_directory_recursively("./ring_buffer_directory"));
}

TEST(ring_buffer_open_file, can_forward_and_rollover_after_last_chunk) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32u;
  fsi.source_path = source_path;

  {
    file_manager::ring_buffer_open_file rb("./ring_buffer_directory",
                                           test_chunk_size, 30U, fsi, mp, 8u);
    rb.set(16u, 20u);
    rb.forward(8u);

    EXPECT_EQ(std::size_t(28u), rb.get_current_chunk());
    EXPECT_EQ(std::size_t(21u), rb.get_first_chunk());
    EXPECT_EQ(std::size_t(28u), rb.get_last_chunk());
  }

  EXPECT_TRUE(
      utils::file::delete_directory_recursively("./ring_buffer_directory"));
}

TEST(ring_buffer_open_file, can_reverse_to_first_chunk) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16u;
  fsi.source_path = source_path;

  {
    file_manager::ring_buffer_open_file rb("./ring_buffer_directory",
                                           test_chunk_size, 30U, fsi, mp, 8u);
    rb.set(0u, 3u);
    rb.reverse(3u);

    EXPECT_EQ(std::size_t(0u), rb.get_current_chunk());
    EXPECT_EQ(std::size_t(0u), rb.get_first_chunk());
    EXPECT_EQ(std::size_t(7u), rb.get_last_chunk());
    for (std::size_t chunk = 0u; chunk < 8u; chunk++) {
      EXPECT_TRUE(rb.get_read_state(chunk));
    }
  }

  EXPECT_TRUE(
      utils::file::delete_directory_recursively("./ring_buffer_directory"));
}

TEST(ring_buffer_open_file,
     can_reverse_to_first_chunk_if_count_is_greater_than_remaining) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16u;
  fsi.source_path = source_path;

  {
    file_manager::ring_buffer_open_file rb("./ring_buffer_directory",
                                           test_chunk_size, 30U, fsi, mp, 8u);
    rb.set(0u, 3u);
    rb.reverse(13u);

    EXPECT_EQ(std::size_t(0u), rb.get_current_chunk());
    EXPECT_EQ(std::size_t(0u), rb.get_first_chunk());
    EXPECT_EQ(std::size_t(7u), rb.get_last_chunk());
    for (std::size_t chunk = 0u; chunk < 8u; chunk++) {
      EXPECT_TRUE(rb.get_read_state(chunk));
    }
  }

  EXPECT_TRUE(
      utils::file::delete_directory_recursively("./ring_buffer_directory"));
}

TEST(ring_buffer_open_file, can_reverse_before_first_chunk) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 16u;
  fsi.source_path = source_path;

  {
    file_manager::ring_buffer_open_file rb("./ring_buffer_directory",
                                           test_chunk_size, 30U, fsi, mp, 8u);
    rb.set(1u, 3u);
    rb.reverse(3u);

    EXPECT_EQ(std::size_t(0u), rb.get_current_chunk());
    EXPECT_EQ(std::size_t(0u), rb.get_first_chunk());
    EXPECT_EQ(std::size_t(7u), rb.get_last_chunk());
    EXPECT_FALSE(rb.get_read_state(0u));
    for (std::size_t chunk = 1u; chunk < 8u; chunk++) {
      EXPECT_TRUE(rb.get_read_state(chunk));
    }
  }

  EXPECT_TRUE(
      utils::file::delete_directory_recursively("./ring_buffer_directory"));
}

TEST(ring_buffer_open_file, can_reverse_and_rollover_before_first_chunk) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32u;
  fsi.source_path = source_path;

  {
    file_manager::ring_buffer_open_file rb("./ring_buffer_directory",
                                           test_chunk_size, 30U, fsi, mp, 8u);
    rb.set(16u, 20u);
    rb.reverse(8u);

    EXPECT_EQ(std::size_t(12u), rb.get_current_chunk());
    EXPECT_EQ(std::size_t(12u), rb.get_first_chunk());
    EXPECT_EQ(std::size_t(19u), rb.get_last_chunk());

    EXPECT_FALSE(rb.get_read_state(12u));
    EXPECT_FALSE(rb.get_read_state(13u));
    EXPECT_FALSE(rb.get_read_state(14u));
    EXPECT_FALSE(rb.get_read_state(15u));
    for (std::size_t chunk = 16u; chunk <= rb.get_last_chunk(); chunk++) {
      EXPECT_TRUE(rb.get_read_state(chunk));
    }
  }

  EXPECT_TRUE(
      utils::file::delete_directory_recursively("./ring_buffer_directory"));
}

TEST(ring_buffer_open_file, can_reverse_full_ring) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32u;
  fsi.source_path = source_path;

  {
    file_manager::ring_buffer_open_file rb("./ring_buffer_directory",
                                           test_chunk_size, 30U, fsi, mp, 8u);
    rb.set(8u, 15u);
    rb.reverse(16u);

    EXPECT_EQ(std::size_t(0u), rb.get_current_chunk());
    EXPECT_EQ(std::size_t(0u), rb.get_first_chunk());
    EXPECT_EQ(std::size_t(7u), rb.get_last_chunk());

    for (std::size_t chunk = 0u; chunk <= rb.get_last_chunk(); chunk++) {
      EXPECT_FALSE(rb.get_read_state(chunk));
    }
  }

  EXPECT_TRUE(
      utils::file::delete_directory_recursively("./ring_buffer_directory"));
}

TEST(ring_buffer_open_file, read_full_file) {
  const auto download_source_path = generate_test_file_name(".", "test");
  auto nf = create_random_file(download_source_path, test_chunk_size * 32u);

  const auto dest_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32u;
  fsi.source_path = generate_test_file_name(".", "test");

  EXPECT_CALL(mp, read_file_bytes)
      .WillRepeatedly([&nf](const std::string & /* api_path */,
                            std::size_t size, std::uint64_t offset,
                            data_buffer &data,
                            stop_type &stop_requested) -> api_error {
        EXPECT_FALSE(stop_requested);
        std::size_t bytes_read{};
        data.resize(size);
        auto ret = nf->read_bytes(&data[0u], size, offset, bytes_read)
                       ? api_error::success
                       : api_error::os_error;
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      });
  {
    file_manager::ring_buffer_open_file rb("./ring_buffer_directory",
                                           test_chunk_size, 30U, fsi, mp, 8u);

    native_file_ptr nf2;
    EXPECT_EQ(api_error::success, native_file::create_or_open(dest_path, nf2));

    auto to_read = fsi.size;
    std::size_t chunk = 0u;
    while (to_read) {
      data_buffer data{};
      EXPECT_EQ(api_error::success,
                rb.read(test_chunk_size, chunk * test_chunk_size, data));

      std::size_t bytes_written{};
      EXPECT_TRUE(nf2->write_bytes(data.data(), data.size(),
                                   chunk * test_chunk_size, bytes_written));
      chunk++;
      to_read -= data.size();
    }
    nf2->close();
    nf->close();

    EXPECT_STREQ(utils::file::generate_sha256(download_source_path).c_str(),
                 utils::file::generate_sha256(dest_path).c_str());
  }

  EXPECT_TRUE(
      utils::file::delete_directory_recursively("./ring_buffer_directory"));
}

TEST(ring_buffer_open_file, read_full_file_in_reverse) {
  const auto download_source_path = generate_test_file_name(".", "test");
  auto nf = create_random_file(download_source_path, test_chunk_size * 32u);

  const auto dest_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32u;
  fsi.source_path = generate_test_file_name(".", "test");

  EXPECT_CALL(mp, read_file_bytes)
      .WillRepeatedly([&nf](const std::string & /* api_path */,
                            std::size_t size, std::uint64_t offset,
                            data_buffer &data,
                            stop_type &stop_requested) -> api_error {
        EXPECT_FALSE(stop_requested);
        std::size_t bytes_read{};
        data.resize(size);
        auto ret = nf->read_bytes(&data[0u], size, offset, bytes_read)
                       ? api_error::success
                       : api_error::os_error;
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      });
  {
    file_manager::ring_buffer_open_file rb("./ring_buffer_directory",
                                           test_chunk_size, 30U, fsi, mp, 8u);

    native_file_ptr nf2;
    EXPECT_EQ(api_error::success, native_file::create_or_open(dest_path, nf2));

    auto to_read = fsi.size;
    std::size_t chunk = rb.get_total_chunks() - 1u;
    while (to_read) {
      data_buffer data{};
      EXPECT_EQ(api_error::success,
                rb.read(test_chunk_size, chunk * test_chunk_size, data));

      std::size_t bytes_written{};
      EXPECT_TRUE(nf2->write_bytes(data.data(), data.size(),
                                   chunk * test_chunk_size, bytes_written));
      chunk--;
      to_read -= data.size();
    }
    nf2->close();
    nf->close();

    EXPECT_STREQ(utils::file::generate_sha256(download_source_path).c_str(),
                 utils::file::generate_sha256(dest_path).c_str());
  }

  EXPECT_TRUE(
      utils::file::delete_directory_recursively("./ring_buffer_directory"));
}

TEST(ring_buffer_open_file, read_full_file_in_partial_chunks) {
  const auto download_source_path = generate_test_file_name(".", "test");
  auto nf = create_random_file(download_source_path, test_chunk_size * 32u);

  const auto dest_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32u;
  fsi.source_path = generate_test_file_name(".", "test");

  EXPECT_CALL(mp, read_file_bytes)
      .WillRepeatedly([&nf](const std::string & /* api_path */,
                            std::size_t size, std::uint64_t offset,
                            data_buffer &data,
                            stop_type &stop_requested) -> api_error {
        EXPECT_FALSE(stop_requested);
        std::size_t bytes_read{};
        data.resize(size);
        auto ret = nf->read_bytes(&data[0u], size, offset, bytes_read)
                       ? api_error::success
                       : api_error::os_error;
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      });
  {
    file_manager::ring_buffer_open_file rb("./ring_buffer_directory",
                                           test_chunk_size, 30U, fsi, mp, 8u);

    native_file_ptr nf2;
    EXPECT_EQ(api_error::success, native_file::create_or_open(dest_path, nf2));

    auto total_read = std::uint64_t(0u);

    while (total_read < fsi.size) {
      data_buffer data{};
      EXPECT_EQ(api_error::success, rb.read(3u, total_read, data));

      std::size_t bytes_written{};
      EXPECT_TRUE(nf2->write_bytes(data.data(), data.size(), total_read,
                                   bytes_written));
      total_read += data.size();
    }
    nf2->close();
    nf->close();

    EXPECT_STREQ(utils::file::generate_sha256(download_source_path).c_str(),
                 utils::file::generate_sha256(dest_path).c_str());
  }

  EXPECT_TRUE(
      utils::file::delete_directory_recursively("./ring_buffer_directory"));
}

TEST(ring_buffer_open_file, read_full_file_in_partial_chunks_in_reverse) {
  const auto download_source_path = generate_test_file_name(".", "test");
  auto nf = create_random_file(download_source_path, test_chunk_size * 32u);

  const auto dest_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 32u;
  fsi.source_path = generate_test_file_name(".", "test");

  EXPECT_CALL(mp, read_file_bytes)
      .WillRepeatedly([&nf](const std::string & /* api_path */,
                            std::size_t size, std::uint64_t offset,
                            data_buffer &data,
                            stop_type &stop_requested) -> api_error {
        EXPECT_FALSE(stop_requested);
        std::size_t bytes_read{};
        data.resize(size);
        auto ret = nf->read_bytes(&data[0u], size, offset, bytes_read)
                       ? api_error::success
                       : api_error::os_error;
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      });
  {
    file_manager::ring_buffer_open_file rb("./ring_buffer_directory",
                                           test_chunk_size, 30U, fsi, mp, 8u);

    native_file_ptr nf2;
    EXPECT_EQ(api_error::success, native_file::create_or_open(dest_path, nf2));

    auto total_read = std::uint64_t(0u);
    const auto read_size = 3u;

    while (total_read < fsi.size) {
      const auto offset = fsi.size - total_read - read_size;
      const auto remain = fsi.size - total_read;

      data_buffer data{};
      EXPECT_EQ(api_error::success,
                rb.read(static_cast<std::size_t>(
                            std::min(remain, std::uint64_t(read_size))),
                        (remain >= read_size) ? offset : 0u, data));

      std::size_t bytes_written{};
      EXPECT_TRUE(nf2->write_bytes(data.data(), data.size(),
                                   (remain >= read_size) ? offset : 0u,
                                   bytes_written));
      total_read += data.size();
    }
    nf2->close();
    nf->close();

    EXPECT_STREQ(utils::file::generate_sha256(download_source_path).c_str(),
                 utils::file::generate_sha256(dest_path).c_str());
  }

  EXPECT_TRUE(
      utils::file::delete_directory_recursively("./ring_buffer_directory"));
}
} // namespace repertory
