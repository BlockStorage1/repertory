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
#include "utils/encrypting_reader.hpp"

#include "events/event_system.hpp"
#include "events/events.hpp"
#include "types/repertory.hpp"
#include "utils/encryption.hpp"
#include "utils/error_utils.hpp"
#include "utils/utils.hpp"

namespace repertory::utils::encryption {
class encrypting_streambuf final : public encrypting_reader::streambuf {
public:
  encrypting_streambuf(const encrypting_streambuf &) = default;
  encrypting_streambuf(encrypting_streambuf &&) = delete;
  auto operator=(const encrypting_streambuf &)
      -> encrypting_streambuf & = delete;
  auto operator=(encrypting_streambuf &&) -> encrypting_streambuf & = delete;

  explicit encrypting_streambuf(const encrypting_reader &reader)
      : reader_(reader) {
    setg(reinterpret_cast<char *>(0), reinterpret_cast<char *>(0),
         reinterpret_cast<char *>(reader_.get_total_size()));
  }

  ~encrypting_streambuf() override = default;

private:
  encrypting_reader reader_;

protected:
  auto seekoff(off_type off, std::ios_base::seekdir dir,
               std::ios_base::openmode which = std::ios_base::out |
                                               std::ios_base::in)
      -> pos_type override {
    if ((which & std::ios_base::in) != std::ios_base::in) {
      throw std::runtime_error("output is not supported");
    }

    const auto set_position = [this](char *next) -> pos_type {
      if ((next <= egptr()) && (next >= eback())) {
        setg(eback(), next, reinterpret_cast<char *>(reader_.get_total_size()));
        return static_cast<std::streamoff>(
            reinterpret_cast<std::uintptr_t>(gptr()));
      }

      return {traits_type::eof()};
    };

    switch (dir) {
    case std::ios_base::beg:
      return set_position(eback() + off);

    case std::ios_base::cur:
      return set_position(gptr() + off);

    case std::ios_base::end:
      return set_position(egptr() + off);
    }

    return encrypting_reader::streambuf::seekoff(off, dir, which);
  }

  auto seekpos(pos_type pos,
               std::ios_base::openmode which = std::ios_base::out |
                                               std::ios_base::in)
      -> pos_type override {
    return seekoff(pos, std::ios_base::beg, which);
  }

  auto uflow() -> int_type override {
    auto ret = underflow();
    if (ret == traits_type::eof()) {
      return ret;
    }

    gbump(1);

    return ret;
  }

  auto underflow() -> int_type override {
    if (gptr() == egptr()) {
      return traits_type::eof();
    }

    reader_.set_read_position(reinterpret_cast<std::uintptr_t>(gptr()));

    char c{};
    const auto res = encrypting_reader::reader_function(&c, 1U, 1U, &reader_);
    if (res != 1) {
      return traits_type::eof();
    }

    return c;
  }

  auto xsgetn(char *ptr, std::streamsize count) -> std::streamsize override {
    if (gptr() == egptr()) {
      return traits_type::eof();
    }

    reader_.set_read_position(reinterpret_cast<std::uintptr_t>(gptr()));

    const auto res = encrypting_reader::reader_function(
        ptr, 1U, static_cast<std::size_t>(count), &reader_);
    if ((res == reader_.get_error_return()) ||
        (reader_.get_stop_requested() && (res == CURL_READFUNC_ABORT))) {
      return traits_type::eof();
    }

    setg(eback(), gptr() + res,
         reinterpret_cast<char *>(reader_.get_total_size()));
    return static_cast<std::streamsize>(res);
  }
};

class encrypting_reader_iostream final : public encrypting_reader::iostream {
public:
  encrypting_reader_iostream(const encrypting_reader_iostream &) = delete;
  encrypting_reader_iostream(encrypting_reader_iostream &&) = delete;

  auto operator=(const encrypting_reader_iostream &)
      -> encrypting_reader_iostream & = delete;
  auto operator=(encrypting_reader_iostream &&)
      -> encrypting_reader_iostream & = delete;

  explicit encrypting_reader_iostream(
      std::unique_ptr<encrypting_streambuf> buffer)
      : encrypting_reader::iostream(buffer.get()), buffer_(std::move(buffer)) {}

