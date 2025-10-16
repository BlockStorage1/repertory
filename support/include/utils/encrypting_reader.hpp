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
#ifndef REPERTORY_INCLUDE_UTILS_ENCRYPTING_READER_HPP_
#define REPERTORY_INCLUDE_UTILS_ENCRYPTING_READER_HPP_
#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)

#include "utils/config.hpp"

#include "utils/encryption.hpp"
#include "utils/hash.hpp"
#include "utils/types/file/i_file.hpp"

namespace repertory::utils::encryption {
class encrypting_reader final {
public:
  encrypting_reader(std::string_view file_name, std::string_view source_path,
                    stop_type_callback stop_requested_cb,
                    std::string_view token,
                    std::optional<std::string> relative_parent_path,
                    std::size_t error_return = 0U);

  encrypting_reader(stop_type_callback stop_requested_cb,
                    std::string_view encrypted_file_path,
                    std::string_view source_path, std::string_view token,
                    std::size_t error_return = 0U);

  encrypting_reader(
      stop_type_callback stop_requested_cb,
      std::string_view encrypted_file_path, std::string_view source_path,
      std::string_view token,
      std::vector<std::array<unsigned char,
                             crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>>
          iv_list,
      std::size_t error_return = 0U);

  encrypting_reader(std::string_view file_name, std::string_view source_path,
                    stop_type_callback stop_requested_cb,
                    std::string_view token, kdf_config cfg,
                    std::optional<std::string> relative_parent_path,
                    std::size_t error_return = 0U);

  encrypting_reader(stop_type_callback stop_requested_cb,
                    std::string_view encrypted_file_path,
                    std::string_view source_path, std::string_view token,
                    kdf_config cfg, std::size_t error_return = 0U);

  encrypting_reader(
      stop_type_callback stop_requested_cb,
      std::string_view encrypted_file_path, std::string_view source_path,
      std::string_view token, kdf_config cfg,
      std::vector<std::array<unsigned char,
                             crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>>
          iv_list,
      std::size_t error_return = 0U);

  encrypting_reader(std::string_view file_name, std::string_view source_path,
                    stop_type_callback stop_requested_cb,
                    const utils::hash::hash_256_t &master_key,
                    const kdf_config &cfg,
                    std::optional<std::string> relative_parent_path,
                    std::size_t error_return = 0U);

  encrypting_reader(std::string_view file_name, std::string_view source_path,
                    stop_type_callback stop_requested_cb,
                    const utils::hash::hash_256_t &master_key,
                    const std::pair<kdf_config, kdf_config> &configs,
                    std::optional<std::string> relative_parent_path,
                    std::size_t error_return = 0U);

  encrypting_reader(stop_type_callback stop_requested_cb,
                    std::string_view encrypted_file_path,
                    std::string_view source_path,
                    const utils::hash::hash_256_t &master_key,
                    const kdf_config &cfg, std::size_t error_return = 0U);

  encrypting_reader(
      stop_type_callback stop_requested_cb,
      std::string_view encrypted_file_path, std::string_view source_path,
      const utils::hash::hash_256_t &master_key, const kdf_config &cfg,
      std::vector<std::array<unsigned char,
                             crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>>
          iv_list,
      std::size_t error_return = 0U);

  encrypting_reader(
      stop_type_callback stop_requested_cb,
      std::string_view encrypted_file_path, std::string_view source_path,
      const utils::hash::hash_256_t &master_key,
      const std::pair<kdf_config, kdf_config> &configs,
      std::vector<std::array<unsigned char,
                             crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>>
          iv_list,
      std::size_t error_return = 0U);

  encrypting_reader(const encrypting_reader &reader);
  encrypting_reader(encrypting_reader &&) = delete;

  auto operator=(const encrypting_reader &) -> encrypting_reader & = delete;
  auto operator=(encrypting_reader &&) -> encrypting_reader & = delete;

