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
#include "comm/packet/common.hpp"

namespace repertory::comm {
non_blocking_guard::non_blocking_guard(boost::asio::ip::tcp::socket &sock_)
    : non_blocking(sock_.non_blocking()), sock(sock_) {
  boost::system::error_code err;
  [[maybe_unused]] auto ret = sock_.non_blocking(true, err);
}

non_blocking_guard::~non_blocking_guard() {
  if (not sock.is_open()) {
    return;
  }

  boost::system::error_code err;
  [[maybe_unused]] auto res = sock.non_blocking(non_blocking, err);
}

auto is_socket_still_alive(boost::asio::ip::tcp::socket &sock) -> bool {
  if (not sock.is_open()) {
    return false;
  }

  non_blocking_guard guard{sock};

  boost::system::error_code err{};
  std::array<std::uint8_t, 1> tmp{};
  [[maybe_unused]] auto available = sock.receive(
      boost::asio::buffer(tmp), boost::asio::socket_base::message_peek, err);

  if (err == boost::asio::error::would_block ||
      err == boost::asio::error::try_again) {
    return true;
  }

  if (err == boost::asio::error::eof ||
      err == boost::asio::error::connection_reset ||
      err == boost::asio::error::operation_aborted ||
      err == boost::asio::error::not_connected ||
      err == boost::asio::error::bad_descriptor ||
      err == boost::asio::error::network_down) {
    return false;
  }

  return not err;
}

void apply_common_socket_properties(boost::asio::ip::tcp::socket &sock) {
  sock.set_option(boost::asio::ip::tcp::no_delay(true));
  sock.set_option(boost::asio::socket_base::linger(false, 0));
  sock.set_option(boost::asio::socket_base::keep_alive(true));
}
} // namespace repertory::comm
