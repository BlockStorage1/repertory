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
TEST(utils_ttl_cache, can_construct_cache) {
  utils::ttl_cache<std::uint8_t> cache;
  EXPECT_EQ(utils::ttl_cache<std::uint8_t>::default_expiration,
            cache.get_ttl());
}

TEST(utils_ttl_cache, can_construct_cache_with_ttl) {
  utils::ttl_cache<std::uint8_t> cache(std::chrono::milliseconds(1000U));
  EXPECT_EQ(std::chrono::milliseconds(1000U), cache.get_ttl());
}

TEST(utils_ttl_cache, can_change_ttl) {
  utils::ttl_cache<std::uint8_t> cache;
  cache.set_ttl(std::chrono::milliseconds(1000U));
  EXPECT_EQ(std::chrono::milliseconds(1000U), cache.get_ttl());
}

TEST(utils_ttl_cache, can_set_and_get) {
  utils::ttl_cache<std::uint8_t> cache;
  cache.set("/test", 21U);
  auto data = cache.get("/test");
  ASSERT_NE(nullptr, data.get());

  EXPECT_EQ(21U, data->load());
}

TEST(utils_ttl_cache, get_returns_nullptr_for_api_path_not_in_cache) {
  utils::ttl_cache<std::uint8_t> cache;
  auto data = cache.get("/test");
  ASSERT_EQ(nullptr, data.get());
}

TEST(utils_ttl_cache, set_and_get_returns_value_and_refreshes_ttl) {
  utils::ttl_cache<std::uint8_t> cache(std::chrono::milliseconds(1000U));

  cache.set("/test", 7U);
  auto data = cache.get("/test");
  {
    EXPECT_TRUE(cache.contains("/test"));
    ASSERT_NE(data, nullptr);
    EXPECT_EQ(7U, data->load());
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(200U));
  {
    EXPECT_TRUE(cache.contains("/test"));
    auto data2 = cache.get("/test");
    ASSERT_NE(data2, nullptr);
    ASSERT_EQ(data.get(), data2.get());
    EXPECT_EQ(7U, data2->load());
  }

  {
    std::this_thread::sleep_for(std::chrono::milliseconds(800U));
    cache.purge_expired();

    auto data3 = cache.get("/test");
    EXPECT_TRUE(cache.contains("/test"));

    ASSERT_NE(data3, nullptr);
    ASSERT_EQ(data.get(), data3.get());
    EXPECT_EQ(7U, data3->load());
  }
}

TEST(utils_ttl_cache, entry_expires_without_refresh) {
  utils::ttl_cache<std::uint8_t> cache(std::chrono::milliseconds(50U));
  cache.set("/test", 42U);

  std::this_thread::sleep_for(std::chrono::milliseconds(51U));
  cache.purge_expired();
  EXPECT_FALSE(cache.contains("/test"));

  auto data = cache.get("/test");
  EXPECT_EQ(nullptr, data.get());
}

TEST(utils_ttl_cache, can_erase) {
  utils::ttl_cache<std::uint8_t> cache(std::chrono::milliseconds(50U));
  cache.set("/test", 42U);
  cache.erase("/test");

  EXPECT_FALSE(cache.contains("/test"));
  auto data = cache.get("/test");
  EXPECT_EQ(nullptr, data.get());
}

TEST(utils_ttl_cache, can_clear) {
  utils::ttl_cache<std::uint8_t> cache(std::chrono::milliseconds(50U));

  cache.set("/test", 42U);
  cache.set("/test2", 42U);
  EXPECT_TRUE(cache.contains("/test"));
  EXPECT_TRUE(cache.contains("/test2"));
  cache.clear();

  {
    EXPECT_FALSE(cache.contains("/test"));
    auto data = cache.get("/test");
    EXPECT_EQ(nullptr, data.get());
  }

  {
    EXPECT_FALSE(cache.contains("/test2"));
    auto data = cache.get("/test2");
    EXPECT_EQ(nullptr, data.get());
  }
}

