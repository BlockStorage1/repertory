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
#include "utils/utils.hpp"

#include "app_config.hpp"
#include "events/event_system.hpp"
#include "events/events.hpp"
#include "providers/i_provider.hpp"
#include "types/startup_exception.hpp"
#include "utils/com_init_wrapper.hpp"
#include "utils/native_file.hpp"
#include "utils/path_utils.hpp"
#include "utils/string_utils.hpp"

namespace repertory::utils {
void calculate_allocation_size(bool directory, std::uint64_t file_size,
                               UINT64 allocation_size,
                               std::string &allocation_meta_size) {
  if (directory) {
    allocation_meta_size = "0";
    return;
  }

  if (file_size > allocation_size) {
    allocation_size = file_size;
  }
  allocation_size =
      ((allocation_size == 0U) ? WINFSP_ALLOCATION_UNIT : allocation_size);
  allocation_size =
      utils::divide_with_ceiling(allocation_size, WINFSP_ALLOCATION_UNIT) *
      WINFSP_ALLOCATION_UNIT;
  allocation_meta_size = std::to_string(allocation_size);
}

auto calculate_read_size(const uint64_t &total_size, std::size_t read_size,
                         const uint64_t &offset) -> std::size_t {
  return static_cast<std::size_t>(
      ((offset + read_size) > total_size)
          ? ((offset < total_size) ? total_size - offset : 0U)
          : read_size);
}

auto compare_version_strings(std::string version1, std::string version2)
    -> int {
  if (utils::string::contains(version1, "-")) {
    version1 = utils::string::split(version1, '-')[0U];
  }

  if (utils::string::contains(version2, "-")) {
    version2 = utils::string::split(version2, '-')[0U];
  }

  auto nums1 = utils::string::split(version1, '.');
  auto nums2 = utils::string::split(version2, '.');

  while (nums1.size() > nums2.size()) {
    nums2.emplace_back("0");
  }

  while (nums2.size() > nums1.size()) {
    nums1.emplace_back("0");
  }

  for (std::size_t i = 0U; i < nums1.size(); i++) {
    const auto int1 = utils::string::to_uint32(nums1[i]);
    const auto int2 = utils::string::to_uint32(nums2[i]);
    const auto res = std::memcmp(&int1, &int2, sizeof(int1));
    if (res != 0) {
      return res;
    }
  }

  return 0;
}

auto convert_api_date(const std::string &date) -> std::uint64_t {
  // 2009-10-12T17:50:30.000Z
  const auto date_parts = utils::string::split(date, '.');
  const auto date_time = date_parts[0U];
  const auto nanos =
      utils::string::to_uint64(utils::string::split(date_parts[1U], 'Z')[0U]);

  struct tm tm1 {};
#ifdef _WIN32
  utils::strptime(date_time.c_str(), "%Y-%m-%dT%T", &tm1);
#else
  strptime(date_time.c_str(), "%Y-%m-%dT%T", &tm1);
#endif
  return nanos + (static_cast<std::uint64_t>(mktime(&tm1)) * NANOS_PER_SECOND);
}

auto create_curl() -> CURL * {
  static std::recursive_mutex mtx;

  unique_recur_mutex_lock lock(mtx);
  curl_global_init(CURL_GLOBAL_DEFAULT);
  lock.unlock();

  return reset_curl(curl_easy_init());
}

auto create_uuid_string() -> std::string {
  std::random_device random_device;
  auto seed_data = std::array<int, std::mt19937::state_size>{};
  std::generate(std::begin(seed_data), std::end(seed_data),
                std::ref(random_device));
  std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
  std::mt19937 generator(seq);
  uuids::uuid_random_generator gen{generator};

  return uuids::to_string(gen());
}

auto create_volume_label(const provider_type &prov) -> std::string {
  return "repertory_" + app_config::get_provider_name(prov);
}

auto download_type_from_string(std::string type,
                               const download_type &default_type)
    -> download_type {
  type = utils::string::to_lower(utils::string::trim(type));
  if (type == "direct") {
    return download_type::direct;
  }

  if (type == "fallback") {
    return download_type::fallback;
  }

  if (type == "ring_buffer") {
    return download_type::ring_buffer;
  }

  return default_type;
}

auto download_type_to_string(const download_type &type) -> std::string {
  switch (type) {
  case download_type::direct:
    return "direct";
  case download_type::fallback:
    return "fallback";
  case download_type::ring_buffer:
    return "ring_buffer";
  default:
    return "fallback";
  }
}

#ifdef _WIN32
// https://www.frenk.com/2009/12/convert-filetime-to-unix-timestamp/
auto filetime_to_unix_time(const FILETIME &ft) -> remote::file_time {
  LARGE_INTEGER date{};
  date.HighPart = ft.dwHighDateTime;
  date.LowPart = ft.dwLowDateTime;
  date.QuadPart -= 116444736000000000ULL;

  return date.QuadPart * 100ULL;
}

void unix_time_to_filetime(const remote::file_time &ts, FILETIME &ft) {
  const auto win_time = (ts / 100ULL) + 116444736000000000ULL;
  ft.dwHighDateTime = win_time >> 32U;
  ft.dwLowDateTime = win_time & 0xFFFFFFFF;
}
#endif

auto generate_random_string(std::uint16_t length) -> std::string {
  std::string ret;
  ret.resize(length);
  for (std::uint16_t i = 0U; i < length; i++) {
    do {
      ret[i] = static_cast<char>(repertory_rand<std::uint8_t>() % 74 + 48);
    } while (((ret[i] >= 91) && (ret[i] <= 96)) ||
             ((ret[i] >= 58) && (ret[i] <= 64)));
  }

  return ret;
}

auto get_attributes_from_meta(const api_meta_map &meta) -> DWORD {
  return static_cast<DWORD>(utils::string::to_uint32(meta.at(META_ATTRIBUTES)));
}

auto get_environment_variable(const std::string &variable) -> std::string {
  static std::mutex mtx{};
  mutex_lock lock{mtx};

  const auto *val = std::getenv(variable.c_str());
  auto ret = std::string(val == nullptr ? "" : val);
  return ret;
}

auto get_file_time_now() -> std::uint64_t {
#ifdef _WIN32
  SYSTEMTIME st{};
  ::GetSystemTime(&st);
  FILETIME ft{};
  ::SystemTimeToFileTime(&st, &ft);
  return static_cast<std::uint64_t>(((LARGE_INTEGER *)&ft)->QuadPart);
#else
  return get_time_now();
#endif
}

void get_local_time_now(struct tm &local_time) {
  memset(&local_time, 0, sizeof(local_time));

  const auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
#ifdef _WIN32
  localtime_s(&local_time, &now);
#else
  static std::mutex mtx{};
  mutex_lock lock{mtx};

  const auto *tmp = std::localtime(&now);
  if (tmp != nullptr) {
    memcpy(&local_time, tmp, sizeof(local_time));
  }
#endif
}

auto get_next_available_port(std::uint16_t first_port,
                             std::uint16_t &available_port) -> bool {
  using namespace boost::asio;
  using ip::tcp;

  boost::system::error_code error_code{};
  while (first_port != 0U) {
    io_service svc{};
    tcp::acceptor acceptor(svc);
    acceptor.open(tcp::v4(), error_code) ||
        acceptor.bind({tcp::v4(), first_port}, error_code);
    if (not error_code) {
      break;
    }

    ++first_port;
  }

  if (not error_code) {
    available_port = first_port;
  }

  return not error_code;
}

auto get_time_now() -> std::uint64_t {
#ifdef _WIN32
  return static_cast<std::uint64_t>(
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
#else
#if __APPLE__
  return std::chrono::nanoseconds(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
#else
  return static_cast<std::uint64_t>(
      std::chrono::nanoseconds(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count());
#endif
#endif
}

auto reset_curl(CURL *curl_handle) -> CURL * {
  curl_easy_reset(curl_handle);
#if __APPLE__
  curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
#endif
  return curl_handle;
}

auto retryable_action(const std::function<bool()> &action) -> bool {
  static constexpr const auto retry_count = 20U;

  auto succeeded = false;
  for (std::uint8_t i = 0U; not(succeeded = action()) && (i < retry_count);
       i++) {
    std::this_thread::sleep_for(100ms);
  }
  return succeeded;
}

void spin_wait_for_mutex(std::function<bool()> complete,
                         std::condition_variable &cond, std::mutex &mtx,
                         const std::string &text) {
  while (not complete()) {
    unique_mutex_lock lock(mtx);
    if (not complete()) {
      if (not text.empty()) {
        /* event_system::instance().raise<DebugLog>(__FUNCTION__,
         * "spin_wait_for_mutex", text); */
      }
      cond.wait_for(lock, 1s);
    }
    lock.unlock();
  }
}

void spin_wait_for_mutex(bool &complete, std::condition_variable &cond,
                         std::mutex &mtx, const std::string &text) {
  while (not complete) {
    unique_mutex_lock lock(mtx);
    if (not complete) {
      if (not text.empty()) {
        /* event_system::instance().raise<DebugLog>(__FUNCTION__,
         * "spin_wait_for_mutex", text); */
      }
      cond.wait_for(lock, 1s);
    }
    lock.unlock();
  }
}
} // namespace repertory::utils
