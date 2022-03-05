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
#ifndef INCLUDE_UTILS_FILE_UTILS_HPP_
#define INCLUDE_UTILS_FILE_UTILS_HPP_

#include "common.hpp"
#include "types/repertory.hpp"
#include "utils/native_file.hpp"

namespace repertory::utils::file {
// Prototypes
api_error assign_and_get_native_file(filesystem_item &fi, native_file_ptr &nf);

std::uint64_t calculate_used_space(std::string path, const bool &recursive);

void change_to_process_directory();

bool copy_directory_recursively(std::string from_path, std::string to_path);

bool copy_file(std::string from_path, std::string to_path);

bool create_full_directory_path(std::string path);

bool delete_directory(std::string path, const bool &recursive = false);

bool delete_directory_recursively(std::string path);

bool delete_file(std::string path);

std::string generate_sha256(const std::string &file_path);

std::uint64_t get_available_drive_space(const std::string &path);

std::deque<std::string> get_directory_files(std::string path, const bool &oldest_first,
                                            const bool &recursive = false);

bool get_accessed_time(const std::string &path, std::uint64_t &accessed);

bool get_file_size(std::string path, std::uint64_t &file_size);

bool get_modified_time(const std::string &path, std::uint64_t &modified);

#ifdef _WIN32
static open_file_data get_read_write_open_flags() { return open_file_data{}; }
#else  // _WIN32
static int get_read_write_open_flags() { return O_RDWR; }
#endif // _WIN32

bool is_directory(const std::string &path);

bool is_file(const std::string &path);

bool is_modified_date_older_than(const std::string &path, const std::chrono::hours &hours);

bool move_file(std::string from, std::string to);

std::vector<std::string> read_file_lines(const std::string &path);

api_error read_from_source(filesystem_item &fi, const std::size_t &read_size,
                           const std::uint64_t &read_offset, std::vector<char> &data);

template <typename t1, typename t2>
bool read_from_stream(t1 *input_file, t2 &buf, std::size_t to_read);

bool read_json_file(const std::string &path, json &data);

bool reset_modified_time(const std::string &path);

api_error truncate_source(filesystem_item &fi, const std::uint64_t &size);

bool write_json_file(const std::string &path, const json &j);

api_error write_to_source(filesystem_item &fi, const std::uint64_t &write_offset,
                          const std::vector<char> &data, std::size_t &bytes_written);

// template implementations
template <typename t1, typename t2>
bool read_from_stream(t1 *input_file, t2 &buf, std::size_t to_read) {
  std::size_t offset = 0u;
  do {
    input_file->read((char *)&buf[offset], to_read);
    to_read -= static_cast<std::size_t>(input_file->gcount());
    offset += static_cast<std::size_t>(input_file->gcount());
  } while (not input_file->fail() && (to_read > 0u));

  return (to_read == 0u);
}
} // namespace repertory::utils::file

#endif // INCLUDE_UTILS_FILE_UTILS_HPP_
