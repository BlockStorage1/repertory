/*
  Copyright <2018-2024> <scott.e.graves@protonmail.com>

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

#include "comm/curl/curl_comm.hpp"
#include "types/repertory.hpp"

namespace repertory {
TEST(curl_comm, can_create_s3_host_config) {
  s3_config config{};
  config.bucket = "repertory";
  config.url = "https://s3.test.com";
  config.region = "any";

  auto hc = curl_comm::create_host_config(config, false);
  EXPECT_STREQ("https", hc.protocol.c_str());
  EXPECT_STREQ("repertory.s3.test.com", hc.host_name_or_ip.c_str());
  EXPECT_TRUE(hc.path.empty());
}

TEST(curl_comm, can_create_s3_host_config_with_path_style) {
  s3_config config{};
  config.bucket = "repertory";
  config.url = "https://s3.test.com";
  config.region = "any";

  auto hc = curl_comm::create_host_config(config, true);
  EXPECT_STREQ("https", hc.protocol.c_str());
  EXPECT_STREQ("s3.test.com", hc.host_name_or_ip.c_str());
  EXPECT_STREQ("/repertory", hc.path.c_str());
}

TEST(curl_comm, can_create_s3_host_config_with_region) {
  s3_config config{};
  config.bucket = "repertory";
  config.url = "https://s3.test.com";
  config.region = "any";
  config.use_region_in_url = true;

  auto hc = curl_comm::create_host_config(config, false);
  EXPECT_STREQ("https", hc.protocol.c_str());
  EXPECT_STREQ("repertory.s3.any.test.com", hc.host_name_or_ip.c_str());
  EXPECT_TRUE(hc.path.empty());
}

TEST(curl_comm, can_create_s3_host_config_with_region_and_path_style) {
  s3_config config{};
  config.bucket = "repertory";
  config.url = "https://s3.test.com";
  config.region = "any";
  config.use_region_in_url = true;

  auto hc = curl_comm::create_host_config(config, true);
  EXPECT_STREQ("https", hc.protocol.c_str());
  EXPECT_STREQ("s3.any.test.com", hc.host_name_or_ip.c_str());
  EXPECT_STREQ("/repertory", hc.path.c_str());
}
} // namespace repertory
