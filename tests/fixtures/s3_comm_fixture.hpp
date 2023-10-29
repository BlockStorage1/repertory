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
#ifndef REPERTORY_S3_COMM_FIXTURE_H
#define REPERTORY_S3_COMM_FIXTURE_H

#if defined(REPERTORY_ENABLE_S3) && defined(REPERTORY_ENABLE_S3_TESTING)

#include "test_common.hpp"

#include "app_config.hpp"
#include "comm/s3/s3_comm.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
class s3_comm_test : public ::testing::Test {
private:
  console_consumer c_;

protected:
  std::unique_ptr<app_config> config_;
  std::unique_ptr<s3_comm> s3_comm_;

public:
  void SetUp() override {
    const auto path = utils::path::absolute("./test/");
    ASSERT_TRUE(utils::file::delete_directory_recursively(path));
    {
      app_config config(provider_type::s3,
                        utils::path::combine(get_test_dir(), {"filebase"}));

      config_ = std::make_unique<app_config>(provider_type::s3, "./test");
      config_->set_event_level(event_level::verbose);
      EXPECT_FALSE(config_
                       ->set_value_by_name("S3Config.AccessKey",
                                           config.get_s3_config().access_key)
                       .empty());
      EXPECT_FALSE(config_
                       ->set_value_by_name("S3Config.SecretKey",
                                           config.get_s3_config().secret_key)
                       .empty());
      EXPECT_FALSE(config_
                       ->set_value_by_name("S3Config.Region",
                                           config.get_s3_config().region)
                       .empty());
      EXPECT_FALSE(
          config_->set_value_by_name("S3Config.URL", config.get_s3_config().url)
              .empty());
      EXPECT_FALSE(
          config_->set_value_by_name("S3Config.Bucket", "repertory").empty());
    }

    s3_comm_ = std::make_unique<s3_comm>(*config_);

    event_system::instance().start();
  }

  void TearDown() override {
    event_system::instance().stop();
    s3_comm_.reset();
    config_.reset();

    const auto path = utils::path::absolute("./test/");
    EXPECT_TRUE(utils::file::delete_directory_recursively(path));
  }
};
} // namespace repertory

#endif // REPERTORY_ENABLE_S3_TESTING
#endif // REPERTORY_S3_COMM_FIXTURE_H
