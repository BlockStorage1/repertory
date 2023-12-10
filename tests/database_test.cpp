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

#include "database/db_common.hpp"
#include "database/db_insert.hpp"
#include "database/db_select.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
TEST(database, db_insert) {
  console_consumer consumer1;
  event_system::instance().start();
  {
    db3_t db3;
    {
      sqlite3 *db3_ptr{nullptr};
      auto res = sqlite3_open_v2(
          utils::path::absolute(
              utils::path::combine(get_test_dir(), {"test.db3"}))
              .c_str(),
          &db3_ptr, SQLITE_OPEN_READWRITE, nullptr);
      ASSERT_EQ(SQLITE_OK, res);
      ASSERT_TRUE(db3_ptr != nullptr);

      db3.reset(db3_ptr);
    }

    auto query = db::db_insert{*db3.get(), "table"}
                     .column_value("column1", "test9")
                     .column_value("column2", "test9");
    auto query_str = query.dump();
    std::cout << query_str << std::endl;
    EXPECT_STREQ(
        R"(INSERT INTO "table" ("column1", "column2") VALUES (?1, ?2);)",
        query_str.c_str());

    query = db::db_insert{*db3.get(), "table"}
                .or_replace()
                .column_value("column1", "test1")
                .column_value("column2", "test2");
    query_str = query.dump();
    std::cout << query_str << std::endl;
    EXPECT_STREQ(
        R"(INSERT OR REPLACE INTO "table" ("column1", "column2") VALUES (?1, ?2);)",
        query_str.c_str());

    auto res = query.go();
    EXPECT_TRUE(res.ok());
    EXPECT_FALSE(res.has_row());
  }

  event_system::instance().stop();
}

TEST(database, db_select) {
  console_consumer consumer1;
  event_system::instance().start();
  {
    db3_t db3;
    {
      sqlite3 *db3_ptr{nullptr};
      auto res = sqlite3_open_v2(
          utils::path::combine(get_test_dir(), {"test.db3"}).c_str(), &db3_ptr,
          SQLITE_OPEN_READWRITE, nullptr);
      ASSERT_EQ(SQLITE_OK, res);
      ASSERT_TRUE(db3_ptr != nullptr);

      db3.reset(db3_ptr);
    }

    auto query = db::db_select{*db3.get(), "table"}
                     .where("column1")
                     .equals("test1")
                     .and_where("column2")
                     .equals("test2");
    auto query_str = query.dump();
    std::cout << query_str << std::endl;
    EXPECT_STREQ(
        R"(SELECT * FROM "table" WHERE ("column1"=?1 AND "column2"=?2);)",
        query_str.c_str());
    auto res = query.go();

    EXPECT_TRUE(res.ok());
    EXPECT_TRUE(res.has_row());
    std::size_t row_count{};
    while (res.has_row()) {
      std::optional<db::db_select::row> row;
      EXPECT_TRUE(res.get_row(row));
      EXPECT_TRUE(row.has_value());
      if (row.has_value()) {
        for (const auto &column : row.value().get_columns()) {
          std::cout << column.get_index() << ':';
          std::cout << column.get_name() << ':';
          std::cout << column.get_value<std::string>() << std::endl;
        }
      }
      ++row_count;
    }
    EXPECT_EQ(std::size_t(1U), row_count);
  }

  event_system::instance().stop();
}
} // namespace repertory
