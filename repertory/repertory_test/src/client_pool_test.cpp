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

#include "test_common.hpp"

#include "comm/packet/client_pool.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/event_system.hpp"

namespace {
class client_pool_test : public ::testing::Test {
public:
  repertory::console_consumer consumer;

protected:
  void SetUp() override { repertory::event_system::instance().start(); }
  void TearDown() override { repertory::event_system::instance().stop(); }
};

template <class callback_t>
[[nodiscard]] auto wait_until(callback_t callback,
                              std::chrono::milliseconds timeout) -> bool {
  const auto began{std::chrono::steady_clock::now()};
  while (not callback()) {
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
    if (std::chrono::steady_clock::now() - began >= timeout) {
      return false;
    }
  }
  return true;
}
} // namespace

namespace repertory {

TEST_F(client_pool_test, execute_invokes_completion) {
  client_pool pool;

  std::atomic<bool> done{false};

  pool.execute(
      "alpha", 1U, []() -> packet::error_type { return packet::error_type{0}; },
      [&](packet::error_type err) -> void {
        EXPECT_EQ(err, packet::error_type{0});
        done = true;
      });

  ASSERT_TRUE(wait_until([&]() -> bool { return done.load(); },
                         std::chrono::milliseconds{500}));
}

TEST_F(client_pool_test, fifo_on_same_thread_id) {
  client_pool pool;

  std::mutex vec_mutex;
  std::vector<int> order;
  std::atomic<std::uint32_t> completed{0U};

  constexpr std::uint32_t count{10U};
  for (std::uint32_t idx = 0U; idx < count; ++idx) {
    pool.execute(
        "alpha", 42U,
        [idx]() -> packet::error_type { return static_cast<int>(idx); },
        [&](auto && /*err*/) -> void {
          std::lock_guard<std::mutex> lock(vec_mutex);
          order.emplace_back(static_cast<int>(order.size()));
          completed.fetch_add(1U, std::memory_order_acq_rel);
        });
  }

  ASSERT_TRUE(wait_until([&]() -> bool { return completed.load() >= count; },
                         std::chrono::milliseconds{2000}));

  ASSERT_EQ(order.size(), static_cast<std::size_t>(count));
  for (int idx = 0; idx < static_cast<int>(count); ++idx) {
    EXPECT_EQ(order[static_cast<std::size_t>(idx)], idx);
  }
}

TEST_F(client_pool_test, parallel_on_different_thread_ids) {
  client_pool pool;

  std::atomic<std::uint32_t> started{0U};
  std::atomic<std::uint32_t> completed{0U};

  const auto barrier = [&started]() -> void {
    started.fetch_add(1U, std::memory_order_acq_rel);
    (void)wait_until([&]() -> bool { return started.load() >= 2U; },
                     std::chrono::milliseconds{500});
  };

  pool.execute(
      "alpha", 1U,
      [&]() -> packet::error_type {
        barrier();
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        return packet::error_type{0};
      },
      [&](packet::error_type err) -> void {
        EXPECT_EQ(err, packet::error_type{0});
        completed.fetch_add(1U, std::memory_order_acq_rel);
      });

  pool.execute(
      "alpha", 2U,
      [&]() -> packet::error_type {
        barrier();
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        return packet::error_type{0};
      },
      [&](packet::error_type err) -> void {
        EXPECT_EQ(err, packet::error_type{0});
        completed.fetch_add(1U, std::memory_order_acq_rel);
      });

  ASSERT_TRUE(wait_until([&]() -> bool { return completed.load() >= 2U; },
                         std::chrono::milliseconds{1000}));
}

TEST_F(client_pool_test, remove_client_then_recreate_pool) {
  client_pool pool;

  std::atomic<bool> first_done{false};
  std::atomic<bool> second_done{false};

  pool.execute(
      "moose", 7U, []() -> packet::error_type { return packet::error_type{0}; },
      [&](packet::error_type err) -> void {
        EXPECT_EQ(err, packet::error_type{0});
        first_done = true;
      });

  ASSERT_TRUE(wait_until([&]() -> bool { return first_done.load(); },
                         std::chrono::milliseconds{500}));

  pool.remove_client("moose");

  pool.execute(
      "moose", 7U, []() -> packet::error_type { return packet::error_type{0}; },
      [&](packet::error_type err) -> void {
        EXPECT_EQ(err, packet::error_type{0});
        second_done = true;
      });

  ASSERT_TRUE(wait_until([&]() -> bool { return second_done.load(); },
                         std::chrono::milliseconds{500}));
}

TEST_F(client_pool_test, shutdown_prevents_future_execute) {
  client_pool pool;

  std::atomic<bool> done{false};
  pool.execute(
      "cmdc", 3U, []() -> packet::error_type { return packet::error_type{0}; },
      [&](packet::error_type) -> void { done = true; });
  ASSERT_TRUE(wait_until([&]() -> bool { return done.load(); },
                         std::chrono::milliseconds{500}));

  pool.shutdown();

  EXPECT_THROW(pool.execute(
                   "cmdc", 3U,
                   []() -> packet::error_type { return packet::error_type{0}; },
                   [](packet::error_type) -> void {}),
               std::runtime_error);
}

TEST_F(client_pool_test, worker_exception_is_contained_and_no_completion) {
  client_pool pool;

  std::atomic<bool> completion_called{false};

  pool.execute(
      "delta", 1U,
      []() -> packet::error_type { throw std::runtime_error("boom"); },
      [&](packet::error_type) -> void { completion_called = true; });

  std::this_thread::sleep_for(std::chrono::milliseconds{150});

  EXPECT_FALSE(completion_called.load());
}

TEST_F(client_pool_test, remove_expired_is_safe_to_call) {
  client_pool pool;
  EXPECT_NO_THROW(pool.remove_expired());
}

TEST_F(client_pool_test, defaults_and_minimum_constants) {
  client_pool pool;

  EXPECT_EQ(pool.get_expired_seconds(), client_pool::default_expired_seconds);
  EXPECT_EQ(client_pool::min_expired_seconds, static_cast<std::uint16_t>(5U));
}

TEST_F(client_pool_test, setter_clamps_below_minimum_to_minimum) {
  client_pool pool;

  pool.set_expired_seconds(0U);
  EXPECT_EQ(pool.get_expired_seconds(), client_pool::min_expired_seconds);

  pool.set_expired_seconds(1U);
  EXPECT_EQ(pool.get_expired_seconds(), client_pool::min_expired_seconds);

  pool.set_expired_seconds(
      static_cast<std::uint16_t>(client_pool::min_expired_seconds - 1U));
  EXPECT_EQ(pool.get_expired_seconds(), client_pool::min_expired_seconds);

  pool.set_expired_seconds(client_pool::min_expired_seconds);
  EXPECT_EQ(pool.get_expired_seconds(), client_pool::min_expired_seconds);

  pool.set_expired_seconds(300U);
  EXPECT_EQ(pool.get_expired_seconds(), static_cast<std::uint16_t>(300));
}

TEST_F(client_pool_test, does_not_remove_queues_before_minimum_threshold) {
  client_pool pool;

  pool.set_expired_seconds(1U);
  EXPECT_EQ(pool.get_expired_seconds(), client_pool::min_expired_seconds);

  std::string client_id{"alpha"};

  std::thread::id tid_one_initial{};
  std::thread::id tid_two_initial{};
  std::atomic<bool> one_done{false};
  std::atomic<bool> two_done{false};

  pool.execute(
      client_id, 1U,
      [&]() -> packet::error_type {
        tid_one_initial = std::this_thread::get_id();
        return packet::error_type{0};
      },
      [&](const packet::error_type &) -> void { one_done = true; });

  pool.execute(
      client_id, 2U,
      [&]() -> packet::error_type {
        tid_two_initial = std::this_thread::get_id();
        return packet::error_type{0};
      },
      [&](const packet::error_type &) -> void { two_done = true; });

  ASSERT_TRUE(wait_until([&]() -> bool { return one_done.load(); },
                         std::chrono::milliseconds{500}));
  ASSERT_TRUE(wait_until([&]() -> bool { return two_done.load(); },
                         std::chrono::milliseconds{500}));

  std::this_thread::sleep_for(std::chrono::milliseconds{1100});
  pool.remove_expired();

  std::thread::id tid_one_after{};
  std::thread::id tid_two_after{};
  std::atomic<bool> one_again{false};
  std::atomic<bool> two_again{false};

  pool.execute(
      client_id, 1U,
      [&]() -> packet::error_type {
        tid_one_after = std::this_thread::get_id();
        return packet::error_type{0};
      },
      [&](const packet::error_type &) -> void { one_again = true; });

  pool.execute(
      client_id, 2U,
      [&]() -> packet::error_type {
        tid_two_after = std::this_thread::get_id();
        return packet::error_type{0};
      },
      [&](const packet::error_type &) -> void { two_again = true; });

  ASSERT_TRUE(wait_until([&]() -> bool { return one_again.load(); },
                         std::chrono::milliseconds{500}));
  ASSERT_TRUE(wait_until([&]() -> bool { return two_again.load(); },
                         std::chrono::milliseconds{500}));

  EXPECT_EQ(tid_one_after, tid_one_initial);
  EXPECT_EQ(tid_two_after, tid_two_initial);
}

TEST_F(client_pool_test,
       remove_expired_returns_quickly_when_no_queues_eligible) {
  client_pool pool;

  pool.set_expired_seconds(client_pool::min_expired_seconds);
  EXPECT_EQ(pool.get_expired_seconds(), client_pool::min_expired_seconds);

  std::string client_id{"moose"};

  std::atomic<bool> started{false};
  constexpr auto job_ms = std::chrono::milliseconds{150};

  pool.execute(
      client_id, 1U,
      [&]() -> packet::error_type {
        started = true;
        std::this_thread::sleep_for(job_ms);
        return packet::error_type{0};
      },
      [&](const packet::error_type &) -> void {});

  ASSERT_TRUE(wait_until([&]() -> bool { return started.load(); },
                         std::chrono::milliseconds{200}));

  auto start_time{std::chrono::steady_clock::now()};
  pool.remove_expired();
  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start_time);

