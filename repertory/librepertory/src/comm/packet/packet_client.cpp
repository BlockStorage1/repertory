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
#include "comm/packet/packet_client.hpp"

#include "events/event_system.hpp"
#include "platform/platform.hpp"
#include "types/repertory.hpp"
#include "utils/collection.hpp"
#include "utils/common.hpp"
#include "utils/error_utils.hpp"
#include "utils/timeout.hpp"
#include "version.hpp"

namespace repertory {
// clang-format off
E_SIMPLE2(packet_client_timeout, error, true,
  std::string, event_name, en, E_FROM_STRING,
  std::string, message, msg, E_FROM_STRING
);
// clang-format on

packet_client::packet_client(remote::remote_config cfg)
    : cfg_(std::move(cfg)), unique_id_(utils::create_uuid_string()) {}

packet_client::~packet_client() {
  allow_connections_ = false;
  close_all();
  io_context_.stop();
}

void packet_client::close(client &cli) {
  try {
    boost::system::error_code err;
    cli.socket.close(err);
  } catch (...) {
  }
}

void packet_client::close_all() {
  unique_mutex_lock clients_lock(clients_mutex_);
  for (auto &cli : clients_) {
    close(*cli);
  }

  clients_.clear();
  unique_id_ = utils::create_uuid_string();
}

void packet_client::connect(client &cli) {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    resolve();
    boost::asio::connect(cli.socket, resolve_results_);
    cli.socket.set_option(boost::asio::ip::tcp::no_delay(true));
    cli.socket.set_option(boost::asio::socket_base::linger(false, 0));

    packet response;
    auto res = read_packet(cli, response);
    if (res != 0) {
      throw std::runtime_error(std::to_string(res));
    }
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "connection handshake failed");
  }
}

auto packet_client::get_client() -> std::shared_ptr<packet_client::client> {
  unique_mutex_lock clients_lock(clients_mutex_);
  if (not allow_connections_) {
    return nullptr;
  }

  if (clients_.empty()) {
    clients_lock.unlock();

    auto cli = std::make_shared<client>(io_context_);
    connect(*cli);
    return cli;
  }

  auto cli = clients_.at(0U);
  utils::collection::remove_element(clients_, cli);
  return cli;
}

void packet_client::put_client(std::shared_ptr<client> &cli) {
  mutex_lock clientsLock(clients_mutex_);
  if (clients_.size() < cfg_.max_connections) {
    clients_.emplace_back(cli);
  }
}

auto packet_client::read_packet(client &cli, packet &response)
    -> packet::error_type {
  data_buffer buffer(sizeof(std::uint32_t));
  const auto read_buffer = [&]() {
    std::uint32_t offset{};
    while (offset < buffer.size()) {
      auto bytes_read = boost::asio::read(
          cli.socket,
          boost::asio::buffer(&buffer[offset], buffer.size() - offset));
      if (bytes_read <= 0) {
        throw std::runtime_error("read failed|" + std::to_string(bytes_read));
      }
      offset += static_cast<std::uint32_t>(bytes_read);
    }
  };
  read_buffer();

  auto size = boost::endian::big_to_native(
      *reinterpret_cast<std::uint32_t *>(buffer.data()));
  buffer.resize(size);

  read_buffer();
  response = std::move(buffer);

  auto ret = response.decrypt(cfg_.encryption_token);
  if (ret == 0) {
    ret = response.decode(cli.nonce);
  }

  return ret;
}

void packet_client::resolve() {
  if (not resolve_results_.empty()) {
    return;
  }

  resolve_results_ =
      tcp::resolver(io_context_)
          .resolve(cfg_.host_name_or_ip, std::to_string(cfg_.api_port));
}

auto packet_client::send(std::string_view method, std::uint32_t &service_flags)
    -> packet::error_type {
  packet request;
  return send(method, request, service_flags);
}

auto packet_client::send(std::string_view method, packet &request,
                         std::uint32_t &service_flags) -> packet::error_type {
  packet response;
  return send(method, request, response, service_flags);
}

auto packet_client::send(std::string_view method, packet &request,
                         packet &response, std::uint32_t &service_flags)
    -> packet::error_type {
  REPERTORY_USES_FUNCTION_NAME();

  auto success = false;
  packet::error_type ret = utils::from_api_error(api_error::error);
  request.encode_top(method);
  request.encode_top(utils::get_thread_id());
  request.encode_top(unique_id_);
  request.encode_top(PACKET_SERVICE_FLAGS);
  request.encode_top(std::string{project_get_version()});

  static constexpr const std::uint8_t max_attempts{5U};
  for (std::uint8_t i = 1U;
       allow_connections_ && not success && (i <= max_attempts); i++) {
    auto current_client = get_client();
    if (current_client) {
      try {
        request.encode_top(current_client->nonce);
        request.encrypt(cfg_.encryption_token);

        timeout request_timeout(
            [method, current_client]() {
              event_system::instance().raise<packet_client_timeout>(
                  "request", std::string{method});
              packet_client::close(*current_client);
            },
            std::chrono::milliseconds(cfg_.send_timeout_ms));

        std::uint32_t offset{};
        while (offset < request.get_size()) {
          auto bytes_written = boost::asio::write(
              current_client->socket,
              boost::asio::buffer(&request[offset],
                                  request.get_size() - offset));
          if (bytes_written <= 0) {
            throw std::runtime_error("write failed|" +
                                     std::to_string(bytes_written));
          }
          offset += static_cast<std::uint32_t>(bytes_written);
        }
        request_timeout.disable();

        timeout response_timeout(
            [method, current_client]() {
              event_system::instance().raise<packet_client_timeout>(
                  "response", std::string{method});
              packet_client::close(*current_client);
            },
            std::chrono::milliseconds(cfg_.recv_timeout_ms));

        ret = read_packet(*current_client, response);
        response_timeout.disable();
        if (ret == 0) {
          ret = response.decode(service_flags);
          if (ret == 0) {
            packet::error_type res{};
            ret = response.decode(res);
            if (ret == 0) {
              ret = res;
              success = true;
              put_client(current_client);
            }
          }
        }
      } catch (const std::exception &e) {
        utils::error::raise_error(function_name, e, "send failed");
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
