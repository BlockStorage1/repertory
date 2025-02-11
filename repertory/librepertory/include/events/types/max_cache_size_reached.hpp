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
#ifndef REPERTORY_INCLUDE_EVENTS_TYPES_MAX_CACHE_SIZE_REACHED_HPP_
#define REPERTORY_INCLUDE_EVENTS_TYPES_MAX_CACHE_SIZE_REACHED_HPP_

#include "events/i_event.hpp"
#include "types/repertory.hpp"

namespace repertory {
struct max_cache_size_reached final : public i_event {
  max_cache_size_reached() = default;
  max_cache_size_reached(std::uint64_t cache_size_,
                         std::string_view function_name_,
                         std::uint64_t max_cache_size_)
      : cache_size(cache_size_),
        function_name(std::string{function_name_}),
        max_cache_size(max_cache_size_) {}

  static constexpr const event_level level{event_level::warn};
  static constexpr const std::string_view name{"max_cache_size_reached"};

  std::uint64_t cache_size{};
  std::string function_name;
  std::uint64_t max_cache_size{};

  [[nodiscard]] auto get_event_level() const -> event_level override {
    return level;
  }

  [[nodiscard]] auto get_name() const -> std::string_view override {
    return name;
  }

  [[nodiscard]] auto get_single_line() const -> std::string override {
    return fmt::format("{}|func|{}|size|{}|max|{}", name, function_name,
                       cache_size, max_cache_size);
  }
};
} // namespace repertory

NLOHMANN_JSON_NAMESPACE_BEGIN
template <> struct adl_serializer<repertory::max_cache_size_reached> {
  static void to_json(json &data,
                      const repertory::max_cache_size_reached &value) {
    data["cache_size"] = value.cache_size;
    data["function_name"] = value.function_name;
    data["max_cache_size"] = value.max_cache_size;
  }

  static void from_json(const json &data,
                        repertory::max_cache_size_reached &value) {
    data.at("cache_size").get_to<std::uint64_t>(value.cache_size);
    data.at("function_name").get_to<std::string>(value.function_name);
    data.at("max_cache_size").get_to<std::uint64_t>(value.max_cache_size);
  }
};
NLOHMANN_JSON_NAMESPACE_END

#endif // REPERTORY_INCLUDE_EVENTS_TYPES_MAX_CACHE_SIZE_REACHED_HPP_
