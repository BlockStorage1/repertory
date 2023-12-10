/*
  Copyright <2018-2023> <scott.e.graves@protonmail.com>

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
#ifndef INCLUDE_UTILS_UTILS_HPP_
#define INCLUDE_UTILS_UTILS_HPP_

#include "types/remote.hpp"
#include "types/repertory.hpp"
#include "utils/unix/unix_utils.hpp"
#include "utils/windows/windows_utils.hpp"

namespace repertory::utils {
void calculate_allocation_size(bool directory, std::uint64_t file_size,
                               UINT64 allocation_size,
                               std::string &allocation_meta_size);

[[nodiscard]] auto calculate_read_size(const uint64_t &total_size,
                                       std::size_t read_size,
                                       const uint64_t &offset) -> std::size_t;

template <typename t>
[[nodiscard]] auto collection_excludes(t collection,
                                       const typename t::value_type &val)
    -> bool;

template <typename t>
[[nodiscard]] auto collection_includes(t collection,
                                       const typename t::value_type &val)
    -> bool;

[[nodiscard]] auto compare_version_strings(std::string version1,
                                           std::string version2) -> int;

[[nodiscard]] auto convert_api_date(const std::string &date) -> std::uint64_t;

[[nodiscard]] auto create_curl() -> CURL *;

[[nodiscard]] auto create_uuid_string() -> std::string;

[[nodiscard]] auto create_volume_label(const provider_type &prov)
    -> std::string;

template <typename t>
[[nodiscard]] auto divide_with_ceiling(const t &n, const t &d) -> t;

[[nodiscard]] auto download_type_from_string(std::string type,
                                             const download_type &default_type)
    -> download_type;

[[nodiscard]] auto download_type_to_string(const download_type &type)
    -> std::string;

template <typename t>
[[nodiscard]] auto from_hex_string(const std::string &str, t &val) -> bool;

[[nodiscard]] auto generate_random_string(std::uint16_t length) -> std::string;

[[nodiscard]] auto get_attributes_from_meta(const api_meta_map &meta) -> DWORD;

[[nodiscard]] auto get_environment_variable(const std::string &variable)
    -> std::string;

[[nodiscard]] auto get_file_time_now() -> std::uint64_t;

void get_local_time_now(struct tm &local_time);

[[nodiscard]] auto get_next_available_port(std::uint16_t first_port,
                                           std::uint16_t &available_port)
    -> bool;

[[nodiscard]] auto get_time_now() -> std::uint64_t;

template <typename data_type>
[[nodiscard]] auto random_between(const data_type &begin, const data_type &end)
    -> data_type;

template <typename t>
void remove_element_from(t &collection, const typename t::value_type &val);

[[nodiscard]] auto reset_curl(CURL *curl_handle) -> CURL *;

[[nodiscard]] auto retryable_action(const std::function<bool()> &action)
    -> bool;

void spin_wait_for_mutex(std::function<bool()> complete,
                         std::condition_variable &cond, std::mutex &mtx,
                         const std::string &text = "");

void spin_wait_for_mutex(bool &complete, std::condition_variable &cond,
                         std::mutex &mtx, const std::string &text = "");

template <typename collection_t>
[[nodiscard]] auto to_hex_string(const collection_t &collection) -> std::string;

// template implementations
template <typename t>
[[nodiscard]] auto collection_excludes(t collection,
                                       const typename t::value_type &val)
    -> bool {
  return std::find(collection.begin(), collection.end(), val) ==
         collection.end();
}

template <typename t>
[[nodiscard]] auto collection_includes(t collection,
                                       const typename t::value_type &val)
    -> bool {
  return std::find(collection.begin(), collection.end(), val) !=
         collection.end();
}

template <typename t>
[[nodiscard]] auto divide_with_ceiling(const t &n, const t &d) -> t {
  return n ? (n / d) + (n % d != 0) : 0;
}

template <typename t>
[[nodiscard]] auto from_hex_string(const std::string &str, t &val) -> bool {
  static constexpr const auto base16 = 16;

  val.clear();
  if (not(str.length() % 2U)) {
    for (std::size_t i = 0U; i < str.length(); i += 2U) {
      val.emplace_back(static_cast<typename t::value_type>(
          strtol(str.substr(i, 2U).c_str(), nullptr, base16)));
    }
    return true;
  }

  return false;
}

template <typename data_type>
[[nodiscard]] auto random_between(const data_type &begin, const data_type &end)
    -> data_type {
  return begin + repertory_rand<data_type>() % ((end + data_type{1}) - begin);
}

template <typename collection_t>
void remove_element_from(collection_t &collection,
                         const typename collection_t::value_type &value) {
  collection.erase(std::remove(collection.begin(), collection.end(), value),
                   collection.end());
}

template <typename collection_t>
[[nodiscard]] auto to_hex_string(const collection_t &collection)
    -> std::string {
  static_assert(sizeof(typename collection_t::value_type) == 1U,
                "value_type must be 1 byte in size");
  static constexpr const auto mask = 0xFF;

  std::stringstream stream;
  for (const auto &val : collection) {
    stream << std::setfill('0') << std::setw(2) << std::hex
           << (static_cast<std::uint32_t>(val) & mask);
  }

  return stream.str();
}
} // namespace repertory::utils

#endif // INCLUDE_UTILS_UTILS_HPP_
