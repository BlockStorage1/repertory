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

#include "comm/packet/packet.hpp"
#include "comm/packet/packet_client.hpp"
#include "comm/packet/packet_server.hpp"
#include "types/remote.hpp"
#include "utils/common.hpp"
#include "utils/utils.hpp"
#include "version.hpp"

using namespace repertory;
using namespace repertory::comm;

namespace {
class test_packet_server final {
public:
  test_packet_server(std::uint16_t port, std::string token,
                     std::uint8_t pool_size = 2)
      : server_(std::make_unique<packet_server>(
            port, std::move(token), pool_size, [](std::string /*client_id*/) {},
            [](std::uint32_t /*service_flags_in*/, std::string /*client_id*/,
               std::uint64_t /*thread_id*/, std::string method,
               packet * /*request*/, packet & /*response*/,
               packet_server::message_complete_callback done) {
              if (method == "ping") {
                done(packet::error_type{0});
              } else {
                done(packet::error_type{-1});
              }
            })) {}

private:
  std::unique_ptr<packet_server> server_;
};

inline auto make_cfg(std::uint16_t port, std::string_view token)
    -> remote::remote_config {
  remote::remote_config cfg{};
  cfg.host_name_or_ip = "127.0.0.1";
  cfg.api_port = port;
  cfg.max_connections = 2U;
  cfg.conn_timeout_ms = 1500U;
  cfg.recv_timeout_ms = 1500U;
  cfg.send_timeout_ms = 1500U;
  cfg.encryption_token = token;
  return cfg;
}

TEST(packet_client_test, can_check_version) {
  std::string token = "cow_moose_doge_chicken";

  std::uint16_t port{};
  ASSERT_TRUE(utils::get_next_available_port(50000U, port));

  test_packet_server server(port, token);

  packet_client client(make_cfg(port, token));

  std::uint32_t min_version{};
  auto api = client.check_version(
      utils::get_version_number(project_get_version()), min_version);

  EXPECT_EQ(api, api_error::success);
  EXPECT_NE(min_version, 0U);
}

TEST(packet_client_test, can_detect_incompatible_version) {
  std::string token = "cow_moose_doge_chicken";

  std::uint16_t port{};
  ASSERT_TRUE(utils::get_next_available_port(50000U, port));

  test_packet_server server(port, token);

  packet_client client(make_cfg(port, token));

  std::uint32_t min_version{};
  auto api =
      client.check_version(utils::get_version_number("1.0.0-rc"), min_version);

  EXPECT_EQ(api, api_error::incompatible_version);
  EXPECT_NE(min_version, 0U);
}

TEST(packet_client_test, can_send_request_and_receive_response) {
  std::string token = "cow_moose_doge_chicken";

  std::uint16_t port{};
  ASSERT_TRUE(utils::get_next_available_port(50000U, port));

  test_packet_server server(port, token);

  packet_client client(make_cfg(port, token));

  std::uint32_t service_flags{};
  packet request;
  packet response;
  auto ret = client.send("ping", request, response, service_flags);

  EXPECT_EQ(ret, 0);
}

TEST(packet_client_test, pooled_connection_reused_on_second_send) {
  std::string token{"test_token"};
  std::uint16_t port{};
  ASSERT_TRUE(utils::get_next_available_port(50000U, port));

  std::atomic<std::uint32_t> close_count{0U};

  packet_server server{
      port, token, 2U,
      [&close_count](std::string /*client_id*/) { ++close_count; },
      [](std::uint32_t /*service_flags_in*/, std::string /*client_id*/,
         std::uint64_t /*thread_id*/, std::string method, packet * /*request*/,
         packet & /*response*/, packet_server::message_complete_callback done) {
        if (method == "ping") {
          done(packet::error_type{0});
        } else {
          done(packet::error_type{-1});
        }
      }};

  packet_client client(::make_cfg(port, token));

  std::uint32_t service_flags{};
  packet req_one;
  packet resp_one;
  auto ret_one = client.send("ping", req_one, resp_one, service_flags);
  EXPECT_EQ(ret_one, 0);

  packet req_two;
  packet resp_two;
  auto ret_two = client.send("ping", req_two, resp_two, service_flags);
  EXPECT_EQ(ret_two, 0);

  EXPECT_EQ(close_count, 0U);
}
} // namespace
