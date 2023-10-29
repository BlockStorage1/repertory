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
#ifndef INCLUDE_COMM_PACKET_PACKET_SERVER_HPP_
#define INCLUDE_COMM_PACKET_PACKET_SERVER_HPP_

#include "comm/packet/client_pool.hpp"
#include "types/repertory.hpp"
#include "utils/utils.hpp"

using namespace boost::asio;
using boost::asio::ip::tcp;

namespace repertory {
class packet_server final {
public:
  using closed_callback = std::function<void(const std::string &)>;
  using message_complete_callback = client_pool::worker_complete_callback;
  using message_handler_callback = std::function<void(
      std::uint32_t, const std::string &, std::uint64_t, const std::string &,
      packet *, packet &, message_complete_callback)>;

public:
  packet_server(std::uint16_t port, std::string token, std::uint8_t pool_size,
                closed_callback closed,
                message_handler_callback message_handler);

  ~packet_server();

private:
  struct connection {
    connection(boost::asio::io_service &io_service, tcp::acceptor &acceptor_)
        : socket(io_service), acceptor(acceptor_) {}

    tcp::socket socket;
    tcp::acceptor &acceptor;
    data_buffer buffer;
    std::string client_id;
    std::string nonce;

    void generate_nonce() { nonce = utils::generate_random_string(256u); }
  };

private:
  const std::string encryption_token_;
  closed_callback closed_;
  message_handler_callback message_handler_;
  boost::asio::io_context io_context_;
  std::unique_ptr<std::thread> server_thread_;
  std::vector<std::thread> service_threads_;
  std::recursive_mutex connection_mutex_;
  std::unordered_map<std::string, std::uint32_t> connection_lookup_;

private:
  void add_client(connection &c, const std::string &client_id);

  void initialize(const uint16_t &port, uint8_t pool_size);

  void listen_for_connection(tcp::acceptor &acceptor);

  void on_accept(std::shared_ptr<connection> c, boost::system::error_code ec);

  void read_header(std::shared_ptr<connection> c);

  void read_packet(std::shared_ptr<connection> c, std::uint32_t data_size);

  void remove_client(connection &c);

  void send_response(std::shared_ptr<connection> c,
                     const packet::error_type &result, packet &response);
};
} // namespace repertory

#endif // INCLUDE_COMM_PACKET_PACKET_SERVER_HPP_
