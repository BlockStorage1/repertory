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
#ifndef REPERTORY_INCLUDE_CLI_COMMON_HPP_
#define REPERTORY_INCLUDE_CLI_COMMON_HPP_

#include "app_config.hpp"
#include "comm/curl/curl_comm.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "providers/provider.hpp"
#include "providers/sia/sia_provider.hpp"
#include "rpc/client/client.hpp"
#include "types/repertory.hpp"
#include "types/rpc.hpp"
#include "utils/cli_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"
#include "version.hpp"

#if defined(_WIN32)
#include "drives/winfsp/remotewinfsp/remote_client.hpp"
#include "drives/winfsp/remotewinfsp/remote_winfsp_drive.hpp"
#include "drives/winfsp/winfsp_drive.hpp"

using repertory_drive = repertory::winfsp_drive;
using remote_client = repertory::remote_winfsp::remote_client;
using remote_drive = repertory::remote_winfsp::remote_winfsp_drive;
using remote_instance = repertory::remote_winfsp::i_remote_instance;
#else // !defined(_WIN32)
#include "drives/fuse/fuse_drive.hpp"
#include "drives/fuse/remotefuse/remote_client.hpp"
#include "drives/fuse/remotefuse/remote_fuse_drive.hpp"

using repertory_drive = repertory::fuse_drive;
using remote_client = repertory::remote_fuse::remote_client;
using remote_drive = repertory::remote_fuse::remote_fuse_drive;
using remote_instance = repertory::remote_fuse::i_remote_instance;
#endif // defined(_WIN32)

namespace repertory::cli {
[[nodiscard]] inline auto handle_error(exit_code code, std::string_view msg)
    -> exit_code {
  fmt::println("{}", static_cast<std::int32_t>(code));
  fmt::println("{}", msg);
  return code;
}

[[nodiscard]] inline auto handle_error(exit_code code,
                                       rpc_response_type response_type,
                                       std::string_view msg) -> exit_code {
  fmt::println("{}", static_cast<std::uint8_t>(response_type));
  fmt::println("{}", msg);
  return code;
}

[[nodiscard]] inline auto check_data_directory(std::string_view data_directory)
    -> exit_code {
  if (not utils::file::directory{utils::path::absolute(data_directory)}
              .exists()) {
    return handle_error(exit_code::mount_not_found, "failed: mount not found");
  }

  return exit_code::success;
}
} // namespace repertory::cli

#endif // REPERTORY_INCLUDE_CLI_COMMON_HPP_
