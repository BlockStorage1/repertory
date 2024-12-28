/*
  Copyright <2018-2024> <scott.evt.graves@protonmail.com>

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
#include "file_manager/cache_size_mgr.hpp"
#include "file_manager/events.hpp"
#include "file_manager/file_manager.hpp"
#include "file_manager/i_open_file.hpp"
#include "mocks/mock_open_file.hpp"
#include "mocks/mock_provider.hpp"
#include "mocks/mock_upload_manager.hpp"
#include "platform/platform.hpp"
#include "types/repertory.hpp"
#include "utils/common.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/event_capture.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"
#include "utils/polling.hpp"
#include "utils/string.hpp"
#include "utils/tasks.hpp"
#include "utils/time.hpp"
#include "utils/utils.hpp"

namespace repertory {
auto file_manager::open(std::shared_ptr<i_closeable_open_file> of,
                        const open_file_data &ofd, std::uint64_t &handle,
                        std::shared_ptr<i_open_file> &open_file) -> api_error {
  return open(of->get_api_path(), of->is_directory(), ofd, handle, open_file,
              of);
}

class file_manager_test : public ::testing::Test {
public:
  console_consumer con_consumer;
  std::unique_ptr<app_config> cfg;
  mock_provider mp;
  static std::atomic<std::size_t> inst;
  std::string file_manager_dir;

protected:
  void SetUp() override {
    event_system::instance().start();

    file_manager_dir = repertory::utils::path::combine(
        repertory::test::get_test_output_dir(),
        {"file_manager_test" + std::to_string(++inst)});

    cfg = std::make_unique<app_config>(provider_type::sia, file_manager_dir);
    cfg->set_enable_download_timeout(false);

    cache_size_mgr::instance().initialize(cfg.get());
  }

  void TearDown() override { event_system::instance().stop(); }
};

std::atomic<std::size_t> file_manager_test::inst{0U};

TEST_F(file_manager_test, can_start_and_stop) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  event_consumer consumer("service_started", [](const event &evt) {
    const auto &evt2 = dynamic_cast<const service_started &>(evt);
    EXPECT_STREQ("file_manager", evt2.get_service().get<std::string>().c_str());
  });
  event_consumer consumer2("service_shutdown_begin", [](const event &evt) {
    const auto &evt2 = dynamic_cast<const service_shutdown_begin &>(evt);
    EXPECT_STREQ("file_manager", evt2.get_service().get<std::string>().c_str());
  });
  event_consumer consumer3("service_shutdown_end", [](const event &evt) {
    const auto &evt2 = dynamic_cast<const service_shutdown_end &>(evt);
    EXPECT_STREQ("file_manager", evt2.get_service().get<std::string>().c_str());
  });

  event_capture capture({
      "service_started",
      "service_shutdown_begin",
      "service_shutdown_end",
  });

  file_manager mgr(*cfg, mp);
  mgr.start();
  mgr.stop();

  capture.wait_for_empty();
}

TEST_F(file_manager_test, can_create_and_close_file) {
  cfg->set_enable_download_timeout(true);

  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  polling::instance().start(cfg.get());

  file_manager mgr(*cfg, mp);
  mgr.start();

  event_capture capture({
      "item_timeout",
      "filesystem_item_opened",
      "filesystem_item_handle_opened",
      "filesystem_item_handle_closed",
      "filesystem_item_closed",
  });

  auto source_path = utils::path::combine(cfg->get_cache_directory(),
                                          {utils::create_uuid_string()});

  std::uint64_t handle{};
  {
    std::shared_ptr<i_open_file> open_file;

    auto now = utils::time::get_time_now();
    auto meta = create_meta_attributes(now, FILE_ATTRIBUTE_ARCHIVE, now + 1U,
                                       now + 2U, false, 1, "key", 2, now + 3U,
                                       3U, 4U, 0U, source_path, 10U, now + 4U);

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

    event_consumer consumer("filesystem_item_opened", [&](const event &evt) {
      const auto &evt2 = dynamic_cast<const filesystem_item_opened &>(evt);
      EXPECT_STREQ("/test_create.txt",
                   evt2.get_api_path().get<std::string>().c_str());
      EXPECT_STREQ(source_path.c_str(),
                   evt2.get_source().get<std::string>().c_str());
      EXPECT_STREQ("0", evt2.get_directory().get<std::string>().c_str());
    });

    event_consumer ec2("filesystem_item_handle_opened", [&](const event &evt) {
      const auto &evt2 =
          dynamic_cast<const filesystem_item_handle_opened &>(evt);
      EXPECT_STREQ("/test_create.txt",
                   evt2.get_api_path().get<std::string>().c_str());
      EXPECT_STREQ(source_path.c_str(),
                   evt2.get_source().get<std::string>().c_str());
      EXPECT_STREQ("0", evt2.get_directory().get<std::string>().c_str());
      EXPECT_STREQ("1", evt2.get_handle().get<std::string>().c_str());
    });

#if defined(_WIN32)
    EXPECT_EQ(api_error::success,
              mgr.create("/test_create.txt", meta, {}, handle, open_file));
#else
    EXPECT_EQ(api_error::success,
              mgr.create("/test_create.txt", meta, O_RDWR, handle, open_file));
#endif
    EXPECT_EQ(std::size_t(1U), mgr.get_open_file_count());
    EXPECT_EQ(std::size_t(1U), mgr.get_open_handle_count());
    EXPECT_EQ(std::uint64_t(1U), handle);
  }

  event_consumer ec3("filesystem_item_closed", [&](const event &evt) {
    const auto &evt2 = dynamic_cast<const filesystem_item_closed &>(evt);
    EXPECT_STREQ("/test_create.txt",
                 evt2.get_api_path().get<std::string>().c_str());
    EXPECT_STREQ(source_path.c_str(),
                 evt2.get_source().get<std::string>().c_str());
    EXPECT_STREQ("0", evt2.get_directory().get<std::string>().c_str());
  });

  event_consumer ec4("filesystem_item_handle_closed", [&](const event &evt) {
    const auto &evt2 = dynamic_cast<const filesystem_item_handle_closed &>(evt);
    EXPECT_STREQ("/test_create.txt",
                 evt2.get_api_path().get<std::string>().c_str());
    EXPECT_STREQ(source_path.c_str(),
                 evt2.get_source().get<std::string>().c_str());
    EXPECT_STREQ("0", evt2.get_directory().get<std::string>().c_str());
    EXPECT_STREQ("1", evt2.get_handle().get<std::string>().c_str());
  });

  mgr.close(handle);

  EXPECT_EQ(std::size_t(1U), mgr.get_open_file_count());
  EXPECT_EQ(std::size_t(0U), mgr.get_open_handle_count());

  capture.wait_for_empty();
  EXPECT_EQ(std::size_t(0U), mgr.get_open_file_count());

  mgr.stop();

  polling::instance().stop();
}

TEST_F(file_manager_test, can_open_and_close_file) {
  cfg->set_enable_download_timeout(true);

  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  polling::instance().start(cfg.get());

  file_manager mgr(*cfg, mp);
  mgr.start();

  event_capture capture({
      "item_timeout",
      "filesystem_item_opened",
      "filesystem_item_handle_opened",
      "filesystem_item_handle_closed",
      "filesystem_item_closed",
  });
  auto source_path = utils::path::combine(cfg->get_cache_directory(),
                                          {utils::create_uuid_string()});

  std::uint64_t handle{};
  {

    auto now = utils::time::get_time_now();
    auto meta = create_meta_attributes(now, FILE_ATTRIBUTE_ARCHIVE, now + 1U,
                                       now + 2U, false, 1U, "key", 2U, now + 3U,
                                       3U, 4U, 0U, source_path, 10U, now + 4U);

    EXPECT_CALL(mp, create_file).Times(0U);

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

    event_consumer consumer("filesystem_item_opened", [&](const event &evt) {
      const auto &evt2 = dynamic_cast<const filesystem_item_opened &>(evt);
      EXPECT_STREQ("/test_open.txt",
                   evt2.get_api_path().get<std::string>().c_str());
      EXPECT_STREQ(source_path.c_str(),
                   evt2.get_source().get<std::string>().c_str());
      EXPECT_STREQ("0", evt2.get_directory().get<std::string>().c_str());
    });

    event_consumer ec2("filesystem_item_handle_opened", [&](const event &evt) {
      const auto &evt2 =
          dynamic_cast<const filesystem_item_handle_opened &>(evt);
      EXPECT_STREQ("/test_open.txt",
                   evt2.get_api_path().get<std::string>().c_str());
      EXPECT_STREQ(source_path.c_str(),
                   evt2.get_source().get<std::string>().c_str());
      EXPECT_STREQ("0", evt2.get_directory().get<std::string>().c_str());
      EXPECT_STREQ("1", evt2.get_handle().get<std::string>().c_str());
    });

    std::shared_ptr<i_open_file> open_file;
#if defined(_WIN32)
    EXPECT_EQ(api_error::success,
              mgr.open("/test_open.txt", false, {}, handle, open_file));
#else
    EXPECT_EQ(api_error::success,
              mgr.open("/test_open.txt", false, O_RDWR, handle, open_file));
#endif
    EXPECT_EQ(std::size_t(1U), mgr.get_open_file_count());
    EXPECT_EQ(std::size_t(1U), mgr.get_open_handle_count());
    EXPECT_EQ(std::uint64_t(1U), handle);
  }

  event_consumer ec3("filesystem_item_closed", [&](const event &evt) {
    const auto &evt2 = dynamic_cast<const filesystem_item_closed &>(evt);
    EXPECT_STREQ("/test_open.txt",
                 evt2.get_api_path().get<std::string>().c_str());
    EXPECT_STREQ(source_path.c_str(),
                 evt2.get_source().get<std::string>().c_str());
    EXPECT_STREQ("0", evt2.get_directory().get<std::string>().c_str());
  });

  event_consumer ec4("filesystem_item_handle_closed", [&](const event &evt) {
    const auto &evt2 = dynamic_cast<const filesystem_item_handle_closed &>(evt);
    EXPECT_STREQ("/test_open.txt",
                 evt2.get_api_path().get<std::string>().c_str());
    EXPECT_STREQ(source_path.c_str(),
                 evt2.get_source().get<std::string>().c_str());
    EXPECT_STREQ("0", evt2.get_directory().get<std::string>().c_str());
    EXPECT_STREQ("1", evt2.get_handle().get<std::string>().c_str());
  });

  mgr.close(handle);

  EXPECT_EQ(std::size_t(1U), mgr.get_open_file_count());
  EXPECT_EQ(std::size_t(0U), mgr.get_open_handle_count());

  capture.wait_for_empty();
  EXPECT_EQ(std::size_t(0U), mgr.get_open_file_count());

  mgr.stop();

  polling::instance().stop();
}

TEST_F(file_manager_test, can_open_and_close_multiple_handles_for_same_file) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  polling::instance().start(cfg.get());

  file_manager mgr(*cfg, mp);
  mgr.start();

  {
    auto source_path = utils::path::combine(cfg->get_cache_directory(),
                                            {utils::create_uuid_string()});

    auto now = utils::time::get_time_now();
    auto meta = create_meta_attributes(now, FILE_ATTRIBUTE_ARCHIVE, now + 1U,
                                       now + 2U, false, 1U, "key", 2U, now + 3U,
                                       3U, 4U, 0U, source_path, 10U, now + 4U);

    EXPECT_CALL(mp, create_file).Times(0U);

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

    std::array<std::uint64_t, 4U> handles{};
    for (std::size_t idx = 0U; idx < handles.size(); ++idx) {
      std::shared_ptr<i_open_file> open_file;
#if defined(_WIN32)
      EXPECT_EQ(api_error::success, mgr.open("/test_open.txt", false, {},
                                             handles.at(idx), open_file));
#else
      EXPECT_EQ(api_error::success, mgr.open("/test_open.txt", false, O_RDWR,
                                             handles.at(idx), open_file));
#endif

      EXPECT_EQ(std::size_t(1U), mgr.get_open_file_count());
      EXPECT_EQ(std::size_t(idx + 1U), mgr.get_open_handle_count());
      EXPECT_EQ(std::uint64_t(idx + 1U), handles.at(idx));
    }

    for (std::size_t idx = 0U; idx < handles.size(); ++idx) {
      EXPECT_EQ(std::size_t(1U), mgr.get_open_file_count());
      EXPECT_EQ(std::size_t(handles.size() - idx), mgr.get_open_handle_count());
      mgr.close(handles.at(idx));
    }
    mgr.stop();
  }

  EXPECT_EQ(std::size_t(0U), mgr.get_open_file_count());
  EXPECT_EQ(std::size_t(0U), mgr.get_open_handle_count());

  polling::instance().stop();
}

TEST_F(file_manager_test,
       download_is_stored_after_write_if_partially_downloaded) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);
  mgr.start();

  auto source_path = utils::path::combine(cfg->get_cache_directory(),
                                          {utils::create_uuid_string()});

  event_consumer consumer(
      "download_resume_added", [&source_path](const event &evt) {
        const auto &evt2 = dynamic_cast<const download_resume_added &>(evt);
        EXPECT_STREQ("/test_write_partial_download.txt",
                     evt2.get_api_path().get<std::string>().c_str());
        EXPECT_STREQ(source_path.c_str(),
                     evt2.get_dest_path().get<std::string>().c_str());
      });

  event_capture capture({"download_resume_added"}, {
                                                       "file_upload_completed",
                                                       "file_upload_queued",
                                                   });

  auto now = utils::time::get_time_now();
  auto meta = create_meta_attributes(
      now, FILE_ATTRIBUTE_ARCHIVE, now + 1U, now + 2U, false, 1U, "key", 2U,
      now + 3U, 3U, 4U,
      utils::encryption::encrypting_reader::get_data_chunk_size() * 4U,
      source_path, 10U, now + 4U);
  auto &file =
      test::create_random_file(utils::string::to_uint64(meta[META_SIZE]));

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

  EXPECT_CALL(mp, read_file_bytes)
      .WillRepeatedly([&file](const std::string & /* api_path */,
                              std::size_t size, std::uint64_t offset,
                              data_buffer &data,
                              stop_type &stop_requested) -> api_error {
        if (stop_requested) {
          return api_error::download_stopped;
        }

        if (offset == 0u) {
          std::size_t bytes_read{};
          data.resize(size);
          auto ret = file.read(data, offset, &bytes_read) ? api_error::success
                                                          : api_error::os_error;
          EXPECT_EQ(bytes_read, data.size());
          return ret;
        }

        while (not stop_requested) {
          std::this_thread::sleep_for(100ms);
        }

        return api_error::download_stopped;
      });

  std::uint64_t handle{};
  std::shared_ptr<i_open_file> open_file;
