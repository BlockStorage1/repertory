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

#include "utils/base64.hpp"
#include "utils/collection.hpp"
#include "utils/config.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/hash.hpp"
#include "utils/path.hpp"

namespace {
constexpr auto resize_by(repertory::data_span &data, std::size_t /* size */)
    -> repertory::data_span & {
  return data;
}

auto resize_by(repertory::data_buffer &data, std::size_t size)
    -> repertory::data_buffer & {
  data.resize(data.size() + size);
  return data;
}
} // namespace

namespace repertory::utils::encryption {
auto kdf_config::to_header() const -> data_buffer {
  kdf_config tmp{*this};
  tmp.checksum = boost::endian::native_to_big(tmp.checksum);
  tmp.unique_id = boost::endian::native_to_big(tmp.unique_id);

  data_buffer ret(size());
  std::memcpy(ret.data(), &tmp, ret.size());
  return ret;
}

auto kdf_config::generate_checksum() const -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  kdf_config tmp = *this;
  tmp.checksum = 0;

  auto hash = utils::hash::create_hash_blake2b_64(tmp.to_header());
  std::uint64_t ret{};
  std::memcpy(&ret, hash.data(), hash.size());
  return ret;
}

auto kdf_config::from_header(data_cspan data, kdf_config &cfg,
                             bool ignore_checksum) -> bool {
  if (data.size() < kdf_config::size()) {
    return false;
  }

  std::memcpy(&cfg, data.data(), kdf_config::size());

  cfg.checksum = boost::endian::big_to_native(cfg.checksum);
  cfg.unique_id = boost::endian::big_to_native(cfg.unique_id);
  return cfg.version == kdf_version::v1 && cfg.kdf == kdf_type::argon2id &&
         cfg.memlimit >= memlimit_level::level1 &&
         cfg.memlimit <= memlimit_level::level4 &&
         cfg.opslimit >= opslimit_level::level1 &&
         cfg.opslimit <= opslimit_level::level3 &&
         (ignore_checksum || cfg.checksum == cfg.generate_checksum());
}

