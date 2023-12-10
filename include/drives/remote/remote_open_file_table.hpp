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
#ifndef INCLUDE_DRIVES_REMOTE_REMOTE_OPEN_FILE_TABLE_HPP_
#define INCLUDE_DRIVES_REMOTE_REMOTE_OPEN_FILE_TABLE_HPP_

#include "types/remote.hpp"
#include "types/repertory.hpp"

namespace repertory {
class remote_open_file_table {
protected:
  remote_open_file_table() = default;

  virtual ~remote_open_file_table() = default;

protected:
  struct compat_open_info {
    std::size_t count = 0u;
    std::string client_id = "";
    std::string path;
  };

  struct open_info {
    std::size_t count = 0u;
    std::string client_id = "";
    PVOID directory_buffer = nullptr;
    std::string path;
  };

private:
  std::unordered_map<remote::file_handle, compat_open_info> compat_lookup_;
  std::recursive_mutex compat_mutex_;
  std::unordered_map<std::string, std::vector<void *>> directory_lookup_;
  std::recursive_mutex directory_mutex_;
  std::unordered_map<native_handle, open_info> file_lookup_;
  mutable std::recursive_mutex file_mutex_;

protected:
  void add_directory(const std::string &client_id, void *dir);

  void close_all(const std::string &client_id);

  virtual void delete_open_directory(void *dir) = 0;

#ifdef _WIN32
  [[nodiscard]] auto get_directory_buffer(const native_handle &handle,
                                          PVOID *&buffer) -> bool;
#endif // _WIN32

  [[nodiscard]] auto get_open_file_path(const native_handle &handle)
      -> std::string;

  [[nodiscard]] auto get_open_info(const native_handle &handle, open_info &oi)
      -> bool;

  [[nodiscard]] auto has_open_directory(const std::string &client_id, void *dir)
      -> bool;

  [[nodiscard]] auto has_compat_open_info(const remote::file_handle &handle,
                                          int error_return) -> int;

  template <typename error_type>
  [[nodiscard]] auto has_open_info(const native_handle &handle,
                                   const error_type &error_return)
      -> error_type {
    recur_mutex_lock file_lock(file_mutex_);
    return ((file_lookup_.find(handle) == file_lookup_.end()) ? error_return
                                                              : 0);
  }

  void remove_all(const std::string &file_path);

  void remove_compat_open_info(const remote::file_handle &handle);

  auto remove_directory(const std::string &client_id, void *dir) -> bool;

  void remove_open_info(const native_handle &handle);

  void set_client_id(const native_handle &handle, const std::string &client_id);

  void set_compat_client_id(const remote::file_handle &handle,
                            const std::string &client_id);

  void set_compat_open_info(const remote::file_handle &handle,
                            const std::string &file_path);

  void set_open_info(const native_handle &handle, open_info oi);

public:
  [[nodiscard]] auto get_open_file_count(const std::string &file_path) const
      -> std::size_t;
};
} // namespace repertory

#endif // INCLUDE_DRIVES_REMOTE_REMOTE_OPEN_FILE_TABLE_HPP_
