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
#include "types/repertory.hpp"
#include "utils/event_capture.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
static constexpr const std::size_t test_chunk_size = 1024u;

static void test_closeable_open_file(const file_manager::open_file &o,
                                     bool directory, const api_error &e,
                                     std::uint64_t size,
                                     const std::string &source_path) {
  EXPECT_EQ(directory, o.is_directory());
  EXPECT_EQ(e, o.get_api_error());
  EXPECT_EQ(std::size_t(0u), o.get_open_file_count());
  EXPECT_EQ(std::uint64_t(size), o.get_file_size());
  EXPECT_STREQ(source_path.c_str(), o.get_source_path().c_str());
  EXPECT_TRUE(o.can_close());
}

static void validate_write(file_manager::open_file &o, std::size_t offset,
                           data_buffer data, std::size_t bytes_written) {
  EXPECT_EQ(data.size(), bytes_written);

  data_buffer read_data{};
  EXPECT_EQ(api_error::success, o.read(data.size(), offset, read_data));

  EXPECT_TRUE(std::equal(data.begin(), data.end(), read_data.begin()));
}

TEST(open_file, properly_initializes_state_for_0_byte_file) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;
  mock_upload_manager um;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = 0u;
  fsi.source_path = source_path;

  file_manager::open_file o(test_chunk_size, 0U, fsi, mp, um);
  EXPECT_EQ(std::size_t(0u), o.get_read_state().size());
  EXPECT_FALSE(o.is_modified());
  EXPECT_EQ(test_chunk_size, o.get_chunk_size());
}

TEST(open_file, properly_initializes_state_based_on_chunk_size) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;
  mock_upload_manager um;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = 8u;
  fsi.source_path = source_path;

  EXPECT_CALL(um, remove_resume)
      .WillOnce(
          [&fsi](const std::string &api_path, const std::string &source_path2) {
            EXPECT_EQ(fsi.api_path, api_path);
            EXPECT_EQ(fsi.source_path, source_path2);
          });

  file_manager::open_file o(1u, 0U, fsi, mp, um);
  EXPECT_EQ(std::size_t(8u), o.get_read_state().size());
  EXPECT_TRUE(o.get_read_state().none());

  EXPECT_FALSE(o.is_modified());

  EXPECT_CALL(mp, set_item_meta(fsi.api_path, META_SOURCE, _))
      .WillOnce(Return(api_error::success));
  EXPECT_EQ(std::size_t(1u), o.get_chunk_size());
}

TEST(open_file, will_not_change_source_path_for_0_byte_file) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;
  mock_upload_manager um;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.directory = false;
  fsi.api_path = "/test.txt";
  fsi.size = 0u;
  fsi.source_path = source_path;

  file_manager::open_file o(0u, 0U, fsi, mp, um);
  test_closeable_open_file(o, false, api_error::success, 0u, source_path);

  o.close();
  EXPECT_EQ(api_error::success, o.get_api_error());
  EXPECT_STREQ(source_path.c_str(), o.get_source_path().c_str());
  EXPECT_TRUE(utils::file::is_file(fsi.source_path));
}

TEST(open_file, will_change_source_path_if_file_size_is_greater_than_0) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;
  mock_upload_manager um;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size;
  fsi.source_path = source_path;

  EXPECT_CALL(um, remove_resume)
      .WillOnce(
          [&fsi](const std::string &api_path, const std::string &source_path2) {
            EXPECT_EQ(fsi.api_path, api_path);
            EXPECT_EQ(fsi.source_path, source_path2);
          });

  EXPECT_CALL(mp, set_item_meta(fsi.api_path, META_SOURCE, _))
      .WillOnce([&fsi](const std::string &, const std::string &,
                       const std::string &source_path2) -> api_error {
        EXPECT_STRNE(fsi.source_path.c_str(), source_path2.c_str());
        return api_error::success;
      });

  file_manager::open_file o(test_chunk_size, 0U, fsi, mp, um);
  test_closeable_open_file(o, false, api_error::success, test_chunk_size,
                           source_path);

  o.close();
  EXPECT_EQ(api_error::download_stopped, o.get_api_error());
  EXPECT_STRNE(source_path.c_str(), o.get_source_path().c_str());
  EXPECT_FALSE(utils::file::is_file(source_path));
}

