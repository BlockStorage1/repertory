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
#if defined(PROJECT_ENABLE_LIBSODIUM)

#include "utils/hash.hpp"

#include "utils/error.hpp"

namespace repertory::utils::encryption {
auto create_hash_blake2b_256(std::string_view data) -> hash_256_t {
  return create_hash_blake2b_t<hash_256_t>(
      reinterpret_cast<const unsigned char *>(data.data()), data.size());
}

auto create_hash_blake2b_256(std::wstring_view data) -> hash_256_t {
  return create_hash_blake2b_t<hash_256_t>(
      reinterpret_cast<const unsigned char *>(data.data()),
      data.size() * sizeof(wchar_t));
}

auto create_hash_blake2b_256(const data_buffer &data) -> hash_256_t {
  return create_hash_blake2b_t<hash_256_t>(
      reinterpret_cast<const unsigned char *>(data.data()),
      data.size() * sizeof(data_buffer::value_type));
}

auto create_hash_blake2b_384(std::string_view data) -> hash_384_t {
  return create_hash_blake2b_t<hash_384_t>(
      reinterpret_cast<const unsigned char *>(data.data()), data.size());
}

auto create_hash_blake2b_384(std::wstring_view data) -> hash_384_t {
  return create_hash_blake2b_t<hash_384_t>(
      reinterpret_cast<const unsigned char *>(data.data()),
      data.size() * sizeof(wchar_t));
}

auto create_hash_blake2b_384(const data_buffer &data) -> hash_384_t {
  return create_hash_blake2b_t<hash_384_t>(
      reinterpret_cast<const unsigned char *>(data.data()),
      data.size() * sizeof(data_buffer::value_type));
}

auto create_hash_blake2b_512(std::string_view data) -> hash_512_t {
  return create_hash_blake2b_t<hash_512_t>(
      reinterpret_cast<const unsigned char *>(data.data()), data.size());
}

auto create_hash_blake2b_512(std::wstring_view data) -> hash_512_t {
  return create_hash_blake2b_t<hash_512_t>(
      reinterpret_cast<const unsigned char *>(data.data()),
      data.size() * sizeof(wchar_t));
}

auto create_hash_blake2b_512(const data_buffer &data) -> hash_512_t {
  return create_hash_blake2b_t<hash_512_t>(
      reinterpret_cast<const unsigned char *>(data.data()),
      data.size() * sizeof(data_buffer::value_type));
}

auto create_hash_sha256(std::string_view data) -> hash_256_t {
  return create_hash_sha256(
      reinterpret_cast<const unsigned char *>(data.data()), data.size());
}

auto create_hash_sha256(std::wstring_view data) -> hash_256_t {
  return create_hash_sha256(
      reinterpret_cast<const unsigned char *>(data.data()),
      data.size() * sizeof(wchar_t));
}

auto create_hash_sha256(const data_buffer &data) -> hash_256_t {
  return create_hash_sha256(
      reinterpret_cast<const unsigned char *>(data.data()),
      data.size() * sizeof(data_buffer::value_type));
}

auto create_hash_sha512(std::string_view data) -> hash_512_t {
  return create_hash_sha512(
      reinterpret_cast<const unsigned char *>(data.data()), data.size());
}

auto create_hash_sha512(std::wstring_view data) -> hash_512_t {
  return create_hash_sha512(
      reinterpret_cast<const unsigned char *>(data.data()),
      data.size() * sizeof(wchar_t));
}

auto create_hash_sha512(const data_buffer &data) -> hash_512_t {
  return create_hash_sha512(
      reinterpret_cast<const unsigned char *>(data.data()),
      data.size() * sizeof(data_buffer::value_type));
}

auto create_hash_sha512(const unsigned char *data,
                        std::size_t data_size) -> hash_512_t {
  REPERTORY_USES_FUNCTION_NAME();

  hash_512_t hash{};

  crypto_hash_sha512_state state{};
  auto res = crypto_hash_sha512_init(&state);
  if (res != 0) {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to initialize sha-512",
                                             std::to_string(res),
                                         });
  }

  res = crypto_hash_sha512_update(&state, data, data_size);
  if (res != 0) {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to update sha-512",
                                             std::to_string(res),
                                         });
  }

  res = crypto_hash_sha512_final(&state, hash.data());
  if (res != 0) {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to finalize sha-512",
                                             std::to_string(res),
                                         });
  }

  return hash;
}

auto create_hash_sha256(const unsigned char *data,
                        std::size_t data_size) -> hash_256_t {
  REPERTORY_USES_FUNCTION_NAME();

  hash_256_t hash{};

  crypto_hash_sha256_state state{};
  auto res = crypto_hash_sha256_init(&state);
  if (res != 0) {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to initialize sha-256",
                                             std::to_string(res),
                                         });
  }

  res = crypto_hash_sha256_update(&state, data, data_size);
  if (res != 0) {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to update sha-256",
                                             std::to_string(res),
                                         });
  }

  res = crypto_hash_sha256_final(&state, hash.data());
  if (res != 0) {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to finalize sha-256",
                                             std::to_string(res),
                                         });
  }

  return hash;
}
} // namespace repertory::utils::encryption

#endif // defined(PROJECT_ENABLE_LIBSODIUM)