#if defined(_WIN32)
  EXPECT_EQ(api_error::success, mgr.open("/test_write_partial_download.txt",
                                         false, {}, handle, open_file));
#else
  EXPECT_EQ(api_error::success, mgr.open("/test_write_partial_download.txt",
                                         false, O_RDWR, handle, open_file));
#endif

  EXPECT_CALL(mp, set_item_meta("/test_write_partial_download.txt", _))
      .WillOnce(
          [](const std::string &, const api_meta_map &meta2) -> api_error {
            EXPECT_NO_THROW(EXPECT_FALSE(meta2.at(META_CHANGED).empty()));
            EXPECT_NO_THROW(EXPECT_FALSE(meta2.at(META_MODIFIED).empty()));
            EXPECT_NO_THROW(EXPECT_FALSE(meta2.at(META_WRITTEN).empty()));
            return api_error::success;
          });
  EXPECT_CALL(mp, upload_file).Times(0u);

  if (not open_file->is_write_supported()) {
    EXPECT_TRUE(mgr.get_open_file(handle, true, open_file));
  }

  std::size_t bytes_written{};
  data_buffer data = {0, 1, 2};
  EXPECT_EQ(api_error::success, open_file->write(0u, data, bytes_written));
  EXPECT_EQ(std::size_t(3u), bytes_written);
  open_file.reset();

  mgr.close(handle);

  EXPECT_EQ(std::size_t(1u), mgr.get_open_file_count());
  EXPECT_EQ(std::size_t(0u), mgr.get_open_handle_count());

  mgr.stop();
  capture.wait_for_empty();

  event_capture ec2({"download_restored", "download_resume_added"},
                    {
                        "file_upload_completed",
                        "file_upload_queued",
                    });
  EXPECT_EQ(std::size_t(0u), mgr.get_open_file_count());
  EXPECT_EQ(std::size_t(0u), mgr.get_open_handle_count());

  auto stored_downloads = mgr.get_stored_downloads();
  EXPECT_EQ(std::size_t(1u), stored_downloads.size());

  EXPECT_STREQ("/test_write_partial_download.txt",
               stored_downloads[0U].api_path.c_str());
  EXPECT_EQ(utils::encryption::encrypting_reader::get_data_chunk_size(),
            stored_downloads[0U].chunk_size);
  auto read_state = stored_downloads[0U].read_state;
  EXPECT_TRUE(read_state[0U]);
  for (std::size_t i = 1U; i < read_state.size(); i++) {
    EXPECT_FALSE(read_state[i]);
  }
  EXPECT_STREQ(source_path.c_str(), stored_downloads[0u].source_path.c_str());

  mgr.start();

  event_consumer es2("download_restored", [&source_path](const event &evt) {
    const auto &evt2 = dynamic_cast<const download_restored &>(evt);
    EXPECT_STREQ("/test_write_partial_download.txt",
                 evt2.get_api_path().get<std::string>().c_str());
    EXPECT_STREQ(source_path.c_str(),
                 evt2.get_dest_path().get<std::string>().c_str());
  });

  EXPECT_EQ(std::size_t(1u), mgr.get_open_file_count());
  EXPECT_EQ(std::size_t(0u), mgr.get_open_handle_count());
  mgr.stop();
  ec2.wait_for_empty();

  EXPECT_EQ(std::size_t(0u), mgr.get_open_file_count());
  EXPECT_EQ(std::size_t(0u), mgr.get_open_handle_count());

  file.close();
}

