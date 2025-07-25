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

#include "app_config.hpp"
#include "events/types/filesystem_item_closed.hpp"
#include "events/types/filesystem_item_handle_closed.hpp"
#include "events/types/filesystem_item_handle_opened.hpp"
#include "events/types/filesystem_item_opened.hpp"
#include "file_manager/cache_size_mgr.hpp"
#include "file_manager/open_file.hpp"
#include "mocks/mock_provider.hpp"
#include "mocks/mock_upload_manager.hpp"
#include "types/repertory.hpp"
#include "utils/event_capture.hpp"
#include "utils/path.hpp"

namespace {
constexpr std::size_t test_chunk_size{1024U};
}

namespace repertory {
class open_file_test : public ::testing::Test {
public:
  console_consumer con_consumer;
  std::unique_ptr<app_config> cfg;
  static std::atomic<std::size_t> inst;
  mock_provider provider;
  mock_upload_manager upload_mgr;

protected:
  void SetUp() override {
    event_system::instance().start();

    auto open_file_dir = repertory::utils::path::combine(
        repertory::test::get_test_output_dir(),
        {"open_file_test" + std::to_string(++inst)});

    cfg = std::make_unique<app_config>(provider_type::sia, open_file_dir);

    cache_size_mgr::instance().initialize(cfg.get());
  }