  ~encrypting_reader_iostream() override = default;

private:
  std::unique_ptr<encrypting_streambuf> buffer_;
};

const std::size_t encrypting_reader::header_size_ = ([]() {
  return crypto_aead_xchacha20poly1305_IETF_NPUBBYTES +
         crypto_aead_xchacha20poly1305_IETF_ABYTES;
})();
const std::size_t encrypting_reader::data_chunk_size_ = (8UL * 1024UL * 1024UL);
const std::size_t encrypting_reader::encrypted_chunk_size_ =
    data_chunk_size_ + header_size_;

encrypting_reader::encrypting_reader(
    const std::string &file_name, const std::string &source_path,
    stop_type &stop_requested, const std::string &token,
    std::optional<std::string> relative_parent_path, std::size_t error_return)
    : key_(utils::encryption::generate_key(token)),
      stop_requested_(stop_requested),
      error_return_(error_return) {
  const auto res = native_file::create_or_open(
      source_path, not relative_parent_path.has_value(), source_file_);
  if (res != api_error::success) {
    throw std::runtime_error("file open failed|src|" + source_path + '|' +
                             api_error_to_string(res));
  }

  data_buffer result;
  utils::encryption::encrypt_data(key_, file_name.c_str(),
                                  strnlen(file_name.c_str(), file_name.size()),
                                  result);
  encrypted_file_name_ = utils::to_hex_string(result);

  if (relative_parent_path.has_value()) {
    for (const auto &part :
         std::filesystem::path(relative_parent_path.value())) {
      utils::encryption::encrypt_data(
          key_, part.string().c_str(),
          strnlen(part.string().c_str(), part.string().size()), result);
      encrypted_file_path_ += '/' + utils::to_hex_string(result);
    }
    encrypted_file_path_ += '/' + encrypted_file_name_;
  }

  std::uint64_t file_size{};
  if (not utils::file::get_file_size(source_path, file_size)) {
    throw std::runtime_error("get file size failed|src|" + source_path + '|' +
                             std::to_string(utils::get_last_error_code()));
  }

  const auto total_chunks = utils::divide_with_ceiling(
      file_size, static_cast<std::uint64_t>(data_chunk_size_));
  total_size_ =
      file_size + (total_chunks * encrypting_reader::get_header_size());
  last_data_chunk_ = total_chunks - 1U;
  last_data_chunk_size_ = (file_size <= data_chunk_size_) ? file_size
                          : (file_size % data_chunk_size_) == 0U
                              ? data_chunk_size_
                              : file_size % data_chunk_size_;
  iv_list_.resize(total_chunks);
  for (auto &iv : iv_list_) {
    randombytes_buf(iv.data(), iv.size());
  }
}

encrypting_reader::encrypting_reader(const std::string &encrypted_file_path,
                                     const std::string &source_path,
                                     stop_type &stop_requested,
                                     const std::string &token,
                                     std::size_t error_return)
    : key_(utils::encryption::generate_key(token)),
      stop_requested_(stop_requested),
      error_return_(error_return) {
  const auto res =
      native_file::create_or_open(source_path, false, source_file_);
  if (res != api_error::success) {
    throw std::runtime_error("file open failed|src|" + source_path + '|' +
                             api_error_to_string(res));
  }

  encrypted_file_path_ = encrypted_file_path;
  encrypted_file_name_ =
      std::filesystem::path(encrypted_file_path_).filename().string();

  std::uint64_t file_size{};
  if (not utils::file::get_file_size(source_path, file_size)) {
    throw std::runtime_error("get file size failed|src|" + source_path + '|' +
                             std::to_string(utils::get_last_error_code()));
  }

  const auto total_chunks = utils::divide_with_ceiling(
      file_size, static_cast<std::uint64_t>(data_chunk_size_));
  total_size_ =
      file_size + (total_chunks * encrypting_reader::get_header_size());
  last_data_chunk_ = total_chunks - 1U;
  last_data_chunk_size_ = (file_size <= data_chunk_size_) ? file_size
                          : (file_size % data_chunk_size_) == 0U
                              ? data_chunk_size_
                              : file_size % data_chunk_size_;
  iv_list_.resize(total_chunks);
  for (auto &iv : iv_list_) {
    randombytes_buf(iv.data(), iv.size());
  }
}

encrypting_reader::encrypting_reader(
    const std::string &encrypted_file_path, const std::string &source_path,
    stop_type &stop_requested, const std::string &token,
    std::vector<
        std::array<unsigned char, crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>>
        iv_list,
    std::size_t error_return)
    : key_(utils::encryption::generate_key(token)),
      stop_requested_(stop_requested),
      error_return_(error_return) {
  const auto res =
      native_file::create_or_open(source_path, false, source_file_);
  if (res != api_error::success) {
    throw std::runtime_error("file open failed|src|" + source_path + '|' +
                             api_error_to_string(res));
  }

  encrypted_file_path_ = encrypted_file_path;
  encrypted_file_name_ =
      std::filesystem::path(encrypted_file_path_).filename().string();

  std::uint64_t file_size{};
  if (not utils::file::get_file_size(source_path, file_size)) {
    throw std::runtime_error("get file size failed|src|" + source_path + '|' +
                             std::to_string(utils::get_last_error_code()));
  }

  const auto total_chunks = utils::divide_with_ceiling(
      file_size, static_cast<std::uint64_t>(data_chunk_size_));
  total_size_ =
      file_size + (total_chunks * encrypting_reader::get_header_size());
  last_data_chunk_ = total_chunks - 1U;
  last_data_chunk_size_ = (file_size <= data_chunk_size_) ? file_size
                          : (file_size % data_chunk_size_) == 0U
                              ? data_chunk_size_
                              : file_size % data_chunk_size_;
  iv_list_ = std::move(iv_list);
}

encrypting_reader::encrypting_reader(const encrypting_reader &reader)
    : key_(reader.key_),
      stop_requested_(reader.stop_requested_),
      error_return_(reader.error_return_),
      chunk_buffers_(reader.chunk_buffers_),
      encrypted_file_name_(reader.encrypted_file_name_),
      encrypted_file_path_(reader.encrypted_file_path_),
      iv_list_(reader.iv_list_),
      last_data_chunk_(reader.last_data_chunk_),
      last_data_chunk_size_(reader.last_data_chunk_size_),
      read_offset_(reader.read_offset_),
      source_file_(native_file::clone(reader.source_file_)),
      total_size_(reader.total_size_) {}

encrypting_reader::~encrypting_reader() {
  if (source_file_) {
    source_file_->close();
  }
}

auto encrypting_reader::calculate_decrypted_size(std::uint64_t total_size)
    -> std::uint64_t {
  return total_size - (utils::divide_with_ceiling(
                           total_size, static_cast<std::uint64_t>(
                                           get_encrypted_chunk_size())) *
                       get_header_size());
}

auto encrypting_reader::calculate_encrypted_size(const std::string &source_path)
    -> std::uint64_t {
  std::uint64_t file_size{};
  if (not utils::file::get_file_size(source_path, file_size)) {
    throw std::runtime_error("get file size failed|src|" + source_path + '|' +
                             std::to_string(utils::get_last_error_code()));
  }

  const auto total_chunks = utils::divide_with_ceiling(
      file_size, static_cast<std::uint64_t>(data_chunk_size_));
  return file_size + (total_chunks * encrypting_reader::get_header_size());
}

auto encrypting_reader::create_iostream() const
    -> std::shared_ptr<encrypting_reader::iostream> {
  return std::make_shared<encrypting_reader_iostream>(
      std::make_unique<encrypting_streambuf>(*this));
}

auto encrypting_reader::reader_function(char *buffer, size_t size,
                                        size_t nitems) -> size_t {
  const auto read_size = static_cast<std::size_t>(std::min(
      static_cast<std::uint64_t>(size * nitems), total_size_ - read_offset_));

  auto chunk = read_offset_ / encrypted_chunk_size_;
  auto chunk_offset = read_offset_ % encrypted_chunk_size_;
  std::size_t total_read = 0u;

  auto ret = false;
  if (read_offset_ < total_size_) {
    try {
      ret = true;
      auto remain = read_size;
      while (not stop_requested_ && ret && remain) {
        if (chunk_buffers_.find(chunk) == chunk_buffers_.end()) {
          auto &chunk_buffer = chunk_buffers_[chunk];
          data_buffer file_data(chunk == last_data_chunk_
                                    ? last_data_chunk_size_
                                    : data_chunk_size_);
          chunk_buffer.resize(file_data.size() + get_header_size());

          std::size_t bytes_read{};
          if ((ret = source_file_->read_bytes(&file_data[0u], file_data.size(),
                                              chunk * data_chunk_size_,
                                              bytes_read))) {
            utils::encryption::encrypt_data(iv_list_.at(chunk), key_, file_data,
                                            chunk_buffer);
          }
        } else if (chunk) {
          chunk_buffers_.erase(chunk - 1u);
        }

        auto &chunk_buffer = chunk_buffers_[chunk];
        const auto to_read = std::min(
            static_cast<std::size_t>(chunk_buffer.size() - chunk_offset),
            remain);
        std::memcpy(buffer + total_read, &chunk_buffer[chunk_offset], to_read);
        total_read += to_read;
        remain -= to_read;
        chunk_offset = 0u;
        chunk++;
        read_offset_ += to_read;
      }
    } catch (const std::exception &e) {
      utils::error::raise_error(__FUNCTION__, e, "exception occurred");
      ret = false;
    }
  }

  return stop_requested_ ? CURL_READFUNC_ABORT
         : ret           ? total_read
                         : error_return_;
}
} // namespace repertory::utils::encryption