TEST_F(file_manager_test, upload_occurs_after_write_if_fully_downloaded) {
  cfg->set_enable_download_timeout(true);

  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  polling::instance().start(cfg.get());

  file_manager mgr(*cfg, mp);
  mgr.start();

  auto source_path = utils::path::combine(cfg->get_cache_directory(),
                                          {utils::create_uuid_string()});

  event_consumer consumer(
      "file_upload_queued", [&source_path](const event &evt) {
        const auto &evt2 = dynamic_cast<const file_upload_queued &>(evt);
        EXPECT_STREQ("/test_write_full_download.txt",
                     evt2.get_api_path().get<std::string>().c_str());
        EXPECT_STREQ(source_path.c_str(),
                     evt2.get_source().get<std::string>().c_str());
      });
  event_consumer es2("file_upload_completed", [&source_path](const event &evt) {
    const auto &evt2 = dynamic_cast<const file_upload_completed &>(evt);
    EXPECT_STREQ("/test_write_full_download.txt",
                 evt2.get_api_path().get<std::string>().c_str());
    EXPECT_STREQ(source_path.c_str(),
                 evt2.get_source().get<std::string>().c_str());
  });

  auto now = utils::time::get_time_now();
  auto meta = create_meta_attributes(
      now, FILE_ATTRIBUTE_ARCHIVE, now + 1u, now + 2u, false, 1, "key", 2,
      now + 3u, 3u, 4u,
      utils::encryption::encrypting_reader::get_data_chunk_size() * 4u,
      source_path, 10, now + 4u);
  auto &file =
      test::create_random_file(utils::string::to_uint64(meta[META_SIZE]));

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

  EXPECT_CALL(mp, read_file_bytes)
      .WillRepeatedly([&file](const std::string & /* api_path */,
                              std::size_t size, std::uint64_t offset,
                              data_buffer &data,
                              stop_type & /* stop_requested */) -> api_error {
        std::size_t bytes_read{};
        data.resize(size);
        auto ret = file.read(data, offset, &bytes_read) ? api_error::success
                                                        : api_error::os_error;
        EXPECT_EQ(bytes_read, data.size());
        return ret;
      });

  std::uint64_t handle{};
  std::shared_ptr<i_open_file> open_file;
#if defined(_WIN32)
  EXPECT_EQ(api_error::success, mgr.open("/test_write_full_download.txt", false,
                                         {}, handle, open_file));
#else
  EXPECT_EQ(api_error::success, mgr.open("/test_write_full_download.txt", false,
                                         O_RDWR, handle, open_file));
#endif

  EXPECT_CALL(mp, set_item_meta("/test_write_full_download.txt", _))
      .WillOnce(
          [](const std::string &, const api_meta_map &meta2) -> api_error {
            EXPECT_NO_THROW(EXPECT_FALSE(meta2.at(META_CHANGED).empty()));
            EXPECT_NO_THROW(EXPECT_FALSE(meta2.at(META_MODIFIED).empty()));
            EXPECT_NO_THROW(EXPECT_FALSE(meta2.at(META_WRITTEN).empty()));
            return api_error::success;
          });

  if (not open_file->is_write_supported()) {
    EXPECT_TRUE(mgr.get_open_file(handle, true, open_file));
  }

  event_capture capture({
      "item_timeout",
      "file_upload_queued",
      "file_upload_completed",
  });

  EXPECT_CALL(mp, upload_file("/test_write_full_download.txt", source_path, _))
      .WillOnce(Return(api_error::success));

  std::size_t bytes_written{};
  data_buffer data = {0, 1, 2};
  EXPECT_EQ(api_error::success, open_file->write(0u, data, bytes_written));
  EXPECT_EQ(std::size_t(3u), bytes_written);

  while (not open_file->is_complete()) {
    std::this_thread::sleep_for(10ms);
  }
  open_file.reset();

  mgr.close(handle);

  capture.wait_for_empty();

  EXPECT_EQ(std::size_t(0U), mgr.get_open_file_count());
  EXPECT_EQ(std::size_t(0U), mgr.get_open_handle_count());

  mgr.stop();

  file.close();

  polling::instance().stop();
}