  void TearDown() override { event_system::instance().stop(); }
};

std::atomic<std::size_t> open_file_test::inst{0U};

static void test_closeable_open_file(const open_file &file, bool directory,
                                     api_error err, std::uint64_t size,
                                     const std::string &source_path) {
  EXPECT_EQ(directory, file.is_directory());
  EXPECT_EQ(err, file.get_api_error());
  EXPECT_EQ(std::size_t(0U), file.get_open_file_count());
  EXPECT_EQ(std::uint64_t(size), file.get_file_size());
  EXPECT_STREQ(source_path.c_str(), file.get_source_path().c_str());
  EXPECT_TRUE(file.can_close());
}

static void validate_write(open_file &file, std::size_t offset,
                           data_buffer data, std::size_t bytes_written) {
  EXPECT_EQ(data.size(), bytes_written);

  data_buffer read_data{};
  EXPECT_EQ(api_error::success, file.read(data.size(), offset, read_data));

  EXPECT_TRUE(std::equal(data.begin(), data.end(), read_data.begin()));
}

TEST_F(open_file_test, properly_initializes_state_for_0_byte_file) {
  const auto source_path =
      test::generate_test_file_name("file_manager_open_file_test");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = 0U;
  fsi.source_path = source_path;

  EXPECT_CALL(upload_mgr, remove_resume)
      .WillOnce(
          [&fsi](const std::string &api_path, const std::string &source_path2) {
            EXPECT_EQ(fsi.api_path, api_path);
            EXPECT_EQ(fsi.source_path, source_path2);
          });

  open_file file(test_chunk_size, 0U, fsi, provider, upload_mgr);
  EXPECT_EQ(std::size_t(0U), file.get_read_state().size());
  EXPECT_FALSE(file.is_modified());
  EXPECT_EQ(test_chunk_size, file.get_chunk_size());
}

TEST_F(open_file_test, properly_initializes_state_based_on_chunk_size) {
  const auto source_path =
      test::generate_test_file_name("file_manager_open_file_test");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = 8U;
  fsi.source_path = source_path;

  EXPECT_CALL(upload_mgr, remove_resume)
      .WillOnce(
          [&fsi](const std::string &api_path, const std::string &source_path2) {
            EXPECT_EQ(fsi.api_path, api_path);
            EXPECT_EQ(fsi.source_path, source_path2);
          });

  open_file file(1U, 0U, fsi, provider, upload_mgr);
  EXPECT_EQ(std::size_t(8U), file.get_read_state().size());
  EXPECT_TRUE(file.get_read_state().none());

  EXPECT_FALSE(file.is_modified());

  EXPECT_CALL(provider, set_item_meta(fsi.api_path, META_SOURCE, _))
      .WillOnce(Return(api_error::success));
  EXPECT_EQ(std::size_t(1U), file.get_chunk_size());
}

TEST_F(open_file_test, will_not_change_source_path_for_0_byte_file) {
  const auto source_path =
      test::generate_test_file_name("file_manager_open_file_test");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = 0U;
  fsi.source_path = source_path;

  EXPECT_CALL(upload_mgr, remove_resume)
      .WillOnce(
          [&fsi](const std::string &api_path, const std::string &source_path2) {
            EXPECT_EQ(fsi.api_path, api_path);
            EXPECT_EQ(fsi.source_path, source_path2);
          });

  open_file file(0U, 0U, fsi, provider, upload_mgr);
  test_closeable_open_file(file, false, api_error::success, 0U, source_path);

  file.close();
  EXPECT_EQ(api_error::success, file.get_api_error());
  EXPECT_STREQ(source_path.c_str(), file.get_source_path().c_str());
  EXPECT_TRUE(utils::file::file(fsi.source_path).exists());
}

TEST_F(open_file_test, will_change_source_path_if_file_size_is_greater_than_0) {
  const auto source_path =
      test::generate_test_file_name("file_manager_open_file_test");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size;
  fsi.source_path = source_path;

  EXPECT_CALL(upload_mgr, remove_resume)
      .WillOnce(
          [&fsi](const std::string &api_path, const std::string &source_path2) {
            EXPECT_EQ(fsi.api_path, api_path);
            EXPECT_EQ(fsi.source_path, source_path2);
          });

  EXPECT_CALL(provider, set_item_meta(fsi.api_path, META_SOURCE, _))
      .WillOnce([&fsi](const std::string &, const std::string &,
                       const std::string &source_path2) -> api_error {
        EXPECT_STRNE(fsi.source_path.c_str(), source_path2.c_str());
        return api_error::success;
      });

  open_file file(test_chunk_size, 0U, fsi, provider, upload_mgr);
  test_closeable_open_file(file, false, api_error::success, test_chunk_size,
                           source_path);

  file.close();
  EXPECT_EQ(api_error::download_stopped, file.get_api_error());
  EXPECT_STRNE(source_path.c_str(), file.get_source_path().c_str());
  EXPECT_FALSE(utils::file::file(source_path).exists());
}

TEST_F(open_file_test,
       will_not_change_source_path_if_file_size_matches_existing_source) {
  auto &rf = test::create_random_file(test_chunk_size);
  const auto source_path = rf.get_path();
  rf.close();

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size;
  fsi.source_path = source_path;

  EXPECT_CALL(upload_mgr, remove_resume)
      .WillOnce(
          [&fsi](const std::string &api_path, const std::string &source_path2) {
            EXPECT_EQ(fsi.api_path, api_path);
            EXPECT_EQ(fsi.source_path, source_path2);
          });

  open_file file(test_chunk_size, 0U, fsi, provider, upload_mgr);
  test_closeable_open_file(file, false, api_error::success, test_chunk_size,
                           source_path);

  file.close();
  EXPECT_EQ(api_error::success, file.get_api_error());
  EXPECT_STREQ(source_path.c_str(), file.get_source_path().c_str());
  EXPECT_TRUE(utils::file::file(source_path).exists());
}

TEST_F(open_file_test, write_with_incomplete_download) {
  const auto source_path = test::generate_test_file_name("test");
  auto &nf = test::create_random_file(test_chunk_size * 2u);

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 2u;
  fsi.source_path = source_path;

  open_file file(test_chunk_size, 0U, fsi, provider, upload_mgr);
  test_closeable_open_file(file, false, api_error::success,
                           test_chunk_size * 2u, source_path);

  EXPECT_CALL(provider, set_item_meta(fsi.api_path, _))
      .WillOnce([](const std::string &, const api_meta_map &meta) -> api_error {
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_CHANGED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_MODIFIED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_WRITTEN).empty()));
        return api_error::success;
      });

  EXPECT_CALL(provider, read_file_bytes)
      .WillRepeatedly([&nf](const std::string & /* api_path */,
                            std::size_t size, std::uint64_t offset,
                            data_buffer &data,
                            stop_type &stop_requested) -> api_error {
        if (stop_requested) {
          return api_error::download_stopped;
        }

        if (offset == 0U) {
          std::size_t bytes_read{};
          data.resize(size);
          auto ret = nf.read(data, offset, &bytes_read) ? api_error::success
                                                        : api_error::os_error;
          EXPECT_EQ(bytes_read, data.size());
          return ret;
        }

        while (not stop_requested) {
          std::this_thread::sleep_for(100ms);
        }
        return api_error::download_stopped;
      });

  EXPECT_CALL(upload_mgr, remove_upload)
      .WillOnce([&fsi](const std::string &api_path) {
        EXPECT_EQ(fsi.api_path, api_path);
      });

  EXPECT_CALL(upload_mgr, store_resume)
      .Times(2)
      .WillRepeatedly([&fsi](const i_open_file &cur_file) {
        EXPECT_EQ(fsi.api_path, cur_file.get_api_path());
        EXPECT_EQ(fsi.source_path, cur_file.get_source_path());
      });

  data_buffer data = {10, 9, 8};
  std::size_t bytes_written{};
  EXPECT_EQ(api_error::success, file.write(0U, data, bytes_written));
  validate_write(file, 0U, data, bytes_written);

  const auto test_state = [&]() {
    EXPECT_STREQ(source_path.c_str(), file.get_source_path().c_str());

    EXPECT_FALSE(file.can_close());

    EXPECT_TRUE(file.is_modified());

    EXPECT_TRUE(file.get_read_state(0U));
    EXPECT_FALSE(file.get_read_state(1u));
  };
  test_state();

  file.close();
  nf.close();

  test_state();

  EXPECT_EQ(api_error::download_incomplete, file.get_api_error());

  EXPECT_TRUE(utils::file::file(fsi.source_path).exists());
}

