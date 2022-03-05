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
#ifndef INCLUDE_DOWNLOAD_BUFFERED_READER_HPP_
#define INCLUDE_DOWNLOAD_BUFFERED_READER_HPP_

#include "common.hpp"
#include "types/repertory.hpp"

namespace repertory {
class app_config;
class buffered_reader final {
public:
  buffered_reader(const app_config &config, const filesystem_item &fsi,
                  const api_reader_callback &api_reader, const std::size_t &chunk_size,
                  const std::size_t &total_chunks, const std::size_t &start_chunk);

  ~buffered_reader();

private:
  const filesystem_item &fsi_;
  const api_reader_callback &api_reader_;
  const std::size_t chunk_size_;
  const std::size_t total_chunks_;
  boost::dynamic_bitset<> ring_state_;

  api_error error_ = api_error::success;
  std::unique_ptr<std::vector<char>> first_chunk_data_;
  std::unique_ptr<std::vector<char>> last_chunk_data_;
  std::size_t read_chunk_index_ = 0u;
  std::mutex read_mutex_;
  std::condition_variable read_notify_;
  std::size_t read_offset_ = 0u;
  bool reset_reader_ = false;
  std::vector<std::vector<char>> ring_data_;
  std::unique_ptr<std::thread> reader_thread_;
  bool stop_requested_ = false;
  std::mutex write_mutex_;
  std::size_t write_chunk_index_ = 0u;

private:
  bool is_active() const { return not stop_requested_ && (error_ == api_error::success); }

  void reader_thread();

public:
  std::size_t get_chunk_size() const { return chunk_size_; }

  void get_first_chunk(std::vector<char> *&data) { data = first_chunk_data_.get(); }

  void get_last_chunk(std::vector<char> *&data) { data = last_chunk_data_.get(); }

  bool has_first_chunk() const { return static_cast<bool>(first_chunk_data_); }

  bool has_last_chunk() const { return static_cast<bool>(last_chunk_data_); }

  void notify_stop_requested();

  api_error read_chunk(const std::size_t &chunk_index, std::vector<char> &data);
};
} // namespace repertory

#endif // INCLUDE_DOWNLOAD_BUFFERED_READER_HPP_
