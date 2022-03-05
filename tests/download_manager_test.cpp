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
#include "download/download_manager.hpp"
#include "events/consumers/console_consumer.hpp"
#include "mocks/mock_open_file_table.hpp"
#include "test_common.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/event_capture.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
static std::string get_source_file_name() {
  return generate_test_file_name("./", "downloader_manager");
}

TEST(downloader_manager, no_hanging_on_download_fail) {
  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
  {
    app_config config(provider_type::sia, "./chunk_data");

    console_consumer c;
    event_system::instance().start();

    event_capture ec({"download_begin", "download_end"}, {"download_timeout"});
    const auto test_source = get_source_file_name();
    utils::file::delete_file(test_source);

    const auto chunk_size = utils::encryption::encrypting_reader::get_data_chunk_size();
    const auto file_size = (chunk_size * 5u) + 2u;
    const auto test_dest = get_source_file_name();
    utils::file::delete_file(test_dest);

    auto nf = create_random_file(test_source, file_size);
    EXPECT_NE(nf, nullptr);
    if (nf) {
      api_reader_callback api_reader = [&](const std::string &, const std::size_t &,
                                           const std::uint64_t &, std::vector<char> &,
                                           const bool &) -> api_error {
#ifdef _WIN32
        ::SetLastError(ERROR_ACCESS_DENIED);
#else
        utils::set_last_error_code(EACCES);
#endif
        return api_error::os_error;
      };

      filesystem_item fsi{};
      fsi.api_path = "/test_chunk";
      fsi.directory = false;
      fsi.lock = std::make_shared<std::recursive_mutex>();
      fsi.source_path = test_dest;
      fsi.size = file_size;

      download_manager dm(config, api_reader, true);
      mock_open_file_table oft(&dm, &fsi);
      EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(1ull));
      dm.start(&oft);

      EXPECT_EQ(api_error::os_error, dm.download_file(1, fsi));

      dm.stop();
      EXPECT_EQ(0u, dm.get_download_count());
      nf->close();

      utils::file::delete_file(fsi.source_path);
    }

    utils::file::delete_file(test_source);
    event_system::instance().stop();
  }

  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
}

TEST(downloader_manager, single_chunk_no_offset) {
  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
  {
    app_config config(provider_type::sia, "./chunk_data");

    console_consumer c;
    event_system::instance().start();

    event_capture ec({"download_begin", "download_end", "download_progress"}, {"DownloadTimeout"});
    const auto test_source = get_source_file_name();
    utils::file::delete_file(test_source);

    const auto chunk_size = utils::encryption::encrypting_reader::get_data_chunk_size();
    const auto file_size = (chunk_size * 5u);
    auto nf = create_random_file(test_source, file_size);
    EXPECT_NE(nf, nullptr);
    if (nf) {
      api_reader_callback api_reader = [&](const std::string &, const std::size_t &size,
                                           const std::uint64_t &offset, std::vector<char> &data,
                                           const bool &) -> api_error {
        data.resize(size);
        std::size_t bytes_read{};
        auto ret = nf->read_bytes(&data[0u], data.size(), offset, bytes_read) ? api_error::success
                                                                              : api_error::os_error;
        EXPECT_EQ(api_error::success, ret);
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      };

      filesystem_item fsi{};
      fsi.api_path = "/test_chunk";
      fsi.directory = false;
      fsi.lock = std::make_shared<std::recursive_mutex>();
      fsi.source_path = test_source + "0";
      fsi.size = file_size;

      download_manager dm(config, api_reader, true);
      mock_open_file_table oft(&dm, &fsi);
      EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(1ull));
      dm.start(&oft);

      std::vector<char> read_buffer;
      EXPECT_EQ(api_error::success, dm.read_bytes(1, fsi, chunk_size, 0, read_buffer));
      EXPECT_EQ(chunk_size, read_buffer.size());

      std::size_t bytes_read{};
      std::vector<char> source_buffer;
      source_buffer.resize(chunk_size);
      EXPECT_TRUE(nf->read_bytes(&source_buffer[0u], source_buffer.size(), 0, bytes_read));
      EXPECT_EQ(read_buffer.size(), bytes_read);
      EXPECT_EQ(read_buffer.size(), source_buffer.size());
      if (read_buffer.size() == source_buffer.size()) {
        EXPECT_EQ(0, memcmp(&read_buffer[0u], &source_buffer[0u], read_buffer.size()));
      }

      dm.stop();
      EXPECT_EQ(0u, dm.get_download_count());
      nf->close();

      utils::file::delete_file(fsi.source_path);
    }

    utils::file::delete_file(test_source);
    event_system::instance().stop();
  }

  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
}

