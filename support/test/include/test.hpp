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
#ifndef REPERTORY_TEST_INCLUDE_TEST_HPP_
#define REPERTORY_TEST_INCLUDE_TEST_HPP_

#if defined(U)
#undef U
#endif // defined(U)

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;
using namespace ::testing;

#define COMMA ,

#include "utils/all.hpp"

namespace repertory::test {
[[nodiscard]] auto create_random_file(std::size_t size)
    -> utils::file::i_file &;

[[nodiscard]] auto
generate_test_file_name(std::string_view file_name_no_extension) -> std::string;

#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
template <typename buffer_t, typename result_t>
static void decrypt_and_verify(const buffer_t &buffer, std::string_view token,
                               result_t &result) {
  EXPECT_TRUE(utils::encryption::decrypt_data(token, buffer, result));
}
#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)

auto generate_test_directory() -> utils::file::i_directory &;

[[nodiscard]] auto get_test_config_dir() -> std::string;

[[nodiscard]] auto get_test_input_dir() -> std::string;

[[nodiscard]] auto get_test_output_dir() -> std::string;
} // namespace repertory::test

#endif // REPERTORY_TEST_INCLUDE_TEST_HPP_
