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

#if defined(PROJECT_ENABLE_BOOST) && defined(PROJECT_ENABLE_JSON)
#include "utils/collection.hpp"
#endif // defined(PROJECT_ENABLE_BOOST) && defined(PROJECT_ENABLE_JSON)
#include "utils/error.hpp"
#include "utils/hash.hpp"

namespace repertory::utils::encryption {
inline constexpr std::uint32_t encryption_header_size{
    crypto_aead_xchacha20poly1305_IETF_NPUBBYTES +
        crypto_aead_xchacha20poly1305_IETF_ABYTES,
};

#if defined(PROJECT_ENABLE_BOOST)
enum class kdf_version : std::uint8_t { v1 };

enum class kdf_type : std::uint8_t { argon2id };

enum class memlimit_level : std::uint8_t {
  level1, // 64MiB
  level2, // 256MiB
  level3, // 512MiB
  level4, // 1GiB
};

enum class opslimit_level : std::uint8_t {
  level1, // interactive
  level2, // moderate
  level3, // sensitive
};

[[nodiscard]] inline auto get_memlimit(memlimit_level memlimit) -> size_t {
  constexpr auto mib512{512ULL * 1024ULL * 1024ULL};

  switch (memlimit) {
  case memlimit_level::level1:
    return crypto_pwhash_MEMLIMIT_INTERACTIVE;

  case memlimit_level::level2:
    return crypto_pwhash_MEMLIMIT_MODERATE;

  case memlimit_level::level3:
    return mib512;

  case memlimit_level::level4:
    return crypto_pwhash_MEMLIMIT_SENSITIVE;
  }

  return mib512;
}

[[nodiscard]] inline auto get_opslimit(opslimit_level opslimit)
    -> unsigned long long {
  switch (opslimit) {
  case opslimit_level::level1:
    return crypto_pwhash_OPSLIMIT_INTERACTIVE;

  case opslimit_level::level2:
    return crypto_pwhash_OPSLIMIT_MODERATE;

  case opslimit_level::level3:
    return crypto_pwhash_OPSLIMIT_SENSITIVE;
  }

  return crypto_pwhash_OPSLIMIT_MODERATE;
}

enum class kdf_context : std::uint8_t {
  data,
  path,
  undefined,
};
using kdf_ctx_t = std::array<char, crypto_kdf_CONTEXTBYTES>;

namespace kdf {
constexpr inline std::array<
    kdf_ctx_t, static_cast<std::size_t>(kdf_context::undefined) + 1U>
    KDF_CTXS{
        {
            {'D', 'A', 'T', 'A', '_', 'C', 'T', 'X'},
            {'F', 'I', 'L', 'E', '_', 'C', 'T', 'X'},
            {'D', 'E', 'F', 'L', '_', 'C', 'T', 'X'},
        },
    };
} // namespace kdf

[[nodiscard]] constexpr inline auto get_kdf_context_name(kdf_context ctx)
    -> kdf_ctx_t {
  const auto idx = static_cast<std::size_t>(ctx);
  return idx < kdf::KDF_CTXS.size() ? kdf::KDF_CTXS.at(idx)
                                    : kdf::KDF_CTXS.back();
}

#pragma pack(push, 1)
struct kdf_config final {
  using salt_t = std::array<std::uint8_t, crypto_pwhash_SALTBYTES>;

  kdf_version version{kdf_version::v1};
  kdf_type kdf{kdf_type::argon2id};
  memlimit_level memlimit{memlimit_level::level3};
  opslimit_level opslimit{opslimit_level::level2};
  std::uint64_t unique_id{};
  salt_t salt{};
  std::uint64_t checksum{};

  template <typename hash_t>
  [[nodiscard]] auto create_subkey(kdf_context ctx, std::size_t unique_id_,
                                   const hash_t &master_key) const
      -> std::pair<hash_t, kdf_config> {
    auto sub_key = derive_subkey<hash_t>(ctx, unique_id_, master_key);

    auto cfg = *this;
    cfg.unique_id = unique_id_;
    cfg.checksum = cfg.generate_checksum();
    return {sub_key, cfg};
  }

  template <typename hash_t>
  [[nodiscard]] static auto derive_subkey(kdf_context ctx,
                                          std::size_t unique_id_,
                                          const hash_t &master_key) -> hash_t {
    REPERTORY_USES_FUNCTION_NAME();

    hash_t sub_key{};
    auto res = crypto_kdf_derive_from_key(
        sub_key.data(), sub_key.size(), unique_id_,
        get_kdf_context_name(ctx).data(), master_key.data());
    if (res != 0) {
      throw repertory::utils::error::create_exception(
          function_name, {
                             "failed to derive sub-key",
                             std::to_string(res),
                         });
    }

    return sub_key;
  }

