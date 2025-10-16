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

#include "comm/packet/common.hpp"

namespace repertory {
TEST(packet_comm_common_test, idle_socket_considered_alive) {
  using boost::asio::ip::tcp;

  boost::asio::io_context io_ctx;
  std::uint16_t port{};
  ASSERT_TRUE(utils::get_next_available_port(50000U, port));

  tcp::acceptor acceptor{io_ctx, tcp::endpoint{tcp::v4(), port}};
  std::shared_ptr<tcp::socket> server_sock =
      std::make_shared<tcp::socket>(io_ctx);
  std::atomic<bool> accepted{false};

  acceptor.async_accept(*server_sock,
                        [&](const boost::system::error_code &err) {
                          if (not err) {
                            accepted = true;
                          }
                        });

  std::thread io_thread{[&]() { io_ctx.run(); }};

  boost::asio::io_context client_ctx;
  tcp::socket client_sock{client_ctx};
  client_sock.connect(
      tcp::endpoint{boost::asio::ip::address_v4::loopback(), port});

  for (std::uint8_t attempt = 0U; attempt < 50U and not accepted; ++attempt) {
    std::this_thread::sleep_for(std::chrono::milliseconds{2});
  }
  ASSERT_TRUE(accepted);

  EXPECT_TRUE(comm::is_socket_still_alive(client_sock));

  boost::system::error_code err;
  [[maybe_unused]] auto res =
      client_sock.shutdown(tcp::socket::shutdown_both, err);
  [[maybe_unused]] auto res2 = client_sock.close(err);

  if (server_sock and server_sock->is_open()) {
    [[maybe_unused]] auto res3 =
        server_sock->shutdown(tcp::socket::shutdown_both, err);
    [[maybe_unused]] auto res4 = server_sock->close(err);
  }

  [[maybe_unused]] auto res4 = acceptor.close(err);
  io_ctx.stop();
  if (io_thread.joinable()) {
    io_thread.join();
  }
}

TEST(packet_comm_common_test, closed_socket_is_not_reused) {
  using boost::asio::ip::tcp;

  boost::asio::io_context io_ctx;
  std::uint16_t port{};
  ASSERT_TRUE(utils::get_next_available_port(50000U, port));

  tcp::acceptor acceptor{io_ctx, tcp::endpoint{tcp::v4(), port}};
  std::shared_ptr<tcp::socket> server_sock =
      std::make_shared<tcp::socket>(io_ctx);
  std::atomic<bool> accepted{false};

  acceptor.async_accept(*server_sock,
                        [&](const boost::system::error_code &err) {
                          if (not err) {
                            accepted = true;
                          }
                        });

  std::thread io_thread{[&]() { io_ctx.run(); }};

  boost::asio::io_context client_ctx;
  tcp::socket client_sock{client_ctx};
  client_sock.connect(
      tcp::endpoint{boost::asio::ip::address_v4::loopback(), port});

  for (std::uint8_t attempt = 0U; attempt < 50U and not accepted; ++attempt) {
    std::this_thread::sleep_for(std::chrono::milliseconds{2});
  }
  ASSERT_TRUE(accepted);

  boost::system::error_code err;
  [[maybe_unused]] auto res =
      client_sock.shutdown(tcp::socket::shutdown_both, err);
  [[maybe_unused]] auto res2 = client_sock.close(err);

  EXPECT_FALSE(comm::is_socket_still_alive(client_sock));

  if (server_sock and server_sock->is_open()) {
    [[maybe_unused]] auto res3 =
        server_sock->shutdown(tcp::socket::shutdown_both, err);
    [[maybe_unused]] auto res4 = server_sock->close(err);
  }

  [[maybe_unused]] auto res5 = acceptor.close(err);
  io_ctx.stop();
  if (io_thread.joinable()) {
    io_thread.join();
  }
}
} // namespace repertory
