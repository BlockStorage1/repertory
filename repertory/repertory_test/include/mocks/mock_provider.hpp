/*
  Copyright <2018-2025> <scott.e.graves@protonmail.com>

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
#ifndef REPERTORY_TEST_INCLUDE_MOCKS_MOCK_PROVIDER_HPP_
#define REPERTORY_TEST_INCLUDE_MOCKS_MOCK_PROVIDER_HPP_

#include "test_common.hpp"

#include "providers/i_provider.hpp"
#include "types/repertory.hpp"

namespace repertory {
class mock_provider : public virtual i_provider {
public:
  mock_provider(bool allow_rename = true) : allow_rename_(allow_rename) {}

private:
  const bool allow_rename_;

public:
  MOCK_METHOD(bool, check_version,
              (std::string & required_version, std::string &returned_version),
              (const, override));

  MOCK_METHOD(api_error, create_directory,
              (const std::string &api_path, api_meta_map &meta), (override));

  MOCK_METHOD(api_error, create_directory_clone_source_meta,
              (const std::string &source_api_path, const std::string &api_path),
              (override));

  MOCK_METHOD(api_error, create_file,
              (const std::string &api_path, api_meta_map &meta), (override));

  MOCK_METHOD(api_error, get_api_path_from_source,
              (const std::string &source_path, std::string &api_path),
              (const, override));

  MOCK_METHOD(std::uint64_t, get_directory_item_count,
              (const std::string &api_path), (const, override));

  MOCK_METHOD(api_error, get_directory_items,
              (const std::string &api_path, directory_item_list &list),
              (const, override));

  MOCK_METHOD(api_error, get_file,
              (const std::string &api_path, api_file &file), (const, override));

  MOCK_METHOD(api_error, get_file_list,
              (api_file_list & list, std::string &marker), (const, override));

  MOCK_METHOD(api_error, get_file_size,
              (const std::string &api_path, std::uint64_t &file_size),
              (const, override));

  MOCK_METHOD(api_error, get_filesystem_item,
              (const std::string &api_path, bool directory,
               filesystem_item &fsi),
              (const, override));

  MOCK_METHOD(api_error, get_filesystem_item_and_file,
              (const std::string &api_path, api_file &file,
               filesystem_item &fsi),
              (const, override));

  MOCK_METHOD(api_error, get_filesystem_item_from_source_path,
              (const std::string &source_path, filesystem_item &fsi),
              (const, override));

  MOCK_METHOD(api_error, get_item_meta,
              (const std::string &api_path, api_meta_map &meta),
              (const, override));

  MOCK_METHOD(api_error, get_item_meta,
              (const std::string &api_path, const std::string &key,
               std::string &value),
              (const, override));

  MOCK_METHOD((std::vector<std::string>), get_pinned_files, (),
              (const, override));

  MOCK_METHOD(provider_type, get_provider_type, (), (const, override));

  MOCK_METHOD(std::uint64_t, get_total_drive_space, (), (const, override));

  MOCK_METHOD(std::uint64_t, get_total_item_count, (), (const, override));

  MOCK_METHOD(std::uint64_t, get_used_drive_space, (), (const, override));

  MOCK_METHOD(bool, is_read_only, (), (const, override));

  MOCK_METHOD(api_error, is_directory,
              (const std::string &api_path, bool &exists), (const, override));

  MOCK_METHOD(api_error, is_file, (const std::string &api_path, bool &exists),
              (const, override));

  bool is_file_writeable(const std::string & /* api_path */) const override {
    return true;
  }

  MOCK_METHOD(bool, is_online, (), (const, override));

  bool is_rename_supported() const override { return allow_rename_; }

  MOCK_METHOD(api_error, read_file_bytes,
              (const std::string &path, std::size_t size, std::uint64_t offset,
               data_buffer &data, stop_type &stop_requested),
              (override));

  MOCK_METHOD(api_error, remove_directory, (const std::string &api_path),
              (override));

  MOCK_METHOD(api_error, remove_file, (const std::string &api_path),
              (override));

  MOCK_METHOD(api_error, remove_item_meta,
              (const std::string &api_path, const std::string &key),
              (override));

  MOCK_METHOD(api_error, rename_file,
              (const std::string &from_api_path,
               const std::string &to_api_path),
              (override));

  MOCK_METHOD(api_error, set_item_meta,
              (const std::string &api_path, const std::string &key,
               const std::string &value),
              (override));

  MOCK_METHOD(api_error, set_item_meta,
              (const std::string &api_path, const api_meta_map &meta),
              (override));

  MOCK_METHOD(bool, start,
              (api_item_added_callback api_item_added, i_file_manager *fm),
              (override));

  MOCK_METHOD(void, stop, (), (override));

  MOCK_METHOD(api_error, upload_file,
              (const std::string &api_path, const std::string &source_path,
               stop_type &stop_requested),
              (override));
};
} // namespace repertory

#endif // REPERTORY_TEST_INCLUDE_MOCKS_MOCK_PROVIDER_HPP_
