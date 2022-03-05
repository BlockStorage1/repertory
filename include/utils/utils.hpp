/*
  Copyright <2018-2022> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute,
  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
  OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef INCLUDE_UTILS_UTILS_HPP_
#define INCLUDE_UTILS_UTILS_HPP_

#include "common.hpp"
#include "types/remote.hpp"
#include "types/repertory.hpp"
#include "utils/unix/unix_utils.hpp"
#include "utils/windows/windows_utils.hpp"

namespace repertory::utils {
hastings api_currency_to_hastings(const api_currency &currency);

void calculate_allocation_size(const bool &directory, const std::uint64_t &file_size,
                               UINT64 allocation_size, std::string &allocation_meta_size);

std::size_t calculate_read_size(const uint64_t &total_size, const std::size_t &read_size,
                                const uint64_t &offset);

template <typename t> bool collection_excludes(t collection, const typename t::value_type &v);

template <typename t> bool collection_includes(t collection, const typename t::value_type &v);

int compare_version_strings(std::string version1, std::string version2);

std::uint64_t convert_api_date(const std::string &date);

CURL *create_curl();

std::string create_uuid_string();

std::string create_volume_label(const provider_type &pt);

template <typename t> t divide_with_ceiling(const t &n, const t &d);

download_type download_type_from_string(std::string type, const download_type &default_type);

std::string download_type_to_string(const download_type &type);

template <typename t> bool from_hex_string(const std::string &str, t &v);

std::string generate_random_string(const std::uint16_t &length);

std::string get_environment_variable(const std::string &variable);

std::uint64_t get_file_time_now();

void get_local_time_now(struct tm &localTime);

bool get_next_available_port(std::uint16_t first_port, std::uint16_t &available_port);

std::uint64_t get_time_now();

api_currency hastings_string_to_api_currency(const std::string &amount);

// bool parse_url(const std::string &url, HostConfig &hc);

template <typename t> t random_between(const t &begin, const t &end);

template <typename t> void remove_element_from(t &v, const typename t::value_type &value);

CURL *reset_curl(CURL *curl_handle);

bool retryable_action(const std::function<bool()> &action);

void spin_wait_for_mutex(std::function<bool()> complete, std::condition_variable &cv,
                         std::mutex &mtx, const std::string &txt = "");

void spin_wait_for_mutex(bool &complete, std::condition_variable &cv, std::mutex &mtx,
                         const std::string &txt = "");

template <typename t> std::string to_hex_string(const t &v);

// template implementations
template <typename t> bool collection_excludes(t collection, const typename t::value_type &v) {
  return std::find(collection.begin(), collection.end(), v) == collection.end();
}

template <typename t> bool collection_includes(t collection, const typename t::value_type &v) {
  return std::find(collection.begin(), collection.end(), v) != collection.end();
}

template <typename t> t divide_with_ceiling(const t &n, const t &d) {
  return n ? (n / d) + (n % d != 0) : 0;
}

template <typename t> bool from_hex_string(const std::string &str, t &v) {
  v.clear();
  if (not(str.length() % 2u)) {
    for (std::size_t i = 0u; i < str.length(); i += 2u) {
      v.emplace_back(
          static_cast<typename t::value_type>(strtol(str.substr(i, 2u).c_str(), nullptr, 16)));
    }
    return true;
  }

  return false;
}

template <typename t> t random_between(const t &begin, const t &end) {
  srand(static_cast<unsigned int>(get_time_now()));
  return begin + rand() % ((end + 1) - begin);
}

template <typename t> void remove_element_from(t &v, const typename t::value_type &value) {
  v.erase(std::remove(v.begin(), v.end(), value), v.end());
}

template <typename t> std::string to_hex_string(const t &v) {
  std::string ret;

  for (const auto &v : v) {
    char h[3] = {0};
#ifdef _WIN32
    sprintf_s(&h[0], sizeof(h), "%x", static_cast<std::uint8_t>(v));
#else
    sprintf(&h[0], "%x", static_cast<std::uint8_t>(v));
#endif
    ret += ((strlen(h) == 1) ? std::string("0") + h : h);
  }

  return ret;
}
} // namespace repertory::utils

#endif // INCLUDE_UTILS_UTILS_HPP_