TEST(downloader_manager, single_chunk_offset_overlap) {
  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
  {
    app_config config(provider_type::sia, "./chunk_data");
    console_consumer c;
    event_system::instance().start();

    event_capture ec(
        {
            "download_begin",
            "download_end",
            "download_progress",
        },
        {"download_timeout"});
    const auto test_source = get_source_file_name();
    utils::file::delete_file(test_source);
    const auto chunk_size = utils::encryption::encrypting_reader::get_data_chunk_size();
    const auto file_size = (chunk_size * 5u);
    auto nf = create_random_file(test_source, file_size);
    EXPECT_NE(nf, nullptr);
    if (nf) {
      api_reader_callback api_reader = [&](const std::string &, const std::size_t &size,
                                           const std::uint64_t &offset, std::vector<char> &data,
                                           const bool &) -> api_error {
        data.resize(size);
        std::size_t bytes_read{};
        auto ret = nf->read_bytes(&data[0u], data.size(), offset, bytes_read) ? api_error::success
                                                                              : api_error::os_error;
        EXPECT_EQ(api_error::success, ret);
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      };

      filesystem_item fsi{};
      fsi.api_path = "/test_chunk";
      fsi.directory = false;
      fsi.lock = std::make_shared<std::recursive_mutex>();
      fsi.source_path = test_source + "0";
      fsi.size = file_size;

      download_manager dm(config, api_reader, true);
      mock_open_file_table oft(&dm, &fsi);
      EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(1ull));
      dm.start(&oft);

      std::vector<char> read_buffer;
      EXPECT_EQ(api_error::success, dm.read_bytes(1, fsi, chunk_size, chunk_size / 2, read_buffer));
      EXPECT_EQ(chunk_size, read_buffer.size());

      std::size_t bytes_read{};
      std::vector<char> source_buffer;
      source_buffer.resize(chunk_size);
      EXPECT_TRUE(
          nf->read_bytes(&source_buffer[0u], source_buffer.size(), chunk_size / 2, bytes_read));
      EXPECT_EQ(read_buffer.size(), bytes_read);
      EXPECT_EQ(read_buffer.size(), source_buffer.size());
      if (read_buffer.size() == source_buffer.size()) {
        EXPECT_EQ(0, memcmp(&read_buffer[0u], &source_buffer[0u], read_buffer.size()));
      }

      dm.stop();
      EXPECT_EQ(0, dm.get_download_count());
      nf->close();

      utils::file::delete_file(fsi.source_path);

      utils::file::delete_file(test_source);
      event_system::instance().stop();
    }
  }

  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
}

TEST(downloader_manager, check_no_overflow_on_read_greater_than_file_size) {
  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
  {
    app_config config(provider_type::sia, "./chunk_data");

    console_consumer c;
    event_system::instance().start();

    event_capture ec({}, {
                             "download_begin",
                             "download_end",
                             "download_timeout",
                         });
    const auto test_source = get_source_file_name();
    utils::file::delete_file(test_source);

    const auto chunk_size = utils::encryption::encrypting_reader::get_data_chunk_size();
    const auto file_size = (chunk_size * 5u);
    auto nf = create_random_file(test_source, file_size);
    EXPECT_NE(nf, nullptr);
    if (nf) {
      api_reader_callback api_reader = [&](const std::string &, const std::size_t &size,
                                           const std::uint64_t &offset, std::vector<char> &data,
                                           const bool &) -> api_error {
        data.resize(size);
        std::size_t bytes_read{};
        auto ret = nf->read_bytes(&data[0u], data.size(), offset, bytes_read) ? api_error::success
                                                                              : api_error::os_error;
        EXPECT_EQ(api_error::success, ret);
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      };

      filesystem_item fsi{};
      fsi.api_path = "/test_chunk";
      fsi.directory = false;
      fsi.lock = std::make_shared<std::recursive_mutex>();
      fsi.source_path = test_source + "0";
      fsi.size = file_size;
      download_manager dm(config, api_reader, true);
      mock_open_file_table oft(&dm, &fsi);
      EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(1ull));
      dm.start(&oft);

      std::vector<char> data;
      EXPECT_EQ(api_error::success, dm.read_bytes(1, fsi, file_size * 2, file_size, data));
      EXPECT_EQ(0, data.size());

      dm.stop();
      EXPECT_EQ(0, dm.get_download_count());
      nf->close();

      utils::file::delete_file(fsi.source_path);

      utils::file::delete_file(test_source);
      event_system::instance().stop();
    }
  }

  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
}

