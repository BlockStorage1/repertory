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
#include "comm/packet/packet.hpp"

#include "events/event_system.hpp"
#include "events/events.hpp"
#include "types/remote.hpp"
#include "types/repertory.hpp"
#include "utils/encryption.hpp"
#include "utils/error_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
void packet::clear() {
  buffer_.clear();
  decode_offset_ = 0U;
}

auto packet::decode(std::string &data) -> packet::error_type {
  const auto *str = &buffer_[decode_offset_];
  const auto length = strnlen(str, buffer_.size() - decode_offset_);
  data = std::string(str, length);
  decode_offset_ += (length + 1);

  return utils::from_api_error(api_error::success);
}

auto packet::decode(std::wstring &data) -> packet::error_type {
  std::string utf8_string;
  const auto ret = decode(utf8_string);
  if (ret == 0) {
    data = utils::string::from_utf8(utf8_string);
  }

  return utils::from_api_error(api_error::success);
}

auto packet::decode(void *&ptr) -> packet::error_type {
  return decode(reinterpret_cast<std::uint64_t &>(ptr));
}

auto packet::decode(void *buffer, std::size_t size) -> packet::error_type {
  if (size != 0U) {
    const auto read_size =
        utils::calculate_read_size(buffer_.size(), size, decode_offset_);
    if (read_size == size) {
      memcpy(buffer, &buffer_[decode_offset_], size);
      decode_offset_ += size;
      return utils::from_api_error(api_error::success);
    }

    return ((decode_offset_ + size) > buffer_.size())
               ? utils::from_api_error(api_error::buffer_overflow)
               : utils::from_api_error(api_error::buffer_too_small);
  }
  return utils::from_api_error(api_error::success);
}

auto packet::decode(std::int8_t &val) -> packet::error_type {
  const auto ret = decode(&val, sizeof(val));
  if (ret == 0) {
    boost::endian::big_to_native_inplace(val);
  }
  return ret;
}

auto packet::decode(std::uint8_t &val) -> packet::error_type {
  const auto ret = decode(&val, sizeof(val));
  if (ret == 0) {
    boost::endian::big_to_native_inplace(val);
  }
  return ret;
}

auto packet::decode(std::int16_t &val) -> packet::error_type {
  const auto ret = decode(&val, sizeof(val));
  if (ret == 0) {
    boost::endian::big_to_native_inplace(val);
  }
  return ret;
}

auto packet::decode(std::uint16_t &val) -> packet::error_type {
  const auto ret = decode(&val, sizeof(val));
  if (ret == 0) {
    boost::endian::big_to_native_inplace(val);
  }
  return ret;
}

auto packet::decode(std::int32_t &val) -> packet::error_type {
  const auto ret = decode(&val, sizeof(val));
  if (ret == 0) {
    boost::endian::big_to_native_inplace(val);
  }
  return ret;
}

auto packet::decode(std::uint32_t &val) -> packet::error_type {
  const auto ret = decode(&val, sizeof(val));
  if (ret == 0) {
    boost::endian::big_to_native_inplace(val);
  }
  return ret;
}

auto packet::decode(std::int64_t &val) -> packet::error_type {
  const auto ret = decode(&val, sizeof(val));
  if (ret == 0) {
    boost::endian::big_to_native_inplace(val);
  }
  return ret;
}

auto packet::decode(std::uint64_t &val) -> packet::error_type {
  const auto ret = decode(&val, sizeof(val));
  if (ret == 0) {
    boost::endian::big_to_native_inplace(val);
  }
  return ret;
}

auto packet::decode(remote::setattr_x &val) -> packet::error_type {
  const auto ret = decode(&val, sizeof(val));
  if (ret == 0) {
    boost::endian::big_to_native_inplace(val.acctime);
    boost::endian::big_to_native_inplace(val.bkuptime);
    boost::endian::big_to_native_inplace(val.chgtime);
    boost::endian::big_to_native_inplace(val.crtime);
    boost::endian::big_to_native_inplace(val.flags);
    boost::endian::big_to_native_inplace(val.gid);
    boost::endian::big_to_native_inplace(val.mode);
    boost::endian::big_to_native_inplace(val.modtime);
    boost::endian::big_to_native_inplace(val.size);
    boost::endian::big_to_native_inplace(val.uid);
    boost::endian::big_to_native_inplace(val.valid);
  }

  return ret;
}

