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
#ifndef INCLUDE_DRIVES_DIRECTORY_CACHE_HPP_
#define INCLUDE_DRIVES_DIRECTORY_CACHE_HPP_

#include "utils/single_thread_service_base.hpp"

namespace repertory {
class directory_iterator;

class directory_cache final : public single_thread_service_base {
public:
  using execute_callback = std::function<void(directory_iterator &)>;

private:
  struct open_directory {
    directory_iterator *iterator;
    std::chrono::system_clock::time_point last_update =
        std::chrono::system_clock::now();
  };

public:
  directory_cache() : single_thread_service_base("directory_cache") {}

  ~directory_cache() override = default;

private:
  std::unordered_map<std::string, open_directory> directory_lookup_;
  std::recursive_mutex directory_mutex_;
  std::unique_ptr<std::thread> refresh_thread_;

protected:
  void service_function() override;

public:
  void execute_action(const std::string &api_path,
                      const execute_callback &execute);

  [[nodiscard]] auto remove_directory(const std::string &api_path)
      -> directory_iterator *;

  void remove_directory(directory_iterator *iterator);

  void set_directory(const std::string &api_path, directory_iterator *iterator);
};
} // namespace repertory

#endif // INCLUDE_DRIVES_DIRECTORY_CACHE_HPP_
