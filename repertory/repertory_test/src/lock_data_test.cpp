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
#include "test_common.hpp"

#include "platform/platform.hpp"

namespace repertory {
TEST(lock_data, lock_and_unlock) {
  {
    lock_data l(provider_type::sia, "1");
    EXPECT_EQ(lock_result::success, l.grab_lock());

    std::thread([]() {
      lock_data l2(provider_type::sia, "1");
      EXPECT_EQ(lock_result::locked, l2.grab_lock(10));
    }).join();
  }

  std::thread([]() {
    lock_data l(provider_type::sia, "1");
    EXPECT_EQ(lock_result::success, l.grab_lock(10));
  }).join();

#if defined(_WIN32)
  lock_data l2(provider_type::remote, "1");
  EXPECT_EQ(lock_result::success, l2.grab_lock());

  lock_data l3(provider_type::remote, "2");
  EXPECT_EQ(lock_result::success, l3.grab_lock());
#endif
}

#if defined(_WIN32)
TEST(lock_data, set_and_unset_mount_state) {
  lock_data l(provider_type::sia, "1");
  EXPECT_TRUE(l.set_mount_state(true, "C:", 99));

  lock_data l2(provider_type::remote, "1");
  EXPECT_TRUE(l2.set_mount_state(true, "D:", 97));

  lock_data l3(provider_type::remote, "2");
  EXPECT_TRUE(l3.set_mount_state(true, "E:", 96));

  json mount_state;
  EXPECT_TRUE(l.get_mount_state(mount_state));

  EXPECT_STREQ(R"({"Active":true,"Location":"C:","PID":99})",
               mount_state["Sia1"].dump().c_str());
  EXPECT_STREQ(R"({"Active":true,"Location":"D:","PID":97})",
               mount_state["Remote1"].dump().c_str());
  EXPECT_STREQ(R"({"Active":true,"Location":"E:","PID":96})",
               mount_state["Remote2"].dump().c_str());

  EXPECT_TRUE(l.set_mount_state(false, "C:", 99));
  EXPECT_TRUE(l2.set_mount_state(false, "D:", 98));
  EXPECT_TRUE(l3.set_mount_state(false, "E:", 97));

  EXPECT_TRUE(l.get_mount_state(mount_state));
  EXPECT_STREQ(R"({"Active":false,"Location":"","PID":-1})",
               mount_state["Sia1"].dump().c_str());
  EXPECT_STREQ(R"({"Active":false,"Location":"","PID":-1})",
               mount_state["Remote1"].dump().c_str());
  EXPECT_STREQ(R"({"Active":false,"Location":"","PID":-1})",
               mount_state["Remote2"].dump().c_str());
}
#else
TEST(lock_data, set_and_unset_mount_state) {
  lock_data l(provider_type::sia, "1");
  EXPECT_TRUE(l.set_mount_state(true, "/mnt/1", 99));

  json mount_state;
  EXPECT_TRUE(l.get_mount_state(mount_state));

  EXPECT_STREQ(R"({"Active":true,"Location":"/mnt/1","PID":99})",
               mount_state["Sia1"].dump().c_str());

  EXPECT_TRUE(l.set_mount_state(false, "/mnt/1", 99));

  EXPECT_TRUE(l.get_mount_state(mount_state));

  EXPECT_STREQ(R"({"Active":false,"Location":"","PID":-1})",
               mount_state["Sia1"].dump().c_str());
}
#endif
} // namespace repertory
