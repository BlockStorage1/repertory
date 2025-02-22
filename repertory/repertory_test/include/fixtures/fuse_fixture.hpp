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
#ifndef REPERTORY_TEST_INCLUDE_FIXTURES_FUSE_FIXTURE_HPP
#define REPERTORY_TEST_INCLUDE_FIXTURES_FUSE_FIXTURE_HPP
#if !defined(_WIN32)

#include "test_common.hpp"

#include "app_config.hpp"
#include "comm/curl/curl_comm.hpp"
#include "db/i_meta_db.hpp"
#include "db/meta_db.hpp"
#include "drives/fuse/fuse_drive.hpp"
#include "platform/platform.hpp"
#include "providers/encrypt/encrypt_provider.hpp"
#include "providers/s3/s3_provider.hpp"
#include "providers/sia/sia_provider.hpp"
#include "types/repertory.hpp"
#include "utils/event_capture.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"
#include "utils/utils.hpp"

#if !defined(ACCESSPERMS)
#define ACCESSPERMS (S_IRWXU | S_IRWXG | S_IRWXO) /* 0777 */
#endif

namespace {
std::atomic<std::size_t> provider_idx{0U};

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

template <typename provider_t> class fuse_test : public ::testing::Test {
public:
  static std::unique_ptr<app_config> config;
  static std::filesystem::path current_directory;
  static provider_type current_provider;
  static std::vector<std::string> drive_args;
  static std::vector<std::string> drive_args2;
  static std::unique_ptr<i_meta_db> meta;
  static std::string mount_location;
  static std::string mount_location2;

protected:
  static void SetUpTestCase() {
    current_directory = std::filesystem::current_path();

    const auto mount_s3 = [&]() {
      {
        auto test_directory = utils::path::combine(
            test::get_test_output_dir(),
            {
                "fuse_test",
                app_config::get_provider_name(current_provider),
            });

        mount_location = utils::path::combine(test_directory, {"mount"});
        ASSERT_TRUE(utils::file::directory(mount_location).create_directory());

        auto cfg_directory = utils::path::combine(test_directory, {"cfg"});
        ASSERT_TRUE(utils::file::directory(cfg_directory).create_directory());

        config = std::make_unique<app_config>(current_provider, cfg_directory);
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

      config->set_database_type(database_type::sqlite);
      meta = create_meta_db(*config);
      execute_mount(drive_args, mount_location);
    };

    const auto mount_sia = [&]() {
      {
        auto test_directory = utils::path::combine(
            test::get_test_output_dir(),
            {
                "fuse_test",
                app_config::get_provider_name(current_provider),
            });

        mount_location = utils::path::combine(test_directory, {"mount"});
        ASSERT_TRUE(utils::file::directory(mount_location).create_directory());

        auto cfg_directory = utils::path::combine(test_directory, {"cfg"});
        ASSERT_TRUE(utils::file::directory(cfg_directory).create_directory());

        config = std::make_unique<app_config>(current_provider, cfg_directory);
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

      config->set_database_type(database_type::sqlite);
      meta = create_meta_db(*config);
      execute_mount(drive_args, mount_location);
    };

    const auto mount_remote = [&]() {
      {
        mount_location2 = mount_location;

        auto test_directory = utils::path::combine(
            test::get_test_output_dir(),
            {
                "fuse_test",
                app_config::get_provider_name(provider_t::type) + '_' +
                    app_config::get_provider_name(provider_t::type2),
            });

        mount_location = utils::path::combine(test_directory, {"mount"});
        ASSERT_TRUE(utils::file::directory(mount_location).create_directory());

        auto cfg_directory = utils::path::combine(test_directory, {"cfg"});
        ASSERT_TRUE(utils::file::directory(cfg_directory).create_directory());

        auto config2 =
            std::make_unique<app_config>(provider_type::remote, cfg_directory);
        config2->set_enable_drive_events(true);
        config2->set_event_level(event_level::trace);
        config2->set_database_type(database_type::sqlite);

        drive_args2 = std::vector<std::string>({
            "-dd",
            config2->get_data_directory(),
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
      execute_unmount(mount_location);
      execute_unmount(mount_location2);
    } else {
      execute_unmount(mount_location);
    }

    meta.reset();
    config.reset();

    std::filesystem::current_path(current_directory);
  }

public:
  static auto create_file_path(std::string &file_name) {
    file_name += std::to_string(++provider_idx);
    auto file_path = utils::path::combine(mount_location, {file_name});
    return file_path;
  }

  static auto create_file_and_test(std::string &file_name, mode_t perms)
      -> std::string {
    file_name += std::to_string(++provider_idx);
    auto file_path = utils::path::combine(mount_location, {file_name});

    auto handle = open(file_path.c_str(), O_CREAT | O_EXCL | O_RDWR, perms);
    EXPECT_LE(1, handle);

    auto opt_size = utils::file::file{file_path}.size();
    EXPECT_TRUE(opt_size.has_value());
    if (opt_size.has_value()) {
      EXPECT_EQ(0U, opt_size.value());
    }

    EXPECT_EQ(0, close(handle));

    EXPECT_TRUE(utils::file::file(file_path).exists());
    EXPECT_FALSE(utils::file::directory(file_path).exists());

    struct stat64 unix_st{};
    EXPECT_EQ(0, stat64(file_path.c_str(), &unix_st));
    EXPECT_EQ(getgid(), unix_st.st_gid);
    EXPECT_EQ(getuid(), unix_st.st_uid);

    return file_path;
  }

  static auto create_file_and_test(std::string &file_name) -> std::string {
    return create_file_and_test(file_name, ACCESSPERMS);
  }

  static auto create_directory_and_test(std::string &dir_name, mode_t perms)
      -> std::string {
    dir_name += std::to_string(++provider_idx);

    auto dir_path = utils::path::combine(mount_location, {dir_name});
    mkdir(dir_path.c_str(), perms);

    EXPECT_TRUE(utils::file::directory(dir_path).exists());
    EXPECT_FALSE(utils::file::file(dir_path).exists());

    struct stat64 unix_st{};
    EXPECT_EQ(0, stat64(dir_path.c_str(), &unix_st));
    EXPECT_EQ(getgid(), unix_st.st_gid);
    EXPECT_EQ(getuid(), unix_st.st_uid);

    return dir_path;
  }

  static auto create_directory_and_test(std::string &dir_name) -> std::string {
    return create_directory_and_test(dir_name, ACCESSPERMS);
  }

  static auto create_root_file(std::string &file_name) -> std::string {
    auto file_path = create_file_and_test(file_name);
    auto api_path = utils::path::create_api_path(file_name);

    [[maybe_unused]] auto res =
        meta->set_item_meta(api_path, {
                                          {META_UID, "0"},
                                          {META_GID, "0"},
                                      });
    std::this_thread::sleep_for(SLEEP_SECONDS);

    return file_path;
  }

  static void execute_mount(auto args, auto location) {
    auto mount_cmd = "./repertory " + utils::string::join(args, ' ');
    std::cout << "mount command: " << mount_cmd << std::endl;
    ASSERT_EQ(0, system(mount_cmd.c_str()));
    std::this_thread::sleep_for(5s);
    ASSERT_TRUE(utils::file::directory{location}.exists());
  }

  static void execute_unmount(auto location) {
    auto unmounted{false};
    for (int i = 0; not unmounted && (i < 50); i++) {
      auto res = fuse_base::unmount(location);
      unmounted = res == 0;
      ASSERT_EQ(0, res);
      if (not unmounted) {
        std::this_thread::sleep_for(5s);
      }
    }

    EXPECT_TRUE(unmounted);
  }

  static void rmdir_and_test(std::string_view dir_path) {
    EXPECT_TRUE(utils::file::directory(dir_path).remove());
    EXPECT_FALSE(utils::file::directory(dir_path).exists());

    EXPECT_FALSE(utils::file::file(dir_path).exists());
  }

  static void unlink_file_and_test(std::string_view file_path) {
    EXPECT_TRUE(utils::file::file(file_path).remove());
    EXPECT_FALSE(utils::file::file(file_path).exists());

    EXPECT_FALSE(utils::file::directory(file_path).exists());
  }

  static void unlink_root_file(const std::string &file_path) {
    auto api_path = utils::path::create_api_path(
        utils::path::strip_to_file_name(file_path));

    [[maybe_unused]] auto res =
        meta->set_item_meta(api_path, {
                                          {META_UID, std::to_string(getuid())},
                                          {META_GID, std::to_string(getgid())},
                                      });
    std::this_thread::sleep_for(SLEEP_SECONDS);

    unlink_file_and_test(file_path);
  }
};

template <typename provider_t>
std::unique_ptr<app_config> fuse_test<provider_t>::config;

template <typename provider_t>
std::filesystem::path fuse_test<provider_t>::current_directory;

template <typename provider_t>
provider_type fuse_test<provider_t>::current_provider{provider_t::type2};

template <typename provider_t>
std::vector<std::string> fuse_test<provider_t>::drive_args;

template <typename provider_t>
std::vector<std::string> fuse_test<provider_t>::drive_args2;

template <typename provider_t>
std::unique_ptr<i_meta_db> fuse_test<provider_t>::meta{};

template <typename provider_t>
std::string fuse_test<provider_t>::mount_location;

template <typename provider_t>
std::string fuse_test<provider_t>::mount_location2;

// using fuse_provider_types = ::testing::Types<local_s3, remote_s3>;
using fuse_provider_types =
    ::testing::Types<local_s3, remote_s3, local_sia, remote_sia>;
} // namespace repertory

#endif // !defined(_WIN32)
#endif // REPERTORY_TEST_INCLUDE_FIXTURES_FUSE_FIXTURE_HPP
