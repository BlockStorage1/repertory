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
#include "utils/encryption.hpp"
#include "events/events.hpp"
#include "events/event_system.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/utils.hpp"

namespace repertory::utils::encryption {
api_error decrypt_file_name(const std::string &encryption_token, std::string &file_name) {
  std::vector<char> buffer;
  if (not utils::from_hex_string(file_name, buffer)) {
    return api_error::error;
  }

  file_name.clear();
  if (not utils::encryption::decrypt_data(encryption_token, buffer, file_name)) {
    return api_error::decryption_error;
  }

  return api_error::success;
}

CryptoPP::SecByteBlock generate_key(const std::string &encryption_token) {
  CryptoPP::SecByteBlock key(CryptoPP::SHA256::DIGESTSIZE);
  CryptoPP::SHA256().CalculateDigest(
      reinterpret_cast<CryptoPP::byte *>(&key[0u]),
      reinterpret_cast<const CryptoPP::byte *>(encryption_token.c_str()),
      strnlen(encryption_token.c_str(), encryption_token.size()));

  return key;
}

api_error read_encrypted_range(
    const http_range &range, const CryptoPP::SecByteBlock &key,
    const std::function<api_error(std::vector<char> &ct, const std::uint64_t &start_offset,
                                  const std::uint64_t &end_offset)> &reader,
    const std::uint64_t &total_size, std::vector<char> &data) {
  static constexpr const auto &encrypted_chunk_size =
      utils::encryption::encrypting_reader::get_encrypted_chunk_size();
  static constexpr const auto &data_chunk_size =
      utils::encryption::encrypting_reader::get_data_chunk_size();
  static constexpr const auto &header_size =
      utils::encryption::encrypting_reader::get_header_size();

  const auto start_chunk = static_cast<std::size_t>(range.begin / data_chunk_size);
  const auto end_chunk = static_cast<std::size_t>(range.end / data_chunk_size);
  auto remain = range.end - range.begin + 1u;
  auto source_offset = static_cast<std::size_t>(range.begin % data_chunk_size);

  for (std::size_t chunk = start_chunk; chunk <= end_chunk; chunk++) {
    std::vector<char> ct;
    const auto start_offset = chunk * encrypted_chunk_size;
    const auto end_offset =
        std::min(start_offset + (total_size - (chunk * data_chunk_size)) + header_size - 1u,
                 static_cast<std::uint64_t>(start_offset + encrypted_chunk_size - 1u));

    const auto result = reader(ct, start_offset, end_offset);
    if (result != api_error::success) {
      return result;
    }

    std::vector<char> source_buffer;
    if (not utils::encryption::decrypt_data(key, ct, source_buffer)) {
      return api_error::decryption_error;
    }
    ct.clear();

    const auto data_size = static_cast<std::size_t>(
        std::min(remain, static_cast<std::uint64_t>(data_chunk_size - source_offset)));
    std::copy(source_buffer.begin() + source_offset,
              source_buffer.begin() + source_offset + data_size, std::back_inserter(data));
    remain -= data_size;
    source_offset = 0u;
  }

  return api_error::success;
}
} // namespace repertory::utils::encryption
