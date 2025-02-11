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
#if defined(_WIN32)

//
// Implemented test cases based on WinFsp tests:
// https://github.com/winfsp/winfsp/blob/v2.0/tst/winfsp-tests
//
#include "fixtures/winfsp_fixture.hpp"

namespace repertory {
TYPED_TEST_CASE(winfsp_test, winfsp_provider_types);

TYPED_TEST(winfsp_test, cr8_nl_can_create_file_of_max_component_length) {
  if (this->current_provider == provider_type::s3) {
    return;
  }

  DWORD max_length{};
  EXPECT_TRUE(::GetVolumeInformationA(this->mount_location.c_str(), nullptr, 0,
                                      nullptr, &max_length, nullptr, nullptr,
                                      0));
  EXPECT_EQ(255U, max_length);

  auto file_path = utils::path::combine(this->mount_location,
                                        {
                                            std::string(max_length, 'a'),
                                        });

  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  EXPECT_TRUE(::CloseHandle(handle));
}

TYPED_TEST(winfsp_test,
           cr8_nl_can_not_create_file_greater_than_max_component_length) {
  if (this->current_provider == provider_type::s3) {
    return;
  }

  DWORD max_length{};
  EXPECT_TRUE(::GetVolumeInformationA(this->mount_location.c_str(), nullptr, 0,
                                      nullptr, &max_length, nullptr, nullptr,
                                      0));
  EXPECT_EQ(255U, max_length);

  auto file_path = utils::path::combine(this->mount_location,
                                        {
                                            std::string(max_length + 1U, 'a'),
                                        });

  auto handle =
      ::CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_EQ(INVALID_HANDLE_VALUE, handle);
  EXPECT_EQ(ERROR_INVALID_NAME, ::GetLastError());
}
} // namespace repertory

#endif // defined(_WIN32)
