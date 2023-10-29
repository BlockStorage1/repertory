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
#include "providers/provider.hpp"

#include "app_config.hpp"
#include "comm/curl/curl_comm.hpp"
#include "comm/i_http_comm.hpp"
#include "comm/i_s3_comm.hpp"
#include "comm/s3/s3_comm.hpp"
#include "events/events.hpp"
#include "providers/encrypt/encrypt_provider.hpp"
#include "providers/s3/s3_provider.hpp"
#include "providers/sia/sia_provider.hpp"
#include "types/startup_exception.hpp"

namespace {
template <typename intf_t, typename comm_t, typename config_t>
inline void create_comm(std::unique_ptr<intf_t> &comm, const config_t &config) {
  if (comm) {
    throw repertory::startup_exception(
        "'create_provider' should only be called once");
  }

  comm = std::make_unique<comm_t>(config);
}
} // namespace

namespace repertory {
auto create_provider(const provider_type &pt, app_config &config)
    -> std::unique_ptr<i_provider> {
  static std::mutex mutex;
  mutex_lock lock(mutex);

  static std::unique_ptr<i_http_comm> comm;
#if defined(REPERTORY_ENABLE_S3)
  static std::unique_ptr<i_s3_comm> s3_comm_;
#endif // defined(REPERTORY_ENABLE_S3)

  switch (pt) {
  case provider_type::sia: {
    create_comm<i_http_comm, curl_comm, host_config>(comm,
                                                     config.get_host_config());
    return std::unique_ptr<i_provider>(
        dynamic_cast<i_provider *>(new sia_provider(config, *comm)));
  }
#if defined(REPERTORY_ENABLE_S3)
  case provider_type::s3: {
    create_comm<i_s3_comm, s3_comm, app_config>(s3_comm_, config);
    return std::unique_ptr<i_provider>(
        dynamic_cast<i_provider *>(new s3_provider(config, *s3_comm_)));
  }
#endif // defined(REPERTORY_ENABLE_S3)
  case provider_type::encrypt: {
    return std::unique_ptr<i_provider>(
        dynamic_cast<i_provider *>(new encrypt_provider(config)));
  }
  case provider_type::unknown:
  default:
    throw startup_exception("provider not supported: " +
                            app_config::get_provider_display_name(pt));
  }
}
} // namespace repertory
