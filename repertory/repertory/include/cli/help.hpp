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
#ifndef REPERTORY_INCLUDE_CLI_HELP_HPP_
#define REPERTORY_INCLUDE_CLI_HELP_HPP_

#include "cli/common.hpp"

namespace repertory::cli::actions {
template <typename drive> inline void help(std::vector<const char *> args) {
  drive::display_options(args);
  std::cout << "Repertory options:" << std::endl;
  std::cout << "    -cv,--check_version               Check daemon version "
               "compatibility"
            << std::endl;
  std::cout << "    -dc,--display_config              Display configuration"
            << std::endl;
  std::cout << "    -dd,--data_directory [directory]  Override data directory"
            << std::endl;
  std::cout << "    -di,--drive_information           Display mounted drive "
               "information"
            << std::endl;
  std::cout << "    -na,--name [name]                 Unique configuration "
               "name [Required for Encrypt, S3 and Sia providers]"
            << std::endl;
  std::cout << "    -s3,--s3                          Enables S3 mode"
            << std::endl;
  std::cout
      << "    -gc,--generate_config             Generate initial configuration"
      << std::endl;
  std::cout << "    -get,--get [name]                 Get configuration value"
            << std::endl;
  std::cout << "    -gdi,--get_directory_items        Get directory list in "
               "json format"
            << std::endl
            << "       [API path]" << std::endl;
  std::cout << "    -gi,--get_item_info               Get item information in "
               "json format"
            << std::endl
            << "       [API path]" << std::endl;
  std::cout
      << "    -gpf,--get_pinned_files           Get a list of all pinned files"
      << std::endl;
  std::cout
      << "    -nc                               Force disable console output"
      << std::endl;
  std::cout
      << "    -of,--open_files                  List all open files and count"
      << std::endl;
  std::cout << "    -rm,--remote_mount [host/ip:port] Enables remote mount mode"
            << std::endl;
  std::cout << "    -rp,--remove                Remove existing provider "
               "configuration"
            << std::endl;
  std::cout << "    -pf,--pin_file     [API path]     Pin a file to cache to "
               "prevent eviction"
            << std::endl;
  std::cout
      << "    -ps,--pinned_status [API path]    Return pinned status for a file"
      << std::endl;
  std::cout << "    -pw,--password [password]   Specify API password"
            << std::endl;
#if !defined(_WIN32)
  std::cout << "    -o s3                             Enables S3 mode for "
               "'fstab' mounts"
            << std::endl;
#endif // _WIN32
  std::cout << "    -set,--set [name] [value]         Set configuration value"
            << std::endl;
  std::cout << "    -status                           Display mount status"
            << std::endl;
  std::cout
      << "    -ui,--ui                          Run embedded management UI"
      << std::endl;
  std::cout << "    -up,--ui_port                     Custom port for embedded "
               "management UI"
            << std::endl;
  std::cout << "    -unmount,--unmount                Unmount and shutdown"
            << std::endl;
  std::cout << "    -uf,--unpin_file [API path]       Unpin a file from cache "
               "to allow eviction"
            << std::endl;
  std::cout << "    -us,--user [user]                 Specify API user name"
            << std::endl;
}
} // namespace repertory::cli::actions

#endif // REPERTORY_INCLUDE_CLI_HELP_HPP_
