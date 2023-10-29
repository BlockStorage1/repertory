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
#ifndef INCLUDE_UTILS_ENCRYPTING_READER_HPP_
#define INCLUDE_UTILS_ENCRYPTING_READER_HPP_

#include "types/repertory.hpp"
#include "utils/file_utils.hpp"

namespace repertory::utils::encryption {
using key_type = std::array<unsigned char, 32U>;

class encrypting_reader final {
public:
  encrypting_reader(
      const std::string &file_name, const std::string &source_path,
      stop_type &stop_requested, const std::string &token,
      std::optional<std::string> relative_parent_path = std::nullopt,
      const size_t error_return = 0);

  encrypting_reader(
      const std::string &encrypted_file_path, const std::string &source_path,
      stop_type &stop_requested, const std::string &token,
      std::vector<std::array<unsigned char,
                             crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>>
          iv_list,
      const size_t error_return = 0);

  encrypting_reader(const encrypting_reader &r);

  ~encrypting_reader();

public:
  using iostream = std::basic_iostream<char, std::char_traits<char>>;
  using streambuf = std::basic_streambuf<char, std::char_traits<char>>;

private:
  const key_type key_;
  stop_type &stop_requested_;
  const size_t error_return_;
  std::unordered_map<std::size_t, data_buffer> chunk_buffers_;
  std::string encrypted_file_name_;
  std::string encrypted_file_path_;
  std::vector<
      std::array<unsigned char, crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>>
      iv_list_;
  std::size_t last_data_chunk_ = 0u;
  std::size_t last_data_chunk_size_ = 0u;
  std::uint64_t read_offset_ = 0u;
  native_file_ptr source_file_;
  std::uint64_t total_size_ = 0u;

private:
  static const std::size_t header_size_;
  static const std::size_t data_chunk_size_;
  static const std::size_t encrypted_chunk_size_;

private:
  auto reader_function(char *buffer, size_t size, size_t nitems) -> size_t;

public:
  [[nodiscard]] static auto calculate_decrypted_size(std::uint64_t total_size)
      -> std::uint64_t;

  [[nodiscard]] static auto
  calculate_encrypted_size(const std::string &source_path) -> std::uint64_t;

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

  [[nodiscard]] auto get_stop_requested() const -> bool {
    return stop_requested_;
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

#endif // INCLUDE_UTILS_ENCRYPTING_READER_HPP_
