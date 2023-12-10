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
#ifndef INCLUDE_PROVIDERS_SIA_SIA_PROVIDER_HPP_
#define INCLUDE_PROVIDERS_SIA_SIA_PROVIDER_HPP_

#include "providers/base_provider.hpp"
#include "types/repertory.hpp"

namespace repertory {
class app_config;
class i_file_manager;
class i_http_comm;

class sia_provider : public base_provider {
public:
  sia_provider(app_config &config, i_http_comm &comm);

  ~sia_provider() override = default;

public:
  sia_provider(const sia_provider &) = delete;
  sia_provider(sia_provider &&) = delete;
  auto operator=(const sia_provider &) -> sia_provider & = delete;
  auto operator=(sia_provider &&) -> sia_provider & = delete;

private:
  [[nodiscard]] auto get_object_info(const std::string &api_path,
                                     json &object_info) const -> api_error;

  [[nodiscard]] auto get_object_list(const std::string &api_path,
                                     nlohmann::json &object_list) const -> bool;

protected:
  [[nodiscard]] auto create_directory_impl(const std::string &api_path,
                                           api_meta_map &meta)
      -> api_error override;

  [[nodiscard]] auto get_directory_items_impl(const std::string &api_path,
                                              directory_item_list &list) const
      -> api_error override;

  [[nodiscard]] auto get_used_drive_space_impl() const
      -> std::uint64_t override;

  [[nodiscard]] auto remove_directory_impl(const std::string &api_path)
      -> api_error override;

  [[nodiscard]] auto remove_file_impl(const std::string &api_path)
      -> api_error override;

  [[nodiscard]] auto upload_file_impl(const std::string &api_path,
                                      const std::string &source_path,
                                      stop_type &stop_requested)
      -> api_error override;

public:
  [[nodiscard]] auto get_directory_item_count(const std::string &api_path) const
      -> std::uint64_t override;

  [[nodiscard]] auto get_file(const std::string &api_path, api_file &file) const
      -> api_error override;

  [[nodiscard]] auto get_file_list(api_file_list &list) const
      -> api_error override;

  [[nodiscard]] auto get_provider_type() const -> provider_type override {
    return provider_type::sia;
  }

  [[nodiscard]] auto get_total_drive_space() const -> std::uint64_t override;

  [[nodiscard]] auto is_direct_only() const -> bool override { return false; }

  [[nodiscard]] auto is_directory(const std::string &api_path,
                                  bool &exists) const -> api_error override;

  [[nodiscard]] auto is_file(const std::string &api_path, bool &exists) const
      -> api_error override;

  [[nodiscard]] auto is_online() const -> bool override;

  [[nodiscard]] auto is_rename_supported() const -> bool override {
    return true;
  }

  [[nodiscard]] auto read_file_bytes(const std::string &api_path,
                                     std::size_t size, std::uint64_t offset,
                                     data_buffer &buffer,
                                     stop_type &stop_requested)
      -> api_error override;

  [[nodiscard]] auto rename_file(const std::string &from_api_path,
                                 const std::string &to_api_path)
      -> api_error override;

  [[nodiscard]] auto start(api_item_added_callback api_item_added,
                           i_file_manager *mgr) -> bool override;

  void stop() override;
};
} // namespace repertory

#endif // INCLUDE_PROVIDERS_SIA_SIA_PROVIDER_HPP_
