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

#include "file_manager/file_manager.hpp"
#include "mocks/mock_provider.hpp"
#include "mocks/mock_upload_manager.hpp"
#include "utils/event_capture.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
static constexpr const std::size_t test_chunk_size = 1024u;

TEST(upload, can_upload_a_valid_file) {
  console_consumer c;

  event_system::instance().start();

  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 4u;
  fsi.source_path = source_path;

  event_consumer ec("file_upload_completed", [&fsi](const event &e) {
    const auto &ee = dynamic_cast<const file_upload_completed &>(e);
    EXPECT_STREQ(fsi.api_path.c_str(),
                 ee.get_api_path().get<std::string>().c_str());
    EXPECT_STREQ(fsi.source_path.c_str(),
                 ee.get_source().get<std::string>().c_str());
    EXPECT_STREQ("success", ee.get_result().get<std::string>().c_str());
    EXPECT_STREQ("0", ee.get_cancelled().get<std::string>().c_str());
  });

  EXPECT_CALL(mp, upload_file(fsi.api_path, fsi.source_path, _))
      .WillOnce([&fsi](const std::string &, const std::string &,
                       stop_type &stop_requested) -> api_error {
        EXPECT_FALSE(stop_requested);
        return api_error::success;
      });
  file_manager::upload upload(fsi, mp);

  event_capture e({"file_upload_completed"});
  e.wait_for_empty();

  EXPECT_EQ(api_error::success, upload.get_api_error());
  EXPECT_FALSE(upload.is_cancelled());

  event_system::instance().stop();
}

TEST(upload, can_cancel_upload) {
  console_consumer c;

  event_system::instance().start();

  const auto source_path = generate_test_file_name(".", "test");
  // create_random_file(source_path, test_chunk_size * 4u)->close();

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 4u;
  fsi.source_path = source_path;

  event_consumer ec("file_upload_completed", [&fsi](const event &e) {
    const auto &ee = dynamic_cast<const file_upload_completed &>(e);
    EXPECT_STREQ(fsi.api_path.c_str(),
                 ee.get_api_path().get<std::string>().c_str());
    EXPECT_STREQ(fsi.source_path.c_str(),
                 ee.get_source().get<std::string>().c_str());
    EXPECT_STREQ("upload_stopped", ee.get_result().get<std::string>().c_str());
    EXPECT_STREQ("1", ee.get_cancelled().get<std::string>().c_str());
  });

  std::mutex mtx;
  std::condition_variable cv;

  EXPECT_CALL(mp, upload_file(fsi.api_path, fsi.source_path, _))
      .WillOnce([&cv, &fsi, &mtx](const std::string &, const std::string &,
                                  stop_type &stop_requested) -> api_error {
        EXPECT_FALSE(stop_requested);

        unique_mutex_lock l(mtx);
        cv.notify_one();
        l.unlock();

        l.lock();
        cv.wait(l);
        l.unlock();

        EXPECT_TRUE(stop_requested);

        return api_error::upload_stopped;
      });

  unique_mutex_lock l(mtx);
  file_manager::upload upload(fsi, mp);
  cv.wait(l);

  upload.cancel();

  cv.notify_one();
  l.unlock();

  event_capture e({"file_upload_completed"});
  e.wait_for_empty();

  EXPECT_EQ(api_error::upload_stopped, upload.get_api_error());
  EXPECT_TRUE(upload.is_cancelled());

  event_system::instance().stop();
}

TEST(upload, can_stop_upload) {
  console_consumer c;

  event_system::instance().start();

  const auto source_path = generate_test_file_name(".", "test");

  mock_provider mp;

  EXPECT_CALL(mp, is_direct_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 4u;
  fsi.source_path = source_path;

  event_consumer ec("file_upload_completed", [&fsi](const event &e) {
    const auto &ee = dynamic_cast<const file_upload_completed &>(e);
    EXPECT_STREQ(fsi.api_path.c_str(),
                 ee.get_api_path().get<std::string>().c_str());
    EXPECT_STREQ(fsi.source_path.c_str(),
                 ee.get_source().get<std::string>().c_str());
    EXPECT_STREQ("upload_stopped", ee.get_result().get<std::string>().c_str());
    EXPECT_STREQ("0", ee.get_cancelled().get<std::string>().c_str());
  });

  EXPECT_CALL(mp, upload_file(fsi.api_path, fsi.source_path, _))
      .WillOnce([&fsi](const std::string &, const std::string &,
                       stop_type &stop_requested) -> api_error {
        std::this_thread::sleep_for(3s);
        EXPECT_TRUE(stop_requested);
        return api_error::upload_stopped;
      });

  event_capture e({"file_upload_completed"});

  { file_manager::upload upload(fsi, mp); }

  e.wait_for_empty();

  event_system::instance().stop();
}
} // namespace repertory
