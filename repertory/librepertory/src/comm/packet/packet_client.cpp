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
#include "comm/packet/packet_client.hpp"

#include "comm/packet/common.hpp"
#include "events/event_system.hpp"
#include "events/types/packet_client_timeout.hpp"
#include "platform/platform.hpp"
#include "types/repertory.hpp"
#include "utils/collection.hpp"
#include "utils/common.hpp"
#include "utils/config.hpp"
#include "utils/error_utils.hpp"
#include "utils/timeout.hpp"
#include "utils/utils.hpp"
#include "version.hpp"

using namespace repertory::comm;

namespace repertory {
packet_client::packet_client(remote::remote_config cfg)
    : cfg_(std::move(cfg)), unique_id_(utils::create_uuid_string()) {
  for (std::uint8_t idx = 0U; idx < cfg.max_connections; ++idx) {
    service_threads_.emplace_back([this]() { io_context_.run(); });
  }
}

packet_client::~packet_client() {
  allow_connections_ = false;

  try {
    close_all();
  } catch (...) {
  }

  try {
    io_context_.stop();
  } catch (...) {
  }

  for (auto &thread : service_threads_) {
    try {
      if (thread.joinable()) {
        thread.join();
      }
    } catch (...) {
    }
  }
}

void packet_client::close(client &cli) noexcept {
  boost::system::error_code err;
  [[maybe_unused]] auto res = cli.socket.cancel(err);

  boost::system::error_code err2;
  [[maybe_unused]] auto res2 =
      cli.socket.shutdown(boost::asio::socket_base::shutdown_both, err2);

  boost::system::error_code err3;
  [[maybe_unused]] auto res3 = cli.socket.close(err3);
}

void packet_client::close_all() {
  mutex_lock clients_lock(clients_mutex_);
  for (auto &cli : clients_) {
    close(*cli);
  }
  clients_.clear();

  resolve_results_.store({});
  unique_id_ = utils::create_uuid_string();
}

auto packet_client::check_version(std::uint32_t client_version,
                                  std::uint32_t &min_version) -> api_error {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    min_version = 0U;

    boost::asio::io_context ctx{};
    client cli(ctx);

    utils::timeout timeout(
        [&cli]() {
          event_system::instance().raise<packet_client_timeout>("connect",
                                                                function_name);
          packet_client::close(cli);
        },
        std::chrono::milliseconds(cfg_.conn_timeout_ms));

    boost::asio::connect(
        cli.socket, tcp::resolver(ctx).resolve(cfg_.host_name_or_ip,
                                               std::to_string(cfg_.api_port)));
    timeout.disable();
    if (not is_socket_still_alive(cli.socket)) {
      throw std::runtime_error("failed to connect");
    }

    comm::apply_common_socket_properties(cli.socket);

    if (not handshake(cli, min_version)) {
      return api_error::comm_error;
    }

    if (client_version < min_version) {
      return api_error::incompatible_version;
    }

    return api_error::success;
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "connection handshake failed");
  }

  return api_error::error;
}

auto packet_client::connect(client &cli) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    utils::timeout timeout(
        [&cli]() {
          event_system::instance().raise<packet_client_timeout>("connect",
                                                                function_name);
          packet_client::close(cli);
        },
        std::chrono::milliseconds(cfg_.conn_timeout_ms));

    resolve();

    boost::asio::connect(cli.socket, resolve_results_.load());
    timeout.disable();

    if (not is_socket_still_alive(cli.socket)) {
      throw std::runtime_error("failed to connect");
    }

    comm::apply_common_socket_properties(cli.socket);

    std::uint32_t min_version{};
    if (not handshake(cli, min_version)) {
      close(cli);
      return false;
    }

    packet response;
    auto res = read_packet(cli, response);
    if (res != 0) {
      throw std::runtime_error(fmt::format("read packet failed|err|{}", res));
    }
    return true;
  } catch (...) {
    close(cli);
    resolve_results_.store({});
    throw;
  }
}

auto packet_client::get_client() -> std::shared_ptr<packet_client::client> {
  REPERTORY_USES_FUNCTION_NAME();

  if (not allow_connections_) {
    return nullptr;
  }

  try {
    unique_mutex_lock clients_lock(clients_mutex_);
    if (clients_.empty()) {
      clients_lock.unlock();

      auto cli = std::make_shared<client>(io_context_);
      if (connect(*cli)) {
        return cli;
      }

      return nullptr;
    }

    auto cli = clients_.at(0U);
    utils::collection::remove_element(clients_, cli);

    if (is_socket_still_alive(cli->socket)) {
      return cli;
    }

    clients_lock.unlock();
    return get_client();
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "get_client failed");
  }

  return nullptr;
}

