/*
 Copyright <2018-2023> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

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

#include "types/repertory.hpp"
#include "utils/encryption.hpp"
#include "utils/file_utils.hpp"

namespace repertory {
static const std::string buffer = "cow moose dog chicken";
static const std::string token = "moose";

static void test_encrypted_result(const data_buffer &result) {
  EXPECT_EQ(buffer.size() +
                utils::encryption::encrypting_reader::get_header_size(),
            result.size());
  std::string data;
  EXPECT_TRUE(utils::encryption::decrypt_data(token, result, data));
  EXPECT_EQ(buffer.size(), data.size());
  EXPECT_STREQ(buffer.c_str(), data.c_str());
}

TEST(encryption, generate_key) {
  const auto key = utils::encryption::generate_key(token);
  const auto str = utils::to_hex_string(key);
  EXPECT_STREQ(
      "182072537ada59e4d6b18034a80302ebae935f66adbdf0f271d3d36309c2d481",
      str.c_str());
}

TEST(encryption, encrypt_data_buffer) {
  data_buffer result;
  utils::encryption::encrypt_data(token, buffer, result);
  test_encrypted_result(result);
}

TEST(encryption, encrypt_data_buffer_with_key) {
  const auto key = utils::encryption::generate_key(token);
  data_buffer result;
  utils::encryption::encrypt_data(key, buffer, result);
  test_encrypted_result(result);
}

TEST(encryption, encrypt_data_pointer) {
  data_buffer result;
  utils::encryption::encrypt_data(token, &buffer[0u], buffer.size(), result);
  test_encrypted_result(result);
}

TEST(encryption, encrypt_data_pointer_with_key) {
  const auto key = utils::encryption::generate_key(token);
  data_buffer result;
  utils::encryption::encrypt_data(key, &buffer[0u], buffer.size(), result);
  test_encrypted_result(result);
}

TEST(encryption, decrypt_data_pointer) {
  const auto key = utils::encryption::generate_key(token);
  data_buffer result;
  utils::encryption::encrypt_data(key, &buffer[0u], buffer.size(), result);

  std::string data;
  EXPECT_TRUE(
      utils::encryption::decrypt_data(token, &result[0u], result.size(), data));

  EXPECT_EQ(buffer.size(), data.size());
  EXPECT_STREQ(buffer.c_str(), data.c_str());
}

TEST(encryption, decrypt_data_buffer_with_key) {
  const auto key = utils::encryption::generate_key(token);
  data_buffer result;
  utils::encryption::encrypt_data(key, &buffer[0u], buffer.size(), result);

  std::string data;
  EXPECT_TRUE(utils::encryption::decrypt_data(key, result, data));

  EXPECT_EQ(buffer.size(), data.size());
  EXPECT_STREQ(buffer.c_str(), data.c_str());
}

TEST(encryption, decrypt_data_pointer_with_key) {
  const auto key = utils::encryption::generate_key(token);
  data_buffer result;
  utils::encryption::encrypt_data(key, &buffer[0u], buffer.size(), result);

  std::string data;
  EXPECT_TRUE(
      utils::encryption::decrypt_data(key, &result[0u], result.size(), data));

  EXPECT_EQ(buffer.size(), data.size());
  EXPECT_STREQ(buffer.c_str(), data.c_str());
}

TEST(encryption, decryption_failure) {
  const auto key = utils::encryption::generate_key(token);
  data_buffer result;
  utils::encryption::encrypt_data(key, &buffer[0u], buffer.size(), result);
  result[0u] = 0u;
  result[1u] = 1u;
  result[2u] = 2u;

  std::string data;
  EXPECT_FALSE(utils::encryption::decrypt_data(key, result, data));
}
} // namespace repertory
