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
#ifndef REPERTORY_SKYNET_PROVIDER_FIXTURE_H
#define REPERTORY_SKYNET_PROVIDER_FIXTURE_H
#if defined(REPERTORY_ENABLE_SKYNET)

#include "comm/curl/curl_comm.hpp"
#include "app_config.hpp"
#include "mocks/mock_open_file_table.hpp"
#include "providers/skynet/skynet_provider.hpp"
#include "test_common.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
class skynet_provider_test : public ::testing::Test {
private:
  console_consumer c_;
  const std::string config_location_ = utils::path::absolute("./skynetprovider");

protected:
  std::unique_ptr<app_config> config_;
  std::unique_ptr<curl_comm> curl_comm_;
  std::unique_ptr<skynet_provider> provider_;
  mock_open_file_table oft_;

public:
  void SetUp() override {
    utils::file::delete_directory_recursively(config_location_);

    config_ = std::make_unique<app_config>(provider_type::skynet, config_location_);
    config_->set_event_level(event_level::verbose);

    event_system::instance().start();
    curl_comm_ = std::make_unique<curl_comm>(*config_.get());
    provider_ = std::make_unique<skynet_provider>(*config_.get(), *curl_comm_.get());

    const auto res = provider_->start(
        [&](const std::string &api_path, const std::string &api_parent, const std::string &source,
            const bool &directory, const std::uint64_t &created_date,
            const std::uint64_t &accessed_date, const std::uint64_t &modified_date,
            const std::uint64_t &changed_date) {
          event_system::instance().raise<filesystem_item_added>(api_path, api_parent, directory);
#ifdef _WIN32
          provider_->set_item_meta(
              api_path, {{META_ATTRIBUTES, std::to_string(directory ? FILE_ATTRIBUTE_DIRECTORY
                                                                    : FILE_ATTRIBUTE_NORMAL |
                                                                          FILE_ATTRIBUTE_ARCHIVE)},
                         {META_CREATION, std::to_string(created_date)},
                         {META_WRITTEN, std::to_string(modified_date)},
                         {META_MODIFIED, std::to_string(modified_date)},
                         { META_ACCESSED,
                           std::to_string(accessed_date) }});
#else
          provider_->set_item_meta(
              api_path,
              {{META_CREATION, std::to_string(created_date)},
               {META_MODIFIED, std::to_string(modified_date)},
               {META_WRITTEN, std::to_string(modified_date)},
               {META_ACCESSED, std::to_string(accessed_date)},
               {META_OSXFLAGS, "0"},
               {META_BACKUP, "0"},
               {META_CHANGED, std::to_string(changed_date)},
               {META_MODE, utils::string::from_uint32(directory ? S_IRUSR | S_IWUSR | S_IXUSR
                                                                : S_IRUSR | S_IWUSR)},
               {META_UID, utils::string::from_uint32(getuid())},
               {META_GID, utils::string::from_uint32(getgid())}});
#endif
          if (not directory && not source.empty()) {
            provider_->set_source_path(api_path, source);
          }
        },
        &oft_);
    EXPECT_FALSE(res);
  }

  void TearDown() override {
    provider_->stop();
    event_system::instance().stop();
    curl_comm_.reset();
    config_.reset();
    provider_.reset();

    utils::file::delete_directory_recursively(config_location_);
  }
};
} // namespace repertory

#endif // REPERTORY_ENABLE_SKYNET
#endif // REPERTORY_SKYNET_PROVIDER_FIXTURE_H
