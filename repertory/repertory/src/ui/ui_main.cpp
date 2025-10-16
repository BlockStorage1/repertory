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
#include "ui/ui_main.hpp"

#include "cli/actions.hpp"
#include "initialize.hpp"
#include "types/repertory.hpp"
#include "ui/mgmt_app_config.hpp"
#include "ui/ui_server.hpp"
#include "utils/cli_utils.hpp"
#include "utils/error_utils.hpp"
#include "utils/file.hpp"

namespace repertory::ui {
[[nodiscard]] auto ui_main(const std::vector<const char *> &args) -> int {
  mgmt_app_config config{
      utils::cli::has_option(args, utils::cli::options::hidden_option),
      utils::cli::has_option(args, utils::cli::options::launch_only_option),
  };

  std::string data;
  auto res = utils::cli::parse_string_option(
      args, utils::cli::options::ui_port_option, data);
  if (res == exit_code::success && not data.empty()) {
    config.set_api_port(utils::string::to_uint16(data));
  }

  if (not utils::file::change_to_process_directory()) {
    return static_cast<std::int32_t>(exit_code::ui_failed);
  }

  const auto run_ui = [&]() -> int {
    REPERTORY_USES_FUNCTION_NAME();

    ui_server server(&config);

    static std::atomic<ui_server *> active_server{&server};
    static const auto quit_handler = [](int /* sig */) {
      REPERTORY_USES_FUNCTION_NAME();

      auto *ptr = active_server.exchange(nullptr);
      if (ptr == nullptr) {
        return;
      }

      try {
        ptr->stop();
      } catch (const std::exception &ex) {
        utils::error::raise_error(function_name, ex, "failed to stop ui");
      }

      repertory::project_cleanup();
    };

    std::signal(SIGINT, quit_handler);
#if !defined(_WIN32)
    std::signal(SIGQUIT, quit_handler);
#endif // !defined(_WIN32)
    std::signal(SIGTERM, quit_handler);

    try {
      server.start();
    } catch (const std::exception &ex) {
      utils::error::raise_error(function_name, ex, "failed to start ui");
    }

    quit_handler(SIGTERM);
    return 0;
  };

#if defined(_WIN32)
  return run_ui();
#else  // !defined(_WIN32)
  repertory::project_cleanup();

  return utils::create_daemon([&]() -> int {
    if (not repertory::project_initialize()) {
      repertory::project_cleanup();
      return -1;
    }

    if (not utils::file::change_to_process_directory()) {
      return -1;
    }

    return run_ui();
  });
#endif // defined(_WIN32)
}
} // namespace repertory::ui
