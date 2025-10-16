// NOLINTBEGIN
#ifndef MACARON_BASE64_H_
#define MACARON_BASE64_H_

/**
 * The MIT License (MIT)
 * Copyright (c) 2016 tomykaira
 * Copyright (c) 2025 scott.e.graves@protonmail.com
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
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace macaron::Base64 {

// --- Alphabets --------------------------------------------------------------

static constexpr std::array<unsigned char, 64U> kStdAlphabet{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/',
};

static constexpr std::array<unsigned char, 64U> kUrlAlphabet{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_',
};

// Decoding table that accepts BOTH standard and URL-safe alphabets.
static constexpr std::array<unsigned char, 256U> kDecodingTable = [] {
  std::array<unsigned char, 256U> t{};
  t.fill(64U);
  // 'A'-'Z'
  for (unsigned char c = 'A'; c <= 'Z'; ++c)
    t[c] = static_cast<unsigned char>(c - 'A');
  // 'a'-'z'
  for (unsigned char c = 'a'; c <= 'z'; ++c)
    t[c] = static_cast<unsigned char>(26 + c - 'a');
  // '0'-'9'
  for (unsigned char c = '0'; c <= '9'; ++c)
    t[c] = static_cast<unsigned char>(52 + c - '0');
  // Standard extras
  t[static_cast<unsigned char>('+')] = 62U;
  t[static_cast<unsigned char>('/')] = 63U;
  // URL-safe extras
  t[static_cast<unsigned char>('-')] = 62U;
  t[static_cast<unsigned char>('_')] = 63U;
  return t;
}();

// --- Encoding ---------------------------------------------------------------

/**
 * Encode to Base64.
 * @param data     pointer to bytes
 * @param len      number of bytes
 * @param url_safe if true, use URL-safe alphabet ("-","_") instead of ("+","/")
 * @param pad      if true, add '=' padding; if false, omit padding (RFC 4648
 * §5)
 */
static std::string Encode(const unsigned char *data, std::size_t len,
                          bool url_safe = false, bool pad = true) {
  const auto &alpha = url_safe ? kUrlAlphabet : kStdAlphabet;

  std::string out;
  if (len == 0U) {
    return out;
  }

  const std::size_t full_blocks = len / 3U;
  const std::size_t rem = len % 3U;

  std::size_t out_len{};
  if (pad) {
    out_len = 4U * ((len + 2U) / 3U);
  } else {
    // Unpadded length per RFC 4648 §5
    out_len = 4U * full_blocks + (rem == 0U ? 0U : (rem == 1U ? 2U : 3U));
  }
  out.assign(out_len, '\0');

  auto *p = reinterpret_cast<unsigned char *>(out.data());
  std::size_t i = 0;

  // Full 3-byte blocks -> 4 chars
  for (; i + 2U < len; i += 3U) {
    const unsigned char b0 = data[i + 0U];
    const unsigned char b1 = data[i + 1U];
    const unsigned char b2 = data[i + 2U];

    *p++ = alpha[(b0 >> 2U) & 0x3F];
    *p++ = alpha[((b0 & 0x03U) << 4U) | ((b1 >> 4U) & 0x0FU)];
    *p++ = alpha[((b1 & 0x0FU) << 2U) | ((b2 >> 6U) & 0x03U)];
    *p++ = alpha[b2 & 0x3FU];
  }

  // Remainder
  if (rem == 1U) {
    const unsigned char b0 = data[i];
    *p++ = alpha[(b0 >> 2U) & 0x3F];
    *p++ = alpha[(b0 & 0x03U) << 4U];
    if (pad) {
      *p++ = '=';
      *p++ = '=';
    }
  } else if (rem == 2U) {
    const unsigned char b0 = data[i + 0U];
    const unsigned char b1 = data[i + 1U];
    *p++ = alpha[(b0 >> 2U) & 0x3F];
    *p++ = alpha[((b0 & 0x03U) << 4U) | ((b1 >> 4U) & 0x0FU)];
    *p++ = alpha[(b1 & 0x0FU) << 2U];
    if (pad) {
      *p++ = '=';
    }
  }

  return out;
}

[[maybe_unused]] static std::string
Encode(std::string_view data, bool url_safe = false, bool pad = true) {
  return Encode(reinterpret_cast<const unsigned char *>(data.data()),
                data.size(), url_safe, pad);
}

[[maybe_unused]] static std::string
EncodeUrlSafe(const unsigned char *data, std::size_t len, bool pad = false) {
  return Encode(data, len, /*url_safe=*/true, /*pad=*/pad);
}

[[maybe_unused]] static std::string EncodeUrlSafe(std::string_view data,
                                                  bool pad = false) {
  return Encode(reinterpret_cast<const unsigned char *>(data.data()),
                data.size(), /*url_safe=*/true, /*pad=*/pad);
}

// --- Decoding ---------------------------------------------------------------

/**
 * Decode standard OR URL-safe Base64.
 * Accepts inputs with or without '=' padding.
 * Throws std::runtime_error on malformed input.
 */