TEST(downloader_manager, check_read_size_greater_than_file_size) {
  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
  {
    app_config config(provider_type::sia, "./chunk_data");

    console_consumer c;
    event_system::instance().start();

    event_capture ec(
        {
            "download_begin",
            "download_end",
            "download_progress",
        },
        {"download_timeout"});
    const auto test_source = get_source_file_name();
    utils::file::delete_file(test_source);

    const auto chunk_size = utils::encryption::encrypting_reader::get_data_chunk_size();
    const auto file_size = (chunk_size * 5u);
    auto nf = create_random_file(test_source, file_size);
    EXPECT_NE(nf, nullptr);
    if (nf) {
      api_reader_callback api_reader = [&](const std::string &, const std::size_t &size,
                                           const std::uint64_t &offset, std::vector<char> &data,
                                           const bool &) -> api_error {
        data.resize(size);
        std::size_t bytes_read{};
        auto ret = nf->read_bytes(&data[0u], data.size(), offset, bytes_read) ? api_error::success
                                                                              : api_error::os_error;
        EXPECT_EQ(api_error::success, ret);
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      };

      filesystem_item fsi{};
      fsi.api_path = "/test_chunk";
      fsi.directory = false;
      fsi.lock = std::make_shared<std::recursive_mutex>();
      fsi.source_path = test_source + "0";
      fsi.size = file_size;

      download_manager dm(config, api_reader, true);
      mock_open_file_table oft(&dm, &fsi);
      EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(1ull));
      dm.start(&oft);

      std::vector<char> data;
      EXPECT_EQ(api_error::success, dm.read_bytes(1, fsi, file_size * 2, 0, data));
      EXPECT_EQ(file_size, data.size());

      dm.stop();
      EXPECT_EQ(0, dm.get_download_count());
      nf->close();

      utils::file::delete_file(fsi.source_path);

      utils::file::delete_file(test_source);
      event_system::instance().stop();
    }
  }

  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
}

TEST(downloader_manager, download_file) {
  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
  {
    app_config config(provider_type::sia, "./chunk_data");

    console_consumer c;
    event_system::instance().start();

    event_capture ec(
        {
            "download_begin",
            "download_end",
            "download_progress",
        },
        {"download_timeout"});
    const auto test_source = get_source_file_name();
    utils::file::delete_file(test_source);

    const auto chunk_size = utils::encryption::encrypting_reader::get_data_chunk_size();
    const auto file_size = (chunk_size * 5u + 8u);
    const auto test_dest = get_source_file_name();
    utils::file::delete_file(test_dest);

    auto nf = create_random_file(test_source, file_size);
    EXPECT_NE(nf, nullptr);
    if (nf) {
      api_reader_callback api_reader = [&](const std::string &, const std::size_t &size,
                                           const std::uint64_t &offset, std::vector<char> &data,
                                           const bool &) -> api_error {
        data.resize(size);
        std::size_t bytes_read{};
        auto ret = nf->read_bytes(&data[0u], data.size(), offset, bytes_read) ? api_error::success
                                                                              : api_error::os_error;
        EXPECT_EQ(api_error::success, ret);
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      };

      filesystem_item fsi{};
      fsi.api_path = "/test_chunk";
      fsi.directory = false;
      fsi.lock = std::make_shared<std::recursive_mutex>();
      fsi.source_path = test_dest;
      fsi.size = file_size;

      download_manager dm(config, api_reader, true);
      mock_open_file_table oft(&dm, &fsi);
      EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(1ull));
      dm.start(&oft);

      EXPECT_EQ(api_error::success, dm.download_file(1, fsi));

      std::uint64_t source_size;
      EXPECT_TRUE(utils::file::get_file_size(test_source, source_size));
      std::uint64_t current_size;
      EXPECT_TRUE(utils::file::get_file_size(fsi.source_path, current_size));
      EXPECT_EQ(source_size, current_size);

      EXPECT_STRCASEEQ(utils::file::generate_sha256(test_source).c_str(),
                       utils::file::generate_sha256(fsi.source_path).c_str());

      dm.stop();
      EXPECT_EQ(0, dm.get_download_count());
      nf->close();

      utils::file::delete_file(fsi.source_path);
    }

    utils::file::delete_file(test_source);
    event_system::instance().stop();
  }

  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
}