auto packet::decode(remote::stat &val) -> packet::error_type {
  const auto ret = decode(&val, sizeof(val));
  if (ret == 0) {
    boost::endian::big_to_native_inplace(val.st_mode);
    boost::endian::big_to_native_inplace(val.st_nlink);
    boost::endian::big_to_native_inplace(val.st_uid);
    boost::endian::big_to_native_inplace(val.st_gid);
    boost::endian::big_to_native_inplace(val.st_atimespec);
    boost::endian::big_to_native_inplace(val.st_mtimespec);
    boost::endian::big_to_native_inplace(val.st_ctimespec);
    boost::endian::big_to_native_inplace(val.st_birthtimespec);
    boost::endian::big_to_native_inplace(val.st_size);
    boost::endian::big_to_native_inplace(val.st_blocks);
    boost::endian::big_to_native_inplace(val.st_blksize);
    boost::endian::big_to_native_inplace(val.st_flags);
  }

  return ret;
}

auto packet::decode(remote::statfs &val) -> packet::error_type {
  const auto ret = decode(&val, sizeof(val));
  if (ret == 0) {
    boost::endian::big_to_native_inplace(val.f_bavail);
    boost::endian::big_to_native_inplace(val.f_bfree);
    boost::endian::big_to_native_inplace(val.f_blocks);
    boost::endian::big_to_native_inplace(val.f_favail);
    boost::endian::big_to_native_inplace(val.f_ffree);
    boost::endian::big_to_native_inplace(val.f_files);
  }
  return ret;
}

auto packet::decode(remote::statfs_x &val) -> packet::error_type {
  auto ret = decode(*dynamic_cast<remote::statfs *>(&val));
  if (ret == 0) {
    ret = decode(&val.f_mntfromname[0U], sizeof(val.f_mntfromname));
  }
  return ret;
}

auto packet::decode(remote::file_info &val) -> packet::error_type {
  const auto ret = decode(&val, sizeof(val));
  if (ret == 0) {
    boost::endian::big_to_native_inplace(val.AllocationSize);
    boost::endian::big_to_native_inplace(val.ChangeTime);
    boost::endian::big_to_native_inplace(val.CreationTime);
    boost::endian::big_to_native_inplace(val.EaSize);
    boost::endian::big_to_native_inplace(val.FileAttributes);
    boost::endian::big_to_native_inplace(val.FileSize);
    boost::endian::big_to_native_inplace(val.HardLinks);
    boost::endian::big_to_native_inplace(val.IndexNumber);
    boost::endian::big_to_native_inplace(val.LastAccessTime);
    boost::endian::big_to_native_inplace(val.LastWriteTime);
    boost::endian::big_to_native_inplace(val.ReparseTag);
  }

  return ret;
}

auto packet::decode_json(packet &response, json &json_data) -> int {
  std::string data;
  auto ret = response.decode(data);
  if (ret == 0) {
    try {
      json_data = json::parse(data);
    } catch (const std::exception &e) {
      utils::error::raise_error(__FUNCTION__, e, "failed to parse json string");
      ret = -EIO;
    }
  }

  return ret;
}

auto packet::decrypt(const std::string &token) -> packet::error_type {
  auto ret = utils::from_api_error(api_error::success);
  try {
    data_buffer result;
    if (not utils::encryption::decrypt_data(token, &buffer_[decode_offset_],
                                            buffer_.size() - decode_offset_,
                                            result)) {
      throw std::runtime_error("decryption failed");
    }
    buffer_ = std::move(result);
    decode_offset_ = 0;
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
    ret = utils::from_api_error(api_error::error);
  }

  return ret;
}

