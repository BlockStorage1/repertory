/*
  Copyright <2018-2024> <scott.e.graves@protonmail.com>

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
#include "test.hpp"

#if defined(PROJECT_ENABLE_SQLITE)

namespace repertory {
class utils_db_sqlite : public ::testing::Test {
public:
  utils::db::sqlite::db3_t db3;

  void SetUp() override {
    {
      sqlite3 *db3_ptr{nullptr};
      auto res = sqlite3_open(":memory:", &db3_ptr);
      ASSERT_EQ(SQLITE_OK, res);
      ASSERT_TRUE(db3_ptr != nullptr);
      db3 = utils::db::sqlite::db3_t{
          db3_ptr,
          utils::db::sqlite::sqlite3_deleter(),
      };
    }

    utils::db::sqlite::db3_stmt_t db3_stmt;
    {
      std::string sql{
          "CREATE TABLE [table] (column1 TEXT PRIMARY KEY UNIQUE "
          "NOT NULL, column2 TEXT NOT NULL);",
      };

      sqlite3_stmt *stmt_ptr{nullptr};
      auto res =
          sqlite3_prepare_v2(db3.get(), sql.c_str(), -1, &stmt_ptr, nullptr);
      db3_stmt = utils::db::sqlite::db3_stmt_t{
          stmt_ptr,
          utils::db::sqlite::sqlite3_statement_deleter(),
      };
      ASSERT_EQ(SQLITE_OK, res);
    }

    auto res = sqlite3_step(db3_stmt.get());
    ASSERT_EQ(SQLITE_DONE, res);
  }

  void TearDown() override { db3.reset(); }
};

static void common_insert(sqlite3 &db3, bool dump = false) {
  auto query = utils::db::sqlite::db_insert{db3, "table"}
                   .column_value("column1", "test0")
                   .column_value("column2", "test1");
  if (dump) {
    std::cout << query.dump() << std::endl;
  }

  auto res = query.go();
  EXPECT_TRUE(res.ok());
}

static void common_select(sqlite3 &db3, std::string value1, std::string value2,
                          bool dump = false) {
  auto query = utils::db::sqlite::db_select{db3, "table"};
  if (dump) {
    std::cout << query.dump() << std::endl;
  }

  auto res = query.go();
  EXPECT_TRUE(res.ok());
  EXPECT_TRUE(res.has_row());

  std::size_t row_count{};
  while (res.has_row()) {
    std::optional<utils::db::sqlite::db_result::row> row;
    EXPECT_TRUE(res.get_row(row));
    EXPECT_TRUE(row.has_value());
    if (row.has_value()) {
      auto columns = row.value().get_columns();
      EXPECT_EQ(std::size_t(2U), columns.size());
      EXPECT_STREQ("column1", columns[0U].get_name().c_str());
      EXPECT_STREQ(value1.c_str(),
                   columns[0U].get_value<std::string>().c_str());
      EXPECT_STREQ("column2", columns[1U].get_name().c_str());
      EXPECT_STREQ(value2.c_str(),
                   columns[1U].get_value<std::string>().c_str());
      for (auto &&column : columns) {
        std::cout << column.get_index() << ':';
        std::cout << column.get_name() << ':';
        std::cout << column.get_value<std::string>() << std::endl;
      }
    }
    ++row_count;
  }

  EXPECT_EQ(std::size_t(1U), row_count);
}

static void common_delete(sqlite3 &db3, bool dump = false) {
  {
    auto query = utils::db::sqlite::db_delete{db3, "table"};
    if (dump) {
      std::cout << query.dump() << std::endl;
    }

    auto res = query.go();
    EXPECT_TRUE(res.ok());
  }

  {
    auto query = utils::db::sqlite::db_select{db3, "table"};
    auto res = query.go();
    EXPECT_TRUE(res.ok());

    std::size_t row_count{};
    while (res.has_row()) {
      ++row_count;
    }

    EXPECT_EQ(std::size_t(0U), row_count);
  }
}

TEST_F(utils_db_sqlite, db_delete_query) {
  auto query = utils::db::sqlite::db_delete{*db3.get(), "table"};
  auto query_str = query.dump();
  std::cout << query_str << std::endl;
  EXPECT_STREQ(R"(DELETE FROM "table";)", query_str.c_str());
}

TEST_F(utils_db_sqlite, db_delete_where_query) {
  auto query = utils::db::sqlite::db_delete{*db3.get(), "table"}
                   .where("column1")
                   .equals("test1")
                   .and_()
                   .where("column2")
                   .equals("test2");
  auto query_str = query.dump();
  std::cout << query_str << std::endl;
  EXPECT_STREQ(R"(DELETE FROM "table" WHERE "column1"=?1 AND "column2"=?2;)",
               query_str.c_str());
}

TEST_F(utils_db_sqlite, db_insert_query) {
  auto query = utils::db::sqlite::db_insert{*db3.get(), "table"}
                   .column_value("column1", "test9")
                   .column_value("column2", "test9");
  auto query_str = query.dump();
  std::cout << query_str << std::endl;
  EXPECT_STREQ(R"(INSERT INTO "table" ("column1", "column2") VALUES (?1, ?2);)",
               query_str.c_str());
}

TEST_F(utils_db_sqlite, db_insert_or_replace_query) {
  auto query = utils::db::sqlite::db_insert{*db3.get(), "table"}
                   .or_replace()
                   .column_value("column1", "test1")
                   .column_value("column2", "test2");
  auto query_str = query.dump();
  std::cout << query_str << std::endl;
  EXPECT_STREQ(
      R"(INSERT OR REPLACE INTO "table" ("column1", "column2") VALUES (?1, ?2);)",
      query_str.c_str());
}

TEST_F(utils_db_sqlite, db_select_query) {
  auto query = utils::db::sqlite::db_select{*db3.get(), "table"};
  auto query_str = query.dump();
  std::cout << query_str << std::endl;
  EXPECT_STREQ(R"(SELECT * FROM "table";)", query_str.c_str());
}

TEST_F(utils_db_sqlite, db_select_where_query) {
  auto query = utils::db::sqlite::db_select{*db3.get(), "table"}
                   .where("column1")
                   .equals("test1")
                   .and_()
                   .where("column2")
                   .equals("test2");
  auto query_str = query.dump();
  std::cout << query_str << std::endl;
  EXPECT_STREQ(R"(SELECT * FROM "table" WHERE "column1"=?1 AND "column2"=?2;)",
               query_str.c_str());
}

TEST_F(utils_db_sqlite, db_select_where_with_group_query) {
  auto query =
      utils::db::sqlite::db_select{*db3.get(), "table"}
          .group([](auto &grp) {
            grp.where("column1").equals("a").or_().where("column1").equals("b");
          })
          .and_()
          .group([](auto &grp) {
            grp.where("column2").equals("c").or_().where("column2").equals("d");
          })
          .or_()
          .group([](auto &grp) {
            grp.where("column1").equals("e").or_().where("column2").equals("f");
          });
  auto query_str = query.dump();
  std::cout << query_str << std::endl;
  EXPECT_STREQ(
      R"(SELECT * FROM "table" WHERE ("column1"=?1 OR "column1"=?2) AND ("column2"=?3 OR "column2"=?4) OR ("column1"=?5 OR "column2"=?6);)",
      query_str.c_str());
}

TEST_F(utils_db_sqlite, db_select_columns_query) {
  auto query = utils::db::sqlite::db_select{*db3.get(), "table"}
                   .column("column1")
                   .column("column2")
                   .where("column1")
                   .equals("test1")
                   .and_()
                   .where("column2")
                   .equals("test2");
  auto query_str = query.dump();
  std::cout << query_str << std::endl;
  EXPECT_STREQ(
      R"(SELECT column1, column2 FROM "table" WHERE "column1"=?1 AND "column2"=?2;)",
      query_str.c_str());
}

TEST_F(utils_db_sqlite, db_update_query) {
  auto query = utils::db::sqlite::db_update{*db3.get(), "table"}
                   .column_value("column1", "moose")
                   .where("column1")
                   .equals("test1")
                   .and_()
                   .where("column2")
                   .equals("test2");
  auto query_str = query.dump();
  std::cout << query_str << std::endl;
  EXPECT_STREQ(
      R"(UPDATE "table" SET "column1"=?1 WHERE "column1"=?2 AND "column2"=?3;)",
      query_str.c_str());
}

TEST_F(utils_db_sqlite, insert_select_delete) {
  common_insert(*db3.get(), true);

  common_select(*db3.get(), "test0", "test1", true);

  common_delete(*db3.get(), true);
}

TEST_F(utils_db_sqlite, insert_update_delete) {
  common_insert(*db3.get());

  {
    auto query = utils::db::sqlite::db_update{*db3.get(), "table"}
                     .column_value("column1", "moose")
                     .where("column1")
                     .equals("test0");
    std::cout << query.dump() << std::endl;
    auto res = query.go();
    EXPECT_TRUE(res.ok());
  }

  common_select(*db3.get(), "moose", "test1");

  common_delete(*db3.get());
}

TEST_F(utils_db_sqlite, insert_or_replace_and_delete) {
  common_insert(*db3.get());

  {
    auto query = utils::db::sqlite::db_insert{*db3.get(), "table"}
                     .or_replace()
                     .column_value("column1", "test0")
                     .column_value("column2", "moose");
    std::cout << query.dump() << std::endl;
    auto res = query.go();
    EXPECT_TRUE(res.ok());
  }

  common_select(*db3.get(), "test0", "moose");

  common_delete(*db3.get());
}
} // namespace repertory

#endif // defined(PROJECT_ENABLE_SQLITE)
