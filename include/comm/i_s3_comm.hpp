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
#ifndef INCLUDE_COMM_I_S3_COMM_HPP_
#define INCLUDE_COMM_I_S3_COMM_HPP_
#if defined(REPERTORY_ENABLE_S3)

#include "common.hpp"
#include "types/repertory.hpp"

namespace repertory {
class i_provider;
class i_s3_comm {
  INTERFACE_SETUP(i_s3_comm);

public:
  typedef std::function<std::string(const std::string &api_path)> get_api_file_token_callback;
  typedef std::function<std::string()> get_key_callback;
  typedef std::function<std::string(const std::string &key, const std::string &object_name)>
      get_name_callback;
  typedef std::function<std::uint64_t()> get_size_callback;
  typedef std::function<std::string()> get_token_callback;
  typedef std::function<api_error(const std::string &key)> set_key_callback;

public:
  virtual api_error create_bucket(const std::string &api_path) = 0;

  virtual bool exists(const std::string &api_path, const get_key_callback &get_key) const = 0;

  virtual void get_bucket_name_and_object_name(const std::string &api_path,
                                               const get_key_callback &get_key,
                                               std::string &bucket_name,
                                               std::string &object_name) const = 0;

  virtual std::size_t
  get_directory_item_count(const std::string &api_path,
                           const meta_provider_callback &meta_provider) const = 0;

  virtual api_error get_directory_items(const std::string &api_path,
                                        const meta_provider_callback &meta_provider,
                                        directory_item_list &list) const = 0;

  virtual s3_config get_s3_config() = 0;

  virtual s3_config get_s3_config() const = 0;

  virtual api_error get_file(const std::string &api_path, const get_key_callback &get_key,
                             const get_name_callback &get_name, const get_token_callback &get_token,
                             api_file &file) const = 0;

  virtual api_error get_file_list(const get_api_file_token_callback &get_api_file_token,
                                  const get_name_callback &get_name, api_file_list &list) const = 0;

  virtual bool is_online() const = 0;

  virtual api_error read_file_bytes(const std::string &api_path, const std::size_t &size,
                                    const std::uint64_t &offset, std::vector<char> &data,
                                    const get_key_callback &get_key,
                                    const get_size_callback &get_size,
                                    const get_token_callback &get_token,
                                    const bool &stop_requested) const = 0;

  virtual api_error remove_file(const std::string &api_path, const get_key_callback &get_key) = 0;

  virtual api_error remove_bucket(const std::string &api_path) = 0;

  virtual api_error rename_file(const std::string &api_path, const std::string &new_api_path) = 0;

  virtual api_error upload_file(const std::string &api_path, const std::string &source_path,
                                const std::string &encryption_token,
                                const get_key_callback &get_key, const set_key_callback &set_key,
                                const bool &stop_requested) = 0;
};
} // namespace repertory

#endif // REPERTORY_ENABLE_S3
#endif // INCLUDE_COMM_I_S3_COMM_HPP_
