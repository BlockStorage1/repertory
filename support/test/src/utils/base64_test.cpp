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
#include "test.hpp"

using macaron::Base64::Decode;
using macaron::Base64::Encode;
using macaron::Base64::EncodeUrlSafe;

namespace {
[[nodiscard]] auto decode_to_string(std::string_view str) -> std::string {
  auto vec = Decode(str);
  return {vec.begin(), vec.end()};
}

[[nodiscard]] auto standard_to_url_safe(std::string str, bool keep_padding)
    -> std::string {
  for (auto &cur_ch : str) {
    if (cur_ch == '+') {
      cur_ch = '-';
    } else if (cur_ch == '/') {
      cur_ch = '_';
    }
  }
  if (not keep_padding) {
    while (not str.empty() && str.back() == '=') {
      str.pop_back();
    }
  }
  return str;
}
} // namespace

TEST(utils_base64, rfc4648_known_vectors_standard_padded) {
  struct vec_case {
    std::string_view in;
    std::string_view b64;
  };
  const std::array<vec_case, 7> vectors{{
      {"", ""},
      {"f", "Zg=="},
      {"fo", "Zm8="},
      {"foo", "Zm9v"},
      {"foob", "Zm9vYg=="},
      {"fooba", "Zm9vYmE="},
      {"foobar", "Zm9vYmFy"},
  }};

  for (const auto &vec_entry : vectors) {
    const auto enc_str =
        Encode(reinterpret_cast<const unsigned char *>(vec_entry.in.data()),
               vec_entry.in.size(), /*url_safe=*/false, /*pad=*/true);
    EXPECT_EQ(enc_str, vec_entry.b64);

    const auto dec_vec = Decode(vec_entry.b64);
    EXPECT_EQ(std::string(dec_vec.begin(), dec_vec.end()), vec_entry.in);
  }
}

TEST(utils_base64, url_safe_padded_and_unpadded_match_transformed_standard) {
  const std::string payload =
      std::string("This+/needs/URL-safe mapping and padding checks.") +
      std::string("\x00\x01\xFE\xFF", 4);

  const auto std_padded =
      Encode(reinterpret_cast<const unsigned char *>(payload.data()),
             payload.size(), /*url_safe=*/false, /*pad=*/true);
  const auto url_padded =
      Encode(reinterpret_cast<const unsigned char *>(payload.data()),
             payload.size(), /*url_safe=*/true, /*pad=*/true);
  const auto url_unpadded =
      Encode(reinterpret_cast<const unsigned char *>(payload.data()),
             payload.size(), /*url_safe=*/true, /*pad=*/false);

  const auto url_from_std_padded =
      standard_to_url_safe(std_padded, /*keep_padding=*/true);
  const auto url_from_std_unpadded =
      standard_to_url_safe(std_padded, /*keep_padding=*/false);

  EXPECT_EQ(url_padded, url_from_std_padded);
  EXPECT_EQ(url_unpadded, url_from_std_unpadded);

  const auto dec_one = Decode(url_padded);
  const auto dec_two = Decode(url_unpadded);
  EXPECT_EQ(std::string(dec_one.begin(), dec_one.end()), payload);
  EXPECT_EQ(std::string(dec_two.begin(), dec_two.end()), payload);
}

TEST(utils_base64, empty_input) {
  const std::string empty_str;
  const auto enc_empty_std =
      Encode(reinterpret_cast<const unsigned char *>(empty_str.data()),
             empty_str.size(), /*url_safe=*/false, /*pad=*/true);
  const auto enc_empty_url =
      Encode(reinterpret_cast<const unsigned char *>(empty_str.data()),
             empty_str.size(), /*url_safe=*/true, /*pad=*/false);
  EXPECT_TRUE(enc_empty_std.empty());
  EXPECT_TRUE(enc_empty_url.empty());

  const auto dec_empty = Decode("");
  EXPECT_TRUE(dec_empty.empty());
}

