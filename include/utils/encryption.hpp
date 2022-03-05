/*
  Copyright <2018-2022> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute,
  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
  OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef INCLUDE_UTILS_ENCRYPTION_HPP_
#define INCLUDE_UTILS_ENCRYPTION_HPP_

#include "common.hpp"
#include "types/repertory.hpp"
#include "utils/encrypting_reader.hpp"

namespace repertory::utils::encryption {
// Prototypes
api_error decrypt_file_name(const std::string &encryption_token, std::string &file_name);

CryptoPP::SecByteBlock generate_key(const std::string &encryption_token);

api_error read_encrypted_range(
    const http_range &range, const CryptoPP::SecByteBlock &key,
    const std::function<api_error(std::vector<char> &ct, const std::uint64_t &start_offset,
                                  const std::uint64_t &end_offset)> &reader,
    const std::uint64_t &total_size, std::vector<char> &data);

// Implementations
template <typename result>
static bool decrypt_data(const CryptoPP::SecByteBlock &key, const char *buffer,
                         const std::size_t &buffer_size, result &res) {
  const auto header_size =
      static_cast<std::uint32_t>(encrypting_reader::get_header_size());
  if (buffer_size > header_size) {
    CryptoPP::XChaCha20Poly1305::Decryption decryption;
    CryptoPP::SecByteBlock iv(decryption.IVSize());
    std::memcpy(&iv[0u], &buffer[0u], iv.size());

    CryptoPP::SecByteBlock mac(decryption.DigestSize());
    std::memcpy(&mac[0u], &buffer[iv.size()], mac.size());
    decryption.SetKeyWithIV(key, key.size(), iv, iv.size());

    const std::uint32_t size =
        boost::endian::native_to_big(static_cast<std::uint32_t>(buffer_size));
    res.resize(buffer_size - header_size);
    return decryption.DecryptAndVerify(
        reinterpret_cast<CryptoPP::byte *>(&res[0u]), &mac[0u], mac.size(), &iv[0u],
        static_cast<int>(iv.size()), reinterpret_cast<const CryptoPP::byte *>(&size), sizeof(size),
        reinterpret_cast<const CryptoPP::byte *>(&buffer[header_size]), res.size());
  }

  return false;
}

template <typename buffer, typename result>
static bool decrypt_data(const CryptoPP::SecByteBlock &key, const buffer &buf, result &res) {
  return decrypt_data<result>(key, &buf[0u], buf.size(), res);
}

template <typename buffer, typename result>
static bool decrypt_data(const std::string &encryption_token, const buffer &buf, result &res) {
  return decrypt_data<buffer, result>(generate_key(encryption_token), buf, res);
}

template <typename result>
static bool decrypt_data(const std::string &encryption_token, const char *buffer,
                         const std::size_t &buffer_size, result &res) {
  return decrypt_data<result>(generate_key(encryption_token), buffer, buffer_size, res);
}

template <typename result>
static void encrypt_data(const CryptoPP::SecByteBlock &key, const char *buffer,
                         const std::size_t &buffer_size, result &res) {
  CryptoPP::XChaCha20Poly1305::Encryption encryption;

  CryptoPP::SecByteBlock iv(encryption.IVSize());
  CryptoPP::OS_GenerateRandomBlock(false, iv, iv.size());

  CryptoPP::SecByteBlock mac(encryption.DigestSize());
  const std::uint32_t header_size =
      static_cast<std::uint32_t>(encrypting_reader::get_header_size());
  const std::uint32_t size =
      boost::endian::native_to_big(static_cast<std::uint32_t>(buffer_size + header_size));

  encryption.SetKeyWithIV(key, key.size(), iv, iv.size());

  res.resize(buffer_size + header_size);
  encryption.EncryptAndAuthenticate(reinterpret_cast<CryptoPP::byte *>(&res[header_size]), &mac[0u],
                                    mac.size(), &iv[0u], static_cast<int>(iv.size()),
                                    reinterpret_cast<const CryptoPP::byte *>(&size), sizeof(size),
                                    reinterpret_cast<const CryptoPP::byte *>(buffer), buffer_size);
  std::memcpy(&res[0u], &iv[0u], iv.size());
  std::memcpy(&res[iv.size()], &mac[0u], mac.size());
}

template <typename result>
static void encrypt_data(const std::string &encryption_token, const char *buffer,
                         const std::size_t &buffer_size, result &res) {
  return encrypt_data<result>(generate_key(encryption_token), buffer, buffer_size, res);
}

template <typename buffer, typename result>
static void encrypt_data(const std::string &encryption_token, const buffer &buf, result &res) {
  return encrypt_data<result>(generate_key(encryption_token), &buf[0u], buf.size(), res);
}

template <typename buffer, typename result>
static void encrypt_data(const CryptoPP::SecByteBlock &key, const buffer &buf, result &res) {
  return encrypt_data<result>(key, &buf[0u], buf.size(), res);
}
} // namespace repertory::utils::encryption

#endif // INCLUDE_UTILS_ENCRYPTION_HPP_