void kdf_config::seal() {
  randombytes_buf(salt.data(), salt.size());
  checksum = generate_checksum();
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

auto decrypt_file_name(std::string_view encryption_token, const kdf_config &cfg,
                       std::string &file_name) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    auto buffer = macaron::Base64::Decode(file_name);

    file_name.clear();
    return utils::encryption::decrypt_data(encryption_token, cfg, buffer,
                                           file_name);
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto decrypt_file_name(const utils::hash::hash_256_t &master_key,
                       std::string &file_name) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    auto buffer = macaron::Base64::Decode(file_name);

    utils::encryption::kdf_config path_cfg;
    if (not utils::encryption::kdf_config::from_header(buffer, path_cfg)) {
      return false;
    }

    auto path_key = path_cfg.recreate_subkey(
        utils::encryption::kdf_context::path, master_key);

    file_name.clear();
    return utils::encryption::decrypt_data(
        path_key, &buffer[utils::encryption::kdf_config::size()],
        buffer.size() - utils::encryption::kdf_config::size(), file_name);
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

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

auto decrypt_file_path(std::string_view encryption_token, const kdf_config &cfg,
                       std::string &file_path) -> bool {
  std::vector<std::string> decrypted_parts;
  for (const auto &part : std::filesystem::path(file_path)) {
    auto file_name = part.string();
    if (file_name == "/") {
      continue;
    }

    if (not decrypt_file_name(encryption_token, cfg, file_name)) {
      return false;
    }

    decrypted_parts.push_back(file_name);
  }

  file_path =
      utils::path::create_api_path(utils::string::join(decrypted_parts, '/'));
  return true;
}

auto decrypt_file_path(const utils::hash::hash_256_t &master_key,
                       std::string &file_path) -> bool {
  std::vector<std::string> decrypted_parts;
  for (const auto &part : std::filesystem::path(file_path)) {
    auto file_name = part.string();
    if (file_name == "/") {
      continue;
    }

    if (not decrypt_file_name(master_key, file_name)) {
      return false;
    }

    decrypted_parts.push_back(file_name);
  }

  file_path =
      utils::path::create_api_path(utils::string::join(decrypted_parts, '/'));
  return true;
}

template <typename data_t>
[[nodiscard]] auto
read_encrypted_range(http_range range, const utils::hash::hash_256_t &key,
                     reader_func_t reader_func, std::uint64_t total_size,
                     data_t &data, std::uint8_t file_header_size,
                     std::size_t &bytes_read) -> bool {
  bytes_read = 0U;

  {
    if (total_size == 0U) {
      return true;
    }

    std::uint64_t begin = range.begin;
    std::uint64_t end = range.end;

    if (begin >= total_size) {
      return true;
    }

    std::uint64_t last = total_size - 1U;
    if (end > last) {
      end = last;
    }

    if (end < begin) {
      return true;
    }

    range = http_range{
        .begin = begin,
        .end = end,
    };
  }

  auto encrypted_chunk_size =
      utils::encryption::encrypting_reader::get_encrypted_chunk_size();
  auto data_chunk_size =
      utils::encryption::encrypting_reader::get_data_chunk_size();

  auto start_chunk = static_cast<std::size_t>(range.begin / data_chunk_size);
  auto end_chunk = static_cast<std::size_t>(range.end / data_chunk_size);
  auto remain = range.end - range.begin + 1U;
  auto source_offset = static_cast<std::size_t>(range.begin % data_chunk_size);

  for (std::size_t chunk = start_chunk; chunk <= end_chunk; chunk++) {
    data_buffer cipher;
    auto start_offset = (chunk * encrypted_chunk_size) + file_header_size;
    auto end_offset = std::min(
        start_offset + (total_size - (chunk * data_chunk_size)) +
            encryption_header_size - 1U,
        static_cast<std::uint64_t>(start_offset + encrypted_chunk_size - 1U));

    if (not reader_func(cipher, start_offset, end_offset)) {
      return false;
    }

    data_buffer source_buffer;
    if (not utils::encryption::decrypt_data(key, cipher, source_buffer)) {
      return false;
    }
    cipher.clear();

    auto data_size = static_cast<std::size_t>(std::min(
        remain, static_cast<std::uint64_t>(data_chunk_size - source_offset)));
    std::copy(std::next(source_buffer.begin(),
                        static_cast<std::int64_t>(source_offset)),
              std::next(source_buffer.begin(),
                        static_cast<std::int64_t>(source_offset + data_size)),
              std::next(resize_by(data, data_size).begin(),
                        static_cast<std::int64_t>(bytes_read)));
    remain -= data_size;
    bytes_read += data_size;
    source_offset = 0U;
  }

  return true;
}

auto read_encrypted_range(const http_range &range,
                          const utils::hash::hash_256_t &key, bool uses_kdf,
                          reader_func_t reader_func, std::uint64_t total_size,
                          data_buffer &data) -> bool {
  std::size_t bytes_read{};
  return read_encrypted_range<data_buffer>(
      range, key, reader_func, total_size, data,
      uses_kdf ? kdf_config::size() : 0U, bytes_read);
}

[[nodiscard]] auto read_encrypted_range(
    const http_range &range, const utils::hash::hash_256_t &key, bool uses_kdf,
    reader_func_t reader_func, std::uint64_t total_size, unsigned char *data,
    std::size_t size, std::size_t &bytes_read) -> bool {
  data_span dest_buffer(data, size);
  return read_encrypted_range<data_span>(
      range, key, reader_func, total_size, dest_buffer,
      uses_kdf ? kdf_config::size() : 0U, bytes_read);
}
} // namespace repertory::utils::encryption

#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined (PROJECT_ENABLE_BOOST)