  template <typename hash_t>
  [[nodiscard]] auto recreate_subkey(kdf_context ctx,
                                     const hash_t &master_key) const -> hash_t {
    return derive_subkey<hash_t>(ctx, unique_id, master_key);
  }

  [[nodiscard]] static auto from_header(data_cspan data, kdf_config &cfg,
                                        bool ignore_checksum = false) -> bool;

  [[nodiscard]] auto generate_checksum() const -> std::uint64_t;

  void seal();

  [[nodiscard]] static constexpr auto size() -> std::size_t {
    return sizeof(kdf_config);
  }

  [[nodiscard]] auto to_header() const -> data_buffer;

  [[nodiscard]] auto operator==(const kdf_config &) const -> bool = default;
  [[nodiscard]] auto operator!=(const kdf_config &) const -> bool = default;
};
#pragma pack(pop)
#endif // defined(PROJECT_ENABLE_BOOST)

template <typename hash_t>
[[nodiscard]] inline auto generate_key(
    std::string_view password,
    std::function<hash_t(const unsigned char *data, std::size_t size)> hasher =
        utils::hash::default_create_hash<hash_t>()) -> hash_t;

template <typename hash_t>
[[nodiscard]] inline auto generate_key(
    std::wstring_view password,
    std::function<hash_t(const unsigned char *data, std::size_t size)> hasher =
        utils::hash::default_create_hash<hash_t>()) -> hash_t;

#if defined(PROJECT_ENABLE_BOOST)
template <typename hash_t>
[[nodiscard]] inline auto generate_key(std::string_view password,
                                       kdf_config &cfg) -> hash_t;

template <typename hash_t>
[[nodiscard]] inline auto generate_key(std::wstring_view password,
                                       kdf_config &cfg) -> hash_t;

template <typename hash_t>
[[nodiscard]] inline auto recreate_key(std::string_view password,
                                       const kdf_config &cfg) -> hash_t;

template <typename hash_t>
[[nodiscard]] inline auto recreate_key(std::wstring_view password,
                                       const kdf_config &cfg) -> hash_t;

template <typename string_t>
[[nodiscard]] auto create_key_argon2id(string_t password, kdf_config &cfg,
                                       utils::hash::hash_256_t &key) -> bool;

template <typename string_t>
[[nodiscard]] auto recreate_key_argon2id(string_t password,
                                         const kdf_config &cfg,
                                         utils::hash::hash_256_t &key) -> bool;

template <typename hash_t, typename string_t>
[[nodiscard]] inline auto
detect_and_recreate_key(string_t password, data_cspan header, hash_t &key,
                        std::optional<kdf_config> &cfg) -> bool;

template <typename hash_t>
[[nodiscard]] inline auto
detect_and_recreate_key(std::string_view password, data_cspan header,
                        hash_t &key, std::optional<kdf_config> &cfg) -> bool;

template <typename hash_t>
[[nodiscard]] inline auto
detect_and_recreate_key(std::wstring_view password, data_cspan header,
                        hash_t &key, std::optional<kdf_config> &cfg) -> bool;

[[nodiscard]] auto decrypt_file_name(std::string_view encryption_token,
                                     std::string &file_name) -> bool;

[[nodiscard]] auto decrypt_file_path(std::string_view encryption_token,
                                     std::string &file_path) -> bool;

[[nodiscard]] auto decrypt_file_name(std::string_view encryption_token,
                                     const kdf_config &cfg,
                                     std::string &file_name) -> bool;

[[nodiscard]] auto decrypt_file_path(std::string_view encryption_token,
                                     const kdf_config &cfg,
                                     std::string &file_path) -> bool;

[[nodiscard]] auto decrypt_file_name(const utils::hash::hash_256_t &master_key,
                                     std::string &file_name) -> bool;

[[nodiscard]] auto decrypt_file_path(const utils::hash::hash_256_t &master_key,
                                     std::string &file_path) -> bool;

template <typename result_t, typename arr_t, std::size_t arr_size>
[[nodiscard]] inline auto decrypt_data(const std::array<arr_t, arr_size> &key,
                                       const unsigned char *buffer,
                                       std::size_t buffer_size, result_t &res)
    -> bool {
  if (buffer_size > encryption_header_size) {
    std::uint32_t size =
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
                                       const buffer_t &buf, result_t &res)
    -> bool {
  return decrypt_data<result_t>(
      key, reinterpret_cast<const unsigned char *>(buf.data()), buf.size(),
      res);
}

template <typename buffer_t, typename result_t,
          typename hash_t = utils::hash::hash_256_t>
[[nodiscard]] inline auto decrypt_data(
    std::string_view password, const buffer_t &buf, result_t &res,
    std::function<hash_t(const unsigned char *data, std::size_t size)> hasher =
        utils::hash::default_create_hash<hash_t>()) -> bool {
  return decrypt_data<buffer_t, result_t>(generate_key(password, hasher), buf,
                                          res);
}

template <typename buffer_t, typename result_t,
          typename hash_t = utils::hash::hash_256_t>
[[nodiscard]] inline auto decrypt_data(std::string_view password,
                                       const kdf_config &cfg,
                                       const buffer_t &buf, result_t &res)
    -> bool {
  return decrypt_data<buffer_t, result_t>(recreate_key<hash_t>(password, cfg),
                                          buf, res);
}

template <typename result_t, typename hash_t = utils::hash::hash_256_t>
[[nodiscard]] inline auto decrypt_data(
    std::string_view password, const unsigned char *buffer,
    std::size_t buffer_size, result_t &res,
    std::function<hash_t(const unsigned char *data, std::size_t size)> hasher =
        utils::hash::default_create_hash<hash_t>()) -> bool {
  return decrypt_data<result_t>(generate_key(password, hasher), buffer,
                                buffer_size, res);
}

template <typename result_t, typename hash_t = utils::hash::hash_256_t>
[[nodiscard]] inline auto decrypt_data(std::string_view password,
                                       const kdf_config &cfg,
                                       const unsigned char *buffer,
                                       std::size_t buffer_size, result_t &res)
    -> bool {
  return decrypt_data<result_t>(recreate_key<hash_t>(password, cfg), buffer,
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

template <typename result_t, typename hash_t = utils::hash::hash_256_t>
inline void encrypt_data(
    std::string_view password, const unsigned char *buffer,
    std::size_t buffer_size, result_t &res,
    std::function<hash_t(const unsigned char *data, std::size_t size)> hasher =
        utils::hash::default_create_hash<hash_t>()) {
  encrypt_data<result_t>(generate_key(password, hasher), buffer, buffer_size,
                         res);
}

template <typename result_t, typename hash_t = utils::hash::hash_256_t>
inline void encrypt_data(std::string_view password, kdf_config &cfg,
                         const unsigned char *buffer, std::size_t buffer_size,
                         result_t &res) {
  encrypt_data<result_t>(generate_key<hash_t>(password, cfg), buffer,
                         buffer_size, res);
}

template <typename buffer_t, typename result_t,
          typename hash_t = utils::hash::hash_256_t>
inline void encrypt_data(
    std::string_view password, const buffer_t &buf, result_t &res,
    std::function<hash_t(const unsigned char *data, std::size_t size)> hasher =
        utils::hash::default_create_hash<hash_t>()) {
  encrypt_data<result_t>(generate_key(password, hasher),
                         reinterpret_cast<const unsigned char *>(buf.data()),
                         buf.size(), res);
}

template <typename buffer_t, typename result_t,
          typename hash_t = utils::hash::hash_256_t>
inline void encrypt_data(std::string_view password, kdf_config &cfg,
                         const buffer_t &buf, result_t &res) {
  encrypt_data<result_t>(generate_key<hash_t>(password, cfg),
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

[[nodiscard]] auto read_encrypted_range(const http_range &range,
                                        const utils::hash::hash_256_t &key,
                                        bool uses_kdf,
                                        reader_func_t reader_func,
                                        std::uint64_t total_size,
                                        data_buffer &data) -> bool;

[[nodiscard]] auto read_encrypted_range(
    const http_range &range, const utils::hash::hash_256_t &key, bool uses_kdf,
    reader_func_t reader_func, std::uint64_t total_size, unsigned char *data,
    std::size_t size, std::size_t &bytes_read) -> bool;

[[nodiscard]] inline auto
read_encrypted_range(const http_range &range,
                     const utils::hash::hash_256_t &key,
                     reader_func_t reader_func, std::uint64_t total_size,
                     data_buffer &data) -> bool {
  return read_encrypted_range(range, key, false, reader_func, total_size, data);
}

[[nodiscard]] inline auto read_encrypted_range(
    const http_range &range, const utils::hash::hash_256_t &key,
    reader_func_t reader_func, std::uint64_t total_size, unsigned char *data,
    std::size_t size, std::size_t &bytes_read) -> bool {
  return read_encrypted_range(range, key, false, reader_func, total_size, data,
                              size, bytes_read);
}

template <typename string_t>
auto create_key_argon2id(string_t password, kdf_config &cfg,
                         utils::hash::hash_256_t &key) -> bool {
  cfg.seal();

  return recreate_key_argon2id(password, cfg, key);
}

template <typename string_t>
auto recreate_key_argon2id(string_t password, const kdf_config &cfg,
                           utils::hash::hash_256_t &key) -> bool {
  return crypto_pwhash(
             reinterpret_cast<unsigned char *>(key.data()), key.size(),
             reinterpret_cast<const char *>(password.data()),
             password.size() * sizeof(typename string_t::value_type),
             cfg.salt.data(), get_opslimit(cfg.opslimit),
             get_memlimit(cfg.memlimit), crypto_pwhash_ALG_ARGON2ID13) == 0;
}
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

#if defined(PROJECT_ENABLE_BOOST)
template <typename hash_t, typename string_t>
inline auto generate_key_impl(string_t password, kdf_config &cfg) -> hash_t {
  REPERTORY_USES_FUNCTION_NAME();

  switch (cfg.version) {
  case kdf_version::v1:
    switch (cfg.kdf) {
    case kdf_type::argon2id: {
      hash_t key{};
      if (not create_key_argon2id(password, cfg, key)) {
        throw utils::error::create_exception(
            function_name, {
                               "failed to generate argon2id key",
                           });
      }

      return key;
    }

    default:
      throw utils::error::create_exception(
          function_name, {
                             "unsupported kdf type",
                             std::to_string(static_cast<std::uint8_t>(cfg.kdf)),
                         });
    }

  default:
    throw utils::error::create_exception(
        function_name,
        {
            "unsupported kdf version",
            std::to_string(static_cast<std::uint8_t>(cfg.version)),
        });
  }
}

template <typename hash_t, typename string_t>
inline auto recreate_key_impl(string_t password, const kdf_config &cfg)
    -> hash_t {
  REPERTORY_USES_FUNCTION_NAME();

  switch (cfg.version) {
  case kdf_version::v1:
    switch (cfg.kdf) {
    case kdf_type::argon2id: {
      hash_t key{};
      if (not recreate_key_argon2id(password, cfg, key)) {
        throw utils::error::create_exception(
            function_name, {
                               "failed to generate argon2id key",
                           });
      }

      return key;
    }

    default:
      throw utils::error::create_exception(
          function_name, {
                             "unsupported kdf type",
                             std::to_string(static_cast<std::uint8_t>(cfg.kdf)),
                         });
    }

  default:
    throw utils::error::create_exception(
        function_name,
        {
            "unsupported kdf version",
            std::to_string(static_cast<std::uint8_t>(cfg.version)),
        });
  }
}

template <typename hash_t>
inline auto generate_key(std::string_view password, kdf_config &cfg) -> hash_t {
  return generate_key_impl<hash_t, std::string_view>(password, cfg);
}

template <typename hash_t>
inline auto generate_key(std::wstring_view password, kdf_config &cfg)
    -> hash_t {
  return generate_key_impl<hash_t, std::wstring_view>(password, cfg);
}

template <typename hash_t>
inline auto recreate_key(std::string_view password, const kdf_config &cfg)
    -> hash_t {
  return recreate_key_impl<hash_t, std::string_view>(password, cfg);
}

template <typename hash_t>
inline auto recreate_key(std::wstring_view password, const kdf_config &cfg)
    -> hash_t {
  return recreate_key_impl<hash_t, std::wstring_view>(password, cfg);
}

template <typename hash_t, typename string_t>
inline auto detect_and_recreate_key(string_t password, data_cspan header,
                                    hash_t &key, std::optional<kdf_config> &cfg)
    -> bool {
  if (header.size() >= kdf_config::size()) {
    kdf_config tmp{};
    if (kdf_config::from_header(header.first(kdf_config::size()), tmp)) {
      cfg = tmp;
      key = recreate_key<hash_t>(password, *cfg);
      return true;
    }
  }

  key = generate_key<hash_t>(password);
  return false;
}

template <typename hash_t>
inline auto detect_and_recreate_key(std::string_view password,
                                    data_cspan header, hash_t &key,
                                    std::optional<kdf_config> &cfg) -> bool {
  return detect_and_recreate_key<hash_t, std::string_view>(password, header,
                                                           key, cfg);
}

template <typename hash_t>
inline auto detect_and_recreate_key(std::wstring_view password,
                                    data_cspan header, hash_t &key,
                                    std::optional<kdf_config> &cfg) -> bool {
  return detect_and_recreate_key<hash_t, std::wstring_view>(password, header,
                                                            key, cfg);
}

#endif // defined(PROJECT_ENABLE_BOOST)
} // namespace repertory::utils::encryption

#if defined(PROJECT_ENABLE_BOOST) && defined(PROJECT_ENABLE_JSON)
NLOHMANN_JSON_NAMESPACE_BEGIN

namespace kdf {
inline constexpr std::string_view JSON_CHECKSUM{"checksum"};
inline constexpr std::string_view JSON_KDF{"kdf"};
inline constexpr std::string_view JSON_MEMLIMIT{"memlimit"};
inline constexpr std::string_view JSON_OPSLIMIT{"opslimit"};
inline constexpr std::string_view JSON_SALT{"salt"};
inline constexpr std::string_view JSON_UNIQUE_ID{"unique_id"};
inline constexpr std::string_view JSON_VERSION{"version"};
} // namespace kdf

template <>
struct adl_serializer<repertory::utils::encryption::kdf_config::salt_t> {
  static void
  to_json(json &data,
          const repertory::utils::encryption::kdf_config::salt_t &value) {
    data = repertory::utils::collection::to_hex_string(value);
  }

  static void
  from_json(const json &data,
            repertory::utils::encryption::kdf_config::salt_t &value) {
    REPERTORY_USES_FUNCTION_NAME();

    repertory::data_buffer buffer{};
    if (not repertory::utils::collection::from_hex_string(
            data.get<std::string>(), buffer)) {
      throw repertory::utils::error::create_exception(
          function_name, {
                             "failed to convert hex string to salt",
                             data.get<std::string>(),
                         });
    }

    if (buffer.size() != value.size()) {
      throw repertory::utils::error::create_exception(
          function_name, {
                             "unexpected length for salt after hex conversion",
                             "expected",
                             std::to_string(value.size()),
                             "actual",
                             std::to_string(buffer.size()),
                         });
    }

    std::copy_n(buffer.begin(), value.size(), value.begin());
  }
};

template <> struct adl_serializer<repertory::utils::encryption::kdf_config> {
  static void to_json(json &data,
                      const repertory::utils::encryption::kdf_config &value) {
    data[kdf::JSON_CHECKSUM] = value.checksum;
    data[kdf::JSON_KDF] = value.kdf;
    data[kdf::JSON_MEMLIMIT] = value.memlimit;
    data[kdf::JSON_OPSLIMIT] = value.opslimit;
    data[kdf::JSON_SALT] = value.salt;
    data[kdf::JSON_UNIQUE_ID] = value.unique_id;
    data[kdf::JSON_VERSION] = value.version;
  }

  static void from_json(const json &data,
                        repertory::utils::encryption::kdf_config &value) {
    data.at(kdf::JSON_CHECKSUM).get_to<std::uint64_t>(value.checksum);
    data.at(kdf::JSON_KDF)
        .get_to<repertory::utils::encryption::kdf_type>(value.kdf);
    data.at(kdf::JSON_MEMLIMIT)
        .get_to<repertory::utils::encryption::memlimit_level>(value.memlimit);
    data.at(kdf::JSON_OPSLIMIT)
        .get_to<repertory::utils::encryption::opslimit_level>(value.opslimit);
    data.at(kdf::JSON_SALT)
        .get_to<repertory::utils::encryption::kdf_config::salt_t>(value.salt);
    data.at(kdf::JSON_UNIQUE_ID).get_to<std::uint64_t>(value.unique_id);
    data.at(kdf::JSON_VERSION)
        .get_to<repertory::utils::encryption::kdf_version>(value.version);
  }
};
NLOHMANN_JSON_NAMESPACE_END
#endif // defined(PROJECT_ENABLE_BOOST) && defined(PROJECT_ENABLE_JSON)

#endif // defined(PROJECT_ENABLE_LIBSODIUM)
#endif // REPERTORY_INCLUDE_UTILS_ENCRYPTION_HPP_
