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
#include "comm/packet/packet_server.hpp"

#include "comm/packet/packet.hpp"
#include "events/event_system.hpp"
#include "events/types/service_start_begin.hpp"
#include "events/types/service_start_end.hpp"
#include "events/types/service_stop_begin.hpp"
#include "events/types/service_stop_end.hpp"
#include "platform/platform.hpp"
#include "types/repertory.hpp"
#include "utils/error_utils.hpp"

namespace repertory {
using std::thread;

packet_server::packet_server(std::uint16_t port, std::string token,
                             std::uint8_t pool_size, closed_callback closed,
                             message_handler_callback message_handler)
    : encryption_token_(std::move(token)),
      closed_(std::move(closed)),
      message_handler_(std::move(message_handler)) {
  REPERTORY_USES_FUNCTION_NAME();

  event_system::instance().raise<service_start_begin>(function_name,
                                                      "packet_server");
  initialize(port, pool_size);
  event_system::instance().raise<service_start_end>(function_name,
                                                    "packet_server");
}

packet_server::~packet_server() {
  REPERTORY_USES_FUNCTION_NAME();

  event_system::instance().raise<service_stop_begin>(function_name,
                                                     "packet_server");
  std::thread([this]() {
    for (std::size_t i = 0U; i < service_threads_.size(); i++) {
      io_context_.stop();
    }
  }).detach();

  server_thread_->join();
  server_thread_.reset();
  event_system::instance().raise<service_stop_end>(function_name,
                                                   "packet_server");
}

void packet_server::add_client(connection &conn, const std::string &client_id) {
  conn.client_id = client_id;

  recur_mutex_lock connection_lock(connection_mutex_);
  if (connection_lookup_.find(client_id) == connection_lookup_.end()) {
    connection_lookup_[client_id] = 1U;
  } else {
    connection_lookup_.at(client_id)++;
  }
}

void packet_server::initialize(const uint16_t &port, uint8_t pool_size) {
  REPERTORY_USES_FUNCTION_NAME();

  pool_size = std::max(std::uint8_t(1U), pool_size);
  server_thread_ = std::make_unique<std::thread>([this, port, pool_size]() {
    tcp::acceptor acceptor(io_context_);
    try {
      auto endpoint = tcp::endpoint(tcp::v4(), port);
      acceptor.open(endpoint.protocol());
      acceptor.set_option(socket_base::reuse_address(true));
      acceptor.bind(endpoint);
      acceptor.listen();
    } catch (const std::exception &e) {
      repertory::utils::error::raise_error(function_name, e,
                                           "exception occurred");
    }
    listen_for_connection(acceptor);

    for (std::uint8_t i = 0U; i < pool_size; i++) {
      service_threads_.emplace_back([this]() { io_context_.run(); });
    }

    for (auto &thread : service_threads_) {
      thread.join();
    }
  });
}

void packet_server::listen_for_connection(tcp::acceptor &acceptor) {
  auto conn =
      std::make_shared<packet_server::connection>(io_context_, acceptor);
  acceptor.async_accept(conn->socket, [this, conn](auto &&err) {
    on_accept(conn, std::forward<decltype(err)>(err));
  });
}

void packet_server::on_accept(std::shared_ptr<connection> conn,
                              boost::system::error_code err) {
  REPERTORY_USES_FUNCTION_NAME();

  listen_for_connection(conn->acceptor);
  if (err) {
    utils::error::raise_error(function_name, err.message());
    std::this_thread::sleep_for(1s);
  } else {
    conn->socket.set_option(boost::asio::ip::tcp::no_delay(true));
    conn->socket.set_option(boost::asio::socket_base::linger(false, 0));

    conn->generate_nonce();

    packet response;
    send_response(conn, 0, response);
  }
}

void packet_server::read_header(std::shared_ptr<connection> conn) {
  REPERTORY_USES_FUNCTION_NAME();

  conn->buffer.resize(sizeof(std::uint32_t));
  boost::asio::async_read(
      conn->socket,
      boost::asio::buffer(conn->buffer.data(), conn->buffer.size()),
      [this, conn](boost::system::error_code err, std::size_t) {
        if (err) {
          remove_client(*conn);
          repertory::utils::error::raise_error(function_name, err.message());
        } else {
          auto to_read =
              *reinterpret_cast<std::uint32_t *>(conn->buffer.data());
          boost::endian::big_to_native_inplace(to_read);
          read_packet(conn, to_read);
        }
      });
}

void packet_server::read_packet(std::shared_ptr<connection> conn,
                                std::uint32_t data_size) {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    const auto read_buffer = [&]() {
      std::uint32_t offset{};
      while (offset < conn->buffer.size()) {
        auto bytes_read = boost::asio::read(
            conn->socket, boost::asio::buffer(&conn->buffer[offset],
                                              conn->buffer.size() - offset));
        if (bytes_read <= 0) {
          throw std::runtime_error("read failed|" + std::to_string(bytes_read));
        }
        offset += static_cast<std::uint32_t>(bytes_read);
      }
    };

    auto should_send_response = true;
    auto response = std::make_shared<packet>();
    conn->buffer.resize(data_size);
    read_buffer();

    packet::error_type ret{};
    auto request = std::make_shared<packet>(conn->buffer);
    if (request->decrypt(encryption_token_) == 0) {
      std::string nonce;
      ret = request->decode(nonce);
      if (ret == 0) {
        if (nonce != conn->nonce) {
          throw std::runtime_error("invalid nonce");
        }
        conn->generate_nonce();

        std::string version;
        ret = request->decode(version);
        if (ret == 0) {
          if (utils::compare_version_strings(
                  version, std::string{REPERTORY_MIN_REMOTE_VERSION}) >= 0) {
            std::uint32_t service_flags{};
            DECODE_OR_IGNORE(request, service_flags);

            std::string client_id;
            DECODE_OR_IGNORE(request, client_id);

            std::uint64_t thread_id{};
            DECODE_OR_IGNORE(request, thread_id);

            std::string method;
            DECODE_OR_IGNORE(request, method);

            if (ret == 0) {
              if (conn->client_id.empty()) {
                add_client(*conn, client_id);
              }

              should_send_response = false;
              message_handler_(service_flags, client_id, thread_id, method,
                               request.get(), *response,
                               [this, conn, request,
                                response](const packet::error_type &result) {
                                 this->send_response(conn, result, *response);
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
      send_response(conn, ret, *response);
    }
  } catch (const std::exception &e) {
    remove_client(*conn);
    utils::error::raise_error(function_name, e, "exception occurred");
  }
}

void packet_server::remove_client(connection &conn) {
  if (not conn.client_id.empty()) {
    recur_mutex_lock connection_lock(connection_mutex_);
    if (--connection_lookup_[conn.client_id] == 0U) {
      connection_lookup_.erase(conn.client_id);
      closed_(conn.client_id);
    }
  }
}

void packet_server::send_response(std::shared_ptr<connection> conn,
                                  const packet::error_type &result,
                                  packet &response) {
  REPERTORY_USES_FUNCTION_NAME();

  response.encode_top(result);
  response.encode_top(PACKET_SERVICE_FLAGS);
  response.encode_top(conn->nonce);
  response.encrypt(encryption_token_);
  response.to_buffer(conn->buffer);

  boost::asio::async_write(
      conn->socket, boost::asio::buffer(conn->buffer),
      [this, conn](boost::system::error_code err, std::size_t /*length*/) {
        if (err) {
          remove_client(*conn);
          utils::error::raise_error(function_name, err.message());
        } else {
          read_header(conn);
        }
      });
}
} // namespace repertory
