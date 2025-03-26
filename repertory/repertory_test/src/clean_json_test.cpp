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
#include "types/repertory.hpp"

namespace repertory {
TEST(clean_json_test, can_clean_values) {
  auto result = clean_json_value(JSON_API_PASSWORD, "moose");
  EXPECT_TRUE(result.empty());

  result = clean_json_value(
      fmt::format("{}.{}", JSON_ENCRYPT_CONFIG, JSON_ENCRYPTION_TOKEN),
      "moose");
  EXPECT_TRUE(result.empty());

  result = clean_json_value(
      fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_API_PASSWORD), "moose");
  EXPECT_TRUE(result.empty());

  result = clean_json_value(
      fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_ENCRYPTION_TOKEN), "moose");
  EXPECT_TRUE(result.empty());

  result = clean_json_value(
      fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_ENCRYPTION_TOKEN), "moose");
  EXPECT_TRUE(result.empty());

  result = clean_json_value(
      fmt::format("{}.{}", JSON_S3_CONFIG, JSON_ENCRYPTION_TOKEN), "moose");
  EXPECT_TRUE(result.empty());

  result = clean_json_value(
      fmt::format("{}.{}", JSON_S3_CONFIG, JSON_SECRET_KEY), "moose");
  EXPECT_TRUE(result.empty());
}

TEST(clean_json_test, can_clean_encrypt_config) {
  auto dir =
      utils::path::combine(test::get_test_output_dir(), {
                                                            "clean_json_test",
                                                            "encrypt",
                                                        });
  app_config cfg(provider_type::encrypt, dir);
  cfg.set_value_by_name(JSON_API_PASSWORD, "moose");
  cfg.set_value_by_name(
      fmt::format("{}.{}", JSON_ENCRYPT_CONFIG, JSON_ENCRYPTION_TOKEN),
      "moose");
  cfg.set_value_by_name(
      fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_ENCRYPTION_TOKEN), "moose");

  auto data = cfg.get_json();
  EXPECT_FALSE(data.at(JSON_API_PASSWORD).get<std::string>().empty());
  EXPECT_FALSE(data.at(JSON_ENCRYPT_CONFIG)
                   .at(JSON_ENCRYPTION_TOKEN)
                   .get<std::string>()
                   .empty());
  EXPECT_FALSE(data.at(JSON_REMOTE_MOUNT)
                   .at(JSON_ENCRYPTION_TOKEN)
                   .get<std::string>()
                   .empty());

  clean_json_config(cfg.get_provider_type(), data);
  EXPECT_TRUE(data.at(JSON_API_PASSWORD).get<std::string>().empty());
  EXPECT_TRUE(data.at(JSON_ENCRYPT_CONFIG)
                  .at(JSON_ENCRYPTION_TOKEN)
                  .get<std::string>()
                  .empty());
  EXPECT_TRUE(data.at(JSON_REMOTE_MOUNT)
                  .at(JSON_ENCRYPTION_TOKEN)
                  .get<std::string>()
                  .empty());
}

TEST(clean_json_test, can_clean_remote_config) {
  auto dir =
      utils::path::combine(test::get_test_output_dir(), {
                                                            "clean_json_test",
                                                            "remote",
                                                        });
  app_config cfg(provider_type::remote, dir);
  cfg.set_value_by_name(JSON_API_PASSWORD, "moose");
  cfg.set_value_by_name(
      fmt::format("{}.{}", JSON_REMOTE_CONFIG, JSON_ENCRYPTION_TOKEN), "moose");

  auto data = cfg.get_json();
  EXPECT_FALSE(data.at(JSON_API_PASSWORD).get<std::string>().empty());
  EXPECT_FALSE(data.at(JSON_REMOTE_CONFIG)
                   .at(JSON_ENCRYPTION_TOKEN)
                   .get<std::string>()
                   .empty());

  clean_json_config(cfg.get_provider_type(), data);
  EXPECT_TRUE(data.at(JSON_API_PASSWORD).get<std::string>().empty());
  EXPECT_TRUE(data.at(JSON_REMOTE_CONFIG)
                  .at(JSON_ENCRYPTION_TOKEN)
                  .get<std::string>()
                  .empty());
}

