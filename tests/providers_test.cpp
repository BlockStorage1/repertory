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
#include "test_common.hpp"

#include "comm/curl/curl_comm.hpp"
#include "comm/s3/s3_comm.hpp"
#include "events/event_system.hpp"
#include "file_manager/file_manager.hpp"
#include "platform/platform.hpp"
#include "providers/encrypt/encrypt_provider.hpp"
#include "providers/i_provider.hpp"
#include "providers/s3/s3_provider.hpp"
#include "providers/sia/sia_provider.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"
#include "gtest/gtest.h"

namespace {
#ifdef _WIN32
static constexpr auto getgid() -> std::uint32_t { return 0U; }
static constexpr auto getuid() -> std::uint32_t { return 0U; }
#endif

const auto check_forced_dirs = [](const repertory::directory_item_list &list) {
  static auto forced_dirs = std::array<std::string, 2>{".", ".."};
  for (std::size_t i = 0U; i < forced_dirs.size(); ++i) {
    const auto &di = list.at(i);
    EXPECT_TRUE(di.directory);
    EXPECT_STREQ(forced_dirs.at(i).c_str(), di.api_path.c_str());
    EXPECT_STREQ("", di.api_parent.c_str());
    EXPECT_EQ(std::size_t(0U), di.size);
  }
};

const auto create_directory = [](repertory::i_provider &provider,
                                 const std::string &api_path) {
  auto date = repertory::utils::get_file_time_now();
  auto meta = repertory::create_meta_attributes(
      date, 0U, date + 1U, date + 2U, true, "", getgid(), "", 0700, date + 3U,
      1U, 2U, 0U, api_path + "_src", getuid(), date + 4U);
  EXPECT_EQ(repertory::api_error::success,
            provider.create_directory(api_path, meta));

  bool exists{};
  EXPECT_EQ(repertory::api_error::success,
            provider.is_directory(api_path, exists));
  EXPECT_TRUE(exists);
};

const auto create_file = [](repertory::i_provider &provider,
                            const std::string &api_path) {
  auto source_path = repertory::generate_test_file_name(".", "test");

  auto date = repertory::utils::get_file_time_now();
  auto meta = repertory::create_meta_attributes(
      date, 0U, date + 1U, date + 2U, false, "", getgid(), "", 0700, date + 3U,
      1U, 2U, 0U, source_path, getuid(), date + 4U);
  EXPECT_EQ(repertory::api_error::success,
            provider.create_file(api_path, meta));

  bool exists{};
  EXPECT_EQ(repertory::api_error::success, provider.is_file(api_path, exists));
  EXPECT_TRUE(exists);

  EXPECT_TRUE(repertory::utils::file::delete_file(source_path));
};

const auto decrypt_parts = [](const repertory::app_config &cfg,
                              std::string &path) {
  if (path != "/" && path != "." && path != "..") {
    auto parts = repertory::utils::string::split(path, '/', false);
    for (auto &part : parts) {
      if (part.empty()) {
        continue;
      }

      EXPECT_EQ(repertory::api_error::success,
                repertory::utils::encryption::decrypt_file_name(
                    cfg.get_encrypt_config().encryption_token, part));
    }
    path = repertory::utils::string::join(parts, '/');
  }
};
} // namespace

