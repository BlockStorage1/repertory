/*
  Copyright <2018-2024> <scott.e.graves@protonmail.com>

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
#ifndef REPERTORY_INCLUDE_FILE_MANAGER_I_FILE_MANAGER_HPP_
#define REPERTORY_INCLUDE_FILE_MANAGER_I_FILE_MANAGER_HPP_

#include "types/repertory.hpp"

namespace repertory {
class i_provider;

class i_file_manager {
  INTERFACE_SETUP(i_file_manager);

public:
  [[nodiscard]] virtual auto evict_file(const std::string &api_path)
      -> bool = 0;

  [[nodiscard]] virtual auto
  get_directory_items(const std::string &api_path) const
      -> directory_item_list = 0;

  [[nodiscard]] virtual auto get_open_files() const
      -> std::unordered_map<std::string, std::size_t> = 0;

  [[nodiscard]] virtual auto has_no_open_file_handles() const -> bool = 0;

  [[nodiscard]] virtual auto is_processing(const std::string &api_path) const
      -> bool = 0;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_FILE_MANAGER_I_FILE_MANAGER_HPP_
