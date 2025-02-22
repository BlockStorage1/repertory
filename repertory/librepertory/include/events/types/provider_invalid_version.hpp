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
#ifndef REPERTORY_INCLUDE_EVENTS_TYPES_PRODIVER_INVALID_VERSION_HPP_
#define REPERTORY_INCLUDE_EVENTS_TYPES_PRODIVER_INVALID_VERSION_HPP_

#include "events/i_event.hpp"
#include "types/repertory.hpp"

namespace repertory {
struct provider_invalid_version final : public i_event {
  provider_invalid_version() = default;
  provider_invalid_version(std::string_view function_name_,
                           std::string required_version_,
                           std::string returned_version_)
      : function_name(std::string(function_name_)),
        required_version(std::move(required_version_)),
        returned_version(std::move(returned_version_)) {}

  static constexpr const event_level level{event_level::error};
  static constexpr const std::string_view name{"provider_invalid_version"};

  std::string function_name;
  std::string required_version;
  std::string returned_version;

  [[nodiscard]] auto get_event_level() const -> event_level override {
    return level;
  }

  [[nodiscard]] auto get_name() const -> std::string_view override {
    return name;
  }

  [[nodiscard]] auto get_single_line() const -> std::string override {
    return fmt::format("{}|func|{}|required|{}|returned|{}", name,
                       function_name, required_version, returned_version);
  }
};
} // namespace repertory

NLOHMANN_JSON_NAMESPACE_BEGIN
template <> struct adl_serializer<repertory::provider_invalid_version> {
  static void to_json(json &data,
                      const repertory::provider_invalid_version &value) {
    data["function_name"] = value.function_name;
    data["required_version"] = value.required_version;
    data["returned_version"] = value.returned_version;
  }

  static void from_json(const json &data,
                        repertory::provider_invalid_version &value) {
    data.at("function_name").get_to(value.function_name);
    data.at("required_version").get_to(value.required_version);
    data.at("returned_version").get_to(value.returned_version);
  }
};
NLOHMANN_JSON_NAMESPACE_END

#endif // REPERTORY_INCLUDE_EVENTS_TYPES_PRODIVER_INVALID_VERSION_HPP_
