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
#ifndef TESTS_MOCKS_MOCKPROVIDER_HPP_
#define TESTS_MOCKS_MOCKPROVIDER_HPP_

#include "providers/i_provider.hpp"
#include "test_common.hpp"

namespace repertory {
class mock_provider : public virtual i_provider {
public:
  mock_provider(const bool &allow_rename = true) : allow_rename_(allow_rename) {}

private:
  const bool allow_rename_;

public:
  MOCK_METHOD2(create_directory, api_error(const std::string &api_path, const api_meta_map &meta));
  MOCK_METHOD2(create_directory_clone_source_meta,
               api_error(const std::string &source_api_path, const std::string &api_path));
  MOCK_METHOD2(create_file, api_error(const std::string &api_path, const api_meta_map &meta));
  MOCK_CONST_METHOD2(get_api_path_from_source,
                     api_error(const std::string &source_path, std::string &api_path));
  MOCK_CONST_METHOD1(get_directory_item_count, std::uint64_t(const std::string &api_path));
  MOCK_CONST_METHOD2(get_directory_items,
                     api_error(const std::string &api_path, directory_item_list &list));
  MOCK_CONST_METHOD2(get_file, api_error(const std::string &api_path, ApiFile &apiFile));
  MOCK_CONST_METHOD1(get_file_list, api_error(api_file_list &list));
  MOCK_CONST_METHOD2(get_file_size,
                     api_error(const std::string &api_path, std::uint64_t &file_size));
  MOCK_CONST_METHOD3(get_filesystem_item, api_error(const std::string &api_path,
                                                    const bool &directory, filesystem_item &fsi));
  MOCK_CONST_METHOD2(get_filesystem_item_from_source_path,
                     api_error(const std::string &source_path, filesystem_item &fsi));
  MOCK_CONST_METHOD2(get_item_meta, api_error(const std::string &api_path, api_meta_map &meta));
  MOCK_CONST_METHOD3(get_item_meta, api_error(const std::string &api_path, const std::string &key,
                                              std::string &value));
  MOCK_CONST_METHOD0(get_pinned_files, std::vector<std::string>());
  MOCK_CONST_METHOD0(get_provider_type, provider_type());
  MOCK_CONST_METHOD0(get_total_drive_space, std::uint64_t());
  MOCK_CONST_METHOD0(get_total_item_count, std::uint64_t());
  MOCK_CONST_METHOD0(get_used_drive_space, std::uint64_t());
  MOCK_CONST_METHOD1(is_directory, bool(const std::string &api_path));
  MOCK_CONST_METHOD1(is_file, bool(const std::string &api_path));
  bool is_file_writeable(const std::string &api_path) const override { return true; }
  MOCK_CONST_METHOD0(IsOnline, bool());
  bool is_rename_supported() const override { return allow_rename_; }
  MOCK_METHOD5(read_file_bytes, api_error(const std::string &path, const std::size_t &size,
                                          const std::uint64_t &offset, std::vector<char> &data,
                                          const bool &stop_requested));
  MOCK_METHOD1(remove_directory, api_error(const std::string &api_path));
  MOCK_METHOD1(remove_file, api_error(const std::string &api_path));
  MOCK_METHOD2(remove_item_meta, api_error(const std::string &api_path, const std::string &key));
  MOCK_METHOD2(rename_file,
               api_error(const std::string &from_api_path, const std::string &to_api_path));
  MOCK_METHOD3(set_item_meta, api_error(const std::string &api_path, const std::string &key,
                                        const std::string &value));
  MOCK_METHOD2(set_item_meta, api_error(const std::string &api_path, const api_meta_map &meta));
  MOCK_METHOD2(set_source_path,
               api_error(const std::string &api_path, const std::string &source_path));
  MOCK_METHOD2(start, bool(api_item_added_callback api_item_added, i_open_file_table *oft));
  MOCK_METHOD0(stop, void());
  MOCK_METHOD3(upload_file, api_error(const std::string &api_path, const std::string &source_path,
                                      const std::string &encryption_token));
};
} // namespace repertory

#endif // TESTS_MOCKS_MOCKPROVIDER_HPP_
