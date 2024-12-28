// NOLINTBEGIN
#ifndef _MACARON_BASE64_H_
#define _MACARON_BASE64_H_

/**
 * The MIT License (MIT)
 * Copyright (c) 2016 tomykaira
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif

#include <array>
#include <string>
#include <vector>

namespace macaron::Base64 {
static std::string Encode(const unsigned char *data, std::size_t len) {
  static constexpr std::array<unsigned char, 64U> sEncodingTable{
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
      'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
      'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
      'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/',
  };

  auto in_len{len};
  std::string ret;
  if (in_len > 0) {
    std::size_t out_len{4U * ((in_len + 2U) / 3U)};
    ret = std::string(out_len, '\0');
    std::size_t i;
    auto *p = reinterpret_cast<unsigned char *>(ret.data());

    for (i = 0U; i < in_len - 2U; i += 3U) {
      *p++ = sEncodingTable[(data[i] >> 2U) & 0x3F];
      *p++ = sEncodingTable[((data[i] & 0x3) << 4U) |
                            ((int)(data[i + 1U] & 0xF0) >> 4U)];
      *p++ = sEncodingTable[((data[i + 1] & 0xF) << 2) |
                            ((int)(data[i + 2U] & 0xC0) >> 6U)];
      *p++ = sEncodingTable[data[i + 2U] & 0x3F];
    }
    if (i < in_len) {
      *p++ = sEncodingTable[(data[i] >> 2U) & 0x3F];
      if (i == (in_len - 1U)) {
        *p++ = sEncodingTable[((data[i] & 0x3) << 4U)];
        *p++ = '=';
      } else {
        *p++ = sEncodingTable[((data[i] & 0x3) << 4U) |
                              ((int)(data[i + 1U] & 0xF0) >> 4U)];
        *p++ = sEncodingTable[((data[i + 1U] & 0xF) << 2U)];
      }
      *p++ = '=';
    }
  }

  return ret;
}

[[maybe_unused]] static std::string Encode(std::string_view data) {
  return Encode(reinterpret_cast<const unsigned char *>(data.data()),
                data.size());
}

[[maybe_unused]] static std::vector<unsigned char>
Decode(std::string_view input) {
  static constexpr std::array<unsigned char, 256> kDecodingTable{
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63, 52, 53, 54, 55, 56, 57,
      58, 59, 60, 61, 64, 64, 64, 64, 64, 64, 64, 0,  1,  2,  3,  4,  5,  6,
      7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
      25, 64, 64, 64, 64, 64, 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
      37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64,
  };

  std::vector<unsigned char> out;
  if (not input.empty()) {
    auto in_len{input.size()};
    if (in_len % 4U != 0U) {
      throw std::runtime_error("Input data size is not a multiple of 4");
    }

    std::size_t out_len{in_len / 4U * 3U};
    if (input[in_len - 1U] == '=') {
      out_len--;
    }
    if (input[in_len - 2U] == '=') {
      out_len--;
    }

    out.resize(out_len);

    for (std::size_t i = 0U, j = 0U; i < in_len;) {
      std::uint32_t a =
          input.at(i) == '='
              ? 0U & i++
              : kDecodingTable[static_cast<unsigned char>(input.at(i++))];
      std::uint32_t b =
          input.at(i) == '='
              ? 0U & i++
              : kDecodingTable[static_cast<unsigned char>(input.at(i++))];
      std::uint32_t c =
          input.at(i) == '='
              ? 0U & i++
              : kDecodingTable[static_cast<unsigned char>(input.at(i++))];
      std::uint32_t d =
          input.at(i) == '='
              ? 0U & i++
              : kDecodingTable[static_cast<unsigned char>(input.at(i++))];

      std::uint32_t triple =
          (a << 3U * 6U) + (b << 2U * 6U) + (c << 1U * 6U) + (d << 0U * 6U);

      if (j < out_len)
        out[j++] = (triple >> 2U * 8U) & 0xFF;
      if (j < out_len)
        out[j++] = (triple >> 1U * 8U) & 0xFF;
      if (j < out_len)
        out[j++] = (triple >> 0U * 8U) & 0xFF;
    }
  }

  return out;
}
} // namespace macaron::Base64

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif /* _MACARON_BASE64_H_ */

// NOLINTEND