TEST_F(open_file_test, write_new_file) {
  const auto source_path =
      test::generate_test_file_name("file_manager_open_file_test");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = 0U;
  fsi.source_path = source_path;

  EXPECT_CALL(upload_mgr, store_resume)
      .WillOnce([&fsi](const i_open_file &file) {
        EXPECT_EQ(fsi.api_path, file.get_api_path());
        EXPECT_EQ(fsi.source_path, file.get_source_path());
      });

  open_file file(test_chunk_size, 0U, fsi, provider, upload_mgr);
  test_closeable_open_file(file, false, api_error::success, 0U, source_path);
  data_buffer data = {10, 9, 8};

  EXPECT_CALL(provider, set_item_meta(fsi.api_path, _))
      .WillOnce([&data](const std::string &,
                        const api_meta_map &meta) -> api_error {
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_CHANGED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_MODIFIED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_SIZE).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_WRITTEN).empty()));
        EXPECT_EQ(data.size(), utils::string::to_size_t(meta.at(META_SIZE)));
        return api_error::success;
      })
      .WillOnce([](const std::string &, const api_meta_map &meta) -> api_error {
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_CHANGED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_MODIFIED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_WRITTEN).empty()));
        return api_error::success;
      });

  EXPECT_CALL(upload_mgr, remove_upload)
      .WillOnce([&fsi](const std::string &api_path) {
        EXPECT_EQ(fsi.api_path, api_path);
      });

  EXPECT_CALL(upload_mgr, queue_upload)
      .WillOnce([&fsi](const i_open_file &cur_file) {
        EXPECT_EQ(fsi.api_path, cur_file.get_api_path());
        EXPECT_EQ(fsi.source_path, cur_file.get_source_path());
      });

  std::size_t bytes_written{};
  EXPECT_EQ(api_error::success, file.write(0U, data, bytes_written));

  const auto test_state = [&]() {
    EXPECT_STREQ(source_path.c_str(), file.get_source_path().c_str());

    EXPECT_FALSE(file.can_close());
    EXPECT_TRUE(file.is_modified());

    EXPECT_TRUE(file.get_read_state(0U));
    EXPECT_EQ(std::size_t(1u), file.get_read_state().size());
    EXPECT_EQ(data.size(), file.get_file_size());
  };
  test_state();

  file.close();

  test_state();

  EXPECT_EQ(api_error::success, file.get_api_error());

  EXPECT_TRUE(utils::file::file(fsi.source_path).exists());
}