TEST_F(file_manager_test, can_evict_file) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);
  mgr.start();

  event_capture capture({
      "filesystem_item_opened",
      "filesystem_item_handle_opened",
      "filesystem_item_handle_closed",
      "filesystem_item_closed",
      "file_upload_completed",
  });

  auto source_path = utils::path::combine(cfg->get_cache_directory(),
                                          {utils::create_uuid_string()});

  auto now = utils::time::get_time_now();

  auto meta = create_meta_attributes(now, FILE_ATTRIBUTE_ARCHIVE, now + 1u,
                                     now + 2u, false, 1, "key", 2, now + 3u, 3u,
                                     4u, 0u, source_path, 10, now + 4u);
  std::uint64_t handle{};
  {
    std::shared_ptr<i_open_file> open_file;

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

#if defined(_WIN32)
    EXPECT_EQ(api_error::success,
              mgr.create("/test_evict.txt", meta, {}, handle, open_file));
#else
    EXPECT_EQ(api_error::success,
              mgr.create("/test_evict.txt", meta, O_RDWR, handle, open_file));
#endif
    EXPECT_CALL(mp, set_item_meta("/test_evict.txt", _))
        .Times(2)
        .WillRepeatedly(Return(api_error::success));
    EXPECT_CALL(mp, upload_file(_, _, _)).WillOnce(Return(api_error::success));

    if (not open_file->is_write_supported()) {
      EXPECT_TRUE(mgr.get_open_file(handle, true, open_file));
    }

    data_buffer data{{0, 1, 1}};
    std::size_t bytes_written{};
    auto res = open_file->write(0U, data, bytes_written);
    EXPECT_EQ(api_error::success, res);

    auto opt_size = utils::file::file{source_path}.size();
    EXPECT_TRUE(opt_size.has_value());
    EXPECT_EQ(static_cast<std::uint64_t>(data.size()), opt_size.value());
  }

  mgr.close(handle);
  capture.wait_for_empty();

  EXPECT_TRUE(utils::retry_action(
      [&mgr]() -> bool { return not mgr.is_processing("/test_evict.txt"); }));

  EXPECT_CALL(mp, get_item_meta(_, META_PINNED, _))
      .WillOnce([](const std::string &api_path, const std::string &key,
                   std::string &value) -> api_error {
        EXPECT_STREQ("/test_evict.txt", api_path.c_str());
        EXPECT_STREQ(META_PINNED.c_str(), key.c_str());
        value = "0";
        return api_error::success;
      });
  EXPECT_TRUE(mgr.evict_file("/test_evict.txt"));
  EXPECT_FALSE(utils::file::file(source_path).exists());

  mgr.stop();
}

