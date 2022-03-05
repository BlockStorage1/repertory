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
#ifndef TESTS_MOCKS_MOCK_OPEN_FILE_TABLE_HPP_
#define TESTS_MOCKS_MOCK_OPEN_FILE_TABLE_HPP_

#include "download/download_manager.hpp"
#include "drives/i_open_file_table.hpp"
#include "test_common.hpp"

namespace repertory {
class mock_open_file_table : public virtual i_open_file_table {
public:
  explicit mock_open_file_table(download_manager *dm = nullptr, filesystem_item *fsi = nullptr)
      : dm_(dm), fsi_(fsi) {}

private:
  download_manager *dm_;
  filesystem_item *fsi_;

public:
  MOCK_CONST_METHOD1(get_open_count, std::uint64_t(const std::string &api_path));

  MOCK_CONST_METHOD0(has_no_open_file_handles, bool());

  MOCK_METHOD1(close, void(const std::uint64_t &handle));

  MOCK_CONST_METHOD1(contains_restore, bool(const std::string &api_path));

  MOCK_METHOD1(evict_file, bool(const std::string &api_path));

  MOCK_CONST_METHOD1(get_directory_items, directory_item_list(const std::string &api_path));

  MOCK_METHOD2(get_open_file, bool(const std::string &api_path, struct filesystem_item *&fsi));

  MOCK_CONST_METHOD0(get_open_files, std::unordered_map<std::string, std::size_t>());

  MOCK_METHOD1(force_schedule_upload, void(const struct filesystem_item &fsi));

  MOCK_METHOD2(open, api_error(const struct filesystem_item &fsi, std::uint64_t &handle));

  MOCK_METHOD3(set_item_meta, api_error(const std::string &api_path, const std::string &key,
                                        const std::string &value));
  bool perform_locked_operation(locked_operation_callback /*locked_operation*/) override {
    if (fsi_ && dm_) {
      fsi_->source_path = dm_->get_source_path(fsi_->api_path);
    }
    return false;
  }

  void update_directory_item(directory_item &) const override {}
};
} // namespace repertory

#endif // TESTS_MOCKS_MOCK_OPEN_FILE_TABLE_HPP_
