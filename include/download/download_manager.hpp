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
#ifndef INCLUDE_DOWNLOAD_DOWNLOAD_MANAGER_HPP_
#define INCLUDE_DOWNLOAD_DOWNLOAD_MANAGER_HPP_

#include "common.hpp"
#include "download/i_download.hpp"
#include "download/i_download_manager.hpp"
#include "events/event_system.hpp"

namespace repertory {
class app_config;
class download_end;
class filesystem_item_handle_closed;
class i_open_file_table;
class download_manager final : public virtual i_download_manager {
  E_CONSUMER();

public:
  download_manager(const app_config &config, api_reader_callback api_reader,
                   const bool &force_download = false);

  ~download_manager() override;

private:
  const app_config &config_;
  const api_reader_callback api_reader_;
  bool force_download_;
  i_open_file_table *oft_ = nullptr;
  mutable std::recursive_mutex download_mutex_;
  std::unordered_map<std::string, std::unordered_map<std::uint64_t, download_ptr>> download_lookup_;
  std::recursive_mutex start_stop_mutex_;
  bool stop_requested_ = true;
  std::unique_ptr<rocksdb::DB> restore_db_;

private:
  bool contains_handle(const std::string &api_path, const std::uint64_t &handle) const;

  download_ptr get_download(std::uint64_t handle, filesystem_item &fsi,
                            const bool &write_Supported);

  void handle_download_end(const download_end &de);

  void on_handle_closed(const filesystem_item_handle_closed &handle_closed);

  void reset_timeout(const std::string &api_path, const bool &file_closed);

  void start_incomplete();

public:
  api_error allocate(const std::uint64_t &handle, filesystem_item &fsi, const std::uint64_t &size,
                     const i_download::allocator_callback &allocator) override;

  bool contains_restore(const std::string &api_path) const override;

  api_error download_file(const std::uint64_t &handle, filesystem_item &fsi) override;

  std::size_t get_download_count() const { return download_lookup_.size(); }

  std::string get_source_path(const std::string &api_path) const;

  bool is_processing(const std::string &api_path) const override;

  bool pause_download(const std::string &api_path) override;

  api_error read_bytes(const std::uint64_t &handle, filesystem_item &fsi, std::size_t read_size,
                       const std::uint64_t &read_offset, std::vector<char> &data) override;

  void rename_download(const std::string &from_api_path, const std::string &to_api_path) override;

  api_error resize(const std::uint64_t &handle, filesystem_item &fsi,
                   const std::uint64_t &size) override;

  void resume_download(const std::string &api_path) override;

  void start(i_open_file_table *oft);

  void stop();

  api_error write_bytes(const std::uint64_t &handle, filesystem_item &fsi,
                        const std::uint64_t &write_offset, std::vector<char> data,
                        std::size_t &bytes_written) override;
};
} // namespace repertory

#endif // INCLUDE_DOWNLOAD_DOWNLOAD_MANAGER_HPP_
