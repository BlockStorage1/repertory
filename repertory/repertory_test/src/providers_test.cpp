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
#include "test_common.hpp"

#include "fixtures/providers_fixture.hpp"
#include "utils/collection.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"

namespace repertory {
TYPED_TEST_SUITE(providers_test, provider_types);

TYPED_TEST(providers_test, get_file_list) {
  api_file_list list{};
  std::string marker;
  EXPECT_EQ(api_error::success, this->provider->get_file_list(list, marker));
  if (this->provider->get_provider_type() == provider_type::encrypt) {
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
      this->decrypt_parts(file.api_parent);
      this->decrypt_parts(file.api_path);
      utils::collection::remove_element(expected_parents, file.api_parent);
      utils::collection::remove_element(expected_paths, file.api_path);
    }
    EXPECT_TRUE(expected_parents.empty());
    EXPECT_TRUE(expected_paths.empty());
  }
}

TYPED_TEST(providers_test, get_and_set_item_meta_with_upload_file) {
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    std::string api_path{};
    EXPECT_EQ(api_error::success,
              this->provider->get_api_path_from_source(
                  utils::path::combine(this->config->get_encrypt_config().path,
                                       {"test.txt"}),
                  api_path));

    std::string val{};
    EXPECT_EQ(api_error::success,
              this->provider->get_item_meta(api_path, META_SOURCE, val));
    EXPECT_FALSE(val.empty());
    EXPECT_TRUE(utils::file::file{val}.exists());

    val.clear();
    EXPECT_EQ(api_error::success,
              this->provider->get_item_meta(api_path, META_DIRECTORY, val));
    EXPECT_FALSE(utils::string::to_bool(val));
    return;
  }

  auto &file = test::create_random_file(128U);
  auto api_path =
      fmt::format("/{}", utils::path::strip_to_file_name(file.get_path()));
  this->create_file(api_path);

  stop_type stop_requested{false};
  ASSERT_EQ(api_error::success, this->provider->upload_file(
                                    api_path, file.get_path(), stop_requested));

  auto size_str = std::to_string(*file.size());
  EXPECT_EQ(api_error::success,
            this->provider->set_item_meta(api_path, META_SIZE, size_str));
  EXPECT_EQ(api_error::success, this->provider->set_item_meta(
                                    api_path, META_SOURCE, file.get_path()));

  std::string val{};
  EXPECT_EQ(api_error::success,
            this->provider->get_item_meta(api_path, META_SIZE, val));
  EXPECT_STREQ(size_str.c_str(), val.c_str());

  val.clear();
  EXPECT_EQ(api_error::success,
            this->provider->get_item_meta(api_path, META_SOURCE, val));
  EXPECT_STREQ(file.get_path().c_str(), val.c_str());

  EXPECT_EQ(api_error::success, this->provider->remove_file(api_path));
}

TYPED_TEST(providers_test, can_create_and_remove_directory) {
  if (this->provider->is_read_only()) {
    api_meta_map meta{};
    EXPECT_EQ(api_error::not_implemented,
              this->provider->create_directory("/moose", meta));

    EXPECT_EQ(api_error::not_implemented,
              this->provider->remove_directory("/moose"));
    return;
  }

  this->create_directory("/pt01");
  EXPECT_EQ(api_error::success, this->provider->remove_directory("/pt01"));

  bool exists{};
  EXPECT_EQ(api_error::success, this->provider->is_directory("/pt01", exists));
  EXPECT_FALSE(exists);
}

TYPED_TEST(providers_test, get_and_set_item_meta2_with_upload_file) {
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    std::string api_path{};
    EXPECT_EQ(api_error::success,
              this->provider->get_api_path_from_source(
                  utils::path::combine(this->config->get_encrypt_config().path,
                                       {"test.txt"}),
                  api_path));

    api_meta_map meta{};
    EXPECT_EQ(api_error::success,
              this->provider->get_item_meta(api_path, meta));
    EXPECT_TRUE(meta.contains(META_SOURCE));
    EXPECT_TRUE(meta.contains(META_DIRECTORY));
    EXPECT_FALSE(utils::string::to_bool(meta[META_DIRECTORY]));
    return;
  }

  auto &file = test::create_random_file(64U);
  auto api_path =
      fmt::format("/{}", utils::path::strip_to_file_name(file.get_path()));
  this->create_file(api_path);

  stop_type stop_requested{false};
  ASSERT_EQ(api_error::success, this->provider->upload_file(
                                    api_path, file.get_path(), stop_requested));

  api_meta_map to_set{
      {META_SIZE, std::to_string(*file.size())},
      {META_SOURCE, file.get_path()},
  };
  EXPECT_EQ(api_error::success,
            this->provider->set_item_meta(api_path, to_set));

  api_meta_map meta{};
  EXPECT_EQ(api_error::success, this->provider->get_item_meta(api_path, meta));
  EXPECT_STREQ(std::to_string(*file.size()).c_str(), meta[META_SIZE].c_str());
  EXPECT_STREQ(file.get_path().c_str(), meta[META_SOURCE].c_str());
  EXPECT_FALSE(utils::string::to_bool(meta[META_DIRECTORY]));

  EXPECT_EQ(api_error::success, this->provider->remove_file(api_path));
}

TYPED_TEST(providers_test, get_item_meta_fails_if_path_not_found) {
  std::string val{};
  EXPECT_EQ(
      api_error::item_not_found,
      this->provider->get_item_meta("/cow/moose/doge/chicken", META_SIZE, val));
  EXPECT_TRUE(val.empty());
}

TYPED_TEST(providers_test, get_item_meta2_fails_if_path_not_found) {
  api_meta_map meta{};
  EXPECT_EQ(api_error::item_not_found,
            this->provider->get_item_meta("/cow/moose/doge/chicken", meta));
  EXPECT_TRUE(meta.empty());
}

