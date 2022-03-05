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
#ifndef INCLUDE_DRIVES_WINFSP_I_WINFSP_DRIVE_HPP_
#define INCLUDE_DRIVES_WINFSP_I_WINFSP_DRIVE_HPP_
#ifdef _WIN32

#include "common.hpp"
#include "types/remote.hpp"
#include "types/repertory.hpp"

namespace repertory {
class i_winfsp_drive {
  INTERFACE_SETUP(i_winfsp_drive);

public:
  virtual std::uint64_t get_directory_item_count(const std::string &api_path) const = 0;

  virtual directory_item_list get_directory_items(const std::string &api_path) const = 0;

  virtual std::uint64_t get_file_size(const std::string &api_path) const = 0;

  virtual api_error get_item_meta(const std::string &api_path, const std::string &name,
                                  std::string &value) const = 0;

  virtual api_error get_item_meta(const std::string &api_path, api_meta_map &meta) const = 0;

  virtual NTSTATUS get_security_by_name(PWSTR file_name, PUINT32 attributes,
                                        PSECURITY_DESCRIPTOR descriptor,
                                        std::uint64_t *descriptor_size) = 0;

  virtual std::uint64_t get_total_drive_space() const = 0;

  virtual std::uint64_t get_total_item_count() const = 0;

  virtual std::uint64_t get_used_drive_space() const = 0;

  virtual void get_volume_info(UINT64 &total_size, UINT64 &free_size,
                               std::string &volume_label) const = 0;

  virtual api_error populate_file_info(const std::string &api_path, remote::file_info &fi) = 0;
};
} // namespace repertory

#endif // _WIN32
#endif // INCLUDE_DRIVES_WINFSP_I_WINFSP_DRIVE_HPP_
