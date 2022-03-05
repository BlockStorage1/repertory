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
#ifndef INCLUDE_DRIVES_DIRECTORY_ITERATOR_HPP_
#define INCLUDE_DRIVES_DIRECTORY_ITERATOR_HPP_

#include "common.hpp"
#include "types/remote.hpp"
#include "types/repertory.hpp"

namespace repertory {
class directory_iterator final {
public:
#ifndef _WIN32
  typedef std::function<void(const std::string &api_path, const std::uint64_t &file_size,
                             const api_meta_map &meta, const bool &directory, struct stat *st)>
      populate_stat_callback;
#endif
public:
  explicit directory_iterator(directory_item_list list) : items_(std::move(list)) {}

  directory_iterator(const directory_iterator &iterator) noexcept : items_(iterator.items_) {}

  directory_iterator(directory_iterator &&iterator) noexcept : items_(std::move(iterator.items_)) {}

private:
  directory_item_list items_;

public:
#ifndef _WIN32
  int fill_buffer(const remote::file_offset &offset, fuse_fill_dir_t filler_function, void *buffer,
                  populate_stat_callback populate_stat);
#endif

  int get(const std::size_t &offset, std::string &item);

  std::size_t get_count() const { return items_.size(); }

  api_error get_directory_item(const std::size_t &offset, directory_item &di);

  api_error get_directory_item(const std::string &api_path, directory_item &di);

  int get_json(const std::size_t &offset, json &item);

  std::size_t get_next_directory_offset(const std::string &api_path) const;

public:
  directory_iterator &operator=(const directory_iterator &iterator) noexcept;

  directory_iterator &operator=(directory_iterator &&iterator) noexcept;

  directory_iterator &operator=(directory_item_list list) noexcept;
};
} // namespace repertory

#endif // INCLUDE_DRIVES_DIRECTORY_ITERATOR_HPP_
