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
#include "fixtures/directory_db_fixture.hpp"
#include "test_common.hpp"

namespace repertory {
static const auto dirs = {"/",
                          "/root",
                          "/root/sub1",
                          "/root/sub2",
                          "/root/sub2/sub2_sub1",
                          "/root/sub2/sub2_sub2",
                          "/root/sub2/sub2_sub2/sub2_sub2_sub1",
                          "/root/sub3"};

TEST_F(directory_db_test, is_directory) {
  for (const auto &dir : dirs) {
    db_->create_directory(dir);
  }

  for (const auto &dir : dirs) {
    EXPECT_TRUE(db_->is_directory(dir));
  }
}

TEST_F(directory_db_test, remove_directory) {
  for (const auto &dir : dirs) {
    db_->create_directory(dir);
  }

  EXPECT_EQ(api_error::success, db_->remove_directory("/root/sub2/sub2_sub1"));
  EXPECT_FALSE(db_->is_directory("/root/sub2/sub2_sub1"));
  EXPECT_EQ(1u, db_->get_sub_directory_count("/root/sub2"));
  EXPECT_TRUE(db_->is_directory("/root/sub2/sub2_sub2"));
}

TEST_F(directory_db_test, get_sub_directory_count) {
  for (const auto &dir : dirs) {
    db_->create_directory(dir);
  }

  EXPECT_EQ(1u, db_->get_sub_directory_count("/"));
  EXPECT_EQ(3u, db_->get_sub_directory_count("/root"));
  EXPECT_EQ(0u, db_->get_sub_directory_count("/root/sub1"));
  EXPECT_EQ(2u, db_->get_sub_directory_count("/root/sub2"));
  EXPECT_EQ(0u, db_->get_sub_directory_count("/root/sub2/sub2_sub1"));
  EXPECT_EQ(1u, db_->get_sub_directory_count("/root/sub2/sub2_sub2"));
  EXPECT_EQ(0u, db_->get_sub_directory_count("/root/sub3"));
}

TEST_F(directory_db_test, populate_sub_directories) {
  for (const auto &dir : dirs) {
    db_->create_directory(dir);
  }

  directory_item_list list{};
  const auto dump_directory_list = [&]() {
    for (const auto &di : list) {
      std::cout << di.to_json().dump(2) << std::endl;
    }
    list.clear();
  };

  std::cout << "/" << std::endl;
  db_->populate_sub_directories(
      "/", [](directory_item &, const bool &) {}, list);
  EXPECT_EQ(1u, list.size());
  dump_directory_list();

  std::cout << std::endl << "/root" << std::endl;
  db_->populate_sub_directories(
      "/root", [](directory_item &, const bool &) {}, list);
  EXPECT_EQ(3u, list.size());
  dump_directory_list();

  std::cout << std::endl << "/root/sub1" << std::endl;
  db_->populate_sub_directories(
      "/root/sub1", [](directory_item &, const bool &) {}, list);
  EXPECT_EQ(0u, list.size());
  dump_directory_list();

  std::cout << std::endl << "/root/sub2" << std::endl;
  db_->populate_sub_directories(
      "/root/sub2", [](directory_item &, const bool &) {}, list);
  EXPECT_EQ(2u, list.size());
  dump_directory_list();

  std::cout << std::endl << "/root/sub2/sub2_sub1" << std::endl;
  db_->populate_sub_directories(
      "/root/sub2/sub2_sub1", [](directory_item &, const bool &) {}, list);
  EXPECT_EQ(0u, list.size());
  dump_directory_list();

  std::cout << std::endl << "/root/sub2/sub2_sub2" << std::endl;
  db_->populate_sub_directories(
      "/root/sub2/sub2_sub2", [](directory_item &, const bool &) {}, list);
  EXPECT_EQ(1u, list.size());
  dump_directory_list();

  std::cout << std::endl << "/root/sub3" << std::endl;
  db_->populate_sub_directories(
      "/root/sub3", [](directory_item &, const bool &) {}, list);
  EXPECT_EQ(0u, list.size());
  dump_directory_list();
}

TEST_F(directory_db_test, is_file) {
  for (const auto &dir : dirs) {
    db_->create_directory(dir);
  }

  EXPECT_EQ(api_error::success, db_->create_file("/cow.txt"));
  EXPECT_TRUE(db_->is_file("/cow.txt"));
  EXPECT_FALSE(db_->is_directory("/cow.txt"));
  EXPECT_EQ(api_error::file_exists, db_->create_file("/cow.txt"));
  EXPECT_EQ(api_error::file_exists, db_->create_directory("/cow.txt"));
}

TEST_F(directory_db_test, remove_file) {
  for (const auto &dir : dirs) {
    db_->create_directory(dir);
  }

  EXPECT_EQ(api_error::success, db_->create_file("/cow.txt"));
  EXPECT_EQ(api_error::item_is_file, db_->remove_directory("/cow.txt"));
  EXPECT_TRUE(db_->remove_file("/cow.txt"));
  EXPECT_FALSE(db_->is_file("/cow.txt"));
}

TEST_F(directory_db_test, get_directory_item_count) {
  db_->create_directory("/");

  EXPECT_EQ(api_error::success, db_->create_file("/cow.txt"));
  EXPECT_EQ(api_error::success, db_->create_file("/cow2.txt"));
  EXPECT_EQ(api_error::success, db_->create_directory("/cow"));
  EXPECT_EQ(3u, db_->get_directory_item_count("/"));
}

TEST_F(directory_db_test, get_file) {
  db_->create_directory("/");

  EXPECT_EQ(api_error::success, db_->create_file("/cow.txt"));

  api_file file{};
  EXPECT_EQ(api_error::success, db_->get_file("/cow.txt", file, [](api_file &file) {
    EXPECT_STREQ("/cow.txt", file.api_path.c_str());
  }));
  EXPECT_STREQ("/cow.txt", file.api_path.c_str());
}

TEST_F(directory_db_test, get_file_list) {
  db_->create_directory("/");

  EXPECT_EQ(api_error::success, db_->create_file("/cow.txt"));
  EXPECT_EQ(api_error::success, db_->create_file("/cow2.txt"));

  api_file_list list;
  int i = 0;
  EXPECT_EQ(api_error::success, db_->get_file_list(list, [&i](api_file &file) {
    if (i++ == 0) {
      EXPECT_STREQ("/cow.txt", file.api_path.c_str());
    } else {
      EXPECT_STREQ("/cow2.txt", file.api_path.c_str());
    }
  }));

  EXPECT_EQ(std::size_t(2u), list.size());
  EXPECT_STREQ("/cow.txt", list[0u].api_path.c_str());
  EXPECT_STREQ("/cow2.txt", list[1u].api_path.c_str());
}

TEST_F(directory_db_test, get_total_item_count) {
  db_->create_directory("/");

  EXPECT_EQ(api_error::success, db_->create_file("/cow.txt"));
  EXPECT_EQ(api_error::success, db_->create_file("/cow2.txt"));

  EXPECT_EQ(api_error::success, db_->create_directory("/cow"));
  EXPECT_EQ(api_error::success, db_->create_directory("/cow/moose"));

  EXPECT_EQ(std::uint64_t(5), db_->get_total_item_count());
}

TEST_F(directory_db_test, populate_directory_files) {
  db_->create_directory("/");

  EXPECT_EQ(api_error::success, db_->create_file("/cow.txt"));
  EXPECT_EQ(api_error::success, db_->create_file("/cow2.txt"));

  directory_item_list list;
  int i = 0;
  db_->populate_directory_files(
      "/",
      [&i](directory_item &di, const bool &) {
        di.meta[META_SIZE] = utils::string::from_int32(i + 1);
        EXPECT_FALSE(di.directory);
        if (i++ == 0) {
          EXPECT_STREQ("/cow.txt", &di.api_path[0]);
        } else {
          EXPECT_STREQ("/cow2.txt", &di.api_path[0]);
        }
      },
      list);

  EXPECT_EQ(std::size_t(2u), list.size());

  EXPECT_EQ(1u, list[0].size);
  EXPECT_STREQ("/cow.txt", &list[0].api_path[0]);

  EXPECT_EQ(2u, list[1].size);
  EXPECT_STREQ("/cow2.txt", &list[1].api_path[0]);
}

TEST_F(directory_db_test, create_directory_fails_if_directory_exists) {
  for (const auto &dir : dirs) {
    db_->create_directory(dir);
  }

  EXPECT_EQ(api_error::directory_exists, db_->create_file("/root/sub1"));
  EXPECT_TRUE(db_->is_directory("/root/sub1"));
}

TEST_F(directory_db_test, create_file_fails_if_file_exists) {
  for (const auto &dir : dirs) {
    db_->create_directory(dir);
  }

  EXPECT_EQ(api_error::success, db_->create_file("/cow.txt"));
  EXPECT_EQ(api_error::file_exists, db_->create_directory("/cow.txt"));
}

TEST_F(directory_db_test, create_file_fails_if_parent_does_not_exist) {
  for (const auto &dir : dirs) {
    db_->create_directory(dir);
  }

  EXPECT_EQ(api_error::directory_not_found, db_->create_file("/moose/cow.txt"));
}

TEST_F(directory_db_test, create_directory_fails_if_parent_does_not_exist) {
  for (const auto &dir : dirs) {
    db_->create_directory(dir);
  }

  EXPECT_EQ(api_error::directory_not_found, db_->create_file("/cow/moose"));
}

TEST_F(directory_db_test, remove_file_fails_if_directory_exists) {
  for (const auto &dir : dirs) {
    db_->create_directory(dir);
  }

  EXPECT_FALSE(db_->remove_file("/root/sub1"));
}

TEST_F(directory_db_test, remove_directory_fails_if_file_exists) {
  for (const auto &dir : dirs) {
    db_->create_directory(dir);
  }

  EXPECT_EQ(api_error::success, db_->create_file("/cow.txt"));
  EXPECT_EQ(api_error::item_is_file, db_->remove_directory("/cow.txt"));
}

TEST_F(directory_db_test, remove_directory_fails_if_sub_directories_exist) {
  db_->create_directory("/");
  db_->create_directory("/sub");
  db_->create_directory("/sub/sub2");

  EXPECT_EQ(api_error::directory_not_empty, db_->remove_directory("/sub"));
  EXPECT_TRUE(db_->is_directory("/sub"));
  EXPECT_TRUE(db_->is_directory("/sub/sub2"));
}

TEST_F(directory_db_test, remove_directory_fails_if_files_exist) {
  db_->create_directory("/");
  db_->create_directory("/sub");
  db_->create_file("/sub/test.txt");

  EXPECT_EQ(api_error::directory_not_empty, db_->remove_directory("/sub"));
  EXPECT_TRUE(db_->is_directory("/sub"));
  EXPECT_TRUE(db_->is_file("/sub/test.txt"));
}

TEST_F(directory_db_test, remove_directory_fails_for_root_directory_by_default) {
  db_->create_directory("/");

  EXPECT_EQ(api_error::access_denied, db_->remove_directory("/"));
  EXPECT_TRUE(db_->is_directory("/"));
}

TEST_F(directory_db_test,
       remove_directory_succeeds_for_root_directory_if_allow_remove_root_is_true) {
  db_->create_directory("/");

  EXPECT_EQ(api_error::success, db_->remove_directory("/", true));
  EXPECT_FALSE(db_->is_directory("/"));
}
} // namespace repertory
