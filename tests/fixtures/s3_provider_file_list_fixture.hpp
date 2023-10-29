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
#ifndef REPERTORY_S3_PROVIDER_FILE_LIST_FIXTURE_H
#define REPERTORY_S3_PROVIDER_FILE_LIST_FIXTURE_H

#if defined(REPERTORY_ENABLE_S3) && defined(REPERTORY_ENABLE_S3_TESTING)

#include "test_common.hpp"

#include "mocks/mock_s3_comm.hpp"
#include "providers/s3/s3_provider.hpp"
#include "types/s3.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
class s3_provider_file_list_test : public ::testing::Test {
public:
  console_consumer c;
  api_file_list list;
  std::unique_ptr<app_config> cfg;
  std::unique_ptr<mock_s3_comm> comm;
  std::unique_ptr<s3_provider> provider;
  std::array<std::uint64_t, 10> times = {
      utils::get_file_time_now(), utils::get_file_time_now(),
      utils::get_file_time_now(), utils::get_file_time_now(),
      utils::get_file_time_now(), utils::get_file_time_now(),
      utils::get_file_time_now(), utils::get_file_time_now(),
      utils::get_file_time_now(), utils::get_file_time_now(),
  };

public:
  void SetUp() override {
    ASSERT_TRUE(utils::file::delete_directory_recursively(
        utils::path::absolute("./data")));

    event_system::instance().start();

    cfg = std::make_unique<app_config>(provider_type::s3,
                                       utils::path::absolute("./data"));
    EXPECT_FALSE(cfg->set_value_by_name("S3Config.Bucket", "bucket").empty());
    EXPECT_FALSE(
        cfg->set_value_by_name("S3Config.URL", "https://url.com").empty());
    comm = std::make_unique<mock_s3_comm>(cfg->get_s3_config());
    provider = std::make_unique<s3_provider>(*cfg, *comm);

    const auto create_file = [this](std::size_t idx,
                                    bool create_sub_directory = false) {
      api_file file{};
      file.accessed_date = times[idx];
      if (create_sub_directory) {
        file.api_path = utils::path::create_api_path(
            "/dir/file_" + std::to_string(idx) + ".txt");
      } else {
        file.api_path = utils::path::create_api_path(
            "/file_" + std::to_string(idx) + ".txt");
      }
      file.api_parent = utils::path::get_parent_api_path(file.api_path);
      file.changed_date = times[idx] + 1u;
      file.creation_date = times[idx] + 2u;
      file.encryption_token = "";
      file.file_size = 100u + idx;
      file.modified_date = times[idx] + 3u;
      this->list.emplace_back(std::move(file));
    };

    for (std::size_t idx = 0u; idx < times.size(); idx++) {
      create_file(idx);
    }

    for (std::size_t idx = 0u; idx < times.size(); idx++) {
      create_file(idx, true);
    }

    EXPECT_CALL(*comm, get_file_list)
        .WillRepeatedly(
            [this](const get_api_file_token_callback &get_api_file_token,
                   const get_name_callback &get_name, api_file_list &l) {
              for (auto i : list) {
                auto object_name = i.api_path;
                object_name = get_name(
                    *(utils::string::split(i.api_path, '/', false).end() - 1u),
                    object_name);
                i.api_path = object_name;
                i.api_parent = utils::path::get_parent_api_path(i.api_path);
                i.encryption_token = get_api_file_token(i.api_path);
                l.emplace_back(i);
              }
              return api_error::success;
            });

    EXPECT_CALL(*comm, get_file)
        .WillRepeatedly(
            [this](const std::string &api_path, const get_key_callback &get_key,
                   const get_name_callback &get_name,
                   const get_token_callback &get_token, api_file &file) {
              auto f = std::find_if(list.begin(), list.end(),
                                    [&api_path](const auto &f) -> bool {
                                      return f.api_path == api_path;
                                    });
              if (f == list.end()) {
                return api_error::item_not_found;
              }

              file = *f;

              auto object_name = api_path;
              const auto key = get_key();
              object_name = get_name(key, object_name);

              file.api_path = utils::path::create_api_path(object_name);
              file.api_parent = utils::path::get_parent_api_path(file.api_path);
              file.encryption_token = get_token();
              return api_error::success;
            });
  }

  void TearDown() override {
    provider->stop();
    comm.reset();
    provider.reset();
    cfg.reset();
    event_system::instance().stop();
    EXPECT_TRUE(utils::file::delete_directory_recursively(
        utils::path::absolute("./data")));
  }
};
} // namespace repertory

#endif // REPERTORY_ENABLE_S3_TESTING
#endif // REPERTORY_S3_PROVIDER_FILE_LIST_FIXTURE_H