[[maybe_unused]] static std::vector<unsigned char>
Decode(std::string_view input) {
  std::vector<unsigned char> out;
  if (input.empty()) {
    return out;
  }

  std::size_t inLen = input.size();
  std::size_t rem = inLen % 4U;

  // padded if multiple of 4 and last char is '='
  bool hasPadding = (rem == 0U) && (inLen >= 4U) && (input[inLen - 1U] == '=');

  // compute output length
  std::size_t outLen{};
  if (hasPadding) {
    outLen = (inLen / 4U) * 3U;
    if (input[inLen - 1U] == '=')
      outLen--;
    if (input[inLen - 2U] == '=')
      outLen--;
  } else {
    if (rem == 1U) {
      throw std::runtime_error("Invalid Base64 length (mod 4 == 1)");
    }
    outLen = (inLen / 4U) * 3U + (rem == 0U ? 0U : (rem == 2U ? 1U : 2U));
  }

  out.resize(outLen);

  auto readVal = [](unsigned char c) -> unsigned char {
    unsigned char v = kDecodingTable[c];
    if (v == 64U) {
      throw std::runtime_error("Invalid Base64 character");
    }
    return v;
  };

  std::size_t i = 0U;
  std::size_t j = 0U;

  // process all full unpadded quartets
  std::size_t lastFull =
      hasPadding ? (inLen - 4U) : (rem == 0U ? inLen : (inLen - rem));

  while (i + 4U <= lastFull) {
    unsigned char a = readVal(static_cast<unsigned char>(input[i + 0U]));
    unsigned char b = readVal(static_cast<unsigned char>(input[i + 1U]));
    unsigned char c = readVal(static_cast<unsigned char>(input[i + 2U]));
    unsigned char d = readVal(static_cast<unsigned char>(input[i + 3U]));
    i += 4U;

    std::uint32_t triple = (static_cast<std::uint32_t>(a) << 18U) |
                           (static_cast<std::uint32_t>(b) << 12U) |
                           (static_cast<std::uint32_t>(c) << 6U) |
                           (static_cast<std::uint32_t>(d));

    if (j < outLen)
      out[j++] = static_cast<unsigned char>((triple >> 16U) & 0xFFU);
    if (j < outLen)
      out[j++] = static_cast<unsigned char>((triple >> 8U) & 0xFFU);
    if (j < outLen)
      out[j++] = static_cast<unsigned char>(triple & 0xFFU);
  }

  // tail: padded quartet or unpadded remainder
  if (i < inLen) {
    std::size_t left = inLen - i;

    if (left == 4U) {
      bool thirdIsPad = (input[i + 2U] == '=');
      bool fourthIsPad = (input[i + 3U] == '=');

      // '=' is never allowed in positions 1 or 2 of any quartet
      if (input[i + 0U] == '=' || input[i + 1U] == '=') {
        throw std::runtime_error("Invalid Base64 padding placement");
      }

      unsigned char a = readVal(static_cast<unsigned char>(input[i + 0U]));
      unsigned char b = readVal(static_cast<unsigned char>(input[i + 1U]));
      unsigned char c = 0U;
      unsigned char d = 0U;

      if (!thirdIsPad) {
        c = readVal(static_cast<unsigned char>(input[i + 2U]));
        if (!fourthIsPad) {
          d = readVal(static_cast<unsigned char>(input[i + 3U]));
        }
      } else {
        // if the 3rd is '=', the 4th must also be '='
        if (!fourthIsPad) {
          throw std::runtime_error("Invalid Base64 padding placement");
        }
      }
      i += 4U;

      std::uint32_t triple = (static_cast<std::uint32_t>(a) << 18U) |
                             (static_cast<std::uint32_t>(b) << 12U) |
                             (static_cast<std::uint32_t>(c) << 6U) |
                             (static_cast<std::uint32_t>(d));

      if (j < outLen)
        out[j++] = static_cast<unsigned char>((triple >> 16U) & 0xFFU);
      if (!thirdIsPad && j < outLen)
        out[j++] = static_cast<unsigned char>((triple >> 8U) & 0xFFU);
      if (!fourthIsPad && !thirdIsPad && j < outLen)
        out[j++] = static_cast<unsigned char>(triple & 0xFFU);

    } else if (left == 2U || left == 3U) {
      unsigned char a = readVal(static_cast<unsigned char>(input[i + 0U]));
      unsigned char b = readVal(static_cast<unsigned char>(input[i + 1U]));
      unsigned char c = (left == 3U)
                            ? readVal(static_cast<unsigned char>(input[i + 2U]))
                            : 0U;
      i += left;

      std::uint32_t triple = (static_cast<std::uint32_t>(a) << 18U) |
                             (static_cast<std::uint32_t>(b) << 12U) |
                             (static_cast<std::uint32_t>(c) << 6U);

      if (j < outLen)
        out[j++] = static_cast<unsigned char>((triple >> 16U) & 0xFFU);
      if (left == 3U && j < outLen)
        out[j++] = static_cast<unsigned char>((triple >> 8U) & 0xFFU);
    } else {
      throw std::runtime_error("Invalid Base64 length (mod 4 == 1)");
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

#endif /* MACARON_BASE64_H_ */
       // NOLINTEND
