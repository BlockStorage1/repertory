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
#ifndef REPERTORY_META_DB_FIXTURE_H
#define REPERTORY_META_DB_FIXTURE_H

#include "test_common.hpp"

#include "app_config.hpp"
#include "db/meta_db.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
class meta_db_test : public ::testing::Test {
private:
  const std::string config_location_ = utils::path::absolute("./metadb");

protected:
  std::unique_ptr<app_config> config_;
  std::unique_ptr<meta_db> db_;

public:
  void SetUp() override {
    ASSERT_TRUE(utils::file::delete_directory_recursively(config_location_));
    config_ =
        std::make_unique<app_config>(provider_type::sia, config_location_);
    db_ = std::make_unique<meta_db>(*config_.get());
  }

  void TearDown() override {
    db_.reset();
    config_.reset();

    EXPECT_TRUE(utils::file::delete_directory_recursively(config_location_));
  }
};
} // namespace repertory

#endif // REPERTORY_META_DB_FIXTURE_H
