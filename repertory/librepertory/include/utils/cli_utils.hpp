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
#ifndef REPERTORY_INCLUDE_UTILS_CLI_UTILS_HPP_
#define REPERTORY_INCLUDE_UTILS_CLI_UTILS_HPP_

#include "types/repertory.hpp"

namespace repertory::utils::cli {
using option = std::array<std::string, 2>;

namespace options {
inline const option check_version_option = {"-cv", "--check_version"};
inline const option display_config_option = {"-dc", "--display_config"};
inline const option data_directory_option = {"-dd", "--data_directory"};
inline const option encrypt_option = {"-en", "--encrypt"};
inline const option drive_information_option = {"-di", "--drive_information"};
inline const option name_option = {"-na", "--name"};
inline const option s3_option = {"-s3", "--s3"};
inline const option generate_config_option = {"-gc", "--generate_config"};
inline const option get_option = {"-get", "--get"};
inline const option get_directory_items_option = {"-gdi",
                                                  "--get_directory_items"};
inline const option get_item_info_option = {"-gi", "--get_item_info"};
inline const option get_pinned_files_option = {"-gpf", "--get_pinned_files"};
inline const option help_option = {"-h", "--help"};
inline const option hidden_option = {"-hidden", "--hidden"};
inline const option launch_only_option = {"-lo", "--launch_only"};
inline const option open_files_option = {"-of", "--open_files"};
inline const option pin_file_option = {"-pf", "--pin_file"};
inline const option pinned_status_option = {"-ps", "--pinned_status"};
inline const option password_option = {"-pw", "--password"};
inline const option remote_mount_option = {"-rm", "--remote_mount"};
inline const option remove_option = {"-rp", "--remove"};
inline const option set_option = {"-set", "--set"};
inline const option status_option = {"-status", "--status"};
inline const option test_option = {"-test", "--test"};
inline const option ui_option = {"-ui", "--ui"};
inline const option ui_port_option = {"-up", "--ui_port"};
inline const option unmount_option = {"-unmount", "--unmount"};
inline const option unpin_file_option = {"-uf", "--unpin_file"};
inline const option user_option = {"-us", "--user"};
inline const option version_option = {"-V", "--version"};

inline const std::vector<option> option_list = {
    check_version_option,
    display_config_option,
    data_directory_option,
    drive_information_option,
    encrypt_option,
    s3_option,
    name_option,
    generate_config_option,
    get_option,
    get_directory_items_option,
    get_item_info_option,
    get_pinned_files_option,
    help_option,
    hidden_option,
    launch_only_option,
    open_files_option,
    password_option,
    pin_file_option,
    pinned_status_option,
    remote_mount_option,
    remove_option,
    set_option,
    status_option,
    test_option,
    ui_option,
    ui_port_option,
    unmount_option,
    unpin_file_option,
    user_option,
    version_option,
};
} // namespace options

// Prototypes
void get_api_authentication_data(std::string &user, std::string &password,
                                 std::uint16_t &port,
                                 std::string_view data_directory);

[[nodiscard]] auto get_provider_type_from_args(std::vector<const char *> args)
    -> provider_type;

[[nodiscard]] auto has_option(std::vector<const char *> args,
                              std::string_view option_name) -> bool;

[[nodiscard]] auto has_option(std::vector<const char *> args, const option &opt)
    -> bool;

[[nodiscard]] auto parse_option(std::vector<const char *> args,
                                std::string_view option_name,
                                std::uint8_t count) -> std::vector<std::string>;

[[nodiscard]] auto parse_string_option(std::vector<const char *> args,
                                       const option &opt, std::string &value)
    -> exit_code;

[[nodiscard]] auto parse_drive_options(std::vector<const char *> args,
                                       provider_type &prov,
                                       std::string &data_directory)
    -> std::vector<std::string>;
} // namespace repertory::utils::cli

#endif // REPERTORY_INCLUDE_UTILS_CLI_UTILS_HPP_
