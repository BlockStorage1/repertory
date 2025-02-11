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
#ifndef REPERTORY_TEST_INCLUDE_FIXTURES_WINFSP_FIXTURE_HPP
#define REPERTORY_TEST_INCLUDE_FIXTURES_WINFSP_FIXTURE_HPP
#if defined(_WIN32)

#include "test_common.hpp"

#include "app_config.hpp"
#include "drives/winfsp/remotewinfsp/remote_winfsp_drive.hpp"
#include "drives/winfsp/winfsp_drive.hpp"
#include "platform/platform.hpp"
#include "providers/i_provider.hpp"
#include "providers/s3/s3_provider.hpp"
#include "providers/sia/sia_provider.hpp"
#include "types/repertory.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"

namespace {
std::atomic<std::size_t> idx{0U};
constexpr const auto SLEEP_SECONDS{1.5s};
} // namespace

namespace repertory {
struct local_s3 final {
  static constexpr const provider_type type{provider_type::s3};
  static constexpr const provider_type type2{provider_type::s3};
};

struct local_sia final {
  static constexpr const provider_type type{provider_type::sia};
  static constexpr const provider_type type2{provider_type::sia};
};

struct remote_s3 final {
  static constexpr const provider_type type{provider_type::remote};
  static constexpr const provider_type type2{provider_type::s3};
};

struct remote_sia final {
  static constexpr const provider_type type{provider_type::remote};
  static constexpr const provider_type type2{provider_type::sia};
};

template <typename provider_t> class winfsp_test : public ::testing::Test {
public:
  static std::filesystem::path current_directory;
  static provider_type current_provider;
  static std::vector<std::string> drive_args;
  static std::vector<std::string> drive_args2;
  static std::string mount_location;
  static std::string mount_location2;

protected:
  static void SetUpTestCase() {
    current_directory = std::filesystem::current_path();

    mount_location = utils::string::to_lower(std::string{"U:"});

    const auto mount_s3 = [&]() {
      {
        auto test_directory = utils::path::combine(
            test::get_test_output_dir(),
            {
                "winfsp_test",
                app_config::get_provider_name(provider_type::s3),
            });

        auto cfg_directory = utils::path::combine(test_directory, {"cfg"});
        ASSERT_TRUE(utils::file::directory(cfg_directory).create_directory());

        auto config =
            std::make_unique<app_config>(provider_type::s3, cfg_directory);
        {
          app_config src_cfg{
              provider_type::s3,
              utils::path::combine(test::get_test_config_dir(), {"s3"}),
          };
          config->set_enable_drive_events(true);
          config->set_event_level(event_level::trace);
          config->set_s3_config(src_cfg.get_s3_config());

          auto r_cfg = config->get_remote_mount();
          r_cfg.enable = true;
          r_cfg.api_port = 30000U;
          config->set_remote_mount(r_cfg);
        }

        drive_args = std::vector<std::string>({
            "-dd",
            config->get_data_directory(),
            "-s3",
            "-na",
            "s3",
            mount_location,
        });
      }
      execute_mount(drive_args, mount_location);
    };

    const auto mount_sia = [&]() {
      {
        auto test_directory = utils::path::combine(
            test::get_test_output_dir(),
            {
                "winfsp_test",
                app_config::get_provider_name(provider_type::sia),
            });

        auto cfg_directory = utils::path::combine(test_directory, {"cfg"});
        ASSERT_TRUE(utils::file::directory(cfg_directory).create_directory());

        auto config =
            std::make_unique<app_config>(provider_type::s3, cfg_directory);
        {
          app_config src_cfg{
              provider_type::sia,
              utils::path::combine(test::get_test_config_dir(), {"sia"}),
          };
          config->set_enable_drive_events(true);
          config->set_event_level(event_level::trace);
          config->set_host_config(src_cfg.get_host_config());
          config->set_sia_config(src_cfg.get_sia_config());

          auto r_cfg = config->get_remote_mount();
          r_cfg.enable = true;
          r_cfg.api_port = 30000U;
          config->set_remote_mount(r_cfg);
        }

        drive_args = std::vector<std::string>({
            "-dd",
            config->get_data_directory(),
            "-na",
            "sia",
            mount_location,
        });
      }

      execute_mount(drive_args, mount_location);
    };

    const auto mount_remote = [&]() {
      {
        auto test_directory = utils::path::combine(
            test::get_test_output_dir(),
            {
                "winfsp_test",
                app_config::get_provider_name(provider_type::remote),
            });

        mount_location2 = mount_location;
        mount_location = utils::string::to_lower(std::string{"V:"});

        auto cfg_directory = utils::path::combine(test_directory, {"cfg2"});
        ASSERT_TRUE(utils::file::directory(cfg_directory).create_directory());

        auto config =
            std::make_unique<app_config>(provider_type::remote, cfg_directory);
        config->set_enable_drive_events(true);
        config->set_event_level(event_level::trace);

        drive_args2 = std::vector<std::string>({
            "-dd",
            config->get_data_directory(),
            "-rm",
            "localhost:30000",
            mount_location,
        });
      }

      execute_mount(drive_args2, mount_location);
    };

    switch (provider_t::type) {
    case provider_type::s3: {
      mount_s3();
    } break;

    case provider_type::sia: {
      mount_sia();
    } break;

    case provider_type::remote: {
      switch (provider_t::type2) {
      case provider_type::s3: {
        mount_s3();
      } break;

      case provider_type::sia: {
        mount_sia();
      } break;

      default:
        throw std::runtime_error("remote provider type is not implemented");
        return;
      }

      mount_remote();
    } break;

    default:
      throw std::runtime_error("provider type is not implemented");
      return;
    }
  }

  static void TearDownTestCase() {
    if (provider_t::type == provider_type::remote) {
      execute_unmount(drive_args2, mount_location);
      execute_unmount(drive_args, mount_location2);
    } else {
      execute_unmount(drive_args, mount_location);
    }
    std::filesystem::current_path(current_directory);
  }

  static void execute_mount(auto args, auto location) {
    auto mount_cmd =
        "start .\\repertory.exe -f " + utils::string::join(args, ' ');
    std::cout << "mount command: " << mount_cmd << std::endl;
    ASSERT_EQ(0, system(mount_cmd.c_str()));
    std::this_thread::sleep_for(5s);
    ASSERT_TRUE(utils::file::directory{location}.exists());
  }

  static void execute_unmount(auto args, auto location) {
    auto unmounted{false};

    auto unmount_cmd =
        ".\\repertory.exe " + utils::string::join(args, ' ') + " -unmount";
    for (int i = 0; not unmounted && (i < 6); i++) {
      std::cout << "unmount command: " << unmount_cmd << std::endl;
      system(unmount_cmd.c_str());
      unmounted = not utils::file::directory{location}.exists();
      if (not unmounted) {
        std::this_thread::sleep_for(5s);
      }
    }

    ASSERT_TRUE(unmounted);
  }
};

template <typename provider_t>
std::filesystem::path winfsp_test<provider_t>::current_directory;

template <typename provider_t>
provider_type winfsp_test<provider_t>::current_provider{provider_t::type2};

template <typename provider_t>
std::vector<std::string> winfsp_test<provider_t>::drive_args;

template <typename provider_t>
std::vector<std::string> winfsp_test<provider_t>::drive_args2;

template <typename provider_t>
std::string winfsp_test<provider_t>::mount_location;

template <typename provider_t>
std::string winfsp_test<provider_t>::mount_location2;

// using winfsp_provider_types = ::testing::Types<local_s3, remote_s3,
// local_sia, remote_sia>;
using winfsp_provider_types = ::testing::Types<local_s3, remote_s3>;
} // namespace repertory

#endif // defined(_WIN32)
#endif // REPERTORY_TEST_INCLUDE_FIXTURES_WINFSP_FIXTURE_HPP
