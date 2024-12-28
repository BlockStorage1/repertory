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
#ifndef REPERTORY_INCLUDE_DRIVES_DIRECTORY_CACHE_HPP_
#define REPERTORY_INCLUDE_DRIVES_DIRECTORY_CACHE_HPP_

#include "utils/single_thread_service_base.hpp"

namespace repertory {
class directory_iterator;

class directory_cache final : public single_thread_service_base {
public:
  using execute_callback = std::function<void(directory_iterator &)>;

private:
  struct open_directory final {
    std::shared_ptr<directory_iterator> iterator;
    std::vector<std::uint64_t> handles;
    std::chrono::system_clock::time_point last_update{
        std::chrono::system_clock::now()};
  };

public:
  directory_cache() : single_thread_service_base("directory_cache") {}
  ~directory_cache() override = default;

  directory_cache(const directory_cache &) = delete;
  directory_cache(directory_cache &&) = delete;
  auto operator=(const directory_cache &) -> directory_cache & = delete;
  auto operator=(directory_cache &&) -> directory_cache & = delete;

private:
  std::unordered_map<std::string, open_directory> directory_lookup_;
  std::recursive_mutex directory_mutex_;
  std::unique_ptr<std::thread> refresh_thread_;

protected:
  void service_function() override;

public:
  void execute_action(const std::string &api_path,
                      const execute_callback &execute);

  [[nodiscard]] auto get_directory(std::uint64_t handle)
      -> std::shared_ptr<directory_iterator>;

  [[nodiscard]] auto remove_directory(const std::string &api_path)
      -> std::shared_ptr<directory_iterator>;

  void remove_directory(std::uint64_t handle);

  void set_directory(const std::string &api_path, std::uint64_t handle,
                     std::shared_ptr<directory_iterator> iterator);
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_DRIVES_DIRECTORY_CACHE_HPP_
