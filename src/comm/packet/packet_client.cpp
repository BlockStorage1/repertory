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
#include "comm/packet/packet_client.hpp"

#include "events/events.hpp"
#include "types/repertory.hpp"
#include "utils/error_utils.hpp"
#include "utils/timeout.hpp"

namespace repertory {
// clang-format off
E_SIMPLE2(packet_client_timeout, error, true,
  std::string, event_name, en, E_STRING,
  std::string, message, msg, E_STRING
);
// clang-format on

packet_client::packet_client(std::string host_name_or_ip,
                             std::uint8_t max_connections, std::uint16_t port,
                             std::uint16_t receive_timeout,
                             std::uint16_t send_timeout,
                             std::string encryption_token)
    : io_context_(),
      host_name_or_ip_(std::move(host_name_or_ip)),
      max_connections_(max_connections ? max_connections : 20u),
      port_(port),
      receive_timeout_(receive_timeout),
      send_timeout_(send_timeout),
      encryption_token_(std::move(encryption_token)),
      unique_id_(utils::create_uuid_string()) {}

packet_client::~packet_client() {
  allow_connections_ = false;
  close_all();
  io_context_.stop();
}

void packet_client::close(client &cli) const {
  try {
    boost::system::error_code ec;
    cli.socket.close(ec);
  } catch (...) {
  }
}

void packet_client::close_all() {
  unique_mutex_lock clients_lock(clients_mutex_);
  for (auto &c : clients_) {
    close(*c.get());
  }

  clients_.clear();
  unique_id_ = utils::create_uuid_string();
}

void packet_client::connect(client &c) {
  try {
    resolve();
    boost::asio::connect(c.socket, resolve_results_);
    c.socket.set_option(boost::asio::ip::tcp::no_delay(true));
    c.socket.set_option(boost::asio::socket_base::linger(false, 0));

    packet response;
    const auto res = read_packet(c, response);
    if (res != 0) {
      throw std::runtime_error(std::to_string(res));
    }
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "connection handshake failed");
  }
}

auto packet_client::get_client() -> std::shared_ptr<packet_client::client> {
  std::shared_ptr<client> ret;

  unique_mutex_lock clients_lock(clients_mutex_);
  if (allow_connections_) {
    if (clients_.empty()) {
      clients_lock.unlock();
      ret = std::make_shared<client>(io_context_);
      connect(*ret);
    } else {
      ret = clients_[0u];
      utils::remove_element_from(clients_, ret);
      clients_lock.unlock();
    }
  }

  return ret;
}

void packet_client::put_client(std::shared_ptr<client> &c) {
  mutex_lock clientsLock(clients_mutex_);
  if (clients_.size() < max_connections_) {
    clients_.emplace_back(c);
  }
}

auto packet_client::read_packet(client &c, packet &response)
    -> packet::error_type {
  data_buffer buffer(sizeof(std::uint32_t));
  const auto read_buffer = [&]() {
    std::uint32_t offset = 0u;
    while (offset < buffer.size()) {
      const auto bytes_read = boost::asio::read(
          c.socket,
          boost::asio::buffer(&buffer[offset], buffer.size() - offset));
      if (bytes_read <= 0) {
        throw std::runtime_error("read failed|" + std::to_string(bytes_read));
      }
      offset += static_cast<std::uint32_t>(bytes_read);
    }
  };
  read_buffer();

  const auto size = boost::endian::big_to_native(
      *reinterpret_cast<std::uint32_t *>(&buffer[0u]));
  buffer.resize(size);

  read_buffer();
  response = std::move(buffer);

  auto ret = response.decrypt(encryption_token_);
  if (ret == 0) {
    ret = response.decode(c.nonce);
  }

  return ret;
}

void packet_client::resolve() {
  if (resolve_results_.empty()) {
    resolve_results_ = tcp::resolver(io_context_)
                           .resolve({host_name_or_ip_, std::to_string(port_)});
  }
}

auto packet_client::send(const std::string &method,
                         std::uint32_t &service_flags) -> packet::error_type {
  packet request;
  return send(method, request, service_flags);
}

auto packet_client::send(const std::string &method, packet &request,
                         std::uint32_t &service_flags) -> packet::error_type {
  packet response;
  return send(method, request, response, service_flags);
}

auto packet_client::send(const std::string &method, packet &request,
                         packet &response, std::uint32_t &service_flags)
    -> packet::error_type {
  auto success = false;
  packet::error_type ret = utils::from_api_error(api_error::error);
  request.encode_top(method);
  request.encode_top(utils::get_thread_id());
  request.encode_top(unique_id_);
  request.encode_top(PACKET_SERVICE_FLAGS);
  request.encode_top(get_repertory_version());

  static const std::uint8_t max_attempts = 5u;
  for (std::uint8_t i = 1u;
       allow_connections_ && not success && (i <= max_attempts); i++) {
    auto c = get_client();
    if (c) {
      try {
        request.encode_top(c->nonce);
        request.encrypt(encryption_token_);

        timeout request_timeout(
            [this, method, c]() {
              event_system::instance().raise<packet_client_timeout>("request",
                                                                    method);
              close(*c.get());
            },
            std::chrono::seconds(send_timeout_));

        std::uint32_t offset = 0u;
        while (offset < request.get_size()) {
          const auto bytes_written = boost::asio::write(
              c->socket, boost::asio::buffer(&request[offset],
                                             request.get_size() - offset));
          if (bytes_written <= 0) {
            throw std::runtime_error("write failed|" +
                                     std::to_string(bytes_written));
          }
          offset += static_cast<std::uint32_t>(bytes_written);
        }
        request_timeout.disable();

        timeout response_timeout(
            [this, method, c]() {
              event_system::instance().raise<packet_client_timeout>("response",
                                                                    method);
              close(*c.get());
            },
            std::chrono::seconds(receive_timeout_));

        ret = read_packet(*c, response);
        response_timeout.disable();
        if (ret == 0) {
          if ((ret = response.decode(service_flags)) == 0) {
            packet::error_type res{};
            if ((ret = response.decode(res)) == 0) {
              ret = res;
              success = true;
              put_client(c);
            }
          }
        }
      } catch (const std::exception &e) {
        utils::error::raise_error(__FUNCTION__, e, "send failed");
        close_all();
        if (allow_connections_ && (i < max_attempts)) {
          std::this_thread::sleep_for(1s);
        }
      }
    }

    if (not allow_connections_) {
      ret = utils::from_api_error(api_error::error);
      success = true;
    }
  }

  return CONVERT_STATUS_NOT_IMPLEMENTED(ret);
}
} // namespace repertory