TEST_F(file_manager_test, evict_file_fails_if_file_is_pinned) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));
  file_manager mgr(*cfg, mp);

  EXPECT_CALL(mp, get_filesystem_item)
      .WillRepeatedly([](const std::string &api_path, bool directory,
                         filesystem_item &fsi) -> api_error {
        fsi.api_path = api_path;
        fsi.api_parent = utils::path::get_parent_api_path(api_path);
        fsi.directory = directory;
        fsi.size = 2U;
        fsi.source_path = "/test/test_open.src";
        return api_error::success;
      });

  EXPECT_CALL(mp, get_item_meta(_, META_PINNED, _))
      .WillOnce([](const std::string &api_path, const std::string &key,
                   std::string &value) -> api_error {
        EXPECT_STREQ("/test_open.txt", api_path.c_str());
        EXPECT_STREQ(META_PINNED.c_str(), key.c_str());
        value = "1";
        return api_error::success;
      });

  EXPECT_FALSE(mgr.evict_file("/test_open.txt"));
}

TEST_F(file_manager_test, evict_file_fails_if_provider_is_read_only) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(true));
  file_manager mgr(*cfg, mp);
  EXPECT_FALSE(mgr.evict_file("/test.txt"));
}

TEST_F(file_manager_test, evict_file_fails_if_file_is_open) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));
  file_manager mgr(*cfg, mp);

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
  std::shared_ptr<i_open_file> open_file{};
#if defined(_WIN32)
  EXPECT_EQ(api_error::success,
            mgr.open("/test_open.txt", false, {}, handle, open_file));
#else
  EXPECT_EQ(api_error::success,
            mgr.open("/test_open.txt", false, O_RDWR, handle, open_file));
#endif

  EXPECT_FALSE(mgr.evict_file("/test_open.txt"));

  mgr.close(handle);
}

TEST_F(file_manager_test, evict_file_fails_if_unable_to_get_filesystem_item) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));
  file_manager mgr(*cfg, mp);

  EXPECT_CALL(mp, get_filesystem_item)
      .WillRepeatedly([](const std::string & /* api_path */,
                         bool /* directory */,
                         filesystem_item & /* fsi */) -> api_error {
        return api_error::error;
      });

  EXPECT_FALSE(mgr.evict_file("/test_open.txt"));
}

TEST_F(file_manager_test, evict_file_fails_if_source_path_is_empty) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));
  file_manager mgr(*cfg, mp);

  EXPECT_CALL(mp, get_filesystem_item)
      .WillRepeatedly([](const std::string &api_path, bool directory,
                         filesystem_item &fsi) -> api_error {
        fsi.api_path = api_path;
        fsi.api_parent = utils::path::get_parent_api_path(api_path);
        fsi.directory = directory;
        fsi.size = 20U;
        return api_error::success;
      });

  EXPECT_FALSE(mgr.evict_file("/test_open.txt"));
}

TEST_F(file_manager_test, evict_file_fails_if_file_is_uploading) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);
  mgr.start();

  event_capture capture({
      "filesystem_item_opened",
      "filesystem_item_handle_opened",
      "filesystem_item_handle_closed",
      "filesystem_item_closed",
      "file_upload_completed",
  });

  auto source_path = utils::path::combine(cfg->get_cache_directory(),
                                          {utils::create_uuid_string()});

  auto now = utils::time::get_time_now();

  auto meta = create_meta_attributes(now, FILE_ATTRIBUTE_ARCHIVE, now + 1u,
                                     now + 2u, false, 1, "", 2, now + 3u, 3u,
                                     4u, 0u, source_path, 10, now + 4u);
  std::uint64_t handle{};
  {
    std::shared_ptr<i_open_file> open_file;

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

#if defined(_WIN32)
    EXPECT_EQ(api_error::success,
              mgr.create("/test_evict.txt", meta, {}, handle, open_file));
#else
    EXPECT_EQ(api_error::success,
              mgr.create("/test_evict.txt", meta, O_RDWR, handle, open_file));
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

    if (not open_file->is_write_supported()) {
      EXPECT_TRUE(mgr.get_open_file(handle, true, open_file));
    }

    data_buffer data{{0, 1, 1}};
    std::size_t bytes_written{};
    EXPECT_EQ(api_error::success, open_file->write(0U, data, bytes_written));

    auto opt_size = utils::file::file{source_path}.size();
    EXPECT_TRUE(opt_size.has_value());
    EXPECT_EQ(static_cast<std::uint64_t>(data.size()), opt_size.value());

    mgr.close(handle);

    EXPECT_TRUE(utils::retry_action(
        [&mgr]() -> bool { return mgr.is_processing("/test_evict.txt"); }));
    EXPECT_FALSE(mgr.evict_file("/test_evict.txt"));
  }

  capture.wait_for_empty();

  EXPECT_TRUE(utils::file::file(source_path).exists());

  mgr.stop();
}

TEST_F(file_manager_test, evict_file_fails_if_file_is_in_upload_queue) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);

  mock_open_file open_file{};
  EXPECT_CALL(open_file, is_directory).WillRepeatedly(Return(false));
  EXPECT_CALL(open_file, get_api_path)
      .WillRepeatedly(Return("/test_evict.txt"));
  EXPECT_CALL(open_file, get_source_path)
      .WillRepeatedly(Return("/test_evict.src"));
  mgr.queue_upload(open_file);

  EXPECT_TRUE(mgr.is_processing("/test_evict.txt"));
  EXPECT_FALSE(mgr.evict_file("/test_evict.txt"));
}

TEST_F(file_manager_test, evict_file_fails_if_file_is_modified) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);

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

  auto file = std::make_shared<mock_open_file>();
  EXPECT_CALL(*file, add).WillOnce(Return());
  EXPECT_CALL(*file, get_api_path).WillRepeatedly(Return("/test_evict.txt"));
  EXPECT_CALL(*file, get_source_path).WillRepeatedly(Return("/test_evict.src"));
  EXPECT_CALL(*file, is_directory).WillOnce(Return(false));
  EXPECT_CALL(*file, is_modified).WillRepeatedly(Return(true));
  EXPECT_CALL(*file, is_write_supported).WillRepeatedly(Return(true));

  std::uint64_t handle{};
  std::shared_ptr<i_open_file> open_file;
#if defined(_WIN32)
  EXPECT_EQ(api_error::success, mgr.open(file, {}, handle, open_file));
#else
  EXPECT_EQ(api_error::success, mgr.open(file, O_RDWR, handle, open_file));
#endif

  EXPECT_TRUE(mgr.is_processing("/test_evict.txt"));
  EXPECT_FALSE(mgr.evict_file("/test_evict.txt"));
}