TYPED_TEST(providers_test, is_file_fails_if_not_found) {
  bool exists{};
  auto res = this->provider->is_file("/cow/moose/doge/chicken", exists);

  EXPECT_EQ(api_error::success, res);
  EXPECT_FALSE(exists);
}

TYPED_TEST(providers_test, is_directory_fails_if_not_found) {
  bool exists{};
  auto res = this->provider->is_directory("/cow/moose/doge/chicken", exists);

  EXPECT_EQ(api_error::success, res);
  EXPECT_FALSE(exists);
}

TYPED_TEST(providers_test, can_create_and_remove_file) {
  if (this->provider->is_read_only()) {
    api_meta_map meta{};
    EXPECT_EQ(api_error::not_implemented,
              this->provider->create_file("/moose.txt", meta));
    return;
  }

  this->create_file("/pt01.txt");

  bool exists{};
  EXPECT_EQ(api_error::success, this->provider->is_file("/pt01.txt", exists));
  EXPECT_TRUE(exists);

  EXPECT_EQ(api_error::success, this->provider->remove_file("/pt01.txt"));

  EXPECT_EQ(api_error::success, this->provider->is_file("/pt01.txt", exists));
  EXPECT_FALSE(exists);
}

TYPED_TEST(providers_test, create_directory_fails_if_already_exists) {
  if (this->provider->is_read_only()) {
    return;
  }

  this->create_directory("/pt01");

  api_meta_map meta{};
  EXPECT_EQ(api_error::directory_exists,
            this->provider->create_directory("/pt01", meta));
  EXPECT_EQ(api_error::success, this->provider->remove_directory("/pt01"));
}

TYPED_TEST(providers_test, create_directory_fails_if_file_already_exists) {
  if (this->provider->is_read_only()) {
    return;
  }

  this->create_file("/pt01");

  api_meta_map meta{};
  EXPECT_EQ(api_error::item_exists,
            this->provider->create_directory("/pt01", meta));

  EXPECT_EQ(api_error::success, this->provider->remove_file("/pt01"));
}

TYPED_TEST(providers_test, create_directory_clone_source_meta) {
  if (this->provider->is_read_only()) {
    EXPECT_EQ(
        api_error::not_implemented,
        this->provider->create_directory_clone_source_meta("/moose", "/moose"));
    return;
  }
  this->create_directory("/clone");

  api_meta_map meta_orig{};
  EXPECT_EQ(api_error::success,
            this->provider->get_item_meta("/clone", meta_orig));

  EXPECT_EQ(
      api_error::success,
      this->provider->create_directory_clone_source_meta("/clone", "/clone2"));

  api_meta_map meta_clone{};
  EXPECT_EQ(api_error::success,
            this->provider->get_item_meta("/clone2", meta_clone));

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

  EXPECT_EQ(api_error::success, this->provider->remove_directory("/clone"));
  EXPECT_EQ(api_error::success, this->provider->remove_directory("/clone2"));
}

TYPED_TEST(providers_test,
           create_directory_clone_source_meta_fails_if_already_exists) {
  if (this->provider->is_read_only()) {
    return;
  }
  this->create_directory("/clone");
  this->create_directory("/clone2");

  EXPECT_EQ(
      api_error::directory_exists,
      this->provider->create_directory_clone_source_meta("/clone", "/clone2"));

  EXPECT_EQ(api_error::success, this->provider->remove_directory("/clone"));
  EXPECT_EQ(api_error::success, this->provider->remove_directory("/clone2"));
}

TYPED_TEST(providers_test,
           create_directory_clone_source_meta_fails_if_directory_not_found) {
  if (this->provider->is_read_only()) {
    return;
  }

  EXPECT_EQ(
      api_error::directory_not_found,
      this->provider->create_directory_clone_source_meta("/clone", "/clone2"));
}

TYPED_TEST(providers_test,
           create_directory_clone_source_meta_fails_if_file_already_exists) {
  if (this->provider->is_read_only()) {
    return;
  }

  this->create_directory("/clone");
  this->create_file("/clone2");

  EXPECT_EQ(
      api_error::item_exists,
      this->provider->create_directory_clone_source_meta("/clone", "/clone2"));

  EXPECT_EQ(api_error::success, this->provider->remove_directory("/clone"));
  EXPECT_EQ(api_error::success, this->provider->remove_file("/clone2"));
}

TYPED_TEST(providers_test, create_file_fails_if_already_exists) {
  if (this->provider->is_read_only()) {
    return;
  }

  this->create_file("/pt01.txt");

  api_meta_map meta{};
  EXPECT_EQ(api_error::item_exists,
            this->provider->create_file("/pt01.txt", meta));

  EXPECT_EQ(api_error::success, this->provider->remove_file("/pt01.txt"));
}

TYPED_TEST(providers_test, create_file_fails_if_directory_already_exists) {
  if (this->provider->is_read_only()) {
    return;
  }

  this->create_directory("/pt01");

  api_meta_map meta{};
  EXPECT_EQ(api_error::directory_exists,
            this->provider->create_file("/pt01", meta));

  EXPECT_EQ(api_error::success, this->provider->remove_directory("/pt01"));
}

TYPED_TEST(providers_test, get_api_path_from_source) {
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    auto source_path =
        utils::path::combine("./test_input/encrypt", {"test.txt"});

    std::string api_path{};
    EXPECT_EQ(api_error::success,
              this->provider->get_api_path_from_source(source_path, api_path));

    std::string file_name{api_path.substr(1U)};
    this->decrypt_parts(file_name);
    EXPECT_STREQ("test.txt", file_name.c_str());
    return;
  }

  this->create_file("/pt01.txt");

  filesystem_item fsi{};
  EXPECT_EQ(api_error::success,
            this->provider->get_filesystem_item("/pt01.txt", false, fsi));

  std::string api_path{};
  EXPECT_EQ(api_error::success, this->provider->get_api_path_from_source(
                                    fsi.source_path, api_path));

  EXPECT_STREQ("/pt01.txt", api_path.c_str());
  EXPECT_EQ(api_error::success, this->provider->remove_file("/pt01.txt"));
}

