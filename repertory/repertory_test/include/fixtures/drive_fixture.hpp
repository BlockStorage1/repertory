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
#ifndef REPERTORY_TEST_INCLUDE_FIXTURES_DRIVE_FIXTURE_HPP
#define REPERTORY_TEST_INCLUDE_FIXTURES_DRIVE_FIXTURE_HPP

#include "test_common.hpp"

#include "app_config.hpp"
#include "platform/platform.hpp"
#include "types/repertory.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/utils.hpp"

#if defined(_WIN32)
#include "drives/winfsp/remotewinfsp/remote_winfsp_drive.hpp"
#include "drives/winfsp/winfsp_drive.hpp"
#else // !defined(_WIN32)
#include "db/i_meta_db.hpp"
#include "db/meta_db.hpp"
#include "drives/fuse/fuse_drive.hpp"

#if !defined(ACCESSPERMS)
// 0777
#define ACCESSPERMS (S_IRWXU | S_IRWXG | S_IRWXO)
#endif // !defined(ACCESSPERMS)
#endif // defined(_WIN32)

namespace {
std::atomic<std::size_t> provider_idx{0U};
constexpr auto SLEEP_SECONDS{1.5s};
} // namespace

namespace repertory {
struct local_s3_no_encryption final {
  static constexpr provider_type type{provider_type::s3};
  static constexpr provider_type type2{provider_type::s3};
  static constexpr std::uint16_t remote_port{41000U};
  static constexpr bool force_legacy_encryption{false};
  static constexpr std::string_view encryption_token{""};
};
struct local_s3_encryption final {
  static constexpr provider_type type{provider_type::s3};
  static constexpr provider_type type2{provider_type::s3};
  static constexpr std::uint16_t remote_port{41000U};
  static constexpr bool force_legacy_encryption{false};
  static constexpr std::string_view encryption_token{"encryption_token"};
};
struct local_s3_legacy_encryption final {
  static constexpr provider_type type{provider_type::s3};
  static constexpr provider_type type2{provider_type::s3};
  static constexpr std::uint16_t remote_port{41000U};
  static constexpr bool force_legacy_encryption{true};
  static constexpr std::string_view encryption_token{"encryption_token"};
};
struct remote_s3_no_encryption final {
  static constexpr provider_type type{provider_type::remote};
  static constexpr provider_type type2{provider_type::s3};
  static constexpr std::uint16_t remote_port{41000U};
  static constexpr bool force_legacy_encryption{false};
  static constexpr std::string_view encryption_token{""};
};
struct remote_s3_encryption final {
  static constexpr provider_type type{provider_type::remote};
  static constexpr provider_type type2{provider_type::s3};
  static constexpr std::uint16_t remote_port{41000U};
  static constexpr bool force_legacy_encryption{false};
  static constexpr std::string_view encryption_token{"encryption_token"};
};
struct remote_s3_legacy_encryption final {
  static constexpr provider_type type{provider_type::remote};
  static constexpr provider_type type2{provider_type::s3};
  static constexpr std::uint16_t remote_port{41000U};
  static constexpr bool force_legacy_encryption{true};
  static constexpr std::string_view encryption_token{"encryption_token"};
};
struct local_sia final {
  static constexpr provider_type type{provider_type::sia};
  static constexpr provider_type type2{provider_type::sia};
  static constexpr std::uint16_t remote_port{41001U};
  static constexpr bool force_legacy_encryption{false};
  static constexpr std::string_view encryption_token{""};
};
struct remote_sia final {
  static constexpr provider_type type{provider_type::remote};
  static constexpr provider_type type2{provider_type::sia};
  static constexpr std::uint16_t remote_port{41001U};
  static constexpr bool force_legacy_encryption{false};
  static constexpr std::string_view encryption_token{""};
};
struct remote_winfsp_to_linux final {
  static constexpr provider_type type{provider_type::remote};
  static constexpr provider_type type2{provider_type::unknown};
  static constexpr std::uint16_t remote_port{41002U};
  static constexpr bool force_legacy_encryption{false};
  static constexpr std::string_view encryption_token{""};
};
struct remote_linux_to_winfsp final {
  static constexpr provider_type type{provider_type::remote};
  static constexpr provider_type type2{provider_type::unknown};
  static constexpr std::uint16_t remote_port{41002U};
  static constexpr bool force_legacy_encryption{false};
  static constexpr std::string_view encryption_token{""};
};

struct platform_ops {
  static void ensure_process_cwd() {
#if !defined(_WIN32)
    EXPECT_TRUE(utils::file::change_to_process_directory());
#endif // !defined(_WIN32)
  }

