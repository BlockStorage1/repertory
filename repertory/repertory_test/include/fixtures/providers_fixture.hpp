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
#ifndef REPERTORY_TEST_INCLUDE_FIXTURES_PROVIDERS_FIXTURE_HPP
#define REPERTORY_TEST_INCLUDE_FIXTURES_PROVIDERS_FIXTURE_HPP

#include "test_common.hpp"

#include "comm/curl/curl_comm.hpp"
#include "comm/i_http_comm.hpp"
#include "events/event_system.hpp"
#include "file_manager/file_manager.hpp"
#include "platform/platform.hpp"
#include "providers/encrypt/encrypt_provider.hpp"
#include "providers/i_provider.hpp"
#include "providers/s3/s3_provider.hpp"
#include "providers/sia/sia_provider.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/utils.hpp"

#if defined(_WIN32)
namespace {
using gid_t = std::uint32_t;
using uid_t = std::uint32_t;
static constexpr auto getgid() -> gid_t { return 0U; }
static constexpr auto getuid() -> uid_t { return 0U; }
} // namespace
#endif // defined(_WIN32)

namespace repertory {
struct encrypt_provider_type final {
  static constexpr provider_type type{provider_type::encrypt};

  static void setup(std::unique_ptr<i_http_comm> & /* comm */,
                    std::unique_ptr<app_config> &config,
                    std::unique_ptr<i_provider> &provider) {
    auto config_path =
        utils::path::combine(test::get_test_output_dir(), {
                                                              "provider",
                                                              "encrypt",
                                                          });

    config = std::make_unique<app_config>(type, config_path);

    auto encrypt_path =
        utils::path::combine(test::get_test_input_dir(), {"encrypt"});

    EXPECT_STREQ(
        encrypt_path.c_str(),
        config->set_value_by_name("EncryptConfig.Path", encrypt_path).c_str());
    EXPECT_STREQ(
        "test_token",
        config->set_value_by_name("EncryptConfig.EncryptionToken", "test_token")
            .c_str());

    provider = std::make_unique<encrypt_provider>(*config);
    EXPECT_TRUE(provider->is_read_only());
    EXPECT_FALSE(provider->is_rename_supported());
    EXPECT_EQ(type, provider->get_provider_type());
  }
};

struct s3_provider_encrypted_type final {
  static constexpr provider_type type{provider_type::s3};

  static void setup(std::unique_ptr<i_http_comm> &comm,
                    std::unique_ptr<app_config> &config,
                    std::unique_ptr<i_provider> &provider) {
    auto config_path =
        utils::path::combine(test::get_test_output_dir(), {
                                                              "provider",
                                                              "s3",
                                                          });

    config = std::make_unique<app_config>(type, config_path);
    {
      app_config src_cfg(
          type, utils::path::combine(test::get_test_config_dir(), {"s3"}));
      auto s3_cfg = src_cfg.get_s3_config();
      s3_cfg.encryption_token = "cow_moose_doge_chicken";
      config->set_s3_config(s3_cfg);
    }

    comm = std::make_unique<curl_comm>(config->get_s3_config());

    provider = std::make_unique<s3_provider>(*config, *comm);
    EXPECT_EQ(type, provider->get_provider_type());
    EXPECT_FALSE(provider->is_read_only());
    EXPECT_FALSE(provider->is_rename_supported());
  }
};

struct s3_provider_unencrypted_type final {
  static constexpr provider_type type{provider_type::s3};

  static void setup(std::unique_ptr<i_http_comm> &comm,
                    std::unique_ptr<app_config> &config,
                    std::unique_ptr<i_provider> &provider) {
    auto config_path =
        utils::path::combine(test::get_test_output_dir(), {
                                                              "provider",
                                                              "s3",
                                                          });

    config = std::make_unique<app_config>(type, config_path);
    {
      app_config src_cfg(
          type, utils::path::combine(test::get_test_config_dir(), {"s3"}));
      auto s3_cfg = src_cfg.get_s3_config();
      s3_cfg.encryption_token = "";
      config->set_s3_config(s3_cfg);
    }

    comm = std::make_unique<curl_comm>(config->get_s3_config());

    provider = std::make_unique<s3_provider>(*config, *comm);
    EXPECT_EQ(type, provider->get_provider_type());
    EXPECT_FALSE(provider->is_read_only());
    EXPECT_FALSE(provider->is_rename_supported());
  }
};

struct sia_provider_type final {
  static constexpr provider_type type{provider_type::sia};

