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
#ifndef REPERTORY_INCLUDE_EVENTS_TYPES_PROVIDER_OFFLINE_HPP_
#define REPERTORY_INCLUDE_EVENTS_TYPES_PROVIDER_OFFLINE_HPP_

#include "events/i_event.hpp"
#include "types/repertory.hpp"

namespace repertory {
struct provider_offline final : public i_event {
  provider_offline() = default;
  provider_offline(std::string_view function_name_,
                   std::string host_name_or_ip_, std::uint16_t port_)
      : function_name(std::string(function_name_)),
        host_name_or_ip(std::move(host_name_or_ip_)),
        port(port_) {}

  static constexpr event_level level{event_level::warn};
  static constexpr std::string_view name{"provider_offline"};

  std::string function_name;
  std::string host_name_or_ip;
  std::uint16_t port{};

  [[nodiscard]] auto get_event_level() const -> event_level override {
    return level;
  }

  [[nodiscard]] auto get_name() const -> std::string_view override {
    return name;
  }

  [[nodiscard]] auto get_single_line() const -> std::string override {
    return fmt::format("{}|func|{}|host|{}|port|{}", name, function_name,
                       host_name_or_ip, port);
  }
};
} // namespace repertory

NLOHMANN_JSON_NAMESPACE_BEGIN
template <> struct adl_serializer<repertory::provider_offline> {
  static void to_json(json &data, const repertory::provider_offline &value) {
    data["function_name"] = value.function_name;
    data["host_name_or_ip"] = value.host_name_or_ip;
    data["port"] = value.port;
  }

  static void from_json(const json &data, repertory::provider_offline &value) {
    data.at("function_name").get_to<std::string>(value.function_name);
    data.at("host_name_or_ip").get_to<std::string>(value.host_name_or_ip);
    data.at("port").get_to<std::uint16_t>(value.port);
  }
};
NLOHMANN_JSON_NAMESPACE_END

#endif // REPERTORY_INCLUDE_EVENTS_TYPES_PROVIDER_OFFLINE_HPP_