  static auto build_cmd(std::string_view args_joined,
                        [[maybe_unused]] bool is_mount) -> std::string {
#if defined(_WIN32)
    if (is_mount) {
      return "start .\\repertory.exe -f " + std::string{args_joined};
    }
    return ".\\repertory.exe " + std::string{args_joined};
#else // !defined(_WIN32)
#if defined(__APPLE__)
    constexpr std::string_view repertory_bin =
        "./repertory.app/Contents/MacOS/repertory ";
#else  // !defined(__APPLE__)
    constexpr std::string_view repertory_bin = "./repertory ";
#endif // defined(__APPLE__)
    return std::string(repertory_bin) + std::string{args_joined};
#endif // defined(_WIN32)
  }

  static void execute_mount(std::vector<std::string> args_without_location,
                            std::string_view location) {
    ensure_process_cwd();
    args_without_location.emplace_back(location);

    const auto cmd = build_cmd(utils::string::join(args_without_location, ' '),
                               /*is_mount*/ true);
    std::cout << "mount command: " << cmd << std::endl;

    ASSERT_EQ(0, system(cmd.c_str()));
    std::this_thread::sleep_for(5s);
    ASSERT_TRUE(utils::file::directory{location}.exists());
  }

  static void execute_unmount(std::vector<std::string> args_without_unmount,
                              [[maybe_unused]] std::string_view location) {
    ensure_process_cwd();
    args_without_unmount.emplace_back("-unmount");

    const auto cmd = build_cmd(utils::string::join(args_without_unmount, ' '),
                               /*is_mount*/ false);
    std::cout << "unmount command: " << cmd << std::endl;

#if defined(_WIN32)
    std::this_thread::sleep_for(10s);
    bool unmounted = false;
    for (int i = 0; !unmounted && i < 6; ++i) {
      (void)system(cmd.c_str());
      unmounted = !utils::file::directory{location}.exists();
      if (!unmounted)
        std::this_thread::sleep_for(5s);
    }
    ASSERT_TRUE(unmounted);
#else  // !defined(_WIN32)
    auto res = system(cmd.c_str());
    EXPECT_EQ(0, res);
#endif // defined(_WIN32)
  }
};

template <typename provider_t> class drive_fixture : public ::testing::Test {
public:
#if !defined(_WIN32)
  static std::unique_ptr<app_config> config;
  static std::unique_ptr<app_config> config2;
  static std::unique_ptr<i_meta_db> meta;
#endif // !defined(_WIN32)
  static std::filesystem::path current_directory;
  static provider_type current_provider;
  static std::vector<std::string> drive_args;
  static std::vector<std::string> drive_args2;
  static std::string mount_location;
  static std::string mount_location2;
  static provider_type mount_provider;

protected:
  static void SetUpTestCase() {
    current_directory = std::filesystem::current_path();

    const auto make_test_dir = [](std::string_view suite,
                                  std::string_view sub) -> std::string {
      return utils::path::combine(test::get_test_output_dir(), {
                                                                   suite,
                                                                   sub,
                                                               });
    };

    const auto make_cfg_dir = [](std::string_view root,
                                 std::string_view name) -> std::string {
      auto cfg = utils::path::combine(root, {name});
      EXPECT_TRUE(utils::file::directory(cfg).create_directory());
      return cfg;
    };

    const auto configure_s3 = [](app_config &cfg_obj) {
      app_config src_cfg{
          provider_type::s3,
          utils::path::combine(test::get_test_config_dir(), {"s3"})};
      auto cfg = src_cfg.get_s3_config();
      cfg.force_legacy_encryption = provider_t::force_legacy_encryption;
      cfg.encryption_token = provider_t::encryption_token;

      cfg_obj.set_enable_drive_events(true);
      cfg_obj.set_event_level(event_level::trace);
      cfg_obj.set_s3_config(cfg);

      auto remote_cfg = cfg_obj.get_remote_mount();
      remote_cfg.enable = true;
      remote_cfg.api_port = provider_t::remote_port;
      cfg_obj.set_remote_mount(remote_cfg);
    };

    const auto configure_sia = [](app_config &cfg_obj) {
      app_config src_cfg{
          provider_type::sia,
          utils::path::combine(test::get_test_config_dir(), {"sia"})};

      cfg_obj.set_enable_drive_events(true);
      cfg_obj.set_event_level(event_level::trace);
      cfg_obj.set_host_config(src_cfg.get_host_config());
      cfg_obj.set_sia_config(src_cfg.get_sia_config());

      auto remote_cfg = cfg_obj.get_remote_mount();
      remote_cfg.enable = true;
      remote_cfg.api_port = provider_t::remote_port;
      cfg_obj.set_remote_mount(remote_cfg);
    };

    const auto mount_local_s3 = [&](bool as_remote) {
      const auto test_dir = make_test_dir(
#if defined(_WIN32)
          "winfsp_test",
#else  // !defined(_WIN32)
          "fuse_test",
#endif // defined(_WIN32)
          fmt::format("{}_{}", app_config::get_provider_name(current_provider),
                      as_remote));
#if defined(_WIN32)
      mount_location = utils::string::to_lower(std::string{"U:"});
#else  // !defined(_WIN32)
      mount_location = utils::path::combine(test_dir, {"mount"});
      ASSERT_TRUE(utils::file::directory(mount_location).create_directory());
#endif // defined(_WIN32)

      auto cfg_dir = make_cfg_dir(test_dir, "cfg");
      auto cfg = std::make_unique<app_config>(provider_type::s3, cfg_dir);
      configure_s3(*cfg);

      drive_args = {"-dd", cfg->get_data_directory(), "-s3", "-na", "s3"};

#if !defined(_WIN32)
      cfg->set_database_type(database_type::sqlite);
      config = std::move(cfg);
      meta = create_meta_db(*config);
#endif // !defined(_WIN32)

      platform_ops::execute_mount(drive_args, mount_location);
    };

    const auto mount_local_sia = [&](bool as_remote) {
      const auto test_dir = make_test_dir(
#if defined(_WIN32)
          "winfsp_test",
#else  // !defined(_WIN32)
          "fuse_test",
#endif // defined(_WIN32)
          fmt::format("{}_{}", app_config::get_provider_name(current_provider),
                      as_remote));
#if defined(_WIN32)
      mount_location = utils::string::to_lower(std::string{"U:"});
#else  // !defined(_WIN32)
      mount_location = utils::path::combine(test_dir, {"mount"});
      ASSERT_TRUE(utils::file::directory(mount_location).create_directory());
#endif // defined(_WIN32)

      auto cfg_dir = make_cfg_dir(test_dir, "cfg");
      auto cfg = std::make_unique<app_config>(provider_type::sia, cfg_dir);
      configure_sia(*cfg);

      drive_args = {"-dd", cfg->get_data_directory(), "-na", "sia"};

#if !defined(_WIN32)
      cfg->set_database_type(database_type::sqlite);
      config = std::move(cfg);
      meta = create_meta_db(*config);
#endif // !defined(_WIN32)

      platform_ops::execute_mount(drive_args, mount_location);
    };

    const auto mount_remote = [&] {
      const auto test_dir = make_test_dir(
#if defined(_WIN32)
          "winfsp_test",
#else  // !defined(_WIN32)
          "fuse_test",
#endif // defined(_WIN32)
          fmt::format("{}_{}_{}",
                      app_config::get_provider_name(provider_t::type),
                      app_config::get_provider_name(provider_t::type2),
                      provider_t::remote_port));

      mount_location2 = mount_location;

#if defined(_WIN32)
      auto letter = utils::get_available_drive_letter('d');
      ASSERT_TRUE(letter.has_value());
      mount_location = utils::string::to_lower(std::string{letter.value()});
#else  // !defined(_WIN32)
      mount_location = utils::path::combine(test_dir, {"mount"});
      ASSERT_TRUE(utils::file::directory(mount_location).create_directory());
#endif // defined(_WIN32)

      auto cfg_dir2 = make_cfg_dir(test_dir, "cfg2");
#if defined(_WIN32)
      auto config2 =
          std::make_unique<app_config>(provider_type::remote, cfg_dir2);
#else  // !defined(_WIN32)
      config2 = std::make_unique<app_config>(provider_type::remote, cfg_dir2);
#endif // defined(_WIN32)
      config2->set_enable_drive_events(true);
      config2->set_event_level(event_level::trace);
#if !defined(_WIN32)
      config2->set_database_type(database_type::sqlite);
#endif // !defined(_WIN32)

      auto rem_cfg = config2->get_remote_config();
      rem_cfg.host_name_or_ip = "localhost";
      rem_cfg.api_port = provider_t::remote_port;
      config2->set_remote_config(rem_cfg);
      drive_args2 = {
          "-dd",
          config2->get_data_directory(),
          "-rm",
          fmt::format("localhost:{}", provider_t::remote_port),
      };

      platform_ops::execute_mount(drive_args2, mount_location);
    };

    switch (provider_t::type) {
    case provider_type::s3: {
      mount_local_s3(false);
    } break;
    case provider_type::sia: {
      mount_local_sia(false);
    } break;
    case provider_type::remote: {
      switch (provider_t::type2) {
      case provider_type::s3:
        mount_local_s3(true);
        break;
      case provider_type::sia:
        mount_local_sia(true);
        break;
      case provider_type::unknown:
        mount_remote();
        return;
      default:
        throw std::runtime_error("remote provider type is not implemented");
      }
      mount_remote();
    } break;
    default:
      throw std::runtime_error("provider type is not implemented");
    }
  }

  static void TearDownTestCase() {
    if (provider_t::type == provider_type::remote) {
      platform_ops::execute_unmount(drive_args2, mount_location);
      if (provider_t::type2 != provider_type::unknown) {
        platform_ops::execute_unmount(drive_args, mount_location2);
      }
    } else {
      platform_ops::execute_unmount(drive_args, mount_location);
    }

#if !defined(_WIN32)
    meta.reset();
    config.reset();
    config2.reset();
#endif // !defined(_WIN32)
    std::filesystem::current_path(current_directory);
  }

#if !defined(_WIN32)
public:
  static auto create_file_path(std::string &file_name) {
    file_name += std::to_string(++provider_idx);
    return utils::path::combine(mount_location, {file_name});
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

    struct stat64 u_stat{};
    EXPECT_EQ(0, stat64(file_path.c_str(), &u_stat));
    EXPECT_EQ(getgid(), u_stat.st_gid);
    EXPECT_EQ(getuid(), u_stat.st_uid);

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
    EXPECT_EQ(0U, utils::file::directory(dir_path).count(false));
    EXPECT_EQ(0U, utils::file::directory(dir_path).count(true));
    EXPECT_FALSE(utils::file::file(dir_path).exists());

    struct stat64 u_stat{};
    EXPECT_EQ(0, stat64(dir_path.c_str(), &u_stat));
    EXPECT_EQ(getgid(), u_stat.st_gid);
    EXPECT_EQ(getuid(), u_stat.st_uid);

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

  static void unlink_root_file(std::string_view file_path) {
    auto api_path = utils::path::create_api_path(
        utils::path::strip_to_file_name(std::string{file_path}));

    [[maybe_unused]] auto res =
        meta->set_item_meta(api_path, {
                                          {META_UID, std::to_string(getuid())},
                                          {META_GID, std::to_string(getgid())},
                                      });
    std::this_thread::sleep_for(SLEEP_SECONDS);

    unlink_file_and_test(file_path);
  }

  static void overwrite_text(std::string_view path, std::string_view data) {
    int desc = ::open(std::string{path}.c_str(), O_WRONLY | O_TRUNC);
    ASSERT_NE(desc, -1);
    write_all(desc, data);
    ::close(desc);
  }

  static void write_all(int desc, std::string_view data) {
    std::size_t off{0U};
    while (off < data.size()) {
      auto written = ::write(desc, &data.at(off), data.length() - off);
      ASSERT_NE(written, -1);
      off += static_cast<size_t>(written);
    }
  }

  static auto slurp(std::string_view path) -> std::string {
    int desc = ::open(std::string{path}.c_str(), O_RDONLY);
    if (desc == -1) {
      return {};
    }
    std::string out;
    std::array<char, 4096U> buf{};
    for (;;) {
      auto bytes_read = ::read(desc, buf.data(), buf.size());
      if (bytes_read == 0) {
        break;
      }
      if (bytes_read == -1) {
        if (errno == EINTR) {
          continue;
        }
        break;
      }

      out.append(buf.begin(), std::next(buf.begin(), bytes_read));
    }

    ::close(desc);
    return out;
  }

  [[nodiscard]] static auto stat_size(std::string_view path) -> off_t {
    struct stat st_unix{};
    int res = ::stat(std::string{path}.c_str(), &st_unix);
    EXPECT_EQ(0, res) << "stat(" << path
                      << ") failed: " << std::strerror(errno);
    return st_unix.st_size;
  }

  static auto read_dirnames(DIR *dir) -> std::set<std::string> {
    std::set<std::string> names;
    while (auto *entry = ::readdir(dir)) {
      const auto *name = entry->d_name;
      if (std::strcmp(name, ".") == 0 || std::strcmp(name, "..") == 0) {
        continue;
      }
      names.emplace(name);
    }

    return names;
  }

#endif // !defined(_WIN32)
};

#if !defined(_WIN32)
template <typename provider_t>
std::unique_ptr<app_config> drive_fixture<provider_t>::config;

template <typename provider_t>
std::unique_ptr<app_config> drive_fixture<provider_t>::config2;

template <typename provider_t>
std::unique_ptr<i_meta_db> drive_fixture<provider_t>::meta{};
#endif // !defined(_WIN32)

template <typename provider_t>
std::filesystem::path drive_fixture<provider_t>::current_directory;

template <typename provider_t>
provider_type drive_fixture<provider_t>::current_provider{provider_t::type2};

template <typename provider_t>
std::vector<std::string> drive_fixture<provider_t>::drive_args;

template <typename provider_t>
std::vector<std::string> drive_fixture<provider_t>::drive_args2;

template <typename provider_t>
std::string drive_fixture<provider_t>::mount_location;

template <typename provider_t>
std::string drive_fixture<provider_t>::mount_location2;

template <typename provider_t>
provider_type drive_fixture<provider_t>::mount_provider{provider_t::type};

using platform_provider_types =
    ::testing::Types<local_s3_no_encryption, local_s3_encryption,
                     local_s3_legacy_encryption, remote_s3_no_encryption,
                     remote_s3_encryption, remote_s3_legacy_encryption,
                     local_sia, remote_sia>;
#if defined(_WIN32)
template <typename provider_t> using winfsp_test = drive_fixture<provider_t>;
#else  // !defined(_WIN32)
template <typename provider_t> using fuse_test = drive_fixture<provider_t>;
#endif // defined(_WIN32)
} // namespace repertory

#endif // REPERTORY_TEST_INCLUDE_FIXTURES_DRIVE_FIXTURE_HPP
