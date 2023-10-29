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
#include "comm/packet/packet_server.hpp"

#include "comm/packet/packet.hpp"
#include "events/event_system.hpp"
#include "events/events.hpp"
#include "types/repertory.hpp"
#include "utils/error_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
using std::thread;

packet_server::packet_server(std::uint16_t port, std::string token,
                             std::uint8_t pool_size, closed_callback closed,
                             message_handler_callback message_handler)
    : encryption_token_(std::move(token)),
      closed_(std::move(closed)),
      message_handler_(std::move(message_handler)) {
  initialize(port, pool_size);
  event_system::instance().raise<service_started>("packet_server");
}

packet_server::~packet_server() {
  event_system::instance().raise<service_shutdown_begin>("packet_server");
  std::thread([this]() {
    for (std::size_t i = 0u; i < service_threads_.size(); i++) {
      io_context_.stop();
    }
  }).detach();

  server_thread_->join();
  server_thread_.reset();
  event_system::instance().raise<service_shutdown_end>("packet_server");
}

void packet_server::add_client(connection &c, const std::string &client_id) {
  c.client_id = client_id;

  recur_mutex_lock connection_lock(connection_mutex_);
  if (connection_lookup_.find(client_id) == connection_lookup_.end()) {
    connection_lookup_[client_id] = 1u;
  } else {
    connection_lookup_[client_id]++;
  }
}

void packet_server::initialize(const uint16_t &port, uint8_t pool_size) {
  pool_size = std::max(uint8_t(1u), pool_size);
  server_thread_ = std::make_unique<std::thread>([this, port, pool_size]() {
    tcp::acceptor acceptor(io_context_);
    try {
      const auto endpoint = tcp::endpoint(tcp::v4(), port);
      acceptor.open(endpoint.protocol());
      acceptor.set_option(socket_base::reuse_address(true));
      acceptor.bind(endpoint);
      acceptor.listen();
    } catch (const std::exception &e) {
      repertory::utils::error::raise_error(__FUNCTION__, e,
                                           "exception occurred");
    }
    listen_for_connection(acceptor);

    for (std::uint8_t i = 0u; i < pool_size; i++) {
      service_threads_.emplace_back([this]() { io_context_.run(); });
    }

    for (auto &th : service_threads_) {
      th.join();
    }
  });
}

void packet_server::listen_for_connection(tcp::acceptor &acceptor) {
  auto c = std::make_shared<packet_server::connection>(io_context_, acceptor);
  acceptor.async_accept(c->socket,
                        boost::bind(&packet_server::on_accept, this, c,
                                    boost::asio::placeholders::error));
}

void packet_server::on_accept(std::shared_ptr<connection> c,
                              boost::system::error_code ec) {
  listen_for_connection(c->acceptor);
  if (ec) {
    utils::error::raise_error(__FUNCTION__, ec.message());
    std::this_thread::sleep_for(1s);
  } else {
    c->socket.set_option(boost::asio::ip::tcp::no_delay(true));
    c->socket.set_option(boost::asio::socket_base::linger(false, 0));

    c->generate_nonce();

    packet response;
    send_response(c, 0, response);
  }
}

void packet_server::read_header(std::shared_ptr<connection> c) {
  static const std::string function_name = __FUNCTION__;

  c->buffer.resize(sizeof(std::uint32_t));
  boost::asio::async_read(
      c->socket, boost::asio::buffer(&c->buffer[0u], c->buffer.size()),
      [this, c](boost::system::error_code ec, std::size_t) {
        if (ec) {
          remove_client(*c);
          repertory::utils::error::raise_error(function_name, ec.message());
        } else {
          auto to_read = *reinterpret_cast<std::uint32_t *>(&c->buffer[0u]);
          boost::endian::big_to_native_inplace(to_read);
          read_packet(c, to_read);
        }
      });
}

void packet_server::read_packet(std::shared_ptr<connection> c,
                                std::uint32_t data_size) {
  try {
    const auto read_buffer = [&]() {
      std::uint32_t offset = 0u;
      while (offset < c->buffer.size()) {
        const auto bytes_read = boost::asio::read(
            c->socket,
            boost::asio::buffer(&c->buffer[offset], c->buffer.size() - offset));
        if (bytes_read <= 0) {
          throw std::runtime_error("read failed|" + std::to_string(bytes_read));
        }
        offset += static_cast<std::uint32_t>(bytes_read);
      }
    };

    auto should_send_response = true;
    auto response = std::make_shared<packet>();
    c->buffer.resize(data_size);
    read_buffer();

    packet::error_type ret;
    auto request = std::make_shared<packet>(c->buffer);
    if (request->decrypt(encryption_token_) == 0) {
      std::string nonce;
      if ((ret = request->decode(nonce)) == 0) {
        if (nonce != c->nonce) {
          throw std::runtime_error("invalid nonce");
        }
        c->generate_nonce();

        std::string version;
        if ((ret = request->decode(version)) == 0) {
          if (utils::compare_version_strings(
                  version, REPERTORY_MIN_REMOTE_VERSION) >= 0) {
            std::uint32_t service_flags = 0u;
            DECODE_OR_IGNORE(request, service_flags);

            std::string client_id;
            DECODE_OR_IGNORE(request, client_id);

            std::uint64_t thread_id = 0u;
            DECODE_OR_IGNORE(request, thread_id);

            std::string method;
            DECODE_OR_IGNORE(request, method);

            if (ret == 0) {
              if (c->client_id.empty()) {
                add_client(*c, client_id);
              }

              should_send_response = false;
              message_handler_(service_flags, client_id, thread_id, method,
                               request.get(), *response,
                               [this, c, request,
                                response](const packet::error_type &result) {
                                 this->send_response(c, result, *response);
                               });
            }
          } else {
            ret = utils::from_api_error(api_error::incompatible_version);
          }
        } else {
          ret = utils::from_api_error(api_error::invalid_version);
        }
      } else {
        throw std::runtime_error("invalid nonce");
      }
    } else {
      throw std::runtime_error("decryption failed");
    }
    if (should_send_response) {
      send_response(c, ret, *response);
    }
  } catch (const std::exception &e) {
    remove_client(*c);
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }
}

void packet_server::remove_client(connection &c) {
  if (not c.client_id.empty()) {
    recur_mutex_lock connection_lock(connection_mutex_);
    if (not --connection_lookup_[c.client_id]) {
      connection_lookup_.erase(c.client_id);
      closed_(c.client_id);
    }
  }
}

void packet_server::send_response(std::shared_ptr<connection> c,
                                  const packet::error_type &result,
                                  packet &response) {
  static const std::string function_name = __FUNCTION__;

  response.encode_top(result);
  response.encode_top(PACKET_SERVICE_FLAGS);
  response.encode_top(c->nonce);
  response.encrypt(encryption_token_);
  response.transfer_into(c->buffer);

  boost::asio::async_write(
      c->socket, boost::asio::buffer(c->buffer),
      [this, c](boost::system::error_code ec, std::size_t /*length*/) {
        if (ec) {
          remove_client(*c);
          utils::error::raise_error(function_name, ec.message());
        } else {
          read_header(c);
        }
      });
}
} // namespace repertory
