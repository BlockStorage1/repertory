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
      ((allocation_size == 0u) ? WINFSP_ALLOCATION_UNIT : allocation_size);
  allocation_size =
      utils::divide_with_ceiling(allocation_size, WINFSP_ALLOCATION_UNIT) *
      WINFSP_ALLOCATION_UNIT;
  allocation_meta_size = std::to_string(allocation_size);
}

auto calculate_read_size(const uint64_t &total_size, std::size_t read_size,
                         const uint64_t &offset) -> std::size_t {
  return static_cast<std::size_t>(
      ((offset + read_size) > total_size)
          ? ((offset < total_size) ? total_size - offset : 0u)
          : read_size);
}

auto compare_version_strings(std::string version1, std::string version2)
    -> int {
  if (utils::string::contains(version1, "-")) {
    version1 = utils::string::split(version1, '-')[0u];
  }

  if (utils::string::contains(version2, "-")) {
    version2 = utils::string::split(version2, '-')[0u];
  }

  auto nums1 = utils::string::split(version1, '.');
  auto nums2 = utils::string::split(version2, '.');

  while (nums1.size() > nums2.size()) {
    nums2.emplace_back("0");
  }

  while (nums2.size() > nums1.size()) {
    nums1.emplace_back("0");
  }

  for (std::size_t i = 0u; i < nums1.size(); i++) {
    const auto int1 = utils::string::to_uint32(nums1[i]);
    const auto int2 = utils::string::to_uint32(nums2[i]);
    const auto res = std::memcmp(&int1, &int2, sizeof(int1));
    if (res) {
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
  return nanos + (mktime(&tm1) * NANOS_PER_SECOND);
}

auto create_curl() -> CURL * {
  static std::recursive_mutex mtx;

  unique_recur_mutex_lock l(mtx);
  curl_global_init(CURL_GLOBAL_DEFAULT);
  l.unlock();

  return reset_curl(curl_easy_init());
}

auto create_uuid_string() -> std::string {
#ifdef _WIN32
  UUID guid{};
  UuidCreate(&guid);

  unsigned char *s;
  UuidToStringA(&guid, &s);

  std::string ret(reinterpret_cast<char *>(s));
  RpcStringFreeA(&s);

  return ret;
#else
#if __linux__
  uuid id;
  id.make(UUID_MAKE_V4);
  return id.string();
#else
  uuid_t guid;
  uuid_generate_random(guid);

  std::string ret;
  ret.resize(37);
  uuid_unparse(guid, &ret[0]);

  return ret.c_str();
#endif
#endif
}

auto create_volume_label(const provider_type &pt) -> std::string {
  return "repertory_" + app_config::get_provider_name(pt);
}

auto download_type_from_string(std::string type,
                               const download_type &default_type)
    -> download_type {
  type = utils::string::to_lower(utils::string::trim(type));
  if (type == "direct") {
    return download_type::direct;
  } else if (type == "fallback") {
    return download_type::fallback;
  } else if (type == "ring_buffer") {
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
  date.QuadPart -= 116444736000000000ull;

  return date.QuadPart * 100ull;
}

void unix_time_to_filetime(const remote::file_time &ts, FILETIME &ft) {
  const auto winTime = (ts / 100ull) + 116444736000000000ull;
  ft.dwHighDateTime = winTime >> 32u;
  ft.dwLowDateTime = winTime & 0xFFFFFFFF;
}
#endif

auto generate_random_string(std::uint16_t length) -> std::string {
  srand(static_cast<unsigned int>(get_time_now()));

  std::string ret;
  ret.resize(length);
  for (std::uint16_t i = 0u; i < length; i++) {
    do {
      ret[i] = static_cast<char>(rand() % 74 + 48);
    } while (((ret[i] >= 91) && (ret[i] <= 96)) ||
             ((ret[i] >= 58) && (ret[i] <= 64)));
  }

  return ret;
}

auto get_attributes_from_meta(const api_meta_map &meta) -> DWORD {
  return static_cast<DWORD>(utils::string::to_uint32(meta.at(META_ATTRIBUTES)));
}

auto get_environment_variable(const std::string &variable) -> std::string {
#ifdef _WIN32
  std::string value;
  auto sz = ::GetEnvironmentVariable(&variable[0], nullptr, 0);
  if (sz > 0) {
    value.resize(sz);
    ::GetEnvironmentVariable(&variable[0], &value[0], sz);
  }

  return value.c_str();
#else
  const auto *v = getenv(variable.c_str());
  return std::string(v ? v : "");
#endif
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
  const auto *tmp = std::localtime(&now);
  if (tmp) {
    memcpy(&local_time, tmp, sizeof(local_time));
  }
#endif
}

auto get_next_available_port(std::uint16_t first_port,
                             std::uint16_t &available_port) -> bool {
  using namespace boost::asio;
  using ip::tcp;
  boost::system::error_code ec;
  do {
    io_service svc;
    tcp::acceptor a(svc);
    a.open(tcp::v4(), ec) || a.bind({tcp::v4(), first_port}, ec);
  } while (ec && (first_port++ < 65535u));

  if (not ec) {
    available_port = first_port;
  }

  return not ec;
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
  auto succeeded = false;
  for (std::uint8_t i = 0u; not(succeeded = action()) && (i < 20u); i++) {
    std::this_thread::sleep_for(100ms);
  }
  return succeeded;
}

void spin_wait_for_mutex(std::function<bool()> complete,
                         std::condition_variable &cv, std::mutex &mtx,
                         const std::string &text) {
  while (not complete()) {
    unique_mutex_lock l(mtx);
    if (not complete()) {
      if (not text.empty()) {
        /* event_system::instance().raise<DebugLog>(__FUNCTION__,
         * "spin_wait_for_mutex", text); */
      }
      cv.wait_for(l, 1s);
    }
    l.unlock();
  }
}

void spin_wait_for_mutex(bool &complete, std::condition_variable &cv,
                         std::mutex &mtx, const std::string &text) {
  while (not complete) {
    unique_mutex_lock l(mtx);
    if (not complete) {
      if (not text.empty()) {
        /* event_system::instance().raise<DebugLog>(__FUNCTION__,
         * "spin_wait_for_mutex", text); */
      }
      cv.wait_for(l, 1s);
    }
    l.unlock();
  }
}
} // namespace repertory::utils
