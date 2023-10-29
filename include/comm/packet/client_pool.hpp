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
#ifndef INCLUDE_COMM_PACKET_CLIENT_POOL_HPP_
#define INCLUDE_COMM_PACKET_CLIENT_POOL_HPP_

#include "comm/packet/packet.hpp"
#include "types/repertory.hpp"

namespace repertory {
class client_pool final {
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
      std::mutex mutex;
      std::condition_variable notify;
      std::deque<std::shared_ptr<work_item>> queue;
    };

  public:
    explicit pool(std::uint8_t pool_size);

    ~pool() { shutdown(); }

  private:
    std::vector<std::unique_ptr<work_queue>> pool_queues_;
    std::vector<std::thread> pool_threads_;
    bool shutdown_ = false;
    std::atomic<std::uint8_t> thread_index_;

  public:
    void execute(std::uint64_t thread_id, const worker_callback &worker,
                 const worker_complete_callback &worker_complete);

    void shutdown();
  };

public:
  explicit client_pool(std::uint8_t pool_size = 10u)
      : pool_size_(pool_size ? pool_size : 10u) {}

  ~client_pool() { shutdown(); }

private:
  const std::uint8_t pool_size_;
  std::unordered_map<std::string, std::shared_ptr<pool>> pool_lookup_;
  std::mutex pool_mutex_;
  bool shutdown_ = false;

public:
  void execute(const std::string &client_id, std::uint64_t thread_id,
               const worker_callback &worker,
               const worker_complete_callback &worker_complete);

  void remove_client(const std::string &client_id);

  void shutdown();
};
} // namespace repertory

#endif // INCLUDE_COMM_PACKET_CLIENT_POOL_HPP_
