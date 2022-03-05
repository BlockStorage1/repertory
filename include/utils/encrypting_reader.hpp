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
#ifndef INCLUDE_UTILS_ENCRYPTING_READER_HPP_
#define INCLUDE_UTILS_ENCRYPTING_READER_HPP_

#include "common.hpp"
#include "utils/file_utils.hpp"

namespace repertory::utils::encryption {
class encrypting_reader final {
public:
  encrypting_reader(const std::string &file_name, const std::string &source_path,
                    const bool &stop_requested, const std::string &token,
                    const size_t error_return = 0);

  encrypting_reader(const encrypting_reader &r);

  ~encrypting_reader();

public:
  typedef std::basic_iostream<char, std::char_traits<char>> iostream;
  typedef std::basic_streambuf<char, std::char_traits<char>> streambuf;

private:
  const CryptoPP::SecByteBlock key_;
  const bool &stop_requested_;
  const size_t error_return_;
  std::unordered_map<std::size_t, std::vector<char>> chunk_buffers_;
  std::string encrypted_file_name_;
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
  size_t reader_function(char *buffer, size_t size, size_t nitems);

public:
  static std::uint64_t calculate_decrypted_size(const std::uint64_t &total_size);

  std::shared_ptr<iostream> create_iostream() const;

  static constexpr const std::size_t &get_encrypted_chunk_size() { return encrypted_chunk_size_; }

  static constexpr const std::size_t &get_data_chunk_size() { return data_chunk_size_; }

  std::string get_encrypted_file_name() const { return encrypted_file_name_; }

  std::size_t get_error_return() const { return error_return_; }

  static constexpr const std::size_t &get_header_size() { return header_size_; }

  const bool &get_stop_requested() const { return stop_requested_; }

  std::uint64_t get_total_size() const { return total_size_; }

  static size_t reader_function(char *buffer, size_t size, size_t nitems, void *instream) {
    return reinterpret_cast<encrypting_reader *>(instream)->reader_function(buffer, size, nitems);
  }

  void set_read_position(const std::uint64_t &position) { read_offset_ = position; }
};
} // namespace repertory::utils::encryption

#endif // INCLUDE_UTILS_ENCRYPTING_READER_HPP_
