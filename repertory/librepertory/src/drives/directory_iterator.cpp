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
#include "drives/directory_iterator.hpp"

#include "utils/error_utils.hpp"
#include "utils/path.hpp"

namespace repertory {
#if !defined(_WIN32)
auto directory_iterator::fill_buffer(const remote::file_offset &offset,
                                     fuse_fill_dir_t filler_function,
                                     void *buffer,
                                     populate_stat_callback populate_stat)
    -> int {
  REPERTORY_USES_FUNCTION_NAME();

  if (offset >= items_.size()) {
    errno = 120;
    return -1;
  }

  try {
    auto next_offset{offset + 1U};

    std::string item_name;
    struct stat u_stat{};

    switch (offset) {
    case 0:
    case 1: {
      item_name = offset == 0U ? "." : "..";
      u_stat.st_mode = S_IFDIR | 0755;
      u_stat.st_nlink = 2;
    } break;

    default: {
      const auto &item = items_.at(offset);
      item_name = utils::path::strip_to_file_name(item.api_path);
      populate_stat(item.api_path, item.size, item.meta, item.directory,
                    &u_stat);
    } break;
    }

#if FUSE_USE_VERSION >= 30
    if (filler_function(buffer, item_name.data(), &u_stat,
                        static_cast<off_t>(next_offset),
                        FUSE_FILL_DIR_PLUS) != 0)
#else  // FUSE_USE_VERSION < 30
    if (filler_function(buffer, item_name.data(), &u_stat,
                        static_cast<off_t>(next_offset)) != 0)
#endif // FUSE_USE_VERSION >= 30
    {
      errno = ENOMEM;
      return -1;
    }
  } catch (const std::exception &e) {
    utils::error::raise_error(function_name, e,
                              "failed to fill fuse directory buffer");
  }

  return 0;
}
#endif // !defined(_WIN32)

auto directory_iterator::get(std::size_t offset, std::string &item) -> int {
  if (offset >= items_.size()) {
    errno = 120;
    return -1;
  }

  item = items_[offset].api_path;
  return 0;
}

auto directory_iterator::get_directory_item(std::size_t offset,
                                            directory_item &di) -> api_error {
  if (offset >= items_.size()) {
    return api_error::directory_end_of_files;
  }

  di = items_[offset];
  return api_error::success;
}

auto directory_iterator::get_directory_item(const std::string &api_path,
                                            directory_item &di) -> api_error {
  auto iter = std::ranges::find_if(items_, [&api_path](auto &&item) -> bool {
    return api_path == item.api_path;
  });
  if (iter == items_.end()) {
    return api_error::item_not_found;
  }

  di = *iter;
  return api_error::success;
}

auto directory_iterator::get_json(std::size_t offset, json &item) -> int {
  if (offset >= items_.size()) {
    errno = 120;
    return -1;
  }

  item = json(items_.at(offset));
  return 0;
}

auto directory_iterator::get_next_directory_offset(
    const std::string &api_path) const -> std::size_t {
  const auto iter =
      std::ranges::find_if(items_, [&api_path](auto &&dir_item) -> bool {
        return api_path == dir_item.api_path;
      });

  return (iter == items_.end()) ? 0U
                                : static_cast<std::size_t>(
                                      std::distance(items_.begin(), iter) + 1);
}

auto directory_iterator::operator=(const directory_iterator &iterator) noexcept
    -> directory_iterator & {
  if (this != &iterator) {
    items_ = iterator.items_;
  }
  return *this;
}

auto directory_iterator::operator=(directory_iterator &&iterator) noexcept
    -> directory_iterator & {
  if (this != &iterator) {
    items_ = std::move(iterator.items_);
  }
  return *this;
}

auto directory_iterator::operator=(directory_item_list list) noexcept
    -> directory_iterator & {
  if (&items_ != &list) {
    items_ = std::move(list);
  }
  return *this;
}
} // namespace repertory