TEST(downloader_manager, download_timeout) {
  console_consumer c;
  event_system::instance().start();

  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
  {
    app_config config(provider_type::sia, "./chunk_data");
    config.set_chunk_downloader_timeout_secs(5);
    config.set_enable_chunk_downloader_timeout(true);

    event_capture ec({
        "filesystem_item_closed",
        "download_timeout",
        "download_begin",
        "download_end",
    });
    const auto test_source = get_source_file_name();
    utils::file::delete_file(test_source);

    const auto chunk_size = utils::encryption::encrypting_reader::get_data_chunk_size();
    const auto file_size = (chunk_size * 5u) + 2;
    const auto test_dest = get_source_file_name();
    utils::file::delete_file(test_dest);

    auto nf = create_random_file(test_source, file_size);
    EXPECT_NE(nf, nullptr);
    if (nf) {
      filesystem_item *pfsi = nullptr;
      auto sent = false;
      api_reader_callback api_reader = [&](const std::string &, const std::size_t &,
                                           const std::uint64_t &, std::vector<char> &,
                                           const bool &stop_requested) -> api_error {
        if (not sent) {
          sent = true;
          event_system::instance().raise<filesystem_item_closed>(pfsi->api_path, pfsi->source_path,
                                                                 false, false);
        }
        while (not stop_requested) {
          std::this_thread::sleep_for(1ms);
        }
        return api_error::download_failed;
      };

      filesystem_item fsi{};
      fsi.api_path = "/test_chunk";
      fsi.directory = false;
      fsi.lock = std::make_shared<std::recursive_mutex>();
      fsi.source_path = test_dest;
      fsi.size = file_size;
      pfsi = &fsi;

      download_manager dm(config, api_reader, true);
      mock_open_file_table oft;
      EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(0ull));
      dm.start(&oft);

      EXPECT_EQ(api_error::download_timeout, dm.download_file(1, fsi));

      dm.stop();
      EXPECT_EQ(0, dm.get_download_count());
      nf->close();

      utils::file::delete_file(fsi.source_path);

      utils::file::delete_file(test_source);
    }

    event_system::instance().stop();
  }

  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
}

TEST(downloader_manager, download_pause_resume) {
  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
  {
    app_config config(provider_type::sia, "./chunk_data");
    console_consumer c;
    event_system::instance().start();

    event_capture ec(
        {
            "download_begin",
            "download_end",
            "download_progress",
            "download_paused",
            "download_resumed",
        },
        {"download_timeout"});
    const auto test_source = get_source_file_name();
    utils::file::delete_file(test_source);

    const auto chunk_size = utils::encryption::encrypting_reader::get_data_chunk_size();
    const auto file_size = (chunk_size * 50u);
    const auto test_dest = get_source_file_name();
    utils::file::delete_file(test_dest);

    auto nf = create_random_file(test_source, file_size);
    EXPECT_NE(nf, nullptr);
    if (nf) {
      api_reader_callback api_reader = [&](const std::string &, const std::size_t &size,
                                           const std::uint64_t &offset, std::vector<char> &data,
                                           const bool &) -> api_error {
        data.resize(size);
        std::size_t bytes_read{};
        auto ret = nf->read_bytes(&data[0u], data.size(), offset, bytes_read) ? api_error::success
                                                                              : api_error::os_error;
        EXPECT_EQ(api_error::success, ret);
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      };

      filesystem_item fsi{};
      fsi.api_path = "/test_chunk";
      fsi.directory = false;
      fsi.lock = std::make_shared<std::recursive_mutex>();
      fsi.source_path = test_dest;
      fsi.size = file_size;

      download_manager dm(config, api_reader, true);
      mock_open_file_table oft(&dm, &fsi);
      EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(1ull));
      dm.start(&oft);

      std::thread th([&]() { EXPECT_EQ(api_error::success, dm.download_file(1, fsi)); });
      EXPECT_TRUE(ec.wait_for_event("download_begin"));
      EXPECT_TRUE(dm.pause_download(fsi.api_path));

      dm.rename_download(fsi.api_path, fsi.api_path + "_cow");

      fsi.api_path += "_cow";
      dm.resume_download(fsi.api_path);
      th.join();

      std::uint64_t source_size;
      EXPECT_TRUE(utils::file::get_file_size(test_source, source_size));
      std::uint64_t current_size;
      EXPECT_TRUE(utils::file::get_file_size(fsi.source_path, current_size));
      EXPECT_EQ(source_size, current_size);

      EXPECT_STRCASEEQ(utils::file::generate_sha256(test_source).c_str(),
                       utils::file::generate_sha256(fsi.source_path).c_str());

      dm.stop();
      EXPECT_EQ(0, dm.get_download_count());
      nf->close();

      utils::file::delete_file(fsi.source_path);
    }

    utils::file::delete_file(test_source);
    event_system::instance().stop();
  }

  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
}

