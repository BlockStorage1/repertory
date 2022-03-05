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
#ifndef INCLUDE_PROVIDERS_I_PROVIDER_HPP_
#define INCLUDE_PROVIDERS_I_PROVIDER_HPP_

#include "common.hpp"
#include "types/repertory.hpp"

namespace repertory {
class i_open_file_table;
class i_provider {
  INTERFACE_SETUP(i_provider);

public:
  virtual api_error create_directory(const std::string &api_path, const api_meta_map &meta) = 0;

  virtual api_error create_directory_clone_source_meta(const std::string &source_api_path,
                                                       const std::string &api_path) = 0;

  virtual api_error create_file(const std::string &api_path, api_meta_map &meta) = 0;

  virtual api_error get_api_path_from_source(const std::string &source_path,
                                             std::string &api_path) const = 0;

  virtual api_error get_file_list(api_file_list &list) const = 0;

  virtual std::uint64_t get_directory_item_count(const std::string &api_path) const = 0;

  virtual api_error get_directory_items(const std::string &api_path,
                                        directory_item_list &list) const = 0;

  virtual api_error get_file(const std::string &api_path, api_file &file) const = 0;

  virtual api_error get_file_size(const std::string &api_path, std::uint64_t &file_size) const = 0;

  virtual api_error get_filesystem_item(const std::string &api_path, const bool &directory,
                                        filesystem_item &fsi) const = 0;

  virtual api_error get_filesystem_item_and_file(const std::string &api_path, api_file &file,
                                                 filesystem_item &fsi) const = 0;

  virtual api_error get_filesystem_item_from_source_path(const std::string &source_path,
                                                         filesystem_item &fsi) const = 0;

  virtual std::vector<std::string> get_pinned_files() const = 0;

  virtual api_error get_item_meta(const std::string &api_path, api_meta_map &meta) const = 0;

  virtual api_error get_item_meta(const std::string &api_path, const std::string &key,
                                  std::string &value) const = 0;

  virtual std::uint64_t get_total_drive_space() const = 0;

  virtual std::uint64_t get_total_item_count() const = 0;

  virtual provider_type get_provider_type() const = 0;

  virtual std::uint64_t get_used_drive_space() const = 0;

  virtual bool is_directory(const std::string &api_path) const = 0;

  virtual bool is_file(const std::string &api_path) const = 0;

  virtual bool is_file_writeable(const std::string &api_path) const = 0;

  virtual bool is_online() const = 0;

  virtual bool is_rename_supported() const = 0;

  virtual api_error read_file_bytes(const std::string &api_path, const std::size_t &size,
                                    const std::uint64_t &offset, std::vector<char> &data,
                                    const bool &stop_requested) = 0;

  virtual api_error remove_directory(const std::string &api_path) = 0;

  virtual api_error remove_file(const std::string &api_path) = 0;

  virtual api_error remove_item_meta(const std::string &api_path, const std::string &key) = 0;

  virtual api_error rename_file(const std::string &fromApiPath, const std::string &toApiPath) = 0;

  virtual api_error set_item_meta(const std::string &api_path, const std::string &key,
                                  const std::string &value) = 0;

  virtual api_error set_item_meta(const std::string &api_path, const api_meta_map &meta) = 0;

  virtual api_error set_source_path(const std::string &api_path,
                                    const std::string &source_path) = 0;

  virtual bool start(api_item_added_callback api_item_added, i_open_file_table *oft) = 0;

  virtual void stop() = 0;

  virtual api_error upload_file(const std::string &api_path, const std::string &source_path,
                               const std::string &encryption_token) = 0;
};
} // namespace repertory

#endif // INCLUDE_PROVIDERS_I_PROVIDER_HPP_
