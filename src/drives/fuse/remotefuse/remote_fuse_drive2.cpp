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
#ifndef _WIN32
#if 0

#include "drives/fuse/remotefuse/remote_fuse_drive2.hpp"
#include "app_config.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/consumers/logging_consumer.hpp"
#include "events/events.hpp"
#include "events/event_system.hpp"
#include "platform/platform.hpp"
#include "rpc/server/server.hpp"
#include "types/remote.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"

namespace repertory::remote_fuse {
api_error remote_fuse_drive2::access_impl(std::string api_path, int mask) {
  return utils::to_api_error(remote_instance_->fuse_access(api_path.c_str(), mask));
}

#ifdef __APPLE__
api_error remote_fuse_drive2::chflags_impl(std::string api_path, uint32_t flags) {
  return utils::to_api_error(remote_instance_->fuse_chflags(path, flags));
}
#endif // __APPLE__

api_error remote_fuse_drive2::chmod_impl(std::string api_path, mode_t mode) {
  return utils::to_api_error(remote_instance_->fuse_chmod(path, mode));
}
} // namespace repertory::remote_fuse

#endif // 0
#endif // _WIN32