TYPED_TEST(providers_test, get_api_path_from_source_fails_if_file_not_found) {
  std::string source_path{};
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    source_path = utils::path::combine(this->config->get_encrypt_config().path,
                                       {"test_not_found.txt"});
  } else {
    source_path = utils::path::combine("./", {"test_not_found.txt"});
  }

  std::string api_path{};
  EXPECT_EQ(api_error::item_not_found,
            this->provider->get_api_path_from_source(source_path, api_path));

  EXPECT_TRUE(api_path.empty());
}

TYPED_TEST(providers_test, get_directory_items) {
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    directory_item_list list{};
    EXPECT_EQ(api_error::success,
              this->provider->get_directory_items("/", list));
    this->check_forced_dirs(list);

    EXPECT_EQ(std::size_t(4U), list.size());

    directory_item_list list_decrypted{list.begin() + 2U, list.end()};
    for (auto &dir_item : list_decrypted) {
      this->decrypt_parts(dir_item.api_parent);
      this->decrypt_parts(dir_item.api_path);
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
    EXPECT_EQ(std::size_t(83U), file->size);
#else
    EXPECT_EQ(std::size_t(82U), file->size);
#endif

    auto source_path = utils::path::combine(
        this->config->get_encrypt_config().path, {"sub10"});
    std::string api_path{};
    EXPECT_EQ(api_error::success,
              this->provider->get_api_path_from_source(source_path, api_path));

    list.clear();
    EXPECT_EQ(api_error::success,
              this->provider->get_directory_items(api_path, list));
    this->check_forced_dirs(list);
    EXPECT_EQ(std::size_t(3U), list.size());

    directory_item_list list_decrypted2{list.begin() + 2U, list.end()};
    for (auto &dir_item : list_decrypted2) {
      this->decrypt_parts(dir_item.api_parent);
      this->decrypt_parts(dir_item.api_path);
    }

    auto file2 =
        std::ranges::find_if(list_decrypted2, [](auto &&dir_item) -> bool {
          return not dir_item.directory;
        });
    EXPECT_LT(file2, list_decrypted2.end());
    EXPECT_STREQ("/sub10/moose.txt", file2->api_path.c_str());
    EXPECT_STREQ("/sub10", file2->api_parent.c_str());
#if defined(_WIN32)
    EXPECT_EQ(std::size_t(82U), file2->size);
#else
    EXPECT_EQ(std::size_t(81U), file2->size);
#endif
    return;
  }

  this->create_file("/pt01.txt");
  this->create_file("/pt02.txt");
  this->create_directory("/dir01");
  this->create_directory("/dir02");

  directory_item_list list{};
  EXPECT_EQ(api_error::success, this->provider->get_directory_items("/", list));
  this->check_forced_dirs(list);
  EXPECT_GE(list.size(), std::size_t(6U));

  auto iter = std::ranges::find_if(
      list, [](auto &&item) -> bool { return item.api_path == "/pt01.txt"; });
  EXPECT_NE(iter, list.end());
  EXPECT_STREQ("/", (*iter).api_parent.c_str());
  EXPECT_FALSE((*iter).directory);
  EXPECT_EQ(std::uint64_t{0U}, (*iter).size);

  iter = std::ranges::find_if(
      list, [](auto &&item) -> bool { return item.api_path == "/pt02.txt"; });
  EXPECT_NE(iter, list.end());
  EXPECT_STREQ("/", (*iter).api_parent.c_str());
  EXPECT_FALSE((*iter).directory);
  EXPECT_EQ(std::uint64_t{0U}, (*iter).size);

  iter = std::ranges::find_if(
      list, [](auto &&item) -> bool { return item.api_path == "/dir01"; });
  EXPECT_NE(iter, list.end());
  EXPECT_STREQ("/", (*iter).api_parent.c_str());
  EXPECT_TRUE((*iter).directory);
  EXPECT_EQ(std::uint64_t{0U}, (*iter).size);

  iter = std::ranges::find_if(
      list, [](auto &&item) -> bool { return item.api_path == "/dir02"; });
  EXPECT_NE(iter, list.end());
  EXPECT_STREQ("/", (*iter).api_parent.c_str());
  EXPECT_TRUE((*iter).directory);
  EXPECT_EQ(std::uint64_t{0U}, (*iter).size);

  EXPECT_EQ(api_error::success, this->provider->remove_file("/pt01.txt"));
  EXPECT_EQ(api_error::success, this->provider->remove_file("/pt02.txt"));
  EXPECT_EQ(api_error::success, this->provider->remove_directory("/dir01"));
  EXPECT_EQ(api_error::success, this->provider->remove_directory("/dir02"));
}
TYPED_TEST(providers_test, get_directory_items_fails_if_directory_not_found) {
  directory_item_list list{};
  EXPECT_EQ(api_error::directory_not_found,
            this->provider->get_directory_items("/not_found", list));
  EXPECT_TRUE(list.empty());
}

TYPED_TEST(providers_test, get_directory_items_fails_if_item_is_file) {
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    auto source_path = utils::path::combine(
        this->config->get_encrypt_config().path, {"test.txt"});

    std::string api_path{};
    EXPECT_EQ(api_error::success,
              this->provider->get_api_path_from_source(source_path, api_path));

    directory_item_list list{};
    EXPECT_EQ(api_error::item_exists,
              this->provider->get_directory_items(api_path, list));
    EXPECT_TRUE(list.empty());
    return;
  }

  this->create_file("/pt01.txt");

  directory_item_list list{};
  EXPECT_EQ(api_error::item_exists,
            this->provider->get_directory_items("/pt01.txt", list));

  EXPECT_EQ(api_error::success, this->provider->remove_file("/pt01.txt"));
}

