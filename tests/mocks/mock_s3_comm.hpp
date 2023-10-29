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
#ifndef TESTS_MOCKS_MOCK_S3_COMM_HPP_
#define TESTS_MOCKS_MOCK_S3_COMM_HPP_
#if defined(REPERTORY_ENABLE_S3) && defined(REPERTORY_ENABLE_S3_TESTING)

#include "test_common.hpp"

#include "comm/i_s3_comm.hpp"
#include "types/repertory.hpp"

namespace repertory {
class mock_s3_comm final : public i_s3_comm {
public:
  mock_s3_comm(s3_config cfg) : s3_config_(cfg) {}

private:
  s3_config s3_config_;

public:
  // [[nodiscard]] virtual api_error create_directory(const std::string
  // &api_path) = 0;
  //
  MOCK_METHOD(api_error, create_directory, (const std::string &api_path),
              (override));

  // [[nodiscard]] virtual api_error directory_exists(const std::string
  // &api_path) const = 0;
  //
  MOCK_METHOD(api_error, directory_exists, (const std::string &api_path),
              (const, override));

  // [[nodiscard]] virtual api_error file_exists(const std::string &api_path,
  //                                             const get_key_callback
  //                                             &get_key) const = 0;
  MOCK_METHOD(api_error, file_exists,
              (const std::string &api_path, const get_key_callback &get_key),
              (const, override));

  // [[nodiscard]] virtual std::size_t
  // get_directory_item_count(const std::string &api_path,
  //                          meta_provider_callback meta_provider) const
  //                          = 0;
  //
  MOCK_METHOD(std::size_t, get_directory_item_count,
              (const std::string &api_path,
               meta_provider_callback meta_provider),
              (const, override));

  // [[nodiscard]] virtual api_error get_directory_items(const std::string
  // &api_path,
  //                                                     const
  //                                                     meta_provider_callback
  //                                                     &meta_provider,
  //                                                     directory_item_list
  //                                                     &list) const = 0;
  //
  MOCK_METHOD(api_error, get_directory_items,
              (const std::string &api_path,
               meta_provider_callback meta_provider,
               directory_item_list &list),
              (const, override));

  // [[nodiscard]] virtual api_error
  // get_directory_list(api_file_list &list) const = 0;
  //
  MOCK_METHOD(api_error, get_directory_list, (api_file_list & list),
              (const, override));

  // [[nodiscard]] virtual api_error get_file(const std::string &api_path,
  //                                          const get_key_callback &get_key,
  //                                          const get_name_callback &get_name,
  //                                          const get_token_callback
  //                                          &get_token, api_file &file) const
  //                                          = 0;
  //
  MOCK_METHOD(api_error, get_file,
              (const std::string &api_path, const get_key_callback &get_key,
               const get_name_callback &get_name,
               const get_token_callback &get_token, api_file &file),
              (const, override));

  // [[nodiscard]] virtual api_error
  // get_file_list(const get_api_file_token_callback &get_api_file_token,
  //               const get_name_callback &get_name, api_file_list &list) const
  //               = 0;
  //
  MOCK_METHOD(api_error, get_file_list,
              (const get_api_file_token_callback &get_api_file_token,
               const get_name_callback &get_name, api_file_list &list),
              (const, override));

  // [[nodiscard]] virtual api_error get_object_list(std::vector<directory_item>
  // &list) const = 0;
  //
  MOCK_METHOD(api_error, get_object_list, (std::vector<directory_item> & list),
              (const, override));

  // virtual std::string get_object_name(const std::string &api_path,
  //                                     const get_key_callback &get_key) const
  //                                     = 0;
  //
  MOCK_METHOD(std::string, get_object_name,
              (const std::string &api_path, const get_key_callback &get_key),
              (const, override));

  [[nodiscard]] s3_config get_s3_config() override { return s3_config_; }

  [[nodiscard]] s3_config get_s3_config() const override { return s3_config_; }

  // [[nodiscard]] virtual bool is_online() const = 0;
  //
  MOCK_METHOD(bool, is_online, (), (const, override));

  // [[nodiscard]] virtual api_error
  // read_file_bytes(const std::string &api_path, std::size_t size, const
  // std::uint64_t &offset,
  //                 data_buffer &data, const get_key_callback &get_key,
  //                 const get_size_callback &get_size, const get_token_callback
  //                 &get_token, stop_type &stop_requested) const = 0;
  //
  MOCK_METHOD(api_error, read_file_bytes,
              (const std::string &api_path, std::size_t size,
               std::uint64_t offset, data_buffer &data,
               const get_key_callback &get_key,
               const get_size_callback &get_size,
               const get_token_callback &get_token, stop_type &stop_requested),
              (const, override));

  // [[nodiscard]] virtual api_error remove_directory(const std::string
  // &api_path) = 0;
  //
  MOCK_METHOD(api_error, remove_directory, (const std::string &api_path),
              (override));

  // [[nodiscard]] virtual api_error remove_file(const std::string &api_path,
  //                                             const get_key_callback
  //                                             &get_key) = 0;
  //
  MOCK_METHOD(api_error, remove_file,
              (const std::string &api_path, const get_key_callback &get_key),
              (override));

  // [[nodiscard]] virtual api_error rename_file(const std::string &api_path,
  //                                             const std::string
  //                                             &new_api_path) = 0;
  //
  MOCK_METHOD(api_error, rename_file,
              (const std::string &api_path, const std::string &new_api_path),
              (override));

  // [[nodiscard]] virtual api_error
  // upload_file(const std::string &api_path, const std::string &source_path,
  //             const std::string &encryption_token, const get_key_callback
  //             &get_key, const set_key_callback &set_key, stop_type
  //             &stop_requested) = 0;
  MOCK_METHOD(api_error, upload_file,
              (const std::string &api_path, const std::string &source_path,
               const std::string &encryption_token,
               const get_key_callback &get_key, const set_key_callback &set_key,
               stop_type &stop_requested),
              (override));
};
} // namespace repertory

#endif // REPERTORY_ENABLE_S3_TESTING
#endif // TESTS_MOCKS_MOCK_S3_COMM_HPP_
