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
#include "comm/curl/curl_comm.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/event_system.hpp"
#include "file_manager/file_manager.hpp"
#include "platform/platform.hpp"
#include "providers/sia/sia_provider.hpp"
#include "types/startup_exception.hpp"
#include "utils/path_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
/* TEST(sia_provider, can_construct_sia_provider) {
  app_config cfg(provider_type::sia,
                 utils::path::combine(get_test_dir(), {"sia"}));
  curl_comm comm(cfg.get_host_config());
  sia_provider provider(cfg, comm);
  EXPECT_EQ(provider_type::sia, provider.get_provider_type());
  EXPECT_FALSE(provider.is_direct_only());
  EXPECT_FALSE(provider.is_rename_supported());
}

TEST(sia_provider, can_create_and_remove_directory) {
  console_consumer cc{};
  event_system::instance().start();

  app_config cfg(provider_type::sia,
                 utils::path::combine(get_test_dir(), {"sia"}));
  curl_comm comm(cfg.get_host_config());
  sia_provider sia(cfg, comm);
  i_provider &provider = sia;

  EXPECT_TRUE(provider.start(
      [&](bool directory, api_file &file) -> api_error {
        return provider_meta_handler(provider, directory, file);
      },
      nullptr));

  api_meta_map meta{};
  meta[META_ACCESSED] = std::to_string(utils::get_file_time_now());
  meta[META_CHANGED] = std::to_string(utils::get_file_time_now());
  meta[META_CREATION] = std::to_string(utils::get_file_time_now());
  meta[META_MODIFIED] = std::to_string(utils::get_file_time_now());
  EXPECT_EQ(api_error::success, provider.create_directory("/moose2", meta));

  bool exists{};
  EXPECT_EQ(api_error::success, provider.is_directory("/moose2", exists));
  EXPECT_TRUE(exists);

  EXPECT_EQ(api_error::success, provider.is_file("/moose2", exists));
  EXPECT_FALSE(exists);

  EXPECT_FALSE(provider.is_file_writeable("/moose2"));
  EXPECT_EQ(api_error::directory_exists, provider.remove_file("/moose2"));
  EXPECT_EQ(api_error::success, provider.remove_directory("/moose2"));

  api_meta_map m{};
  EXPECT_EQ(api_error::item_not_found, provider.get_item_meta("/moose2", m));
  EXPECT_TRUE(m.empty());

  provider.stop();

  event_system::instance().stop();
}

TEST(sia_provider, can_create_and_remove_file) {
  console_consumer cc{};
  event_system::instance().start();

  app_config cfg(provider_type::sia,
                 utils::path::combine(get_test_dir(), {"sia"}));
  curl_comm comm(cfg.get_host_config());
  sia_provider sia(cfg, comm);
  i_provider &provider = sia;

  EXPECT_TRUE(provider.start(
      [&](bool directory, api_file &file) -> api_error {
        return provider_meta_handler(provider, directory, file);
      },
      nullptr));

  api_meta_map meta{};
  meta[META_ACCESSED] = std::to_string(utils::get_file_time_now());
  meta[META_CHANGED] = std::to_string(utils::get_file_time_now());
  meta[META_CREATION] = std::to_string(utils::get_file_time_now());
  meta[META_MODIFIED] = std::to_string(utils::get_file_time_now());
  meta[META_SIZE] = "0";
  EXPECT_EQ(api_error::success, provider.create_file("/moose.txt", meta));

  bool exists{};
  EXPECT_EQ(api_error::success, provider.is_file("/moose.txt", exists));
  EXPECT_TRUE(exists);

  EXPECT_EQ(api_error::success, provider.is_directory("/moose.txt", exists));
  EXPECT_FALSE(exists);

  EXPECT_TRUE(provider.is_file_writeable("/moose.txt"));
  EXPECT_EQ(api_error::item_not_found, provider.remove_directory("/moose.txt"));
  EXPECT_EQ(api_error::success, provider.remove_file("/moose.txt"));

  api_meta_map m{};
  EXPECT_EQ(api_error::item_not_found, provider.get_item_meta("/moose.txt", m));
  EXPECT_TRUE(m.empty());

  provider.stop();

  event_system::instance().stop();
}

TEST(sia_provider, can_get_file_list) {
  console_consumer cc{};
  event_system::instance().start();

  app_config cfg(provider_type::sia,
                 utils::path::combine(get_test_dir(), {"sia"}));
  curl_comm comm(cfg.get_host_config());
  sia_provider sia(cfg, comm);
  i_provider &provider = sia;

  EXPECT_TRUE(provider.start(
      [&](bool directory, api_file &file) -> api_error {
        return provider_meta_handler(provider, directory, file);
      },
      nullptr));

  api_file_list list{};
  EXPECT_EQ(api_error::success, provider.get_file_list(list));

  provider.stop();

  event_system::instance().stop();
}

TEST(sia_provider, can_get_and_set_item_meta) {
  console_consumer cc{};
  event_system::instance().start();

  app_config cfg(provider_type::sia,
                 utils::path::combine(get_test_dir(), {"sia"}));
  curl_comm comm(cfg.get_host_config());
  sia_provider sia(cfg, comm);
  i_provider &provider = sia;

  EXPECT_TRUE(provider.start(
      [&](bool directory, api_file &file) -> api_error {
        return provider_meta_handler(provider, directory, file);
      },
      nullptr));

  api_meta_map meta{};
  meta[META_ACCESSED] = std::to_string(utils::get_file_time_now());
  meta[META_CHANGED] = std::to_string(utils::get_file_time_now());
  meta[META_CREATION] = std::to_string(utils::get_file_time_now());
  meta[META_MODIFIED] = std::to_string(utils::get_file_time_now());
  EXPECT_EQ(api_error::success, provider.create_directory("/moose2", meta));
  EXPECT_EQ(api_error::success, provider.create_file("/moose2.txt", meta));

  {
    api_meta_map m{
        {"test_one", "one"},
        {"test_two", "two"},
    };
    EXPECT_EQ(api_error::success, provider.set_item_meta("/moose2", m));

    api_meta_map m2{};
    EXPECT_EQ(api_error::success, provider.get_item_meta("/moose2", m2));
    EXPECT_EQ(meta.size() + m.size(), m2.size());

    EXPECT_STREQ("one", m2["test_one"].c_str());
    EXPECT_STREQ("two", m2["test_two"].c_str());
  }

  {
    api_meta_map m{
        {"test_one", "one1"},
        {"test_two", "two2"},
    };
    EXPECT_EQ(api_error::success, provider.set_item_meta("/moose2.txt", m));

    api_meta_map m2{};
    EXPECT_EQ(api_error::success, provider.get_item_meta("/moose2.txt", m2));
    EXPECT_EQ(meta.size() + m.size(), m2.size());

    EXPECT_STREQ("one1", m2["test_one"].c_str());
    EXPECT_STREQ("two2", m2["test_two"].c_str());
  }

  EXPECT_EQ(api_error::success, provider.remove_directory("/moose2"));
  EXPECT_EQ(api_error::success, provider.remove_file("/moose2.txt"));

  provider.stop();

  event_system::instance().stop();
}

TEST(sia_provider, can_get_and_set_individual_item_meta) {
  console_consumer cc{};
  event_system::instance().start();

  app_config cfg(provider_type::sia,
                 utils::path::combine(get_test_dir(), {"sia"}));
  curl_comm comm(cfg.get_host_config());
  sia_provider sia(cfg, comm);
  i_provider &provider = sia;

  EXPECT_TRUE(provider.start(
      [&](bool directory, api_file &file) -> api_error {
        return provider_meta_handler(provider, directory, file);
      },
      nullptr));

  api_meta_map meta{};
  meta[META_ACCESSED] = std::to_string(utils::get_file_time_now());
  meta[META_CHANGED] = std::to_string(utils::get_file_time_now());
  meta[META_CREATION] = std::to_string(utils::get_file_time_now());
  meta[META_MODIFIED] = std::to_string(utils::get_file_time_now());
  EXPECT_EQ(api_error::success, provider.create_directory("/moose2", meta));
  EXPECT_EQ(api_error::success, provider.create_file("/moose2.txt", meta));

  {
    EXPECT_EQ(api_error::success,
              provider.set_item_meta("/moose2", "test_meta", "cow2"));
    std::string value{};
    EXPECT_EQ(api_error::success,
              provider.get_item_meta("/moose2", "test_meta", value));
    EXPECT_STREQ("cow2", value.c_str());
  }

  {
    EXPECT_EQ(api_error::success,
              provider.set_item_meta("/moose2.txt", "test_meta", "cow"));
    std::string value{};
    EXPECT_EQ(api_error::success,
              provider.get_item_meta("/moose2.txt", "test_meta", value));
    EXPECT_STREQ("cow", value.c_str());
  }

  EXPECT_EQ(api_error::success, provider.remove_directory("/moose2"));
  EXPECT_EQ(api_error::success, provider.remove_file("/moose2.txt"));

  provider.stop();

  event_system::instance().stop();
}

TEST(sia_provider, can_read_file_bytes) {
  console_consumer cc{};
  event_system::instance().start();

  app_config cfg(provider_type::sia,
                 utils::path::combine(get_test_dir(), {"sia"}));
  curl_comm comm(cfg.get_host_config());
  sia_provider sia(cfg, comm);
  i_provider &provider = sia;

  EXPECT_TRUE(provider.start(
      [&](bool directory, api_file &file) -> api_error {
        return provider_meta_handler(provider, directory, file);
      },
      nullptr));

  data_buffer data{};
  stop_type stop_requested = false;
  EXPECT_EQ(api_error::success,
            provider.read_file_bytes("/renterd_linux_amd64.zip", 10U, 0U, data,
                                     stop_requested));
  EXPECT_EQ(10U, data.size());

  provider.stop();

  event_system::instance().stop();
}

TEST(sia_provider, can_upload_file) {
  console_consumer cc{};
  event_system::instance().start();

  app_config cfg(provider_type::sia,
                 utils::path::combine(get_test_dir(), {"sia"}));
  curl_comm comm(cfg.get_host_config());
  sia_provider sia(cfg, comm);
  i_provider &provider = sia;

  EXPECT_TRUE(provider.start(
      [&](bool directory, api_file &file) -> api_error {
        return provider_meta_handler(provider, directory, file);
      },
      nullptr));

  stop_type stop_requested = false;
  EXPECT_EQ(api_error::success,
            provider.upload_file("/sia_provider_test.cpp", __FILE__, "",
                                 stop_requested));
  api_meta_map meta = {{"test", "test"}};
  EXPECT_EQ(api_error::success,
            provider.set_item_meta("/sia_provider_test.cpp", meta));
  EXPECT_EQ(api_error::success, provider.remove_file("/sia_provider_test.cpp"));

  provider.stop();

  event_system::instance().stop();
} */
} // namespace repertory
