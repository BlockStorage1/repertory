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
#ifndef REPERTORY_INCLUDE_EVENTS_TYPES_DOWNLOAD_TYPE_SELECTED_HPP_
#define REPERTORY_INCLUDE_EVENTS_TYPES_DOWNLOAD_TYPE_SELECTED_HPP_

#include "events/i_event.hpp"
#include "types/repertory.hpp"

namespace repertory {
struct download_type_selected final : public i_event {
  download_type_selected() = default;
  download_type_selected(std::string_view api_path_,
                         std::string_view dest_path_,
                         std::string_view function_name_, download_type type_)
      : api_path(api_path_),
        dest_path(dest_path_),
        function_name(function_name_),
        type(type_) {}

  static constexpr event_level level{event_level::debug};
  static constexpr std::string_view name{"download_type_selected"};

  std::string api_path;
  std::string dest_path;
  std::string function_name;
  download_type type{};

  [[nodiscard]] auto get_event_level() const -> event_level override {
    return level;
  }

  [[nodiscard]] auto get_name() const -> std::string_view override {
    return name;
  }

  [[nodiscard]] auto get_single_line() const -> std::string override {
    return fmt::format("{}|func|{}|ap|{}|dp|{}|type|{}", name, function_name,
                       api_path, dest_path, download_type_to_string(type));
  }
};
} // namespace repertory

NLOHMANN_JSON_NAMESPACE_BEGIN
template <> struct adl_serializer<repertory::download_type_selected> {
  static void to_json(json &data,
                      const repertory::download_type_selected &value) {
    data["api_path"] = value.api_path;
    data["dest_path"] = value.dest_path;
    data["function_name"] = value.function_name;
    data["type"] = repertory::download_type_to_string(value.type);
  }

  static void from_json(const json &data,
                        repertory::download_type_selected &value) {
    data.at("api_path").get_to<std::string>(value.api_path);
    data.at("dest_path").get_to<std::string>(value.dest_path);
    data.at("function_name").get_to<std::string>(value.function_name);
    value.type = repertory::download_type_from_string(
        data.at("type").get<std::string>());
  }
};
NLOHMANN_JSON_NAMESPACE_END

#endif // REPERTORY_INCLUDE_EVENTS_TYPES_DOWNLOAD_TYPE_SELECTED_HPP_
