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

#include "app_config.hpp"
#include "events/consumers/console_consumer.hpp"
#include "providers/encrypt/encrypt_provider.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
/* TEST(encrypt_provider, can_construct_encrypt_provider) {
  ASSERT_TRUE(
      utils::file::delete_directory_recursively("./encrypt_provider_test"));

  {
    console_consumer cc{};
    event_system::instance().start();

    app_config cfg(provider_type::encrypt, "./encrypt_provider_test");
    encrypt_provider provider(cfg);

    event_system::instance().stop();
  }

  ASSERT_TRUE(
      utils::file::delete_directory_recursively("./encrypt_provider_test"));
}

TEST(encrypt_provider, can_get_file_list) {
  ASSERT_TRUE(
      utils::file::delete_directory_recursively("./encrypt_provider_test"));

  {
    console_consumer cc{};
    event_system::instance().start();

    const auto path = std::filesystem::path(utils::path::absolute(__FILE__))
                          .parent_path()
                          .string();

    app_config cfg(provider_type::encrypt, "./encrypt_provider_test");
    EXPECT_STREQ(
        path.c_str(),
        cfg.set_value_by_name("EncryptConfig.Path", path.c_str()).c_str());
    EXPECT_STREQ(
        "test_token",
        cfg.set_value_by_name("EncryptConfig.EncryptionToken", "test_token")
            .c_str());
    encrypt_provider provider(cfg);

    api_file_list list{};
    EXPECT_EQ(api_error::success, provider.get_file_list(list));

    api_file_list list2{};
    EXPECT_EQ(api_error::success, provider.get_file_list(list2));

    EXPECT_EQ(list.size(), list2.size());
    for (std::size_t idx = 0U; idx < list.size(); idx++) {
      EXPECT_STREQ(list.at(idx).api_path.c_str(),
                   list2.at(idx).api_path.c_str());
      EXPECT_STREQ(list.at(idx).api_parent.c_str(),
                   list2.at(idx).api_parent.c_str());
      EXPECT_EQ(list.at(idx).accessed_date, list2.at(idx).accessed_date);
      EXPECT_EQ(list.at(idx).changed_date, list2.at(idx).changed_date);
      EXPECT_TRUE(list.at(idx).encryption_token.empty());
      EXPECT_TRUE(list2.at(idx).encryption_token.empty());
      EXPECT_EQ(list.at(idx).file_size, list2.at(idx).file_size);
      EXPECT_TRUE(list.at(idx).key.empty());
      EXPECT_TRUE(list2.at(idx).key.empty());
      EXPECT_EQ(list.at(idx).modified_date, list2.at(idx).modified_date);
      EXPECT_STREQ(list.at(idx).source_path.c_str(),
                   list2.at(idx).source_path.c_str());
    }

    event_system::instance().stop();
  }

  ASSERT_TRUE(
      utils::file::delete_directory_recursively("./encrypt_provider_test"));
}

TEST(encrypt_provider, can_encrypt_and_decrypt_file) {
  ASSERT_TRUE(
      utils::file::delete_directory_recursively("./encrypt_provider_test"));

  {
    console_consumer cc{};
    event_system::instance().start();

    const auto path = std::filesystem::path(utils::path::absolute(__FILE__))
                          .parent_path()
                          .string();

    app_config cfg(provider_type::encrypt, "./encrypt_provider_test");
    EXPECT_STREQ(
        path.c_str(),
        cfg.set_value_by_name("EncryptConfig.Path", path.c_str()).c_str());
    EXPECT_STREQ(
        "test_token",
        cfg.set_value_by_name("EncryptConfig.EncryptionToken", "test_token")
            .c_str());
    encrypt_provider provider(cfg);

    api_file_list list{};
    EXPECT_EQ(api_error::success, provider.get_file_list(list));
    for (const auto &file : list) {
      std::string file_path{file.api_path};
      EXPECT_EQ(api_error::success,
                utils::encryption::decrypt_file_path(
                    cfg.get_encrypt_config().encryption_token, file_path));
      file_path =
          utils::path::combine(cfg.get_encrypt_config().path, {file_path});

      EXPECT_TRUE(std::filesystem::equivalent(file.source_path, file_path));

      std::vector<char> encrypted_data{};
      stop_type stop_requested = false;
      EXPECT_EQ(api_error::success,
                provider.read_file_bytes(file.api_path, file.file_size, 0U,
                                         encrypted_data, stop_requested));
      EXPECT_EQ(file.file_size, encrypted_data.size());

      std::vector<char> decrypted_data{};
      EXPECT_TRUE(utils::encryption::decrypt_data(
          cfg.get_encrypt_config().encryption_token, encrypted_data,
          decrypted_data));

      native_file::native_file_ptr nf{};
      EXPECT_EQ(api_error::success, native_file::open(file.source_path, nf));

      std::uint64_t file_size{};
      EXPECT_TRUE(nf->get_file_size(file_size));
      EXPECT_EQ(file_size, decrypted_data.size());

      std::size_t bytes_read{};
      std::vector<char> file_data{};
      file_data.resize(file_size);
      EXPECT_TRUE(nf->read_bytes(file_data.data(), file_size, 0U, bytes_read));
      nf->close();

      EXPECT_EQ(file_size, bytes_read);
      for (std::uint64_t i = 0U; i < bytes_read; i++) {
        ASSERT_EQ(file_data[i], decrypted_data[i]);
      }
    }

    event_system::instance().stop();
  }

  ASSERT_TRUE(
      utils::file::delete_directory_recursively("./encrypt_provider_test"));
} */
} // namespace repertory