TEST(utils_base64, remainder_boundaries_round_trip) {
  const std::string str_one = "A";     // rem 1
  const std::string str_two = "AB";    // rem 2
  const std::string str_thr = "ABC";   // rem 0
  const std::string str_fou = "ABCD";  // rem 1 after blocks
  const std::string str_fiv = "ABCDE"; // rem 2 after blocks

  for (const auto *str_ptr :
       {&str_one, &str_two, &str_thr, &str_fou, &str_fiv}) {
    const auto enc_std =
        Encode(reinterpret_cast<const unsigned char *>(str_ptr->data()),
               str_ptr->size(), false, true);
    const auto dec_std = Decode(enc_std);
    EXPECT_EQ(std::string(dec_std.begin(), dec_std.end()), *str_ptr);

    const auto enc_url_pad =
        Encode(reinterpret_cast<const unsigned char *>(str_ptr->data()),
               str_ptr->size(), true, true);
    const auto dec_url_pad = Decode(enc_url_pad);
    EXPECT_EQ(std::string(dec_url_pad.begin(), dec_url_pad.end()), *str_ptr);

    const auto enc_url_nopad =
        Encode(reinterpret_cast<const unsigned char *>(str_ptr->data()),
               str_ptr->size(), true, false);
    const auto dec_url_nopad = Decode(enc_url_nopad);
    EXPECT_EQ(std::string(dec_url_nopad.begin(), dec_url_nopad.end()),
              *str_ptr);
  }
}

TEST(utils_base64, decode_accepts_standard_and_url_safe_forms) {
  const std::string input_str = "Man is distinguished, not only by his reason.";
  const auto std_padded =
      Encode(reinterpret_cast<const unsigned char *>(input_str.data()),
             input_str.size(), false, true);
  const auto url_padded =
      Encode(reinterpret_cast<const unsigned char *>(input_str.data()),
             input_str.size(), true, true);
  const auto url_unpadded =
      Encode(reinterpret_cast<const unsigned char *>(input_str.data()),
             input_str.size(), true, false);

  const auto dec_std = Decode(std_padded);
  const auto dec_url_pad = Decode(url_padded);
  const auto dec_url_nopad = Decode(url_unpadded);

  EXPECT_EQ(std::string(dec_std.begin(), dec_std.end()), input_str);
  EXPECT_EQ(std::string(dec_url_pad.begin(), dec_url_pad.end()), input_str);
  EXPECT_EQ(std::string(dec_url_nopad.begin(), dec_url_nopad.end()), input_str);
}

TEST(utils_base64, all_byte_values_round_trip) {
  std::vector<unsigned char> byte_vec(256);
  for (size_t idx = 0; idx < byte_vec.size(); ++idx) {
    byte_vec[idx] = static_cast<unsigned char>(idx);
  }

  const auto enc_std = Encode(byte_vec.data(), byte_vec.size(), false, true);
  const auto dec_std = Decode(enc_std);
  ASSERT_EQ(dec_std.size(), byte_vec.size());
  EXPECT_TRUE(std::equal(dec_std.begin(), dec_std.end(), byte_vec.begin()));

  const auto enc_url = Encode(byte_vec.data(), byte_vec.size(), true, false);
  const auto dec_url = Decode(enc_url);
  ASSERT_EQ(dec_url.size(), byte_vec.size());
  EXPECT_TRUE(std::equal(dec_url.begin(), dec_url.end(), byte_vec.begin()));
}

TEST(utils_base64, wrapper_encode_url_safe_equals_flagged_encode) {
  const std::string data_str = "wrap me!";
  const auto enc_wrap_a =
      EncodeUrlSafe(reinterpret_cast<const unsigned char *>(data_str.data()),
                    data_str.size(), /*pad=*/false);
  const auto enc_wrap_b =
      Encode(reinterpret_cast<const unsigned char *>(data_str.data()),
             data_str.size(), /*url_safe=*/true, /*pad=*/false);
  EXPECT_EQ(enc_wrap_a, enc_wrap_b);

  const auto enc_wrap_a2 = EncodeUrlSafe(data_str, /*pad=*/true);
  const auto enc_wrap_b2 = Encode(data_str, /*url_safe=*/true, /*pad=*/true);
  EXPECT_EQ(enc_wrap_a2, enc_wrap_b2);
}

