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
#ifndef INCLUDE_COMM_AWS_S3_AWS_S3_COMM_HPP_
#define INCLUDE_COMM_AWS_S3_AWS_S3_COMM_HPP_
#if defined(REPERTORY_ENABLE_S3)

#include "common.hpp"
#include "comm/i_s3_comm.hpp"

namespace repertory {
class app_config;
class aws_s3_comm final : public virtual i_s3_comm {
public:
  explicit aws_s3_comm(const app_config &config);

  ~aws_s3_comm() override;

private:
  struct cache_entry final {
    std::chrono::system_clock::time_point expiration;
    directory_item_list items;

    void reset_timeout(std::chrono::seconds timeout) {
      timeout = std::max(std::chrono::seconds(5u), timeout);
      expiration = std::chrono::system_clock::now() + timeout;
    }
  };

private:
  const app_config &config_;
  s3_config s3_config_;
  Aws::SDKOptions sdk_options_;
  std::unique_ptr<Aws::S3::S3Client> s3_client_;
  mutable std::recursive_mutex cached_directories_mutex_;
  mutable std::unordered_map<std::string, cache_entry> cached_directories_;

private:
  void clear_expired_directories();

  bool get_cached_directory_item_count(const std::string &api_path, std::size_t &count) const;

  bool get_cached_directory_items(const std::string &api_path,
                                  const meta_provider_callback &meta_provider,
                                  directory_item_list &list) const;

  bool get_cached_file_exists(const std::string &api_path, bool &exists) const;

  api_error grab_directory_items(const std::string &api_path,
                                 const meta_provider_callback &meta_provider,
                                 directory_item_list &list) const;

  api_error get_file_list(const std::string &bucket_name,
                          const get_api_file_token_callback &get_api_file_token,
                          const get_name_callback &get_name, api_file_list &list) const;

  void remove_cached_directory(const std::string &api_path);

  void set_cached_directory_items(const std::string &api_path, directory_item_list list) const;

public:
  api_error create_bucket(const std::string &api_path) override;

  bool exists(const std::string &api_path, const get_key_callback &get_key) const override;

  void get_bucket_name_and_object_name(const std::string &api_path, const get_key_callback &getKey,
                                       std::string &bucketName,
                                       std::string &objectName) const override;

  std::size_t get_directory_item_count(const std::string &api_path,
                                       const meta_provider_callback &meta_provider) const override;

  api_error get_directory_items(const std::string &api_path,
                                const meta_provider_callback &meta_provider,
                                directory_item_list &list) const override;

  api_error get_file(const std::string &api_path, const get_key_callback &get_key,
                     const get_name_callback &get_name, const get_token_callback &get_token,
                     api_file &file) const override;

  api_error get_file_list(const get_api_file_token_callback &get_api_file_token,
                          const get_name_callback &get_name, api_file_list &list) const override;

  s3_config get_s3_config() override { return s3_config_; }

  s3_config get_s3_config() const override { return s3_config_; }

  bool is_online() const override { return s3_client_->ListBuckets().IsSuccess(); }

  api_error read_file_bytes(const std::string &api_path, const std::size_t &size,
                            const std::uint64_t &offset, std::vector<char> &data,
                            const get_key_callback &get_key, const get_size_callback &get_size,
                            const get_token_callback &get_token,
                            const bool &stop_requested) const override;

  api_error remove_bucket(const std::string &api_path) override;

  api_error remove_file(const std::string &api_path, const get_key_callback &get_key) override;

  api_error rename_file(const std::string &api_path, const std::string &new_api_path) override;

  api_error upload_file(const std::string &api_path, const std::string &source_path,
                        const std::string &encryption_token, const get_key_callback &get_key,
                        const set_key_callback &set_key, const bool &stop_requested) override;
};
} // namespace repertory

#endif // REPERTORY_ENABLE_S3
#endif // INCLUDE_COMM_AWS_S3_AWS_S3_COMM_HPP_
