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
#include "app_config.hpp"
#include "download/direct_download.hpp"
#include "download/download.hpp"
#include "download/ring_download.hpp"
#include "mocks/mock_open_file_table.hpp"
#include "test_common.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
static const std::size_t READ_SIZE = 1024u * 1024u;
static const std::int32_t DOWNLOADER_COUNT = 3;

static std::string get_source_file_name() { return generate_test_file_name("./", "downloaders"); }

static std::shared_ptr<i_download> create_download(const std::int32_t &index, app_config &config,
                                                   filesystem_item &fsi,
                                                   api_reader_callback &api_reader,
                                                   i_open_file_table &oft) {
  return (index == 0)   ? std::dynamic_pointer_cast<i_download>(std::make_shared<ring_download>(
                            config, fsi, api_reader, 0, READ_SIZE, 6u * READ_SIZE))
         : (index == 1) ? std::dynamic_pointer_cast<i_download>(
                              std::make_shared<direct_download>(config, fsi, api_reader, 0))
                        : std::dynamic_pointer_cast<i_download>(std::make_shared<download>(
                              config, fsi, api_reader, 6u * READ_SIZE, oft));
}

static void run_full_file_test(const std::size_t &max_read_size,
                               const std::size_t &source_file_size) {
  const auto source_file_path = get_source_file_name();
  {
    console_consumer c;
    utils::file::delete_directory_recursively(utils::path::absolute("./downloaders_data"));
    app_config config(provider_type::sia, "./downloaders_data");

    utils::file::delete_file(source_file_path);

    auto source_file = create_random_file(source_file_path, source_file_size);
    EXPECT_NE(source_file, nullptr);
    if (source_file) {
      api_reader_callback api_reader =
          [&source_file](const std::string &, const std::size_t &size, const std::uint64_t &offset,
                         std::vector<char> &data, const bool &) -> api_error {
        data.resize(size);
        std::size_t bytes_read{};
        auto ret = source_file->read_bytes(&data[0u], data.size(), offset, bytes_read)
                       ? api_error::success
                       : api_error::os_error;
        if (ret != api_error::success) {
          std::cout << utils::get_last_error_code() << std::endl;
        }
        EXPECT_EQ(api_error::success, ret);
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      };

      event_system::instance().start();
      for (auto i = 0; i < DOWNLOADER_COUNT; i++) {
        filesystem_item fsi{};
        fsi.api_path = "/Test.dat";
        fsi.size = source_file_size;
        fsi.api_parent = "/";
        fsi.directory = false;
        fsi.lock = std::make_shared<std::recursive_mutex>();

        mock_open_file_table oft;
        EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(1ull));
        auto download = create_download(i, config, fsi, api_reader, oft);

        auto size_remain = source_file_size;
        auto result_code = api_error::success;
        auto offset = 0ull;
        auto total_read = 0ull;
        while ((result_code == api_error::success) && (size_remain > 0)) {
          const auto read_size = std::min(max_read_size, static_cast<std::size_t>(size_remain));
          std::vector<char> data;
          if ((result_code = download->read_bytes(0u, read_size, offset, data)) ==
              api_error::success) {
            total_read += data.size();
            EXPECT_EQ(read_size, data.size());
            if (data.size() == read_size) {
              std::size_t bytes_read = 0;
              std::vector<char> data2(data.size());
              result_code = source_file->read_bytes(&data2[0u], data2.size(), offset, bytes_read)
                                ? api_error::success
                                : api_error::os_error;
              EXPECT_EQ(data.size(), data2.size());
              EXPECT_EQ(api_error::success, result_code);
              const auto comp = memcmp(&data[0u], &data2[0u], data.size());
              EXPECT_EQ(0, comp);
            }

            size_remain -= read_size;
            offset += read_size;
          }
          EXPECT_EQ(api_error::success, result_code);
        }
        EXPECT_EQ(source_file_size, total_read);
        download.reset();
      }
      source_file->close();
    }
    event_system::instance().stop();
  }

  utils::file::delete_file(source_file_path);
  utils::file::delete_directory_recursively(utils::path::absolute("./downloaders_data"));
}

