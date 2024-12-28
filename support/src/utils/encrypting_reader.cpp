/*
  Copyright <2018-2024> <scott.e.graves@protonmail.com>

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

#include "utils/encrypting_reader.hpp"

#include "utils/collection.hpp"
#include "utils/common.hpp"
#include "utils/encryption.hpp"
#include "utils/error.hpp"
#include "utils/file.hpp"
#include "utils/unix.hpp"
#include "utils/windows.hpp"

#if !defined(CURL_READFUNC_ABORT)
#define CURL_READFUNC_ABORT (-1)
#endif // !defined(CURL_READFUNC_ABORT)

namespace repertory::utils::encryption {
class encrypting_streambuf final : public encrypting_reader::streambuf {
public:
  encrypting_streambuf(const encrypting_streambuf &) = default;
  encrypting_streambuf(encrypting_streambuf &&) = delete;
  auto
  operator=(const encrypting_streambuf &) -> encrypting_streambuf & = delete;
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
    REPERTORY_USES_FUNCTION_NAME();

    if ((which & std::ios_base::in) != std::ios_base::in) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "output is not supported",
                                           });
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

  auto seekpos(pos_type pos, std::ios_base::openmode which =
                                 std::ios_base::out |
                                 std::ios_base::in) -> pos_type override {
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

    auto res = encrypting_reader::reader_function(
        ptr, 1U, static_cast<std::size_t>(count), &reader_);
    if ((res == reader_.get_error_return()) ||
        (reader_.get_stop_requested() &&
         (res == static_cast<std::size_t>(CURL_READFUNC_ABORT)))) {
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
    std::string_view file_name, std::string_view source_path,
    stop_type &stop_requested, std::string_view token,
    std::optional<std::string> relative_parent_path, std::size_t error_return)
    : key_(utils::encryption::generate_key<utils::encryption::hash_256_t>(
          token)),
      stop_requested_(stop_requested),
      error_return_(error_return),
      source_file_(utils::file::file::open_or_create_file(source_path, true)) {
  REPERTORY_USES_FUNCTION_NAME();

  if (not *source_file_) {
    throw utils::error::create_exception(function_name, {
                                                            "file open failed",
                                                            source_path,
                                                        });
  }

  data_buffer result;
  utils::encryption::encrypt_data(
      key_, reinterpret_cast<const unsigned char *>(file_name.data()),
      file_name.size(), result);
  encrypted_file_name_ = utils::collection::to_hex_string(result);

  if (relative_parent_path.has_value()) {
    for (auto &&part :
         utils::string::split(relative_parent_path.value(),
                              utils::path::directory_seperator, false)) {
      utils::encryption::encrypt_data(
          key_, reinterpret_cast<const unsigned char *>(part.c_str()),
          strnlen(part.c_str(), part.size()), result);
      encrypted_file_path_ += '/' + utils::collection::to_hex_string(result);
    }
    encrypted_file_path_ += '/' + encrypted_file_name_;
  }

  auto opt_size = source_file_->size();
  if (not opt_size.has_value()) {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to get file size",
                                             source_file_->get_path(),
                                         });
  }
  auto file_size = opt_size.value();

  const auto total_chunks = utils::divide_with_ceiling(
      file_size, static_cast<std::uint64_t>(data_chunk_size_));
  total_size_ = file_size + (total_chunks * encryption_header_size);
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

encrypting_reader::encrypting_reader(std::string_view encrypted_file_path,
                                     std::string_view source_path,
                                     stop_type &stop_requested,
                                     std::string_view token,
                                     std::size_t error_return)
    : key_(utils::encryption::generate_key<utils::encryption::hash_256_t>(
          token)),
      stop_requested_(stop_requested),
      error_return_(error_return),
      source_file_(utils::file::file::open_or_create_file(source_path, true)) {
  REPERTORY_USES_FUNCTION_NAME();

  if (not *source_file_) {
    throw utils::error::create_exception(function_name, {
                                                            "file open failed",
                                                            source_path,
                                                        });
  }

  encrypted_file_path_ = encrypted_file_path;
  encrypted_file_name_ = utils::path::strip_to_file_name(encrypted_file_path_);

  auto opt_size = source_file_->size();
  if (not opt_size.has_value()) {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to get file size",
                                             source_file_->get_path(),
                                         });
  }
  auto file_size = opt_size.value();

  const auto total_chunks = utils::divide_with_ceiling(
      file_size, static_cast<std::uint64_t>(data_chunk_size_));
  total_size_ = file_size + (total_chunks * encryption_header_size);
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
    std::string_view encrypted_file_path, std::string_view source_path,
    stop_type &stop_requested, std::string_view token,
    std::vector<
        std::array<unsigned char, crypto_aead_xchacha20poly1305_IETF_NPUBBYTES>>
        iv_list,
    std::size_t error_return)
    : key_(utils::encryption::generate_key<utils::encryption::hash_256_t>(
          token)),
      stop_requested_(stop_requested),
      error_return_(error_return),
      source_file_(utils::file::file::open_or_create_file(source_path, true)) {
  REPERTORY_USES_FUNCTION_NAME();

  if (not *source_file_) {
    throw utils::error::create_exception(function_name, {
                                                            "file open failed",
                                                            source_path,
                                                        });
  }

  encrypted_file_path_ = encrypted_file_path;
  encrypted_file_name_ = utils::path::strip_to_file_name(encrypted_file_path_);

  auto opt_size = source_file_->size();
  if (not opt_size.has_value()) {
    throw utils::error::create_exception(
        function_name, {
                           "get file size failed",
                           std::to_string(utils::get_last_error_code()),
                           source_file_->get_path(),
                       });
  }
  auto file_size{opt_size.value()};

  const auto total_chunks = utils::divide_with_ceiling(
      file_size, static_cast<std::uint64_t>(data_chunk_size_));
  total_size_ = file_size + (total_chunks * encryption_header_size);
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
      source_file_(
          utils::file::file::open_file(reader.source_file_->get_path(), true)),
      chunk_buffers_(reader.chunk_buffers_),
      encrypted_file_name_(reader.encrypted_file_name_),
      encrypted_file_path_(reader.encrypted_file_path_),
      iv_list_(reader.iv_list_),
      last_data_chunk_(reader.last_data_chunk_),
      last_data_chunk_size_(reader.last_data_chunk_size_),
      read_offset_(reader.read_offset_),
      total_size_(reader.total_size_) {
  REPERTORY_USES_FUNCTION_NAME();

  if (not *source_file_) {
    throw utils::error::create_exception(
        function_name, {
                           "file open failed",
                           std::to_string(utils::get_last_error_code()),
                           source_file_->get_path(),
                       });
  }
}

auto encrypting_reader::calculate_decrypted_size(std::uint64_t total_size)
    -> std::uint64_t {
  return total_size - (utils::divide_with_ceiling(
                           total_size, static_cast<std::uint64_t>(
                                           get_encrypted_chunk_size())) *
                       encryption_header_size);
}

auto encrypting_reader::calculate_encrypted_size(std::string_view source_path)
    -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  auto opt_size = utils::file::file{source_path}.size();
  if (not opt_size.has_value()) {
    throw utils::error::create_exception(
        function_name, {
                           "get file size failed",
                           std::to_string(utils::get_last_error_code()),
                           source_path,
                       });
  }
  auto file_size{opt_size.value()};

  const auto total_chunks = utils::divide_with_ceiling(
      file_size, static_cast<std::uint64_t>(data_chunk_size_));
  return file_size + (total_chunks * encryption_header_size);
}

auto encrypting_reader::create_iostream() const
    -> std::shared_ptr<encrypting_reader::iostream> {
  return std::make_shared<encrypting_reader_iostream>(
      std::make_unique<encrypting_streambuf>(*this));
}

auto encrypting_reader::reader_function(char *buffer, size_t size,
                                        size_t nitems) -> size_t {
  REPERTORY_USES_FUNCTION_NAME();

  const auto read_size = static_cast<std::size_t>(std::min(
      static_cast<std::uint64_t>(size * nitems), total_size_ - read_offset_));

  auto chunk = read_offset_ / encrypted_chunk_size_;
  auto chunk_offset = read_offset_ % encrypted_chunk_size_;
  std::size_t total_read{};

  auto ret = false;
  if (read_offset_ < total_size_) {
    try {
      ret = true;
      auto remain = read_size;
      while (not stop_requested_ && ret && (remain != 0U)) {
        if (chunk_buffers_.find(chunk) == chunk_buffers_.end()) {
          auto &chunk_buffer = chunk_buffers_[chunk];
          data_buffer file_data(chunk == last_data_chunk_
                                    ? last_data_chunk_size_
                                    : data_chunk_size_);
          chunk_buffer.resize(file_data.size() + encryption_header_size);

          std::size_t bytes_read{};
          if ((ret = source_file_->read(file_data, chunk * data_chunk_size_,
                                        &bytes_read))) {
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
      utils::error::handle_exception(function_name, e);
      ret = false;
    } catch (...) {
      utils::error::handle_exception(function_name);
      ret = false;
    }
  }

  return stop_requested_ ? static_cast<std::size_t>(CURL_READFUNC_ABORT)
         : ret           ? total_read
                         : error_return_;
}
} // namespace repertory::utils::encryption

#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