TEST(open_file,
     will_not_change_source_path_if_file_size_matches_existing_source) {
  const auto source_path = generate_test_file_name(".", "test");
  create_random_file(source_path, test_chunk_size)->close();

  mock_provider mp;
  mock_upload_manager um;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size;
  fsi.source_path = source_path;

  file_manager::open_file o(test_chunk_size, 0U, fsi, mp, um);
  test_closeable_open_file(o, false, api_error::success, test_chunk_size,
                           source_path);

  o.close();
  EXPECT_EQ(api_error::success, o.get_api_error());
  EXPECT_STREQ(source_path.c_str(), o.get_source_path().c_str());
  EXPECT_TRUE(utils::file::is_file(source_path));
}

TEST(open_file, write_with_incomplete_download) {
  const auto source_path = generate_test_file_name(".", "test");
  auto nf = create_random_file(generate_test_file_name(".", "test_src"),
                               test_chunk_size * 2u);

  mock_provider mp;
  mock_upload_manager um;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 2u;
  fsi.source_path = source_path;

  file_manager::open_file o(test_chunk_size, 0U, fsi, mp, um);
  test_closeable_open_file(o, false, api_error::success, test_chunk_size * 2u,
                           source_path);

  EXPECT_CALL(mp, set_item_meta(fsi.api_path, _))
      .WillOnce([](const std::string &, const api_meta_map &meta) -> api_error {
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_CHANGED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_MODIFIED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_WRITTEN).empty()));
        return api_error::success;
      });

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

  EXPECT_CALL(um, remove_upload).WillOnce([&fsi](const std::string &api_path) {
    EXPECT_EQ(fsi.api_path, api_path);
  });

  EXPECT_CALL(um, store_resume)
      .Times(2)
      .WillRepeatedly([&fsi](const i_open_file &cur_file) {
        EXPECT_EQ(fsi.api_path, cur_file.get_api_path());
        EXPECT_EQ(fsi.source_path, cur_file.get_source_path());
      });

  data_buffer data = {10, 9, 8};
  std::size_t bytes_written{};
  EXPECT_EQ(api_error::success, o.write(0u, data, bytes_written));
  validate_write(o, 0u, data, bytes_written);

  const auto test_state = [&]() {
    EXPECT_STREQ(source_path.c_str(), o.get_source_path().c_str());

    EXPECT_FALSE(o.can_close());

    EXPECT_TRUE(o.is_modified());

    EXPECT_TRUE(o.get_read_state(0u));
    EXPECT_FALSE(o.get_read_state(1u));
  };
  test_state();

  o.close();
  nf->close();

  test_state();

  EXPECT_EQ(api_error::download_incomplete, o.get_api_error());

  EXPECT_TRUE(utils::file::is_file(fsi.source_path));
}

