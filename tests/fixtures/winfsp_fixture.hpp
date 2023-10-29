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
#ifndef REPERTORY_WINFSP_FIXTURE_H
#define REPERTORY_WINFSP_FIXTURE_H
#if _WIN32

#include "test_common.hpp"

#include "app_config.hpp"
#include "comm/s3/s3_comm.hpp"
#include "comm/curl/curl_comm.hpp"
#include "drives/winfsp/winfsp_drive.hpp"
#include "platform/platform.hpp"
#include "providers/s3/s3_provider.hpp"
#include "providers/sia/sia_provider.hpp"

extern std::size_t PROVIDER_INDEX;

namespace repertory {
class winfsp_test : public ::testing::Test {
public:
  lock_data lock_data_;
  std::unique_ptr<app_config> config;
  std::unique_ptr<curl_comm> comm;
  std::unique_ptr<i_provider> provider;
  std::unique_ptr<winfsp_drive> drive;
#ifdef REPERTORY_ENABLE_S3
  std::unique_ptr<s3_comm> s3_comm_;
#endif

protected:
  void SetUp() override {
    if (PROVIDER_INDEX != 0) {
      if (PROVIDER_INDEX == 1) {
#ifdef REPERTORY_ENABLE_S3
        EXPECT_TRUE(utils::file::delete_directory_recursively(
            "./winfsp_test" + std::to_string(PROVIDER_INDEX)));

        app_config src_cfg(provider_type::s3,
                           utils::path::combine(get_test_dir(), {"filebase"}));
        config = std::make_unique<app_config>(
            provider_type::s3,
            "./winfsp_test" + std::to_string(PROVIDER_INDEX));
        EXPECT_FALSE(config
                         ->set_value_by_name("S3Config.AccessKey",
                                             src_cfg.get_s3_config().access_key)
                         .empty());
        EXPECT_FALSE(config
                         ->set_value_by_name("S3Config.SecretKey",
                                             src_cfg.get_s3_config().secret_key)
                         .empty());
        EXPECT_FALSE(config
                         ->set_value_by_name("S3Config.Region",
                                             src_cfg.get_s3_config().region)
                         .empty());
        EXPECT_FALSE(
            config
                ->set_value_by_name("S3Config.URL", src_cfg.get_s3_config().url)
                .empty());
        EXPECT_FALSE(
            config->set_value_by_name("S3Config.Bucket", "repertory").empty());
        config->set_event_level(event_level::verbose);
        config->set_enable_drive_events(true);
        event_system::instance().start();

        s3_comm_ = std::make_unique<s3_comm>(*config);
        provider = std::make_unique<s3_provider>(*config, *s3_comm_);
        drive = std::make_unique<winfsp_drive>(*config, lock_data_, *provider);
#endif
        return;
      }

      if (PROVIDER_INDEX == 2) {
        EXPECT_TRUE(utils::file::delete_directory_recursively(
            "./winfsp_test" + std::to_string(PROVIDER_INDEX)));

        app_config src_cfg(provider_type::sia,
                           utils::path::combine(get_test_dir(), {"sia"}));
        config = std::make_unique<app_config>(
            provider_type::sia,
            "./winfsp_test" + std::to_string(PROVIDER_INDEX));
        [[maybe_unused]] auto val = config->set_value_by_name(
            "HostConfig.AgentString", src_cfg.get_host_config().agent_string);
        EXPECT_FALSE(
            config
                ->set_value_by_name("HostConfig.ApiPassword",
                                    src_cfg.get_host_config().api_password)
                .empty());
        EXPECT_FALSE(config
                         ->set_value_by_name(
                             "HostConfig.ApiPort",
                             std::to_string(src_cfg.get_host_config().api_port))
                         .empty());
        EXPECT_FALSE(
            config
                ->set_value_by_name("HostConfig.HostNameOrIp",
                                    src_cfg.get_host_config().host_name_or_ip)
                .empty());
        config->set_event_level(event_level::debug);
        config->set_enable_drive_events(true);
        event_system::instance().start();

        comm = std::make_unique<curl_comm>(config->get_host_config());
        provider = std::make_unique<sia_provider>(*config, *comm);
        drive = std::make_unique<winfsp_drive>(*config, lock_data_, *provider);

        return;
      }
    }
  }

  void TearDown() override {
    if (PROVIDER_INDEX != 0) {
      drive.reset();
      provider.reset();
#ifdef REPERTORY_ENABLE_S3
      s3_comm_.reset();
#endif
      comm.reset();
      config.reset();

      event_system::instance().stop();
      EXPECT_TRUE(utils::file::delete_directory_recursively(
          "./winfsp_test" + std::to_string(PROVIDER_INDEX)));
    }
  }
};
} // namespace repertory

#endif
#endif // REPERTORY_WINFSP_FIXTURE_H
