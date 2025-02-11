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
#include "test.hpp"

namespace repertory {
TEST(utils_common, calculate_read_size) {
  auto read_size = utils::calculate_read_size(0U, 0U, 0U);
  EXPECT_EQ(0U, read_size);

  read_size = utils::calculate_read_size(5U, 0U, 0U);
  EXPECT_EQ(0U, read_size);

  read_size = utils::calculate_read_size(0U, 6U, 7U);
  EXPECT_EQ(0U, read_size);

  read_size = utils::calculate_read_size(7U, 1U, 7U);
  EXPECT_EQ(0U, read_size);

  read_size = utils::calculate_read_size(5U, 5U, 0U);
  EXPECT_EQ(5U, read_size);

  read_size = utils::calculate_read_size(5U, 5U, 1U);
  EXPECT_EQ(4U, read_size);
}

TEST(utils_common, version_equal) {
  EXPECT_EQ(0, utils::compare_version_strings("", ""));
  EXPECT_EQ(0, utils::compare_version_strings("1.0", "1.0"));
  EXPECT_EQ(0, utils::compare_version_strings("1.0.0", "1.0"));
  EXPECT_EQ(0, utils::compare_version_strings("1.0.0.0", "1.0"));
  EXPECT_EQ(0, utils::compare_version_strings("1.0.0.0", "1.0.0"));

  EXPECT_EQ(0, utils::compare_version_strings(L"", L""));
  EXPECT_EQ(0, utils::compare_version_strings(L"1.0", L"1.0"));
  EXPECT_EQ(0, utils::compare_version_strings(L"1.0.0", L"1.0"));
  EXPECT_EQ(0, utils::compare_version_strings(L"1.0.0.0", L"1.0"));
  EXPECT_EQ(0, utils::compare_version_strings(L"1.0.0.0", L"1.0.0"));
}

TEST(utils_common, version_greater) {
  EXPECT_EQ(1, utils::compare_version_strings("1.0.1", ""));
  EXPECT_EQ(1, utils::compare_version_strings("1.0.1", "1.0"));
  EXPECT_EQ(1, utils::compare_version_strings("1.0.1", "1.0.0"));
  EXPECT_EQ(1, utils::compare_version_strings("1.0.1", "1.0.0.0"));
  EXPECT_EQ(1, utils::compare_version_strings("1.0.1.0", "1.0"));
  EXPECT_EQ(1, utils::compare_version_strings("1.0.1.0", "1.0.0"));
  EXPECT_EQ(1, utils::compare_version_strings("1.0.1.0", "1.0.0.0"));
  EXPECT_EQ(1, utils::compare_version_strings("1.0", "0.9.9"));
  EXPECT_EQ(1, utils::compare_version_strings("1.0.1", "0.9.9"));
  EXPECT_EQ(1, utils::compare_version_strings("1.0.1.0", "0.9.9"));

  EXPECT_EQ(1, utils::compare_version_strings("1.0.1", ""));
  EXPECT_EQ(1, utils::compare_version_strings(L"1.0.1", L"1.0"));
  EXPECT_EQ(1, utils::compare_version_strings(L"1.0.1", L"1.0.0"));
  EXPECT_EQ(1, utils::compare_version_strings(L"1.0.1", L"1.0.0.0"));
  EXPECT_EQ(1, utils::compare_version_strings(L"1.0.1.0", L"1.0"));
  EXPECT_EQ(1, utils::compare_version_strings(L"1.0.1.0", L"1.0.0"));
  EXPECT_EQ(1, utils::compare_version_strings(L"1.0.1.0", L"1.0.0.0"));
  EXPECT_EQ(1, utils::compare_version_strings(L"1.0", L"0.9.9"));
  EXPECT_EQ(1, utils::compare_version_strings(L"1.0.1", L"0.9.9"));
  EXPECT_EQ(1, utils::compare_version_strings(L"1.0.1.0", L"0.9.9"));
}

TEST(utils_common, version_less) {
  EXPECT_EQ(-1, utils::compare_version_strings("", "1.0"));
  EXPECT_EQ(-1, utils::compare_version_strings("0.9.9", "1.0"));
  EXPECT_EQ(-1, utils::compare_version_strings("0.9.9", "1.0.1"));
  EXPECT_EQ(-1, utils::compare_version_strings("0.9.9", "1.0.1.0"));
  EXPECT_EQ(-1, utils::compare_version_strings("1.0", "1.0.1"));
  EXPECT_EQ(-1, utils::compare_version_strings("1.0", "1.0.1.0"));
  EXPECT_EQ(-1, utils::compare_version_strings("1.0.0", "1.0.1"));
  EXPECT_EQ(-1, utils::compare_version_strings("1.0.0", "1.0.1.0"));
  EXPECT_EQ(-1, utils::compare_version_strings("1.0.0.0", "1.0.1"));
  EXPECT_EQ(-1, utils::compare_version_strings("1.0.0.0", "1.0.1.0"));

  EXPECT_EQ(-1, utils::compare_version_strings(L"", L"1.0"));
  EXPECT_EQ(-1, utils::compare_version_strings(L"0.9.9", L"1.0"));
  EXPECT_EQ(-1, utils::compare_version_strings(L"0.9.9", L"1.0.1"));
  EXPECT_EQ(-1, utils::compare_version_strings(L"0.9.9", L"1.0.1.0"));
  EXPECT_EQ(-1, utils::compare_version_strings(L"1.0", L"1.0.1"));
  EXPECT_EQ(-1, utils::compare_version_strings(L"1.0", L"1.0.1.0"));
  EXPECT_EQ(-1, utils::compare_version_strings(L"1.0.0", L"1.0.1"));
  EXPECT_EQ(-1, utils::compare_version_strings(L"1.0.0", L"1.0.1.0"));
  EXPECT_EQ(-1, utils::compare_version_strings(L"1.0.0.0", L"1.0.1"));
  EXPECT_EQ(-1, utils::compare_version_strings(L"1.0.0.0", L"1.0.1.0"));
}

#if defined(PROJECT_ENABLE_STDUUID)
TEST(utils_common, create_uuid_string) {
  {
    const auto uuid1 = utils::create_uuid_string();
    const auto uuid2 = utils::create_uuid_string();
    ASSERT_EQ(36U, uuid1.size());
    ASSERT_EQ(36U, uuid2.size());
    ASSERT_STRNE(uuid1.c_str(), uuid2.c_str());
  }
  {
    const auto uuid1 = utils::create_uuid_wstring();
    const auto uuid2 = utils::create_uuid_wstring();
    ASSERT_EQ(36U, uuid1.size());
    ASSERT_EQ(36U, uuid2.size());
    ASSERT_STRNE(uuid1.c_str(), uuid2.c_str());
  }
}
#endif // defined(PROJECT_ENABLE_STDUUID)

#if defined(PROJECT_ENABLE_LIBSODIUM)
TEST(utils_common, generate_secure_random) {
  {
    auto r1 = utils::generate_secure_random<std::size_t>();
    auto r2 = utils::generate_secure_random<std::size_t>();
    EXPECT_NE(r1, r2);
  }

  {
    auto r1 = utils::generate_secure_random<std::vector<std::uint8_t>>(6U);
    auto r2 = utils::generate_secure_random<std::vector<std::uint8_t>>(6U);
    EXPECT_EQ(6U, r1.size());
    EXPECT_EQ(r1.size(), r2.size());
    EXPECT_NE(r1, r2);
  }

  {
    auto r1 = utils::generate_secure_random<std::array<std::uint8_t, 4U>>();
    auto r2 = utils::generate_secure_random<std::array<std::uint8_t, 4U>>();
    EXPECT_EQ(4U, r1.size());
    EXPECT_EQ(r1.size(), r2.size());
    EXPECT_NE(0, std::memcmp(r1.data(), r2.data(), r1.size()));
  }

  {
    auto r1 = utils::generate_secure_random<std::string>(6U);
    auto r2 = utils::generate_secure_random<std::string>(6U);
    EXPECT_EQ(6U, r1.size());
    EXPECT_EQ(r1.size(), r2.size());
    EXPECT_NE(0, std::memcmp(r1.data(), r2.data(), r1.size()));
  }

  {
    auto r1 = utils::generate_secure_random<std::wstring>(6U);
    auto r2 = utils::generate_secure_random<std::wstring>(6U);
    EXPECT_EQ(6U, r1.size());
    EXPECT_EQ(r1.size(), r2.size());
    EXPECT_NE(0, std::memcmp(r1.data(), r2.data(), r1.size()));
  }
}
#endif // defined(PROJECT_ENABLE_LIBSODIUM)

TEST(utils_common, divide_with_ceiling) {
  auto r = utils::divide_with_ceiling(12, 5);
  EXPECT_EQ(3, r);

  r = utils::divide_with_ceiling(12, 4);
  EXPECT_EQ(3, r);

  r = utils::divide_with_ceiling(1, 2);
  EXPECT_EQ(1, r);

  r = utils::divide_with_ceiling(2, 2);
  EXPECT_EQ(1, r);

  r = utils::divide_with_ceiling(0, 2);
  EXPECT_EQ(0, r);
}

TEST(utils_common, generate_random_between_for_signed_integers) {
  static constexpr const auto max_iterations{1000000UL};

  for (std::size_t idx = 0U; idx < max_iterations; ++idx) {
    auto res = utils::generate_random_between(5, 12);
    EXPECT_GE(res, 5);
    EXPECT_LE(res, 12);
  }

  for (std::size_t idx = 0U; idx < max_iterations; ++idx) {
    auto res = utils::generate_random_between(-5, 12);
    EXPECT_GE(res, -5);
    EXPECT_LE(res, 12);
  }

  for (std::size_t idx = 0U; idx < max_iterations; ++idx) {
    auto res = utils::generate_random_between(-5, -1);
    EXPECT_GE(res, -5);
    EXPECT_LE(res, -1);
  }
}

TEST(utils_common, generate_random_between_for_unsigned_integers) {
  static constexpr const auto max_iterations{1000000UL};

  for (std::size_t idx = 0U; idx < max_iterations; ++idx) {
    auto res = utils::generate_random_between(5U, 12U);
    EXPECT_GE(res, 5);
    EXPECT_LE(res, 12);
  }
}

TEST(utils_common, generate_random_between_throws_error_on_invalid_range) {
  EXPECT_THROW(
      {
        try {
          [[maybe_unused]] auto res = utils::generate_random_between(12, 5);
        } catch (const std::range_error &e) {
          EXPECT_STREQ("end must be greater than begin", e.what());
          throw;
        }
      },
      std::range_error);

  EXPECT_THROW(
      {
        try {
          [[maybe_unused]] auto res = utils::generate_random_between(12, 12);
        } catch (const std::range_error &e) {
          EXPECT_STREQ("end must be greater than begin", e.what());
          throw;
        }
      },
      std::range_error);
}

TEST(utils_common, generate_random_string) {
  static constexpr const auto max_iterations{10000L};

  const auto test_string = [](auto str) {
    static std::vector<decltype(str)> list{};

    EXPECT_FALSE(utils::collection::includes(list, str));
    list.push_back(str);

    EXPECT_EQ(16U, str.size());
    for (auto &&ch : str) {
      auto ch_int = static_cast<std::uint32_t>(ch);
      EXPECT_GE(ch_int, 48U);
      EXPECT_LE(ch_int, 73U + 48U);
    }
  };

  for (std::size_t idx = 0U; idx < max_iterations; ++idx) {
    test_string(utils::generate_random_string(16U));
    test_string(utils::generate_random_wstring(16U));
  }
}

TEST(utils_common, generate_random_string_for_zero_length) {
  EXPECT_TRUE(utils::generate_random_string(0U).empty());
  EXPECT_TRUE(utils::generate_random_wstring(0U).empty());
}

TEST(utils_common, get_environment_variable) {
  static constexpr const std::string path_env{"PATH"};
  std::string path;

#if defined(_WIN32)
  path.resize(repertory::max_path_length + 1U);
  auto size = ::GetEnvironmentVariableA(path_env.c_str(), path.data(), 0U);

  path.resize(size);
  ::GetEnvironmentVariableA(path_env.c_str(), path.data(),
                            static_cast<DWORD>(path.size()));
#else  // !defined(_WIN32)
  path = std::getenv(path_env.c_str());
#endif // defined(_WIN32)

  EXPECT_STREQ(path.c_str(), utils::get_environment_variable(path_env).c_str());
  EXPECT_STREQ(
      utils::string::from_utf8(path).c_str(),
      utils::get_environment_variable(utils::string::from_utf8(path_env))
          .c_str());
}

#if defined(PROJECT_ENABLE_BOOST)
TEST(utils_common, get_next_available_port) {
  std::uint16_t available_port{};
  for (std::uint16_t port = 1025U; port < 1030U; ++port) {
    EXPECT_TRUE(utils::get_next_available_port(port, available_port));
    EXPECT_GE(available_port, port);
  }
}

TEST(utils_common, get_next_available_port_fails_if_starting_point_is_zero) {
  std::uint16_t available_port{};
  EXPECT_FALSE(utils::get_next_available_port(0U, available_port));
  EXPECT_EQ(0U, available_port);
}
#endif // defined(PROJECT_ENABLE_BOOST)
} // namespace repertory
