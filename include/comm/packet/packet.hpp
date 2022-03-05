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
#ifndef INCLUDE_COMM_PACKET_PACKET_HPP_
#define INCLUDE_COMM_PACKET_PACKET_HPP_

#include "common.hpp"
#include "types/remote.hpp"
#include "types/repertory.hpp"

namespace repertory {
#define DECODE_OR_RETURN(p, value)                                                                 \
  if ((ret = (p)->decode(value)) != 0)                                                             \
  return ret
#define DECODE_OR_IGNORE(p, value)                                                                 \
  if (ret == 0)                                                                                    \
  ret = (p)->decode(value)

class packet final {
public:
  typedef std::int32_t error_type;

public:
  packet() = default;

  explicit packet(std::vector<char> buffer) : buffer_(std::move(buffer)) {}

  explicit packet(std::vector<char> &&buffer) : buffer_(std::move(buffer)) {}

  packet(const packet &p) noexcept : buffer_(p.buffer_), decode_offset_(p.decode_offset_) {}

  packet(packet &&p) noexcept : buffer_(std::move(p.buffer_)), decode_offset_(p.decode_offset_) {}

private:
  std::vector<char> buffer_;
  std::size_t decode_offset_ = 0u;

public:
  static int decode_json(packet &response, json &json_data);

public:
  void clear();

  char *current_pointer() {
    return (decode_offset_ < buffer_.size()) ? &buffer_[decode_offset_] : nullptr;
  }

  const char *current_pointer() const {
    return (decode_offset_ < buffer_.size()) ? &buffer_[decode_offset_] : nullptr;
  }

  error_type decode(std::string &data);

  error_type decode(std::wstring &data);

  error_type decode(void *buffer, const size_t &size);

  error_type decode(void *&ptr);

  error_type decode(std::int8_t &i);

  error_type decode(std::uint8_t &i);

  error_type decode(std::int16_t &i);

  error_type decode(std::uint16_t &i);

  error_type decode(std::int32_t &i);

  error_type decode(std::uint32_t &i);

  error_type decode(std::int64_t &i);

  error_type decode(std::uint64_t &i);

  error_type decode(remote::open_flags &i) {
    return decode(reinterpret_cast<std::uint32_t &>(i));
  }

  error_type decode(remote::setattr_x &i);

  error_type decode(remote::stat &i);

  error_type decode(remote::statfs &i);

  error_type decode(remote::statfs_x &i);

  error_type decode(remote::file_info &i);

  error_type decrypt(const std::string &token);

  void encode(const void *buffer, const std::size_t &size, bool should_reserve = true);

  void encode(char *str) { encode(std::string(str ? str : "")); }

  void encode(const char *str) { encode(std::string(str ? str : "")); }

  void encode(const std::string &str);

  void encode(wchar_t *str);

  void encode(const wchar_t *str);

  void encode(const std::wstring &str);

  void encode(void *ptr) {
    encode(static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(ptr)));
  }

  void encode(std::int8_t i);

  void encode(std::uint8_t i);

  void encode(std::int16_t i);

  void encode(std::uint16_t i);

  void encode(std::int32_t i);

  void encode(std::uint32_t i);

  void encode(std::int64_t i);

  void encode(std::uint64_t i);

  void encode(remote::open_flags i) { encode(static_cast<std::uint32_t>(i)); }

  void encode(remote::setattr_x i);

  void encode(remote::stat i);

  void encode(remote::statfs i, bool should_reserve = true);

  void encode(remote::statfs_x i);

  void encode(remote::file_info i);

  void encode_top(const void *buffer, const std::size_t &size, bool should_reserve = true);

  void encode_top(const std::string &str);

  void encode_top(const std::wstring &str);

  void encode_top(void *ptr) {
    encode_top(static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(ptr)));
  }

  void encode_top(std::int8_t i);

  void encode_top(std::uint8_t i);

  void encode_top(std::int16_t i);

  void encode_top(std::uint16_t i);

  void encode_top(std::int32_t i);

  void encode_top(std::uint32_t i);

  void encode_top(std::int64_t i);

  void encode_top(std::uint64_t i);

  void encode_top(remote::open_flags i) { encode_top(static_cast<std::uint32_t>(i)); }

  void encode_top(remote::setattr_x i);

  void encode_top(remote::stat i);

  void encode_top(remote::statfs i, bool should_reserve = true);

  void encode_top(remote::statfs_x i);

  void encode_top(remote::file_info i);

  void encrypt(const std::string &token);

  std::uint32_t get_size() const { return static_cast<std::uint32_t>(buffer_.size()); }

  void transfer_into(std::vector<char> &buffer);

public:
  packet &operator=(const std::vector<char> &buffer) noexcept;

  packet &operator=(std::vector<char> &&buffer) noexcept;

  packet &operator=(const packet &p) noexcept;

  packet &operator=(packet &&p) noexcept;

  char &operator[](const size_t &index) { return buffer_[index]; }

  const char &operator[](const size_t &index) const { return buffer_.at(index); }
};
typedef packet packet;
} // namespace repertory

#endif // INCLUDE_COMM_PACKET_PACKET_HPP_
