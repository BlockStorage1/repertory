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
#ifndef TESTS_FIXTURES_FUSE_FIXTURE_HPP_
#define TESTS_FIXTURES_FUSE_FIXTURE_HPP_
#if !_WIN32

#include "app_config.hpp"
#include "drives/fuse/fuse_drive.hpp"
#include "mocks/mock_comm.hpp"
#include "mocks/mock_s3_comm.hpp"
#include "platform/platform.hpp"
#include "providers/s3/s3_provider.hpp"
#include "providers/skynet/skynet_provider.hpp"
#include "providers/sia/sia_provider.hpp"
#include "test_common.hpp"

namespace repertory {
class fuse_test : public ::testing::Test {
public:
  mock_comm mock_sia_comm_;
#ifdef REPERTORY_ENABLE_SKYNET
  mock_comm mock_skynet_comm_;
#endif

#ifdef REPERTORY_ENABLE_S3_TESTING
  /* mock_s3_comm mock_s3_comm_; */
#ifdef REPERTORY_ENABLE_SKYNET
  std::array<std::tuple<std::shared_ptr<app_config>, std::shared_ptr<i_provider>,
                        std::shared_ptr<fuse_drive>>,
             2>
      provider_tests_;
#else
  std::array<std::tuple<std::shared_ptr<app_config>, std::shared_ptr<i_provider>,
                        std::shared_ptr<fuse_drive>>,
             1>
      provider_tests_;
#endif
#else
#ifdef REPERTORY_ENABLE_SKYNET
  std::array<std::tuple<std::shared_ptr<app_config>, std::shared_ptr<i_provider>,
                        std::shared_ptr<fuse_drive>>,
             2>
      provider_tests_;
#else
  std::array<std::tuple<std::shared_ptr<app_config>, std::shared_ptr<i_provider>,
                        std::shared_ptr<fuse_drive>>,
             1>
      provider_tests_;
#endif
#endif
  lock_data lock_data_;

protected:
  void SetUp() override {
    std::size_t provider_index = 0u;
    utils::file::delete_directory_recursively("./fuse_test");
    auto config = std::make_shared<app_config>(provider_type::sia, "./fuse_test");
    config->set_enable_drive_events(true);
    config->set_event_level(event_level::verbose);
    config->set_api_port(11115);
    auto sp = std::make_shared<sia_provider>(*config, mock_sia_comm_);
    auto drive = std::make_shared<fuse_drive>(*config, lock_data_, *sp);
    provider_tests_[provider_index++] = {config, sp, drive};

#ifdef REPERTORY_ENABLE_SKYNET
    utils::file::delete_directory_recursively("./fuse_test2");
    config = std::make_shared<app_config>(provider_type::skynet, "./fuse_test2");
    config->set_enable_drive_events(true);
    config->set_event_level(event_level::verbose);
    config->set_api_port(11116);
    auto skp = std::make_shared<skynet_provider>(*config, mock_skynet_comm_);
    drive = std::make_shared<fuse_drive>(*config, lock_data_, *skp);
    provider_tests_[provider_index++] = {config, skp, drive};
#endif

#ifdef REPERTORY_ENABLE_S3_TESTING
    // utils::file::delete_directory_recursively("./fuse_test3");
    // config = std::make_shared<app_config>(provider_type::s3, "./fuse_test3");
    // config->SetEnableDriveEvents(true);
    // config->set_event_level(event_level::Verbose);
    // config->SetAPIPort(11117);
    //{
    //  app_config config(provider_type::s3, "../../filebase");
    //  config->set_event_level(event_level::Verbose);
    //  config->set_value_by_name("S3Config.AccessKey", config.get_s3_config().AccessKey);
    //  config->set_value_by_name("S3Config.SecretKey", config.get_s3_config().SecretKey);
    //  config->set_value_by_name("S3Config.Region", config.get_s3_config().Region);
    //  config->set_value_by_name("S3Config.URL", config.get_s3_config().URL);
    //  config->set_value_by_name("S3Config.Bucket", "repertory");
    //}
    // mock_s3_comm_.SetS3Config(config->get_s3_config());

    // auto s3p = std::make_shared<s3_provider>(*config, mock_s3_comm_);
    // drive = std::make_shared<fuse_drive>(*config, lock_data_, *s3p);
    // provider_tests_[provider_index++] = {config, s3p, drive};
#endif
    event_system::instance().start();
  }

  void TearDown() override {
    for (auto &i : provider_tests_) {
      std::get<2>(i).reset();
    }
    event_system::instance().stop();
    for (auto &i : provider_tests_) {
      std::get<1>(i).reset();
      std::get<0>(i).reset();
    }
    utils::file::delete_directory_recursively("./fuse_test");
    utils::file::delete_directory_recursively("./fuse_test2");
    utils::file::delete_directory_recursively("./fuse_test3");
  }
};
} // namespace repertory

#endif
#endif // TESTS_FIXTURES_FUSE_FIXTURE_HPP_
