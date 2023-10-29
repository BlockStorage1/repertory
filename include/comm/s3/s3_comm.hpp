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
#ifndef INCLUDE_COMM_S3_S3_COMM_HPP_
#define INCLUDE_COMM_S3_S3_COMM_HPP_
#if defined(REPERTORY_ENABLE_S3)

#include "comm/i_http_comm.hpp"
#include "comm/i_s3_comm.hpp"
#include "types/repertory.hpp"

namespace repertory {
class app_config;

class s3_comm final : public i_s3_comm {
public:
  explicit s3_comm(const app_config &config);
  s3_comm(s3_comm &&comm);

  ~s3_comm() override;

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
  std::unique_ptr<i_http_comm> s3_client_;

private:
  mutable std::recursive_mutex cached_directories_mutex_;
  mutable std::unordered_map<std::string, cache_entry> cached_directories_;

protected:
  bool active_ = true;

private:
  void clear_expired_directories();

  [[nodiscard]] auto
  get_cached_directory_item_count(const std::string &api_path,
                                  std::size_t &count) const -> bool;

  [[nodiscard]] auto
  get_cached_directory_items(const std::string &api_path,
                             meta_provider_callback meta_provider,
                             directory_item_list &list) const -> bool;

  [[nodiscard]] auto get_cached_file_exists(const std::string &api_path) const
      -> bool;

  [[nodiscard]] auto grab_directory_items(const std::string &api_path,
                                          meta_provider_callback meta_provider,
                                          directory_item_list &list) const
      -> api_error;

  void raise_begin(const std::string &function_name,
                   const std::string &api_path) const;

  [[nodiscard]] auto raise_end(const std::string &function_name,
                               const std::string &api_path,
                               const api_error &error, long code) const
      -> api_error;

  void remove_cached_directory(const std::string &api_path);

  void set_cached_directory_items(const std::string &api_path,
                                  directory_item_list list) const;

public:
  [[nodiscard]] auto create_directory(const std::string &api_path)
      -> api_error override;

  [[nodiscard]] auto directory_exists(const std::string &api_path) const
      -> api_error override;

  [[nodiscard]] auto file_exists(const std::string &api_path,
                                 const get_key_callback &get_key) const
      -> api_error override;

  [[nodiscard]] auto
  get_directory_item_count(const std::string &api_path,
                           meta_provider_callback meta_provider) const
      -> std::size_t override;

  [[nodiscard]] auto get_directory_items(const std::string &api_path,
                                         meta_provider_callback meta_provider,
                                         directory_item_list &list) const
      -> api_error override;

  [[nodiscard]] auto get_directory_list(api_file_list &list) const
      -> api_error override;

  [[nodiscard]] auto get_file(const std::string &api_path,
                              const get_key_callback &get_key,
                              const get_name_callback &get_name,
                              const get_token_callback &get_token,
                              api_file &file) const -> api_error override;

  [[nodiscard]] auto
  get_file_list(const get_api_file_token_callback &get_api_file_token,
                const get_name_callback &get_name, api_file_list &list) const
      -> api_error override;

  [[nodiscard]] auto get_object_list(std::vector<directory_item> &list) const
      -> api_error override;

  [[nodiscard]] auto get_object_name(const std::string &api_path,
                                     const get_key_callback &getKey) const
      -> std::string override;

  [[nodiscard]] auto get_s3_config() -> s3_config override {
    return s3_config_;
  }

  [[nodiscard]] auto get_s3_config() const -> s3_config override {
    return s3_config_;
  }

  [[nodiscard]] auto is_online() const -> bool override {
    // TODO implement this
    return true;
  }

  [[nodiscard]] auto read_file_bytes(
      const std::string &api_path, std::size_t size, std::uint64_t offset,
      data_buffer &data, const get_key_callback &get_key,
      const get_size_callback &get_size, const get_token_callback &get_token,
      stop_type &stop_requested) const -> api_error override;

  [[nodiscard]] auto remove_directory(const std::string &api_path)
      -> api_error override;

  [[nodiscard]] auto remove_file(const std::string &api_path,
                                 const get_key_callback &get_key)
      -> api_error override;

  [[nodiscard]] auto rename_file(const std::string &api_path,
                                 const std::string &new_api_path)
      -> api_error override;

  [[nodiscard]] auto
  upload_file(const std::string &api_path, const std::string &source_path,
              const std::string &encryption_token,
              const get_key_callback &get_key, const set_key_callback &set_key,
              stop_type &stop_requested) -> api_error override;
};
} // namespace repertory

#endif // REPERTORY_ENABLE_S3
#endif // INCLUDE_COMM_S3_S3_COMM_HPP_
