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
#ifndef TESTS_TEST_COMMON_HPP_
#define TESTS_TEST_COMMON_HPP_

#ifdef U
#undef U
#endif

REPERTORY_IGNORE_WARNINGS_ENABLE()
#include <gmock/gmock.h>
#include <gtest/gtest.h>
REPERTORY_IGNORE_WARNINGS_DISABLE()

#include "events/consumers/console_consumer.hpp"
#include "events/event_system.hpp"
#include "events/events.hpp"
#include "utils/encryption.hpp"
#include "utils/file_utils.hpp"
#include "utils/native_file.hpp"

#define COMMA ,

using ::testing::_;
using namespace ::testing;

namespace repertory {
[[nodiscard]] auto create_random_file(std::string path, std::size_t size)
    -> native_file_ptr;

void delete_generated_files();

[[nodiscard]] auto
generate_test_file_name(const std::string &directory,
                        const std::string &file_name_no_extension)
    -> std::string;

template <typename T, typename T2>
static void decrypt_and_verify(const T &buffer, const std::string &token,
                               T2 &result) {
  EXPECT_TRUE(utils::encryption::decrypt_data(token, buffer, result));
}

[[nodiscard]] auto get_test_dir() -> std::string;
} // namespace repertory

#endif // TESTS_TEST_COMMON_HPP_