TYPED_TEST(providers_test, get_directory_item_count) {
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    EXPECT_EQ(std::size_t(2U), this->provider->get_directory_item_count("/"));
    EXPECT_EQ(std::size_t(0U),
              this->provider->get_directory_item_count("/not_found"));

    auto source_path =
        utils::path::combine(test::get_test_input_dir(), {"encrypt", "sub10"});

    std::string api_path{};
    EXPECT_EQ(api_error::success,
              this->provider->get_api_path_from_source(source_path, api_path));
    EXPECT_EQ(std::size_t(1U),
              this->provider->get_directory_item_count(api_path));
    return;
  }

  this->create_file("/pt01.txt");
  this->create_file("/pt02.txt");
  this->create_directory("/dir01");
  this->create_directory("/dir02");

  directory_item_list list{};
  EXPECT_EQ(api_error::success, this->provider->get_directory_items("/", list));
  this->check_forced_dirs(list);
  EXPECT_GE(list.size(), std::size_t(6U));

  EXPECT_EQ(api_error::success, this->provider->remove_file("/pt01.txt"));
  EXPECT_EQ(api_error::success, this->provider->remove_file("/pt02.txt"));
  EXPECT_EQ(api_error::success, this->provider->remove_directory("/dir01"));
  EXPECT_EQ(api_error::success, this->provider->remove_directory("/dir02"));
}

TYPED_TEST(providers_test, get_file) {
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    auto source_path = utils::path::combine(
        this->config->get_encrypt_config().path, {"test.txt"});

    std::string api_path{};
    EXPECT_EQ(api_error::success,
              this->provider->get_api_path_from_source(source_path, api_path));

    api_file file{};
    EXPECT_EQ(api_error::success, this->provider->get_file(api_path, file));
    this->decrypt_parts(file.api_path);
    this->decrypt_parts(file.api_parent);

    EXPECT_STREQ("/test.txt", file.api_path.c_str());
    EXPECT_STREQ("/", file.api_parent.c_str());
#if defined(_WIN32)
    EXPECT_EQ(std::size_t(83U), file.file_size);
#else
    EXPECT_EQ(std::size_t(82U), file.file_size);
#endif
    EXPECT_STREQ(source_path.c_str(), file.source_path.c_str());
    return;
  }

  this->create_file("/pt01.txt");

  api_file file{};
  EXPECT_EQ(api_error::success, this->provider->get_file("/pt01.txt", file));

  EXPECT_STREQ("/pt01.txt", file.api_path.c_str());
  EXPECT_STREQ("/", file.api_parent.c_str());
  EXPECT_LT(utils::time::get_time_now() - (utils::time::NANOS_PER_SECOND * 5U),
            file.accessed_date);
  EXPECT_LT(utils::time::get_time_now() - (utils::time::NANOS_PER_SECOND * 5U),
            file.changed_date);
  EXPECT_LT(utils::time::get_time_now() - (utils::time::NANOS_PER_SECOND * 5U),
            file.modified_date);

  EXPECT_EQ(api_error::success, this->provider->remove_file("/pt01.txt"));
}
TYPED_TEST(providers_test, get_file_fails_if_file_not_found) {
  api_file file{};
  EXPECT_EQ(api_error::item_not_found,
            this->provider->get_file("/not_found", file));
}

TYPED_TEST(providers_test, get_file_fails_if_item_is_directory) {
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    auto source_path = utils::path::combine(
        this->config->get_encrypt_config().path, {"sub10"});

    std::string api_path{};
    EXPECT_EQ(api_error::success,
              this->provider->get_api_path_from_source(source_path, api_path));

    api_file file{};
    EXPECT_EQ(api_error::directory_exists,
              this->provider->get_file(api_path, file));
    return;
  }

  this->create_directory("/dir01");

  api_file file{};
  EXPECT_EQ(api_error::directory_exists,
            this->provider->get_file("/dir01", file));

  EXPECT_EQ(api_error::success, this->provider->remove_directory("/dir01"));
}
TYPED_TEST(providers_test, get_file_size) {
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    auto source_path = utils::path::combine(
        this->config->get_encrypt_config().path, {"test.txt"});
    auto src_size = utils::file::file{source_path}.size();
    EXPECT_TRUE(src_size.has_value());

    std::string api_path{};
    EXPECT_EQ(api_error::success,
              this->provider->get_api_path_from_source(source_path, api_path));

    std::uint64_t size{};
    auto res = this->provider->get_file_size(api_path, size);
    EXPECT_EQ(api_error::success, res);
    EXPECT_EQ(utils::encryption::encrypting_reader::calculate_encrypted_size(
                  src_size.value(), true),
              size);
    return;
  }

  auto &file = test::create_random_file(128U);
  auto api_path =
      fmt::format("/{}", utils::path::strip_to_file_name(file.get_path()));
  this->create_file(api_path);

  stop_type stop_requested{false};
  auto res =
      this->provider->upload_file(api_path, file.get_path(), stop_requested);
  ASSERT_EQ(api_error::success, res);

  std::uint64_t size{};
  res = this->provider->get_file_size(api_path, size);
  EXPECT_EQ(api_error::success, res);
  EXPECT_EQ(*file.size(), size);

  res = this->provider->remove_file(api_path);
  EXPECT_EQ(api_error::success, res);
}

TYPED_TEST(providers_test, get_file_size_fails_if_path_not_found) {
  std::uint64_t size{};
  auto res = this->provider->get_file_size("/cow/moose/doge/chicken", size);

  EXPECT_EQ(api_error::item_not_found, res);
  EXPECT_EQ(0U, size);
}

