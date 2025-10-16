/*
 Copyright <2018-2025> <scott.e.graves@protonmail.com>

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
#include "test.hpp"

#if defined(PROJECT_ENABLE_LIBSODIUM)

namespace {
#if defined(PROJECT_ENABLE_BOOST)
const std::string buffer = "cow moose dog chicken";
#endif // defined(PROJECT_ENABLE_BOOST)

const auto get_stop_requested = []() -> bool { return false; };
} // namespace

namespace repertory {
static constexpr std::string_view token{"moose"};
static constexpr std::wstring_view token_w{L"moose"};

TEST(utils_encryption, generate_key) {
  auto key1 = utils::encryption::generate_key<utils::hash::hash_256_t>(token);
  EXPECT_STREQ(
      "ab4a0b004e824962913f7c0f79582b6ec7a3b8726426ca61d1a0a28ce5049e96",
      utils::collection::to_hex_string(key1).c_str());

  auto key2 = utils::encryption::generate_key<utils::hash::hash_256_t>("moose");
  auto key3 = utils::encryption::generate_key<utils::hash::hash_256_t>("moose");
  EXPECT_EQ(key2, key3);

  auto key4 =
      utils::encryption::generate_key<utils::hash::hash_256_t>("moose2");
  EXPECT_NE(key2, key4);

  auto key1_w =
      utils::encryption::generate_key<utils::hash::hash_256_t>(token_w);
  EXPECT_NE(key1, key1_w);
#if defined(_WIN32)
  EXPECT_STREQ(
      L"4f5eb2a2ab34e3777b230465283923080b9ba59311e74058ccd74185131d11fe",
      utils::collection::to_hex_wstring(key1_w).c_str());
#else // !defined(_WIN32)
  EXPECT_STREQ(
      L"0392d95ed3eee9772fbb9af68fedf829a8eb0adbe8575d9691cc9a752196766a",
      utils::collection::to_hex_wstring(key1_w).c_str());
#endif

  auto key2_w =
      utils::encryption::generate_key<utils::hash::hash_256_t>(L"moose");
  auto key3_w =
      utils::encryption::generate_key<utils::hash::hash_256_t>(L"moose");
  EXPECT_EQ(key2_w, key3_w);
  EXPECT_NE(key2_w, key2);
  EXPECT_NE(key3_w, key3);

  auto key4_w =
      utils::encryption::generate_key<utils::hash::hash_256_t>(L"moose2");
  EXPECT_NE(key2_w, key4_w);
  EXPECT_NE(key4_w, key4);
}

TEST(utils_encryption, generate_key_default_hasher_is_blake2b_256) {
  auto key1 = utils::encryption::generate_key<utils::hash::hash_256_t>(token);
  auto key2 = utils::encryption::generate_key<utils::hash::hash_256_t>(
      token, [](auto &&data, auto &&size) -> auto {
        return utils::hash::create_hash_blake2b_256(
            std::string_view(reinterpret_cast<const char *>(data), size));
      });
  EXPECT_EQ(key1, key2);

  auto key1_w =
      utils::encryption::generate_key<utils::hash::hash_256_t>(token_w);
  auto key2_w = utils::encryption::generate_key<utils::hash::hash_256_t>(
      token_w, [](auto &&data, auto &&size) -> auto {
        return utils::hash::create_hash_blake2b_256(std::wstring_view(
            reinterpret_cast<const wchar_t *>(data), size / sizeof(wchar_t)));
      });
  EXPECT_EQ(key1_w, key2_w);

  EXPECT_NE(key1_w, key1);
  EXPECT_NE(key2_w, key2);
}

TEST(utils_encryption, generate_key_with_hasher) {
  auto key1 = utils::encryption::generate_key<utils::hash::hash_256_t>(
      token, utils::hash::blake2b_256_hasher);
  EXPECT_STREQ(
      "ab4a0b004e824962913f7c0f79582b6ec7a3b8726426ca61d1a0a28ce5049e96",
      utils::collection::to_hex_string(key1).c_str());

  auto key2 = utils::encryption::generate_key<utils::hash::hash_256_t>(
      token, utils::hash::sha256_hasher);
  EXPECT_NE(key1, key2);

  EXPECT_STREQ(
      "182072537ada59e4d6b18034a80302ebae935f66adbdf0f271d3d36309c2d481",
      utils::collection::to_hex_string(key2).c_str());

  auto key1_w = utils::encryption::generate_key<utils::hash::hash_256_t>(
      token_w, utils::hash::blake2b_256_hasher);
#if defined(_WIN32)
  EXPECT_STREQ(
      L"4f5eb2a2ab34e3777b230465283923080b9ba59311e74058ccd74185131d11fe",
      utils::collection::to_hex_wstring(key1_w).c_str());
#else // !defined(_WIN32)
  EXPECT_STREQ(
      L"0392d95ed3eee9772fbb9af68fedf829a8eb0adbe8575d9691cc9a752196766a",
      utils::collection::to_hex_wstring(key1_w).c_str());
#endif

  auto key2_w = utils::encryption::generate_key<utils::hash::hash_256_t>(
      token_w, utils::hash::sha256_hasher);
  EXPECT_NE(key1_w, key2_w);

#if defined(_WIN32)
  EXPECT_STREQ(
      L"918e4c6d39bb373f139b5fac8ec0548a9770da399b2835608974ffeac7fab6c4",
      utils::collection::to_hex_wstring(key2_w).c_str());
#else // !defined(_WIN32)
  EXPECT_STREQ(
      L"590ac70125bec4501172937f6a2cbdeb22a87b5e40d5595eccd06b2b20548d8f",
      utils::collection::to_hex_wstring(key2_w).c_str());
#endif

  EXPECT_NE(key1_w, key1);
  EXPECT_NE(key2_w, key2);
}

#if defined(PROJECT_ENABLE_BOOST)
TEST(utils_encryption, generate_argon2id_key) {
  utils::encryption::kdf_config cfg;

  {
    auto key1 =
        utils::encryption::generate_key<utils::hash::hash_256_t>(token, cfg);

    auto key2 =
        utils::encryption::generate_key<utils::hash::hash_256_t>(token, cfg);
    EXPECT_NE(key1, key2);

    auto key3 =
        utils::encryption::generate_key<utils::hash::hash_256_t>(token, cfg);
    EXPECT_NE(key3, key1);

    auto key4 =
        utils::encryption::generate_key<utils::hash::hash_256_t>(token, cfg);
    EXPECT_NE(key4, key2);

    EXPECT_NE(key3, key4);
  }

  {
    auto key1 =
        utils::encryption::generate_key<utils::hash::hash_256_t>(token_w, cfg);

    auto key2 =
        utils::encryption::generate_key<utils::hash::hash_256_t>(token_w, cfg);
    EXPECT_NE(key1, key2);

    auto key3 =
        utils::encryption::generate_key<utils::hash::hash_256_t>(token_w, cfg);
    EXPECT_NE(key3, key1);

    auto key4 =
        utils::encryption::generate_key<utils::hash::hash_256_t>(token_w, cfg);
    EXPECT_NE(key4, key2);

    EXPECT_NE(key3, key4);
  }
}

TEST(utils_encryption, recreate_argon2id_key) {
  utils::encryption::kdf_config cfg;

  {
    auto key1 =
        utils::encryption::generate_key<utils::hash::hash_256_t>(token, cfg);

    auto key2 =
        utils::encryption::recreate_key<utils::hash::hash_256_t>(token, cfg);
    EXPECT_EQ(key1, key2);
  }

  {
    auto key1 =
        utils::encryption::generate_key<utils::hash::hash_256_t>(token_w, cfg);

    auto key2 =
        utils::encryption::recreate_key<utils::hash::hash_256_t>(token_w, cfg);
    EXPECT_EQ(key1, key2);
  }
}

static void test_encrypted_result(const data_buffer &result) {
  EXPECT_EQ(buffer.size() + utils::encryption::encryption_header_size,
            result.size());
  std::string data;
  EXPECT_TRUE(utils::encryption::decrypt_data(token, result, data));
  EXPECT_EQ(buffer.size(), data.size());
  EXPECT_STREQ(buffer.c_str(), data.c_str());
}

static void
test_encrypted_result_using_argon2id(const data_buffer &result,
                                     const utils::encryption::kdf_config &cfg) {
  EXPECT_EQ(buffer.size() + utils::encryption::encryption_header_size,
            result.size());
  std::string data;
  EXPECT_TRUE(utils::encryption::decrypt_data(token, cfg, result, data));
  EXPECT_EQ(buffer.size(), data.size());
  EXPECT_STREQ(buffer.c_str(), data.c_str());
}

TEST(utils_encryption, encrypt_data_buffer) {
  data_buffer result;
  utils::encryption::encrypt_data(token, buffer, result);
  test_encrypted_result(result);
}

TEST(utils_encryption, encrypt_data_buffer_with_key) {
  const auto key =
      utils::encryption::generate_key<utils::hash::hash_256_t>(token);
  data_buffer result;
  utils::encryption::encrypt_data(key, buffer, result);
  test_encrypted_result(result);
}

TEST(utils_encryption, encrypt_data_pointer) {
  data_buffer result;
  utils::encryption::encrypt_data(
      token, reinterpret_cast<const unsigned char *>(buffer.data()),
      buffer.size(), result);
  test_encrypted_result(result);
}

TEST(utils_encryption, encrypt_data_pointer_with_key) {
  const auto key =
      utils::encryption::generate_key<utils::hash::hash_256_t>(token);
  data_buffer result;
  utils::encryption::encrypt_data(
      key, reinterpret_cast<const unsigned char *>(buffer.data()),
      buffer.size(), result);
  test_encrypted_result(result);
}

TEST(utils_encryption, decrypt_data_pointer) {
  const auto key =
      utils::encryption::generate_key<utils::hash::hash_256_t>(token);
  data_buffer result;
  utils::encryption::encrypt_data(
      key, reinterpret_cast<const unsigned char *>(buffer.data()),
      buffer.size(), result);

  std::string data;
  EXPECT_TRUE(utils::encryption::decrypt_data(token, result.data(),
                                              result.size(), data));

  EXPECT_EQ(buffer.size(), data.size());
  EXPECT_STREQ(buffer.c_str(), data.c_str());
}

TEST(utils_encryption, decrypt_data_buffer_with_key) {
  const auto key =
      utils::encryption::generate_key<utils::hash::hash_256_t>(token);
  data_buffer result;
  utils::encryption::encrypt_data(
      key, reinterpret_cast<const unsigned char *>(buffer.data()),
      buffer.size(), result);

  std::string data;
  EXPECT_TRUE(utils::encryption::decrypt_data(key, result, data));

  EXPECT_EQ(buffer.size(), data.size());
  EXPECT_STREQ(buffer.c_str(), data.c_str());
}

TEST(utils_encryption, decrypt_data_pointer_with_key) {
  const auto key =
      utils::encryption::generate_key<utils::hash::hash_256_t>(token);
  data_buffer result;
  utils::encryption::encrypt_data(
      key, reinterpret_cast<const unsigned char *>(buffer.data()),
      buffer.size(), result);

  std::string data;
  EXPECT_TRUE(
      utils::encryption::decrypt_data(key, result.data(), result.size(), data));

  EXPECT_EQ(buffer.size(), data.size());
  EXPECT_STREQ(buffer.c_str(), data.c_str());
}

TEST(utils_encryption, decryption_failure) {
  const auto key =
      utils::encryption::generate_key<utils::hash::hash_256_t>(token);
  data_buffer result;
  utils::encryption::encrypt_data(
      key, reinterpret_cast<const unsigned char *>(buffer.data()),
      buffer.size(), result);
  result[0U] = 0U;
  result[1U] = 1U;
  result[2U] = 2U;

  std::string data;
  EXPECT_FALSE(utils::encryption::decrypt_data(key, result, data));
}

TEST(utils_encryption, decrypt_file_name) {
  auto &source_file = test::create_random_file(
      8U * utils::encryption::encrypting_reader::get_data_chunk_size());
  EXPECT_TRUE(source_file);
  if (source_file) {
    utils::encryption::encrypting_reader reader(
        "test.dat", source_file.get_path(), get_stop_requested, token,
        std::nullopt);

    auto file_name = reader.get_encrypted_file_name();

    EXPECT_EQ(true, utils::encryption::decrypt_file_name(token, file_name));
    EXPECT_STREQ("test.dat", file_name.c_str());
  }
}

TEST(utils_encryption, decrypt_file_path) {
  auto &source_file = test::create_random_file(
      8U * utils::encryption::encrypting_reader::get_data_chunk_size());
  EXPECT_TRUE(source_file);
  if (source_file) {
    utils::encryption::encrypting_reader reader(
        "test.dat", source_file.get_path(), get_stop_requested, token,
        "moose/cow");

    auto file_path = reader.get_encrypted_file_path();

    EXPECT_EQ(true, utils::encryption::decrypt_file_path(token, file_path));
    EXPECT_STREQ("/moose/cow/test.dat", file_path.c_str());
  }
}

TEST(utils_encryption, encrypt_data_buffer_using_argon2id) {
  utils::encryption::kdf_config cfg;

  data_buffer result;
  utils::encryption::encrypt_data(token, cfg, buffer, result);
  test_encrypted_result_using_argon2id(result, cfg);
}

TEST(utils_encryption, encrypt_data_pointer_using_argon2id) {
  utils::encryption::kdf_config cfg;

  data_buffer result;
  utils::encryption::encrypt_data(
      token, cfg, reinterpret_cast<const unsigned char *>(buffer.data()),
      buffer.size(), result);
  test_encrypted_result_using_argon2id(result, cfg);
}

// TEST(utils_encryption, decrypt_file_name_using_argon2id) {}

// TEST(utils_encryption, decrypt_file_path_using_argon2id) {}
//
// TEST(utils_encryption, decrypt_file_name_using_argon2id_master_key) {}

// TEST(utils_encryption, decrypt_file_path_using_argon2id_master_key) {}
#endif // defined(PROJECT_ENABLE_BOOST)
} // namespace repertory

#endif // defined(PROJECT_ENABLE_LIBSODIUM)
