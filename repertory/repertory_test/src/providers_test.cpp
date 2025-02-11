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
#if 0

#include "test_common.hpp"

#include "comm/curl/curl_comm.hpp"
#include "events/event_system.hpp"
#include "file_manager/file_manager.hpp"
#include "platform/platform.hpp"
#include "providers/encrypt/encrypt_provider.hpp"
#include "providers/i_provider.hpp"
#include "providers/s3/s3_provider.hpp"
#include "providers/sia/sia_provider.hpp"
#include "utils/collection.hpp"
#include "utils/file_utils.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"
#include "utils/utils.hpp"

namespace {
#if defined(_WIN32)
using gid_t = std::uint32_t;
using uid_t = std::uint32_t;
static constexpr auto getgid() -> gid_t { return 0U; }
static constexpr auto getuid() -> uid_t { return 0U; }
#endif // defined(_WIN32)

const auto check_forced_dirs = [](const repertory::directory_item_list &list) {
  static auto forced_dirs = std::array<std::string, 2>{".", ".."};
  for (std::size_t i = 0U; i < forced_dirs.size(); ++i) {
    const auto &item = list.at(i);
    EXPECT_TRUE(item.directory);
    EXPECT_STREQ(forced_dirs.at(i).c_str(), item.api_path.c_str());
    EXPECT_STREQ("", item.api_parent.c_str());
    EXPECT_EQ(std::size_t(0U), item.size);
  }
};

const auto create_directory = [](repertory::i_provider &provider,
                                 const std::string &api_path) {
  auto date = repertory::utils::time::get_time_now();
  auto meta = repertory::create_meta_attributes(
      date, 1U, date + 1U, date + 2U, true, getgid(), "", 0700, date + 3U, 2U,
      3U, 0U, api_path + "_src", getuid(), date + 4U);
  EXPECT_EQ(repertory::api_error::success,
            provider.create_directory(api_path, meta));

  bool exists{};
  EXPECT_EQ(repertory::api_error::success,
            provider.is_directory(api_path, exists));
  EXPECT_TRUE(exists);

  repertory::api_meta_map meta2{};
  EXPECT_EQ(repertory::api_error::success,
            provider.get_item_meta(api_path, meta2));

  EXPECT_EQ(date, repertory::utils::string::to_uint64(
                      meta2[repertory::META_ACCESSED]));
  EXPECT_EQ(1U, repertory::utils::string::to_uint64(
                    meta2[repertory::META_ATTRIBUTES]));
  EXPECT_EQ(date + 1U, repertory::utils::string::to_uint64(
                           meta2[repertory::META_CHANGED]));
  EXPECT_EQ(date + 2U, repertory::utils::string::to_uint64(
                           meta2[repertory::META_CREATION]));
  EXPECT_TRUE(
      repertory::utils::string::to_bool(meta2.at(repertory::META_DIRECTORY)));
  EXPECT_EQ(getgid(), static_cast<gid_t>(repertory::utils::string::to_uint32(
                          meta2[repertory::META_GID])));
  EXPECT_EQ(std::uint32_t(0700),
            repertory::utils::string::to_uint32(meta2[repertory::META_MODE]));
  EXPECT_EQ(date + 3U, repertory::utils::string::to_uint64(
                           meta2[repertory::META_MODIFIED]));
  EXPECT_EQ(2U,
            repertory::utils::string::to_uint64(meta2[repertory::META_BACKUP]));
  EXPECT_EQ(
      3U, repertory::utils::string::to_uint64(meta2[repertory::META_OSXFLAGS]));
  EXPECT_FALSE(
      repertory::utils::string::to_bool(meta2[repertory::META_PINNED]));
  EXPECT_EQ(std::uint64_t(0U),
            repertory::utils::string::to_uint64(meta2[repertory::META_SIZE]));
  EXPECT_STREQ((api_path + "_src").c_str(),
               meta2[repertory::META_SOURCE].c_str());
  EXPECT_EQ(getuid(), static_cast<uid_t>(repertory::utils::string::to_uint32(
                          meta2[repertory::META_UID])));
  EXPECT_EQ(date + 4U, repertory::utils::string::to_uint64(
                           meta2[repertory::META_WRITTEN]));
};

const auto create_file = [](repertory::i_provider &provider,
                            const std::string &api_path) {
  auto source_path = repertory::test::generate_test_file_name("providers_test");

  auto date = repertory::utils::time::get_time_now();
  auto meta = repertory::create_meta_attributes(
      date, 1U, date + 1U, date + 2U, false, getgid(), "", 0700, date + 3U, 2U,
      3U, 0U, source_path, getuid(), date + 4U);
  EXPECT_EQ(repertory::api_error::success,
            provider.create_file(api_path, meta));

  bool exists{};
  EXPECT_EQ(repertory::api_error::success, provider.is_file(api_path, exists));
  EXPECT_TRUE(exists);

  EXPECT_TRUE(repertory::utils::file::file{source_path}.remove());

  repertory::api_meta_map meta2{};
  EXPECT_EQ(repertory::api_error::success,
            provider.get_item_meta(api_path, meta2));

  EXPECT_EQ(date, repertory::utils::string::to_uint64(
                      meta2[repertory::META_ACCESSED]));
  EXPECT_EQ(1U, repertory::utils::string::to_uint64(
                    meta2[repertory::META_ATTRIBUTES]));
  EXPECT_EQ(date + 1U, repertory::utils::string::to_uint64(
                           meta2[repertory::META_CHANGED]));
  EXPECT_EQ(date + 2U, repertory::utils::string::to_uint64(
                           meta2[repertory::META_CREATION]));
  EXPECT_FALSE(
      repertory::utils::string::to_bool(meta2.at(repertory::META_DIRECTORY)));
  EXPECT_EQ(getgid(), static_cast<gid_t>(repertory::utils::string::to_uint32(
                          meta2[repertory::META_GID])));
  EXPECT_EQ(std::uint32_t(0700),
            repertory::utils::string::to_uint32(meta2[repertory::META_MODE]));
  EXPECT_EQ(date + 3U, repertory::utils::string::to_uint64(
                           meta2[repertory::META_MODIFIED]));
  EXPECT_EQ(2U,
            repertory::utils::string::to_uint64(meta2[repertory::META_BACKUP]));
  EXPECT_EQ(
      3U, repertory::utils::string::to_uint64(meta2[repertory::META_OSXFLAGS]));
  EXPECT_FALSE(
      repertory::utils::string::to_bool(meta2[repertory::META_PINNED]));
  EXPECT_EQ(std::uint64_t(0U),
            repertory::utils::string::to_uint64(meta2[repertory::META_SIZE]));
  EXPECT_STREQ(source_path.c_str(), meta2[repertory::META_SOURCE].c_str());
  EXPECT_EQ(getuid(), static_cast<uid_t>(repertory::utils::string::to_uint32(
                          meta2[repertory::META_UID])));
  EXPECT_EQ(date + 4U, repertory::utils::string::to_uint64(
                           meta2[repertory::META_WRITTEN]));
};

const auto decrypt_parts = [](const repertory::app_config &cfg,
                              std::string &path) {
  if (path != "/" && path != "." && path != "..") {
    auto parts = repertory::utils::string::split(path, '/', false);
    for (auto &part : parts) {
      if (part.empty()) {
        continue;
      }

      EXPECT_EQ(true, repertory::utils::encryption::decrypt_file_name(
                          cfg.get_encrypt_config().encryption_token, part));
    }
    path = repertory::utils::string::join(parts, '/');
  }
};
} // namespace