TEST_F(open_file_test, write_new_file_multiple_chunks) {
  const auto source_path =
      test::generate_test_file_name("file_manager_open_file_test");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = 0U;
  fsi.source_path = source_path;

  EXPECT_CALL(upload_mgr, store_resume)
      .WillOnce([&fsi](const i_open_file &file) {
        EXPECT_EQ(fsi.api_path, file.get_api_path());
        EXPECT_EQ(fsi.source_path, file.get_source_path());
      });
  open_file file(test_chunk_size, 0U, fsi, provider, upload_mgr);
  test_closeable_open_file(file, false, api_error::success, 0U, source_path);
  data_buffer data = {10, 9, 8};

  EXPECT_CALL(provider, set_item_meta(fsi.api_path, _))
      .WillOnce([&data](const std::string &,
                        const api_meta_map &meta) -> api_error {
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_CHANGED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_MODIFIED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_SIZE).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_WRITTEN).empty()));
        EXPECT_EQ(data.size(), utils::string::to_size_t(meta.at(META_SIZE)));
        return api_error::success;
      })
      .WillOnce([](const std::string &, const api_meta_map &meta) -> api_error {
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_CHANGED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_MODIFIED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_WRITTEN).empty()));
        return api_error::success;
      })
      .WillOnce(
          [&data](const std::string &, const api_meta_map &meta) -> api_error {
            EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_CHANGED).empty()));
            EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_MODIFIED).empty()));
            EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_SIZE).empty()));
            EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_WRITTEN).empty()));
            EXPECT_EQ(data.size() + test_chunk_size,
                      utils::string::to_size_t(meta.at(META_SIZE)));
            return api_error::success;
          })
      .WillOnce([](const std::string &, const api_meta_map &meta) -> api_error {
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_CHANGED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_MODIFIED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_WRITTEN).empty()));
        return api_error::success;
      });

  EXPECT_CALL(upload_mgr, remove_upload)
      .WillOnce([&fsi](const std::string &api_path) {
        EXPECT_EQ(fsi.api_path, api_path);
      });

  EXPECT_CALL(upload_mgr, queue_upload)
      .WillOnce([&fsi](const i_open_file &cur_file) {
        EXPECT_EQ(fsi.api_path, cur_file.get_api_path());
        EXPECT_EQ(fsi.source_path, cur_file.get_source_path());
      });

  std::size_t bytes_written{};
  EXPECT_EQ(api_error::success, file.write(0U, data, bytes_written));
  EXPECT_EQ(api_error::success,
            file.write(test_chunk_size, data, bytes_written));

  const auto test_state = [&]() {
    EXPECT_STREQ(source_path.c_str(), file.get_source_path().c_str());

    EXPECT_FALSE(file.can_close());
    EXPECT_TRUE(file.is_modified());

    EXPECT_EQ(std::size_t(2u), file.get_read_state().size());
    for (std::size_t i = 0U; i < 2u; i++) {
      EXPECT_TRUE(file.get_read_state(i));
    }

    EXPECT_EQ(data.size() + test_chunk_size, file.get_file_size());
  };
  test_state();

  file.close();

  test_state();

  EXPECT_EQ(api_error::success, file.get_api_error());

  EXPECT_TRUE(utils::file::file(fsi.source_path).exists());
}

