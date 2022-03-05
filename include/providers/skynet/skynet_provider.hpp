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
#ifndef INCLUDE_PROVIDERS_SKYNET_SKYNET_PROVIDER_HPP_
#define INCLUDE_PROVIDERS_SKYNET_SKYNET_PROVIDER_HPP_
#if defined(REPERTORY_ENABLE_SKYNET)

#include "common.hpp"
#include "db/directory_db.hpp"
#include "db/meta_db.hpp"
#include "events/event_system.hpp"
#include "providers/base_provider.hpp"
#include "upload/upload_manager.hpp"

namespace repertory {
class app_config;
class i_comm;
class skynet_provider final : public base_provider {
  E_CONSUMER();

public:
  skynet_provider(app_config &config, i_comm &comm);

  ~skynet_provider() override { E_CONSUMER_RELEASE(); }

private:
  i_comm &comm_;
  directory_db directory_db_;
  upload_manager upload_manager_;
  bool stop_requested_ = false;
  std::atomic<std::size_t> next_download_index_;
  std::atomic<std::size_t> next_upload_index_;
  std::shared_ptr<std::vector<host_config>> download_list_;
  std::shared_ptr<std::vector<host_config>> upload_list_;
  mutable std::mutex portal_mutex_;

private:
  std::size_t get_retry_count() const;

  void populate_api_file(api_file &file) const;

  void process_export(json &result, const std::string &api_path) const;

  void upload_completed(const std::string &api_path, const std::string &, const json &data);

  api_error upload_handler(const upload_manager::upload &upload, json &data, json &error);

protected:
  void notify_directory_added(const std::string &api_path, const std::string &api_parent) override;

  api_error notify_file_added(const std::string &, const std::string &,
                              const std::uint64_t &) override;

public:
  api_error create_directory(const std::string &api_path, const api_meta_map &meta) override;

  api_error create_file(const std::string &api_path, api_meta_map &meta) override;

  json export_all() const;

  json export_list(const std::vector<std::string> &api_path_list) const;

  std::uint64_t get_directory_item_count(const std::string &api_path) const override;

  api_error get_directory_items(const std::string &api_path,
                                directory_item_list &list) const override;

  api_error get_file(const std::string &api_path, api_file &file) const override;

  api_error get_file_list(api_file_list &list) const override;

  api_error get_file_size(const std::string &api_path, std::uint64_t &file_size) const override;

  host_config get_host_config(const bool &upload);

  provider_type get_provider_type() const override { return provider_type::skynet; }

  api_error get_skynet_metadata(const std::string &skylink, json &json_meta);

  std::uint64_t get_total_drive_space() const override {
    return std::numeric_limits<std::int64_t>::max() / std::int64_t(2);
  }

  std::uint64_t get_total_item_count() const override {
    return directory_db_.get_total_item_count();
  }

  api_error import_skylink(const skylink_import &si);

  bool is_directory(const std::string &api_path) const override;

  bool is_file(const std::string &api_path) const override;

  bool is_file_writeable(const std::string &api_path) const override;

  bool is_online() const override { return true; }

  bool is_processing(const std::string &api_path) const;

  bool is_rename_supported() const override { return true; }

  api_error read_file_bytes(const std::string &api_path, const std::size_t &size,
                            const std::uint64_t &offset, std::vector<char> &data,
                            const bool &stop_requested) override;

  api_error remove_directory(const std::string &api_path) override;

  api_error remove_file(const std::string &api_path) override;

  api_error rename_file(const std::string &from_api_path, const std::string &to_api_path) override;

  bool start(api_item_added_callback api_item_added, i_open_file_table *oft) override;

  void stop() override;

  bool update_portal_list();

  api_error upload_file(const std::string &api_path, const std::string &source_path,
                        const std::string &encryption_token) override;
};
} // namespace repertory

#endif // REPERTORY_ENABLE_SKYNET
#endif // INCLUDE_PROVIDERS_SKYNET_SKYNET_PROVIDER_HPP_
