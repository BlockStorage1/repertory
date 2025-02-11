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
#ifndef REPERTORY_INCLUDE_COMM_PACKET_PACKET_HPP_
#define REPERTORY_INCLUDE_COMM_PACKET_PACKET_HPP_

#include "types/remote.hpp"
#include "types/repertory.hpp"

namespace repertory {
#define DECODE_OR_RETURN(p, value)                                             \
  if ((ret = (p)->decode(value)) != 0)                                         \
  return ret
#define DECODE_OR_IGNORE(p, value)                                             \
  if (ret == 0)                                                                \
  ret = (p)->decode(value)

class packet final {
public:
  using error_type = std::int32_t;

public:
  packet() = default;

  explicit packet(data_buffer buffer) : buffer_(std::move(buffer)) {}

  explicit packet(data_buffer &&buffer) : buffer_(std::move(buffer)) {}

  packet(const packet &pkt) noexcept = default;

  packet(packet &&pkt) noexcept
      : buffer_(std::move(pkt.buffer_)), decode_offset_(pkt.decode_offset_) {}

  ~packet() = default;

private:
  data_buffer buffer_{};
  std::size_t decode_offset_{0U};

public:
  [[nodiscard]] static auto decode_json(packet &response, json &json_data)
      -> int;

public:
  void clear();

  [[nodiscard]] auto current_pointer() -> unsigned char * {
    return (decode_offset_ < buffer_.size()) ? &buffer_.at(decode_offset_)
                                             : nullptr;
  }

  [[nodiscard]] auto current_pointer() const -> const unsigned char * {
    return (decode_offset_ < buffer_.size()) ? &buffer_.at(decode_offset_)
                                             : nullptr;
  }

  [[nodiscard]] auto decode(std::string &data) -> error_type;

  [[nodiscard]] auto decode(std::wstring &data) -> error_type;

  [[nodiscard]] auto decode(void *buffer, std::size_t size) -> error_type;

  [[nodiscard]] auto decode(void *&ptr) -> error_type;

  [[nodiscard]] auto decode(std::int8_t &val) -> error_type;

  [[nodiscard]] auto decode(std::uint8_t &val) -> error_type;

  [[nodiscard]] auto decode(std::int16_t &val) -> error_type;

  [[nodiscard]] auto decode(std::uint16_t &val) -> error_type;

  [[nodiscard]] auto decode(std::int32_t &val) -> error_type;

  [[nodiscard]] auto decode(std::uint32_t &val) -> error_type;

  [[nodiscard]] auto decode(std::int64_t &val) -> error_type;

  [[nodiscard]] auto decode(std::uint64_t &val) -> error_type;

  [[nodiscard]] auto decode(remote::open_flags &val) -> error_type {
    return decode(reinterpret_cast<std::uint32_t &>(val));
  }

  [[nodiscard]] auto decode(remote::setattr_x &val) -> error_type;

  [[nodiscard]] auto decode(remote::stat &val) -> error_type;

  [[nodiscard]] auto decode(remote::statfs &val) -> error_type;

  [[nodiscard]] auto decode(remote::statfs_x &val) -> error_type;

  [[nodiscard]] auto decode(remote::file_info &val) -> error_type;

  [[nodiscard]] auto decrypt(std::string_view token) -> error_type;

  void encode(const void *buffer, std::size_t size, bool should_reserve = true);

  void encode(const char *str) {
    encode(std::string{str == nullptr ? "" : str});
  }

  void encode(std::string_view str);

  void encode(const wchar_t *str);

  void encode(std::wstring_view str);

  void encode(void *ptr) {
    encode(static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(ptr)));
  }

  void encode(std::int8_t val);

  void encode(std::uint8_t val);

  void encode(std::int16_t val);

  void encode(std::uint16_t val);

  void encode(std::int32_t val);

  void encode(std::uint32_t val);

  void encode(std::int64_t val);

  void encode(std::uint64_t val);

  void encode(remote::open_flags val) {
    encode(static_cast<std::uint32_t>(val));
  }

  void encode(remote::setattr_x val);

  void encode(remote::stat val);

  void encode(remote::statfs val, bool should_reserve = true);

  void encode(remote::statfs_x val);

  void encode(remote::file_info val);

  void encode_top(const void *buffer, std::size_t size,
                  bool should_reserve = true);

  void encode_top(std::string_view str);

  void encode_top(std::wstring_view str);

  void encode_top(void *ptr) {
    encode_top(
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(ptr)));
  }

  void encode_top(std::int8_t val);

  void encode_top(std::uint8_t val);

  void encode_top(std::int16_t val);

  void encode_top(std::uint16_t val);

  void encode_top(std::int32_t val);

  void encode_top(std::uint32_t val);

  void encode_top(std::int64_t val);

  void encode_top(std::uint64_t val);

  void encode_top(remote::open_flags val) {
    encode_top(static_cast<std::uint32_t>(val));
  }

  void encode_top(remote::setattr_x val);

  void encode_top(remote::stat val);

  void encode_top(remote::statfs val, bool should_reserve = true);

  void encode_top(remote::statfs_x val);

  void encode_top(remote::file_info val);

  void encrypt(std::string_view token);

  [[nodiscard]] auto get_size() const -> std::uint32_t {
    return static_cast<std::uint32_t>(buffer_.size());
  }

  void to_buffer(data_buffer &buffer);

public:
  auto operator=(const data_buffer &buffer) noexcept -> packet &;

  auto operator=(data_buffer &&buffer) noexcept -> packet &;

  auto operator=(const packet &pkt) noexcept -> packet &;

  auto operator=(packet &&pkt) noexcept -> packet &;

  [[nodiscard]] auto operator[](std::size_t index) -> unsigned char & {
    return buffer_[index];
  }

  [[nodiscard]] auto operator[](std::size_t index) const -> const
      unsigned char & {
    return buffer_.at(index);
  }
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_COMM_PACKET_PACKET_HPP_