TYPED_TEST(providers_test, get_filesystem_item) {
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    std::string api_path{};
    EXPECT_EQ(api_error::success,
              this->provider->get_api_path_from_source(
                  utils::path::combine(this->config->get_encrypt_config().path,
                                       {"test.txt"}),
                  api_path));

    filesystem_item fsi{};
    auto res = this->provider->get_filesystem_item(api_path, false, fsi);
    EXPECT_EQ(api_error::success, res);

    EXPECT_FALSE(fsi.directory);
    EXPECT_EQ(api_path, fsi.api_path);

    std::uint64_t size{};
    res = this->provider->get_file_size(api_path, size);
    ASSERT_EQ(api_error::success, res);
    EXPECT_EQ(size, fsi.size);

    return;
  }

  auto &file = test::create_random_file(128U);
  auto api_path =
      fmt::format("/{}", utils::path::strip_to_file_name(file.get_path()));
  this->create_file(api_path);

  stop_type stop_requested{false};
  auto res =
      this->provider->upload_file(api_path, file.get_path(), stop_requested);
  ASSERT_EQ(api_error::success, res);
  EXPECT_EQ(api_error::success,
            this->provider->set_item_meta(api_path, META_SIZE,
                                          std::to_string(*file.size())));

  filesystem_item fsi{};
  res = this->provider->get_filesystem_item(api_path, false, fsi);
  EXPECT_EQ(api_error::success, res);
  EXPECT_EQ(api_path, fsi.api_path);
  EXPECT_FALSE(fsi.directory);
  EXPECT_EQ(*file.size(), fsi.size);

  res = this->provider->remove_file(api_path);
  EXPECT_EQ(api_error::success, res);
}

TYPED_TEST(providers_test, get_filesystem_item_root_is_directory) {
  filesystem_item fsi{};
  auto res = this->provider->get_filesystem_item("/", true, fsi);

  EXPECT_EQ(api_error::success, res);
  EXPECT_TRUE(fsi.directory);
  EXPECT_EQ("/", fsi.api_path);
}

TYPED_TEST(providers_test, get_filesystem_item_fails_if_file_is_not_found) {
  filesystem_item fsi{};
  auto res = this->provider->get_filesystem_item("/cow/moose/doge/chicken",
                                                 false, fsi);
  EXPECT_EQ(api_error::item_not_found, res);
}

TYPED_TEST(providers_test,
           get_filesystem_item_fails_if_directory_is_not_found) {
  filesystem_item fsi{};
  auto res =
      this->provider->get_filesystem_item("/cow/moose/doge/chicken", true, fsi);
  EXPECT_EQ(api_error::directory_not_found, res);
}

TYPED_TEST(providers_test, get_filesystem_item_from_source_path) {
  std::string api_path;
  std::string source_path;
  std::uint64_t size{};
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    source_path = utils::path::combine(this->config->get_encrypt_config().path,
                                       {"test.txt"});
    auto src_size = utils::file::file{source_path}.size();
    EXPECT_TRUE(src_size.has_value());

    size = utils::encryption::encrypting_reader::calculate_encrypted_size(
        src_size.value(), true);
  } else {
    size = 128U;
    auto &file = test::create_random_file(size);
    api_path =
        fmt::format("/{}", utils::path::strip_to_file_name(file.get_path()));
    source_path = file.get_path();
    this->create_file(api_path);

    EXPECT_EQ(api_error::success,
              this->provider->set_item_meta(
                  api_path, {
                                {META_SIZE, std::to_string(size)},
                                {META_SOURCE, source_path},
                            }));

    stop_type stop_requested{false};
    auto res =
        this->provider->upload_file(api_path, source_path, stop_requested);
    ASSERT_EQ(api_error::success, res);
  }

  filesystem_item fsi{};
  auto res =
      this->provider->get_filesystem_item_from_source_path(source_path, fsi);
  EXPECT_EQ(api_error::success, res);
  EXPECT_FALSE(fsi.directory);
  EXPECT_EQ(size, fsi.size);

  if (this->provider->get_provider_type() != provider_type::encrypt) {
    EXPECT_EQ(api_error::success, this->provider->remove_file(api_path));
  }
}
TYPED_TEST(providers_test,
           get_filesystem_item_from_source_path_fails_if_file_is_not_found) {
  filesystem_item fsi{};
  auto res = this->provider->get_filesystem_item_from_source_path(
      "/cow/moose/doge/chicken", fsi);
  EXPECT_EQ(api_error::item_not_found, res);
}

TYPED_TEST(providers_test, remove_file_fails_if_file_not_found) {
  auto res = this->provider->remove_file("/cow/moose/doge/chicken");
  if (this->provider->is_read_only()) {
    EXPECT_EQ(api_error::not_implemented, res);
    return;
  }
  EXPECT_EQ(api_error::item_not_found, res);
}

TYPED_TEST(providers_test, remove_file_fails_if_item_is_directory) {
  if (this->provider->is_read_only()) {
    EXPECT_EQ(api_error::not_implemented,
              this->provider->remove_file("/dir01"));
    return;
  }

  this->create_directory("/dir01");
  EXPECT_EQ(api_error::directory_exists, this->provider->remove_file("/dir01"));
  EXPECT_EQ(api_error::success, this->provider->remove_directory("/dir01"));
}

TYPED_TEST(providers_test, remove_directory_fails_if_item_is_file) {
  if (this->provider->is_read_only()) {
    EXPECT_EQ(api_error::not_implemented,
              this->provider->remove_directory("/pt01.txt"));
    return;
  }

  this->create_file("/pt01.txt");
  EXPECT_EQ(api_error::item_not_found,
            this->provider->remove_directory("/pt01.txt"));
  EXPECT_EQ(api_error::success, this->provider->remove_file("/pt01.txt"));
}

