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
#include "utils/utils.hpp"
#include "app_config.hpp"
#include "events/events.hpp"
#include "events/event_system.hpp"
#include "providers/i_provider.hpp"
#include "types/skynet.hpp"
#include "types/startup_exception.hpp"
#include "utils/com_init_wrapper.hpp"
#include "utils/native_file.hpp"
#include "utils/path_utils.hpp"
#include "utils/string_utils.hpp"

namespace repertory::utils {
hastings api_currency_to_hastings(const api_currency &currency) {
  ttmath::Parser<api_currency> parser;
  parser.Parse(currency.ToString() + " * (10 ^ 24)");
  ttmath::Conv conv;
  conv.scient_from = 256;
  conv.base = 10u;
  conv.round = 0;
  return parser.stack[0u].value.ToString(conv);
}

void calculate_allocation_size(const bool &directory, const std::uint64_t &file_size,
                               UINT64 allocation_size, std::string &allocation_meta_size) {
  if (directory) {
    allocation_meta_size = "0";
    return;
  }

  if (file_size > allocation_size) {
    allocation_size = file_size;
  }
  allocation_size = ((allocation_size == 0u) ? WINFSP_ALLOCATION_UNIT : allocation_size);
  allocation_size =
      utils::divide_with_ceiling(allocation_size, WINFSP_ALLOCATION_UNIT) * WINFSP_ALLOCATION_UNIT;
  allocation_meta_size = std::to_string(allocation_size);
}

std::size_t calculate_read_size(const uint64_t &total_size, const std::size_t &read_size,
                                const uint64_t &offset) {
  return static_cast<std::size_t>(((offset + read_size) > total_size)
                                      ? ((offset < total_size) ? total_size - offset : 0u)
                                      : read_size);
}

int compare_version_strings(std::string version1, std::string version2) {
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

std::uint64_t convert_api_date(const std::string &date) {
  //"2019-02-21T02:24:37.653091916-06:00"
  const auto parts = utils::string::split(date, '.');
  const auto dt = parts[0];
  const auto nanos = utils::string::to_uint64(utils::string::split(parts[1u], '-')[0u]);

  struct tm tm1 {};
#ifdef _WIN32
  auto convert_time = [](time_t t) -> std::uint64_t {
    const auto ll = Int32x32To64(t, 10000000) + 116444736000000000ull;
    ULARGE_INTEGER ft{};
    ft.LowPart = static_cast<DWORD>(ll);
    ft.HighPart = static_cast<DWORD>(ll >> 32);
    return ft.QuadPart;
  };

  const auto parts2 = utils::string::split(dt, 'T');
  const auto date_parts = utils::string::split(parts2[0u], '-');
  const auto time_parts = utils::string::split(parts2[1u], ':');
  tm1.tm_year = utils::string::to_int32(date_parts[0u]) - 1900;
  tm1.tm_mon = utils::string::to_int32(date_parts[1u]) - 1;
  tm1.tm_mday = utils::string::to_int32(date_parts[2u]);
  tm1.tm_hour = utils::string::to_uint32(time_parts[0u]);
  tm1.tm_min = utils::string::to_uint32(time_parts[1u]);
  tm1.tm_sec = utils::string::to_uint32(time_parts[2u]);
  tm1.tm_wday = -1;
  tm1.tm_yday = -1;
  tm1.tm_isdst = -1;
  return (nanos / 100) + convert_time(mktime(&tm1));
#else
  strptime(&dt[0], "%Y-%m-%dT%T", &tm1);
  return nanos + (mktime(&tm1) * NANOS_PER_SECOND);
#endif
}

CURL *create_curl() { return reset_curl(curl_easy_init()); }

std::string create_uuid_string() {
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

std::string create_volume_label(const provider_type &pt) {
  return "repertory_" + app_config::get_provider_name(pt);
}

download_type download_type_from_string(std::string type, const download_type &default_type) {
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

std::string download_type_to_string(const download_type &type) {
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
remote::file_time filetime_to_unix_time(const FILETIME &ft) {
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

std::string generate_random_string(const std::uint16_t &length) {
  srand(static_cast<unsigned int>(get_time_now()));

  std::string ret;
  ret.resize(length);
  for (std::uint16_t i = 0u; i < length; i++) {
    do {
      ret[i] = static_cast<char>(rand() % 74 + 48);
    } while (((ret[i] >= 91) && (ret[i] <= 96)) || ((ret[i] >= 58) && (ret[i] <= 64)));
  }

  return ret;
}

std::string get_environment_variable(const std::string &variable) {
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

std::uint64_t get_file_time_now() {
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

  const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
#ifdef _WIN32
  localtime_s(&local_time, &now);
#else
  const auto *tmp = std::localtime(&now);
  if (tmp) {
    memcpy(&local_time, tmp, sizeof(local_time));
  }
#endif
}

#ifdef _WIN32
#endif

bool get_next_available_port(std::uint16_t first_port, std::uint16_t &available_port) {
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

std::uint64_t get_time_now() {
#ifdef _WIN32
  return static_cast<std::uint64_t>(
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
#else
#if __APPLE__
  return std::chrono::nanoseconds(std::chrono::system_clock::now().time_since_epoch()).count();
#else
  return static_cast<std::uint64_t>(
      std::chrono::nanoseconds(std::chrono::high_resolution_clock::now().time_since_epoch())
          .count());
#endif
#endif
}

api_currency hastings_string_to_api_currency(const std::string &amount) {
  ttmath::Parser<api_currency> parser;
  parser.Parse(amount + " / (10 ^ 24)");
  return parser.stack[0].value;
}

CURL *reset_curl(CURL *curl_handle) {
  curl_easy_reset(curl_handle);
#if __APPLE__
  curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
#endif
  return curl_handle;
}

bool retryable_action(const std::function<bool()> &action) {
  auto succeeded = false;
  for (auto i = 0; not(succeeded = action()) && (i < 10); i++) {
    std::this_thread::sleep_for(100ms);
  }
  return succeeded;
}

/* bool parse_url(const std::string &url, HostConfig &hc) { */
/*   static const auto pathRegex = std::regex( */
/*       R"(^((\w+):)?(\/\/((\w+)?(:(\w+))?@)?([^\/\?:]+)(:(\d+))?)?(\/?([^\/\?#][^\?#]*)?)?(\?([^#]+))?(#(\w*))?$)");
 */
/*  */
/*   std::smatch results; */
/*   if (std::regex_search(url, results, pathRegex)) { */
/*     hc.HostNameOrIp = results[8u].str(); */
/*     hc.Path = utils::path::create_api_path(results[11u].str()); */
/*     hc.Protocol = utils::string::to_lower(results[2u].str()); */
/*     hc.ApiPort = results[10u].str().empty() ? ((hc.Protocol == "https") ? 443u : 80u) */
/*                                             : utils::string::to_uint16(results[10u].str()); */
/*     return not hc.HostNameOrIp.empty() && not hc.Protocol.empty() && */
/*            ((hc.Protocol == "http") || (hc.Protocol == "https")); */
/*   } */
/*  */
/*   return false; */
/* } */

void spin_wait_for_mutex(std::function<bool()> complete, std::condition_variable &cv,
                         std::mutex &mtx, const std::string &text) {
  while (not complete()) {
    unique_mutex_lock l(mtx);
    if (not complete()) {
      if (not text.empty()) {
        /* event_system::instance().raise<DebugLog>(__FUNCTION__, "spin_wait_for_mutex", text); */
      }
      cv.wait_for(l, 5s);
    }
    l.unlock();
  }
}

void spin_wait_for_mutex(bool &complete, std::condition_variable &cv, std::mutex &mtx,
                         const std::string &text) {
  while (not complete) {
    unique_mutex_lock l(mtx);
    if (not complete) {
      if (not text.empty()) {
        /* event_system::instance().raise<DebugLog>(__FUNCTION__, "spin_wait_for_mutex", text); */
      }
      cv.wait_for(l, 5s);
    }
    l.unlock();
  }
}
} // namespace repertory::utils
