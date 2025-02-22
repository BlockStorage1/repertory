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

#include "events/types/file_upload_completed.hpp"
#include "file_manager/upload.hpp"
#include "mocks/mock_provider.hpp"
#include "utils/event_capture.hpp"

namespace repertory {
static constexpr const std::size_t test_chunk_size{1024U};

TEST(upload_test, can_upload_a_valid_file) {
  console_consumer con;

  event_system::instance().start();

  const auto source_path = test::generate_test_file_name("upload_test");

  mock_provider mock_prov;

  EXPECT_CALL(mock_prov, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 4U;
  fsi.source_path = source_path;

  event_consumer evt_com(
      file_upload_completed::name, [&fsi](const i_event &evt) {
        const auto &comp_evt = dynamic_cast<const file_upload_completed &>(evt);
        EXPECT_STREQ(fsi.api_path.c_str(), comp_evt.api_path.c_str());
        EXPECT_STREQ(fsi.source_path.c_str(), comp_evt.source_path.c_str());
        EXPECT_EQ(api_error::success, comp_evt.error);
        EXPECT_FALSE(comp_evt.cancelled);
      });

  EXPECT_CALL(mock_prov, upload_file(fsi.api_path, fsi.source_path, _))
      .WillOnce([](const std::string &, const std::string &,
                   stop_type &stop_requested) -> api_error {
        EXPECT_FALSE(stop_requested);
        return api_error::success;
      });
  upload upload(fsi, mock_prov);

  event_capture evt_cap({file_upload_completed::name});
  evt_cap.wait_for_empty();

  EXPECT_EQ(api_error::success, upload.get_api_error());
  EXPECT_FALSE(upload.is_cancelled());

  event_system::instance().stop();
}

TEST(upload_test, can_cancel_upload) {
  console_consumer con;

  event_system::instance().start();

  const auto source_path = test::generate_test_file_name("upload_test");

  mock_provider mock_provider;

  EXPECT_CALL(mock_provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 4U;
  fsi.source_path = source_path;

  event_consumer evt_con(
      file_upload_completed::name, [&fsi](const i_event &evt) {
        const auto &comp_evt = dynamic_cast<const file_upload_completed &>(evt);
        EXPECT_STREQ(fsi.api_path.c_str(), comp_evt.api_path.c_str());
        EXPECT_STREQ(fsi.source_path.c_str(), comp_evt.source_path.c_str());
        EXPECT_EQ(api_error::comm_error, comp_evt.error);
        EXPECT_TRUE(comp_evt.cancelled);
      });

  std::mutex mtx;
  std::condition_variable notify;

  EXPECT_CALL(mock_provider, upload_file(fsi.api_path, fsi.source_path, _))
      .WillOnce([&notify, &mtx](const std::string &, const std::string &,
                                stop_type &stop_requested) -> api_error {
        EXPECT_FALSE(stop_requested);

        unique_mutex_lock lock(mtx);
        notify.notify_one();
        lock.unlock();

        lock.lock();
        notify.wait(lock);
        lock.unlock();

        EXPECT_TRUE(stop_requested);

        return api_error::comm_error;
      });

  unique_mutex_lock lock(mtx);
  upload upload(fsi, mock_provider);
  notify.wait(lock);

  upload.cancel();

  notify.notify_one();
  lock.unlock();

  event_capture evt_cap({file_upload_completed::name});
  evt_cap.wait_for_empty();

  EXPECT_EQ(api_error::comm_error, upload.get_api_error());
  EXPECT_TRUE(upload.is_cancelled());

  event_system::instance().stop();
}

TEST(upload_test, can_stop_upload) {
  console_consumer con;

  event_system::instance().start();

  const auto source_path = test::generate_test_file_name("upload_test");

  mock_provider mock_provider;

  EXPECT_CALL(mock_provider, is_read_only()).WillRepeatedly(Return(false));

  filesystem_item fsi;
  fsi.api_path = "/test.txt";
  fsi.size = test_chunk_size * 4U;
  fsi.source_path = source_path;

  event_consumer evt_con(
      file_upload_completed::name, [&fsi](const i_event &evt) {
        const auto &evt_com = dynamic_cast<const file_upload_completed &>(evt);
        EXPECT_STREQ(fsi.api_path.c_str(), evt_com.api_path.c_str());
        EXPECT_STREQ(fsi.source_path.c_str(), evt_com.source_path.c_str());
        EXPECT_EQ(api_error::comm_error, evt_com.error);
        EXPECT_FALSE(evt_com.cancelled);
      });

  EXPECT_CALL(mock_provider, upload_file(fsi.api_path, fsi.source_path, _))
      .WillOnce([](const std::string &, const std::string &,
                   stop_type &stop_requested) -> api_error {
        std::this_thread::sleep_for(3s);
        EXPECT_TRUE(stop_requested);
        return api_error::comm_error;
      });

  event_capture evt_cap({file_upload_completed::name});

  {
    upload upload(fsi, mock_provider);
  }

  evt_cap.wait_for_empty();

  event_system::instance().stop();
}
} // namespace repertory
