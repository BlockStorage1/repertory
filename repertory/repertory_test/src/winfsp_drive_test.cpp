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
#if defined(_WIN32)

// TODO revisit create_allocation
// TODO revisit create_backup
// TODO revisit create_notraverse
// TODO revisit create_related
// TODO revisit create_restore
// TODO revisit create_sd
// TODO revisit create_share
// TODO revisit delete_access_test
// TODO revisit delete_ex_test
// TODO revisit getfileattr_test
// TODO revisit query_winfsp_test
// TODO revisit rename_backslash_test
// TODO revisit rename_caseins_test
// TODO revisit rename_ex_test
// TODO revisit rename_flipflop_test
// TODO revisit rename_mmap_test
// TODO revisit rename_open_test
// TODO revisit rename_pid_test
// TODO revisit rename_standby_test
// TODO revisit setvolinfo_test

//
// Implemented test cases based on WinFsp tests:
// https://github.com/winfsp/winfsp/blob/v2.0/tst/winfsp-tests
//
#include "fixtures/winfsp_fixture.hpp"

namespace repertory {
TYPED_TEST_CASE(winfsp_test, winfsp_provider_types);

TYPED_TEST(winfsp_test, can_set_current_directory_to_mount_location) {
  EXPECT_TRUE(::SetCurrentDirectoryA(this->mount_location.c_str()));

  EXPECT_TRUE(::SetCurrentDirectoryA(this->current_directory.string().c_str()));
}
} // namespace repertory

#endif // defined(_WIN32)