void packet::encode(const void *buffer, std::size_t size, bool should_reserve) {
  if (size != 0U) {
    if (should_reserve) {
      buffer_.reserve(buffer_.size() + size);
    }
    const auto *char_buffer = reinterpret_cast<const char *>(buffer);
    buffer_.insert(buffer_.end(), char_buffer, char_buffer + size);
  }
}

void packet::encode(const std::string &str) {
  const auto len = strnlen(str.c_str(), str.size());
  buffer_.reserve(len + 1 + buffer_.size());
  encode(str.c_str(), len, false);
  buffer_.emplace_back(0);
}

void packet::encode(const wchar_t *str) {
  encode(utils::string::to_utf8(str == nullptr ? L"" : str));
}

void packet::encode(const std::wstring &str) {
  encode(utils::string::to_utf8(str));
}

void packet::encode(std::int8_t val) {
  boost::endian::native_to_big_inplace(val);
  encode(&val, sizeof(val), true);
}

void packet::encode(std::uint8_t val) {
  boost::endian::native_to_big_inplace(val);
  encode(&val, sizeof(val), true);
}

void packet::encode(std::int16_t val) {
  boost::endian::native_to_big_inplace(val);
  encode(&val, sizeof(val), true);
}

void packet::encode(std::uint16_t val) {
  boost::endian::native_to_big_inplace(val);
  encode(&val, sizeof(val), true);
}

void packet::encode(std::int32_t val) {
  boost::endian::native_to_big_inplace(val);
  encode(&val, sizeof(val), true);
}

void packet::encode(std::uint32_t val) {
  boost::endian::native_to_big_inplace(val);
  encode(&val, sizeof(val), true);
}

void packet::encode(std::int64_t val) {
  boost::endian::native_to_big_inplace(val);
  encode(&val, sizeof(val), true);
}

void packet::encode(std::uint64_t val) {
  boost::endian::native_to_big_inplace(val);
  encode(&val, sizeof(val), true);
}

void packet::encode(remote::setattr_x val) {
  boost::endian::native_to_big_inplace(val.acctime);
  boost::endian::native_to_big_inplace(val.bkuptime);
  boost::endian::native_to_big_inplace(val.chgtime);
  boost::endian::native_to_big_inplace(val.crtime);
  boost::endian::native_to_big_inplace(val.flags);
  boost::endian::native_to_big_inplace(val.gid);
  boost::endian::native_to_big_inplace(val.mode);
  boost::endian::native_to_big_inplace(val.modtime);
  boost::endian::native_to_big_inplace(val.size);
  boost::endian::native_to_big_inplace(val.uid);
  boost::endian::native_to_big_inplace(val.valid);

  encode(&val, sizeof(val), true);
}

void packet::encode(remote::stat val) {
  boost::endian::native_to_big_inplace(val.st_mode);
  boost::endian::native_to_big_inplace(val.st_nlink);
  boost::endian::native_to_big_inplace(val.st_uid);
  boost::endian::native_to_big_inplace(val.st_gid);
  boost::endian::native_to_big_inplace(val.st_atimespec);
  boost::endian::native_to_big_inplace(val.st_mtimespec);
  boost::endian::native_to_big_inplace(val.st_ctimespec);
  boost::endian::native_to_big_inplace(val.st_birthtimespec);
  boost::endian::native_to_big_inplace(val.st_size);
  boost::endian::native_to_big_inplace(val.st_blocks);
  boost::endian::native_to_big_inplace(val.st_blksize);
  boost::endian::native_to_big_inplace(val.st_flags);

  encode(&val, sizeof(val), true);
}

void packet::encode(remote::statfs val, bool should_reserve) {
  boost::endian::native_to_big_inplace(val.f_bavail);
  boost::endian::native_to_big_inplace(val.f_bfree);
  boost::endian::native_to_big_inplace(val.f_blocks);
  boost::endian::native_to_big_inplace(val.f_favail);
  boost::endian::native_to_big_inplace(val.f_ffree);
  boost::endian::native_to_big_inplace(val.f_files);

  encode(&val, sizeof(remote::statfs), should_reserve);
}