TEST(utils_base64, unpadded_length_rules) {
  const auto enc_one = Encode("f", /*url_safe=*/true, /*pad=*/false);
  const auto enc_two = Encode("fo", /*url_safe=*/true, /*pad=*/false);
  const auto enc_thr = Encode("foo", /*url_safe=*/true, /*pad=*/false);
  EXPECT_EQ(enc_one.size(), 2U);
  EXPECT_EQ(enc_two.size(), 3U);
  EXPECT_EQ(enc_thr.size(), 4U);

  EXPECT_EQ(Decode(enc_one), std::vector<unsigned char>({'f'}));
  EXPECT_EQ(Decode(enc_two), std::vector<unsigned char>({'f', 'o'}));
  EXPECT_EQ(Decode(enc_thr), std::vector<unsigned char>({'f', 'o', 'o'}));
}

TEST(utils_base64, errors_length_mod4_eq_1) {
  EXPECT_THROW(Decode("A"), std::runtime_error);
  EXPECT_THROW(Decode("AAAAA"), std::runtime_error);
}

TEST(utils_base64, errors_invalid_characters) {
  EXPECT_THROW(Decode("Zm9v YmFy"), std::runtime_error);
  EXPECT_THROW(Decode("Zm9v*YmFy"), std::runtime_error);
  EXPECT_THROW(Decode("Z=g="), std::runtime_error);
}

TEST(utils_base64, reject_whitespace_and_controls) {
  // newline, tab, and space should be rejected (decoder does not skip
  // whitespace)
  EXPECT_THROW(Decode("Zg==\n"), std::runtime_error);
  EXPECT_THROW(Decode("Zg==\t"), std::runtime_error);
  EXPECT_THROW(Decode("Z g=="), std::runtime_error);
}

TEST(utils_base64, reject_padding_in_nonfinal_quartet) {
  // '=' cannot appear before the final quartet
  EXPECT_THROW(Decode("AAA=AAAA"), std::runtime_error);
  EXPECT_THROW(Decode("Zg==Zg=="), std::runtime_error);
}

TEST(utils_base64, reject_padding_in_first_two_slots_of_final_quartet) {
  // '=' only allowed in slots 3 and/or 4 of the final quartet
  EXPECT_THROW(Decode("=AAA"), std::runtime_error);
  EXPECT_THROW(Decode("A=AA"), std::runtime_error);
  EXPECT_THROW(
      Decode("Z=g="),
      std::runtime_error); // already in your suite, kept for completeness
}

TEST(utils_base64, reject_incorrect_padding_count_for_length) {
  // "f" must be "Zg==" (two '='). One '=' is invalid.
  EXPECT_THROW(Decode("Zg="), std::runtime_error);

  // "foo" must be unpadded ("Zm9v"). Extra '=' is invalid.
  EXPECT_THROW(Decode("Zm9v="), std::runtime_error);

  // "fo" must have exactly one '=' -> "Zm8="
  // Correct cases:
  EXPECT_NO_THROW(Decode("Zm8="));
  EXPECT_NO_THROW(Decode("Zm9v"));
}

TEST(utils_base64, accept_unpadded_equivalents_when_legal) {
  EXPECT_EQ(decode_to_string("Zg"), "f");
  EXPECT_EQ(decode_to_string("Zm8"), "fo");
  EXPECT_EQ(decode_to_string("Zm9v"), "foo");
  EXPECT_EQ(decode_to_string("Zm9vYmE"), "fooba");
}

TEST(utils_base64, mixed_alphabet_is_accepted) {
  const std::string input_str = "any+/mix_/of+chars/";
  const auto std_padded =
      Encode(reinterpret_cast<const unsigned char *>(input_str.data()),
             input_str.size(), /*url_safe=*/false, /*pad=*/true);

  std::string mixed = std_padded;
  for (char &cur_ch : mixed) {
    if (cur_ch == '+') {
      cur_ch = '-';
    } else if (cur_ch == '/') {
      cur_ch = '_';
    }
  }

  EXPECT_EQ(decode_to_string(mixed), input_str);
}

TEST(utils_base64, invalid_non_ascii_octets_in_input) {
  // Extended bytes like 0xFF are not valid Base64 characters
  std::string bad = "Zg==";
  bad[1] = static_cast<char>(0xFF);
  EXPECT_THROW(Decode(bad), std::runtime_error);
}