TEST_F(open_file_test, resize_file_to_0_bytes) {
  auto &r_file = test::create_random_file(test_chunk_size * 4U);
  const auto source_path = r_file.get_path();
  r_file.close();

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 4U;
  fsi.source_path = source_path;

  EXPECT_EQ(api_error::success, cache_size_mgr::instance().expand(fsi.size));

  open_file file(test_chunk_size, 0U, fsi, provider, upload_mgr);
  test_closeable_open_file(file, false, api_error::success, fsi.size,
                           source_path);
  EXPECT_CALL(provider, set_item_meta(fsi.api_path, _))
      .WillOnce([](const std::string &, const api_meta_map &meta) -> api_error {
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_CHANGED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_MODIFIED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_SIZE).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_WRITTEN).empty()));
        EXPECT_EQ(std::size_t(0U),
                  utils::string::to_size_t(meta.at(META_SIZE)));
        return api_error::success;
      });

  EXPECT_CALL(upload_mgr, remove_upload)
      .WillOnce([&fsi](const std ::string &api_path) {
        EXPECT_EQ(fsi.api_path, api_path);
      });

  EXPECT_CALL(upload_mgr, queue_upload)
      .WillOnce([&fsi](const i_open_file &cur_file) {
        EXPECT_EQ(fsi.api_path, cur_file.get_api_path());
        EXPECT_EQ(fsi.source_path, cur_file.get_source_path());
      });
  EXPECT_CALL(upload_mgr, store_resume)
      .WillOnce([&fsi](const i_open_file &cur_file) {
        EXPECT_EQ(fsi.api_path, cur_file.get_api_path());
        EXPECT_EQ(fsi.source_path, cur_file.get_source_path());
      });

  EXPECT_EQ(api_error::success, file.resize(0U));

  EXPECT_EQ(std::size_t(0U), file.get_file_size());
  EXPECT_FALSE(file.can_close());
  EXPECT_TRUE(file.is_modified());

  EXPECT_EQ(std::size_t(0U), file.get_read_state().size());
}

TEST_F(open_file_test, resize_file_by_full_chunk) {
  auto &r_file = test::create_random_file(test_chunk_size * 4U);
  const auto source_path = r_file.get_path();
  r_file.close();

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 4U;
  fsi.source_path = source_path;

  EXPECT_EQ(api_error::success, cache_size_mgr::instance().expand(fsi.size));

  EXPECT_CALL(upload_mgr, store_resume)
      .WillOnce([&fsi](const i_open_file &file) {
        EXPECT_EQ(fsi.api_path, file.get_api_path());
        EXPECT_EQ(fsi.source_path, file.get_source_path());
      });

  open_file file(test_chunk_size, 0U, fsi, provider, upload_mgr);
  test_closeable_open_file(file, false, api_error::success, fsi.size,
                           source_path);
  EXPECT_CALL(provider, set_item_meta(fsi.api_path, _))
      .WillOnce([](const std::string &, const api_meta_map &meta) -> api_error {
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_CHANGED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_MODIFIED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_SIZE).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_WRITTEN).empty()));
        EXPECT_EQ(std::size_t(test_chunk_size * 3U),
                  utils::string::to_size_t(meta.at(META_SIZE)));
        return api_error::success;
      });

  EXPECT_CALL(upload_mgr, remove_upload)
      .WillOnce([&fsi](const std::string &api_path) {
        EXPECT_EQ(fsi.api_path, api_path);
      });

  EXPECT_CALL(upload_mgr, queue_upload)
      .WillOnce([&fsi](const i_open_file &cur_file) {
        EXPECT_EQ(fsi.api_path, cur_file.get_api_path());
        EXPECT_EQ(fsi.source_path, cur_file.get_source_path());
      });

  EXPECT_EQ(api_error::success, file.resize(test_chunk_size * 3U));

  EXPECT_EQ(std::size_t(test_chunk_size * 3U), file.get_file_size());
  EXPECT_FALSE(file.can_close());
  EXPECT_TRUE(file.is_modified());
  EXPECT_EQ(std::size_t(3U), file.get_read_state().size());
}

