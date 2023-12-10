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
#include "utils/encryption.hpp"

#include "events/event_system.hpp"
#include "events/events.hpp"
#include "types/repertory.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/utils.hpp"

namespace repertory::utils::encryption {
auto decrypt_file_path(const std::string &encryption_token,
                       std::string &file_path) -> api_error {
  std::string decrypted_file_path{};
  for (const auto &part : std::filesystem::path(file_path)) {
    auto file_name = part.string();
    if (file_name == "/") {
      continue;
    }

    auto res = decrypt_file_name(encryption_token, file_name);
    if (res != api_error::success) {
      return res;
    }

    decrypted_file_path += '/' + file_name;
  }

  file_path = decrypted_file_path;
  return api_error::success;
}

auto decrypt_file_name(const std::string &encryption_token,
                       std::string &file_name) -> api_error {
  data_buffer buffer;
  if (not utils::from_hex_string(file_name, buffer)) {
    return api_error::error;
  }

  file_name.clear();
  if (not utils::encryption::decrypt_data(encryption_token, buffer,
                                          file_name)) {
    return api_error::decryption_error;
  }

  return api_error::success;
}

auto generate_key(const std::string &encryption_token) -> key_type {
  crypto_hash_sha256_state state{};
  auto res = crypto_hash_sha256_init(&state);
  if (res != 0) {
    throw std::runtime_error("failed to initialize sha256|" +
                             std::to_string(res));
  }
  res = crypto_hash_sha256_update(
      &state, reinterpret_cast<const unsigned char *>(encryption_token.c_str()),
      strnlen(encryption_token.c_str(), encryption_token.size()));
  if (res != 0) {
    throw std::runtime_error("failed to update sha256|" + std::to_string(res));
  }

  key_type ret{};
  res = crypto_hash_sha256_final(&state, ret.data());
  if (res != 0) {
    throw std::runtime_error("failed to finalize sha256|" +
                             std::to_string(res));
  }

  return ret;
}

auto read_encrypted_range(const http_range &range, const key_type &key,
                          reader_func reader, std::uint64_t total_size,
                          data_buffer &data) -> api_error {
  const auto encrypted_chunk_size =
      utils::encryption::encrypting_reader::get_encrypted_chunk_size();
  const auto data_chunk_size =
      utils::encryption::encrypting_reader::get_data_chunk_size();
  const auto header_size =
      utils::encryption::encrypting_reader::get_header_size();

  const auto start_chunk =
      static_cast<std::size_t>(range.begin / data_chunk_size);
  const auto end_chunk = static_cast<std::size_t>(range.end / data_chunk_size);
  auto remain = range.end - range.begin + 1U;
  auto source_offset = static_cast<std::size_t>(range.begin % data_chunk_size);

  for (std::size_t chunk = start_chunk; chunk <= end_chunk; chunk++) {
    data_buffer cypher;
    const auto start_offset = chunk * encrypted_chunk_size;
    const auto end_offset = std::min(
        start_offset + (total_size - (chunk * data_chunk_size)) + header_size -
            1U,
        static_cast<std::uint64_t>(start_offset + encrypted_chunk_size - 1U));

    const auto result = reader(cypher, start_offset, end_offset);
    if (result != api_error::success) {
      return result;
    }

    data_buffer source_buffer;
    if (not utils::encryption::decrypt_data(key, cypher, source_buffer)) {
      return api_error::decryption_error;
    }
    cypher.clear();

    const auto data_size = static_cast<std::size_t>(std::min(
        remain, static_cast<std::uint64_t>(data_chunk_size - source_offset)));
    std::copy(std::next(source_buffer.begin(),
                        static_cast<std::int64_t>(source_offset)),
              std::next(source_buffer.begin(),
                        static_cast<std::int64_t>(source_offset + data_size)),
              std::back_inserter(data));
    remain -= data_size;
    source_offset = 0U;
  }

  return api_error::success;
}
} // namespace repertory::utils::encryption
