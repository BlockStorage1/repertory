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
#include "db/retry_db.hpp"
#include "test_common.hpp"

namespace repertory {
TEST(retry_db, exists_and_set) {
  const std::string data_directory = "./retrydb_exists_and_set";
  utils::file::delete_directory_recursively(data_directory);
  {
    app_config config(provider_type::sia, data_directory);
    retry_db db(config);

    const auto api_path = "my_test/path.txt";

    EXPECT_FALSE(db.exists(api_path));
    db.set(api_path);
    EXPECT_TRUE(db.exists(api_path));
#ifdef _WIN32
    EXPECT_FALSE(db.exists(utils::string::to_upper(api_path)));
#endif
  }

  utils::file::delete_directory_recursively(data_directory);
}

TEST(retry_db, rename) {
  const std::string data_directory = "./retrydb_rename";
  utils::file::delete_directory_recursively(data_directory);
  {
    app_config config(provider_type::sia, data_directory);
    retry_db db(config);

    const auto api_path = "my_test/path.txt";
    const auto api_path2 = "my_test/path2.txt";

    db.set(api_path);
    db.rename(api_path, api_path2);
    EXPECT_FALSE(db.exists(api_path));
    EXPECT_TRUE(db.exists(api_path2));
  }

  utils::file::delete_directory_recursively(data_directory);
}

TEST(retry_db, remove) {
  const std::string data_directory = "./retrydb_remove";
  utils::file::delete_directory_recursively(data_directory);
  {
    app_config config(provider_type::sia, data_directory);
    retry_db db(config);

    const auto api_path = "my_test/path.txt";

    db.set(api_path);
    db.remove(api_path);
    EXPECT_FALSE(db.exists(api_path));
  }

  utils::file::delete_directory_recursively(data_directory);
}

TEST(retry_db, process_all) {
  const std::string data_directory = "./retrydb_process_all";
  utils::file::delete_directory_recursively(data_directory);
  {
    app_config config(provider_type::sia, data_directory);
    retry_db db(config);

    const auto api_path = "my_test/path.txt";

    for (auto i = 0; i < 10; i++) {
      db.set(api_path + std::to_string(i));
    }

    std::vector<std::string> items;
    db.process_all([&items](const std::string &api_path) -> bool {
      items.emplace_back(api_path);
      return false;
    });
    EXPECT_EQ((std::size_t)10u, items.size());

    for (auto i = 0; i < 10; i++) {
      EXPECT_TRUE(db.exists(api_path + std::to_string(i)));
      utils::remove_element_from(items, api_path + std::to_string(i));
    }
    EXPECT_TRUE(items.empty());

    db.process_all([&items](const std::string &api_path) -> bool {
      items.emplace_back(api_path);
      return true;
    });
    EXPECT_EQ((std::size_t)10u, items.size());

    for (auto i = 0; i < 10; i++) {
      EXPECT_FALSE(db.exists(api_path + std::to_string(i)));
      utils::remove_element_from(items, api_path + std::to_string(i));
    }
    EXPECT_TRUE(items.empty());

    db.process_all([&items](const std::string &api_path) -> bool {
      items.emplace_back(api_path);
      return true;
    });
    EXPECT_TRUE(items.empty());
  }

  utils::file::delete_directory_recursively(data_directory);
}
} // namespace repertory
