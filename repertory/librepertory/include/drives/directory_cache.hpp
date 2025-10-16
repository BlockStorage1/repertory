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
#ifndef REPERTORY_INCLUDE_DRIVES_DIRECTORY_CACHE_HPP_
#define REPERTORY_INCLUDE_DRIVES_DIRECTORY_CACHE_HPP_

namespace repertory {
class directory_iterator;

class directory_cache final {
public:
  using execute_callback = std::function<void(directory_iterator &)>;

private:
  struct open_directory final {
    std::shared_ptr<directory_iterator> iterator;
    std::vector<std::uint64_t> handles;
  };

public:
  directory_cache() = default;
  ~directory_cache() = default;

  directory_cache(const directory_cache &) = delete;
  directory_cache(directory_cache &&) = delete;
  auto operator=(const directory_cache &) -> directory_cache & = delete;
  auto operator=(directory_cache &&) -> directory_cache & = delete;

private:
  std::unordered_map<std::string, open_directory> directory_lookup_;
  std::recursive_mutex directory_mutex_;

public:
  void execute_action(std::string_view api_path,
                      const execute_callback &execute);

  [[nodiscard]] auto get_directory(std::uint64_t handle)
      -> std::shared_ptr<directory_iterator>;

  auto remove_directory(std::string_view api_path)
      -> std::shared_ptr<directory_iterator>;

  void remove_directory(std::uint64_t handle);

  void set_directory(std::string_view api_path, std::uint64_t handle,
                     std::shared_ptr<directory_iterator> iterator);
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_DRIVES_DIRECTORY_CACHE_HPP_
