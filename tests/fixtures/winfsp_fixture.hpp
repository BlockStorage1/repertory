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
#ifndef REPERTORY_WINFSP_FIXTURE_H
#define REPERTORY_WINFSP_FIXTURE_H
#if _WIN32

#include "app_config.hpp"
#include "drives/winfsp/winfsp_drive.hpp"
#include "mocks/mock_comm.hpp"
#include "platform/platform.hpp"
#include "providers/sia/sia_provider.hpp"
#include "providers/skynet/skynet_provider.hpp"
#include "test_common.hpp"

extern std::size_t PROVIDER_INDEX;

namespace repertory {
class winfsp_test : public ::testing::Test {
public:
  mock_comm mock_sia_comm_;
  mock_comm mock_skynet_comm_;
  std::array<std::tuple<std::shared_ptr<app_config>, std::shared_ptr<i_provider>,
                        std::shared_ptr<winfsp_drive>>,
             2>
      provider_tests_;
  lock_data lock_data_;

protected:
  void SetUp() override {
    if (PROVIDER_INDEX != 0) {
      utils::file::delete_directory_recursively("./winfsp_test");
      auto config = std::make_shared<app_config>(provider_type::sia, "./winfsp_test");
      config->set_enable_drive_events(true);
      config->set_event_level(event_level::verbose);
      config->set_api_port(11115);
      auto sp = std::make_shared<sia_provider>(*config, mock_sia_comm_);
      auto drive = std::make_shared<winfsp_drive>(*config, lock_data_, *sp);
      provider_tests_[0] = {config, sp, drive};

      config = std::make_shared<app_config>(provider_type::skynet, "./winfsp_test");
      config->set_enable_drive_events(true);
      config->set_event_level(event_level::verbose);
      config->set_api_port(11116);
      auto skp = std::make_shared<skynet_provider>(*config, mock_skynet_comm_);
      drive = std::make_shared<winfsp_drive>(*config, lock_data_, *skp);
      provider_tests_[1] = {config, skp, drive};

      event_system::instance().start();
    }
  }

  void TearDown() override {
    if (PROVIDER_INDEX != 0) {
      for (auto &i : provider_tests_) {
        std::get<2>(i).reset();
      }
      event_system::instance().stop();
      for (auto &i : provider_tests_) {
        std::get<1>(i).reset();
        std::get<0>(i).reset();
      }
      utils::file::delete_directory_recursively("./winfsp_test");
    }
  }
};
} // namespace repertory

#endif
#endif // REPERTORY_WINFSP_FIXTURE_H