static void run_read_past_full_buffer_test() {
  const auto max_read_size = READ_SIZE;
  const auto source_file_path = get_source_file_name();
  const auto source_file_size = 20u * READ_SIZE;

  utils::file::delete_directory_recursively(utils::path::absolute("./downloaders_data"));
  {
    app_config config(provider_type::sia, "./downloaders_data");

    utils::file::delete_file(source_file_path);

    console_consumer c;
    auto source_file = create_random_file(source_file_path, source_file_size);
    EXPECT_NE(source_file, nullptr);

    if (source_file) {
      api_reader_callback api_reader = [&](const std::string &, const std::size_t &size,
                                           const std::uint64_t &offset, std::vector<char> &data,
                                           const bool &) -> api_error {
        data.resize(size);
        std::size_t bytes_read{};
        const auto ret = source_file->read_bytes(&data[0u], data.size(), offset, bytes_read)
                             ? api_error::success
                             : api_error::os_error;
        EXPECT_EQ(api_error::success, ret);
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      };

      event_system::instance().start();
      for (auto i = 0; i < DOWNLOADER_COUNT; i++) {
        filesystem_item fsi{};
        fsi.api_path = "/Test.dat";
        fsi.size = source_file_size;
        fsi.api_parent = "/";
        fsi.directory = false;
        fsi.lock = std::make_shared<std::recursive_mutex>();

        mock_open_file_table oft;
        EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(1ull));
        auto download = create_download(i, config, fsi, api_reader, oft);

        auto size_remain = source_file_size;
        auto result_code = api_error::success;
        auto offset = 0ull;
        while ((result_code == api_error::success) && (size_remain > 0)) {
          const auto read_size = std::min(max_read_size, static_cast<std::size_t>(size_remain));
          std::vector<char> data;
          if ((result_code = download->read_bytes(0u, read_size, offset, data)) ==
              api_error::success) {
            EXPECT_EQ(read_size, data.size());
            if (data.size() == read_size) {
              std::size_t bytes_read = 0;
              std::vector<char> data2(data.size());
              result_code = source_file->read_bytes(&data2[0u], data2.size(), offset, bytes_read)
                                ? api_error::success
                                : api_error::os_error;
              EXPECT_EQ(api_error::success, result_code);
              EXPECT_EQ(data.size(), bytes_read);
              const auto comp = memcmp(&data[0u], &data2[0u], data.size());
              EXPECT_EQ(0, comp);
            }

            if (size_remain == source_file_size) {
              size_remain -= (read_size + (6ull * READ_SIZE));
              offset += (read_size + (6ull * READ_SIZE));
            } else {
              size_remain -= read_size;
              offset += read_size;
            }
          }

          EXPECT_EQ(api_error::success, result_code);
        }
        download.reset();
      }
      source_file->close();
      utils::file::delete_file(source_file_path);
    }
    event_system::instance().stop();
  }

  utils::file::delete_directory_recursively(utils::path::absolute("./downloaders_data"));
}

static void run_read_with_seek_behind() {
  const auto max_read_size = READ_SIZE;
  const auto source_file_path = get_source_file_name();
  const auto source_file_size = 20u * READ_SIZE;

  utils::file::delete_directory_recursively(utils::path::absolute("./downloaders_data"));
  {
    app_config config(provider_type::sia, "./downloaders_data");

    utils::file::delete_file(source_file_path);

    console_consumer c;
    auto source_file = create_random_file(source_file_path, source_file_size);
    EXPECT_NE(source_file, nullptr);

    if (source_file) {
      api_reader_callback api_reader = [&](const std::string &, const std::size_t &size,
                                           const std::uint64_t &offset, std::vector<char> &data,
                                           const bool &) -> api_error {
        data.resize(size);
        std::size_t bytes_read{};
        auto ret = source_file->read_bytes(&data[0u], data.size(), offset, bytes_read)
                       ? api_error::success
                       : api_error::os_error;
        EXPECT_EQ(api_error::success, ret);
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      };

      event_system::instance().start();

      for (auto i = 0; i < DOWNLOADER_COUNT; i++) {
        filesystem_item fsi{};
        fsi.api_path = "/Test.dat";
        fsi.size = source_file_size;
        fsi.api_parent = "/";
        fsi.directory = false;
        fsi.lock = std::make_shared<std::recursive_mutex>();

        mock_open_file_table oft;
        EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(1ull));
        auto download = create_download(i, config, fsi, api_reader, oft);

        auto size_remain = source_file_size;
        auto result_code = api_error::success;
        auto offset = 0ull;
        auto read_count = 0;
        while ((result_code == api_error::success) && (size_remain > 0)) {
          const auto read_size = std::min(max_read_size, static_cast<std::size_t>(size_remain));
          std::vector<char> data;
          if ((result_code = download->read_bytes(0u, read_size, offset, data)) ==
              api_error::success) {
            EXPECT_EQ(read_size, data.size());
            if (data.size() == read_size) {
              std::size_t bytes_read = 0;
              std::vector<char> data2(data.size());
              result_code = source_file->read_bytes(&data2[0u], data2.size(), offset, bytes_read)
                                ? api_error::success
                                : api_error::os_error;
              EXPECT_EQ(api_error::success, result_code);
              EXPECT_EQ(data.size(), bytes_read);
              const auto comp = memcmp(&data[0u], &data2[0u], data.size());
              EXPECT_EQ(0, comp);
            }

            if (++read_count == 3) {
              size_remain += (read_size * 2);
              offset -= (read_size * 2);
            } else {
              size_remain -= read_size;
              offset += read_size;
            }
          }
          EXPECT_EQ(api_error::success, result_code);
        }
        download.reset();
      }
      source_file->close();
      utils::file::delete_file(source_file_path);
    }
    event_system::instance().stop();
  }

  utils::file::delete_directory_recursively(utils::path::absolute("./downloaders_data"));
}

