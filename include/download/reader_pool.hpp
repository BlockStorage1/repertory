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
#ifndef INCLUDE_DOWNLOAD_READER_POOL_HPP_
#define INCLUDE_DOWNLOAD_READER_POOL_HPP_

#include "common.hpp"
#include "types/repertory.hpp"

namespace repertory {
class reader_pool final {
public:
  typedef std::function<void(api_error error)> completed_callback;

private:
  struct pool_work_item {
    pool_work_item(std::string api_path, const std::size_t &read_size,
                   const std::uint64_t &read_offset, std::vector<char> &data,
                   completed_callback completed)
        : api_path(std::move(api_path)), read_size(read_size), read_offset(read_offset), data(data),
          completed(completed) {}

    std::string api_path;
    std::size_t read_size;
    std::uint64_t read_offset;
    std::vector<char> &data;
    completed_callback completed;
  };

public:
  reader_pool(const std::size_t &pool_size, const api_reader_callback &api_reader)
      : pool_size_(pool_size), api_reader_(api_reader) {
    start();
  }

  ~reader_pool() { stop(); }

private:
  const std::size_t pool_size_;
  const api_reader_callback &api_reader_;
  bool paused_ = false;
  bool restart_active_ = false;
  bool stop_requested_ = false;
  std::mutex work_mutex_;
  std::condition_variable work_notify_;
  std::deque<std::shared_ptr<pool_work_item>> work_queue_;
  std::vector<std::thread> work_threads_;
  std::uint16_t active_count_ = 0u;

private:
  void process_work_item(pool_work_item &work);

  void start();

  void stop();

  void wait_for_resume(unique_mutex_lock &lock);

public:
  void pause();

  void queue_read_bytes(const std::string &api_path, const std::size_t &read_size,
                        const std::uint64_t &read_offset, std::vector<char> &data,
                        completed_callback completed);

  void restart();

  void resume();
};
} // namespace repertory

#endif // INCLUDE_DOWNLOAD_READER_POOL_HPP_
