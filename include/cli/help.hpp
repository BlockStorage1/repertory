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
#ifndef INCLUDE_CLI_HELP_HPP_
#define INCLUDE_CLI_HELP_HPP_

#include "common.hpp"

namespace repertory::cli::actions {
template <typename drive> static void help(const int &argc, char *argv[]) {
  drive::display_options(argc, argv);
  std::cout << "Repertory options:" << std::endl;
  std::cout << "    -cv,--check_version               Check daemon version compatibility"
            << std::endl;
  std::cout << "    -dc,--display_config              Display configuration" << std::endl;
  std::cout << "    -dd,--data_directory [directory]  Override data directory" << std::endl;
  std::cout << "    -di,--drive_information           Display mounted drive information"
            << std::endl;
#if defined(REPERTORY_ENABLE_SKYNET)
  std::cout << "    -ea,--export_all                  Export all Skynet skylinks" << std::endl;
  std::cout << "    -ex,--export [path1,path2,...]    Export one or more Skynet skylinks"
            << std::endl;
#endif // defined(REPERTORY_ENABLE_SKYNET)
#if defined(REPERTORY_ENABLE_S3)
  std::cout << "    -s3,--s3                          Enables S3 mode" << std::endl;
  std::cout << "    -na,--name                        Unique name for S3 instance [Required]"
            << std::endl;
#endif // defined(REPERTORY_ENABLE_S3)
  std::cout << "    -gc,--generate_config             Generate initial configuration" << std::endl;
  std::cout << "    -get,--get [name]                 Get configuration value" << std::endl;
  std::cout << "    -gdi,--get_directory_items        Get directory list in json format"
            << std::endl
            << "       [API path]" << std::endl;
  std::cout << "    -gpf,--get_pinned_files           Get a list of all pinned files" << std::endl;
  std::cout << "    -gt,--generate_template           Generate configuration template" << std::endl;
#if defined(REPERTORY_ENABLE_SKYNET)
  std::cout << "    -ij,--import_json [json_array]    Import Skynet skylink(s)" << std::endl;
  std::cout << "      [json_array] format:" << std::endl;
  std::cout << "        [{" << std::endl;
  std::cout << R"(            "directory": "/parent",)" << std::endl;
  std::cout << R"(            "skylink": "AACeCiD6WQG6DzDcCdIu3cFPSxMUMoQPx46NYSyijNMKUA",)"
            << std::endl;
  std::cout << R"(            "token": "encryption password")" << std::endl;
  std::cout << "        }]" << std::endl;
  std::cout << "      NOTE: 'directory' and 'token' are optional" << std::endl;
  std::cout << "    -im,--import [list]               Import Skynet skylink(s)" << std::endl;
  std::cout << "      [list] format:" << std::endl;
  std::cout << "        directory=<directory>:skylink=<skylink>:token=<token>;..." << std::endl;
  std::cout << std::endl;
  std::cout << "      NOTE: 'directory' and 'token' are optional" << std::endl;
  std::cout << "      NOTE: Use '@sem@' to escape a ';'" << std::endl;
  std::cout << "            Use '@comma@' to escape a ','" << std::endl;
  std::cout << "            Use '@equal@' to escape an '='" << std::endl;
  std::cout << "            Use '@dbl_quote@' to escape a '\"'" << std::endl;
#endif // defined(REPERTORY_ENABLE_SKYNET)
  std::cout << "    -nc                               Force disable console output" << std::endl;
  std::cout << "    -of,--open_files                  List all open files and count" << std::endl;
  std::cout << "    -rm,--remote_mount [host/ip:port] Enables remote mount mode" << std::endl;
  std::cout << "    -pf,--pin_file     [API path]     Pin a file to cache to prevent eviction"
            << std::endl;
  std::cout << "    -ps,--pinned_status [API path]    Return pinned status for a file" << std::endl;
  std::cout << "    -pw,--password                    Specify API password" << std::endl;
#if defined(REPERTORY_ENABLE_SKYNET)
  std::cout << "    -sk,--skynet                      [EXPERIMENTAL] Enables Skynet mode"
            << std::endl;
#endif // defined(REPERTORY_ENABLE_SKYNET)
#ifndef _WIN32
#if defined(REPERTORY_ENABLE_S3)
  std::cout << "    -o s3                             Enables S3 mode for 'fstab' mounts"
            << std::endl;
#endif // defined(REPERTORY_ENABLE_S3)
#if defined(REPERTORY_ENABLE_SKYNET)
  std::cout << "    -o sk,-o skynet                   Enables Skynet mode for 'fstab' mounts"
            << std::endl;
#endif // defined(REPERTORY_ENABLE_SKYNET)
#endif // _WIN32
  std::cout << "    -set,--set [name] [value]         Set configuration value" << std::endl;
  std::cout << "    -status                           Display mount status" << std::endl;
#if defined(REPERTORY_ENABLE_SKYNET)
  std::cout << "    -tsa,--test_skynet_auth           Test Skynet portal authentication"
            << std::endl;
  std::cout << "      [URL] [user] [password] [agent string] [API key]" << std::endl;
#endif // defined(REPERTORY_ENABLE_SKYNET)
  std::cout << "    -unmount,--unmount                Unmount and shutdown" << std::endl;
  std::cout << "    -uf,--unpin_file [API path]       Unpin a file from cache to allow eviction"
            << std::endl;
  std::cout << "    -us,--user                        Specify API user name" << std::endl;
}
} // namespace repertory::cli::actions

#endif // INCLUDE_CLI_HELP_HPP_