TEST(utils_base64, large_buffer_round_trip_and_sizes) {
  // Deterministic pseudo-random buffer
  const std::size_t byte_len = 1 << 20; // 1 MiB
  std::vector<unsigned char> data_vec(byte_len);
  unsigned int val = 0x12345678U;
  for (unsigned char &idx : data_vec) {
    val ^= val << 13;
    val ^= val >> 17;
    val ^= val << 5; // xorshift32
    idx = static_cast<unsigned char>(val & 0xFFU);
  }

  // Padded encode length should be 4 * ceil(N/3)
  const auto enc_pad = Encode(data_vec.data(), data_vec.size(),
                              /*url_safe=*/false, /*pad=*/true);
  const std::size_t expected_padded = 4U * ((byte_len + 2U) / 3U);
  EXPECT_EQ(enc_pad.size(), expected_padded);

  // Unpadded encode length rule (RFC 4648 §5)
  const auto enc_nopad = Encode(data_vec.data(), data_vec.size(),
                                /*url_safe=*/true, /*pad=*/false);
  const std::size_t rem = byte_len % 3U;
  const std::size_t expected_unpadded =
      4U * (byte_len / 3U) + (rem == 0U ? 0U : (rem == 1U ? 2U : 3U));
  EXPECT_EQ(enc_nopad.size(), expected_unpadded);

  // Round-trips
  const auto dec_pad = Decode(enc_pad);
  const auto dec_nopad = Decode(enc_nopad);
  ASSERT_EQ(dec_pad.size(), data_vec.size());
  ASSERT_EQ(dec_nopad.size(), data_vec.size());
  EXPECT_TRUE(std::equal(dec_pad.begin(), dec_pad.end(), data_vec.begin()));
  EXPECT_TRUE(std::equal(dec_nopad.begin(), dec_nopad.end(), data_vec.begin()));
}

TEST(utils_base64, url_safe_round_trip_various_lengths) {
  for (std::size_t len : {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 32U, 33U, 64U, 65U}) {
    std::vector<unsigned char> buf(len);
    for (std::size_t i = 0; i < len; ++i) {
      buf[i] = static_cast<unsigned char>(i * 13U + 7U);
    }

    const auto enc_unpadded =
        Encode(buf.data(), buf.size(), /*url_safe=*/true, /*pad=*/false);
    const auto enc_padded =
        Encode(buf.data(), buf.size(), /*url_safe=*/true, /*pad=*/true);

    const auto dec_unpadded = Decode(enc_unpadded);
    const auto dec_padded = Decode(enc_padded);

    ASSERT_EQ(dec_unpadded.size(), buf.size());
    ASSERT_EQ(dec_padded.size(), buf.size());
    EXPECT_TRUE(
        std::equal(dec_unpadded.begin(), dec_unpadded.end(), buf.begin()));
    EXPECT_TRUE(std::equal(dec_padded.begin(), dec_padded.end(), buf.begin()));
  }
}

TEST(utils_base64, reject_trailing_garbage_after_padding) {
  // Anything after final '=' padding is invalid
  EXPECT_THROW(Decode("Zg==A"), std::runtime_error);
  EXPECT_THROW(Decode("Zm8=A"), std::runtime_error);
}

TEST(utils_base64, reject_three_padding_chars_total) {
  // Any string with total length %4==1 is invalid (e.g., "Zg===")
  EXPECT_THROW(Decode("Zg==="), std::runtime_error);
}

TEST(utils_base64, standard_vs_url_safe_encoding_equivalence) {
  const std::string msg = "base64 / url-safe + cross-check";

  const auto std_enc =
      Encode(reinterpret_cast<const unsigned char *>(msg.data()), msg.size(),
             /*url_safe=*/false, /*pad=*/true);
  const auto url_enc =
      Encode(reinterpret_cast<const unsigned char *>(msg.data()), msg.size(),
             /*url_safe=*/true, /*pad=*/true);

  std::string transformed = std_enc;
  for (char &cur_ch : transformed) {
    if (cur_ch == '+') {
      cur_ch = '-';
    } else if (cur_ch == '/') {
      cur_ch = '_';
    }
  }

  EXPECT_EQ(url_enc, transformed);

  // decode once, then construct
  EXPECT_EQ(decode_to_string(url_enc), msg);
}
