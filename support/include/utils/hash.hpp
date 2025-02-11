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
#ifndef REPERTORY_INCLUDE_UTILS_HASH_HPP_
#define REPERTORY_INCLUDE_UTILS_HASH_HPP_
#if defined(PROJECT_ENABLE_LIBSODIUM)

#include "utils/config.hpp"

#include "utils/error.hpp"

namespace repertory::utils::encryption {
using hash_256_t = std::array<unsigned char, 32U>;
using hash_384_t = std::array<unsigned char, 48U>;
using hash_512_t = std::array<unsigned char, 64U>;

[[nodiscard]] auto create_hash_blake2b_256(std::string_view data) -> hash_256_t;

[[nodiscard]] auto
create_hash_blake2b_256(std::wstring_view data) -> hash_256_t;

[[nodiscard]] auto
create_hash_blake2b_256(const data_buffer &data) -> hash_256_t;

[[nodiscard]] auto create_hash_blake2b_384(std::string_view data) -> hash_384_t;

[[nodiscard]] auto
create_hash_blake2b_384(std::wstring_view data) -> hash_384_t;

[[nodiscard]] auto
create_hash_blake2b_384(const data_buffer &data) -> hash_384_t;

[[nodiscard]] auto
create_hash_blake2b_512(std::wstring_view data) -> hash_512_t;

[[nodiscard]] auto
create_hash_blake2b_512(const data_buffer &data) -> hash_512_t;

[[nodiscard]] auto create_hash_blake2b_512(std::string_view data) -> hash_512_t;

template <typename hash_t>
[[nodiscard]] auto create_hash_blake2b_t(const unsigned char *data,
                                         std::size_t data_size) -> hash_t;

[[nodiscard]] auto create_hash_sha256(std::string_view data) -> hash_256_t;

[[nodiscard]] auto create_hash_sha256(std::wstring_view data) -> hash_256_t;

[[nodiscard]] auto create_hash_sha256(const data_buffer &data) -> hash_256_t;

[[nodiscard]] auto create_hash_sha256(const unsigned char *data,
                                      std::size_t data_size) -> hash_256_t;

[[nodiscard]] auto create_hash_sha512(std::string_view data) -> hash_512_t;

[[nodiscard]] auto create_hash_sha512(std::wstring_view data) -> hash_512_t;

[[nodiscard]] auto create_hash_sha512(const data_buffer &data) -> hash_512_t;

[[nodiscard]] auto create_hash_sha512(const unsigned char *data,
                                      std::size_t data_size) -> hash_512_t;

template <typename hash_t>
[[nodiscard]] inline auto default_create_hash() -> const
    std::function<hash_t(const unsigned char *data, std::size_t size)> &;

template <typename hash_t>
auto create_hash_blake2b_t(const unsigned char *data,
                           std::size_t data_size) -> hash_t {
  REPERTORY_USES_FUNCTION_NAME();

  hash_t hash{};

  crypto_generichash_blake2b_state state{};
  auto res = crypto_generichash_blake2b_init(&state, nullptr, 0U, hash.size());
  if (res != 0) {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to initialize blake2b",
                                             std::to_string(hash.size() * 8U),
                                             std::to_string(res),
                                         });
  }

  res = crypto_generichash_blake2b_update(&state, data, data_size);
  if (res != 0) {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to update blake2b",
                                             std::to_string(hash.size() * 8U),
                                             std::to_string(res),
                                         });
  }

  res = crypto_generichash_blake2b_final(&state, hash.data(), hash.size());
  if (res != 0) {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to finalize blake2b",
                                             std::to_string(hash.size() * 8U),
                                             std::to_string(res),
                                         });
  }

  return hash;
}

inline const std::function<hash_256_t(const unsigned char *data,
                                      std::size_t size)>
    blake2b_256_hasher =
        [](const unsigned char *data, std::size_t data_size) -> hash_256_t {
  return create_hash_blake2b_t<hash_256_t>(data, data_size);
};

inline const std::function<hash_384_t(const unsigned char *data,
                                      std::size_t size)>
    blake2b_384_hasher =
        [](const unsigned char *data, std::size_t data_size) -> hash_384_t {
  return create_hash_blake2b_t<hash_384_t>(data, data_size);
};

inline const std::function<hash_512_t(const unsigned char *data,
                                      std::size_t size)>
    blake2b_512_hasher =
        [](const unsigned char *data, std::size_t data_size) -> hash_512_t {
  return create_hash_blake2b_t<hash_512_t>(data, data_size);
};

inline const std::function<hash_256_t(const unsigned char *data,
                                      std::size_t size)>
    sha256_hasher =
        [](const unsigned char *data, std::size_t data_size) -> hash_256_t {
  return create_hash_sha256(data, data_size);
};

inline const std::function<hash_512_t(const unsigned char *data,
                                      std::size_t size)>
    sha512_hasher =
        [](const unsigned char *data, std::size_t data_size) -> hash_512_t {
  return create_hash_sha512(data, data_size);
};

template <>
[[nodiscard]] inline auto default_create_hash<hash_256_t>() -> const
    std::function<hash_256_t(const unsigned char *data, std::size_t size)> & {
  return blake2b_256_hasher;
}

template <>
[[nodiscard]] inline auto default_create_hash<hash_384_t>() -> const
    std::function<hash_384_t(const unsigned char *data, std::size_t size)> & {
  return blake2b_384_hasher;
}

template <>
[[nodiscard]] inline auto default_create_hash<hash_512_t>() -> const
    std::function<hash_512_t(const unsigned char *data, std::size_t size)> & {
  return blake2b_512_hasher;
}
} // namespace repertory::utils::encryption

#endif // defined(PROJECT_ENABLE_LIBSODIUM)
#endif // REPERTORY_INCLUDE_UTILS_HASH_HPP_
