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
#include "download/download.hpp"
#include "mocks/mock_open_file_table.hpp"
#include "test_common.hpp"
#include "utils/event_capture.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
static const std::size_t READ_SIZE = 1024u * 1024u;

static std::string get_source_file_name() { return generate_test_file_name("./", "download"); }

static void write_and_test(download &d, const std::vector<char> &buffer, const bool &to_end) {
  if (to_end) {
    std::size_t bytes_written{};
    EXPECT_EQ(api_error::success, d.write_bytes(0u, 20u * READ_SIZE, buffer, bytes_written,
                                                [](std::uint64_t, std::uint64_t, bool) {}));

    std::vector<char> data;
    EXPECT_EQ(api_error::success, d.read_bytes(0u, buffer.size(), 20u * READ_SIZE, data));
    EXPECT_EQ(std::vector<char>(buffer), data);
  } else {
    std::size_t bytes_written{};
    EXPECT_EQ(api_error::success,
              d.write_bytes(0u, READ_SIZE + READ_SIZE - 2u, buffer, bytes_written,
                            [](std::uint64_t, std::uint64_t, bool) {}));

    std::vector<char> data;
    EXPECT_EQ(api_error::success, d.read_bytes(0u, 4u, READ_SIZE + READ_SIZE - 2u, data));
    EXPECT_EQ(std::vector<char>(buffer), data);
  }
}

TEST(download, write_non_cached_file) {
  mock_open_file_table oft;
  console_consumer c;
  event_system::instance().start();
  utils::file::delete_directory_recursively(utils::path::absolute("./download_data"));

  const auto source_file_path = get_source_file_name();
  const auto source_file_size = 20u * READ_SIZE;
  utils::file::delete_file(source_file_path);
  auto source_file = create_random_file(source_file_path, source_file_size);

  EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(0ull));
  filesystem_item fsi2{};
  EXPECT_CALL(oft, force_schedule_upload(_))
      .Times(2)
      .WillRepeatedly(DoAll(SaveArg<0>(&fsi2), Return()));

  api_reader_callback api_reader = [&](const std::string &, const std::size_t &size,
                                       const std::uint64_t &offset, std::vector<char> &data,
                                       const bool &) -> api_error {
    data.resize(size);
    const auto chunk = (offset / READ_SIZE);
    if (chunk < 1u || (chunk > 2u)) {
      std::this_thread::sleep_for(100ms);
    }

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

  filesystem_item fsi{};
  fsi.api_path = "/Test.dat";
  fsi.size = source_file_size;
  fsi.api_parent = "/";
  fsi.directory = false;
  fsi.lock = std::make_shared<std::recursive_mutex>();

  {
    event_capture ec({"download_begin", "download_end", "download_progress"});
    app_config config(provider_type::sia, "./download_data");

    {
      download d(config, fsi, api_reader, READ_SIZE, oft);

      write_and_test(d, {'a', 'a', 'a', 'a'}, false);
      ec.wait_for_event("download_end");

      const auto new_source_file_path = d.get_source_path();
      EXPECT_TRUE(utils::file::is_file(new_source_file_path));
      EXPECT_STRNE(source_file_path.c_str(), new_source_file_path.c_str());
      EXPECT_STREQ(new_source_file_path.c_str(), fsi2.source_path.c_str());
      EXPECT_STREQ(fsi.api_path.c_str(), fsi2.api_path.c_str());
      EXPECT_FALSE(fsi2.source_path_changed);
      EXPECT_FALSE(fsi2.changed);
      write_and_test(d, {'b', 'b', 'b', 'b'}, false);

      filesystem_item fsi3{};
      boost::dynamic_bitset<> read_state, write_state;
      std::size_t chunk_size{}, last_chunk_size{};
      d.get_state_information(fsi3, chunk_size, last_chunk_size, read_state, write_state);
      EXPECT_STREQ(fsi2.source_path.c_str(), fsi3.source_path.c_str());
      EXPECT_FALSE(write_state[0]);
      EXPECT_TRUE(write_state[1]);
      EXPECT_TRUE(write_state[2]);
      for (std::size_t i = 3u; i < write_state.size(); i++) {
        EXPECT_FALSE(write_state[i]);
      }
      EXPECT_TRUE(read_state.all());
    }
  }
  source_file->close();

  event_system::instance().stop();
  utils::file::delete_directory_recursively(utils::path::absolute("./download_data"));
  utils::file::delete_file(source_file_path);
}

