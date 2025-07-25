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
#ifndef REPERTORY_INCLUDE_UTILS_FILE_HPP_
#define REPERTORY_INCLUDE_UTILS_FILE_HPP_

#include "utils/config.hpp"

#include "utils/path.hpp"
#include "utils/string.hpp"
#include "utils/types/file/i_directory.hpp"
#include "utils/types/file/i_file.hpp"
#include "utils/types/file/i_fs_item.hpp"

namespace repertory::utils::directory {
[[nodiscard]] auto temp() -> std::string;
}

namespace repertory::utils::file {
[[nodiscard]] auto change_to_process_directory() -> bool;

// INFO: has test
[[nodiscard]] auto create_temp_name(std::string_view file_part) -> std::string;

// INFO: has test
[[nodiscard]] auto
create_temp_name(std::wstring_view file_part) -> std::wstring;

// INFO: has test
[[nodiscard]] inline auto
directory_exists_in_path(std::string_view path,
                         std::string_view sub_directory) -> bool;

// INFO: has test
[[nodiscard]] inline auto
directory_exists_in_path(std::wstring_view path,
                         std::wstring_view sub_directory) -> bool;

// INFO: has test
[[nodiscard]] inline auto
file_exists_in_path(std::string_view path, std::string_view file_name) -> bool;

// INFO: has test
[[nodiscard]] inline auto
file_exists_in_path(std::wstring_view path,
                    std::wstring_view file_name) -> bool;

// INFO: has test
[[nodiscard]] auto
get_free_drive_space(std::string_view path) -> std::optional<std::uint64_t>;

// INFO: has test
[[nodiscard]] auto
get_free_drive_space(std::wstring_view path) -> std::optional<std::uint64_t>;

// INFO: has test
[[nodiscard]] auto get_time(std::string_view path,
                            time_type type) -> std::optional<std::uint64_t>;

// INFO: has test
[[nodiscard]] auto get_time(std::wstring_view path,
                            time_type type) -> std::optional<std::uint64_t>;

// INFO: has test
[[nodiscard]] auto
get_times(std::string_view path) -> std::optional<file_times>;

// INFO: has test
[[nodiscard]] auto
get_times(std::wstring_view path) -> std::optional<file_times>;

// INFO: has test
[[nodiscard]] auto
get_total_drive_space(std::string_view path) -> std::optional<std::uint64_t>;

// INFO: has test
[[nodiscard]] auto
get_total_drive_space(std::wstring_view path) -> std::optional<std::uint64_t>;

#if defined(PROJECT_ENABLE_LIBDSM)
[[nodiscard]] auto
smb_create_and_validate_relative_path(std::string_view smb_path,
                                      std::string_view rel_path) -> std::string;

// INFO: has test
[[nodiscard]] auto
smb_create_relative_path(std::string_view smb_path) -> std::string;

// INFO: has test
[[nodiscard]] auto
smb_create_search_path(std::string_view smb_path) -> std::string;

// INFO: has test
[[nodiscard]] auto
smb_create_smb_path(std::string_view smb_path,
                    std::string_view rel_path) -> std::string;

[[nodiscard]] auto
smb_get_parent_path(std::string_view smb_path) -> std::string;

[[nodiscard]] auto smb_get_root_path(std::string_view smb_path) -> std::string;

[[nodiscard]] auto smb_get_unc_path(std::string_view smb_path) -> std::string;

[[nodiscard]] auto smb_get_uri_path(std::string_view smb_path) -> std::string;

[[nodiscard]] auto smb_get_uri_path(std::string_view smb_path,
                                    std::string_view user,
                                    std::string_view password) -> std::string;

// INFO: has test
[[nodiscard]] auto smb_parent_is_same(std::string_view smb_path1,
                                      std::string_view smb_path2) -> bool;

#define SMB_MOD_RW2                                                            \
  (SMB_MOD_READ | SMB_MOD_WRITE | SMB_MOD_READ_EXT | SMB_MOD_WRITE_EXT |       \
   SMB_MOD_READ_ATTR | SMB_MOD_WRITE_ATTR | SMB_MOD_READ_CTL)
#endif // defined(PROJECT_ENABLE_LIBDSM)

#if defined(PROJECT_ENABLE_JSON)
#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
// INFO: has test
[[nodiscard]] auto
read_json_file(std::string_view path, nlohmann::json &data,
               std::optional<std::string_view> password = std::nullopt) -> bool;

// INFO: has test
[[nodiscard]] auto read_json_file(
    std::wstring_view path, nlohmann::json &data,
    std::optional<std::wstring_view> password = std::nullopt) -> bool;

// INFO: has test
[[nodiscard]] auto write_json_file(
    std::string_view path, const nlohmann::json &data,
    std::optional<std::string_view> password = std::nullopt) -> bool;

// INFO: has test
[[nodiscard]] auto write_json_file(
    std::wstring_view path, const nlohmann::json &data,
    std::optional<std::wstring_view> password = std::nullopt) -> bool;
#else  // !defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
// INFO: has test
[[nodiscard]] auto read_json_file(std::string_view path,
                                  nlohmann::json &data) -> bool;

// INFO: has test
[[nodiscard]] auto read_json_file(std::wstring_view path,
                                  nlohmann::json &data) -> bool;

// INFO: has test
[[nodiscard]] auto write_json_file(std::string_view path,
                                   const nlohmann::json &data) -> bool;

// INFO: has test
[[nodiscard]] auto write_json_file(std::wstring_view path,
                                   const nlohmann::json &data) -> bool;
#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
#endif // defined(PROJECT_ENABLE_JSON)
} // namespace repertory::utils::file

#endif // REPERTORY_INCLUDE_UTILS_FILE_HPP_

#include "utils/file_directory.hpp"
#include "utils/file_enc_file.hpp"
#include "utils/file_file.hpp"
#include "utils/file_smb_directory.hpp"
#include "utils/file_smb_file.hpp"
#include "utils/file_thread_file.hpp"