TEST(clean_json_test, can_clean_s3_config) {
  auto dir =
      utils::path::combine(test::get_test_output_dir(), {
                                                            "clean_json_test",
                                                            "s3",
                                                        });
  app_config cfg(provider_type::s3, dir);
  cfg.set_value_by_name(JSON_API_PASSWORD, "moose");
  cfg.set_value_by_name(
      fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_ENCRYPTION_TOKEN), "moose");
  cfg.set_value_by_name(
      fmt::format("{}.{}", JSON_S3_CONFIG, JSON_ENCRYPTION_TOKEN), "moose");
  cfg.set_value_by_name(fmt::format("{}.{}", JSON_S3_CONFIG, JSON_SECRET_KEY),
                        "moose");

  auto data = cfg.get_json();
  EXPECT_FALSE(data.at(JSON_API_PASSWORD).get<std::string>().empty());
  EXPECT_FALSE(data.at(JSON_REMOTE_MOUNT)
                   .at(JSON_ENCRYPTION_TOKEN)
                   .get<std::string>()
                   .empty());
  EXPECT_FALSE(data.at(JSON_S3_CONFIG)
                   .at(JSON_ENCRYPTION_TOKEN)
                   .get<std::string>()
                   .empty());
  EXPECT_FALSE(
      data.at(JSON_S3_CONFIG).at(JSON_SECRET_KEY).get<std::string>().empty());

  clean_json_config(cfg.get_provider_type(), data);
  EXPECT_TRUE(data.at(JSON_API_PASSWORD).get<std::string>().empty());
  EXPECT_TRUE(data.at(JSON_REMOTE_MOUNT)
                  .at(JSON_ENCRYPTION_TOKEN)
                  .get<std::string>()
                  .empty());
  EXPECT_TRUE(data.at(JSON_S3_CONFIG)
                  .at(JSON_ENCRYPTION_TOKEN)
                  .get<std::string>()
                  .empty());
  EXPECT_TRUE(
      data.at(JSON_S3_CONFIG).at(JSON_SECRET_KEY).get<std::string>().empty());
}

TEST(clean_json_test, can_clean_sia_config) {
  auto dir =
      utils::path::combine(test::get_test_output_dir(), {
                                                            "clean_json_test",
                                                            "sia",
                                                        });
  app_config cfg(provider_type::sia, dir);
  cfg.set_value_by_name(JSON_API_PASSWORD, "moose");
  cfg.set_value_by_name(
      fmt::format("{}.{}", JSON_HOST_CONFIG, JSON_API_PASSWORD), "moose");
  cfg.set_value_by_name(
      fmt::format("{}.{}", JSON_REMOTE_MOUNT, JSON_ENCRYPTION_TOKEN), "moose");

  auto data = cfg.get_json();
  EXPECT_FALSE(data.at(JSON_API_PASSWORD).get<std::string>().empty());
  EXPECT_FALSE(data.at(JSON_HOST_CONFIG)
                   .at(JSON_API_PASSWORD)
                   .get<std::string>()
                   .empty());
  EXPECT_FALSE(data.at(JSON_REMOTE_MOUNT)
                   .at(JSON_ENCRYPTION_TOKEN)
                   .get<std::string>()
                   .empty());

  clean_json_config(cfg.get_provider_type(), data);
  EXPECT_TRUE(data.at(JSON_API_PASSWORD).get<std::string>().empty());
  EXPECT_TRUE(data.at(JSON_HOST_CONFIG)
                  .at(JSON_API_PASSWORD)
                  .get<std::string>()
                  .empty());
  EXPECT_TRUE(data.at(JSON_REMOTE_MOUNT)
                  .at(JSON_ENCRYPTION_TOKEN)
                  .get<std::string>()
                  .empty());
}
} // namespace repertory
