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
#ifndef INCLUDE_DRIVES_I_OPEN_FILE_TABLE_HPP_
#define INCLUDE_DRIVES_I_OPEN_FILE_TABLE_HPP_

#include "common.hpp"
#include "types/repertory.hpp"

namespace repertory {
class i_provider;
class i_open_file_table {
  INTERFACE_SETUP(i_open_file_table);

public:
  typedef std::function<bool(i_open_file_table &oft, i_provider &provider)>
      locked_operation_callback;

public:
  virtual void close(const std::uint64_t &handle) = 0;

  virtual bool contains_restore(const std::string &api_path) const = 0;

  virtual bool evict_file(const std::string &api_path) = 0;

  virtual void force_schedule_upload(const filesystem_item &fsi) = 0;

  virtual directory_item_list get_directory_items(const std::string &api_path) const = 0;

  virtual std::uint64_t get_open_count(const std::string &api_path) const = 0;

  virtual bool get_open_file(const std::string &api_path, filesystem_item *&fsi) = 0;

  virtual std::unordered_map<std::string, std::size_t> get_open_files() const = 0;

  virtual bool has_no_open_file_handles() const = 0;

  virtual api_error open(const filesystem_item &fsi, std::uint64_t &handle) = 0;

  virtual bool perform_locked_operation(locked_operation_callback locked_operation) = 0;

  virtual api_error set_item_meta(const std::string &api_path, const std::string &key,
                                  const std::string &value) = 0;

  virtual void update_directory_item(directory_item &di) const = 0;
};
} // namespace repertory

#endif // INCLUDE_DRIVES_I_OPEN_FILE_TABLE_HPP_
