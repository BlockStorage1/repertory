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
#ifndef REPERTORY_INCLUDE_COMM_PACKET_CLIENT_POOL_HPP_
#define REPERTORY_INCLUDE_COMM_PACKET_CLIENT_POOL_HPP_

#include "comm/packet/packet.hpp"

namespace repertory {
class client_pool final {
public:
  static constexpr const std::uint16_t default_expired_seconds{120U};
  static constexpr const std::uint16_t min_expired_seconds{5U};

public:
  using worker_callback = std::function<packet::error_type()>;
  using worker_complete_callback =
      std::function<void(const packet::error_type &)>;

private:
  class pool final {
  private:
    struct work_item final {
      work_item(worker_callback worker,
                worker_complete_callback worker_complete)
          : work(std::move(worker)),
            work_complete(std::move(worker_complete)) {}

      worker_callback work;
      worker_complete_callback work_complete;
    };

    struct work_queue final {
      work_queue();
      ~work_queue();

      work_queue(const work_queue &) = delete;
      work_queue(work_queue &&) = delete;

      std::deque<std::shared_ptr<work_item>> actions;
      std::atomic<std::chrono::steady_clock::time_point> modified{
          std::chrono::steady_clock::now(),
      };
      std::mutex mutex;
      std::condition_variable notify;
      stop_type shutdown{false};
      std::unique_ptr<std::thread> thread;

      auto operator=(const work_queue &) -> work_queue & = delete;
      auto operator=(work_queue &&) -> work_queue & = delete;

    private:
      void work_thread();
    };

  public:
    pool() noexcept = default;

    ~pool();

  public:
    pool(const pool &) = delete;
    pool(pool &&) = delete;
    auto operator=(const pool &) -> pool & = delete;
    auto operator=(pool &&) -> pool & = delete;

  private:
    std::mutex pool_mtx_;
    std::unordered_map<std::uint64_t, std::shared_ptr<work_queue>> pool_queues_;

  public:
    void execute(std::uint64_t thread_id, worker_callback worker,
                 worker_complete_callback worker_complete);

    void remove_expired(std::uint16_t seconds);

    void shutdown();
  };

public:
  client_pool() noexcept;

  ~client_pool() { shutdown(); }

public:
  client_pool(const client_pool &) = delete;
  client_pool(client_pool &&) = delete;
  auto operator=(const client_pool &) -> client_pool & = delete;
  auto operator=(client_pool &&) -> client_pool & = delete;

private:
  std::unordered_map<std::string, std::unique_ptr<pool>> pool_lookup_;
  std::mutex pool_mutex_;
  stop_type shutdown_{false};
  std::atomic<std::uint16_t> expired_seconds_{default_expired_seconds};

public:
  [[nodiscard]] auto get_expired_seconds() const -> std::uint16_t;

  void execute(std::string client_id, std::uint64_t thread_id,
               worker_callback worker,
               worker_complete_callback worker_complete);

  void remove_client(std::string client_id);

  void remove_expired();

  void set_expired_seconds(std::uint16_t seconds);

  void shutdown();
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_COMM_PACKET_CLIENT_POOL_HPP_
