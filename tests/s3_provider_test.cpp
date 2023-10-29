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
#if defined(REPERTORY_ENABLE_S3) && defined(REPERTORY_ENABLE_S3_TESTING)

#include "test_common.hpp"

#include "app_config.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/event_system.hpp"
#include "file_manager/file_manager.hpp"
#include "fixtures/s3_provider_file_list_fixture.hpp"
#include "mocks/mock_s3_comm.hpp"
#include "platform/platform.hpp"
#include "providers/s3/s3_provider.hpp"
#include "types/s3.hpp"
#include "types/startup_exception.hpp"
#include "utils/path_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
// TEST(s3_provider, can_construct_s3_provider) {
//   {
//     app_config cfg(provider_type::s3, "./s3_provider_test");
//     EXPECT_FALSE(cfg.set_value_by_name("S3Config.Bucket", "bucket").empty());
//     EXPECT_FALSE(
//         cfg.set_value_by_name("S3Config.URL", "https://url.com").empty());
//     mock_s3_comm comm(cfg.get_s3_config());
//     s3_provider s3(cfg, comm);
//     EXPECT_EQ(s3.get_total_item_count(), 0u);
//   }
//
//   EXPECT_TRUE(utils::file::delete_directory_recursively("./s3_provider_test"));
// }
//
TEST(s3_provider, start_fails_with_empty_bucket) {
  {
    app_config cfg(provider_type::s3, "./s3_provider_test");
    EXPECT_TRUE(cfg.set_value_by_name("S3Config.Bucket", "").empty());
    EXPECT_FALSE(
        cfg.set_value_by_name("S3Config.URL", "https://url.com").empty());
    mock_s3_comm comm(cfg.get_s3_config());
    s3_provider s3(cfg, comm);
    file_manager fm(cfg, s3);

    try {
      auto res = s3.start(
          [](bool, api_file & /* file */) -> api_error {
            return api_error::success;
          },
          &fm);
      std::cerr << "unexpected return-should throw|err|" << res << std::endl;
    } catch (const startup_exception &e) {
      EXPECT_STREQ("s3 bucket name cannot be empty", e.what());
      return;
    }

    throw std::runtime_error("exception not thrown");
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./s3_provider_test"));
}

TEST(s3_provider, start_fails_when_provider_is_offline) {
  {
    app_config cfg(provider_type::s3, "./s3_provider_test");
    EXPECT_FALSE(cfg.set_value_by_name("S3Config.Bucket", "bucket").empty());
    EXPECT_FALSE(
        cfg.set_value_by_name("S3Config.URL", "https://url.com").empty());
    cfg.set_online_check_retry_secs(2u);
    mock_s3_comm comm(cfg.get_s3_config());
    s3_provider s3(cfg, comm);
    file_manager fm(cfg, s3);

    EXPECT_CALL(comm, is_online()).WillRepeatedly(Return(false));
    EXPECT_FALSE(s3.start([](bool, api_file & /* file */)
                              -> api_error { return api_error::success; },
                          &fm));
  }

  EXPECT_TRUE(utils::file::delete_directory_recursively("./s3_provider_test"));
}

// TEST(s3_provider, get_empty_file_list) {
//   {
//     app_config cfg(provider_type::s3, "./s3_provider_test");
//     EXPECT_FALSE(cfg.set_value_by_name("S3Config.Bucket", "bucket").empty());
//     EXPECT_FALSE(
//         cfg.set_value_by_name("S3Config.URL", "https://url.com").empty());
//     cfg.set_online_check_retry_secs(2u);
//     mock_s3_comm comm(cfg.get_s3_config());
//     s3_provider s3(cfg, comm);
//     file_manager fm(cfg, s3);
//
//     api_file_list list{};
//     EXPECT_CALL(comm, get_file_list)
//         .WillOnce([](const get_api_file_token_callback &,
//                      const get_name_callback &,
//                      api_file_list &) { return api_error::success; });
//
//     EXPECT_EQ(api_error::success, s3.get_file_list(list));
//   }
//
//   EXPECT_TRUE(utils::file::delete_directory_recursively("./s3_provider_test"));
// }
//
// TEST_F(s3_provider_file_list_test, can_add_new_files_and_directories) {
//   provider->set_callback([this](bool directory, api_file &file) -> api_error
//   {
//     std::cout << "added|api_path|" << file.api_path << "|api_parent|"
//               << file.api_parent << "|source|" << file.source_path
//               << "|directory|" << directory << "|create_date|"
//               << file.creation_date << "|access_date|" << file.accessed_date
//               << "|modified_date|" << file.modified_date << "|changed_date|"
//               << file.changed_date << std::endl;
//     return provider_meta_handler(*provider, directory, file);
//   });
//
//   api_file_list l{};
//   auto res = provider->get_file_list(l);
//   EXPECT_EQ(api_error::success, res);
//   EXPECT_EQ(list.size(), l.size());
//   EXPECT_EQ(std::size_t(22u), provider->get_total_item_count());
//
//   bool exists{};
//   EXPECT_EQ(api_error::success, provider->is_directory("/", exists));
//   EXPECT_TRUE(exists);
//
//   EXPECT_EQ(api_error::success, provider->is_directory("/dir", exists));
//   EXPECT_TRUE(exists);
//
//   const auto check_file = [this, &l](std::size_t idx,
//                                      bool check_sub_directory) {
//     const auto &file = l.at(idx);
//     const auto base_idx = idx - (check_sub_directory ? l.size() / 2 : 0u);
//     EXPECT_EQ(this->times[base_idx], file.accessed_date);
//     if (check_sub_directory) {
//       EXPECT_EQ(utils::path::create_api_path("/dir/file_" +
//                                              std::to_string(base_idx) +
//                                              ".txt"),
//                 file.api_path);
//       EXPECT_EQ(utils::path::get_parent_api_path(utils::path::create_api_path(
//                     "/dir/file_" + std::to_string(base_idx) + ".txt")),
//                 file.api_parent);
//     } else {
//       EXPECT_EQ(utils::path::create_api_path("/file_" +
//                                              std::to_string(base_idx) +
//                                              ".txt"),
//                 file.api_path);
//       EXPECT_EQ(utils::path::get_parent_api_path(utils::path::create_api_path(
//                     "/file_" + std::to_string(base_idx) + ".txt")),
//                 file.api_parent);
//     }
//     EXPECT_EQ(this->times[base_idx] + 1u, file.changed_date);
//     EXPECT_EQ(this->times[base_idx] + 2u, file.creation_date);
//     EXPECT_TRUE(file.encryption_token.empty());
//     EXPECT_EQ(100u + base_idx, file.file_size);
//     EXPECT_EQ(this->times[base_idx] + 3u, file.modified_date);
//   };
//
//   for (std::size_t idx = 0u; idx < l.size() / 2u; idx++) {
//     check_file(idx, false);
//   }
//
//   for (std::size_t idx = l.size() / 2u; idx < l.size(); idx++) {
//     check_file(idx, true);
//   }
// }
} // namespace repertory

#endif // REPERTORY_ENABLE_S3_TESTING
