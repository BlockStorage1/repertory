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
#ifndef REPERTORY_INCLUDE_EVENTS_TYPES_DRIVE_MOUNT_RESULT_HPP_
#define REPERTORY_INCLUDE_EVENTS_TYPES_DRIVE_MOUNT_RESULT_HPP_

#include "events/i_event.hpp"
#include "types/repertory.hpp"

namespace repertory {
struct drive_mount_result final : public i_event {
  drive_mount_result() = default;
  drive_mount_result(std::string_view function_name_,
                     std::string mount_location_, std::string result_)
      : function_name(std::string(function_name_)),
        mount_location(std::move(mount_location_)),
        result(std::move(result_)) {}

  static constexpr const event_level level{event_level::info};
  static constexpr const std::string_view name{"drive_mount_result"};

  std::string function_name;
  std::string mount_location;
  std::string result;

  [[nodiscard]] auto get_event_level() const -> event_level override {
    return level;
  }

  [[nodiscard]] auto get_name() const -> std::string_view override {
    return name;
  }

  [[nodiscard]] auto get_single_line() const -> std::string override {
    return fmt::format("{}|func|{}|location|{}|result|{}", name, function_name,
                       mount_location, result);
  }
};
} // namespace repertory

NLOHMANN_JSON_NAMESPACE_BEGIN
template <> struct adl_serializer<repertory::drive_mount_result> {
  static void to_json(json &data, const repertory::drive_mount_result &value) {
    data["function_name"] = value.function_name;
    data["mount_location"] = value.mount_location;
    data["result"] = value.result;
  }

  static void from_json(const json &data,
                        repertory::drive_mount_result &value) {
    data.at("function_name").get_to<std::string>(value.function_name);
    data.at("mount_location").get_to<std::string>(value.mount_location);
    data.at("result").get_to<std::string>(value.result);
  }
};
NLOHMANN_JSON_NAMESPACE_END

#endif // REPERTORY_INCLUDE_EVENTS_TYPES_DRIVE_MOUNT_RESULT_HPP_
