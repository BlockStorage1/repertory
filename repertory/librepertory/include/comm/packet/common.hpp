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
#ifndef REPERTORY_INCLUDE_COMM_PACKET_COMMON_HPP_
#define REPERTORY_INCLUDE_COMM_PACKET_COMMON_HPP_

namespace repertory::comm {
inline static constexpr std::uint32_t max_packet_bytes{32U * 1024U * 1024U};
inline constexpr const std::uint8_t max_read_attempts{2U};
inline constexpr const std::uint16_t packet_nonce_size{256U};
inline constexpr const std::size_t read_write_size{131072U};
inline constexpr const std::uint16_t server_handshake_timeout_ms{3000U};

struct non_blocking_guard final {
  non_blocking_guard(const non_blocking_guard &) = delete;
  non_blocking_guard(non_blocking_guard &&) = delete;

  auto operator=(const non_blocking_guard &) -> non_blocking_guard & = delete;
  auto operator=(non_blocking_guard &&) -> non_blocking_guard & = delete;

  explicit non_blocking_guard(boost::asio::ip::tcp::socket &sock_);

  ~non_blocking_guard();

private:
  bool non_blocking;
  boost::asio::ip::tcp::socket &sock;
};

void apply_common_socket_properties(boost::asio::ip::tcp::socket &sock);

[[nodiscard]] auto is_socket_still_alive(boost::asio::ip::tcp::socket &sock)
    -> bool;
} // namespace repertory::comm

#endif // REPERTORY_INCLUDE_COMM_PACKET_COMMON_HPP_
