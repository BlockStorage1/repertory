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
#ifndef INCLUDE_PROVIDERS_S3_S3_PROVIDER_HPP_
#define INCLUDE_PROVIDERS_S3_S3_PROVIDER_HPP_
#if defined(REPERTORY_ENABLE_S3)

#include "common.hpp"
#include "db/directory_db.hpp"
#include "providers/base_provider.hpp"
#include "upload/upload_manager.hpp"

namespace repertory {
class app_config;
class i_s3_comm;
class s3_provider final : public base_provider {
public:
  s3_provider(app_config &config, i_s3_comm &s3_comm);

private:
  i_s3_comm &s3_comm_;
  directory_db directory_db_;
  upload_manager upload_manager_;
  std::uint64_t total_item_count_ = 0u;

private:
  void build_directories();

  void update_item_meta(directory_item &di, const bool &format_path) const;

  void upload_completed(const std::string &, const std::string &, const json &) {}

  api_error upload_handler(const upload_manager::upload &upload, json &, json &);

protected:
  std::string format_api_path(std::string api_path) const override;

  void notify_directory_added(const std::string &api_path, const std::string &api_parent) override;

  api_error notify_file_added(const std::string &api_path, const std::string &api_parent,
                              const std::uint64_t &size) override;

  std::string &restore_api_path(std::string &api_path) const override;

public:
  api_error create_directory(const std::string &api_path, const api_meta_map &meta) override;

  api_error create_file(const std::string &api_path, api_meta_map &meta) override;

  api_error get_file_list(api_file_list &list) const override;

  std::uint64_t get_directory_item_count(const std::string &api_path) const override;

  api_error get_directory_items(const std::string &api_path,
                                directory_item_list &list) const override;

  api_error get_file(const std::string &api_path, api_file &file) const override;

  api_error get_file_size(const std::string &api_path, std::uint64_t &file_size) const override;

  provider_type get_provider_type() const override { return provider_type::s3; }

  std::uint64_t get_total_drive_space() const override {
    return std::numeric_limits<std::int64_t>::max() / std::int64_t(2);
  }

  std::uint64_t get_total_item_count() const override { return total_item_count_; }

  bool is_directory(const std::string &api_path) const override;

  bool is_file(const std::string &api_path) const override;

  bool is_online() const override;

  bool is_processing(const std::string &api_path) const;

  bool is_rename_supported() const override { return false; }

  api_error read_file_bytes(const std::string &api_path, const std::size_t &size,
                            const std::uint64_t &offset, std::vector<char> &data,
                            const bool &stop_requested) override;

  api_error remove_directory(const std::string &api_path) override;

  api_error remove_file(const std::string &api_path) override;

  api_error rename_file(const std::string &from_api_path, const std::string &to_api_path) override;

  bool start(api_item_added_callback api_item_added, i_open_file_table *oft) override;

  void stop() override;

  api_error upload_file(const std::string &api_path, const std::string &source_path,
                        const std::string &encryption_token) override;
};
} // namespace repertory

#endif // REPERTORY_ENABLE_S3
#endif // INCLUDE_PROVIDERS_S3_S3_PROVIDER_HPP_
