#include "common.hpp"
#include "utils/cli_utils.hpp"
#include "utils/global_data.hpp"
#include "utils/polling.hpp"

#ifdef REPERTORY_TESTING
#include "test_common.hpp"
#include "events/event_system.hpp"
#include "utils/string_utils.hpp"

using namespace repertory;

#ifdef _WIN32
std::size_t PROVIDER_INDEX = 0u;
#endif // _WIN32

int main(int argc, char **argv) {
#ifdef _WIN32
  if (utils::cli::has_option(argc, argv, "--provider_index")) {
    PROVIDER_INDEX = static_cast<std::size_t>(
        utils::string::to_uint64(utils::cli::parse_option(argc, argv, "--provider_index", 1u)[0u]) +
        1u);
  }
#endif // _WIN32

  curl_global_init(CURL_GLOBAL_DEFAULT);

  InitGoogleTest(&argc, argv);
  const auto ret = RUN_ALL_TESTS();

  curl_global_cleanup();
  return ret;
}
#else // REPERTORY_TESTING
#include "cli/actions.hpp"
#include "types/repertory.hpp"

using namespace repertory;

int main(int argc, char **argv) {
  if (argc == 1) {
    argc++;
    static std::string cmd(argv[0u]);
    static std::vector<const char *> v({&cmd[0u], "-h"});
    argv = (char **)&v[0u];
  }

  auto pt = utils::cli::get_provider_type_from_args(argc, argv);

  std::string data_directory;
  auto ret = utils::cli::parse_string_option(argc, argv, utils::cli::options::data_directory_option,
                                             data_directory);

  std::string password;
  ret = (ret == exit_code::success)
            ? utils::cli::parse_string_option(argc, argv, utils::cli::options::password_option,
                                              password)
            : ret;

  std::string user;
  ret = (ret == exit_code::success)
            ? utils::cli::parse_string_option(argc, argv, utils::cli::options::user_option, user)
            : ret;

  std::string remote_host;
  std::uint16_t remote_port = 0u;
  std::string unique_id;
  if ((ret == exit_code::success) && (pt == provider_type::remote)) {
    std::string data;
    if ((ret = utils::cli::parse_string_option(argc, argv, utils::cli::options::remote_mount_option,
                                               data)) == exit_code::success) {
      const auto parts = utils::string::split(data, ':');
      if (parts.size() != 2) {
        std::cerr << "Invalid syntax for host/port '-rm host:port,--remote_mount host:port'"
                  << std::endl;
        ret = exit_code::invalid_syntax;
      } else {
        unique_id = parts[0u] + ':' + parts[1u];
        remote_host = parts[0u];
        try {
          remote_port = utils::string::to_uint16(parts[1u]);
          data_directory = utils::path::combine(
              data_directory.empty() ? app_config::default_data_directory(pt) : data_directory,
              {utils::string::replace_copy(unique_id, ':', '_')});
        } catch (const std::exception &e) {
          std::cerr << (e.what() ? e.what() : "Unable to parse port") << std::endl;
          ret = exit_code::invalid_syntax;
        }
      }
    }
  }

#ifdef REPERTORY_ENABLE_S3
  if ((ret == exit_code::success) && (pt == provider_type::s3)) {
    std::string data;
    if ((ret = utils::cli::parse_string_option(argc, argv, utils::cli::options::name_option,
                                               data)) == exit_code::success) {
      unique_id = utils::string::trim(data);
      if (unique_id.empty()) {
        std::cerr << "Name of S3 instance not provided" << std::endl;
        ret = exit_code::invalid_syntax;
      } else {
        data_directory = utils::path::combine(
            data_directory.empty() ? app_config::default_data_directory(pt) : data_directory,
            {unique_id});
      }
    }
  }
#endif // REPERTORY_ENABLE_S3

  int mount_result = 0;
  if (ret == exit_code::success) {
    if (utils::cli::has_option(argc, argv, utils::cli::options::help_option)) {
      cli::actions::help<repertory_drive>(argc, argv);
    } else if (utils::cli::has_option(argc, argv, utils::cli::options::version_option)) {
      cli::actions::version<repertory_drive>(argc, argv);
    } else {
      ret = exit_code::option_not_found;
      for (std::size_t i = 0;
           (ret == exit_code::option_not_found) && (i < utils::cli::options::option_list.size());
           i++) {
        ret = cli::actions::perform_action(utils::cli::options::option_list[i], argc, argv,
                                           data_directory, pt, unique_id, user, password);
      }
      if (ret == exit_code::option_not_found) {
        ret = cli::actions::mount(argc, argv, data_directory, mount_result, pt, remote_host,
                                  remote_port, unique_id);
      }
    }
  }

  return ((ret == exit_code::mount_result) ? mount_result : static_cast<int>(ret));
}

#endif // REPERTORY_TESTING
