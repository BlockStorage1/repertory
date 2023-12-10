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

#include "app_config.hpp"
#include "file_manager/events.hpp"
#include "file_manager/file_manager.hpp"
#include "file_manager/i_open_file.hpp"
#include "mocks/mock_open_file.hpp"
#include "mocks/mock_provider.hpp"
#include "mocks/mock_upload_manager.hpp"
#include "platform/platform.hpp"
#include "types/repertory.hpp"
#include "utils/event_capture.hpp"
#include "utils/file_utils.hpp"
#include "utils/native_file.hpp"
#include "utils/path_utils.hpp"
#include "utils/polling.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
#ifdef REPERTORY_TESTING
auto file_manager::open(std::shared_ptr<i_closeable_open_file> of,
                        const open_file_data &ofd, std::uint64_t &handle,
                        std::shared_ptr<i_open_file> &f) -> api_error {
  return open(of->get_api_path(), of->is_directory(), ofd, handle, f, of);
}
#endif

TEST(file_manager, can_start_and_stop) {
  {
    console_consumer c;
    event_system::instance().start();

    app_config cfg(provider_type::sia, "./fm_test");
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    event_consumer es("service_started", [](const event &e) {
      const auto &ee = dynamic_cast<const service_started &>(e);
      EXPECT_STREQ("file_manager", ee.get_service().get<std::string>().c_str());
    });
    event_consumer es2("service_shutdown_begin", [](const event &e) {
      const auto &ee = dynamic_cast<const service_shutdown_begin &>(e);
      EXPECT_STREQ("file_manager", ee.get_service().get<std::string>().c_str());
    });
    event_consumer es3("service_shutdown_end", [](const event &e) {
      const auto &ee = dynamic_cast<const service_shutdown_end &>(e);
      EXPECT_STREQ("file_manager", ee.get_service().get<std::string>().c_str());
    });

    event_capture ec(
        {"service_started", "service_shutdown_begin", "service_shutdown_end"});

    file_manager fm(cfg, mp);
    fm.start();
    fm.stop();

    ec.wait_for_empty();
  }

  event_system::instance().stop();
  // EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, can_create_and_close_file) {
  {
    console_consumer c;
    event_system::instance().start();

    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));
    polling::instance().start(&cfg);

    file_manager fm(cfg, mp);
    fm.start();

    event_capture capture({
        "item_timeout",
        "filesystem_item_opened",
        "filesystem_item_handle_opened",
        "filesystem_item_handle_closed",
        "filesystem_item_closed",
    });

    const auto source_path = utils::path::combine(
        cfg.get_cache_directory(), {utils::create_uuid_string()});

    std::uint64_t handle{};
    {
      std::shared_ptr<i_open_file> f;

      const auto now = utils::get_file_time_now();
      auto meta = create_meta_attributes(
          now, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE, now + 1u,
          now + 2u, false, 1, "key", 2, now + 3u, 3u, 4u, 0u, source_path, 10,
          now + 4u);

      EXPECT_CALL(mp, create_file("/test_create.txt", meta))
          .WillOnce(Return(api_error::success));
      EXPECT_CALL(mp, get_filesystem_item)
          .WillOnce([&meta](const std::string &api_path, bool directory,
                            filesystem_item &fsi) -> api_error {
            EXPECT_STREQ("/test_create.txt", api_path.c_str());
            EXPECT_FALSE(directory);
            fsi.api_path = api_path;
            fsi.api_parent = utils::path::get_parent_api_path(api_path);
            fsi.directory = directory;
            fsi.size = utils::string::to_uint64(meta[META_SIZE]);
            fsi.source_path = meta[META_SOURCE];
            return api_error::success;
          });

      event_consumer ec("filesystem_item_opened", [&](const event &e) {
        const auto &ee = dynamic_cast<const filesystem_item_opened &>(e);
        EXPECT_STREQ("/test_create.txt",
                     ee.get_api_path().get<std::string>().c_str());
        EXPECT_STREQ(source_path.c_str(),
                     ee.get_source().get<std::string>().c_str());
        EXPECT_STREQ("0", ee.get_directory().get<std::string>().c_str());
      });

      event_consumer ec2("filesystem_item_handle_opened", [&](const event &e) {
        const auto &ee = dynamic_cast<const filesystem_item_handle_opened &>(e);
        EXPECT_STREQ("/test_create.txt",
                     ee.get_api_path().get<std::string>().c_str());
        EXPECT_STREQ(source_path.c_str(),
                     ee.get_source().get<std::string>().c_str());
        EXPECT_STREQ("0", ee.get_directory().get<std::string>().c_str());
        EXPECT_STREQ("1", ee.get_handle().get<std::string>().c_str());
      });

#ifdef _WIN32
      EXPECT_EQ(api_error::success,
                fm.create("/test_create.txt", meta, {}, handle, f));
#else
      EXPECT_EQ(api_error::success,
                fm.create("/test_create.txt", meta, O_RDWR, handle, f));
#endif
      EXPECT_EQ(std::size_t(1u), fm.get_open_file_count());
      EXPECT_EQ(std::size_t(1u), fm.get_open_handle_count());
      EXPECT_EQ(std::uint64_t(1u), handle);
    }

    event_consumer ec3("filesystem_item_closed", [&](const event &e) {
      const auto &ee = dynamic_cast<const filesystem_item_closed &>(e);
      EXPECT_STREQ("/test_create.txt",
                   ee.get_api_path().get<std::string>().c_str());
      EXPECT_STREQ(source_path.c_str(),
                   ee.get_source().get<std::string>().c_str());
      EXPECT_STREQ("0", ee.get_directory().get<std::string>().c_str());
    });

    event_consumer ec4("filesystem_item_handle_closed", [&](const event &e) {
      const auto &ee = dynamic_cast<const filesystem_item_handle_closed &>(e);
      EXPECT_STREQ("/test_create.txt",
                   ee.get_api_path().get<std::string>().c_str());
      EXPECT_STREQ(source_path.c_str(),
                   ee.get_source().get<std::string>().c_str());
      EXPECT_STREQ("0", ee.get_directory().get<std::string>().c_str());
      EXPECT_STREQ("1", ee.get_handle().get<std::string>().c_str());
    });

    fm.close(handle);

    EXPECT_EQ(std::size_t(1u), fm.get_open_file_count());
    EXPECT_EQ(std::size_t(0u), fm.get_open_handle_count());

    capture.wait_for_empty();
    EXPECT_EQ(std::size_t(0u), fm.get_open_file_count());

    fm.stop();
  }

  polling::instance().stop();
  event_system::instance().stop();
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, can_open_and_close_file) {
  {
    console_consumer c;
    event_system::instance().start();

    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    polling::instance().start(&cfg);
    file_manager fm(cfg, mp);
    fm.start();

    event_capture capture({
        "item_timeout",
        "filesystem_item_opened",
        "filesystem_item_handle_opened",
        "filesystem_item_handle_closed",
        "filesystem_item_closed",
    });
    const auto source_path = utils::path::combine(
        cfg.get_cache_directory(), {utils::create_uuid_string()});

    std::uint64_t handle{};
    {

      const auto now = utils::get_file_time_now();
      auto meta = create_meta_attributes(
          now, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE, now + 1u,
          now + 2u, false, 1, "key", 2, now + 3u, 3u, 4u, 0u, source_path, 10,
          now + 4u);

      EXPECT_CALL(mp, create_file).Times(0u);

      EXPECT_CALL(mp, get_filesystem_item)
          .WillOnce([&meta](const std::string &api_path, bool directory,
                            filesystem_item &fsi) -> api_error {
            EXPECT_STREQ("/test_open.txt", api_path.c_str());
            EXPECT_FALSE(directory);
            fsi.api_path = api_path;
            fsi.api_parent = utils::path::get_parent_api_path(api_path);
            fsi.directory = directory;
            fsi.size = utils::string::to_uint64(meta[META_SIZE]);
            fsi.source_path = meta[META_SOURCE];
            return api_error::success;
          });

      event_consumer ec("filesystem_item_opened", [&](const event &e) {
        const auto &ee = dynamic_cast<const filesystem_item_opened &>(e);
        EXPECT_STREQ("/test_open.txt",
                     ee.get_api_path().get<std::string>().c_str());
        EXPECT_STREQ(source_path.c_str(),
                     ee.get_source().get<std::string>().c_str());
        EXPECT_STREQ("0", ee.get_directory().get<std::string>().c_str());
      });

      event_consumer ec2("filesystem_item_handle_opened", [&](const event &e) {
        const auto &ee = dynamic_cast<const filesystem_item_handle_opened &>(e);
        EXPECT_STREQ("/test_open.txt",
                     ee.get_api_path().get<std::string>().c_str());
        EXPECT_STREQ(source_path.c_str(),
                     ee.get_source().get<std::string>().c_str());
        EXPECT_STREQ("0", ee.get_directory().get<std::string>().c_str());
        EXPECT_STREQ("1", ee.get_handle().get<std::string>().c_str());
      });

      std::shared_ptr<i_open_file> f;
#ifdef _WIN32
      EXPECT_EQ(api_error::success,
                fm.open("/test_open.txt", false, {}, handle, f));
#else
      EXPECT_EQ(api_error::success,
                fm.open("/test_open.txt", false, O_RDWR, handle, f));
#endif
      EXPECT_EQ(std::size_t(1u), fm.get_open_file_count());
      EXPECT_EQ(std::size_t(1u), fm.get_open_handle_count());
      EXPECT_EQ(std::uint64_t(1u), handle);
    }

    event_consumer ec3("filesystem_item_closed", [&](const event &e) {
      const auto &ee = dynamic_cast<const filesystem_item_closed &>(e);
      EXPECT_STREQ("/test_open.txt",
                   ee.get_api_path().get<std::string>().c_str());
      EXPECT_STREQ(source_path.c_str(),
                   ee.get_source().get<std::string>().c_str());
      EXPECT_STREQ("0", ee.get_directory().get<std::string>().c_str());
    });

    event_consumer ec4("filesystem_item_handle_closed", [&](const event &e) {
      const auto &ee = dynamic_cast<const filesystem_item_handle_closed &>(e);
      EXPECT_STREQ("/test_open.txt",
                   ee.get_api_path().get<std::string>().c_str());
      EXPECT_STREQ(source_path.c_str(),
                   ee.get_source().get<std::string>().c_str());
      EXPECT_STREQ("0", ee.get_directory().get<std::string>().c_str());
      EXPECT_STREQ("1", ee.get_handle().get<std::string>().c_str());
    });

    fm.close(handle);

    EXPECT_EQ(std::size_t(1u), fm.get_open_file_count());
    EXPECT_EQ(std::size_t(0u), fm.get_open_handle_count());

    capture.wait_for_empty();
    EXPECT_EQ(std::size_t(0u), fm.get_open_file_count());

    fm.stop();
  }

  polling::instance().stop();
  event_system::instance().stop();
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, can_open_and_close_multiple_handles_for_same_file) {
  {
    console_consumer c;
    event_system::instance().start();

    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    polling::instance().start(&cfg);
    file_manager fm(cfg, mp);
    fm.start();

    {
      const auto source_path = utils::path::combine(
          cfg.get_cache_directory(), {utils::create_uuid_string()});

      const auto now = utils::get_file_time_now();
      auto meta = create_meta_attributes(
          now, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE, now + 1u,
          now + 2u, false, 1, "key", 2, now + 3u, 3u, 4u, 0u, source_path, 10,
          now + 4u);

      EXPECT_CALL(mp, create_file).Times(0u);

      EXPECT_CALL(mp, get_filesystem_item)
          .WillOnce([&meta](const std::string &api_path, bool directory,
                            filesystem_item &fsi) -> api_error {
            EXPECT_STREQ("/test_open.txt", api_path.c_str());
            EXPECT_FALSE(directory);
            fsi.api_path = api_path;
            fsi.api_parent = utils::path::get_parent_api_path(api_path);
            fsi.directory = directory;
            fsi.size = utils::string::to_uint64(meta[META_SIZE]);
            fsi.source_path = meta[META_SOURCE];
            return api_error::success;
          });

      std::array<std::uint64_t, 4u> handles;
      for (std::uint8_t i = 0u; i < handles.size(); i++) {
        std::shared_ptr<i_open_file> f;
#ifdef _WIN32
        EXPECT_EQ(api_error::success,
                  fm.open("/test_open.txt", false, {}, handles[i], f));
#else
        EXPECT_EQ(api_error::success,
                  fm.open("/test_open.txt", false, O_RDWR, handles[i], f));
#endif

        EXPECT_EQ(std::size_t(1u), fm.get_open_file_count());
        EXPECT_EQ(std::size_t(i + 1u), fm.get_open_handle_count());
        EXPECT_EQ(std::uint64_t(i + 1u), handles[i]);
      }

      for (std::uint8_t i = 0u; i < handles.size(); i++) {
        EXPECT_EQ(std::size_t(1u), fm.get_open_file_count());
        EXPECT_EQ(std::size_t(handles.size() - i), fm.get_open_handle_count());
        fm.close(handles[i]);
      }
      fm.stop();
    }

    EXPECT_EQ(std::size_t(0u), fm.get_open_file_count());
    EXPECT_EQ(std::size_t(0u), fm.get_open_handle_count());
  }

  polling::instance().stop();
  event_system::instance().stop();
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, download_is_stored_after_write_if_partially_downloaded) {
  {
    console_consumer c;
    event_system::instance().start();

    app_config cfg(provider_type::sia, "./fm_test");
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);
    fm.start();

    const auto source_path = utils::path::combine(
        cfg.get_cache_directory(), {utils::create_uuid_string()});

    event_consumer es("download_stored", [&source_path](const event &e) {
      const auto &ee = dynamic_cast<const download_stored &>(e);
      EXPECT_STREQ("/test_write_partial_download.txt",
                   ee.get_api_path().get<std::string>().c_str());
      EXPECT_STREQ(source_path.c_str(),
                   ee.get_dest_path().get<std::string>().c_str());
    });
    event_capture ec({"download_stored"},
                     {"file_upload_completed", "file_upload_queued"});

    const auto now = utils::get_file_time_now();
    auto meta = create_meta_attributes(
        now, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE, now + 1u, now + 2u,
        false, 1, "key", 2, now + 3u, 3u, 4u,
        utils::encryption::encrypting_reader::get_data_chunk_size() * 4u,
        source_path, 10, now + 4u);
    auto nf = create_random_file(generate_test_file_name(".", "test_src"),
                                 utils::string::to_uint64(meta[META_SIZE]));

    EXPECT_CALL(mp, get_filesystem_item)
        .WillRepeatedly([&meta](const std::string &api_path, bool directory,
                                filesystem_item &fsi) -> api_error {
          EXPECT_STREQ("/test_write_partial_download.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          fsi.api_path = api_path;
          fsi.api_parent = utils::path::get_parent_api_path(api_path);
          fsi.directory = directory;
          fsi.size = utils::string::to_uint64(meta[META_SIZE]);
          fsi.source_path = meta[META_SOURCE];
          return api_error::success;
        });

    std::uint64_t handle{};
    std::shared_ptr<i_open_file> f;
#ifdef _WIN32
    EXPECT_EQ(api_error::success, fm.open("/test_write_partial_download.txt",
                                          false, {}, handle, f));
#else
    EXPECT_EQ(api_error::success, fm.open("/test_write_partial_download.txt",
                                          false, O_RDWR, handle, f));
#endif

    EXPECT_CALL(mp, read_file_bytes)
        .WillRepeatedly([&nf](const std::string & /* api_path */,
                              std::size_t size, std::uint64_t offset,
                              data_buffer &data,
                              stop_type &stop_requested) -> api_error {
          if (stop_requested) {
            return api_error::download_stopped;
          }

          if (offset == 0u) {
            std::size_t bytes_read{};
            data.resize(size);
            auto ret = nf->read_bytes(&data[0u], size, offset, bytes_read)
                           ? api_error::success
                           : api_error::os_error;
            EXPECT_EQ(bytes_read, data.size());
            return ret;
          }

          while (not stop_requested) {
            std::this_thread::sleep_for(100ms);
          }

          return api_error::download_stopped;
        });
    EXPECT_CALL(mp, set_item_meta("/test_write_partial_download.txt", _))
        .WillOnce(
            [](const std::string &, const api_meta_map &meta2) -> api_error {
              EXPECT_NO_THROW(EXPECT_FALSE(meta2.at(META_CHANGED).empty()));
              EXPECT_NO_THROW(EXPECT_FALSE(meta2.at(META_MODIFIED).empty()));
              EXPECT_NO_THROW(EXPECT_FALSE(meta2.at(META_WRITTEN).empty()));
              return api_error::success;
            });
    EXPECT_CALL(mp, upload_file).Times(0u);

    std::size_t bytes_written{};
    data_buffer data = {0, 1, 2};
    EXPECT_EQ(api_error::success, f->write(0u, data, bytes_written));
    EXPECT_EQ(std::size_t(3u), bytes_written);
    f.reset();

    fm.close(handle);

    EXPECT_EQ(std::size_t(1u), fm.get_open_file_count());
    EXPECT_EQ(std::size_t(0u), fm.get_open_handle_count());

    fm.stop();
    ec.wait_for_empty();

    event_capture ec2({"download_restored", "download_stored"},
                      {"file_upload_completed", "file_upload_queued"});
    EXPECT_EQ(std::size_t(0u), fm.get_open_file_count());
    EXPECT_EQ(std::size_t(0u), fm.get_open_handle_count());

    auto stored_downloads = fm.get_stored_downloads();
    EXPECT_EQ(std::size_t(1u), stored_downloads.size());
    std::cout << stored_downloads[0u].dump(2) << std::endl;

    EXPECT_STREQ("/test_write_partial_download.txt",
                 stored_downloads[0u]["path"].get<std::string>().c_str());
    EXPECT_EQ(utils::encryption::encrypting_reader::get_data_chunk_size(),
              stored_downloads[0u]["chunk_size"].get<std::size_t>());
    auto read_state = utils::string::to_dynamic_bitset(
        stored_downloads[0u]["read_state"].get<std::string>());
    EXPECT_TRUE(read_state[0u]);
    for (std::size_t i = 1u; i < read_state.size(); i++) {
      EXPECT_FALSE(read_state[i]);
    }
    EXPECT_STREQ(source_path.c_str(),
                 stored_downloads[0u]["source"].get<std::string>().c_str());

    fm.start();

    event_consumer es2("download_restored", [&source_path](const event &e) {
      const auto &ee = dynamic_cast<const download_restored &>(e);
      EXPECT_STREQ("/test_write_partial_download.txt",
                   ee.get_api_path().get<std::string>().c_str());
      EXPECT_STREQ(source_path.c_str(),
                   ee.get_dest_path().get<std::string>().c_str());
    });

    EXPECT_EQ(std::size_t(1u), fm.get_open_file_count());
    EXPECT_EQ(std::size_t(0u), fm.get_open_handle_count());
    fm.stop();
    ec2.wait_for_empty();

    EXPECT_EQ(std::size_t(0u), fm.get_open_file_count());
    EXPECT_EQ(std::size_t(0u), fm.get_open_handle_count());

    nf->close();
  }

  event_system::instance().stop();
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, upload_occurs_after_write_if_fully_downloaded) {
  {
    console_consumer c;
    event_system::instance().start();

    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    polling::instance().start(&cfg);
    file_manager fm(cfg, mp);
    fm.start();

    const auto source_path = utils::path::combine(
        cfg.get_cache_directory(), {utils::create_uuid_string()});

    event_consumer es("file_upload_queued", [&source_path](const event &e) {
      const auto &ee = dynamic_cast<const file_upload_queued &>(e);
      EXPECT_STREQ("/test_write_full_download.txt",
                   ee.get_api_path().get<std::string>().c_str());
      EXPECT_STREQ(source_path.c_str(),
                   ee.get_source().get<std::string>().c_str());
    });
    event_consumer es2("file_upload_completed", [&source_path](const event &e) {
      const auto &ee = dynamic_cast<const file_upload_completed &>(e);
      EXPECT_STREQ("/test_write_full_download.txt",
                   ee.get_api_path().get<std::string>().c_str());
      EXPECT_STREQ(source_path.c_str(),
                   ee.get_source().get<std::string>().c_str());
    });
    event_capture ec({"download_end"});

    const auto now = utils::get_file_time_now();
    auto meta = create_meta_attributes(
        now, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE, now + 1u, now + 2u,
        false, 1, "key", 2, now + 3u, 3u, 4u,
        utils::encryption::encrypting_reader::get_data_chunk_size() * 4u,
        source_path, 10, now + 4u);
    auto nf = create_random_file(generate_test_file_name(".", "test_src"),
                                 utils::string::to_uint64(meta[META_SIZE]));

    EXPECT_CALL(mp, get_filesystem_item)
        .WillRepeatedly([&meta](const std::string &api_path, bool directory,
                                filesystem_item &fsi) -> api_error {
          EXPECT_STREQ("/test_write_full_download.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          fsi.api_path = api_path;
          fsi.api_parent = utils::path::get_parent_api_path(api_path);
          fsi.directory = directory;
          fsi.size = utils::string::to_uint64(meta[META_SIZE]);
          fsi.source_path = meta[META_SOURCE];
          return api_error::success;
        });

    std::uint64_t handle{};
    std::shared_ptr<i_open_file> f;
#ifdef _WIN32
    EXPECT_EQ(api_error::success,
              fm.open("/test_write_full_download.txt", false, {}, handle, f));
#else
    EXPECT_EQ(api_error::success, fm.open("/test_write_full_download.txt",
                                          false, O_RDWR, handle, f));
#endif

    EXPECT_CALL(mp, read_file_bytes)
        .WillRepeatedly([&nf](const std::string & /* api_path */,
                              std::size_t size, std::uint64_t offset,
                              data_buffer &data,
                              stop_type & /* stop_requested */) -> api_error {
          std::size_t bytes_read{};
          data.resize(size);
          auto ret = nf->read_bytes(&data[0u], size, offset, bytes_read)
                         ? api_error::success
                         : api_error::os_error;
          EXPECT_EQ(bytes_read, data.size());
          return ret;
        });
    EXPECT_CALL(mp, set_item_meta("/test_write_full_download.txt", _))
        .WillOnce(
            [](const std::string &, const api_meta_map &meta2) -> api_error {
              EXPECT_NO_THROW(EXPECT_FALSE(meta2.at(META_CHANGED).empty()));
              EXPECT_NO_THROW(EXPECT_FALSE(meta2.at(META_MODIFIED).empty()));
              EXPECT_NO_THROW(EXPECT_FALSE(meta2.at(META_WRITTEN).empty()));
              return api_error::success;
            });
    std::size_t bytes_written{};
    data_buffer data = {0, 1, 2};
    EXPECT_EQ(api_error::success, f->write(0u, data, bytes_written));
    EXPECT_EQ(std::size_t(3u), bytes_written);
    f.reset();

    ec.wait_for_empty();

    EXPECT_CALL(mp,
                upload_file("/test_write_full_download.txt", source_path, _))
        .WillOnce(Return(api_error::success));

    event_capture ec2(
        {"item_timeout", "file_upload_queued", "file_upload_completed"});
    fm.close(handle);

    ec2.wait_for_empty();

    EXPECT_EQ(std::size_t(0U), fm.get_open_file_count());
    EXPECT_EQ(std::size_t(0U), fm.get_open_handle_count());

    fm.stop();

    nf->close();
  }

  polling::instance().stop();
  event_system::instance().stop();
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, can_evict_file) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
  {
    console_consumer c;
    event_system::instance().start();

    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);
    fm.start();

    event_capture capture({
        "filesystem_item_opened",
        "filesystem_item_handle_opened",
        "filesystem_item_handle_closed",
        "filesystem_item_closed",
        "file_upload_completed",
    });

    const auto source_path = utils::path::combine(
        cfg.get_cache_directory(), {utils::create_uuid_string()});

    const auto now = utils::get_file_time_now();

    auto meta = create_meta_attributes(
        now, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE, now + 1u, now + 2u,
        false, 1, "key", 2, now + 3u, 3u, 4u, 0u, source_path, 10, now + 4u);
    std::uint64_t handle{};
    {
      std::shared_ptr<i_open_file> f;

      EXPECT_CALL(mp, create_file("/test_evict.txt", meta))
          .WillOnce(Return(api_error::success));
      EXPECT_CALL(mp, get_filesystem_item)
          .WillRepeatedly([&meta](const std::string &api_path, bool directory,
                                  filesystem_item &fsi) -> api_error {
            EXPECT_STREQ("/test_evict.txt", api_path.c_str());
            EXPECT_FALSE(directory);
            fsi.api_path = api_path;
            fsi.api_parent = utils::path::get_parent_api_path(api_path);
            fsi.directory = directory;
            fsi.size = utils::string::to_uint64(meta[META_SIZE]);
            fsi.source_path = meta[META_SOURCE];
            return api_error::success;
          });

#ifdef _WIN32
      EXPECT_EQ(api_error::success,
                fm.create("/test_evict.txt", meta, {}, handle, f));
#else
      EXPECT_EQ(api_error::success,
                fm.create("/test_evict.txt", meta, O_RDWR, handle, f));
#endif
      EXPECT_CALL(mp, set_item_meta("/test_evict.txt", _))
          .Times(2)
          .WillRepeatedly(Return(api_error::success));
      EXPECT_CALL(mp, upload_file(_, _, _))
          .WillOnce(Return(api_error::success));

      data_buffer data{{0, 1, 1}};
      std::size_t bytes_written{};
      EXPECT_EQ(api_error::success, f->write(0U, data, bytes_written));

      std::uint64_t file_size{};
      EXPECT_TRUE(utils::file::get_file_size(source_path, file_size));
      EXPECT_EQ(static_cast<std::uint64_t>(data.size()), file_size);
    }

    fm.close(handle);
    capture.wait_for_empty();

    EXPECT_TRUE(utils::retryable_action(
        [&fm]() -> bool { return not fm.is_processing("/test_evict.txt"); }));

    EXPECT_CALL(mp, get_item_meta(_, META_SOURCE, _))
        .WillOnce([&source_path](const std::string &api_path,
                                 const std::string &key,
                                 std::string &value) -> api_error {
          EXPECT_STREQ("/test_evict.txt", api_path.c_str());
          EXPECT_STREQ(META_SOURCE.c_str(), key.c_str());
          value = source_path;
          return api_error::success;
        });
    EXPECT_CALL(mp, get_item_meta(_, META_PINNED, _))
        .WillOnce([](const std::string &api_path, const std::string &key,
                     std::string &value) -> api_error {
          EXPECT_STREQ("/test_evict.txt", api_path.c_str());
          EXPECT_STREQ(META_PINNED.c_str(), key.c_str());
          value = "0";
          return api_error::success;
        });
    EXPECT_TRUE(fm.evict_file("/test_evict.txt"));
    EXPECT_FALSE(utils::file::is_file(source_path));

    fm.stop();
  }

  event_system::instance().stop();
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, evict_file_fails_if_file_is_pinned) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));
    file_manager fm(cfg, mp);

    EXPECT_CALL(mp, get_item_meta(_, META_PINNED, _))
        .WillOnce([](const std::string &api_path, const std::string &key,
                     std::string &value) -> api_error {
          EXPECT_STREQ("/test_open.txt", api_path.c_str());
          EXPECT_STREQ(META_PINNED.c_str(), key.c_str());
          value = "1";
          return api_error::success;
        });

    EXPECT_FALSE(fm.evict_file("/test_open.txt"));
  }

  event_system::instance().stop();
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, evict_file_fails_if_provider_is_direct_only) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(true));
    file_manager fm(cfg, mp);
    EXPECT_FALSE(fm.evict_file("/test.txt"));
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, evict_file_fails_if_file_is_open) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));
    file_manager fm(cfg, mp);

    EXPECT_CALL(mp, get_filesystem_item)
        .WillOnce([](const std::string &api_path, bool directory,
                     filesystem_item &fsi) -> api_error {
          EXPECT_STREQ("/test_open.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          fsi.api_path = api_path;
          fsi.api_parent = utils::path::get_parent_api_path(api_path);
          fsi.directory = directory;
          fsi.size = 0U;
          return api_error::success;
        });

    EXPECT_CALL(mp, set_item_meta(_, _, _))
        .WillOnce([](const std::string &api_path, const std::string &key,
                     const std::string &value) -> api_error {
          EXPECT_STREQ("/test_open.txt", api_path.c_str());
          EXPECT_STREQ(META_SOURCE.c_str(), key.c_str());
          EXPECT_FALSE(value.empty());
          return api_error::success;
        });

    std::uint64_t handle{};
    std::shared_ptr<i_open_file> f{};
#ifdef _WIN32
    EXPECT_EQ(api_error::success,
              fm.open("/test_open.txt", false, {}, handle, f));
#else
    EXPECT_EQ(api_error::success,
              fm.open("/test_open.txt", false, O_RDWR, handle, f));
#endif

    EXPECT_FALSE(fm.evict_file("/test_open.txt"));

    fm.close(handle);
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager,
     evict_file_fails_if_unable_to_get_source_path_from_item_meta) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));
    file_manager fm(cfg, mp);

    EXPECT_CALL(mp, get_item_meta(_, META_SOURCE, _))
        .WillOnce([](const std::string &api_path, const std::string &key,
                     std::string & /*value*/) -> api_error {
          EXPECT_STREQ("/test_open.txt", api_path.c_str());
          EXPECT_STREQ(META_SOURCE.c_str(), key.c_str());
          return api_error::error;
        });

    EXPECT_CALL(mp, get_item_meta(_, META_PINNED, _))
        .WillOnce([](const std::string &api_path, const std::string &key,
                     std::string &value) -> api_error {
          EXPECT_STREQ("/test_open.txt", api_path.c_str());
          EXPECT_STREQ(META_PINNED.c_str(), key.c_str());
          value = "0";
          return api_error::success;
        });

    EXPECT_FALSE(fm.evict_file("/test_open.txt"));
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, evict_file_fails_if_source_path_is_empty) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));
    file_manager fm(cfg, mp);

    EXPECT_CALL(mp, get_item_meta(_, META_SOURCE, _))
        .WillOnce([](const std::string &api_path, const std::string &key,
                     std::string &value) -> api_error {
          EXPECT_STREQ("/test_open.txt", api_path.c_str());
          EXPECT_STREQ(META_SOURCE.c_str(), key.c_str());
          value = "";
          return api_error::success;
        });
    EXPECT_CALL(mp, get_item_meta(_, META_PINNED, _))
        .WillOnce([](const std::string &api_path, const std::string &key,
                     std::string &value) -> api_error {
          EXPECT_STREQ("/test_open.txt", api_path.c_str());
          EXPECT_STREQ(META_PINNED.c_str(), key.c_str());
          value = "0";
          return api_error::success;
        });

    EXPECT_FALSE(fm.evict_file("/test_open.txt"));
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, evict_file_fails_if_file_is_uploading) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
  {
    console_consumer c;
    event_system::instance().start();

    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);
    fm.start();

    event_capture capture({
        "filesystem_item_opened",
        "filesystem_item_handle_opened",
        "filesystem_item_handle_closed",
        "filesystem_item_closed",
        "file_upload_completed",
    });

    const auto source_path = utils::path::combine(
        cfg.get_cache_directory(), {utils::create_uuid_string()});

    const auto now = utils::get_file_time_now();

    auto meta = create_meta_attributes(
        now, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE, now + 1u, now + 2u,
        false, 1, "", 2, now + 3u, 3u, 4u, 0u, source_path, 10, now + 4u);
    std::uint64_t handle{};
    {
      std::shared_ptr<i_open_file> f;

      EXPECT_CALL(mp, create_file("/test_evict.txt", meta))
          .WillOnce(Return(api_error::success));
      EXPECT_CALL(mp, get_filesystem_item)
          .WillRepeatedly([&meta](const std::string &api_path, bool directory,
                                  filesystem_item &fsi) -> api_error {
            EXPECT_STREQ("/test_evict.txt", api_path.c_str());
            EXPECT_FALSE(directory);
            fsi.api_path = api_path;
            fsi.api_parent = utils::path::get_parent_api_path(api_path);
            fsi.directory = directory;
            fsi.size = utils::string::to_uint64(meta[META_SIZE]);
            fsi.source_path = meta[META_SOURCE];
            return api_error::success;
          });

#ifdef _WIN32
      EXPECT_EQ(api_error::success,
                fm.create("/test_evict.txt", meta, {}, handle, f));
#else
      EXPECT_EQ(api_error::success,
                fm.create("/test_evict.txt", meta, O_RDWR, handle, f));
#endif
      EXPECT_CALL(mp, set_item_meta("/test_evict.txt", _))
          .Times(2)
          .WillRepeatedly(Return(api_error::success));
      EXPECT_CALL(mp, upload_file)
          .WillOnce([](const std::string &api_path,
                       const std::string &source_path2,
                       stop_type & /*stop_requested*/) -> api_error {
            EXPECT_STREQ("/test_evict.txt", api_path.c_str());
            EXPECT_FALSE(source_path2.empty());
            std::this_thread::sleep_for(3s);
            return api_error::success;
          });

      data_buffer data{{0, 1, 1}};
      std::size_t bytes_written{};
      EXPECT_EQ(api_error::success, f->write(0U, data, bytes_written));

      std::uint64_t file_size{};
      EXPECT_TRUE(utils::file::get_file_size(source_path, file_size));
      EXPECT_EQ(static_cast<std::uint64_t>(data.size()), file_size);
      fm.close(handle);

      EXPECT_TRUE(utils::retryable_action(
          [&fm]() -> bool { return fm.is_processing("/test_evict.txt"); }));
      EXPECT_FALSE(fm.evict_file("/test_evict.txt"));
    }

    capture.wait_for_empty();

    EXPECT_TRUE(utils::file::is_file(source_path));

    fm.stop();
  }

  event_system::instance().stop();
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, evict_file_fails_if_file_is_in_upload_queue) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);

    mock_open_file of{};
    EXPECT_CALL(of, is_directory).WillRepeatedly(Return(false));
    EXPECT_CALL(of, get_api_path).WillRepeatedly(Return("/test_evict.txt"));
    EXPECT_CALL(of, get_source_path).WillRepeatedly(Return("/test_evict.src"));
    fm.queue_upload(of);

    EXPECT_TRUE(fm.is_processing("/test_evict.txt"));
    EXPECT_FALSE(fm.evict_file("/test_evict.txt"));
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, evict_file_fails_if_file_is_modified) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);
    EXPECT_CALL(mp, get_filesystem_item)
        .WillOnce([](const std::string &api_path, bool directory,
                     filesystem_item &fsi) -> api_error {
          EXPECT_STREQ("/test_evict.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          fsi.api_path = api_path;
          fsi.api_parent = utils::path::get_parent_api_path(api_path);
          fsi.directory = directory;
          fsi.size = 1U;
          fsi.source_path = "/test_evict.src";
          return api_error::success;
        });

    auto of = std::make_shared<mock_open_file>();
    EXPECT_CALL(*of, is_directory).WillOnce(Return(false));
    EXPECT_CALL(*of, add).WillOnce(Return());
    EXPECT_CALL(*of, get_api_path).WillRepeatedly(Return("/test_evict.txt"));
    EXPECT_CALL(*of, get_source_path).WillRepeatedly(Return("/test_evict.src"));
    EXPECT_CALL(*of, is_modified).Times(2).WillRepeatedly(Return(true));

    std::uint64_t handle{};
    std::shared_ptr<i_open_file> f;
#ifdef _WIN32
    EXPECT_EQ(api_error::success, fm.open(of, {}, handle, f));
#else
    EXPECT_EQ(api_error::success, fm.open(of, O_RDWR, handle, f));
#endif

    EXPECT_TRUE(fm.is_processing("/test_evict.txt"));
    EXPECT_FALSE(fm.evict_file("/test_evict.txt"));
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, evict_file_fails_if_file_is_not_complete) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);
    EXPECT_CALL(mp, get_filesystem_item)
        .WillOnce([](const std::string &api_path, bool directory,
                     filesystem_item &fsi) -> api_error {
          EXPECT_STREQ("/test_evict.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          fsi.api_path = api_path;
          fsi.api_parent = utils::path::get_parent_api_path(api_path);
          fsi.directory = directory;
          fsi.size = 1U;
          return api_error::success;
        });

    auto of = std::make_shared<mock_open_file>();
    EXPECT_CALL(*of, is_directory).WillOnce(Return(false));
    EXPECT_CALL(*of, add).WillOnce(Return());
    EXPECT_CALL(*of, get_api_path).WillRepeatedly(Return("/test_evict.txt"));
    EXPECT_CALL(*of, get_source_path).WillRepeatedly(Return("/test_evict.src"));
    EXPECT_CALL(*of, is_modified).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(*of, is_complete).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(mp, set_item_meta("/test_evict.txt", META_SOURCE, _))
        .WillOnce(Return(api_error::success));

    std::uint64_t handle{};
    std::shared_ptr<i_open_file> f;
#ifdef _WIN32
    EXPECT_EQ(api_error::success, fm.open(of, {}, handle, f));
#else
    EXPECT_EQ(api_error::success, fm.open(of, O_RDWR, handle, f));
#endif

    EXPECT_TRUE(fm.is_processing("/test_evict.txt"));
    EXPECT_FALSE(fm.evict_file("/test_evict.txt"));
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, can_get_directory_items) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);
    EXPECT_CALL(mp, get_directory_items)
        .WillOnce([](const std::string &api_path,
                     directory_item_list &list) -> api_error {
          EXPECT_STREQ("/", api_path.c_str());
          list.insert(list.begin(), directory_item{
                                        "..",
                                        "",
                                        true,
                                    });
          list.insert(list.begin(), directory_item{
                                        ".",
                                        "",
                                        true,
                                    });
          return api_error::success;
        });
    auto list = fm.get_directory_items("/");
    EXPECT_EQ(std::size_t(2U), list.size());
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, file_is_not_opened_if_provider_create_file_fails) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    const auto now = utils::get_file_time_now();
    auto meta = create_meta_attributes(
        now, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE, now + 1u, now + 2u,
        false, 1, "", 2, now + 3u, 3u, 4u, 0u, "/test_create.src", 10,
        now + 4u);
    file_manager fm(cfg, mp);

    EXPECT_CALL(mp, create_file("/test_create.txt", meta))
        .WillOnce(Return(api_error::error));

    std::uint64_t handle{};
    std::shared_ptr<i_open_file> f;
