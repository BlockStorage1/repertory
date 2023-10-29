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
#ifndef INCLUDE_CLI_MOUNT_HPP_
#define INCLUDE_CLI_MOUNT_HPP_

#include "app_config.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "providers/provider.hpp"
#include "types/repertory.hpp"
#include "utils/cli_utils.hpp"
#include "utils/com_init_wrapper.hpp"
#include "utils/file_utils.hpp"
#include "utils/string_utils.hpp"

#ifdef _WIN32
#include "drives/winfsp/remotewinfsp/remote_client.hpp"
#include "drives/winfsp/remotewinfsp/remote_winfsp_drive.hpp"
#include "drives/winfsp/winfsp_drive.hpp"

using repertory_drive = repertory::winfsp_drive;
using remote_client = repertory::remote_winfsp::remote_client;
using remote_drive = repertory::remote_winfsp::remote_winfsp_drive;
using remote_instance = repertory::remote_winfsp::i_remote_instance;
#else
#include "drives/fuse/fuse_drive.hpp"
#include "drives/fuse/remotefuse/remote_client.hpp"
#include "drives/fuse/remotefuse/remote_fuse_drive.hpp"

using repertory_drive = repertory::fuse_drive;
using remote_client = repertory::remote_fuse::remote_client;
using remote_drive = repertory::remote_fuse::remote_fuse_drive;
using remote_instance = repertory::remote_fuse::i_remote_instance;
#endif

namespace repertory::cli::actions {
[[nodiscard]] inline auto
mount(int argc, char *argv[], std::string data_directory, int &mount_result,
      provider_type pt, const std::string &remote_host,
      std::uint16_t remote_port, const std::string &unique_id) -> exit_code {
  auto ret = exit_code::success;

  lock_data lock(pt, unique_id);
  const auto res = lock.grab_lock();
  if (res == lock_result::locked) {
    ret = exit_code::mount_active;
    std::cerr << app_config::get_provider_display_name(pt)
              << " mount is already active" << std::endl;
  } else if (res == lock_result::success) {
    const auto generate_config = utils::cli::has_option(
        argc, argv, utils::cli::options::generate_config_option);
    if (generate_config) {
      app_config config(pt, data_directory);
      if (pt == provider_type::remote) {
        config.set_enable_remote_mount(false);
        config.set_is_remote_mount(true);
        config.set_remote_host_name_or_ip(remote_host);
        config.set_remote_port(remote_port);
        config.save();
      }
      std::cout << "Generated " << app_config::get_provider_display_name(pt)
                << " Configuration" << std::endl;
      std::cout << config.get_config_file_path() << std::endl;
      ret = utils::file::is_file(config.get_config_file_path())
                ? exit_code::success
                : exit_code::file_creation_failed;
    } else {
#ifdef _WIN32
      if (utils::cli::has_option(argc, argv,
                                 utils::cli::options::hidden_option)) {
        ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
      }
#endif
      const auto drive_args =
          utils::cli::parse_drive_options(argc, argv, pt, data_directory);
      app_config config(pt, data_directory);
#ifdef _WIN32
      if (config.get_enable_mount_manager() &&
          not utils::is_process_elevated()) {
        com_init_wrapper cw;
        if (not lock.set_mount_state(true, "elevating", -1)) {
          std::cerr << "failed to set mount state" << std::endl;
        }
        lock.release();

        mount_result = utils::run_process_elevated(argc, argv);
        lock_data lock2(pt, unique_id);
        if (lock2.grab_lock() == lock_result::success) {
          if (not lock2.set_mount_state(false, "", -1)) {
            std::cerr << "failed to set mount state" << std::endl;
          }
          lock2.release();
        }

        return exit_code::mount_result;
      }
#endif
      std::cout << "Initializing " << app_config::get_provider_display_name(pt)
                << (unique_id.empty() ? ""
                    : (pt == provider_type::s3)
                        ? " [" + unique_id + ']'
                        : " [" + remote_host + ':' +
                              std::to_string(remote_port) + ']')
                << " Drive" << std::endl;
      if (pt == provider_type::remote) {
        std::uint16_t port = 0u;
        if (utils::get_next_available_port(config.get_api_port(), port)) {
          config.set_remote_host_name_or_ip(remote_host);
          config.set_remote_port(remote_port);
          config.set_api_port(port);
          config.set_is_remote_mount(true);
          config.set_enable_remote_mount(false);
          config.save();
          try {
            remote_drive drive(
                config,
                [&config]() -> std::unique_ptr<remote_instance> {
                  return std::unique_ptr<remote_instance>(
                      new remote_client(config));
                },
                lock);
            if (not lock.set_mount_state(true, "", -1)) {
              std::cerr << "failed to set mount state" << std::endl;
            }
            mount_result = drive.mount(drive_args);
            ret = exit_code::mount_result;
          } catch (const std::exception &e) {
            std::cerr << "FATAL: " << e.what() << std::endl;
            ret = exit_code::startup_exception;
          }
        } else {
          std::cerr << "FATAL: Unable to get available port" << std::endl;
          ret = exit_code::startup_exception;
        }
      } else {
        config.set_is_remote_mount(false);

        try {
          auto provider = create_provider(pt, config);
          repertory_drive drive(config, lock, *provider);
          if (not lock.set_mount_state(true, "", -1)) {
            std::cerr << "failed to set mount state" << std::endl;
          }
          mount_result = drive.mount(drive_args);
          ret = exit_code::mount_result;
        } catch (const std::exception &e) {
          std::cerr << "FATAL: " << e.what() << std::endl;
          ret = exit_code::startup_exception;
        }
      }
    }
  } else {
    ret = exit_code::lock_failed;
  }

  return ret;
}
} // namespace repertory::cli::actions

#endif // INCLUDE_CLI_MOUNT_HPP_
