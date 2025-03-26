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
#include "utils/common.hpp"

#include "utils/error.hpp"
#include "utils/string.hpp"

namespace repertory::utils {
auto compare_version_strings(std::string version1, std::string version2)
    -> std::int32_t {

  if (utils::string::contains(version1, "-")) {
    version1 = utils::string::split(version1, '-', true)[0U];
  }

  if (utils::string::contains(version2, "-")) {
    version2 = utils::string::split(version2, '-', true)[0U];
  }

  auto nums1 = utils::string::split(version1, '.', true);
  auto nums2 = utils::string::split(version2, '.', true);

  while (nums1.size() > nums2.size()) {
    nums2.emplace_back("0");
  }

  while (nums2.size() > nums1.size()) {
    nums1.emplace_back("0");
  }

  for (std::size_t idx = 0U; idx < nums1.size(); idx++) {
    auto int1 = utils::string::to_uint32(nums1[idx]);
    auto int2 = utils::string::to_uint32(nums2[idx]);
    auto res = std::memcmp(&int1, &int2, sizeof(int1));
    if (res != 0) {
      return res;
    }
  }

  return 0;
}

auto compare_version_strings(std::wstring_view version1,
                             std::wstring_view version2) -> std::int32_t {
  return compare_version_strings(utils::string::to_utf8(version1),
                                 utils::string::to_utf8(version2));
}

#if defined(PROJECT_ENABLE_STDUUID)
auto create_uuid_string() -> std::string {
  std::random_device random_device{};
  auto seed_data = std::array<int, std::mt19937::state_size>{};
  std::generate(std::begin(seed_data), std::end(seed_data),
                std::ref(random_device));
  std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
  std::mt19937 generator(seq);
  uuids::uuid_random_generator gen{generator};

  return uuids::to_string(gen());
}

auto create_uuid_wstring() -> std::wstring {
  return utils::string::from_utf8(create_uuid_string());
}
#endif // defined(PROJECT_ENABLE_STDUUID)

auto generate_random_string(std::size_t length) -> std::string {
  std::string ret;
  if (length == 0U) {
    return ret;
  }

  ret.resize(length);
  for (auto &ch : ret) {
    std::array<unsigned int, 3U> random_list{
        generate_random_between(0U, 57U),
        generate_random_between(65U, 90U),
        generate_random_between(97U, 255U),
    };
    ch = static_cast<char>(
        random_list.at(generate_random_between(0U, 2U)) % 74U + 48U);
  }

  return ret;
}

auto generate_random_wstring(std::size_t length) -> std::wstring {
  return utils::string::from_utf8(generate_random_string(length));
}

auto get_environment_variable(std::string_view variable) -> std::string {
  static std::mutex mtx{};
  mutex_lock lock{mtx};

  const auto *val = std::getenv(std::string{variable}.c_str());
  return std::string{val == nullptr ? "" : val};
}

auto get_environment_variable(std::wstring_view variable) -> std::wstring {
  return utils::string::from_utf8(
      get_environment_variable(utils::string::to_utf8(variable)));
}

#if defined(PROJECT_ENABLE_BOOST)
auto get_next_available_port(std::uint16_t first_port,
                             std::uint16_t &available_port) -> bool {
  if (first_port == 0U) {
    return false;
  }

  using namespace boost::asio;
  using ip::tcp;

  boost::system::error_code error_code{};

  std::uint32_t check_port{first_port};
  while (check_port <= 65535U) {
    {
      io_context ctx{};
      tcp::socket socket(ctx);
      socket.connect(
          {
              tcp::endpoint(ip::address_v4::loopback(),
                            static_cast<std::uint16_t>(check_port)),
          },
          error_code);
      if (not error_code) {
        ++check_port;
        continue;
      }
    }

    {
      io_context ctx{};
      tcp::acceptor acceptor(ctx);
      acceptor.open(tcp::v4(), error_code);
      if (error_code) {
        ++check_port;
        continue;
      }
      acceptor.set_option(boost::asio::ip::tcp::acceptor::linger(true, 0));
      acceptor.bind({tcp::v4(), static_cast<std::uint16_t>(check_port)},
                    error_code);
      if (error_code) {
        ++check_port;
        continue;
      }
    }

    available_port = static_cast<std::uint16_t>(check_port);
    return true;
  }

  return false;
}
#endif // defined(PROJECT_ENABLE_BOOST)

auto retry_action(retryable_action_t action, std::size_t retry_count,
                  std::chrono::milliseconds retry_wait) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    for (std::size_t idx = 0U; idx < retry_count; ++idx) {
      if (action()) {
        return true;
      }

      std::this_thread::sleep_for(retry_wait);
    }
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}
} // namespace repertory::utils