  ~encrypting_reader() noexcept = default;

public:
  using iostream = std::basic_iostream<char, std::char_traits<char>>;
  using kdf_pair_t = std::pair<data_buffer, data_buffer>;
  using key_pair_t =
      std::pair<utils::hash::hash_256_t, utils::hash::hash_256_t>;
  using streambuf = std::basic_streambuf<char, std::char_traits<char>>;

private:
  key_pair_t keys_;
  stop_type_callback stop_requested_cb_;
  size_t error_return_;
  std::unique_ptr<utils::file::i_file> source_file_;
  std::string encrypted_file_name_;
  std::string encrypted_file_path_;
  std::vector<
      std::array<unsigned char, crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>>
      iv_list_;

private:
  std::unordered_map<std::size_t, data_buffer> chunk_buffers_;
  std::optional<kdf_pair_t> kdf_headers_;
  std::size_t last_data_chunk_{};
  std::size_t last_data_chunk_size_{};
  std::uint64_t read_offset_{};
  std::uint64_t total_size_{};

private:
  static const std::size_t header_size_;
  static const std::size_t data_chunk_size_;
  static const std::size_t encrypted_chunk_size_;

private:
  auto reader_function(char *buffer, size_t size, size_t nitems) -> size_t;

  void common_initialize(bool procces_iv_list);

  void common_initialize_kdf_data(const kdf_config &cfg,
                                  const utils::hash::hash_256_t &master_key);

  void common_initialize_kdf_keys(std::string_view token, kdf_config &cfg);

  void common_initialize_kdf_path(const utils::hash::hash_256_t &master_key);

  void create_encrypted_paths(std::string_view file_name,
                              std::optional<std::string> relative_parent_path);

public:
  [[nodiscard]] static auto calculate_decrypted_size(std::uint64_t total_size,
                                                     bool uses_kdf)
      -> std::uint64_t;

  [[nodiscard]] static auto
  calculate_encrypted_size(std::string_view source_path, bool uses_kdf)
      -> std::uint64_t;

  [[nodiscard]] static auto calculate_encrypted_size(std::uint64_t size,
                                                     bool uses_kdf)
      -> std::uint64_t;

  [[nodiscard]] auto create_iostream() const -> std::shared_ptr<iostream>;

  [[nodiscard]] static constexpr auto get_encrypted_chunk_size()
      -> std::size_t {
    return encrypted_chunk_size_;
  }

  [[nodiscard]] static constexpr auto get_data_chunk_size() -> std::size_t {
    return data_chunk_size_;
  }

  [[nodiscard]] auto get_encrypted_file_name() const -> std::string {
    return encrypted_file_name_;
  }

  [[nodiscard]] auto get_encrypted_file_path() const -> std::string {
    return encrypted_file_path_;
  }

  [[nodiscard]] auto get_error_return() const -> std::size_t {
    return error_return_;
  }

  [[nodiscard]] static constexpr auto get_header_size() -> std::size_t {
    return header_size_;
  }

  [[nodiscard]] auto get_iv_list() -> std::vector<
      std::array<unsigned char, crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>> {
    return iv_list_;
  }

  [[nodiscard]] auto get_kdf_config_for_data() const
      -> std::optional<kdf_config>;

  [[nodiscard]] auto get_kdf_config_for_path() const
      -> std::optional<kdf_config>;

  [[nodiscard]] auto get_stop_requested() const -> bool {
    return stop_requested_cb_();
  }

  [[nodiscard]] auto get_total_size() const -> std::uint64_t {
    return total_size_;
  }

  [[nodiscard]] static auto reader_function(char *buffer, size_t size,
                                            size_t nitems, void *instream)
      -> size_t {
    return reinterpret_cast<encrypting_reader *>(instream)->reader_function(
        buffer, size, nitems);
  }

  void set_read_position(std::uint64_t position) { read_offset_ = position; }
};
} // namespace repertory::utils::encryption

#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
#endif // REPERTORY_INCLUDE_UTILS_ENCRYPTING_READER_HPP_