TEST_F(file_manager_test, evict_file_fails_if_file_is_not_complete) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);
  EXPECT_CALL(mp, get_filesystem_item)
      .WillOnce([](const std::string &api_path, bool directory,
                   filesystem_item &fsi) -> api_error {
        EXPECT_STREQ("/test_evict.txt", api_path.c_str());
        EXPECT_FALSE(directory);
        fsi.api_parent = utils::path::get_parent_api_path(api_path);
        fsi.api_path = api_path;
        fsi.directory = directory;
        fsi.size = 1U;
        return api_error::success;
      });

  auto file = std::make_shared<mock_open_file>();
  EXPECT_CALL(*file, add).WillOnce(Return());
  EXPECT_CALL(*file, get_api_path).WillRepeatedly(Return("/test_evict.txt"));
  EXPECT_CALL(*file, get_source_path).WillRepeatedly(Return("/test_evict.src"));
  EXPECT_CALL(*file, is_complete).WillRepeatedly(Return(false));
  EXPECT_CALL(*file, is_directory).WillOnce(Return(false));
  EXPECT_CALL(*file, is_modified).WillRepeatedly(Return(false));
  EXPECT_CALL(*file, is_write_supported).WillRepeatedly(Return(true));
  EXPECT_CALL(mp, set_item_meta("/test_evict.txt", META_SOURCE, _))
      .WillOnce(Return(api_error::success));

  std::uint64_t handle{};
  std::shared_ptr<i_open_file> open_file;
#if defined(_WIN32)
  EXPECT_EQ(api_error::success, mgr.open(file, {}, handle, open_file));
#else
  EXPECT_EQ(api_error::success, mgr.open(file, O_RDWR, handle, open_file));
#endif

  EXPECT_TRUE(mgr.is_processing("/test_evict.txt"));
  EXPECT_FALSE(mgr.evict_file("/test_evict.txt"));
}

TEST_F(file_manager_test, can_get_directory_items) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);
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
  auto list = mgr.get_directory_items("/");
  EXPECT_EQ(std::size_t(2U), list.size());
}

TEST_F(file_manager_test, file_is_not_opened_if_provider_create_file_fails) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  auto now = utils::time::get_time_now();
  auto meta = create_meta_attributes(now, FILE_ATTRIBUTE_ARCHIVE, now + 1u,
                                     now + 2u, false, 1, "", 2, now + 3u, 3u,
                                     4u, 0u, "/test_create.src", 10, now + 4u);
  file_manager mgr(*cfg, mp);

  EXPECT_CALL(mp, create_file("/test_create.txt", meta))
      .WillOnce(Return(api_error::error));

  std::uint64_t handle{};
  std::shared_ptr<i_open_file> open_file;
#if defined(_WIN32)
  EXPECT_EQ(api_error::error,
            mgr.create("/test_create.txt", meta, {}, handle, open_file));
#else
  EXPECT_EQ(api_error::error,
            mgr.create("/test_create.txt", meta, O_RDWR, handle, open_file));
#endif
  EXPECT_FALSE(open_file);
  EXPECT_EQ(std::size_t(0U), mgr.get_open_file_count());
}

TEST_F(file_manager_test, create_fails_if_provider_create_is_unsuccessful) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);

  EXPECT_CALL(mp, create_file("/test_create.txt", _))
      .WillOnce(Return(api_error::error));

  std::uint64_t handle{};
  std::shared_ptr<i_open_file> open_file{};
  api_meta_map meta{};
#if defined(_WIN32)
  EXPECT_EQ(api_error::error,
            mgr.create("/test_create.txt", meta, {}, handle, open_file));
#else
  EXPECT_EQ(api_error::error,
            mgr.create("/test_create.txt", meta, O_RDWR, handle, open_file));
#endif
  EXPECT_EQ(std::size_t(0U), mgr.get_open_file_count());
}

TEST_F(file_manager_test, get_open_file_fails_if_file_is_not_open) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);

  std::shared_ptr<i_open_file> open_file{};
  EXPECT_FALSE(mgr.get_open_file(0U, true, open_file));
  EXPECT_FALSE(open_file);

  EXPECT_FALSE(mgr.get_open_file(0U, false, open_file));
  EXPECT_FALSE(open_file);
}

TEST_F(file_manager_test,
       get_open_file_promotes_non_writeable_file_if_writeable_is_specified) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);

  auto non_writeable = std::make_shared<mock_open_file>();
  EXPECT_CALL(*non_writeable, is_directory).WillOnce(Return(false));
  EXPECT_CALL(*non_writeable, add).WillOnce(Return());
  EXPECT_CALL(*non_writeable, get_api_path)
      .WillRepeatedly(Return("/test_open.txt"));
  EXPECT_CALL(*non_writeable, get_source_path)
      .WillRepeatedly(Return("/test_open.src"));
  EXPECT_CALL(*non_writeable, is_modified).WillRepeatedly(Return(true));
  EXPECT_CALL(*non_writeable, is_write_supported).WillRepeatedly(Return(false));
  EXPECT_CALL(*non_writeable, is_write_supported).WillRepeatedly(Return(false));

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
  std::shared_ptr<i_open_file> open_file{};
#if defined(_WIN32)
  EXPECT_EQ(api_error::success, mgr.open(non_writeable, {}, handle, open_file));
  EXPECT_CALL(*non_writeable, get_open_data())
      .WillOnce([&handle]() -> std::map<std::uint64_t, open_file_data> & {
        static std::map<std::uint64_t, open_file_data> map;
        map[handle] = {};
        return map;
      });
#else  // !defined(_WIN32)
  EXPECT_EQ(api_error::success,
            mgr.open(non_writeable, O_RDWR, handle, open_file));
  EXPECT_CALL(*non_writeable, get_open_data())
      .WillOnce([&handle]() -> std::map<std::uint64_t, open_file_data> & {
        static std::map<std::uint64_t, open_file_data> map;
        map[handle] = O_RDWR;
        return map;
      });
#endif // defined(_WIN32)

  EXPECT_CALL(mp, set_item_meta("/test_open.txt", META_SOURCE, _))
      .WillOnce(Return(api_error::success));

  EXPECT_CALL(*non_writeable, has_handle(1)).WillOnce([]() -> bool {
    return true;
  });
  EXPECT_TRUE(mgr.get_open_file(handle, true, open_file));
  EXPECT_NE(non_writeable.get(), open_file.get());
  EXPECT_EQ(std::size_t(1U), mgr.get_open_file_count());

  std::shared_ptr<i_open_file> file2{};
  EXPECT_TRUE(mgr.get_open_file(handle, false, file2));
  EXPECT_EQ(open_file.get(), file2.get());
  EXPECT_EQ(std::size_t(1U), mgr.get_open_file_count());
}

