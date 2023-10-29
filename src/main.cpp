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
#ifdef REPERTORY_TESTING
#include "events/event_system.hpp"
#include "test_common.hpp"
#include "utils/string_utils.hpp"
#else
#include "cli/actions.hpp"
#include "types/repertory.hpp"
#endif
#include "utils/cli_utils.hpp"
#include "utils/polling.hpp"

using namespace repertory;

#if defined(REPERTORY_TESTING) && defined(_WIN32)
std::size_t PROVIDER_INDEX{0U};
#endif

auto main(int argc, char **argv) -> int {
  repertory_init();

#ifdef REPERTORY_TESTING
#ifdef _WIN32
  if (utils::cli::has_option(argc, argv, "--provider_index")) {
    PROVIDER_INDEX = static_cast<std::size_t>(
        utils::string::to_uint64(
            utils::cli::parse_option(argc, argv, "--provider_index", 1U)[0U]) +
        1U);
  }
#endif // _WIN32

  InitGoogleTest(&argc, argv);
  auto ret = RUN_ALL_TESTS();
  delete_generated_files();
#else
  if (argc == 1) {
    argc++;
    static std::string cmd(argv[0U]);
    static std::vector<const char *> v({&cmd[0U], "-h"});
    argv = (char **)&v[0U];
  }

  auto pt = utils::cli::get_provider_type_from_args(argc, argv);

  std::string data_directory;
  auto res = utils::cli::parse_string_option(
      argc, argv, utils::cli::options::data_directory_option, data_directory);

  std::string password;
  res = (res == exit_code::success)
            ? utils::cli::parse_string_option(
                  argc, argv, utils::cli::options::password_option, password)
            : res;

  std::string user;
  res = (res == exit_code::success)
            ? utils::cli::parse_string_option(
                  argc, argv, utils::cli::options::user_option, user)
            : res;

  std::string remote_host;
  std::uint16_t remote_port{};
  std::string unique_id;
  if ((res == exit_code::success) && (pt == provider_type::remote)) {
    std::string data;
    if ((res = utils::cli::parse_string_option(
             argc, argv, utils::cli::options::remote_mount_option, data)) ==
        exit_code::success) {
      const auto parts = utils::string::split(data, ':');
      if (parts.size() != 2) {
        std::cerr << "Invalid syntax for host/port '-rm "
                     "host:port,--remote_mount host:port'"
                  << std::endl;
        res = exit_code::invalid_syntax;
      } else {
        unique_id = parts[0U] + ':' + parts[1U];
        remote_host = parts[0U];
        try {
          remote_port = utils::string::to_uint16(parts[1U]);
          data_directory = utils::path::combine(
              data_directory.empty() ? app_config::default_data_directory(pt)
                                     : data_directory,
              {utils::string::replace_copy(unique_id, ':', '_')});
        } catch (const std::exception &e) {
          std::cerr << (e.what() ? e.what() : "Unable to parse port")
                    << std::endl;
          res = exit_code::invalid_syntax;
        }
      }
    }
  }

#ifdef REPERTORY_ENABLE_S3
  if ((res == exit_code::success) && (pt == provider_type::s3)) {
    std::string data;
    if ((res = utils::cli::parse_string_option(argc, argv,
                                               utils::cli::options::name_option,
                                               data)) == exit_code::success) {
      unique_id = utils::string::trim(data);
      if (unique_id.empty()) {
        std::cerr << "Name of S3 instance not provided" << std::endl;
        res = exit_code::invalid_syntax;
      } else {
        data_directory = utils::path::combine(
            data_directory.empty() ? app_config::default_data_directory(pt)
                                   : data_directory,
            {unique_id});
      }
    }
  }
#endif // REPERTORY_ENABLE_S3

  int mount_result{};
  if (res == exit_code::success) {
    if (utils::cli::has_option(argc, argv, utils::cli::options::help_option)) {
      cli::actions::help<repertory_drive>(argc, argv);
    } else if (utils::cli::has_option(argc, argv,
                                      utils::cli::options::version_option)) {
      cli::actions::version<repertory_drive>(argc, argv);
    } else {
      res = exit_code::option_not_found;
      for (std::size_t idx = 0U;
           (res == exit_code::option_not_found) &&
           (idx < utils::cli::options::option_list.size());
           idx++) {
        res = cli::actions::perform_action(
            utils::cli::options::option_list[idx], argc, argv, data_directory,
            pt, unique_id, user, password);
      }
      if (res == exit_code::option_not_found) {
        res = cli::actions::mount(argc, argv, data_directory, mount_result, pt,
                                  remote_host, remote_port, unique_id);
      }
    }
  }

  auto ret =
      ((res == exit_code::mount_result) ? mount_result
                                        : static_cast<std::int32_t>(res));

#endif

  repertory_shutdown();
  return ret;
}