#ifdef _WIN32
    EXPECT_EQ(api_error::error,
              fm.create("/test_create.txt", meta, {}, handle, f));
#else
    EXPECT_EQ(api_error::error,
              fm.create("/test_create.txt", meta, O_RDWR, handle, f));
#endif
    EXPECT_FALSE(f);
    EXPECT_EQ(std::size_t(0U), fm.get_open_file_count());
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, create_fails_if_provider_create_is_unsuccessful) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);

    EXPECT_CALL(mp, create_file("/test_create.txt", _))
        .WillOnce(Return(api_error::error));

    std::uint64_t handle{};
    std::shared_ptr<i_open_file> f{};
    api_meta_map meta{};
#ifdef _WIN32
    EXPECT_EQ(api_error::error,
              fm.create("/test_create.txt", meta, {}, handle, f));
#else
    EXPECT_EQ(api_error::error,
              fm.create("/test_create.txt", meta, O_RDWR, handle, f));
#endif
    EXPECT_EQ(std::size_t(0U), fm.get_open_file_count());
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, get_open_file_fails_if_file_is_not_open) {

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);

    std::shared_ptr<i_open_file> f{};
    EXPECT_FALSE(fm.get_open_file(0U, true, f));
    EXPECT_FALSE(f);

    EXPECT_FALSE(fm.get_open_file(0U, false, f));
    EXPECT_FALSE(f);
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager,
     get_open_file_promotes_non_writeable_file_if_writeable_is_specified) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);

    auto non_writeable = std::make_shared<mock_open_file>();
    EXPECT_CALL(*non_writeable, is_directory).WillOnce(Return(false));
    EXPECT_CALL(*non_writeable, add).WillOnce(Return());
    EXPECT_CALL(*non_writeable, get_api_path)
        .WillRepeatedly(Return("/test_open.txt"));
    EXPECT_CALL(*non_writeable, get_source_path)
        .WillRepeatedly(Return("/test_open.src"));
    EXPECT_CALL(*non_writeable, is_modified).WillRepeatedly(Return(true));
    EXPECT_CALL(*non_writeable, is_write_supported)
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*non_writeable, is_write_supported)
        .WillRepeatedly(Return(false));

    EXPECT_CALL(*non_writeable, get_filesystem_item)
        .WillOnce([api_path = "/test_open.txt"]() -> filesystem_item {
          filesystem_item fsi{};
          fsi.api_path = api_path;
          fsi.api_parent = utils::path::get_parent_api_path(api_path);
          fsi.directory = false;
          fsi.size = 10U;
          fsi.source_path = "/test_open.src";
          return fsi;
        });

    EXPECT_CALL(mp, get_filesystem_item)
        .WillOnce([](const std::string &api_path, bool directory,
                     filesystem_item &fsi) -> api_error {
          EXPECT_STREQ("/test_open.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          fsi.api_path = api_path;
          fsi.api_parent = utils::path::get_parent_api_path(api_path);
          fsi.directory = directory;
          fsi.size = 10U;
          fsi.source_path = "/test_open.src";
          return api_error::success;
        });

    std::uint64_t handle{};
    std::shared_ptr<i_open_file> f{};
#ifdef _WIN32
    EXPECT_EQ(api_error::success, fm.open(non_writeable, {}, handle, f));
    EXPECT_CALL(*non_writeable, get_open_data())
        .WillOnce([&handle]() -> std::map<std::uint64_t, open_file_data> {
          return {{handle, {}}};
        });
#else
    EXPECT_EQ(api_error::success, fm.open(non_writeable, O_RDWR, handle, f));
    EXPECT_CALL(*non_writeable, get_open_data())
        .WillOnce([&handle]() -> std::map<std::uint64_t, open_file_data> {
          return {{handle, O_RDWR}};
        });
#endif

    EXPECT_CALL(mp, set_item_meta("/test_open.txt", META_SOURCE, _))
        .WillOnce(Return(api_error::success));

    EXPECT_CALL(*non_writeable, has_handle(1)).WillOnce([]() -> bool {
      return true;
    });
    EXPECT_TRUE(fm.get_open_file(handle, true, f));
    EXPECT_NE(non_writeable.get(), f.get());
    EXPECT_EQ(std::size_t(1U), fm.get_open_file_count());

    std::shared_ptr<i_open_file> f2{};
    EXPECT_TRUE(fm.get_open_file(handle, false, f2));
    EXPECT_EQ(f.get(), f2.get());
    EXPECT_EQ(std::size_t(1U), fm.get_open_file_count());
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, open_file_fails_if_file_is_not_found) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);

    std::shared_ptr<i_open_file> f{};
    EXPECT_FALSE(fm.get_open_file(1U, true, f));
    EXPECT_EQ(std::size_t(0U), fm.get_open_file_count());
    EXPECT_FALSE(f);

    EXPECT_FALSE(fm.get_open_file(1U, false, f));
    EXPECT_FALSE(f);
    EXPECT_EQ(std::size_t(0U), fm.get_open_file_count());
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, open_file_fails_if_provider_get_filesystem_item_fails) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);

    auto of = std::make_shared<mock_open_file>();
    EXPECT_CALL(*of, is_directory).WillRepeatedly(Return(false));
    EXPECT_CALL(*of, get_api_path).WillRepeatedly(Return("/test_open.txt"));
    EXPECT_CALL(*of, get_source_path).WillRepeatedly(Return("/test_open.src"));

    EXPECT_CALL(mp, get_filesystem_item)
        .WillOnce([](const std::string &api_path, bool directory,
                     filesystem_item & /*fsi*/) -> api_error {
          EXPECT_STREQ("/test_open.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          return api_error::error;
        });

    std::uint64_t handle{};
    std::shared_ptr<i_open_file> f{};
#ifdef _WIN32
    EXPECT_EQ(api_error::error, fm.open(of, {}, handle, f));
#else
    EXPECT_EQ(api_error::error, fm.open(of, O_RDWR, handle, f));
#endif
    EXPECT_FALSE(fm.get_open_file(1U, true, f));
    EXPECT_EQ(std::size_t(0U), fm.get_open_file_count());
    EXPECT_FALSE(f);

    EXPECT_FALSE(fm.get_open_file(1U, false, f));
    EXPECT_FALSE(f);
    EXPECT_EQ(std::size_t(0U), fm.get_open_file_count());
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, open_file_fails_if_provider_set_item_meta_fails) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);

    auto of = std::make_shared<mock_open_file>();
    EXPECT_CALL(*of, is_directory).WillRepeatedly(Return(false));
    EXPECT_CALL(*of, get_api_path).WillRepeatedly(Return("/test_open.txt"));
    EXPECT_CALL(*of, get_source_path).WillRepeatedly(Return("/test_open.src"));

    EXPECT_CALL(mp, get_filesystem_item)
        .WillOnce([](const std::string &api_path, bool directory,
                     filesystem_item &fsi) -> api_error {
          EXPECT_STREQ("/test_open.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          fsi.api_path = api_path;
          fsi.api_parent = utils::path::get_parent_api_path(api_path);
          fsi.directory = directory;
          fsi.size = 0U;
          return api_error::success;
        });

    EXPECT_CALL(mp, set_item_meta("/test_open.txt", META_SOURCE, _))
        .WillOnce(Return(api_error::error));

    std::uint64_t handle{};
    std::shared_ptr<i_open_file> f{};
#ifdef _WIN32
    EXPECT_EQ(api_error::error, fm.open(of, {}, handle, f));
#else
    EXPECT_EQ(api_error::error, fm.open(of, O_RDWR, handle, f));
#endif
    EXPECT_FALSE(fm.get_open_file(1U, true, f));
    EXPECT_EQ(std::size_t(0U), fm.get_open_file_count());
    EXPECT_FALSE(f);

    EXPECT_FALSE(fm.get_open_file(1U, false, f));
    EXPECT_FALSE(f);
    EXPECT_EQ(std::size_t(0U), fm.get_open_file_count());
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, open_file_creates_source_path_if_empty) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);

    auto of = std::make_shared<mock_open_file>();
    EXPECT_CALL(*of, add).WillOnce(Return());
    EXPECT_CALL(*of, is_directory).WillRepeatedly(Return(false));
    EXPECT_CALL(*of, is_write_supported).WillRepeatedly(Return(true));
    EXPECT_CALL(*of, get_api_path).WillRepeatedly(Return("/test_open.txt"));
    EXPECT_CALL(*of, get_source_path).WillRepeatedly(Return(""));

    EXPECT_CALL(mp, get_filesystem_item)
        .WillOnce([](const std::string &api_path, bool directory,
                     filesystem_item &fsi) -> api_error {
          EXPECT_STREQ("/test_open.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          fsi.api_path = api_path;
          fsi.api_parent = utils::path::get_parent_api_path(api_path);
          fsi.directory = directory;
          fsi.size = 0U;
          return api_error::success;
        });

    EXPECT_CALL(mp, set_item_meta("/test_open.txt", _, _))
        .WillOnce([](const std::string &api_path, const std::string &key,
                     const std::string &value) -> api_error {
          EXPECT_STREQ("/test_open.txt", api_path.c_str());
          EXPECT_STREQ(META_SOURCE.c_str(), key.c_str());
          EXPECT_FALSE(value.empty());
          return api_error::success;
        });

    std::uint64_t handle{};
    std::shared_ptr<i_open_file> f{};
#ifdef _WIN32
    EXPECT_EQ(api_error::success, fm.open(of, {}, handle, f));
#else
    EXPECT_EQ(api_error::success, fm.open(of, O_RDWR, handle, f));
#endif
    EXPECT_CALL(*of, has_handle(1)).Times(2).WillRepeatedly([]() -> bool {
      return true;
    });

    EXPECT_TRUE(fm.get_open_file(1U, true, f));
    EXPECT_EQ(std::size_t(1U), fm.get_open_file_count());
    EXPECT_TRUE(f);

    EXPECT_TRUE(fm.get_open_file(1U, false, f));
    EXPECT_TRUE(f);
    EXPECT_EQ(std::size_t(1U), fm.get_open_file_count());
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, open_file_first_file_handle_is_not_zero) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);

    auto of = std::make_shared<mock_open_file>();
    EXPECT_CALL(*of, add).WillOnce(Return());
    EXPECT_CALL(*of, is_directory).WillRepeatedly(Return(false));
    EXPECT_CALL(*of, is_write_supported).WillRepeatedly(Return(true));
    EXPECT_CALL(*of, get_api_path).WillRepeatedly(Return("/test_open.txt"));
    EXPECT_CALL(*of, get_source_path).WillRepeatedly(Return("/test_open.src"));

    EXPECT_CALL(mp, get_filesystem_item)
        .WillOnce([](const std::string &api_path, bool directory,
                     filesystem_item &fsi) -> api_error {
          EXPECT_STREQ("/test_open.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          fsi.api_path = api_path;
          fsi.api_parent = utils::path::get_parent_api_path(api_path);
          fsi.directory = directory;
          fsi.size = 0U;
          fsi.source_path = "/test_open.src";
          return api_error::success;
        });

    std::uint64_t handle{};
    std::shared_ptr<i_open_file> f{};
#ifdef _WIN32
    EXPECT_EQ(api_error::success, fm.open(of, {}, handle, f));
#else
    EXPECT_EQ(api_error::success, fm.open(of, O_RDWR, handle, f));
#endif
    EXPECT_CALL(*of, has_handle(1)).WillOnce([]() -> bool { return true; });

    EXPECT_TRUE(fm.get_open_file(1U, true, f));
    EXPECT_GT(handle, std::uint64_t(0U));
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, can_remove_file) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);

    native_file::native_file_ptr f{};
    EXPECT_EQ(api_error::success,
              native_file::create_or_open("./test_remove.txt", f));
    f->close();
    EXPECT_TRUE(utils::file::is_file("./test_remove.txt"));

    EXPECT_CALL(mp, get_filesystem_item)
        .WillOnce([](const std::string &api_path, bool directory,
                     filesystem_item &fsi) -> api_error {
          EXPECT_STREQ("/test_remove.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          fsi.api_path = api_path;
          fsi.api_parent = utils::path::get_parent_api_path(api_path);
          fsi.directory = directory;
          fsi.size = 0U;
          fsi.source_path = "./test_remove.txt";
          return api_error::success;
        });
    EXPECT_CALL(mp, remove_file("/test_remove.txt"))
        .WillOnce(Return(api_error::success));

    EXPECT_EQ(api_error::success, fm.remove_file("/test_remove.txt"));

    EXPECT_FALSE(utils::file::is_file("./test_remove.txt"));
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, can_queue_and_remove_upload) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  console_consumer c;
  event_system::instance().start();

  {
    event_capture ec({"file_upload_queued", "download_stored_removed"});

    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);

    mock_open_file of{};
    EXPECT_CALL(of, get_api_path).WillOnce(Return("/test_queue.txt"));
    EXPECT_CALL(of, get_source_path).WillOnce(Return("/test_queue.src"));

    EXPECT_FALSE(fm.is_processing("/test_queue.txt"));
    fm.queue_upload(of);
    EXPECT_TRUE(fm.is_processing("/test_queue.txt"));

    fm.remove_upload("/test_queue.txt");
    EXPECT_FALSE(fm.is_processing("/test_queue.txt"));

    ec.wait_for_empty();
  }

  event_system::instance().stop();
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, remove_file_fails_if_open_file_is_modified) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);

    auto of = std::make_shared<mock_open_file>();
    EXPECT_CALL(*of, add).WillOnce(Return());
    EXPECT_CALL(*of, get_api_path).WillRepeatedly(Return("/test_remove.txt"));
    EXPECT_CALL(*of, get_source_path).WillRepeatedly(Return(""));
    EXPECT_CALL(*of, is_modified).WillOnce(Return(true));
    EXPECT_CALL(*of, is_directory).WillOnce(Return(false));

    EXPECT_CALL(mp, get_filesystem_item)
        .WillOnce([](const std::string &api_path, bool directory,
                     filesystem_item &fsi) -> api_error {
          EXPECT_STREQ("/test_remove.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          fsi.api_path = api_path;
          fsi.api_parent = utils::path::get_parent_api_path(api_path);
          fsi.directory = directory;
          fsi.size = 0U;
          return api_error::success;
        });

    EXPECT_CALL(mp, set_item_meta(_, _, _))
        .WillOnce([](const std::string &api_path, const std::string &key,
                     const std::string &value) -> api_error {
          EXPECT_STREQ("/test_remove.txt", api_path.c_str());
          EXPECT_STREQ(META_SOURCE.c_str(), key.c_str());
          EXPECT_FALSE(value.empty());
          return api_error::success;
        });

    std::uint64_t handle{};
    std::shared_ptr<i_open_file> f{};
#ifdef _WIN32
    EXPECT_EQ(api_error::success, fm.open(of, {}, handle, f));
#else
    EXPECT_EQ(api_error::success, fm.open(of, O_RDWR, handle, f));
#endif

    EXPECT_EQ(api_error::file_in_use, fm.remove_file("/test_remove.txt"));
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, file_is_closed_after_download_timeout) {
  {
    console_consumer c;
    event_system::instance().start();

    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_chunk_downloader_timeout_secs(3U);
    polling::instance().start(&cfg);

    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);
    fm.start();

    const auto source_path = utils::path::combine(
        cfg.get_cache_directory(), {utils::create_uuid_string()});

    event_consumer es("item_timeout", [](const event &e) {
      const auto &ee = dynamic_cast<const item_timeout &>(e);
      EXPECT_STREQ("/test_download_timeout.txt",
                   ee.get_api_path().get<std::string>().c_str());
    });

    const auto now = utils::get_file_time_now();
    auto meta = create_meta_attributes(
        now, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE, now + 1u, now + 2u,
        false, 1, "key", 2, now + 3u, 3u, 4u,
        utils::encryption::encrypting_reader::get_data_chunk_size() * 4u,
        source_path, 10, now + 4u);

    EXPECT_CALL(mp, get_filesystem_item)
        .WillRepeatedly([&meta](const std::string &api_path, bool directory,
                                filesystem_item &fsi) -> api_error {
          EXPECT_STREQ("/test_download_timeout.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          fsi.api_path = api_path;
          fsi.api_parent = utils::path::get_parent_api_path(api_path);
          fsi.directory = directory;
          fsi.size = utils::string::to_uint64(meta[META_SIZE]);
          fsi.source_path = meta[META_SOURCE];
          return api_error::success;
        });

    event_capture ec({"item_timeout"});

    std::uint64_t handle{};
    std::shared_ptr<i_open_file> f;
#ifdef _WIN32
    EXPECT_EQ(api_error::success,
              fm.open("/test_download_timeout.txt", false, {}, handle, f));
#else
    EXPECT_EQ(api_error::success,
              fm.open("/test_download_timeout.txt", false, O_RDWR, handle, f));
#endif

    EXPECT_CALL(mp, read_file_bytes)
        .WillRepeatedly([](const std::string & /* api_path */,
                           std::size_t /*size*/, std::uint64_t offset,
                           data_buffer & /*data*/,
                           stop_type &stop_requested) -> api_error {
          if (stop_requested) {
            return api_error::download_stopped;
          }

          if (offset == 0u) {
            return api_error::success;
          }

          while (not stop_requested) {
            std::this_thread::sleep_for(100ms);
          }

          return api_error::download_stopped;
        });

    data_buffer data{};
    EXPECT_EQ(api_error::success, f->read(1U, 0U, data));

    fm.close(handle);

    EXPECT_CALL(mp, set_item_meta("/test_download_timeout.txt", META_SOURCE, _))
        .WillOnce(Return(api_error::success));

    EXPECT_EQ(std::size_t(1U), fm.get_open_file_count());
    ec.wait_for_empty();

    EXPECT_EQ(std::size_t(0U), fm.get_open_file_count());
    fm.stop();

    polling::instance().stop();
  }

  event_system::instance().stop();
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, remove_file_fails_if_file_does_not_exist) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);

    EXPECT_CALL(mp, get_filesystem_item)
        .WillOnce([](const std::string &api_path, const bool &directory,
                     filesystem_item & /*fsi*/) -> api_error {
          EXPECT_STREQ("/test_remove.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          return api_error::item_not_found;
        });

    EXPECT_EQ(api_error::item_not_found, fm.remove_file("/test_remove.txt"));
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}

TEST(file_manager, remove_file_fails_if_provider_remove_file_fails) {
  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));

  {
    app_config cfg(provider_type::sia, "./fm_test");
    cfg.set_enable_chunk_downloader_timeout(false);
    mock_provider mp;

    EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

    file_manager fm(cfg, mp);

    EXPECT_CALL(mp, get_filesystem_item)
        .WillOnce([](const std::string &api_path, const bool &directory,
                     filesystem_item &fsi) -> api_error {
          EXPECT_STREQ("/test_remove.txt", api_path.c_str());
          EXPECT_FALSE(directory);
          fsi.api_path = api_path;
          fsi.api_parent = utils::path::get_parent_api_path(api_path);
          fsi.directory = directory;
          fsi.size = 0U;
          return api_error::success;
        });
    EXPECT_CALL(mp, remove_file("/test_remove.txt"))
        .WillOnce(Return(api_error::item_not_found));

    EXPECT_EQ(api_error::item_not_found, fm.remove_file("/test_remove.txt"));
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./fm_test"));
}
} // namespace repertory
