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
#ifndef INCLUDE_PROVIDERS_S3_S3_PROVIDER_HPP_
#define INCLUDE_PROVIDERS_S3_S3_PROVIDER_HPP_
#if defined(REPERTORY_ENABLE_S3)

#include "db/directory_db.hpp"
#include "providers/base_provider.hpp"
#include "types/repertory.hpp"

namespace repertory {
class app_config;
class i_file_manager;
class i_s3_comm;

class s3_provider final : public base_provider {
public:
  s3_provider(app_config &config, i_s3_comm &s3_comm);

  ~s3_provider() override = default;

private:
  i_s3_comm &s3_comm_;

private:
  std::unique_ptr<directory_db> directory_db_;
  std::unique_ptr<std::thread> background_thread_;

private:
  void create_directories();

  void create_parent_directories(const api_file &file, bool directory);

  void update_item_meta(directory_item &di) const;

protected:
  [[nodiscard]] auto check_file_exists(const std::string &api_path) const
      -> api_error override;

  [[nodiscard]] auto handle_rename_file(const std::string &from_api_path,
                                        const std::string &to_api_path,
                                        const std::string &source_path)
      -> api_error override;

  [[nodiscard]] auto notify_directory_added(const std::string &api_path,
                                            const std::string &api_parent)
      -> api_error override;

  [[nodiscard]] auto notify_file_added(const std::string &api_path,
                                       const std::string &api_parent,
                                       std::uint64_t size)
      -> api_error override;

  [[nodiscard]] auto populate_directory_items(const std::string &api_path,
                                              directory_item_list &list) const
      -> api_error override;

  [[nodiscard]] auto populate_file(const std::string &api_path,
                                   api_file &file) const -> api_error override;

public:
#ifdef REPERTORY_TESTING
  void set_callback(api_item_added_callback cb) { api_item_added_ = cb; }
#endif // REPERTORY_TESTING

  [[nodiscard]] auto create_directory(const std::string &api_path,
                                      api_meta_map &meta) -> api_error override;

  [[nodiscard]] auto create_file(const std::string &api_path,
                                 api_meta_map &meta) -> api_error override;

  [[nodiscard]] auto get_directory_item_count(const std::string &api_path) const
      -> std::uint64_t override;

  [[nodiscard]] auto get_file_list(api_file_list &list) const
      -> api_error override;

  [[nodiscard]] auto get_provider_type() const -> provider_type override {
    return provider_type::s3;
  }

  [[nodiscard]] auto get_total_drive_space() const -> std::uint64_t override {
    return std::numeric_limits<std::int64_t>::max() / std::int64_t(2);
  }

  [[nodiscard]] auto get_total_item_count() const -> std::uint64_t override;

  [[nodiscard]] auto is_direct_only() const -> bool override { return false; }

  [[nodiscard]] auto is_directory(const std::string &api_path,
                                  bool &exists) const -> api_error override;

  [[nodiscard]] auto is_file(const std::string &api_path, bool &exists) const
      -> api_error override;

  [[nodiscard]] auto is_online() const -> bool override;

  [[nodiscard]] auto is_rename_supported() const -> bool override {
    return false;
  }

  [[nodiscard]] auto read_file_bytes(const std::string &api_path,
                                     std::size_t size, std::uint64_t offset,
                                     data_buffer &data,
                                     stop_type &stop_requested)
      -> api_error override;

  [[nodiscard]] auto remove_directory(const std::string &api_path)
      -> api_error override;

  [[nodiscard]] auto remove_file(const std::string &api_path)
      -> api_error override;

  [[nodiscard]] auto start(api_item_added_callback api_item_added,
                           i_file_manager *fm) -> bool override;

  void stop() override;

  [[nodiscard]] auto
  upload_file(const std::string &api_path, const std::string &source_path,
              const std::string &encryption_token, stop_type &stop_requested)
      -> api_error override;
};
} // namespace repertory

#endif // REPERTORY_ENABLE_S3
#endif // INCLUDE_PROVIDERS_S3_S3_PROVIDER_HPP_
