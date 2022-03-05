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
#ifndef INCLUDE_DOWNLOAD_DIRECT_DOWNLOAD_HPP_
#define INCLUDE_DOWNLOAD_DIRECT_DOWNLOAD_HPP_

#include "common.hpp"
#include "download/i_download.hpp"
#include "types/repertory.hpp"

namespace repertory {
class buffered_reader;
class app_config;
class direct_download final : public virtual i_download {
public:
  direct_download(const app_config &config, filesystem_item fsi,
                  const api_reader_callback &api_reader, const std::uint64_t &handle);

  ~direct_download() override;

private:
  const app_config &config_;
  const filesystem_item fsi_;
  const api_reader_callback &api_reader_;
  const std::uint64_t handle_;

  api_error error_ = api_error::success;
  std::unique_ptr<buffered_reader> buffered_reader_;
  bool disable_download_end_ = false;
  bool download_end_notified_ = false;
  double progress_ = 0.0;
  std::mutex read_mutex_;
  bool stop_requested_ = false;

private:
  bool is_active() const { return not stop_requested_ && (error_ == api_error::success); }

  void notify_download_end();

  void set_api_error(const api_error &error);

public:
  api_error allocate(const std::uint64_t & /*handle*/, const std::uint64_t & /*size*/,
                     const allocator_callback & /*allocator*/,
                     const completer_callback & /*completer*/) override {
    return api_error::not_implemented;
  }

  api_error download_all() override { return api_error::not_implemented; }

  api_error get_result() const override { return error_; }

  std::string get_source_path() const override { return ""; }

  void get_state_information(filesystem_item &, std::size_t &, std::size_t &,
                             boost::dynamic_bitset<> &, boost::dynamic_bitset<> &) override {}

  bool get_write_supported() const override { return false; }

  void notify_stop_requested() override;

  bool pause() override { return false; }

  api_error read_bytes(const std::uint64_t &, std::size_t read_size,
                       const std::uint64_t &read_offset, std::vector<char> &data) override;

  void reset_timeout(const bool &) override {}

  void resume() override {}

  void set_api_path(const std::string &) override {}

  void set_disable_download_end(const bool &disable) override { disable_download_end_ = disable; }

  api_error write_bytes(const std::uint64_t &, const std::uint64_t &, std::vector<char>,
                        std::size_t &bytes_written,
                        const completer_callback & /*completer*/) override {
    bytes_written = 0u;
    return api_error::not_implemented;
  }
};
} // namespace repertory

#endif // INCLUDE_DOWNLOAD_DIRECT_DOWNLOAD_HPP_
