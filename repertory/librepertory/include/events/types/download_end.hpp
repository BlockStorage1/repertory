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
#ifndef REPERTORY_INCLUDE_EVENTS_TYPES_DOWNLOAD_END_HPP_
#define REPERTORY_INCLUDE_EVENTS_TYPES_DOWNLOAD_END_HPP_

#include "events/i_event.hpp"
#include "types/repertory.hpp"

namespace repertory {
struct download_end final : public i_event {
  download_end() = default;
  download_end(std::string api_path_, std::string dest_path_, api_error error_,
               std::string_view function_name_)
      : api_path(std::move(api_path_)),
        dest_path(std::move(dest_path_)),
        error(error_),
        function_name(std::string(function_name_)) {}

  static constexpr event_level level{event_level::info};
  static constexpr std::string_view name{"download_end"};

  std::string api_path;
  std::string dest_path;
  api_error error{};
  std::string function_name;

  [[nodiscard]] auto get_event_level() const -> event_level override {
    return level;
  }

  [[nodiscard]] auto get_name() const -> std::string_view override {
    return name;
  }

  [[nodiscard]] auto get_single_line() const -> std::string override {
    return fmt::format("{}|func|{}|ap|{}|dp|{}|error|{}", name, function_name,
                       api_path, dest_path, api_error_to_string(error));
  }
};
} // namespace repertory

NLOHMANN_JSON_NAMESPACE_BEGIN
template <> struct adl_serializer<repertory::download_end> {
  static void to_json(json &data, const repertory::download_end &value) {
    data["api_path"] = value.api_path;
    data["dest_path"] = value.dest_path;
    data["error"] = repertory::api_error_to_string(value.error);
    data["function_name"] = value.function_name;
  }

  static void from_json(const json &data, repertory::download_end &value) {
    data.at("api_path").get_to<std::string>(value.api_path);
    data.at("dest_path").get_to<std::string>(value.dest_path);
    value.error =
        repertory::api_error_from_string(data.at("error").get<std::string>());
    data.at("function_name").get_to<std::string>(value.function_name);
  }
};
NLOHMANN_JSON_NAMESPACE_END

#endif // REPERTORY_INCLUDE_EVENTS_TYPES_DOWNLOAD_END_HPP_
