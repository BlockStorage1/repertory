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
#ifndef INCLUDE_COMM_I_S3_COMM_HPP_
#define INCLUDE_COMM_I_S3_COMM_HPP_
#if defined(REPERTORY_ENABLE_S3)

#include "types/repertory.hpp"
#include "types/s3.hpp"

namespace repertory {
class i_provider;

class i_s3_comm {
  INTERFACE_SETUP(i_s3_comm);

public:
  [[nodiscard]] virtual auto create_directory(const std::string &api_path)
      -> api_error = 0;

  [[nodiscard]] virtual auto directory_exists(const std::string &api_path) const
      -> api_error = 0;

  [[nodiscard]] virtual auto file_exists(const std::string &api_path,
                                         const get_key_callback &get_key) const
      -> api_error = 0;

  [[nodiscard]] virtual auto
  get_directory_item_count(const std::string &api_path,
                           meta_provider_callback meta_provider) const
      -> std::size_t = 0;

  [[nodiscard]] virtual auto
  get_directory_items(const std::string &api_path,
                      meta_provider_callback meta_provider,
                      directory_item_list &list) const -> api_error = 0;

  [[nodiscard]] virtual auto get_directory_list(api_file_list &list) const
      -> api_error = 0;

  [[nodiscard]] virtual auto get_file(const std::string &api_path,
                                      const get_key_callback &get_key,
                                      const get_name_callback &get_name,
                                      const get_token_callback &get_token,
                                      api_file &file) const -> api_error = 0;

  [[nodiscard]] virtual auto
  get_file_list(const get_api_file_token_callback &get_api_file_token,
                const get_name_callback &get_name, api_file_list &list) const
      -> api_error = 0;

  [[nodiscard]] virtual auto
  get_object_list(std::vector<directory_item> &list) const -> api_error = 0;

  [[nodiscard]] virtual auto
  get_object_name(const std::string &api_path,
                  const get_key_callback &get_key) const -> std::string = 0;

  [[nodiscard]] virtual auto get_s3_config() -> s3_config = 0;

  [[nodiscard]] virtual auto get_s3_config() const -> s3_config = 0;

  [[nodiscard]] virtual auto is_online() const -> bool = 0;

  [[nodiscard]] virtual auto read_file_bytes(
      const std::string &api_path, std::size_t size, std::uint64_t offset,
      data_buffer &data, const get_key_callback &get_key,
      const get_size_callback &get_size, const get_token_callback &get_token,
      stop_type &stop_requested) const -> api_error = 0;

  [[nodiscard]] virtual auto remove_directory(const std::string &api_path)
      -> api_error = 0;

  [[nodiscard]] virtual auto remove_file(const std::string &api_path,
                                         const get_key_callback &get_key)
      -> api_error = 0;

  [[nodiscard]] virtual auto rename_file(const std::string &api_path,
                                         const std::string &new_api_path)
      -> api_error = 0;

  [[nodiscard]] virtual auto
  upload_file(const std::string &api_path, const std::string &source_path,
              const std::string &encryption_token,
              const get_key_callback &get_key, const set_key_callback &set_key,
              stop_type &stop_requested) -> api_error = 0;
};
} // namespace repertory

#endif // REPERTORY_ENABLE_S3
#endif // INCLUDE_COMM_I_S3_COMM_HPP_
