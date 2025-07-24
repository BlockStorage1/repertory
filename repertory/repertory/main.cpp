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
#if defined(PROJECT_ENABLE_BACKWARD_CPP)
#include "backward.hpp"
#endif // defined(PROJECT_ENABLE_BACKWARD_CPP)

#include "cli/actions.hpp"
#include "initialize.hpp"
#include "types/repertory.hpp"
#include "utils/cli_utils.hpp"
#include "utils/polling.hpp"

using namespace repertory;

auto main(int argc, char **argv) -> int {
#if defined(PROJECT_ENABLE_BACKWARD_CPP)
  static backward::SignalHandling sh;
#endif // defined(PROJECT_ENABLE_BACKWARD_CPP)

  if (not repertory::project_initialize()) {
    std::cerr << "fatal: failed to initialize repertory" << std::endl;
    repertory::project_cleanup();
    return -1;
  }

  std::vector<const char *> args;
  {
    auto args_span = std::span(argv, static_cast<std::size_t>(argc));
    std::ranges::copy(args_span, std::back_inserter(args));
  }

  if (argc == 1) {
    args.push_back("-h");
  }

  auto prov = utils::cli::get_provider_type_from_args(args);

  std::string data_directory;
  auto res = utils::cli::parse_string_option(
      args, utils::cli::options::data_directory_option, data_directory);

  std::string password;
  res = (res == exit_code::success)
            ? utils::cli::parse_string_option(
                  args, utils::cli::options::password_option, password)
            : res;

  std::string user;
  res = (res == exit_code::success)
            ? utils::cli::parse_string_option(
                  args, utils::cli::options::user_option, user)
            : res;

  std::string remote_host;
  std::uint16_t remote_port{};
  std::string unique_id;
  if (res == exit_code::success) {
    if (prov == provider_type::remote) {
      std::string data;
      res = utils::cli::parse_string_option(
          args, utils::cli::options::remote_mount_option, data);
      if (res == exit_code::success) {
        const auto parts = utils::string::split(data, ':', false);
        if (parts.size() != 2) {
          std::cerr << "Invalid syntax for host/port '-rm "
                       "host:port,--remote_mount host:port'"
                    << std::endl;
          res = exit_code::invalid_syntax;
        } else {
          unique_id = parts.at(0U) + ':' + parts.at(1U);
          remote_host = parts.at(0U);
          try {
            remote_port = utils::string::to_uint16(parts.at(1U));
            data_directory =
                data_directory.empty()
                    ? utils::path::combine(
                          app_config::default_data_directory(prov),
                          {
                              utils::string::replace_copy(unique_id, ':', '_'),
                          })
                    : utils::path::absolute(data_directory);
          } catch (const std::exception &e) {
            std::cerr << (e.what() == nullptr ? "Unable to parse port"
                                              : e.what())
                      << std::endl;
            res = exit_code::invalid_syntax;
          }
        }
      }
    } else if ((prov == provider_type::s3) || (prov == provider_type::sia) ||
               (prov == provider_type::encrypt)) {
      std::string data;
      res = utils::cli::parse_string_option(
          args, utils::cli::options::name_option, data);
      if (res == exit_code::success) {
        unique_id = utils::string::trim(data);
        if (unique_id.empty()) {
          if (prov == provider_type::sia) {
            unique_id = "default";
          } else {
            std::cerr << "Configuration name for '"
                      << app_config::get_provider_display_name(prov)
                      << "' was not provided" << std::endl;
            res = exit_code::invalid_syntax;
          }
        }
      }

      if (res == exit_code::success) {
        data_directory =
            data_directory.empty()
                ? utils::path::combine(app_config::default_data_directory(prov),
                                       {unique_id})
                : utils::path::absolute(data_directory);
      }
    }
  }

  int mount_result{};
  if (res == exit_code::success) {
    if (utils::cli::has_option(args, utils::cli::options::help_option)) {
      cli::actions::help<repertory_drive>(args);
    } else if (utils::cli::has_option(args,
                                      utils::cli::options::version_option)) {
      cli::actions::version<repertory_drive>(args);
    } else {
      res = exit_code::option_not_found;
      for (std::size_t idx = 0U;
           (res == exit_code::option_not_found) &&
           (idx < utils::cli::options::option_list.size());
           idx++) {
        try {
          res = cli::actions::perform_action(
              utils::cli::options::option_list[idx], args, data_directory, prov,
              unique_id, user, password);
        } catch (const std::exception &ex) {
          res = exit_code::exception;
        } catch (...) {
          res = exit_code::exception;
        }
      }

      if (res == exit_code::option_not_found) {
        res = cli::actions::mount(args, data_directory, mount_result, prov,
                                  remote_host, remote_port, unique_id);
      }
    }
  }

  auto ret =
      ((res == exit_code::mount_result) ? mount_result
                                        : static_cast<std::int32_t>(res));

  repertory::project_cleanup();

  return ret;
}