TYPED_TEST(providers_test, remove_directory_fails_if_directory_not_found) {
  auto res = this->provider->remove_directory("/cow/moose/doge/chicken");
  if (this->provider->is_read_only()) {
    EXPECT_EQ(api_error::not_implemented, res);
    return;
  }
  EXPECT_EQ(api_error::item_not_found, res);
}

TYPED_TEST(providers_test, get_pinned_files) {
  if (this->provider->is_read_only()) {
    auto pinned = this->provider->get_pinned_files();
    EXPECT_TRUE(pinned.empty());
    return;
  }

  this->create_file("/pin01.txt");
  this->create_file("/pin02.txt");
  this->create_file("/nopin01.txt");

  EXPECT_EQ(api_error::success,
            this->provider->set_item_meta("/pin01.txt", META_PINNED, "true"));
  EXPECT_EQ(api_error::success,
            this->provider->set_item_meta("/pin02.txt", META_PINNED, "true"));
  EXPECT_EQ(api_error::success, this->provider->set_item_meta(
                                    "/nopin01.txt", META_PINNED, "false"));

  auto pinned = this->provider->get_pinned_files();
  EXPECT_EQ(std::size_t(2U), pinned.size());

  EXPECT_TRUE(this->pinned_includes_api_path(pinned, "/pin01.txt"));
  EXPECT_TRUE(this->pinned_includes_api_path(pinned, "/pin02.txt"));
  EXPECT_FALSE(this->pinned_includes_api_path(pinned, "/nopin01.txt"));

  EXPECT_EQ(api_error::success, this->provider->remove_file("/pin01.txt"));
  EXPECT_EQ(api_error::success, this->provider->remove_file("/pin02.txt"));
  EXPECT_EQ(api_error::success, this->provider->remove_file("/nopin01.txt"));
}

TYPED_TEST(providers_test, remove_pin_updates_pinned_files) {
  if (this->provider->is_read_only()) {
    auto pinned = this->provider->get_pinned_files();
    EXPECT_TRUE(pinned.empty());
    return;
  }

  this->create_file("/pin01.txt");
  this->create_file("/pin02.txt");
  EXPECT_EQ(api_error::success,
            this->provider->set_item_meta("/pin01.txt", META_PINNED, "true"));
  EXPECT_EQ(api_error::success,
            this->provider->set_item_meta("/pin02.txt", META_PINNED, "true"));

  auto pinned = this->provider->get_pinned_files();
  EXPECT_EQ(std::size_t(2U), pinned.size());

  EXPECT_EQ(api_error::success,
            this->provider->set_item_meta("/pin02.txt", META_PINNED, "false"));
  pinned = this->provider->get_pinned_files();
  EXPECT_EQ(std::size_t(1U), pinned.size());
  EXPECT_TRUE(this->pinned_includes_api_path(pinned, "/pin01.txt"));
  EXPECT_FALSE(this->pinned_includes_api_path(pinned, "/pin02.txt"));

  EXPECT_EQ(api_error::success,
            this->provider->set_item_meta("/pin01.txt", META_PINNED, "false"));
  pinned = this->provider->get_pinned_files();
  EXPECT_TRUE(pinned.empty());

  EXPECT_EQ(api_error::success, this->provider->remove_file("/pin01.txt"));
  EXPECT_EQ(api_error::success, this->provider->remove_file("/pin02.txt"));
}

TYPED_TEST(providers_test, remove_file_updates_pinned_files) {
  if (this->provider->is_read_only()) {
    auto pinned = this->provider->get_pinned_files();
    EXPECT_TRUE(pinned.empty());
    return;
  }

  this->create_file("/pin_keep.txt");
  this->create_file("/pin_delete.txt");
  this->create_file("/nopin.txt");

  EXPECT_EQ(api_error::success, this->provider->set_item_meta(
                                    "/pin_keep.txt", META_PINNED, "true"));
  EXPECT_EQ(api_error::success, this->provider->set_item_meta(
                                    "/pin_delete.txt", META_PINNED, "true"));
  EXPECT_EQ(api_error::success,
            this->provider->set_item_meta("/nopin.txt", META_PINNED, "false"));

  auto pinned = this->provider->get_pinned_files();
  EXPECT_EQ(std::size_t(2U), pinned.size());

  EXPECT_TRUE(this->pinned_includes_api_path(pinned, "/pin_keep.txt"));
  EXPECT_TRUE(this->pinned_includes_api_path(pinned, "/pin_delete.txt"));
  EXPECT_FALSE(this->pinned_includes_api_path(pinned, "/nopin.txt"));

  EXPECT_EQ(api_error::success, this->provider->remove_file("/pin_delete.txt"));

  pinned = this->provider->get_pinned_files();
  EXPECT_EQ(std::size_t(1U), pinned.size());
  EXPECT_TRUE(this->pinned_includes_api_path(pinned, "/pin_keep.txt"));
  EXPECT_FALSE(this->pinned_includes_api_path(pinned, "/pin_delete.txt"));

  EXPECT_EQ(api_error::success, this->provider->remove_file("/pin_keep.txt"));
  EXPECT_EQ(api_error::success, this->provider->remove_file("/nopin.txt"));
}

TYPED_TEST(providers_test, get_total_item_count) {
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    // TODO revisit
    /* std::uint64_t count{this->provider->get_total_item_count()};
    EXPECT_EQ(3U, count); */
    return;
  }

  std::uint64_t before{this->provider->get_total_item_count()};

  this->create_file("/count01.txt");
  this->create_file("/count02.txt");

  std::uint64_t mid{this->provider->get_total_item_count()};
  EXPECT_EQ(before + 2U, mid);

  EXPECT_EQ(api_error::success, this->provider->remove_file("/count01.txt"));
  EXPECT_EQ(api_error::success, this->provider->remove_file("/count02.txt"));

  std::uint64_t after{this->provider->get_total_item_count()};
  EXPECT_EQ(before, after);
}