TEST(open_file, write_new_file) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;
  mock_upload_manager um;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = 0u;
  fsi.source_path = source_path;

  EXPECT_CALL(um, store_resume).WillOnce([&fsi](const i_open_file &o) {
    EXPECT_EQ(fsi.api_path, o.get_api_path());
    EXPECT_EQ(fsi.source_path, o.get_source_path());
  });

  file_manager::open_file o(test_chunk_size, 0U, fsi, mp, um);
  test_closeable_open_file(o, false, api_error::success, 0u, source_path);
  data_buffer data = {10, 9, 8};

  EXPECT_CALL(mp, set_item_meta(fsi.api_path, _))
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

  EXPECT_CALL(um, remove_upload).WillOnce([&fsi](const std::string &api_path) {
    EXPECT_EQ(fsi.api_path, api_path);
  });

  EXPECT_CALL(um, queue_upload).WillOnce([&fsi](const i_open_file &cur_file) {
    EXPECT_EQ(fsi.api_path, cur_file.get_api_path());
    EXPECT_EQ(fsi.source_path, cur_file.get_source_path());
  });

  std::size_t bytes_written{};
  EXPECT_EQ(api_error::success, o.write(0u, data, bytes_written));

  const auto test_state = [&]() {
    EXPECT_STREQ(source_path.c_str(), o.get_source_path().c_str());

    EXPECT_FALSE(o.can_close());
    EXPECT_TRUE(o.is_modified());

    EXPECT_TRUE(o.get_read_state(0u));
    EXPECT_EQ(std::size_t(1u), o.get_read_state().size());
    EXPECT_EQ(data.size(), o.get_file_size());
  };
  test_state();

  o.close();

  test_state();

  EXPECT_EQ(api_error::success, o.get_api_error());

  EXPECT_TRUE(utils::file::is_file(fsi.source_path));
}

TEST(open_file, write_new_file_multiple_chunks) {
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;
  mock_upload_manager um;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = 0u;
  fsi.source_path = source_path;

  EXPECT_CALL(um, store_resume).WillOnce([&fsi](const i_open_file &o) {
    EXPECT_EQ(fsi.api_path, o.get_api_path());
    EXPECT_EQ(fsi.source_path, o.get_source_path());
  });
  file_manager::open_file o(test_chunk_size, 0U, fsi, mp, um);
  test_closeable_open_file(o, false, api_error::success, 0u, source_path);
  data_buffer data = {10, 9, 8};

  EXPECT_CALL(mp, set_item_meta(fsi.api_path, _))
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

  EXPECT_CALL(um, remove_upload).WillOnce([&fsi](const std::string &api_path) {
    EXPECT_EQ(fsi.api_path, api_path);
  });

  EXPECT_CALL(um, queue_upload).WillOnce([&fsi](const i_open_file &cur_file) {
    EXPECT_EQ(fsi.api_path, cur_file.get_api_path());
    EXPECT_EQ(fsi.source_path, cur_file.get_source_path());
  });

  std::size_t bytes_written{};
  EXPECT_EQ(api_error::success, o.write(0u, data, bytes_written));
  EXPECT_EQ(api_error::success, o.write(test_chunk_size, data, bytes_written));

  const auto test_state = [&]() {
    EXPECT_STREQ(source_path.c_str(), o.get_source_path().c_str());

    EXPECT_FALSE(o.can_close());
    EXPECT_TRUE(o.is_modified());

    EXPECT_EQ(std::size_t(2u), o.get_read_state().size());
    for (std::size_t i = 0u; i < 2u; i++) {
      EXPECT_TRUE(o.get_read_state(i));
    }

    EXPECT_EQ(data.size() + test_chunk_size, o.get_file_size());
  };
  test_state();

  o.close();

  test_state();

  EXPECT_EQ(api_error::success, o.get_api_error());

  EXPECT_TRUE(utils::file::is_file(fsi.source_path));
}

