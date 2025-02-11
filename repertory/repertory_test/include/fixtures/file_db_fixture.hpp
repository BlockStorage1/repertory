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
#ifndef REPERTORY_TEST_INCLUDE_FIXTURES_FILE_DB_FIXTURE_HPP
#define REPERTORY_TEST_INCLUDE_FIXTURES_FILE_DB_FIXTURE_HPP

#include "test_common.hpp"

#include "app_config.hpp"
#include "db/impl/rdb_file_db.hpp"
#include "db/impl/sqlite_file_db.hpp"
#include "events/consumers/console_consumer.hpp"
#include "events/event_system.hpp"

namespace repertory {
template <typename db_t> class file_db_test : public ::testing::Test {
protected:
  static std::unique_ptr<app_config> config;
  static console_consumer console_;
  static std::unique_ptr<db_t> file_db;

protected:
  static void SetUpTestCase() {
    static std::uint64_t idx{};

    event_system::instance().start();

    auto cfg_directory = utils::path::combine(test::get_test_output_dir(),
                                              {
                                                  "file_db_test",
                                                  std::to_string(++idx),
                                              });
    config = std::make_unique<app_config>(provider_type::s3, cfg_directory);
    file_db = std::make_unique<db_t>(*config);
  }

  static void TearDownTestCase() {
    file_db.reset();
    config.reset();
    event_system::instance().stop();
  }
};

using file_db_types = ::testing::Types<rdb_file_db, sqlite_file_db>;

template <typename db_t> std::unique_ptr<app_config> file_db_test<db_t>::config;

template <typename db_t> console_consumer file_db_test<db_t>::console_;

template <typename db_t> std::unique_ptr<db_t> file_db_test<db_t>::file_db;
} // namespace repertory

#endif // REPERTORY_TEST_INCLUDE_FIXTURES_FILE_DB_FIXTURE_HPP
