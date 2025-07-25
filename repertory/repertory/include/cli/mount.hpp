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
#ifndef REPERTORY_INCLUDE_CLI_MOUNT_HPP_
#define REPERTORY_INCLUDE_CLI_MOUNT_HPP_

#include "cli/common.hpp"

namespace repertory::cli::actions {
[[nodiscard]] inline auto
mount(std::vector<const char *> args, std::string data_directory,
      int &mount_result, provider_type prov, const std::string &remote_host,
      std::uint16_t remote_port, const std::string &unique_id) -> exit_code {
  lock_data global_lock(provider_type::unknown, "global");
  {
    auto lock_result = global_lock.grab_lock(100U);
    if (lock_result != lock_result::success) {
      std::cerr << "FATAL: Unable to get global lock" << std::endl;
      return exit_code::lock_failed;
    }
  }

  lock_data lock(prov, unique_id);
  auto lock_result = lock.grab_lock();
  if (lock_result == lock_result::locked) {
    std::cerr << app_config::get_provider_display_name(prov)
              << " mount is already active" << std::endl;
    return exit_code::mount_active;
  }

  if (lock_result != lock_result::success) {
    std::cerr << "FATAL: Unable to get provider lock" << std::endl;
    return exit_code::lock_failed;
  }

  if (utils::cli::has_option(args,
                             utils::cli::options::generate_config_option)) {
    app_config config(prov, data_directory);
    if (prov == provider_type::remote) {
      auto remote_config = config.get_remote_config();
      remote_config.host_name_or_ip = remote_host;
      remote_config.api_port = remote_port;
      config.set_remote_config(remote_config);
    } else if (prov == provider_type::sia &&
               config.get_sia_config().bucket.empty()) {
      [[maybe_unused]] auto bucket =
          config.set_value_by_name("SiaConfig.Bucket", unique_id);
    }

    std::cout << "Generated " << app_config::get_provider_display_name(prov)
              << " Configuration" << std::endl;
    std::cout << config.get_config_file_path() << std::endl;
    return utils::file::file(config.get_config_file_path()).exists()
               ? exit_code::success
               : exit_code::file_creation_failed;
  }

#if defined(_WIN32)
  if (utils::cli::has_option(args, utils::cli::options::hidden_option)) {
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
  }
#endif // defined(_WIN32)

  auto drive_args = utils::cli::parse_drive_options(args, prov, data_directory);
  app_config config(prov, data_directory);
  {
    std::uint16_t port{};
    if (not utils::get_next_available_port(config.get_api_port(), port)) {
      std::cerr << "FATAL: Unable to get available port" << std::endl;
      return exit_code::startup_exception;
    }
    config.set_api_port(port);
  }

#if defined(_WIN32)
  if (config.get_enable_mount_manager() && not utils::is_process_elevated()) {
    utils::com_init_wrapper wrapper;
    if (not lock.set_mount_state(true, "elevating", -1)) {
      std::cerr << "failed to set mount state" << std::endl;
    }
    lock.release();
    global_lock.release();

    mount_result = utils::run_process_elevated(args);
    lock_data prov_lock(prov, unique_id);
    if (prov_lock.grab_lock() == lock_result::success) {
      if (not prov_lock.set_mount_state(false, "", -1)) {
        std::cerr << "failed to set mount state" << std::endl;
      }
      prov_lock.release();
    }

    return exit_code::mount_result;
  }
#endif // defined(_WIN32)

  std::cout << "Initializing " << app_config::get_provider_display_name(prov)
            << (unique_id.empty() ? ""
                : (prov == provider_type::remote)
                    ? " [" + remote_host + ':' + std::to_string(remote_port) +
                          ']'
                    : " [" + unique_id + ']')
            << " Drive" << std::endl;
  if (prov == provider_type::remote) {
    try {
      auto remote_cfg = config.get_remote_config();
      remote_cfg.host_name_or_ip = remote_host;
      remote_cfg.api_port = remote_port;
      config.set_remote_config(remote_cfg);

      remote_drive drive(
          config,
          [&config]() -> std::unique_ptr<remote_instance> {
            return std::unique_ptr<remote_instance>(new remote_client(config));
          },
          lock);
      if (not lock.set_mount_state(true, "", -1)) {
        std::cerr << "failed to set mount state" << std::endl;
      }

      global_lock.release();
      mount_result = drive.mount(drive_args);
      return exit_code::mount_result;
    } catch (const std::exception &e) {
      std::cerr << "FATAL: " << e.what() << std::endl;
    }

    return exit_code::startup_exception;
  }

  try {
    if (prov == provider_type::sia && config.get_sia_config().bucket.empty()) {
      [[maybe_unused]] auto bucket =
          config.set_value_by_name("SiaConfig.Bucket", unique_id);
    }

    auto provider = create_provider(prov, config);
    repertory_drive drive(config, lock, *provider);
    if (not lock.set_mount_state(true, "", -1)) {
      std::cerr << "failed to set mount state" << std::endl;
    }

    global_lock.release();
    mount_result = drive.mount(drive_args);
    return exit_code::mount_result;
  } catch (const std::exception &e) {
    std::cerr << "FATAL: " << e.what() << std::endl;
  }

  return exit_code::startup_exception;
}
} // namespace repertory::cli::actions

#endif // REPERTORY_INCLUDE_CLI_MOUNT_HPP_
