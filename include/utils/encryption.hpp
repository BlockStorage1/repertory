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
#ifndef INCLUDE_UTILS_ENCRYPTION_HPP_
#define INCLUDE_UTILS_ENCRYPTION_HPP_

#include "types/repertory.hpp"
#include "utils/encrypting_reader.hpp"

namespace repertory::utils::encryption {
// Prototypes
[[nodiscard]] auto decrypt_file_path(const std::string &encryption_token,
                                     std::string &file_path) -> api_error;

[[nodiscard]] auto decrypt_file_name(const std::string &encryption_token,
                                     std::string &file_name) -> api_error;

[[nodiscard]] auto generate_key(const std::string &encryption_token)
    -> key_type;

[[nodiscard]] auto read_encrypted_range(
    const http_range &range, const key_type &key,
    const std::function<api_error(data_buffer &ct, std::uint64_t start_offset,
                                  std::uint64_t end_offset)> &reader,
    std::uint64_t total_size, data_buffer &data) -> api_error;

// Implementations
template <typename result>
[[nodiscard]] inline auto decrypt_data(const key_type &key, const char *buffer,
                                       std::size_t buffer_size, result &res)
    -> bool {
  const auto header_size =
      static_cast<std::uint32_t>(encrypting_reader::get_header_size());
  if (buffer_size > header_size) {
    const std::uint32_t size =
        boost::endian::native_to_big(static_cast<std::uint32_t>(buffer_size));
    res.resize(buffer_size - header_size);
    return crypto_aead_xchacha20poly1305_ietf_decrypt_detached(
               reinterpret_cast<unsigned char *>(&res[0u]), nullptr,
               reinterpret_cast<const unsigned char *>(&buffer[header_size]),
               res.size(),
               reinterpret_cast<const unsigned char *>(
                   &buffer[crypto_aead_xchacha20poly1305_IETF_NPUBBYTES]),
               reinterpret_cast<const unsigned char *>(&size), sizeof(size),
               reinterpret_cast<const unsigned char *>(buffer),
               key.data()) == 0;
  }

  return false;
}

template <typename buffer, typename result>
[[nodiscard]] inline auto decrypt_data(const key_type &key, const buffer &buf,
                                       result &res) -> bool {
  return decrypt_data<result>(key, &buf[0u], buf.size(), res);
}

template <typename buffer, typename result>
[[nodiscard]] inline auto decrypt_data(const std::string &encryption_token,
                                       const buffer &buf, result &res) -> bool {
  return decrypt_data<buffer, result>(generate_key(encryption_token), buf, res);
}

template <typename result>
[[nodiscard]] inline auto decrypt_data(const std::string &encryption_token,
                                       const char *buffer,
                                       std::size_t buffer_size, result &res)
    -> bool {
  return decrypt_data<result>(generate_key(encryption_token), buffer,
                              buffer_size, res);
}

template <typename result>
inline void
encrypt_data(const std::array<unsigned char,
                              crypto_aead_xchacha20poly1305_IETF_NPUBBYTES> &iv,
             const key_type &key, const char *buffer, std::size_t buffer_size,
             result &res) {
  std::array<unsigned char, crypto_aead_xchacha20poly1305_IETF_ABYTES> mac{};

  const auto header_size =
      static_cast<std::uint32_t>(encrypting_reader::get_header_size());

  const std::uint32_t size = boost::endian::native_to_big(
      static_cast<std::uint32_t>(buffer_size + header_size));

  res.resize(buffer_size + header_size);

  unsigned long long mac_length{};
  if (crypto_aead_xchacha20poly1305_ietf_encrypt_detached(
          reinterpret_cast<unsigned char *>(&res[header_size]), mac.data(),
          &mac_length, reinterpret_cast<const unsigned char *>(buffer),
          buffer_size, reinterpret_cast<const unsigned char *>(&size),
          sizeof(size), nullptr, iv.data(), key.data()) != 0) {
    throw std::runtime_error("encryption failed");
  }

  std::memcpy(&res[0u], &iv[0u], iv.size());
  std::memcpy(&res[iv.size()], &mac[0u], mac.size());
}

template <typename result>
inline void encrypt_data(const key_type &key, const char *buffer,
                         std::size_t buffer_size, result &res) {
  std::array<unsigned char, crypto_aead_xchacha20poly1305_IETF_NPUBBYTES> iv{};
  randombytes_buf(iv.data(), iv.size());

  encrypt_data<result>(iv, key, buffer, buffer_size, res);
}

template <typename result>
inline void encrypt_data(const std::string &encryption_token,
                         const char *buffer, std::size_t buffer_size,
                         result &res) {
  encrypt_data<result>(generate_key(encryption_token), buffer, buffer_size,
                       res);
}

template <typename buffer, typename result>
inline void encrypt_data(const std::string &encryption_token, const buffer &buf,
                         result &res) {
  encrypt_data<result>(generate_key(encryption_token), &buf[0u], buf.size(),
                       res);
}

template <typename buffer, typename result>
inline void encrypt_data(const key_type &key, const buffer &buf, result &res) {
  encrypt_data<result>(key, &buf[0u], buf.size(), res);
}

template <typename buffer, typename result>
inline void
encrypt_data(const std::array<unsigned char,
                              crypto_aead_xchacha20poly1305_IETF_NPUBBYTES> &iv,
             const key_type &key, const buffer &buf, result &res) {
  encrypt_data<result>(iv, key, &buf[0u], buf.size(), res);
}
} // namespace repertory::utils::encryption

#endif // INCLUDE_UTILS_ENCRYPTION_HPP_
