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
#ifndef REPERTORY_INCLUDE_UTILS_ENCRYPTION_HPP_
#define REPERTORY_INCLUDE_UTILS_ENCRYPTION_HPP_
#if defined(PROJECT_ENABLE_LIBSODIUM)

#include "utils/config.hpp"

#include "utils/error.hpp"
#include "utils/hash.hpp"

namespace repertory::utils::encryption {
inline constexpr const std::uint32_t encryption_header_size{
    crypto_aead_xchacha20poly1305_IETF_NPUBBYTES +
        crypto_aead_xchacha20poly1305_IETF_ABYTES,
};

template <typename hash_t>
inline auto generate_key(
    std::string_view password,
    std::function<hash_t(const unsigned char *data, std::size_t size)> hasher =
        default_create_hash<hash_t>()) -> hash_t;

template <typename hash_t>
inline auto generate_key(
    std::wstring_view password,
    std::function<hash_t(const unsigned char *data, std::size_t size)> hasher =
        default_create_hash<hash_t>()) -> hash_t;

#if defined(PROJECT_ENABLE_BOOST)
[[nodiscard]] auto decrypt_file_name(std::string_view encryption_token,
                                     std::string &file_name) -> bool;

[[nodiscard]] auto decrypt_file_path(std::string_view encryption_token,
                                     std::string &file_path) -> bool;

template <typename result_t, typename arr_t, std::size_t arr_size>
[[nodiscard]] inline auto decrypt_data(const std::array<arr_t, arr_size> &key,
                                       const unsigned char *buffer,
                                       std::size_t buffer_size,
                                       result_t &res) -> bool {
  if (buffer_size > encryption_header_size) {
    const std::uint32_t size =
        boost::endian::native_to_big(static_cast<std::uint32_t>(buffer_size));
    res.resize(buffer_size - encryption_header_size);
    return crypto_aead_xchacha20poly1305_ietf_decrypt_detached(
               reinterpret_cast<unsigned char *>(res.data()), nullptr,
               &buffer[encryption_header_size], res.size(),
               &buffer[crypto_aead_xchacha20poly1305_IETF_NPUBBYTES],
               reinterpret_cast<const unsigned char *>(&size), sizeof(size),
               buffer, key.data()) == 0;
  }

  return false;
}

template <typename buffer_t, typename result_t, typename arr_t,
          std::size_t arr_size>
[[nodiscard]] inline auto decrypt_data(const std::array<arr_t, arr_size> &key,
                                       const buffer_t &buf,
                                       result_t &res) -> bool {
  return decrypt_data<result_t>(
      key, reinterpret_cast<const unsigned char *>(buf.data()), buf.size(),
      res);
}

template <typename buffer_t, typename result_t, typename hash_t = hash_256_t>
[[nodiscard]] inline auto decrypt_data(
    std::string_view password, const buffer_t &buf, result_t &res,
    std::function<hash_t(const unsigned char *data, std::size_t size)> hasher =
        default_create_hash<hash_t>()) -> bool {
  return decrypt_data<buffer_t, result_t>(generate_key(password, hasher), buf,
                                          res);
}

template <typename result_t, typename hash_t = hash_256_t>
[[nodiscard]] inline auto decrypt_data(
    std::string_view password, const unsigned char *buffer,
    std::size_t buffer_size, result_t &res,
    std::function<hash_t(const unsigned char *data, std::size_t size)> hasher =
        default_create_hash<hash_t>()) -> bool {
  return decrypt_data<result_t>(generate_key(password, hasher), buffer,
                                buffer_size, res);
}

template <typename result_t, typename arr_t, std::size_t arr_size>
inline void
encrypt_data(const std::array<unsigned char,
                              crypto_aead_xchacha20poly1305_IETF_NPUBBYTES> &iv,
             const std::array<arr_t, arr_size> &key,
             const unsigned char *buffer, std::size_t buffer_size,
             result_t &res) {
  REPERTORY_USES_FUNCTION_NAME();

  std::array<unsigned char, crypto_aead_xchacha20poly1305_IETF_ABYTES> mac{};

  const std::uint32_t size = boost::endian::native_to_big(
      static_cast<std::uint32_t>(buffer_size + encryption_header_size));

  res.resize(buffer_size + encryption_header_size);

  unsigned long long mac_length{};
  if (crypto_aead_xchacha20poly1305_ietf_encrypt_detached(
          reinterpret_cast<unsigned char *>(&res[encryption_header_size]),
          mac.data(), &mac_length, buffer, buffer_size,
          reinterpret_cast<const unsigned char *>(&size), sizeof(size), nullptr,
          iv.data(), key.data()) != 0) {
    throw repertory::utils::error::create_exception(function_name,
                                                    {
                                                        "encryption failed",
                                                    });
  }

  std::memcpy(res.data(), iv.data(), iv.size());
  std::memcpy(&res[iv.size()], mac.data(), mac.size());
}

template <typename result_t, typename arr_t, std::size_t arr_size>
inline void encrypt_data(const std::array<arr_t, arr_size> &key,
                         const unsigned char *buffer, std::size_t buffer_size,
                         result_t &res) {
  std::array<unsigned char, crypto_aead_xchacha20poly1305_IETF_NPUBBYTES> iv{};
  randombytes_buf(iv.data(), iv.size());

  encrypt_data<result_t>(iv, key, buffer, buffer_size, res);
}

template <typename result_t, typename hash_t = hash_256_t>
inline void encrypt_data(
    std::string_view password, const unsigned char *buffer,
    std::size_t buffer_size, result_t &res,
    std::function<hash_t(const unsigned char *data, std::size_t size)> hasher =
        default_create_hash<hash_t>()) {
  encrypt_data<result_t>(generate_key(password, hasher), buffer, buffer_size,
                         res);
}

template <typename buffer_t, typename result_t, typename hash_t = hash_256_t>
inline void encrypt_data(
    std::string_view password, const buffer_t &buf, result_t &res,
    std::function<hash_t(const unsigned char *data, std::size_t size)> hasher =
        default_create_hash<hash_t>()) {
  encrypt_data<result_t>(generate_key(password, hasher),
                         reinterpret_cast<const unsigned char *>(buf.data()),
                         buf.size(), res);
}

template <typename buffer_t, typename result_t, typename arr_t,
          std::size_t arr_size>
inline void encrypt_data(const std::array<arr_t, arr_size> &key,
                         const buffer_t &buf, result_t &res) {
  encrypt_data<result_t>(key,
                         reinterpret_cast<const unsigned char *>(buf.data()),
                         buf.size(), res);
}

template <typename buffer_t, typename result_t, typename arr_t,
          std::size_t arr_size>
inline void
encrypt_data(const std::array<unsigned char,
                              crypto_aead_xchacha20poly1305_IETF_NPUBBYTES> &iv,
             const std::array<arr_t, arr_size> &key, const buffer_t &buf,
             result_t &res) {
  encrypt_data<result_t>(iv, key,
                         reinterpret_cast<const unsigned char *>(buf.data()),
                         buf.size(), res);
}

using reader_func_t =
    std::function<bool(data_buffer &cypher_text, std::uint64_t start_offset,
                       std::uint64_t end_offset)>;

[[nodiscard]] auto
read_encrypted_range(const http_range &range,
                     const utils::encryption::hash_256_t &key,
                     reader_func_t reader_func, std::uint64_t total_size,
                     data_buffer &data) -> bool;

[[nodiscard]] auto read_encrypted_range(
    const http_range &range, const utils::encryption::hash_256_t &key,
    reader_func_t reader_func, std::uint64_t total_size, unsigned char *data,
    std::size_t size, std::size_t &bytes_read) -> bool;
#endif // defined(PROJECT_ENABLE_BOOST)

template <typename hash_t>
inline auto generate_key(
    std::string_view password,
    std::function<hash_t(const unsigned char *data, std::size_t size)> hasher)
    -> hash_t {
  return hasher(reinterpret_cast<const unsigned char *>(password.data()),
                password.size());
}

template <typename hash_t>
inline auto generate_key(
    std::wstring_view password,
    std::function<hash_t(const unsigned char *data, std::size_t size)> hasher)
    -> hash_t {
  return hasher(reinterpret_cast<const unsigned char *>(password.data()),
                password.size() * sizeof(wchar_t));
}
} // namespace repertory::utils::encryption

#endif // defined(PROJECT_ENABLE_LIBSODIUM)
#endif // REPERTORY_INCLUDE_UTILS_ENCRYPTION_HPP_
