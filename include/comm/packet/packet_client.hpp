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
#ifndef INCLUDE_COMM_PACKET_PACKET_CLIENT_HPP_
#define INCLUDE_COMM_PACKET_PACKET_CLIENT_HPP_

#include "comm/packet/packet.hpp"

using boost::asio::ip::tcp;

namespace repertory {
class packet_client final {
private:
  struct client final {
    explicit client(boost::asio::io_context &ctx) : nonce(""), socket(ctx) {}

    std::string nonce;
    tcp::socket socket;
  };

public:
  packet_client(std::string host_name_or_ip, std::uint8_t max_connections,
                std::uint16_t port, std::uint16_t receive_timeout,
                std::uint16_t send_timeout, std::string encryption_token);

  ~packet_client();

private:
  boost::asio::io_context io_context_;
  const std::string host_name_or_ip_;
  const std::uint8_t max_connections_;
  const std::uint16_t port_;
  const std::uint16_t receive_timeout_;
  const std::uint16_t send_timeout_;
  const std::string encryption_token_;
  std::string unique_id_;

  bool allow_connections_ = true;
  boost::asio::ip::basic_resolver<boost::asio::ip::tcp>::results_type
      resolve_results_;
  std::mutex clients_mutex_;
  std::vector<std::shared_ptr<client>> clients_;

private:
  void close(client &c) const;

  void close_all();

  void connect(client &c);

  [[nodiscard]] auto get_client() -> std::shared_ptr<client>;

  void put_client(std::shared_ptr<client> &c);

  [[nodiscard]] auto read_packet(client &c, packet &response)
      -> packet::error_type;

  void resolve();

public:
  [[nodiscard]] auto send(const std::string &method,
                          std::uint32_t &service_flags) -> packet::error_type;

  [[nodiscard]] auto send(const std::string &method, packet &request,
                          std::uint32_t &service_flags) -> packet::error_type;

  [[nodiscard]] auto send(const std::string &method, packet &request,
                          packet &response, std::uint32_t &service_flags)
      -> packet::error_type;
};
} // namespace repertory

#endif // INCLUDE_COMM_PACKET_PACKET_CLIENT_HPP_