TEST_F(file_manager_test, open_file_fails_if_file_is_not_found) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);

  std::shared_ptr<i_open_file> open_file{};
  EXPECT_FALSE(mgr.get_open_file(1U, true, open_file));
  EXPECT_EQ(std::size_t(0U), mgr.get_open_file_count());
  EXPECT_FALSE(open_file);

  EXPECT_FALSE(mgr.get_open_file(1U, false, open_file));
  EXPECT_FALSE(open_file);
  EXPECT_EQ(std::size_t(0U), mgr.get_open_file_count());
}

TEST_F(file_manager_test,
       open_file_fails_if_provider_get_filesystem_item_fails) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);

  auto file = std::make_shared<mock_open_file>();
  EXPECT_CALL(*file, is_directory).WillRepeatedly(Return(false));
  EXPECT_CALL(*file, get_api_path).WillRepeatedly(Return("/test_open.txt"));
  EXPECT_CALL(*file, get_source_path).WillRepeatedly(Return("/test_open.src"));

  EXPECT_CALL(mp, get_filesystem_item)
      .WillOnce([](const std::string &api_path, bool directory,
                   filesystem_item & /*fsi*/) -> api_error {
        EXPECT_STREQ("/test_open.txt", api_path.c_str());
        EXPECT_FALSE(directory);
        return api_error::error;
      });

  std::uint64_t handle{};
  std::shared_ptr<i_open_file> open_file{};
#if defined(_WIN32)
  EXPECT_EQ(api_error::error, mgr.open(file, {}, handle, open_file));
#else
  EXPECT_EQ(api_error::error, mgr.open(file, O_RDWR, handle, open_file));
#endif
  EXPECT_FALSE(mgr.get_open_file(1U, true, open_file));
  EXPECT_EQ(std::size_t(0U), mgr.get_open_file_count());
  EXPECT_FALSE(open_file);

  EXPECT_FALSE(mgr.get_open_file(1U, false, open_file));
  EXPECT_FALSE(open_file);
  EXPECT_EQ(std::size_t(0U), mgr.get_open_file_count());
}

TEST_F(file_manager_test, open_file_fails_if_provider_set_item_meta_fails) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);

  auto file = std::make_shared<mock_open_file>();
  EXPECT_CALL(*file, is_directory).WillRepeatedly(Return(false));
  EXPECT_CALL(*file, get_api_path).WillRepeatedly(Return("/test_open.txt"));
  EXPECT_CALL(*file, get_source_path).WillRepeatedly(Return("/test_open.src"));

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
  std::shared_ptr<i_open_file> open_file{};
#if defined(_WIN32)
  EXPECT_EQ(api_error::error, mgr.open(file, {}, handle, open_file));
#else
  EXPECT_EQ(api_error::error, mgr.open(file, O_RDWR, handle, open_file));
#endif
  EXPECT_FALSE(mgr.get_open_file(1U, true, open_file));
  EXPECT_EQ(std::size_t(0U), mgr.get_open_file_count());
  EXPECT_FALSE(open_file);

  EXPECT_FALSE(mgr.get_open_file(1U, false, open_file));
  EXPECT_FALSE(open_file);
  EXPECT_EQ(std::size_t(0U), mgr.get_open_file_count());
}

TEST_F(file_manager_test, open_file_creates_source_path_if_empty) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);

  auto file = std::make_shared<mock_open_file>();
  EXPECT_CALL(*file, add).WillOnce(Return());
  EXPECT_CALL(*file, is_directory).WillRepeatedly(Return(false));
  EXPECT_CALL(*file, is_write_supported).WillRepeatedly(Return(true));
  EXPECT_CALL(*file, get_api_path).WillRepeatedly(Return("/test_open.txt"));
  EXPECT_CALL(*file, get_source_path).WillRepeatedly(Return(""));

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
  std::shared_ptr<i_open_file> open_file{};
#if defined(_WIN32)
  EXPECT_EQ(api_error::success, mgr.open(file, {}, handle, open_file));
#else
  EXPECT_EQ(api_error::success, mgr.open(file, O_RDWR, handle, open_file));
#endif
  EXPECT_CALL(*file, has_handle(1)).Times(2).WillRepeatedly([]() -> bool {
    return true;
  });

  EXPECT_TRUE(mgr.get_open_file(1U, true, open_file));
  EXPECT_EQ(std::size_t(1U), mgr.get_open_file_count());
  EXPECT_TRUE(open_file);

  EXPECT_TRUE(mgr.get_open_file(1U, false, open_file));
  EXPECT_TRUE(open_file);
  EXPECT_EQ(std::size_t(1U), mgr.get_open_file_count());
}

TEST_F(file_manager_test, open_file_first_file_handle_is_not_zero) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);

  auto file = std::make_shared<mock_open_file>();
  EXPECT_CALL(*file, add).WillOnce(Return());
  EXPECT_CALL(*file, is_directory).WillRepeatedly(Return(false));
  EXPECT_CALL(*file, is_write_supported).WillRepeatedly(Return(true));
  EXPECT_CALL(*file, get_api_path).WillRepeatedly(Return("/test_open.txt"));
  EXPECT_CALL(*file, get_source_path).WillRepeatedly(Return("/test_open.src"));

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
  std::shared_ptr<i_open_file> open_file{};
#if defined(_WIN32)
  EXPECT_EQ(api_error::success, mgr.open(file, {}, handle, open_file));
#else
  EXPECT_EQ(api_error::success, mgr.open(file, O_RDWR, handle, open_file));
#endif
  EXPECT_CALL(*file, has_handle(1)).WillOnce([]() -> bool { return true; });

  EXPECT_TRUE(mgr.get_open_file(1U, true, open_file));
  EXPECT_GT(handle, std::uint64_t(0U));
}

TEST_F(file_manager_test, can_remove_file) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);

  {
    auto file = utils::file::file::open_or_create_file("./test_remove.txt");
    EXPECT_TRUE(*file);
  }
  EXPECT_TRUE(utils::file::file("./test_remove.txt").exists());

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

  EXPECT_EQ(api_error::success, mgr.remove_file("/test_remove.txt"));

  EXPECT_FALSE(utils::file::file("./test_remove.txt").exists());
}

