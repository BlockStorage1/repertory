/*
  Copyright <2018-2025> <scott.e.graves@protonmail.com>
  ...
*/
#include "test_common.hpp"

#include "file_manager/direct_open_file.hpp"
#include "mocks/mock_provider.hpp"

namespace {
constexpr std::size_t test_chunk_size{1024U};
} // namespace

namespace repertory {
class direct_open_file_test : public ::testing::Test {
public:
  console_consumer con_consumer;
  mock_provider provider;

protected:
  void SetUp() override { event_system::instance().start(); }
  void TearDown() override { event_system::instance().stop(); }
};

namespace {
[[nodiscard]] auto build_test_buffer(repertory::utils::file::i_file &file)
    -> repertory::data_buffer {
  repertory::data_buffer buf;
  EXPECT_TRUE(file.read_all(buf, 0U));
  return buf;
}
} // namespace

TEST_F(direct_open_file_test, read_full_file) {
  auto &source_file = test::create_random_file(test_chunk_size * 32U);
  auto dest_path = test::generate_test_file_name("direct_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.directory = false;
  fsi.size = test_chunk_size * 32U;

  auto test_buffer = build_test_buffer(source_file);

  EXPECT_CALL(provider, read_file_bytes)
      .WillRepeatedly([test_buffer](std::string_view, std::size_t size,
                                    std::uint64_t offset, data_buffer &data,
                                    stop_type &stop_requested) -> api_error {
        if (stop_requested) {
          return api_error::download_stopped;
        }
        std::size_t avail =
            (offset < test_buffer.size())
                ? (test_buffer.size() - static_cast<std::size_t>(offset))
                : 0U;
        std::size_t to_copy = std::min(size, avail);
        data.resize(to_copy);
        if (to_copy != 0U) {
          std::memcpy(data.data(),
                      test_buffer.data() + static_cast<std::size_t>(offset),
                      to_copy);
        }
        return api_error::success;
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

    auto hash1 = utils::file::file(source_file.get_path()).sha256();
    auto hash2 = utils::file::file(dest_path).sha256();
    EXPECT_TRUE(hash1.has_value());
    EXPECT_TRUE(hash2.has_value());
    if (hash1.has_value() && hash2.has_value()) {
      EXPECT_STREQ(hash1.value().c_str(), hash2.value().c_str());
    }
  }

  source_file.close();
}

TEST_F(direct_open_file_test, read_full_file_in_reverse) {
  auto &source_file = test::create_random_file(test_chunk_size * 32U);
  auto dest_path = test::generate_test_file_name("direct_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.directory = false;
  fsi.size = test_chunk_size * 32U;

  auto test_buffer = build_test_buffer(source_file);

  EXPECT_CALL(provider, read_file_bytes)
      .WillRepeatedly([test_buffer](std::string_view, std::size_t size,
                                    std::uint64_t offset, data_buffer &data,
                                    stop_type &stop_requested) -> api_error {
        if (stop_requested) {
          return api_error::download_stopped;
        }
        std::size_t avail =
            (offset < test_buffer.size())
                ? (test_buffer.size() - static_cast<std::size_t>(offset))
                : 0U;
        std::size_t to_copy = std::min(size, avail);
        data.resize(to_copy);
        if (to_copy != 0U) {
          std::memcpy(data.data(),
                      test_buffer.data() + static_cast<std::size_t>(offset),
                      to_copy);
        }
        return api_error::success;
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

  auto test_buffer = build_test_buffer(source_file);

  EXPECT_CALL(provider, read_file_bytes)
      .WillRepeatedly([test_buffer](std::string_view, std::size_t size,
                                    std::uint64_t offset, data_buffer &data,
                                    stop_type &stop_requested) -> api_error {
        if (stop_requested) {
          return api_error::download_stopped;
        }
        std::size_t avail =
            (offset < test_buffer.size())
                ? (test_buffer.size() - static_cast<std::size_t>(offset))
                : 0U;
        std::size_t to_copy = std::min(size, avail);
        data.resize(to_copy);
        if (to_copy != 0U) {
          std::memcpy(data.data(),
                      test_buffer.data() + static_cast<std::size_t>(offset),
                      to_copy);
        }
        return api_error::success;
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

  auto test_buffer = build_test_buffer(source_file);

  EXPECT_CALL(provider, read_file_bytes)
      .WillRepeatedly([test_buffer](std::string_view, std::size_t size,
                                    std::uint64_t offset, data_buffer &data,
                                    stop_type &stop_requested) -> api_error {
        if (stop_requested) {
          return api_error::download_stopped;
        }
        std::size_t avail =
            (offset < test_buffer.size())
                ? (test_buffer.size() - static_cast<std::size_t>(offset))
                : 0U;
        std::size_t to_copy = std::min(size, avail);
        data.resize(to_copy);
        if (to_copy != 0U) {
          std::memcpy(data.data(),
                      test_buffer.data() + static_cast<std::size_t>(offset),
                      to_copy);
        }
        return api_error::success;
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

TEST_F(direct_open_file_test, clamp_read_past_eof_returns_remaining_only) {
  auto &source_file = test::create_random_file(test_chunk_size * 32U + 11U);
  auto dest_path = test::generate_test_file_name("direct_open_file");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.directory = false;
  fsi.size = test_chunk_size * 32U + 11U;

  auto test_buffer = build_test_buffer(source_file);

  EXPECT_CALL(provider, read_file_bytes)
      .WillRepeatedly([test_buffer](std::string_view, std::size_t size,
                                    std::uint64_t offset, data_buffer &data,
                                    stop_type &stop_requested) -> api_error {
        if (stop_requested) {
          return api_error::download_stopped;
        }
        std::size_t avail =
            (offset < test_buffer.size())
                ? (test_buffer.size() - static_cast<std::size_t>(offset))
                : 0U;
        std::size_t to_copy = std::min(size, avail);
        data.resize(to_copy);
        if (to_copy != 0U) {
          std::memcpy(data.data(),
                      test_buffer.data() + static_cast<std::size_t>(offset),
                      to_copy);
        }
        return api_error::success;
      });

  {
    direct_open_file file(test_chunk_size, 30U, fsi, provider);

    auto out = utils::file::file::open_or_create_file(dest_path);
    EXPECT_TRUE(out);

    std::uint64_t total_read{0U};
    for (std::size_t i = 0U; i < 32U; ++i) {
      data_buffer data{};
      EXPECT_EQ(api_error::success,
                file.read(test_chunk_size, total_read, data));
      std::size_t bytes_written{};
      EXPECT_TRUE(out->write(data, total_read, &bytes_written));
      total_read += data.size();
    }

    {
      data_buffer data{};
      EXPECT_EQ(api_error::success,
                file.read(test_chunk_size, total_read, data));
      EXPECT_EQ(std::size_t(11U), data.size());
      std::size_t bytes_written{};
      EXPECT_TRUE(out->write(data, total_read, &bytes_written));
      total_read += data.size();
    }

    out->close();
    source_file.close();

    auto h1 = utils::file::file(source_file.get_path()).sha256();
    auto h2 = utils::file::file(dest_path).sha256();
    ASSERT_TRUE(h1.has_value());
    ASSERT_TRUE(h2.has_value());
    EXPECT_STREQ(h1->c_str(), h2->c_str());
  }
}

TEST_F(direct_open_file_test, cross_boundary_small_read_is_correct) {
  auto &source_file = test::create_random_file(test_chunk_size * 4U);
  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.directory = false;
  fsi.size = test_chunk_size * 4U;

  auto test_buffer = build_test_buffer(source_file);

  EXPECT_CALL(provider, read_file_bytes)
      .WillRepeatedly([test_buffer](std::string_view, std::size_t size,
                                    std::uint64_t offset, data_buffer &data,
                                    stop_type &stop_requested) -> api_error {
        if (stop_requested) {
          return api_error::download_stopped;
        }
        std::size_t avail =
            (offset < test_buffer.size())
                ? (test_buffer.size() - static_cast<std::size_t>(offset))
                : 0U;
        std::size_t to_copy = std::min(size, avail);
        data.resize(to_copy);
        if (to_copy != 0U) {
          std::memcpy(data.data(),
                      test_buffer.data() + static_cast<std::size_t>(offset),
                      to_copy);
        }
        return api_error::success;
      });

  {
    direct_open_file file(test_chunk_size, 30U, fsi, provider);

    std::size_t read_size{7U};
    std::uint64_t offset{test_chunk_size - 3U};
    data_buffer data{};
    EXPECT_EQ(api_error::success, file.read(read_size, offset, data));
    EXPECT_EQ(read_size, data.size());

    EXPECT_EQ(0,
              std::memcmp(test_buffer.data() + offset, data.data(), read_size));
  }

  source_file.close();
}

TEST_F(direct_open_file_test, random_seek_pattern_reads_match_source) {
  auto &source_file = test::create_random_file(test_chunk_size * 16U);
  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.directory = false;
  fsi.size = test_chunk_size * 16U;

  auto test_buffer = build_test_buffer(source_file);

  EXPECT_CALL(provider, read_file_bytes)
      .WillRepeatedly([test_buffer](std::string_view, std::size_t size,
                                    std::uint64_t offset, data_buffer &data,
                                    stop_type &stop_requested) -> api_error {
        if (stop_requested) {
          return api_error::download_stopped;
        }
        std::size_t avail =
            (offset < test_buffer.size())
                ? (test_buffer.size() - static_cast<std::size_t>(offset))
                : 0U;
        std::size_t to_copy = std::min(size, avail);
        data.resize(to_copy);
        if (to_copy != 0U) {
          std::memcpy(data.data(),
                      test_buffer.data() + static_cast<std::size_t>(offset),
                      to_copy);
        }
        return api_error::success;
      });

  {
    direct_open_file file(test_chunk_size, 30U, fsi, provider);

    struct req {
      std::size_t size;
      std::uint64_t off;
    };
    std::vector<req> pattern_list{
        {3U, 0U},
        {64U, test_chunk_size - 1U},
        {11U, test_chunk_size * 2U + 5U},
        {512U, test_chunk_size * 7U + 13U},
        {5U, test_chunk_size * 16U - 5U},
    };

    for (const auto &pattern : pattern_list) {
      data_buffer data{};
      EXPECT_EQ(api_error::success, file.read(pattern.size, pattern.off, data));
      std::size_t avail =
          (pattern.off < test_buffer.size())
              ? (test_buffer.size() - static_cast<std::size_t>(pattern.off))
              : 0U;
      std::size_t expected_size = std::min(pattern.size, avail);
      ASSERT_EQ(expected_size, data.size());
      EXPECT_EQ(0, std::memcmp(test_buffer.data() + pattern.off, data.data(),
                               expected_size));
    }
  }

  source_file.close();
}

TEST_F(direct_open_file_test, provider_error_is_propagated) {
  auto &source_file = test::create_random_file(test_chunk_size * 4U);
  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.directory = false;
  fsi.size = test_chunk_size * 4U;

  auto test_buffer = build_test_buffer(source_file);

  EXPECT_CALL(provider, read_file_bytes)
      .WillRepeatedly([test_buffer](std::string_view, std::size_t /* size */,
                                    std::uint64_t /* offset */,
                                    data_buffer & /* data */,
                                    stop_type &stop_requested) -> api_error {
        if (stop_requested) {
          return api_error::download_stopped;
        }

        return api_error::os_error;
      });

  {
    direct_open_file file(test_chunk_size, 30U, fsi, provider);

    data_buffer data{};
    EXPECT_EQ(api_error::os_error, file.read(17U, 0U, data));
  }

  source_file.close();
}

TEST_F(direct_open_file_test, tiny_file_smaller_than_chunk) {
  auto &source_file = test::create_random_file(17U);
  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.directory = false;
  fsi.size = 17U;

  auto test_buffer = build_test_buffer(source_file);

  EXPECT_CALL(provider, read_file_bytes)
      .WillRepeatedly([test_buffer](std::string_view, std::size_t size,
                                    std::uint64_t offset, data_buffer &data,
                                    stop_type &stop_requested) -> api_error {
        if (stop_requested) {
          return api_error::download_stopped;
        }
        std::size_t avail =
            (offset < test_buffer.size())
                ? (test_buffer.size() - static_cast<std::size_t>(offset))
                : 0U;
        std::size_t to_copy = std::min(size, avail);
        data.resize(to_copy);
        if (to_copy != 0U) {
          std::memcpy(data.data(),
                      test_buffer.data() + static_cast<std::size_t>(offset),
                      to_copy);
        }
        return api_error::success;
      });

  {
    direct_open_file file(test_chunk_size, 30U, fsi, provider);

    data_buffer data{};
    EXPECT_EQ(api_error::success, file.read(test_chunk_size, 0U, data));
    EXPECT_EQ(std::size_t(17U), data.size());
  }

  source_file.close();
}
} // namespace repertory