  static void setup(std::unique_ptr<i_http_comm> &comm,
                    std::unique_ptr<app_config> &config,
                    std::unique_ptr<i_provider> &provider) {
    auto config_path =
        utils::path::combine(test::get_test_output_dir(), {
                                                              "provider",
                                                              "sia",
                                                          });

    config = std::make_unique<app_config>(type, config_path);
    {
      app_config src_cfg(
          provider_type::sia,
          utils::path::combine(test::get_test_config_dir(), {"sia"}));
      config->set_host_config(src_cfg.get_host_config());
      config->set_sia_config(src_cfg.get_sia_config());
    }

    comm = std::make_unique<curl_comm>(config->get_host_config());

    provider = std::make_unique<sia_provider>(*config, *comm);

    EXPECT_EQ(type, provider->get_provider_type());
    EXPECT_FALSE(provider->is_read_only());
    EXPECT_TRUE(provider->is_rename_supported());
  }
};

template <typename provider_t> class providers_test : public ::testing::Test {
public:
  static std::unique_ptr<i_http_comm> comm;
  static std::unique_ptr<app_config> config;
  static console_consumer consumer;
  static std::unique_ptr<file_manager> mgr;
  static std::unique_ptr<i_provider> provider;

protected:
  static void SetUpTestCase() {
    event_system::instance().start();

    provider_t::setup(comm, config, provider);
    mgr = std::make_unique<file_manager>(*config, *provider);

    EXPECT_TRUE(provider->start(
        [](bool directory, api_file &file) -> api_error {
          return provider_meta_handler(*provider, directory, file);
        },
        mgr.get()));

    mgr->start();
    EXPECT_TRUE(provider->is_online());
  }

  static void TearDownTestCase() {
    provider->stop();
    mgr->stop();

    mgr.reset();
    provider.reset();
    comm.reset();

    event_system::instance().stop();
  }

protected:
  static void check_forced_dirs(const directory_item_list &list) {
    static auto forced_dirs = std::array<std::string, 2>{".", ".."};
    for (std::size_t i = 0U; i < forced_dirs.size(); ++i) {
      const auto &item = list.at(i);
      EXPECT_TRUE(item.directory);
      EXPECT_STREQ(forced_dirs.at(i).c_str(), item.api_path.c_str());
      EXPECT_STREQ("", item.api_parent.c_str());
      EXPECT_EQ(std::size_t(0U), item.size);
    }
  }

  static void create_directory(std::string_view api_path) {
    auto date = utils::time::get_time_now();
    auto meta = create_meta_attributes(
        date, 1U, date + 1U, date + 2U, true, getgid(), "", 0700, date + 3U, 2U,
        0U, std::string{api_path} + "_src", getuid(), date + 4U);
    EXPECT_EQ(api_error::success, provider->create_directory(api_path, meta));

    bool exists{};
    EXPECT_EQ(api_error::success, provider->is_directory(api_path, exists));
    EXPECT_TRUE(exists);

    api_meta_map meta2{};
    EXPECT_EQ(api_error::success, provider->get_item_meta(api_path, meta2));

    EXPECT_EQ(date, utils::string::to_uint64(meta2[META_ACCESSED]));
    EXPECT_EQ(1U, utils::string::to_uint64(meta2[META_ATTRIBUTES]));
    EXPECT_EQ(date + 1U, utils::string::to_uint64(meta2[META_CHANGED]));
    EXPECT_EQ(date + 2U, utils::string::to_uint64(meta2[META_CREATION]));
    EXPECT_TRUE(utils::string::to_bool(meta2.at(META_DIRECTORY)));
    EXPECT_EQ(getgid(),
              static_cast<gid_t>(utils::string::to_uint32(meta2[META_GID])));
    EXPECT_EQ(std::uint32_t(0700), utils::string::to_uint32(meta2[META_MODE]));
    EXPECT_EQ(date + 3U, utils::string::to_uint64(meta2[META_MODIFIED]));
    EXPECT_EQ(2U, utils::string::to_uint64(meta2[META_OSXFLAGS]));
    EXPECT_FALSE(utils::string::to_bool(meta2[META_PINNED]));
    EXPECT_EQ(std::uint64_t(0U), utils::string::to_uint64(meta2[META_SIZE]));
    EXPECT_EQ(getuid(),
              static_cast<uid_t>(utils::string::to_uint32(meta2[META_UID])));
    EXPECT_EQ(date + 4U, utils::string::to_uint64(meta2[META_WRITTEN]));
  }

  static void create_file(std::string_view api_path) {
    auto source_path = test::generate_test_file_name("providers_test");

    auto date = utils::time::get_time_now();
    auto meta = create_meta_attributes(date, 1U, date + 1U, date + 2U, false,
                                       getgid(), "", 0700, date + 3U, 2U, 0U,
                                       source_path, getuid(), date + 4U);
    EXPECT_EQ(api_error::success, provider->create_file(api_path, meta));

    bool exists{};
    EXPECT_EQ(api_error::success, provider->is_file(api_path, exists));
    EXPECT_TRUE(exists);

    EXPECT_TRUE(utils::file::file{source_path}.remove());

    api_meta_map meta2{};
    EXPECT_EQ(api_error::success, provider->get_item_meta(api_path, meta2));

    EXPECT_EQ(date, utils::string::to_uint64(meta2[META_ACCESSED]));
    EXPECT_EQ(1U, utils::string::to_uint64(meta2[META_ATTRIBUTES]));
    EXPECT_EQ(date + 1U, utils::string::to_uint64(meta2[META_CHANGED]));
    EXPECT_EQ(date + 2U, utils::string::to_uint64(meta2[META_CREATION]));
    EXPECT_FALSE(utils::string::to_bool(meta2.at(META_DIRECTORY)));
    EXPECT_EQ(getgid(),
              static_cast<gid_t>(utils::string::to_uint32(meta2[META_GID])));
    EXPECT_EQ(std::uint32_t(0700), utils::string::to_uint32(meta2[META_MODE]));
    EXPECT_EQ(date + 3U, utils::string::to_uint64(meta2[META_MODIFIED]));
    EXPECT_EQ(2U, utils::string::to_uint64(meta2[META_OSXFLAGS]));
    EXPECT_FALSE(utils::string::to_bool(meta2[META_PINNED]));
    EXPECT_EQ(std::uint64_t(0U), utils::string::to_uint64(meta2[META_SIZE]));
    EXPECT_STREQ(source_path.c_str(), meta2[META_SOURCE].c_str());
    EXPECT_EQ(getuid(),
              static_cast<uid_t>(utils::string::to_uint32(meta2[META_UID])));
    EXPECT_EQ(date + 4U, utils::string::to_uint64(meta2[META_WRITTEN]));
  }

  static void decrypt_parts(std::string &path) {
    if (path != "/" && path != "." && path != "..") {
      utils::hash::hash_256_t key{};
      EXPECT_TRUE(utils::encryption::recreate_key_argon2id(
          config->get_encrypt_config().encryption_token,
          config->get_encrypt_config().kdf_cfg, key));
      auto parts = utils::string::split(path, '/', false);
      for (auto &part : parts) {
        if (part.empty()) {
          continue;
        }

        EXPECT_TRUE(utils::encryption::decrypt_file_name(key, part));
      }
      path = utils::string::join(parts, '/');
    }
  }

  [[nodiscard]] static auto
  pinned_includes_api_path(const auto &pinned, std::string_view expected_path)
      -> bool {
    return std::ranges::any_of(pinned,
                               [&expected_path](auto &&api_path) -> bool {
                                 return api_path == expected_path;
                               });
  };
};

template <typename provider_t>
std::unique_ptr<i_http_comm> providers_test<provider_t>::comm;

template <typename provider_t>
std::unique_ptr<app_config> providers_test<provider_t>::config;

template <typename provider_t>
console_consumer providers_test<provider_t>::consumer;

template <typename provider_t>
std::unique_ptr<file_manager> providers_test<provider_t>::mgr;

template <typename provider_t>
std::unique_ptr<i_provider> providers_test<provider_t>::provider;

using provider_types =
    ::testing::Types<encrypt_provider_type, s3_provider_encrypted_type,
                     s3_provider_unencrypted_type, sia_provider_type>;
} // namespace repertory

#endif // REPERTORY_TEST_INCLUDE_FIXTURES_PROVIDERS_FIXTURE_HPP