TEST(open_file, resize_file_to_0_bytes) {
  const auto source_path = generate_test_file_name(".", "test");
  create_random_file(source_path, test_chunk_size * 4u)->close();

  mock_provider mp;
  mock_upload_manager um;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 4u;
  fsi.source_path = source_path;

  file_manager::open_file o(test_chunk_size, 0U, fsi, mp, um);
  test_closeable_open_file(o, false, api_error::success, fsi.size, source_path);
  EXPECT_CALL(mp, set_item_meta(fsi.api_path, _))
      .WillOnce([](const std::string &, const api_meta_map &meta) -> api_error {
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_CHANGED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_MODIFIED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_SIZE).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_WRITTEN).empty()));
        EXPECT_EQ(std::size_t(0u),
                  utils::string::to_size_t(meta.at(META_SIZE)));
        return api_error::success;
      });

  EXPECT_CALL(um, remove_upload).WillOnce([&fsi](const std ::string &api_path) {
    EXPECT_EQ(fsi.api_path, api_path);
  });

  EXPECT_CALL(um, queue_upload).WillOnce([&fsi](const i_open_file &cur_file) {
    EXPECT_EQ(fsi.api_path, cur_file.get_api_path());
    EXPECT_EQ(fsi.source_path, cur_file.get_source_path());
  });
  EXPECT_CALL(um, store_resume).WillOnce([&fsi](const i_open_file &cur_file) {
    EXPECT_EQ(fsi.api_path, cur_file.get_api_path());
    EXPECT_EQ(fsi.source_path, cur_file.get_source_path());
  });

  EXPECT_EQ(api_error::success, o.resize(0u));

  EXPECT_EQ(std::size_t(0u), o.get_file_size());
  EXPECT_FALSE(o.can_close());
  EXPECT_TRUE(o.is_modified());

  EXPECT_EQ(std::size_t(0u), o.get_read_state().size());
}

TEST(open_file, resize_file_by_full_chunk) {
  const auto source_path = generate_test_file_name(".", "test");
  create_random_file(source_path, test_chunk_size * 4u)->close();

  mock_provider mp;
  mock_upload_manager um;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 4u;
  fsi.source_path = source_path;

  EXPECT_CALL(um, store_resume).WillOnce([&fsi](const i_open_file &o) {
    EXPECT_EQ(fsi.api_path, o.get_api_path());
    EXPECT_EQ(fsi.source_path, o.get_source_path());
  });

  file_manager::open_file o(test_chunk_size, 0U, fsi, mp, um);
  test_closeable_open_file(o, false, api_error::success, fsi.size, source_path);
  EXPECT_CALL(mp, set_item_meta(fsi.api_path, _))
      .WillOnce([](const std::string &, const api_meta_map &meta) -> api_error {
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_CHANGED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_MODIFIED).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_SIZE).empty()));
        EXPECT_NO_THROW(EXPECT_FALSE(meta.at(META_WRITTEN).empty()));
        EXPECT_EQ(std::size_t(test_chunk_size * 3u),
                  utils::string::to_size_t(meta.at(META_SIZE)));
        return api_error::success;
      });

  EXPECT_CALL(um, remove_upload).WillOnce([&fsi](const std::string &api_path) {
    EXPECT_EQ(fsi.api_path, api_path);
  });

  EXPECT_CALL(um, queue_upload).WillOnce([&fsi](const i_open_file &cur_file) {
    EXPECT_EQ(fsi.api_path, cur_file.get_api_path());
    EXPECT_EQ(fsi.source_path, cur_file.get_source_path());
  });

  EXPECT_EQ(api_error::success, o.resize(test_chunk_size * 3u));

  EXPECT_EQ(std::size_t(test_chunk_size * 3u), o.get_file_size());
  EXPECT_FALSE(o.can_close());
  EXPECT_TRUE(o.is_modified());
  EXPECT_EQ(std::size_t(3u), o.get_read_state().size());
}

