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
#ifndef REPERTORY_INCLUDE_COMM_PACKET_PACKET_CLIENT_HPP_
#define REPERTORY_INCLUDE_COMM_PACKET_PACKET_CLIENT_HPP_

#include "comm/packet/packet.hpp"
#include "types/remote.hpp"

using boost::asio::ip::tcp;

namespace repertory {
class packet_client final {
private:
  struct client final {
    explicit client(boost::asio::io_context &ctx) : socket(ctx) {}
    std::string nonce;
    tcp::socket socket;
  };

public:
  packet_client(remote::remote_config cfg);

  ~packet_client();

  packet_client(const packet_client &) = delete;
  packet_client(packet_client &&) = delete;
  auto operator=(const packet_client &) -> packet_client & = delete;
  auto operator=(packet_client &&) -> packet_client & = delete;

private:
  boost::asio::io_context io_context_;
  remote::remote_config cfg_;
  std::string unique_id_;

private:
  bool allow_connections_{true};
  boost::asio::ip::basic_resolver<boost::asio::ip::tcp>::results_type
      resolve_results_;
  std::mutex clients_mutex_;
  std::vector<std::shared_ptr<client>> clients_;

private:
  static void close(client &cli);

  void close_all();

  void connect(client &cli);

  [[nodiscard]] auto get_client() -> std::shared_ptr<client>;

  void put_client(std::shared_ptr<client> &cli);

  [[nodiscard]] auto read_packet(client &cli, packet &response)
      -> packet::error_type;

  void resolve();

public:
  [[nodiscard]] auto send(std::string_view method, std::uint32_t &service_flags)
      -> packet::error_type;

  [[nodiscard]] auto send(std::string_view method, packet &request,
                          std::uint32_t &service_flags) -> packet::error_type;

  [[nodiscard]] auto send(std::string_view method, packet &request,
                          packet &response, std::uint32_t &service_flags)
      -> packet::error_type;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_COMM_PACKET_PACKET_CLIENT_HPP_