auto packet_client::handshake(client &cli, std::uint32_t &min_version) const
    -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    min_version = 0U;

    data_buffer buffer;
    {
      packet tmp;
      tmp.encode(utils::get_version_number(project_get_version()));
      tmp.encode(~utils::get_version_number(project_get_version()));
      tmp.encode(utils::generate_random_string(packet_nonce_size));
      tmp.to_buffer(buffer);
    }

    read_data(cli, buffer);
    packet response(buffer);

    auto res = response.decode(min_version);
    if (res != 0) {
      throw std::runtime_error("failed to decode server version");
    }

    std::uint32_t min_version_check{};
    res = response.decode(min_version_check);
    if (res != 0) {
      throw std::runtime_error("failed to decode server version");
    }

    if (min_version_check != ~min_version) {
      throw std::runtime_error("failed to decode server version");
    }

    response.encrypt(cfg_.encryption_token, false);
    write_data(cli, response);

    return true;
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e, "handshake failed");
  }

  return false;
}

void packet_client::put_client(std::shared_ptr<client> &cli) {
  if (not cli || not is_socket_still_alive(cli->socket)) {
    return;
  }

  mutex_lock clients_lock(clients_mutex_);
  if (clients_.size() < cfg_.max_connections) {
    clients_.emplace_back(cli);
  }
}

void packet_client::read_data(client &cli, data_buffer &buffer) const {
  REPERTORY_USES_FUNCTION_NAME();

  utils::timeout timeout(
      [&cli]() {
        event_system::instance().raise<packet_client_timeout>("response",
                                                              function_name);
        packet_client::close(cli);
      },
      std::chrono::milliseconds(cfg_.recv_timeout_ms));

  std::uint32_t offset{};
  while (offset < buffer.size()) {
    auto to_read = std::min(read_write_size, buffer.size() - offset);
    auto bytes_read = boost::asio::read(
        cli.socket, boost::asio::buffer(&buffer[offset], to_read));
    if (bytes_read <= 0) {
      throw std::runtime_error("read failed|" + std::to_string(bytes_read));
    }
    offset += static_cast<std::uint32_t>(bytes_read);
    timeout.reset();
  }
}

auto packet_client::read_packet(client &cli, packet &response) const
    -> packet::error_type {
  data_buffer buffer(sizeof(std::uint32_t));
  read_data(cli, buffer);

  std::uint32_t size{};
  std::memcpy(&size, buffer.data(), buffer.size());
  boost::endian::big_to_native_inplace(size);

  if (size > comm::max_packet_bytes) {
    throw std::runtime_error(fmt::format("packet too large|size|{}", size));
  }

  if (size < utils::encryption::encryption_header_size) {
    throw std::runtime_error(fmt::format("packet too small|size|{}", size));
  }

  buffer.resize(size);
  read_data(cli, buffer);

  response = std::move(buffer);

  auto ret = response.decrypt(cfg_.encryption_token);
  if (ret == 0) {
    ret = response.decode(cli.nonce);
  }

  return ret;
}

void packet_client::resolve() {
  boost::asio::ip::basic_resolver<boost::asio::ip::tcp>::results_type cached =
      resolve_results_;
  if (not cached.empty()) {
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
  auto ret = utils::from_api_error(api_error::error);

  auto base_request = request;
  base_request.encode_top(method);
  base_request.encode_top(utils::get_thread_id());
  base_request.encode_top(unique_id_.load());
  base_request.encode_top(PACKET_SERVICE_FLAGS);
  base_request.encode_top(std::string{project_get_version()});

  for (std::uint8_t retry = 1U;
       allow_connections_ && not success && (retry <= max_read_attempts);
       ++retry) {
    const auto retry_after_sleep = [&]() {
      if (allow_connections_ && (retry < max_read_attempts)) {
        std::this_thread::sleep_for(1s);
      }
    };

    auto current_client = get_client();
    if (not current_client) {
      retry_after_sleep();
      continue;
    }

    try {
      auto current_request = base_request;
      current_request.encode_top(current_client->nonce);
      request = current_request;

      current_request.encrypt(cfg_.encryption_token, true);
      write_data(*current_client, current_request);

      ret = read_packet(*current_client, response);
      if (ret == 0) {
        ret = response.decode(service_flags);
        if (ret == 0) {
          packet::error_type res{};
          ret = response.decode(res);
          if (ret == 0) {
            ret = res;
            success = true;
            put_client(current_client);
            continue;
          }
        }
      }
    } catch (const std::exception &e) {
      utils::error::raise_error(function_name, e, "send failed");
      close(*current_client);
      retry_after_sleep();
    }
  }

  if (not allow_connections_) {
    ret = utils::from_api_error(api_error::error);
  }

  return CONVERT_STATUS_NOT_IMPLEMENTED(ret);
}

void packet_client::write_data(client &cli, const packet &request) const {
  REPERTORY_USES_FUNCTION_NAME();

  utils::timeout timeout(
      [&cli]() {
        event_system::instance().raise<packet_client_timeout>("request",
                                                              function_name);
        packet_client::close(cli);
      },
      std::chrono::milliseconds(cfg_.send_timeout_ms));

  std::uint32_t offset{};
  while (offset < request.get_size()) {
    auto to_write =
        std::min(read_write_size, std::size_t{request.get_size() - offset});
    auto bytes_written = boost::asio::write(
        cli.socket, boost::asio::buffer(&request[offset], to_write));
    if (bytes_written <= 0) {
      throw std::runtime_error("write failed|" + std::to_string(bytes_written));
    }
    offset += static_cast<std::uint32_t>(bytes_written);
    timeout.reset();
  }
}
} // namespace repertory