TEST(Download, write_non_cached_file_and_grow_size) {
  mock_open_file_table oft;
  console_consumer c;
  event_system::instance().start();
  utils::file::delete_directory_recursively(utils::path::absolute("./download_data"));

  const auto source_file_path = get_source_file_name();
  const auto source_file_size = 20u * READ_SIZE;
  utils::file::delete_file(source_file_path);
  auto sourceFile = create_random_file(source_file_path, source_file_size);

  EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(0ull));
  filesystem_item fsi2{};
  EXPECT_CALL(oft, force_schedule_upload(_)).WillOnce(DoAll(SaveArg<0>(&fsi2), Return()));

  api_reader_callback api_reader = [&](const std::string &, const std::size_t &size,
                                       const std::uint64_t &offset, std::vector<char> &data,
                                       const bool &) -> api_error {
    data.resize(size);
    const auto chunk = (offset / READ_SIZE);
    if (chunk < 20u) {
      std::this_thread::sleep_for(100ms);
    }

    std::size_t bytes_read{};
    auto ret = sourceFile->read_bytes(&data[0u], data.size(), offset, bytes_read)
                   ? api_error::success
                   : api_error::os_error;
    if (ret != api_error::success) {
      std::cout << utils::get_last_error_code() << std::endl;
    }
    EXPECT_EQ(api_error::success, ret);
    EXPECT_EQ(bytes_read, data.size());
    return ret;
  };

  filesystem_item fsi{};
  fsi.api_path = "/Test.dat";
  fsi.size = source_file_size;
  fsi.api_parent = "/";
  fsi.directory = false;
  fsi.lock = std::make_shared<std::recursive_mutex>();

  {
    event_capture ec({"download_begin", "download_end", "download_progress"});
    app_config config(provider_type::sia, "./download_data");
    {
      download d(config, fsi, api_reader, READ_SIZE, oft);

      write_and_test(d, {'a', 'a', 'a', 'a'}, true);
      ec.wait_for_event("download_end");

      const auto new_source_file_path = d.get_source_path();
      EXPECT_TRUE(utils::file::is_file(new_source_file_path));
      EXPECT_STRNE(source_file_path.c_str(), new_source_file_path.c_str());
      EXPECT_STREQ(new_source_file_path.c_str(), fsi2.source_path.c_str());
      EXPECT_STREQ(fsi.api_path.c_str(), fsi2.api_path.c_str());
      EXPECT_FALSE(fsi2.source_path_changed);
      EXPECT_FALSE(fsi2.changed);
      EXPECT_EQ(source_file_size + 4u, fsi2.size);

      filesystem_item fsi3{};
      boost::dynamic_bitset<> read_state, write_state;
      std::size_t chunk_size{}, last_chunk_size{};
      d.get_state_information(fsi3, chunk_size, last_chunk_size, read_state, write_state);
      EXPECT_STREQ(fsi2.source_path.c_str(), fsi3.source_path.c_str());
      EXPECT_EQ(21u, write_state.size());
      EXPECT_EQ(write_state.size(), read_state.size());
      EXPECT_EQ(source_file_size + 4u, fsi3.size);

      for (std::size_t i = 0u; i < write_state.size() - 1u; i++) {
        EXPECT_FALSE(write_state[i]);
      }

      EXPECT_TRUE(write_state[20u]);
      EXPECT_TRUE(read_state.all());
    }
  }
  sourceFile->close();

  event_system::instance().stop();
  utils::file::delete_directory_recursively(utils::path::absolute("./download_data"));
  utils::file::delete_file(source_file_path);
}
} // namespace repertory