namespace repertory {
static void can_create_and_remove_directory(i_provider &provider) {
  if (provider.is_read_only()) {
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
  if (provider.is_read_only()) {
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
  if (provider.is_read_only()) {
    return;
  }

  create_file(provider, "/pt01");

  api_meta_map meta{};
  EXPECT_EQ(api_error::item_exists, provider.create_directory("/pt01", meta));

  EXPECT_EQ(api_error::success, provider.remove_file("/pt01"));
}

static void create_directory_clone_source_meta(i_provider &provider) {
  if (provider.is_read_only()) {
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
  for (const auto &item : meta_orig) {
    if (item.first == META_KEY) {
      if (item.second.empty() && meta_clone[item.first].empty()) {
        continue;
      }
      EXPECT_STRNE(item.second.c_str(), meta_clone[item.first].c_str());
      continue;
    }
    EXPECT_STREQ(item.second.c_str(), meta_clone[item.first].c_str());
  }

  EXPECT_EQ(api_error::success, provider.remove_directory("/clone"));
  EXPECT_EQ(api_error::success, provider.remove_directory("/clone2"));
}

static void create_directory_clone_source_meta_fails_if_already_exists(
    i_provider &provider) {
  if (provider.is_read_only()) {
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
  if (provider.is_read_only()) {
    return;
  }

  EXPECT_EQ(api_error::directory_not_found,
            provider.create_directory_clone_source_meta("/clone", "/clone2"));
}

static void create_directory_clone_source_meta_fails_if_file_already_exists(
    i_provider &provider) {
  if (provider.is_read_only()) {
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
  if (provider.is_read_only()) {
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
  if (provider.is_read_only()) {
    return;
  }

  create_file(provider, "/pt01.txt");

  api_meta_map meta{};
  EXPECT_EQ(api_error::item_exists, provider.create_file("/pt01.txt", meta));

  EXPECT_EQ(api_error::success, provider.remove_file("/pt01.txt"));
}

static void
create_file_fails_if_directory_already_exists(i_provider &provider) {
  if (provider.is_read_only()) {
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
        utils::path::combine("./test_date/encrypt", {"test.txt"});

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
  EXPECT_EQ(api_error::success, provider.remove_file("/pt01.txt"));
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
        utils::path::combine(test::get_test_input_dir(), {"encrypt", "sub10"});

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
    for (auto &dir_item : list_decrypted) {
      decrypt_parts(cfg, dir_item.api_parent);
      decrypt_parts(cfg, dir_item.api_path);
    }

    auto dir =
        std::ranges::find_if(list_decrypted, [](auto &&dir_item) -> bool {
          return dir_item.directory;
        });
    EXPECT_LT(dir, list_decrypted.end());
    EXPECT_STREQ("/sub10", dir->api_path.c_str());
    EXPECT_STREQ("/", dir->api_parent.c_str());
    EXPECT_EQ(std::size_t(0U), dir->size);

    auto file =
        std::ranges::find_if(list_decrypted, [](auto &&dir_item) -> bool {
          return not dir_item.directory;
        });
    EXPECT_LT(file, list_decrypted.end());
    EXPECT_STREQ("/test.txt", file->api_path.c_str());
    EXPECT_STREQ("/", file->api_parent.c_str());
#if defined(_WIN32)
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
    for (auto &dir_item : list_decrypted2) {
      decrypt_parts(cfg, dir_item.api_parent);
      decrypt_parts(cfg, dir_item.api_path);
    }

    auto file2 =
        std::ranges::find_if(list_decrypted2, [](auto &&dir_item) -> bool {
          return not dir_item.directory;
        });
    EXPECT_LT(file2, list_decrypted2.end());
    EXPECT_STREQ("/sub10/moose.txt", file2->api_path.c_str());
    EXPECT_STREQ("/sub10", file2->api_parent.c_str());
#if defined(_WIN32)
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
#if defined(_WIN32)
    EXPECT_EQ(std::size_t(47U), file.file_size);
#else
    EXPECT_EQ(std::size_t(46U), file.file_size);
#endif
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
  std::string marker;
  EXPECT_EQ(api_error::success, provider.get_file_list(list, marker));
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
      utils::collection::remove_element(expected_parents, file.api_parent);
      utils::collection::remove_element(expected_paths, file.api_path);
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
  get_directory_items(cfg, provider);
  get_directory_items_fails_if_directory_not_found(provider);
  get_directory_items_fails_if_item_is_file(cfg, provider);

  get_directory_item_count(cfg, provider);

  get_file(cfg, provider);
  get_file_fails_if_file_not_found(provider);
  get_file_fails_if_item_is_directory(cfg, provider);

  // TODO need to test read when file size changes for encrypt provider
  /* get_file_list(provider);
  get_file_size(provider);
  get_filesystem_item(provider);
  get_filesystem_item_and_file(provider);
  get_filesystem_item_from_source_path(provider);
  get_item_meta(provider);
  get_item_meta2(provider);
  get_pinned_files(provider);
  get_total_drive_space(provider);
  get_total_item_count(provider);
  get_used_drive_space(provider);
  is_directory(provider);
  is_file(provider);
  is_file_writeable(provider);
  read_file_bytes(provider);
  remove_directory(provider);
  remove_file(provider);
  remove_item_meta(provider);
  rename_file(provider);
  set_item_meta(provider);
  set_item_meta2(provider);
  upload_file(provider); */
}

TEST(providers, encrypt_provider) {
  const auto config_path =
      utils::path::combine(test::get_test_output_dir(), {"encrypt_provider"});

  console_consumer consumer{};
  event_system::instance().start();

  {
    app_config cfg(provider_type::encrypt, config_path);

    const auto encrypt_path =
        utils::path::combine(test::get_test_input_dir(), {"encrypt"});

    EXPECT_STREQ(
        encrypt_path.c_str(),
        cfg.set_value_by_name("EncryptConfig.Path", encrypt_path).c_str());
    EXPECT_STREQ(
        "test_token",
        cfg.set_value_by_name("EncryptConfig.EncryptionToken", "test_token")
            .c_str());

    encrypt_provider provider{cfg};
    file_manager mgr(cfg, provider);
    mgr.start();

    EXPECT_TRUE(provider.start(
        [&provider](bool directory, api_file &file) -> api_error {
          return provider_meta_handler(provider, directory, file);
        },
        &mgr));
    EXPECT_EQ(provider_type::encrypt, provider.get_provider_type());
    EXPECT_TRUE(provider.is_read_only());
    EXPECT_TRUE(provider.is_online());
    EXPECT_FALSE(provider.is_rename_supported());

    run_tests(cfg, provider);

    provider.stop();
    mgr.stop();
  }

  event_system::instance().stop();
}

TEST(providers, s3_provider) {
  const auto config_path =
      utils::path::combine(test::get_test_output_dir(), {"s3_provider"});

  console_consumer consumer{};
  event_system::instance().start();

  {
    app_config cfg(provider_type::s3, config_path);
    {
      app_config src_cfg(
          provider_type::s3,
          utils::path::combine(test::get_test_config_dir(), {"storj"}));
      cfg.set_s3_config(src_cfg.get_s3_config());
    }

    curl_comm comm{cfg.get_s3_config()};
    s3_provider provider{cfg, comm};
    file_manager mgr(cfg, provider);
    mgr.start();

    EXPECT_TRUE(provider.start(
        [&provider](bool directory, api_file &file) -> api_error {
          return provider_meta_handler(provider, directory, file);
        },
        &mgr));
    EXPECT_EQ(provider_type::s3, provider.get_provider_type());
    EXPECT_FALSE(provider.is_read_only());
    EXPECT_TRUE(provider.is_online());
    EXPECT_FALSE(provider.is_rename_supported());

    run_tests(cfg, provider);

    provider.stop();
    mgr.stop();
  }

  event_system::instance().stop();
}

TEST(providers, sia_provider) {
  const auto config_path =
      utils::path::combine(test::get_test_output_dir(), {"sia_provider"});

  console_consumer consumer{};
  event_system::instance().start();

  {
    app_config cfg(provider_type::sia, config_path);
    {
      app_config src_cfg(
          provider_type::sia,
          utils::path::combine(test::get_test_config_dir(), {"sia"}));
      cfg.set_host_config(src_cfg.get_host_config());
    }

    curl_comm comm{cfg.get_host_config()};
    sia_provider provider{cfg, comm};
    file_manager mgr(cfg, provider);
    mgr.start();

    EXPECT_TRUE(provider.start(
        [&provider](bool directory, api_file &file) -> api_error {
          return provider_meta_handler(provider, directory, file);
        },
        &mgr));
    EXPECT_EQ(provider_type::sia, provider.get_provider_type());
    EXPECT_FALSE(provider.is_read_only());
    EXPECT_TRUE(provider.is_online());
    EXPECT_TRUE(provider.is_rename_supported());

    run_tests(cfg, provider);

    provider.stop();
    mgr.stop();
  }

  event_system::instance().stop();
}
} // namespace repertory

#endif // 0
