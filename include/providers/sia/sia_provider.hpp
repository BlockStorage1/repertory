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
#ifndef INCLUDE_PROVIDERS_SIA_SIAPROVIDER_HPP_
#define INCLUDE_PROVIDERS_SIA_SIAPROVIDER_HPP_

#include "common.hpp"
#include "providers/base_provider.hpp"

namespace repertory {
class app_config;
class i_comm;
class sia_provider : public base_provider {
public:
  sia_provider(app_config &config, i_comm &comm);

  ~sia_provider() override = default;

private:
  i_comm &comm_;
  std::uint64_t total_drive_space_ = 0u;
  bool stop_requested_ = false;
  std::mutex start_stop_mutex_;
  std::condition_variable start_stop_notify_;
  std::unique_ptr<std::thread> drive_space_thread_;

private:
  api_item_added_callback &get_api_item_added() { return api_item_added_; }

  i_comm &get_comm() { return comm_; }

  i_comm &get_comm() const { return comm_; } // TODO Fix non-const reference return

private:
  void calculate_total_drive_space();

  api_error calculate_used_drive_space(std::uint64_t &used_space);

  void drive_space_thread();

  bool processed_orphaned_file(const std::string &source_path,
                               const std::string &api_path = "") const;

  void remove_deleted_files();

  void remove_expired_orphaned_files();

  void remove_unknown_source_files();

protected:
  api_error check_file_exists(const std::string &api_path) const;

  bool check_directory_found(const json &error) const;

  bool check_not_found(const json &error) const;

  virtual void cleanup();

  void create_api_file(const json &json_file, const std::string &path_name, api_file &file) const;

  api_error get_directory(const std::string &api_path, json &result, json &error) const;

  api_error notify_file_added(const std::string &api_path, const std::string &api_parent,
                              const std::uint64_t &size) override;

  void set_api_file_dates(const json &file, api_file &apiFile) const;

public:
  api_error create_directory(const std::string &api_path, const api_meta_map &meta) override;

  std::uint64_t get_directory_item_count(const std::string &api_path) const override;

  api_error get_directory_items(const std::string &api_path,
                                directory_item_list &list) const override;

  api_error get_file(const std::string &api_path, api_file &file) const override;

  api_error get_file_list(api_file_list &list) const override;

  api_error get_file_size(const std::string &api_path, std::uint64_t &file_size) const override;

  api_error get_filesystem_item(const std::string &api_path, const bool &directory,
                                filesystem_item &fsi) const override;

  api_error get_filesystem_item_and_file(const std::string &api_path, api_file &apiFile,
                                         filesystem_item &fsi) const override;

  provider_type get_provider_type() const override { return provider_type::sia; }

  std::uint64_t get_total_drive_space() const override { return total_drive_space_; }

  std::uint64_t get_total_item_count() const override;

  bool is_directory(const std::string &api_path) const override;

  bool is_file(const std::string &api_path) const override;

  bool is_online() const override;

  bool is_rename_supported() const override { return true; }

  api_error read_file_bytes(const std::string &api_path, const std::size_t &size,
                            const std::uint64_t &offset, std::vector<char> &buffer,
                            const bool &stop_requested) override;

  api_error remove_directory(const std::string &api_path) override;

  api_error remove_file(const std::string &api_path) override;

  api_error rename_file(const std::string &from_api_path, const std::string &to_api_path) override;

  bool start(api_item_added_callback api_item_added, i_open_file_table *oft) override;

  void stop() override;

  api_error upload_file(const std::string &api_path, const std::string &source_path,
                        const std::string &) override;
};
} // namespace repertory

#endif // INCLUDE_PROVIDERS_SIA_SIAPROVIDER_HPP_