TEST(downloader_manager, store_and_resume_incomplete_download_after_write) {
  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
  {
    app_config config(provider_type::sia, "./chunk_data");
    console_consumer c;
    event_system::instance().start();

    const auto test_source = get_source_file_name();
    utils::file::delete_file(test_source);

    const auto chunk_size = utils::encryption::encrypting_reader::get_data_chunk_size();
    const auto file_size = (chunk_size * 10u);
    const auto test_dest = utils::path::absolute("./test_chunk_dest");
    utils::file::delete_file(test_dest);

    auto nf = create_random_file(test_source, file_size);
    EXPECT_NE(nf, nullptr);
    if (nf) {
      api_reader_callback api_reader = [&](const std::string &, const std::size_t &size,
                                           const std::uint64_t &offset, std::vector<char> &data,
                                           const bool &) -> api_error {
        data.resize(size);
        std::size_t bytes_read{};
        auto ret = nf->read_bytes(&data[0u], data.size(), offset, bytes_read) ? api_error::success
                                                                              : api_error::os_error;
        EXPECT_EQ(api_error::success, ret);
        EXPECT_EQ(bytes_read, data.size());
        std::this_thread::sleep_for(2s);
        return ret;
      };

      filesystem_item fsi{};
      fsi.api_path = "/test_chunk";
      fsi.directory = false;
      fsi.lock = std::make_shared<std::recursive_mutex>();
      fsi.source_path = test_dest;
      fsi.size = file_size;

      download_manager dm(config, api_reader, true);

      mock_open_file_table oft(&dm, &fsi);
      EXPECT_CALL(oft, get_open_count(_)).WillRepeatedly(Return(0ull));
      filesystem_item fsi2{};
      EXPECT_CALL(oft, force_schedule_upload(_)).WillOnce(DoAll(SaveArg<0>(&fsi2), Return()));
      EXPECT_CALL(oft, open(_, _))
          .WillOnce(DoAll(SetArgReferee<1>(1u), Return(api_error::success)));
      EXPECT_CALL(oft, close(1u)).WillOnce(Return());

      {
        event_capture ec({
            "download_begin",
            "download_end",
            "download_progress",
            "download_stored",
        });

        dm.start(&oft);

        std::size_t bytes_written{};
        EXPECT_EQ(api_error::success,
                  dm.write_bytes(1, fsi, chunk_size - 2u, {'a', 'a', 'a', 'a'}, bytes_written));

        dm.stop();
        ec.wait_for_event("download_stored");
      }
      {
        event_capture ec({
            "download_begin",
            "download_end",
            "download_progress",
            "download_restored",
        });

        dm.start(&oft);

        ec.wait_for_event("download_restored");
        ec.wait_for_event("download_end");

        dm.stop();
      }

      nf->close();

      utils::file::delete_file(fsi.source_path);
    }

    utils::file::delete_file(test_source);
    event_system::instance().stop();
  }

  utils::file::delete_directory_recursively(utils::path::absolute("./chunk_data"));
}
} // namespace repertory
