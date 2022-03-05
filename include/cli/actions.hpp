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
#ifndef INCLUDE_CLI_ACTIONS_HPP_
#define INCLUDE_CLI_ACTIONS_HPP_

#include "common.hpp"
#include "cli/check_version.hpp"
#include "cli/display_config.hpp"
#include "cli/drive_information.hpp"
#include "cli/export.hpp"
#include "cli/export_all.hpp"
#include "cli/get.hpp"
#include "cli/get_directory_items.hpp"
#include "cli/get_pinned_files.hpp"
#include "cli/help.hpp"
#include "cli/import.hpp"
#include "cli/import_json.hpp"
#include "cli/mount.hpp"
#include "cli/open_files.hpp"
#include "cli/pin_file.hpp"
#include "cli/pinned_status.hpp"
#include "cli/set.hpp"
#include "cli/status.hpp"
#include "cli/test_skynet_auth.hpp"
#include "cli/unmount.hpp"
#include "cli/unpin_file.hpp"
#include "cli/version.hpp"
#include "utils/cli_utils.hpp"

namespace repertory::cli::actions {
typedef std::function<exit_code(const int &argc, char *argv[], const std::string &data_directory,
                                const provider_type &pt, const std::string &unique_id,
                                std::string user, std::string password)>
    action;

struct option_hasher {
  std::size_t operator()(const utils::cli::option &opt) const {
    return std::hash<std::string>()(opt[0u] + '|' + opt[1u]);
  }
};

static const std::unordered_map<utils::cli::option, action, option_hasher> option_actions = {
    {utils::cli::options::check_version_option, cli::actions::check_version},
    {utils::cli::options::display_config_option, cli::actions::display_config},
    {utils::cli::options::drive_information_option, cli::actions::drive_information},
    {utils::cli::options::export_option, cli::actions::export_action},
    {utils::cli::options::export_all_option, cli::actions::export_all},
    {utils::cli::options::get_directory_items_option, cli::actions::get_directory_items},
    {utils::cli::options::get_option, cli::actions::get},
    {utils::cli::options::get_pinned_files_option, cli::actions::get_pinned_files},
    {utils::cli::options::import_option, cli::actions::import},
    {utils::cli::options::import_json_option, cli::actions::import_json},
    {utils::cli::options::open_files_option, cli::actions::open_files},
    {utils::cli::options::pin_file_option, cli::actions::pin_file},
    {utils::cli::options::pinned_status_option, cli::actions::pinned_status},
    {utils::cli::options::set_option, cli::actions::set},
    {utils::cli::options::status_option, cli::actions::status},
#if defined(REPERTORY_ENABLE_SKYNET)
    {utils::cli::options::test_skynet_auth_option, cli::actions::test_skynet_auth},
#endif
    {utils::cli::options::unmount_option, cli::actions::unmount},
    {utils::cli::options::unpin_file_option, cli::actions::unpin_file},
};

static exit_code perform_action(const utils::cli::option &opt, const int &argc, char *argv[],
                                const std::string &data_directory, const provider_type &pt,
                                const std::string &unique_id, std::string user,
                                std::string password) {
  if (utils::cli::has_option(argc, argv, opt)) {
    if (option_actions.find(opt) != option_actions.end()) {
      return option_actions.at(opt)(argc, argv, data_directory, pt, unique_id, std::move(user),
                                    std::move(password));
    }
  }

  return exit_code::option_not_found;
}
} // namespace repertory::cli::actions

#endif // INCLUDE_CLI_ACTIONS_HPP_