  EXPECT_LT(elapsed_ms.count(), 50);
}

TEST_F(client_pool_test, removes_after_minimum_threshold) {
  client_pool pool;

  pool.set_expired_seconds(client_pool::min_expired_seconds);
  const auto threshold_secs = static_cast<int>(pool.get_expired_seconds());
  EXPECT_GE(threshold_secs, static_cast<int>(client_pool::min_expired_seconds));

  std::string client_id{"cmdc"};

  static thread_local std::int32_t tls_counter{0};

  std::atomic<std::int32_t> first_gen{-1};
  std::atomic<bool> first_done{false};

  pool.execute(
      client_id, 1U,
      [&]() -> packet::error_type {
        first_gen = tls_counter;
        tls_counter += 1;
        return packet::error_type{0};
      },
      [&](const packet::error_type &) -> void { first_done = true; });

  ASSERT_TRUE(wait_until([&]() -> bool { return first_done.load(); },
                         std::chrono::milliseconds{500}));

  std::this_thread::sleep_for(std::chrono::seconds{threshold_secs} +
                              std::chrono::milliseconds{200});

  pool.remove_expired();

  std::atomic<std::int32_t> second_gen{-1};
  std::atomic<bool> second_done{false};

  pool.execute(
      client_id, 1U,
      [&]() -> packet::error_type {
        second_gen = tls_counter;
        tls_counter += 1;
        return packet::error_type{0};
      },
      [&](const packet::error_type &) -> void { second_done = true; });

  ASSERT_TRUE(wait_until([&]() -> bool { return second_done.load(); },
                         std::chrono::milliseconds{500}));

  EXPECT_EQ(first_gen.load(), 0);
  EXPECT_EQ(second_gen.load(), 0);
}
} // namespace repertory