void packet::encode(remote::statfs_x val) {
  buffer_.reserve(buffer_.size() + sizeof(remote::statfs) +
                  sizeof(val.f_mntfromname));
  encode(*dynamic_cast<remote::statfs *>(&val), false);
  encode(&val.f_mntfromname[0], sizeof(val.f_mntfromname), false);
}

void packet::encode(remote::file_info val) {
  boost::endian::native_to_big_inplace(val.FileAttributes);
  boost::endian::native_to_big_inplace(val.ReparseTag);
  boost::endian::native_to_big_inplace(val.AllocationSize);
  boost::endian::native_to_big_inplace(val.FileSize);
  boost::endian::native_to_big_inplace(val.CreationTime);
  boost::endian::native_to_big_inplace(val.LastAccessTime);
  boost::endian::native_to_big_inplace(val.LastWriteTime);
  boost::endian::native_to_big_inplace(val.ChangeTime);
  boost::endian::native_to_big_inplace(val.IndexNumber);
  boost::endian::native_to_big_inplace(val.HardLinks);
  boost::endian::native_to_big_inplace(val.EaSize);

  encode(&val, sizeof(val), true);
}

void packet::encode_top(const void *buffer, std::size_t size,
                        bool should_reserve) {
  if (size != 0U) {
    if (should_reserve) {
      buffer_.reserve(buffer_.size() + size);
    }
    const auto *char_buffer = reinterpret_cast<const char *>(buffer);
    buffer_.insert(buffer_.begin(), char_buffer, char_buffer + size);
  }
}

void packet::encode_top(const std::string &str) {
  const auto len = strnlen(str.c_str(), str.size());
  buffer_.reserve(len + 1U + buffer_.size());
  encode_top(str.c_str(), len, false);
  buffer_.insert(buffer_.begin() + static_cast<std::int32_t>(len), 0);
}

void packet::encode_top(const std::wstring &str) {
  encode_top(utils::string::to_utf8(str));
}

void packet::encode_top(std::int8_t val) {
  boost::endian::native_to_big_inplace(val);
  encode_top(&val, sizeof(val), true);
}

void packet::encode_top(std::uint8_t val) {
  boost::endian::native_to_big_inplace(val);
  encode_top(&val, sizeof(val), true);
}

void packet::encode_top(std::int16_t val) {
  boost::endian::native_to_big_inplace(val);
  encode_top(&val, sizeof(val), true);
}

void packet::encode_top(std::uint16_t val) {
  boost::endian::native_to_big_inplace(val);
  encode_top(&val, sizeof(val), true);
}

void packet::encode_top(std::int32_t val) {
  boost::endian::native_to_big_inplace(val);
  encode_top(&val, sizeof(val), true);
}

void packet::encode_top(std::uint32_t val) {
  boost::endian::native_to_big_inplace(val);
  encode_top(&val, sizeof(val), true);
}

void packet::encode_top(std::int64_t val) {
  boost::endian::native_to_big_inplace(val);
  encode_top(&val, sizeof(val), true);
}

void packet::encode_top(std::uint64_t val) {
  boost::endian::native_to_big_inplace(val);
  encode_top(&val, sizeof(val), true);
}

void packet::encode_top(remote::setattr_x val) {
  boost::endian::native_to_big_inplace(val.acctime);
  boost::endian::native_to_big_inplace(val.bkuptime);
  boost::endian::native_to_big_inplace(val.chgtime);
  boost::endian::native_to_big_inplace(val.crtime);
  boost::endian::native_to_big_inplace(val.flags);
  boost::endian::native_to_big_inplace(val.gid);
  boost::endian::native_to_big_inplace(val.mode);
  boost::endian::native_to_big_inplace(val.modtime);
  boost::endian::native_to_big_inplace(val.size);
  boost::endian::native_to_big_inplace(val.uid);
  boost::endian::native_to_big_inplace(val.valid);

  encode_top(&val, sizeof(val), true);
}

