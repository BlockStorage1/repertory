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
#ifndef INCLUDE_UTILS_FILE_UTILS_HPP_
#define INCLUDE_UTILS_FILE_UTILS_HPP_

#include "types/repertory.hpp"
#include "utils/native_file.hpp"

namespace repertory::utils::file {
// Prototypes
[[nodiscard]] auto calculate_used_space(std::string path, bool recursive)
    -> std::uint64_t;

void change_to_process_directory();

[[nodiscard]] auto copy_directory_recursively(std::string from_path,
                                              std::string to_path) -> bool;

[[nodiscard]] auto copy_file(std::string from_path, std::string to_path)
    -> bool;

[[nodiscard]] auto create_full_directory_path(std::string path) -> bool;

[[nodiscard]] auto delete_directory(std::string path, bool recursive = false)
    -> bool;

[[nodiscard]] auto delete_directory_recursively(std::string path) -> bool;

[[nodiscard]] auto delete_file(std::string path) -> bool;

[[nodiscard]] auto generate_sha256(const std::string &file_path) -> std::string;

[[nodiscard]] auto get_accessed_time(const std::string &path,
                                     std::uint64_t &accessed) -> bool;

[[nodiscard]] auto get_directory_files(std::string path, bool oldest_first,
                                       bool recursive = false)
    -> std::deque<std::string>;

[[nodiscard]] auto get_free_drive_space(const std::string &path)
    -> std::uint64_t;

[[nodiscard]] auto get_total_drive_space(const std::string &path)
    -> std::uint64_t;

[[nodiscard]] auto get_file_size(std::string path, std::uint64_t &file_size)
    -> bool;

[[nodiscard]] auto get_modified_time(const std::string &path,
                                     std::uint64_t &modified) -> bool;

[[nodiscard]] auto is_directory(const std::string &path) -> bool;

[[nodiscard]] auto is_file(const std::string &path) -> bool;

[[nodiscard]] auto is_modified_date_older_than(const std::string &path,
                                               const std::chrono::hours &hours)
    -> bool;

[[nodiscard]] auto move_file(std::string from, std::string to) -> bool;

[[nodiscard]] auto read_file_lines(const std::string &path)
    -> std::vector<std::string>;

[[nodiscard]] auto read_json_file(const std::string &path, json &data) -> bool;

[[nodiscard]] auto reset_modified_time(const std::string &path) -> bool;

[[nodiscard]] auto retry_delete_directory(const std::string &dir) -> bool;

[[nodiscard]] auto retry_delete_file(const std::string &file) -> bool;

[[nodiscard]] auto write_json_file(const std::string &path, const json &j)
    -> bool;
} // namespace repertory::utils::file

#endif // INCLUDE_UTILS_FILE_UTILS_HPP_