TEST_F(file_manager_test, can_queue_and_remove_upload) {
  event_capture capture({
      "file_upload_queued",
      "download_resume_removed",
  });

  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);

  mock_open_file file{};
  EXPECT_CALL(file, get_api_path).WillOnce(Return("/test_queue.txt"));
  EXPECT_CALL(file, get_source_path).WillOnce(Return("/test_queue.src"));

  EXPECT_FALSE(mgr.is_processing("/test_queue.txt"));
  mgr.queue_upload(file);
  EXPECT_TRUE(mgr.is_processing("/test_queue.txt"));

  mgr.remove_upload("/test_queue.txt");
  EXPECT_FALSE(mgr.is_processing("/test_queue.txt"));

  capture.wait_for_empty();
}

TEST_F(file_manager_test, file_is_closed_after_download_timeout) {
  cfg->set_enable_download_timeout(true);
  cfg->set_download_timeout_secs(3U);

  polling::instance().start(cfg.get());

  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);
  mgr.start();

  auto source_path = utils::path::combine(cfg->get_cache_directory(),
                                          {utils::create_uuid_string()});

  event_consumer consumer("item_timeout", [](const event &evt) {
    const auto &evt2 = dynamic_cast<const item_timeout &>(evt);
    EXPECT_STREQ("/test_download_timeout.txt",
                 evt2.get_api_path().get<std::string>().c_str());
  });

  auto now = utils::time::get_time_now();
  auto meta = create_meta_attributes(
      now, FILE_ATTRIBUTE_ARCHIVE, now + 1u, now + 2u, false, 1, "key", 2,
      now + 3u, 3u, 4u,
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

  event_capture capture({"item_timeout"});

  EXPECT_CALL(mp, read_file_bytes)
      .WillRepeatedly([](const std::string & /* api_path */, std::size_t size,
                         std::uint64_t offset, data_buffer &data,
                         stop_type &stop_requested) -> api_error {
        if (stop_requested) {
          return api_error::download_stopped;
        }

        if (offset == 0U) {
          data.resize(size);
          return api_error::success;
        }

        while (not stop_requested) {
          std::this_thread::sleep_for(100ms);
        }

        return api_error::download_stopped;
      });

  std::uint64_t handle{};
  std::shared_ptr<i_open_file> open_file;
#if defined(_WIN32)
  EXPECT_EQ(api_error::success, mgr.open("/test_download_timeout.txt", false,
                                         {}, handle, open_file));
#else
  EXPECT_EQ(api_error::success, mgr.open("/test_download_timeout.txt", false,
                                         O_RDWR, handle, open_file));
#endif

  data_buffer data{};
  EXPECT_EQ(api_error::success, open_file->read(1U, 0U, data));

  mgr.close(handle);

  if (open_file->is_write_supported()) {
    EXPECT_CALL(mp, set_item_meta("/test_download_timeout.txt", META_SOURCE, _))
        .WillOnce(Return(api_error::success));
  }

  EXPECT_EQ(std::size_t(1U), mgr.get_open_file_count());
  capture.wait_for_empty();

  EXPECT_EQ(std::size_t(0U), mgr.get_open_file_count());
  mgr.stop();

  polling::instance().stop();
}

TEST_F(file_manager_test, remove_file_fails_if_file_does_not_exist) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);

  EXPECT_CALL(mp, get_filesystem_item)
      .WillOnce([](const std::string &api_path, const bool &directory,
                   filesystem_item & /*fsi*/) -> api_error {
        EXPECT_STREQ("/test_remove.txt", api_path.c_str());
        EXPECT_FALSE(directory);
        return api_error::item_not_found;
      });

  EXPECT_EQ(api_error::item_not_found, mgr.remove_file("/test_remove.txt"));
}

TEST_F(file_manager_test, remove_file_fails_if_provider_remove_file_fails) {
  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  file_manager mgr(*cfg, mp);

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

  EXPECT_EQ(api_error::item_not_found, mgr.remove_file("/test_remove.txt"));
}

TEST_F(file_manager_test,
       resize_greater_than_chunk_size_sets_new_chunks_to_read) {
  cfg->set_enable_download_timeout(true);

  EXPECT_CALL(mp, is_read_only()).WillRepeatedly(Return(false));

  polling::instance().start(cfg.get());

  file_manager mgr(*cfg, mp);
  mgr.start();

  event_capture capture({
      "item_timeout",
      "filesystem_item_opened",
      "filesystem_item_handle_opened",
      "filesystem_item_handle_closed",
      "filesystem_item_closed",
  });

  std::uint64_t handle{};
  EXPECT_CALL(mp, upload_file).Times(AnyNumber());

  auto source_path = utils::path::combine(cfg->get_cache_directory(),
                                          {utils::create_uuid_string()});
  auto now = utils::time::get_time_now();
  auto meta = create_meta_attributes(now, FILE_ATTRIBUTE_ARCHIVE, now + 1U,
                                     now + 2U, false, 1U, "key", 2, now + 3U,
                                     3U, 4U, 0U, source_path, 10U, now + 4U);

  EXPECT_CALL(mp, create_file("/test_create.txt", meta))
      .WillOnce(Return(api_error::success));

  EXPECT_CALL(mp, get_filesystem_item)
      .WillRepeatedly([&meta](const std::string &api_path, bool directory,
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

  EXPECT_CALL(mp, set_item_meta("/test_create.txt", _))
      .WillOnce([](const std::string &,
                   const api_meta_map &new_meta) -> api_error {
        EXPECT_EQ(utils::encryption::encrypting_reader::get_data_chunk_size() *
                      4UL,
                  utils::string::to_uint64(new_meta.at(META_SIZE)));
        return api_error::success;
      });
  {
    std::shared_ptr<i_open_file> open_file;

#if defined(_WIN32)
    EXPECT_EQ(api_error::success,
              mgr.create("/test_create.txt", meta, {}, handle, open_file));
#else  // !defined(_WIN32)
    EXPECT_EQ(api_error::success,
              mgr.create("/test_create.txt", meta, O_RDWR, handle, open_file));
#endif // defined(_WIN32)

    EXPECT_EQ(
        api_error::success,
        open_file->resize(
            utils::encryption::encrypting_reader::get_data_chunk_size() * 4UL));

    EXPECT_TRUE(open_file->get_read_state().all());
  }

  mgr.close(handle);

  capture.wait_for_empty();

  mgr.stop();

  polling::instance().stop();
}
} // namespace repertory