TYPED_TEST(providers_test, get_used_drive_space) {
  if (this->provider->is_read_only()) {
    // TODO revisit
    /* api_file_list list{};
    std::string marker;
    EXPECT_EQ(api_error::success, this->provider->get_file_list(list, marker));

    std::uint64_t sum_sizes{};
    for (const auto &file : list) {
      std::uint64_t size{};
      EXPECT_EQ(api_error::success,
                this->provider->get_file_size(file.api_path, size));
      sum_sizes += size;
    }

    std::uint64_t used{this->provider->get_used_drive_space()};
    EXPECT_EQ(sum_sizes, used); */
    return;
  }

  std::uint64_t before{this->provider->get_used_drive_space()};

  auto &file1 = test::create_random_file(96U);
  auto &file2 = test::create_random_file(128U);

  auto api_path1 =
      fmt::format("/{}", utils::path::strip_to_file_name(file1.get_path()));
  auto api_path2 =
      fmt::format("/{}", utils::path::strip_to_file_name(file2.get_path()));

  this->create_file(api_path1);
  this->create_file(api_path2);

  stop_type stop_requested{false};
  ASSERT_EQ(
      api_error::success,
      this->provider->upload_file(api_path1, file1.get_path(), stop_requested));
  ASSERT_EQ(
      api_error::success,
      this->provider->upload_file(api_path2, file2.get_path(), stop_requested));

  EXPECT_EQ(api_error::success,
            this->provider->set_item_meta(api_path1, META_SIZE,
                                          std::to_string(*file1.size())));
  EXPECT_EQ(api_error::success,
            this->provider->set_item_meta(api_path2, META_SIZE,
                                          std::to_string(*file2.size())));

  std::uint64_t mid{this->provider->get_used_drive_space()};
  EXPECT_EQ(before + *file1.size() + *file2.size(), mid);

  EXPECT_EQ(api_error::success, this->provider->remove_file(api_path1));
  EXPECT_EQ(api_error::success, this->provider->remove_file(api_path2));

  std::uint64_t after{this->provider->get_used_drive_space()};
  EXPECT_EQ(before, after);
}

TYPED_TEST(providers_test, get_total_drive_space) {
  std::uint64_t total{this->provider->get_total_drive_space()};
  std::uint64_t used{this->provider->get_used_drive_space()};
  if (total != 0U) {
    EXPECT_GE(total, used);
  }
}

TYPED_TEST(providers_test, remove_item_meta) {
  std::string api_path{"/rim_custom_ok.txt"};
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    EXPECT_EQ(api_error::success,
              this->provider->remove_item_meta(api_path, "user.custom"));
    return;
  }

  this->create_file(api_path);

  EXPECT_EQ(api_error::success,
            this->provider->set_item_meta(api_path, "user.custom", "abc123"));

  api_meta_map before{};
  EXPECT_EQ(api_error::success,
            this->provider->get_item_meta(api_path, before));
  EXPECT_TRUE(before.contains("user.custom"));

  EXPECT_EQ(api_error::success,
            this->provider->remove_item_meta(api_path, "user.custom"));

  api_meta_map after{};
  EXPECT_EQ(api_error::success, this->provider->get_item_meta(api_path, after));
  EXPECT_FALSE(after.contains("user.custom"));

  EXPECT_EQ(api_error::success, this->provider->remove_file(api_path));
}

TYPED_TEST(providers_test, remove_item_meta_path_not_found) {
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    EXPECT_EQ(api_error::success,
              this->provider->remove_item_meta("/cow_moose_doge_chicken",
                                               "user.custom"));
    return;
  }

  auto res = this->provider->remove_item_meta("/cow_moose_doge_chicken",
                                              "user.custom");
  EXPECT_EQ(api_error::item_not_found, res);
}

TYPED_TEST(providers_test, remove_item_meta_restricted_names_fail) {
  std::string api_path;
  if (this->provider->get_provider_type() == provider_type::encrypt) {
    auto source_path = utils::path::combine(
        this->config->get_encrypt_config().path, {"test.txt"});
    EXPECT_EQ(api_error::success,
              this->provider->get_api_path_from_source(source_path, api_path));
  } else {
    api_path = "/rim_restricted.txt";
    this->create_file(api_path);
  }

  for (const auto &key : META_USED_NAMES) {
    auto res = this->provider->remove_item_meta(api_path, std::string{key});
    EXPECT_EQ(api_error::permission_denied, res);

    api_meta_map meta{};
    EXPECT_EQ(api_error::success,
              this->provider->get_item_meta(api_path, meta));
    EXPECT_TRUE(meta.contains(std::string{key}));
  }

  if (this->provider->get_provider_type() != provider_type::encrypt) {
    EXPECT_EQ(api_error::success, this->provider->remove_file(api_path));
  }
}

TYPED_TEST(providers_test, rename_file) {
  if (not this->provider->is_rename_supported()) {
    auto res = this->provider->rename_file("/rn_src.txt", "/rn_dst.txt");
    EXPECT_EQ(api_error::not_implemented, res);
    return;
  }

  std::string src{"/rn_src.txt"};
  std::string dst{"/rn_dst.txt"};
  this->create_file(src);

  std::string src_meta_size{};
  std::string src_meta_source{};
  EXPECT_EQ(api_error::success,
            this->provider->get_item_meta(src, META_SIZE, src_meta_size));
  EXPECT_EQ(api_error::success,
            this->provider->get_item_meta(src, META_SOURCE, src_meta_source));

  EXPECT_EQ(api_error::success, this->provider->rename_file(src, dst));

  bool exists{};
  EXPECT_EQ(api_error::success, this->provider->is_file(src, exists));
  EXPECT_FALSE(exists);
  EXPECT_EQ(api_error::success, this->provider->is_file(dst, exists));
  EXPECT_TRUE(exists);

  std::string dst_meta_size{};
  std::string dst_meta_source{};
  EXPECT_EQ(api_error::success,
            this->provider->get_item_meta(dst, META_SIZE, dst_meta_size));
  EXPECT_EQ(api_error::success,
            this->provider->get_item_meta(dst, META_SOURCE, dst_meta_source));

  EXPECT_STREQ(src_meta_size.c_str(), dst_meta_size.c_str());
  EXPECT_STREQ(src_meta_source.c_str(), dst_meta_source.c_str());

  EXPECT_EQ(api_error::success, this->provider->remove_file(dst));
}