TEST_F(open_file_test, can_add_handle) {
  event_system::instance().start();
  console_consumer c;
  const auto source_path =
      test::generate_test_file_name("file_manager_open_file_test");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 4u;
  fsi.source_path = source_path;

  event_consumer ec(filesystem_item_opened::name, [&fsi](const i_event &e) {
    const auto &ee = dynamic_cast<const filesystem_item_opened &>(e);
    EXPECT_STREQ(fsi.api_path.c_str(), ee.api_path.c_str());
    EXPECT_STREQ(fsi.source_path.c_str(), ee.source_path.c_str());
    EXPECT_FALSE(ee.directory);
  });

  event_consumer ec2(
      filesystem_item_handle_opened::name, [&fsi](const i_event &e) {
        const auto &ee = dynamic_cast<const filesystem_item_handle_opened &>(e);
        EXPECT_STREQ(fsi.api_path.c_str(), ee.api_path.c_str());
        EXPECT_STREQ(fsi.source_path.c_str(), ee.source_path.c_str());
        EXPECT_FALSE(ee.directory);
        EXPECT_EQ(std::uint64_t(1U), ee.handle);
      });

  EXPECT_CALL(provider, set_item_meta(fsi.api_path, META_SOURCE, _))
      .WillOnce(Return(api_error::success));
  EXPECT_CALL(upload_mgr, remove_resume)
      .WillOnce(
          [&fsi](const std::string &api_path, const std::string &source_path2) {
            EXPECT_EQ(fsi.api_path, api_path);
            EXPECT_EQ(fsi.source_path, source_path2);
          });

  event_capture capture({
      filesystem_item_opened::name,
      filesystem_item_handle_opened::name,
  });

  open_file o(test_chunk_size, 0U, fsi, provider, upload_mgr);
#if defined(_WIN32)
  o.add(1u, {});
  EXPECT_EQ(nullptr, o.get_open_data(1u).directory_buffer);
#else
  o.add(1u, O_RDWR | O_SYNC);
  EXPECT_EQ(O_RDWR | O_SYNC, o.get_open_data(1u));
#endif

  capture.wait_for_empty();

  event_system::instance().stop();
}

TEST_F(open_file_test, can_remove_handle) {
  console_consumer c;

  event_system::instance().start();

  const auto source_path =
      test::generate_test_file_name("file_manager_open_file_test");

  EXPECT_CALL(provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 4u;
  fsi.source_path = source_path;

  event_consumer ec(filesystem_item_closed::name, [&fsi](const i_event &e) {
    const auto &ee = dynamic_cast<const filesystem_item_closed &>(e);
    EXPECT_STREQ(fsi.api_path.c_str(), ee.api_path.c_str());
    EXPECT_STREQ(fsi.source_path.c_str(), ee.source_path.c_str());
    EXPECT_FALSE(ee.directory);
  });

  event_consumer ec2(
      filesystem_item_handle_closed::name, [&fsi](const i_event &e) {
        const auto &ee = dynamic_cast<const filesystem_item_handle_closed &>(e);
        EXPECT_STREQ(fsi.api_path.c_str(), ee.api_path.c_str());
        EXPECT_STREQ(fsi.source_path.c_str(), ee.source_path.c_str());
        EXPECT_FALSE(ee.directory);
        EXPECT_EQ(std::uint64_t(1U), ee.handle);
      });

  EXPECT_CALL(upload_mgr, remove_resume)
      .WillOnce(
          [&fsi](const std::string &api_path, const std::string &source_path2) {
            EXPECT_EQ(fsi.api_path, api_path);
            EXPECT_EQ(fsi.source_path, source_path2);
          });
  EXPECT_CALL(provider, set_item_meta(fsi.api_path, META_SOURCE, _))
      .WillOnce(Return(api_error::success));

  event_capture capture({
      filesystem_item_opened::name,
      filesystem_item_handle_opened::name,
      filesystem_item_handle_closed::name,
      filesystem_item_closed::name,
  });

  open_file o(test_chunk_size, 0U, fsi, provider, upload_mgr);
#if defined(_WIN32)
  o.add(1u, {});
#else
  o.add(1u, O_RDWR | O_SYNC);
#endif
  o.remove(1u);

  capture.wait_for_empty();

  event_system::instance().stop();
}

TEST_F(open_file_test,
       can_read_locally_after_write_with_file_size_greater_than_existing_size) {
}

TEST_F(open_file_test, test_valid_download_chunks) {}

TEST_F(open_file_test, test_full_download_with_partial_chunk) {}

TEST_F(open_file_test, source_is_read_after_full_download) {}
} // namespace repertory
