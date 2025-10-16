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
#ifndef REPERTORY_INCLUDE_UTILS_COMMON_HPP_
#define REPERTORY_INCLUDE_UTILS_COMMON_HPP_

#include "utils/config.hpp"

namespace repertory::utils {
struct result final {
  std::string function_name;
  bool ok{true};
  std::string reason{"success"};

  [[nodiscard]] operator bool() const { return ok; }
};

using retryable_action_t = std::function<bool()>;

[[nodiscard]] constexpr auto calculate_read_size(std::uint64_t total_size,
                                                 std::size_t read_size,
                                                 std::uint64_t offset)
    -> std::size_t {
  return static_cast<std::size_t>(
      ((offset + read_size) > total_size)
          ? ((offset < total_size) ? total_size - offset : 0U)
          : read_size);
}

[[nodiscard]] auto compare_version_strings(std::string version1,
                                           std::string version2)
    -> std::int32_t;

[[nodiscard]] auto compare_version_strings(std::wstring_view version1,
                                           std::wstring_view version2)
    -> std::int32_t;

#if defined(PROJECT_ENABLE_STDUUID)
[[nodiscard]] auto create_uuid_string() -> std::string;

[[nodiscard]] auto create_uuid_wstring() -> std::wstring;
#endif // defined(PROJECT_ENABLE_STDUUID)

template <typename result_t, typename data_t>
[[nodiscard]] inline constexpr auto divide_with_ceiling(result_t numerator,
                                                        data_t denominator)
    -> result_t;

template <typename data_t>
[[nodiscard]] inline auto generate_random_between(data_t begin, data_t end)
    -> data_t;

[[nodiscard]] auto generate_random_string(std::size_t length) -> std::string;

[[nodiscard]] auto generate_random_wstring(std::size_t length) -> std::wstring;

#if defined(PROJECT_ENABLE_LIBSODIUM)
template <typename data_t>
[[nodiscard]] inline auto generate_secure_random() -> data_t;

template <typename data_t>
[[nodiscard]] inline auto generate_secure_random(std::size_t size) -> data_t;
#endif // defined(PROJECT_ENABLE_LIBSODIUM)

[[nodiscard]] auto get_environment_variable(std::string_view variable)
    -> std::string;

[[nodiscard]] auto get_environment_variable(std::wstring_view variable)
    -> std::wstring;

#if defined(PROJECT_ENABLE_BOOST)
[[nodiscard]] auto get_next_available_port(std::uint16_t first_port,
                                           std::uint16_t &available_port)
    -> bool;
#endif // defined(PROJECT_ENABLE_BOOST)

[[nodiscard]] auto retry_action(
    retryable_action_t action, std::size_t retry_count = 200U,
    std::chrono::milliseconds retry_wait = std::chrono::milliseconds(10))
    -> bool;

template <typename result_t, typename data_t>
inline constexpr auto divide_with_ceiling(result_t numerator,
                                          data_t denominator) -> result_t {
  static_assert(std::is_integral_v<std::remove_cv_t<data_t>>,
                "denominator must be an integral type");

  return denominator == 0
             ? 0
             : (numerator / denominator) + (numerator % denominator != 0);
}

template <typename data_t>
inline auto generate_random_between(data_t begin, data_t end) -> data_t {
  static_assert(std::is_integral_v<std::remove_cv_t<data_t>>,
                "data_t must be an integral type");
  if (end <= begin) {
    throw std::range_error("end must be greater than begin");
  }

  thread_local std::mt19937 gen(
      static_cast<unsigned long>(std::time(nullptr) ^ std::random_device{}()));
  std::uniform_int_distribution<data_t> dis(begin, end);
  return dis(gen);
}

#if defined(PROJECT_ENABLE_LIBSODIUM)
template <typename data_t> inline auto generate_secure_random() -> data_t {
  static_assert(!is_collection<std::decay_t<data_t>>::value,
                "data_t is a vector or collection");
  data_t ret{};
  randombytes_buf(&ret, sizeof(ret));
  return ret;
}

template <typename data_t>
inline auto generate_secure_random(std::size_t size) -> data_t {
  static_assert(is_collection<std::decay_t<data_t>>::value,
                "data_t is not a vector or collection");
  data_t ret;
  ret.resize(size);
  randombytes_buf(ret.data(), ret.size() * sizeof(typename data_t::value_type));
  return ret;
}
#endif // defined(PROJECT_ENABLE_LIBSODIUM)
} // namespace repertory::utils

#endif // REPERTORY_INCLUDE_UTILS_COMMON_HPP_