TEST(utils_ttl_cache, can_handle_concurrent_access) {
  utils::ttl_cache<std::uint8_t> cache(std::chrono::milliseconds(5000U));

  std::atomic<bool> start{false};
  std::thread writer([&] {
    while (not start.load()) {
    }
    for (std::uint8_t ttl = 0U; ttl < 100U; ++ttl) {
      cache.set("/key", ttl);
      std::this_thread::yield();
    }
  });

  std::thread reader([&] {
    while (not start.load()) {
    }
    for (std::uint8_t ttl = 0U; ttl < 100U; ++ttl) {
      auto data = cache.get("/key");
      if (data) {
        [[maybe_unused]] auto res = data->load();
      }
      std::this_thread::yield();
    }
  });

  start = true;
  writer.join();
  reader.join();

  auto data = cache.get("/key");
  ASSERT_NE(data, nullptr);
  [[maybe_unused]] auto res = data->load();
}

TEST(utils_ttl_cache, can_handle_custom_atomic) {
  utils::ttl_cache<std::string, utils::atomic> cache(
      std::chrono::milliseconds(5000U));
  cache.set("/test", "test");
  auto data = cache.get("/test");
  ASSERT_NE(nullptr, data.get());
  EXPECT_STREQ("test", data->load().c_str());
}

TEST(utils_ttl_cache, get_renews_after_ttl_if_purge_expired_is_not_called) {
  utils::ttl_cache<std::uint8_t> cache(std::chrono::milliseconds(50U));
  cache.set("/test", 9U);

  std::this_thread::sleep_for(std::chrono::milliseconds(75U));

  auto data = cache.get("/test");
  ASSERT_NE(nullptr, data.get());
  EXPECT_EQ(9U, data->load());

  cache.purge_expired();
  EXPECT_TRUE(cache.contains("/test"));
}

TEST(utils_ttl_cache, can_update_data) {
  utils::ttl_cache<std::uint8_t> cache;

  cache.set("/test", 1U);
  auto data = cache.get("/test");
  ASSERT_NE(nullptr, data.get());
  EXPECT_EQ(1U, data->load());

  cache.set("/test", 2U);
  auto data2 = cache.get("/test");
  ASSERT_NE(nullptr, data2.get());
  EXPECT_EQ(data.get(), data2.get());
  EXPECT_EQ(2U, data2->load());
}

TEST(utils_ttl_cache, purge_expired_removes_only_expired_entries) {
  utils::ttl_cache<std::uint8_t> cache(std::chrono::milliseconds(1000U));
  cache.set("/test1", 1U);
  cache.set("/test2", 2U);

  std::this_thread::sleep_for(std::chrono::milliseconds(500U));
  auto data = cache.get("/test2");
  ASSERT_NE(data, nullptr);

  std::this_thread::sleep_for(std::chrono::milliseconds(501U));
  cache.purge_expired();

  EXPECT_FALSE(cache.contains("/test1"));
  EXPECT_TRUE(cache.contains("/test2"));
}

TEST(utils_ttl_cache, can_handle_non_existing_items_without_failure) {
  utils::ttl_cache<std::uint8_t> cache;
  cache.set("/exists", 5U);
  EXPECT_TRUE(cache.contains("/exists"));

  cache.erase("/not_found");
  EXPECT_TRUE(cache.contains("/exists"));

  auto data = cache.get("/exists");
  ASSERT_NE(nullptr, data.get());
  EXPECT_EQ(5U, data->load());
}

TEST(utils_ttl_cache, changing_ttl_affects_only_future_expirations) {
  utils::ttl_cache<std::uint8_t> cache(std::chrono::milliseconds(1000U));
  cache.set("/test", 11U);

  cache.set_ttl(std::chrono::milliseconds(100U));

  std::this_thread::sleep_for(std::chrono::milliseconds(200U));
  cache.purge_expired();
  EXPECT_TRUE(cache.contains("/test"));

  auto data = cache.get("/test");
  ASSERT_NE(nullptr, data.get());
  EXPECT_EQ(11U, data->load());

  std::this_thread::sleep_for(std::chrono::milliseconds(200U));
  cache.purge_expired();
  EXPECT_FALSE(cache.contains("/test"));
}
} // namespace repertory
