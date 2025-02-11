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
static const option check_version_option = {"-cv", "--check_version"};
static const option display_config_option = {"-dc", "--display_config"};
static const option data_directory_option = {"-dd", "--data_directory"};
static const option encrypt_option = {"-en", "--encrypt"};
static const option drive_information_option = {"-di", "--drive_information"};
static const option name_option = {"-na", "--name"};
static const option s3_option = {"-s3", "--s3"};
static const option generate_config_option = {"-gc", "--generate_config"};
static const option get_option = {"-get", "--get"};
static const option get_directory_items_option = {"-gdi",
                                                  "--get_directory_items"};
static const option get_pinned_files_option = {"-gpf", "--get_pinned_files"};
static const option help_option = {"-h", "--help"};
static const option hidden_option = {"-hidden", "--hidden"};
static const option open_files_option = {"-of", "--open_files"};
static const option pin_file_option = {"-pf", "--pin_file"};
static const option pinned_status_option = {"-ps", "--pinned_status"};
static const option password_option = {"-pw", "--password"};
static const option remote_mount_option = {"-rm", "--remote_mount"};
static const option set_option = {"-set", "--set"};
static const option status_option = {"-status", "--status"};
static const option unmount_option = {"-unmount", "--unmount"};
static const option unpin_file_option = {"-uf", "--unpin_file"};
static const option user_option = {"-us", "--user"};
static const option version_option = {"-V", "--version"};

static const std::vector<option> option_list = {
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
    get_pinned_files_option,
    help_option,
    hidden_option,
    open_files_option,
    password_option,
    pin_file_option,
    pinned_status_option,
    remote_mount_option,
    set_option,
    status_option,
    unmount_option,
    unpin_file_option,
    user_option,
    version_option,
};
} // namespace options

// Prototypes
void get_api_authentication_data(std::string &user, std::string &password,
                                 std::uint16_t &port, const provider_type &prov,
                                 const std::string &data_directory);

[[nodiscard]] auto
get_provider_type_from_args(std::vector<const char *> args) -> provider_type;

[[nodiscard]] auto has_option(std::vector<const char *> args,
                              const std::string &option_name) -> bool;

[[nodiscard]] auto has_option(std::vector<const char *> args,
                              const option &opt) -> bool;

[[nodiscard]] auto parse_option(std::vector<const char *> args,
                                const std::string &option_name,
                                std::uint8_t count) -> std::vector<std::string>;

[[nodiscard]] auto parse_string_option(std::vector<const char *> args,
                                       const option &opt,
                                       std::string &value) -> exit_code;

[[nodiscard]] auto
parse_drive_options(std::vector<const char *> args, provider_type &prov,
                    std::string &data_directory) -> std::vector<std::string>;
} // namespace repertory::utils::cli

#endif // REPERTORY_INCLUDE_UTILS_CLI_UTILS_HPP_
