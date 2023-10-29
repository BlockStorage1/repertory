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
                                       const typename t::value_type &v) -> bool;

template <typename t>
[[nodiscard]] auto collection_includes(t collection,
                                       const typename t::value_type &v) -> bool;

[[nodiscard]] auto compare_version_strings(std::string version1,
                                           std::string version2) -> int;

[[nodiscard]] auto convert_api_date(const std::string &date) -> std::uint64_t;

[[nodiscard]] auto create_curl() -> CURL *;

[[nodiscard]] auto create_uuid_string() -> std::string;

[[nodiscard]] auto create_volume_label(const provider_type &pt) -> std::string;

template <typename t>
[[nodiscard]] auto divide_with_ceiling(const t &n, const t &d) -> t;

[[nodiscard]] auto download_type_from_string(std::string type,
                                             const download_type &default_type)
    -> download_type;

[[nodiscard]] auto download_type_to_string(const download_type &type)
    -> std::string;

template <typename t>
[[nodiscard]] auto from_hex_string(const std::string &str, t &v) -> bool;

[[nodiscard]] auto generate_random_string(std::uint16_t length) -> std::string;

[[nodiscard]] auto get_attributes_from_meta(const api_meta_map &meta) -> DWORD;

[[nodiscard]] auto get_environment_variable(const std::string &variable)
    -> std::string;

[[nodiscard]] auto get_file_time_now() -> std::uint64_t;

void get_local_time_now(struct tm &localTime);

[[nodiscard]] auto get_next_available_port(std::uint16_t first_port,
                                           std::uint16_t &available_port)
    -> bool;

[[nodiscard]] auto get_time_now() -> std::uint64_t;

template <typename t>
[[nodiscard]] auto random_between(const t &begin, const t &end) -> t;

template <typename t>
void remove_element_from(t &v, const typename t::value_type &value);

[[nodiscard]] auto reset_curl(CURL *curl_handle) -> CURL *;

[[nodiscard]] auto retryable_action(const std::function<bool()> &action)
    -> bool;

void spin_wait_for_mutex(std::function<bool()> complete,
                         std::condition_variable &cv, std::mutex &mtx,
                         const std::string &txt = "");

void spin_wait_for_mutex(bool &complete, std::condition_variable &cv,
                         std::mutex &mtx, const std::string &txt = "");

template <typename t>
[[nodiscard]] auto to_hex_string(const t &v) -> std::string;

// template implementations
template <typename t>
[[nodiscard]] auto collection_excludes(t collection,
                                       const typename t::value_type &v)
    -> bool {
  return std::find(collection.begin(), collection.end(), v) == collection.end();
}

template <typename t>
[[nodiscard]] auto collection_includes(t collection,
                                       const typename t::value_type &v)
    -> bool {
  return std::find(collection.begin(), collection.end(), v) != collection.end();
}

template <typename t>
[[nodiscard]] auto divide_with_ceiling(const t &n, const t &d) -> t {
  return n ? (n / d) + (n % d != 0) : 0;
}

template <typename t>
[[nodiscard]] auto from_hex_string(const std::string &str, t &v) -> bool {
  v.clear();
  if (not(str.length() % 2u)) {
    for (std::size_t i = 0u; i < str.length(); i += 2u) {
      v.emplace_back(static_cast<typename t::value_type>(
          strtol(str.substr(i, 2u).c_str(), nullptr, 16)));
    }
    return true;
  }

  return false;
}

template <typename t>
[[nodiscard]] auto random_between(const t &begin, const t &end) -> t {
  srand(static_cast<unsigned int>(get_time_now()));
  return begin + rand() % ((end + 1) - begin);
}

template <typename t>
void remove_element_from(t &v, const typename t::value_type &value) {
  v.erase(std::remove(v.begin(), v.end(), value), v.end());
}

template <typename t>
[[nodiscard]] auto to_hex_string(const t &value) -> std::string {
  std::string ret{};

  std::array<char, 3> h{};
  for (const auto &num : value) {
#ifdef _WIN32
    sprintf_s(h.data(), h.size() - 1U, "%x", static_cast<std::uint8_t>(num));
#else
    sprintf(h.data(), "%x", static_cast<std::uint8_t>(num));
#endif

    ret += (strlen(h.data()) == 1) ? std::string("0") + h.data() : h.data();
  }

  return ret;
}
} // namespace repertory::utils

#endif // INCLUDE_UTILS_UTILS_HPP_