namespace repertory {
static void can_create_and_remove_directory(i_provider &provider) {
  if (provider.is_direct_only()) {
    api_meta_map meta{};
    EXPECT_EQ(api_error::not_implemented,
              provider.create_directory("/moose", meta));

    EXPECT_EQ(api_error::not_implemented, provider.remove_directory("/moose"));
    return;
  }

  create_directory(provider, "/pt01");
  EXPECT_EQ(api_error::success, provider.remove_directory("/pt01"));

  bool exists{};
  EXPECT_EQ(api_error::success, provider.is_directory("/pt01", exists));
  EXPECT_FALSE(exists);
}

static void create_directory_fails_if_already_exists(i_provider &provider) {
  if (provider.is_direct_only()) {
    return;
  }

  create_directory(provider, "/pt01");

  api_meta_map meta{};
  EXPECT_EQ(api_error::directory_exists,
            provider.create_directory("/pt01", meta));
  EXPECT_EQ(api_error::success, provider.remove_directory("/pt01"));
}

static void
create_directory_fails_if_file_already_exists(i_provider &provider) {
  if (provider.is_direct_only()) {
    return;
  }

  create_file(provider, "/pt01");

  api_meta_map meta{};
  EXPECT_EQ(api_error::item_exists, provider.create_directory("/pt01", meta));

  EXPECT_EQ(api_error::success, provider.remove_file("/pt01"));
}

static void create_directory_clone_source_meta(i_provider &provider) {
  if (provider.is_direct_only()) {
    EXPECT_EQ(api_error::not_implemented,
              provider.create_directory_clone_source_meta("/moose", "/moose"));
    return;
  }
  create_directory(provider, "/clone");

  api_meta_map meta_orig{};
  EXPECT_EQ(api_error::success, provider.get_item_meta("/clone", meta_orig));

  EXPECT_EQ(api_error::success,
            provider.create_directory_clone_source_meta("/clone", "/clone2"));

  api_meta_map meta_clone{};
  EXPECT_EQ(api_error::success, provider.get_item_meta("/clone2", meta_clone));

  EXPECT_EQ(meta_orig.size(), meta_clone.size());
  for (const auto &kv : meta_orig) {
    EXPECT_STREQ(kv.second.c_str(), meta_clone[kv.first].c_str());
  }

  EXPECT_EQ(api_error::success, provider.remove_directory("/clone"));
  EXPECT_EQ(api_error::success, provider.remove_directory("/clone2"));
}

static void create_directory_clone_source_meta_fails_if_already_exists(
    i_provider &provider) {
  if (provider.is_direct_only()) {
    return;
  }
  create_directory(provider, "/clone");
  create_directory(provider, "/clone2");

  EXPECT_EQ(api_error::directory_exists,
            provider.create_directory_clone_source_meta("/clone", "/clone2"));

  EXPECT_EQ(api_error::success, provider.remove_directory("/clone"));
  EXPECT_EQ(api_error::success, provider.remove_directory("/clone2"));
}

static void create_directory_clone_source_meta_fails_if_directory_not_found(
    i_provider &provider) {
  if (provider.is_direct_only()) {
    return;
  }

  EXPECT_EQ(api_error::directory_not_found,
            provider.create_directory_clone_source_meta("/clone", "/clone2"));
}

static void create_directory_clone_source_meta_fails_if_file_already_exists(
    i_provider &provider) {
  if (provider.is_direct_only()) {
    return;
  }

  create_directory(provider, "/clone");
  create_file(provider, "/clone2");

  EXPECT_EQ(api_error::item_exists,
            provider.create_directory_clone_source_meta("/clone", "/clone2"));

  EXPECT_EQ(api_error::success, provider.remove_directory("/clone"));
  EXPECT_EQ(api_error::success, provider.remove_file("/clone2"));
}

static void can_create_and_remove_file(i_provider &provider) {
  if (provider.is_direct_only()) {
    api_meta_map meta{};
    EXPECT_EQ(api_error::not_implemented,
              provider.create_file("/moose.txt", meta));
    return;
  }

  create_file(provider, "/pt01.txt");

  bool exists{};
  EXPECT_EQ(api_error::success, provider.is_file("/pt01.txt", exists));
  EXPECT_TRUE(exists);

  EXPECT_EQ(api_error::success, provider.remove_file("/pt01.txt"));

  EXPECT_EQ(api_error::success, provider.is_file("/pt01.txt", exists));
  EXPECT_FALSE(exists);
}

static void create_file_fails_if_already_exists(i_provider &provider) {
  if (provider.is_direct_only()) {
    return;
  }

  create_file(provider, "/pt01.txt");

  api_meta_map meta{};
  EXPECT_EQ(api_error::item_exists, provider.create_file("/pt01.txt", meta));

  EXPECT_EQ(api_error::success, provider.remove_file("/pt01.txt"));
}

static void
create_file_fails_if_directory_already_exists(i_provider &provider) {
  if (provider.is_direct_only()) {
    return;
  }

  create_directory(provider, "/pt01");

  api_meta_map meta{};
  EXPECT_EQ(api_error::directory_exists, provider.create_file("/pt01", meta));

  EXPECT_EQ(api_error::success, provider.remove_directory("/pt01"));
}

static void get_api_path_from_source(const app_config &cfg,
                                     i_provider &provider) {
  if (provider.get_provider_type() == provider_type::encrypt) {
    const auto source_path =
        utils::path::combine(cfg.get_encrypt_config().path, {"test.txt"});

    std::string api_path{};
    EXPECT_EQ(api_error::success,
              provider.get_api_path_from_source(source_path, api_path));

    std::string file_name{api_path.substr(1U)};
    decrypt_parts(cfg, file_name);
    EXPECT_STREQ("test.txt", file_name.c_str());
    return;
  }

  create_file(provider, "/pt01.txt");

  filesystem_item fsi{};
  EXPECT_EQ(api_error::success,
            provider.get_filesystem_item("/pt01.txt", false, fsi));

  std::string api_path{};
  EXPECT_EQ(api_error::success,
            provider.get_api_path_from_source(fsi.source_path, api_path));

  EXPECT_STREQ("/pt01.txt", api_path.c_str());
}

static void
get_api_path_from_source_fails_if_file_not_found(const app_config &cfg,
                                                 i_provider &provider) {
  std::string source_path{};
  if (provider.get_provider_type() == provider_type::encrypt) {
    source_path = utils::path::combine(cfg.get_encrypt_config().path,
                                       {"test_not_found.txt"});
  } else {
    source_path = utils::path::combine("./", {"test_not_found.txt"});
  }

  std::string api_path{};
  EXPECT_EQ(api_error::item_not_found,
            provider.get_api_path_from_source(source_path, api_path));

  EXPECT_TRUE(api_path.empty());
}

static void get_directory_item_count(const app_config &cfg,
                                     i_provider &provider) {
  if (provider.get_provider_type() == provider_type::encrypt) {
    EXPECT_EQ(std::size_t(2U), provider.get_directory_item_count("/"));
    EXPECT_EQ(std::size_t(0U), provider.get_directory_item_count("/not_found"));

    const auto source_path =
        utils::path::combine(cfg.get_encrypt_config().path, {"sub10"});

    std::string api_path{};
    EXPECT_EQ(api_error::success,
              provider.get_api_path_from_source(source_path, api_path));
    EXPECT_EQ(std::size_t(1U), provider.get_directory_item_count(api_path));
  }
}

static void get_directory_items(const app_config &cfg, i_provider &provider) {
  directory_item_list list{};
  EXPECT_EQ(api_error::success, provider.get_directory_items("/", list));
  check_forced_dirs(list);

  if (provider.get_provider_type() == provider_type::encrypt) {
    EXPECT_EQ(std::size_t(4U), list.size());

    directory_item_list list_decrypted{list.begin() + 2U, list.end()};
    for (auto &di : list_decrypted) {
      decrypt_parts(cfg, di.api_parent);
      decrypt_parts(cfg, di.api_path);
    }

    auto dir = std::find_if(
        list_decrypted.begin(), list_decrypted.end(),
        [](const directory_item &di) -> bool { return di.directory; });
    EXPECT_LT(dir, list_decrypted.end());
    EXPECT_STREQ("/sub10", dir->api_path.c_str());
    EXPECT_STREQ("/", dir->api_parent.c_str());
    EXPECT_EQ(std::size_t(0U), dir->size);

    auto file = std::find_if(
        list_decrypted.begin(), list_decrypted.end(),
        [](const directory_item &di) -> bool { return not di.directory; });
    EXPECT_LT(file, list_decrypted.end());
    EXPECT_STREQ("/test.txt", file->api_path.c_str());
    EXPECT_STREQ("/", file->api_parent.c_str());
#ifdef _WIN32
    EXPECT_EQ(std::size_t(47U), file->size);
#else
    EXPECT_EQ(std::size_t(46U), file->size);
#endif

    const auto source_path =
        utils::path::combine(cfg.get_encrypt_config().path, {"sub10"});
    std::string api_path{};
    EXPECT_EQ(api_error::success,
              provider.get_api_path_from_source(source_path, api_path));

    list.clear();
    EXPECT_EQ(api_error::success, provider.get_directory_items(api_path, list));
    check_forced_dirs(list);
    EXPECT_EQ(std::size_t(3U), list.size());

    directory_item_list list_decrypted2{list.begin() + 2U, list.end()};
    for (auto &di : list_decrypted2) {
      decrypt_parts(cfg, di.api_parent);
      decrypt_parts(cfg, di.api_path);
    }

    auto file2 = std::find_if(
        list_decrypted2.begin(), list_decrypted2.end(),
        [](const directory_item &di) -> bool { return not di.directory; });
    EXPECT_LT(file2, list_decrypted2.end());
    EXPECT_STREQ("/sub10/moose.txt", file2->api_path.c_str());
    EXPECT_STREQ("/sub10", file2->api_parent.c_str());
#ifdef _WIN32
    EXPECT_EQ(std::size_t(46U), file2->size);
#else
    EXPECT_EQ(std::size_t(45U), file2->size);
#endif
  }
}

static void
get_directory_items_fails_if_directory_not_found(i_provider &provider) {
  directory_item_list list{};
  EXPECT_EQ(api_error::directory_not_found,
            provider.get_directory_items("/not_found", list));
  EXPECT_TRUE(list.empty());
}

static void get_directory_items_fails_if_item_is_file(const app_config &cfg,
                                                      i_provider &provider) {
  if (provider.get_provider_type() == provider_type::encrypt) {
    const auto source_path =
        utils::path::combine(cfg.get_encrypt_config().path, {"test.txt"});

    std::string api_path{};
    EXPECT_EQ(api_error::success,
              provider.get_api_path_from_source(source_path, api_path));

    directory_item_list list{};
    EXPECT_EQ(api_error::item_exists,
              provider.get_directory_items(api_path, list));
    EXPECT_TRUE(list.empty());
  }
}

static void get_file(const app_config &cfg, i_provider &provider) {
  if (provider.get_provider_type() == provider_type::encrypt) {
    const auto source_path =
        utils::path::combine(cfg.get_encrypt_config().path, {"test.txt"});

    std::string api_path{};
    EXPECT_EQ(api_error::success,
              provider.get_api_path_from_source(source_path, api_path));

    api_file file{};
    EXPECT_EQ(api_error::success, provider.get_file(api_path, file));
    decrypt_parts(cfg, file.api_path);
    decrypt_parts(cfg, file.api_parent);

    EXPECT_STREQ("/test.txt", file.api_path.c_str());
    EXPECT_STREQ("/", file.api_parent.c_str());
#ifdef _WIN32
    EXPECT_EQ(std::size_t(47U), file.file_size);
#else
    EXPECT_EQ(std::size_t(46U), file.file_size);
#endif
    EXPECT_TRUE(file.encryption_token.empty());
    EXPECT_STREQ(source_path.c_str(), file.source_path.c_str());
  }
}

static void get_file_fails_if_file_not_found(i_provider &provider) {
  api_file file{};
  EXPECT_EQ(api_error::item_not_found, provider.get_file("/not_found", file));
}

static void get_file_fails_if_item_is_directory(const app_config &cfg,
                                                i_provider &provider) {
  if (provider.get_provider_type() == provider_type::encrypt) {
    const auto source_path =
        utils::path::combine(cfg.get_encrypt_config().path, {"sub10"});

    std::string api_path{};
    EXPECT_EQ(api_error::success,
              provider.get_api_path_from_source(source_path, api_path));

    api_file file{};
    EXPECT_EQ(api_error::directory_exists, provider.get_file(api_path, file));
  }
}

static void get_file_list(const app_config &cfg, i_provider &provider) {
  api_file_list list{};
  EXPECT_EQ(api_error::success, provider.get_file_list(list));
  if (provider.get_provider_type() == provider_type::encrypt) {
    EXPECT_EQ(std::size_t(2U), list.size());

    std::vector<std::string> expected_parents{
        {"/"},
        {"/sub10"},
    };
    std::vector<std::string> expected_paths{
        {"/test.txt"},
        {"/sub10/moose.txt"},
    };

    for (auto &file : list) {
      decrypt_parts(cfg, file.api_parent);
      decrypt_parts(cfg, file.api_path);
      utils::remove_element_from(expected_parents, file.api_parent);
      utils::remove_element_from(expected_paths, file.api_path);
    }
    EXPECT_TRUE(expected_parents.empty());
    EXPECT_TRUE(expected_paths.empty());
  }
}

static void run_tests(const app_config &cfg, i_provider &provider) {
  get_file_list(cfg, provider);
  ASSERT_FALSE(::testing::Test::HasFailure());

  can_create_and_remove_directory(provider);
  can_create_and_remove_file(provider);

  create_directory_fails_if_already_exists(provider);
  create_directory_fails_if_file_already_exists(provider);

  create_directory_clone_source_meta(provider);
  create_directory_clone_source_meta_fails_if_already_exists(provider);
  create_directory_clone_source_meta_fails_if_directory_not_found(provider);
  create_directory_clone_source_meta_fails_if_file_already_exists(provider);

  create_file_fails_if_already_exists(provider);
  create_file_fails_if_directory_already_exists(provider);

  get_api_path_from_source(cfg, provider);
  get_api_path_from_source_fails_if_file_not_found(cfg, provider);

  // TODO: continue here
  get_directory_item_count(cfg, provider);

  get_directory_items(cfg, provider);
  get_directory_items_fails_if_directory_not_found(provider);
  get_directory_items_fails_if_item_is_file(cfg, provider);

  get_file(cfg, provider);
  get_file_fails_if_file_not_found(provider);
  get_file_fails_if_item_is_directory(cfg, provider);
}

TEST(providers, encrypt_provider) {
  const auto config_path = utils::path::absolute("./providers_test_encrypt");
  ASSERT_TRUE(utils::file::delete_directory_recursively(config_path));

  console_consumer cc{};
  event_system::instance().start();
  {
    app_config cfg(provider_type::encrypt, config_path);

    const auto encrypt_path = utils::path::combine(
        std::filesystem::path(utils::path::absolute(__FILE__))
            .parent_path()
            .string(),
        {"encrypt"});

    EXPECT_STREQ(
        encrypt_path.c_str(),
        cfg.set_value_by_name("EncryptConfig.Path", encrypt_path.c_str())
            .c_str());
    EXPECT_STREQ(
        "test_token",
        cfg.set_value_by_name("EncryptConfig.EncryptionToken", "test_token")
            .c_str());

    encrypt_provider provider{cfg};
    file_manager fm(cfg, provider);
    fm.start();

    EXPECT_TRUE(provider.start(
        [&provider](bool directory, api_file &file) -> api_error {
          return provider_meta_handler(provider, directory, file);
        },
        &fm));
    EXPECT_EQ(provider_type::encrypt, provider.get_provider_type());
    EXPECT_TRUE(provider.is_direct_only());
    EXPECT_TRUE(provider.is_online());
    EXPECT_FALSE(provider.is_rename_supported());

    run_tests(cfg, provider);

    provider.stop();
    fm.stop();
  }
  event_system::instance().stop();

  ASSERT_TRUE(utils::file::delete_directory_recursively(config_path));
}

#if defined(REPERTORY_ENABLE_S3) && defined(REPERTORY_ENABLE_S3_TESTING)
TEST(providers, s3_provider) {
  const auto config_path = utils::path::absolute("./providers_test_s3");
  ASSERT_TRUE(utils::file::delete_directory_recursively(config_path));

  console_consumer cc{};
  event_system::instance().start();
  {
    app_config cfg(provider_type::s3, config_path);
    {
      app_config src_cfg(provider_type::s3,
                         utils::path::combine(get_test_dir(), {"filebase"}));
      cfg.set_s3_config(src_cfg.get_s3_config());
    }

    s3_comm comm{cfg};
    s3_provider provider{cfg, comm};
    file_manager fm(cfg, provider);
    fm.start();

    EXPECT_TRUE(provider.start(
        [&provider](bool directory, api_file &file) -> api_error {
          return provider_meta_handler(provider, directory, file);
        },
        &fm));
    EXPECT_EQ(provider_type::s3, provider.get_provider_type());
    EXPECT_FALSE(provider.is_direct_only());
    EXPECT_TRUE(provider.is_online());
    EXPECT_FALSE(provider.is_rename_supported());

    run_tests(cfg, provider);

    provider.stop();
    fm.stop();
  }
  event_system::instance().stop();

  ASSERT_TRUE(utils::file::delete_directory_recursively(config_path));
}
#endif

TEST(providers, sia_provider) {
  const auto config_path = utils::path::absolute("./providers_test_sia");
  ASSERT_TRUE(utils::file::delete_directory_recursively(config_path));

  console_consumer cc{};
  event_system::instance().start();
  {
    app_config cfg(provider_type::sia, config_path);
    {
      app_config src_cfg(provider_type::sia,
                         utils::path::combine(get_test_dir(), {"sia"}));
      cfg.set_host_config(src_cfg.get_host_config());
    }

    curl_comm comm{cfg.get_host_config()};
    sia_provider provider{cfg, comm};
    file_manager fm(cfg, provider);
    fm.start();

    EXPECT_TRUE(provider.start(
        [&provider](bool directory, api_file &file) -> api_error {
          return provider_meta_handler(provider, directory, file);
        },
        &fm));
    EXPECT_EQ(provider_type::sia, provider.get_provider_type());
    EXPECT_FALSE(provider.is_direct_only());
    EXPECT_TRUE(provider.is_online());
    EXPECT_FALSE(provider.is_rename_supported());

    run_tests(cfg, provider);

    provider.stop();
    fm.stop();
  }
  event_system::instance().stop();

  ASSERT_TRUE(utils::file::delete_directory_recursively(config_path));
}
} // namespace repertory
