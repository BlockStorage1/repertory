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
#ifndef REPERTORY_INCLUDE_EVENTS_TYPES_FILE_UPLOAD_REMOVED_HPP_
#define REPERTORY_INCLUDE_EVENTS_TYPES_FILE_UPLOAD_REMOVED_HPP_

#include "events/i_event.hpp"
#include "types/repertory.hpp"

namespace repertory {
struct file_upload_removed final : public i_event {
  file_upload_removed() = default;
  file_upload_removed(std::string api_path_, std::string_view function_name_)
      : api_path(std::move(api_path_)),
        function_name(std::string(function_name_)) {}

  static constexpr event_level level{event_level::debug};
  static constexpr std::string_view name{"file_upload_removed"};

  std::string api_path;
  std::string function_name;

  [[nodiscard]] auto get_event_level() const -> event_level override {
    return level;
  }

  [[nodiscard]] auto get_name() const -> std::string_view override {
    return name;
  }

  [[nodiscard]] auto get_single_line() const -> std::string override {
    return fmt::format("{}|func|{}|ap|{}", name, function_name, api_path);
  }
};
} // namespace repertory

NLOHMANN_JSON_NAMESPACE_BEGIN
template <> struct adl_serializer<repertory::file_upload_removed> {
  static void to_json(json &data, const repertory::file_upload_removed &value) {
    data["api_path"] = value.api_path;
    data["function_name"] = value.function_name;
  }

  static void from_json(const json &data,
                        repertory::file_upload_removed &value) {
    data.at("api_path").get_to<std::string>(value.api_path);
    data.at("function_name").get_to<std::string>(value.function_name);
  }
};
NLOHMANN_JSON_NAMESPACE_END

#endif // REPERTORY_INCLUDE_EVENTS_TYPES_FILE_UPLOAD_REMOVED_HPP_