void packet::encode_top(remote::stat val) {
  boost::endian::native_to_big_inplace(val.st_mode);
  boost::endian::native_to_big_inplace(val.st_nlink);
  boost::endian::native_to_big_inplace(val.st_uid);
  boost::endian::native_to_big_inplace(val.st_gid);
  boost::endian::native_to_big_inplace(val.st_atimespec);
  boost::endian::native_to_big_inplace(val.st_mtimespec);
  boost::endian::native_to_big_inplace(val.st_ctimespec);
  boost::endian::native_to_big_inplace(val.st_birthtimespec);
  boost::endian::native_to_big_inplace(val.st_size);
  boost::endian::native_to_big_inplace(val.st_blocks);
  boost::endian::native_to_big_inplace(val.st_blksize);
  boost::endian::native_to_big_inplace(val.st_flags);

  encode_top(&val, sizeof(val), true);
}

void packet::encode_top(remote::statfs val, bool should_reserve) {
  boost::endian::native_to_big_inplace(val.f_bavail);
  boost::endian::native_to_big_inplace(val.f_bfree);
  boost::endian::native_to_big_inplace(val.f_blocks);
  boost::endian::native_to_big_inplace(val.f_favail);
  boost::endian::native_to_big_inplace(val.f_ffree);
  boost::endian::native_to_big_inplace(val.f_files);

  encode_top(&val, sizeof(remote::statfs), should_reserve);
}

void packet::encode_top(remote::statfs_x val) {
  buffer_.reserve(buffer_.size() + sizeof(remote::statfs) +
                  sizeof(val.f_mntfromname));
  encode_top(&val.f_mntfromname[0], sizeof(val.f_mntfromname), false);
  encode_top(*dynamic_cast<remote::statfs *>(&val), false);
}

void packet::encode_top(remote::file_info val) {
  boost::endian::native_to_big_inplace(val.FileAttributes);
  boost::endian::native_to_big_inplace(val.ReparseTag);
  boost::endian::native_to_big_inplace(val.AllocationSize);
  boost::endian::native_to_big_inplace(val.FileSize);
  boost::endian::native_to_big_inplace(val.CreationTime);
  boost::endian::native_to_big_inplace(val.LastAccessTime);
  boost::endian::native_to_big_inplace(val.LastWriteTime);
  boost::endian::native_to_big_inplace(val.ChangeTime);
  boost::endian::native_to_big_inplace(val.IndexNumber);
  boost::endian::native_to_big_inplace(val.HardLinks);
  boost::endian::native_to_big_inplace(val.EaSize);

  encode_top(&val, sizeof(val), true);
}

void packet::encrypt(const std::string &token) {
  try {
    data_buffer result;
    utils::encryption::encrypt_data(token, buffer_, result);
    buffer_ = std::move(result);
    encode_top(static_cast<std::uint32_t>(buffer_.size()));
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, "exception occurred");
  }
}

void packet::transfer_into(data_buffer &buffer) {
  buffer = std::move(buffer_);
  buffer_ = data_buffer();
  decode_offset_ = 0;
}

auto packet::operator=(const data_buffer &buffer) noexcept -> packet & {
  if (&buffer_ != &buffer) {
    buffer_ = buffer;
    decode_offset_ = 0;
  }

  return *this;
}

auto packet::operator=(data_buffer &&buffer) noexcept -> packet & {
  if (&buffer_ != &buffer) {
    buffer_ = std::move(buffer);
    decode_offset_ = 0;
  }

  return *this;
}

auto packet::operator=(const packet &pkt) noexcept -> packet & {
  if (this != &pkt) {
    buffer_ = pkt.buffer_;
    decode_offset_ = pkt.decode_offset_;
  }

  return *this;
}

auto packet::operator=(packet &&pkt) noexcept -> packet & {
  if (this != &pkt) {
    buffer_ = std::move(pkt.buffer_);
    decode_offset_ = pkt.decode_offset_;
  }

  return *this;
}
} // namespace repertory
