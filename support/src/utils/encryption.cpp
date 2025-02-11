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
#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)

#include "utils/encryption.hpp"

#include "utils/collection.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/path.hpp"

namespace repertory::utils::encryption {
auto decrypt_file_path(std::string_view encryption_token,
                       std::string &file_path) -> bool {
  std::vector<std::string> decrypted_parts;
  for (const auto &part : std::filesystem::path(file_path)) {
    auto file_name = part.string();
    if (file_name == "/") {
      continue;
    }

    if (not decrypt_file_name(encryption_token, file_name)) {
      return false;
    }

    decrypted_parts.push_back(file_name);
  }

  file_path =
      utils::path::create_api_path(utils::string::join(decrypted_parts, '/'));
  return true;
}

auto decrypt_file_name(std::string_view encryption_token,
                       std::string &file_name) -> bool {
  data_buffer buffer;
  if (not utils::collection::from_hex_string(file_name, buffer)) {
    return false;
  }

  file_name.clear();
  return utils::encryption::decrypt_data(encryption_token, buffer, file_name);
}

auto read_encrypted_range(const http_range &range,
                          const utils::encryption::hash_256_t &key,
                          reader_func_t reader_func, std::uint64_t total_size,
                          data_buffer &data) -> bool {
  auto encrypted_chunk_size =
      utils::encryption::encrypting_reader::get_encrypted_chunk_size();
  auto data_chunk_size =
      utils::encryption::encrypting_reader::get_data_chunk_size();

  auto start_chunk = static_cast<std::size_t>(range.begin / data_chunk_size);
  auto end_chunk = static_cast<std::size_t>(range.end / data_chunk_size);
  auto remain = range.end - range.begin + 1U;
  auto source_offset = static_cast<std::size_t>(range.begin % data_chunk_size);

  for (std::size_t chunk = start_chunk; chunk <= end_chunk; chunk++) {
    data_buffer cypher;
    auto start_offset = chunk * encrypted_chunk_size;
    auto end_offset = std::min(
        start_offset + (total_size - (chunk * data_chunk_size)) +
            encryption_header_size - 1U,
        static_cast<std::uint64_t>(start_offset + encrypted_chunk_size - 1U));

    if (not reader_func(cypher, start_offset, end_offset)) {
      return false;
    }

    data_buffer source_buffer;
    if (not utils::encryption::decrypt_data(key, cypher, source_buffer)) {
      return false;
    }
    cypher.clear();

    auto data_size = static_cast<std::size_t>(std::min(
        remain, static_cast<std::uint64_t>(data_chunk_size - source_offset)));
    std::copy(std::next(source_buffer.begin(),
                        static_cast<std::int64_t>(source_offset)),
              std::next(source_buffer.begin(),
                        static_cast<std::int64_t>(source_offset + data_size)),
              std::back_inserter(data));
    remain -= data_size;
    source_offset = 0U;
  }

  return true;
}

auto read_encrypted_range(const http_range &range,
                          const utils::encryption::hash_256_t &key,
                          reader_func_t reader_func, std::uint64_t total_size,
                          unsigned char *data, std::size_t size,
                          std::size_t &bytes_read) -> bool {
  bytes_read = 0U;

  auto encrypted_chunk_size =
      utils::encryption::encrypting_reader::get_encrypted_chunk_size();
  auto data_chunk_size =
      utils::encryption::encrypting_reader::get_data_chunk_size();

  auto start_chunk = static_cast<std::size_t>(range.begin / data_chunk_size);
  auto end_chunk = static_cast<std::size_t>(range.end / data_chunk_size);
  auto remain = range.end - range.begin + 1U;
  auto source_offset = static_cast<std::size_t>(range.begin % data_chunk_size);

  std::span dest_buffer(data, size);
  for (std::size_t chunk = start_chunk; chunk <= end_chunk; chunk++) {
    data_buffer cypher;
    auto start_offset = chunk * encrypted_chunk_size;
    auto end_offset = std::min(
        start_offset + (total_size - (chunk * data_chunk_size)) +
            encryption_header_size - 1U,
        static_cast<std::uint64_t>(start_offset + encrypted_chunk_size - 1U));

    if (not reader_func(cypher, start_offset, end_offset)) {
      return false;
    }

    data_buffer source_buffer;
    if (not utils::encryption::decrypt_data(key, cypher, source_buffer)) {
      return false;
    }
    cypher.clear();

    auto data_size = static_cast<std::size_t>(std::min(
        remain, static_cast<std::uint64_t>(data_chunk_size - source_offset)));
    std::copy(
        std::next(source_buffer.begin(),
                  static_cast<std::int64_t>(source_offset)),
        std::next(source_buffer.begin(),
                  static_cast<std::int64_t>(source_offset + data_size)),
        std::next(dest_buffer.begin(), static_cast<std::int64_t>(bytes_read)));
    remain -= data_size;
    bytes_read += data_size;
    source_offset = 0U;
  }

  return true;
}
} // namespace repertory::utils::encryption

#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined (PROJECT_ENABLE_BOOST)