TEST(open_file, can_add_handle) {
  event_system::instance().start();
  console_consumer c;
  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;
  mock_upload_manager um;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 4u;
  fsi.source_path = source_path;

  event_consumer ec("filesystem_item_opened", [&fsi](const event &e) {
    const auto &ee = dynamic_cast<const filesystem_item_opened &>(e);
    EXPECT_STREQ(fsi.api_path.c_str(),
                 ee.get_api_path().get<std::string>().c_str());
    EXPECT_STREQ(fsi.source_path.c_str(),
                 ee.get_source().get<std::string>().c_str());
    EXPECT_STREQ("0", ee.get_directory().get<std::string>().c_str());
  });

  event_consumer ec2("filesystem_item_handle_opened", [&fsi](const event &e) {
    const auto &ee = dynamic_cast<const filesystem_item_handle_opened &>(e);
    EXPECT_STREQ(fsi.api_path.c_str(),
                 ee.get_api_path().get<std::string>().c_str());
    EXPECT_STREQ(fsi.source_path.c_str(),
                 ee.get_source().get<std::string>().c_str());
    EXPECT_STREQ("0", ee.get_directory().get<std::string>().c_str());
    EXPECT_STREQ("1", ee.get_handle().get<std::string>().c_str());
  });

  EXPECT_CALL(mp, set_item_meta(fsi.api_path, META_SOURCE, _))
      .WillOnce(Return(api_error::success));
  EXPECT_CALL(um, remove_resume)
      .WillOnce(
          [&fsi](const std::string &api_path, const std::string &source_path2) {
            EXPECT_EQ(fsi.api_path, api_path);
            EXPECT_EQ(fsi.source_path, source_path2);
          });

  event_capture capture(
      {"filesystem_item_opened", "filesystem_item_handle_opened"});

  file_manager::open_file o(test_chunk_size, 0U, fsi, mp, um);
#ifdef _WIN32
  o.add(1u, {});
  EXPECT_EQ(nullptr, o.get_open_data(1u).directory_buffer);
#else
  o.add(1u, O_RDWR | O_SYNC);
  EXPECT_EQ(O_RDWR | O_SYNC, o.get_open_data(1u));
#endif

  capture.wait_for_empty();

  event_system::instance().stop();
}

TEST(open_file, can_remove_handle) {
  event_system::instance().start();
  console_consumer c;

  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;
  mock_upload_manager um;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 4u;
  fsi.source_path = source_path;

  event_consumer ec("filesystem_item_closed", [&fsi](const event &e) {
    const auto &ee = dynamic_cast<const filesystem_item_closed &>(e);
    EXPECT_STREQ(fsi.api_path.c_str(),
                 ee.get_api_path().get<std::string>().c_str());
    EXPECT_STREQ(fsi.source_path.c_str(),
                 ee.get_source().get<std::string>().c_str());
    EXPECT_STREQ("0", ee.get_directory().get<std::string>().c_str());
  });

  event_consumer ec2("filesystem_item_handle_closed", [&fsi](const event &e) {
    const auto &ee = dynamic_cast<const filesystem_item_handle_closed &>(e);
    EXPECT_STREQ(fsi.api_path.c_str(),
                 ee.get_api_path().get<std::string>().c_str());
    EXPECT_STREQ(fsi.source_path.c_str(),
                 ee.get_source().get<std::string>().c_str());
    EXPECT_STREQ("0", ee.get_directory().get<std::string>().c_str());
    EXPECT_STREQ("1", ee.get_handle().get<std::string>().c_str());
  });

  EXPECT_CALL(um, remove_resume)
      .WillOnce(
          [&fsi](const std::string &api_path, const std::string &source_path2) {
            EXPECT_EQ(fsi.api_path, api_path);
            EXPECT_EQ(fsi.source_path, source_path2);
          });
  EXPECT_CALL(mp, set_item_meta(fsi.api_path, META_SOURCE, _))
      .WillOnce(Return(api_error::success));

  event_capture capture({
      "filesystem_item_opened",
      "filesystem_item_handle_opened",
      "filesystem_item_handle_closed",
      "filesystem_item_closed",
  });

  file_manager::open_file o(test_chunk_size, 0U, fsi, mp, um);
#ifdef _WIN32
  o.add(1u, {});
#else
  o.add(1u, O_RDWR | O_SYNC);
#endif
  o.remove(1u);

  capture.wait_for_empty();

  event_system::instance().stop();
}

TEST(open_file,
     can_read_locally_after_write_with_file_size_greater_than_existing_size) {}

TEST(open_file, test_valid_download_chunks) {}

TEST(open_file, test_full_download_with_partial_chunk) {}

TEST(open_file, source_is_read_after_full_download) {}
} // namespace repertory