TYPED_TEST(providers_test, rename_file_fails_if_source_not_found) {
  if (not this->provider->is_rename_supported()) {
    auto res = this->provider->rename_file("/rn_missing.txt", "/rn_any.txt");
    EXPECT_EQ(api_error::not_implemented, res);
    return;
  }

  auto res = this->provider->rename_file("/rn_missing.txt", "/rn_any.txt");
  EXPECT_EQ(api_error::item_not_found, res);
}

TYPED_TEST(providers_test, rename_file_fails_if_destination_exists) {
  if (not this->provider->is_rename_supported()) {
    if (this->provider->get_provider_type() != provider_type::encrypt) {
      this->create_file("/rn_src_conflict.txt");
      this->create_file("/rn_dst_conflict.txt");
    }
    auto res = this->provider->rename_file("/rn_src_conflict.txt",
                                           "/rn_dst_conflict.txt");
    EXPECT_EQ(api_error::not_implemented, res);

    if (this->provider->get_provider_type() != provider_type::encrypt) {
      EXPECT_EQ(api_error::success,
                this->provider->remove_file("/rn_src_conflict.txt"));
      EXPECT_EQ(api_error::success,
                this->provider->remove_file("/rn_dst_conflict.txt"));
    }
    return;
  }

  std::string src{"/rn_src_conflict.txt"};
  std::string dst{"/rn_dst_conflict.txt"};
  this->create_file(src);
  this->create_file(dst);

  auto res = this->provider->rename_file(src, dst);
  EXPECT_EQ(api_error::item_exists, res);

  bool exists{};
  EXPECT_EQ(api_error::success, this->provider->is_file(src, exists));
  EXPECT_TRUE(exists);
  EXPECT_EQ(api_error::success, this->provider->is_file(dst, exists));
  EXPECT_TRUE(exists);

  EXPECT_EQ(api_error::success, this->provider->remove_file(src));
  EXPECT_EQ(api_error::success, this->provider->remove_file(dst));
}

TYPED_TEST(providers_test, rename_file_fails_if_destination_is_directory) {
  if (not this->provider->is_rename_supported()) {
    if (this->provider->get_provider_type() != provider_type::encrypt) {
      this->create_file("/rn_src_conflict.txt");
      this->create_directory("/rn_dst_conflict");
    }
    auto res =
        this->provider->rename_file("/rn_src_conflict.txt", "/rn_dst_conflict");
    EXPECT_EQ(api_error::not_implemented, res);

    if (this->provider->get_provider_type() != provider_type::encrypt) {
      EXPECT_EQ(api_error::success,
                this->provider->remove_file("/rn_src_conflict.txt"));
      EXPECT_EQ(api_error::success,
                this->provider->remove_directory("/rn_dst_conflict"));
    }
    return;
  }

  std::string src{"/rn_src_conflict.txt"};
  std::string dst{"/rn_dst_conflict"};
  this->create_file(src);
  this->create_directory(dst);

  auto res = this->provider->rename_file(src, dst);
  EXPECT_EQ(api_error::directory_exists, res);

  bool exists{};
  EXPECT_EQ(api_error::success, this->provider->is_file(src, exists));
  EXPECT_TRUE(exists);
  EXPECT_EQ(api_error::success, this->provider->is_directory(dst, exists));
  EXPECT_TRUE(exists);

  EXPECT_EQ(api_error::success, this->provider->remove_file(src));
  EXPECT_EQ(api_error::success, this->provider->remove_directory(dst));
}

TYPED_TEST(providers_test, upload_file_not_implemented_on_read_only) {
  if (not this->provider->is_read_only()) {
    return;
  }

  auto &file = test::create_random_file(16U);
  stop_type stop_requested{false};
  auto res = this->provider->upload_file("/ro_upload.txt", file.get_path(),
                                         stop_requested);
  EXPECT_EQ(api_error::not_implemented, res);
}

TYPED_TEST(providers_test, upload_file_fails_if_source_is_not_found) {
  if (this->provider->is_read_only()) {
    return;
  }

  stop_type stop_requested{false};
  auto res = this->provider->upload_file(
      "/no_src_upload.txt", "/path/does/not/exist.bin", stop_requested);
  EXPECT_NE(res, api_error::success);
}

TYPED_TEST(providers_test,
           file_is_not_a_directory_and_a_directory_is_not_a_file) {
  if (this->provider->is_read_only()) {
    return;
  }

  std::string file_api_path{"/xf_file.txt"};
  std::string dir_api_path{"/xd_dir"};

  this->create_file(file_api_path);
  this->create_directory(dir_api_path);

  bool exists{};
  EXPECT_EQ(api_error::success,
            this->provider->is_directory(file_api_path, exists));
  EXPECT_FALSE(exists);

  EXPECT_EQ(api_error::success, this->provider->is_file(dir_api_path, exists));
  EXPECT_FALSE(exists);

  EXPECT_EQ(api_error::success, this->provider->remove_file(file_api_path));
  EXPECT_EQ(api_error::success, this->provider->remove_directory(dir_api_path));
}
} // namespace repertory