static void run_seek_begin_to_end_to_begin() {
  const auto max_read_size = READ_SIZE;
  const auto source_file_path = get_source_file_name();
  const auto source_file_size = 20u * READ_SIZE;

  utils::file::delete_directory_recursively(utils::path::absolute("./downloaders_data"));
  {
    app_config config(provider_type::sia, "./downloaders_data");

    utils::file::delete_file(source_file_path);

    console_consumer c;
    auto source_file = create_random_file(source_file_path, source_file_size);
    EXPECT_NE(source_file, nullptr);

    if (source_file) {

      api_reader_callback api_reader = [&](const std::string &, const std::size_t &size,
                                           const std::uint64_t &offset, std::vector<char> &data,
                                           const bool &) -> api_error {
        data.resize(size);
        std::size_t bytes_read{};
        auto ret = source_file->read_bytes(&data[0u], data.size(), offset, bytes_read)
                       ? api_error::success
                       : api_error::os_error;
        EXPECT_EQ(api_error::success, ret);
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      };

      event_system::instance().start();

      for (auto i = 0; i < DOWNLOADER_COUNT; i++) {
        filesystem_item fsi{};
        fsi.api_path = "/Test.dat";
        fsi.size = source_file_size;
        fsi.api_parent = "/";
        fsi.directory = false;
        fsi.lock = std::make_shared<std::recursive_mutex>();

        mock_open_file_table oft;
        EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(1ull));
        auto download = create_download(i, config, fsi, api_reader, oft);

        auto size_remain = source_file_size;
        auto result_code = api_error::success;
        auto offset = 0ull;
        auto readCount = 0;
        while ((result_code == api_error::success) && (readCount < 4)) {
          const auto read_size = std::min(max_read_size, static_cast<std::size_t>(size_remain));
          std::vector<char> data;
          if ((result_code = download->read_bytes(0u, read_size, offset, data)) ==
              api_error::success) {
            EXPECT_EQ(read_size, data.size());
            if (data.size() == read_size) {
              std::size_t bytes_read = 0;
              std::vector<char> data2(data.size());
              result_code = source_file->read_bytes(&data2[0u], data2.size(), offset, bytes_read)
                                ? api_error::success
                                : api_error::os_error;
              EXPECT_EQ(api_error::success, result_code);
              EXPECT_EQ(data.size(), bytes_read);
              const auto comp = memcmp(&data[0u], &data2[0u], data.size());
              EXPECT_EQ(0, comp);
              if (comp) {
                std::cout << "mismatch-" << read_size << ':' << offset << std::endl;
              }
            }

            ++readCount;
            if (readCount == 1) {
              size_remain = read_size;
              offset = source_file_size - read_size;
            } else if (readCount == 2) {
              std::this_thread::sleep_for(10ms);
              size_remain = source_file_size;
              offset = 0ull;
            } else {
              size_remain -= read_size;
              offset += read_size;
            }
          }
          EXPECT_EQ(api_error::success, result_code);
        }
        download.reset();
      }
      source_file->close();
      utils::file::delete_file(source_file_path);
    }
    event_system::instance().stop();
  }

  utils::file::delete_directory_recursively(utils::path::absolute("./downloaders_data"));
}

TEST(downloaders, read_full_file) {
  const auto max_read_size = READ_SIZE;
  const auto source_file_size = 20u * READ_SIZE;
  run_full_file_test(max_read_size, source_file_size);
}

TEST(downloaders, read_full_file_with_overlapping_chunks) {
  const auto max_read_size = READ_SIZE + (READ_SIZE / 2);
  const auto source_file_size = 20u * READ_SIZE;
  run_full_file_test(max_read_size, source_file_size);
}

TEST(downloaders, read_full_file_with_non_matching_chunk_size) {
  const auto max_read_size = READ_SIZE;
  const auto source_file_size = (20u * READ_SIZE) + 252u;
  run_full_file_test(max_read_size, source_file_size);
}

TEST(downloaders, read_full_file_with_partial_reads) {
  const auto max_read_size = 32u * 1024u;
  const auto source_file_size = 20u * READ_SIZE;
  run_full_file_test(max_read_size, source_file_size);
}

TEST(downloaders, read_full_file_with_partial_overlapping_reads) {
  const auto max_read_size = (READ_SIZE / 2) + 20u;
  const auto source_file_size = 20u * READ_SIZE;
  run_full_file_test(max_read_size, source_file_size);
}

TEST(downloaders, read_past_full_buffer) { run_read_past_full_buffer_test(); }

TEST(downloaders, read_with_seek_behind) { run_read_with_seek_behind(); }

TEST(downloaders, seek_begin_to_end_to_begin) { run_seek_begin_to_end_to_begin(); }
} // namespace repertory
