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

#include "utils/file_enc_file.hpp"

#include "utils/common.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/encryption.hpp"

namespace repertory::utils::file {
auto enc_file::attach_file(fs_file_t file) -> fs_file_t {
  return fs_file_t{
      new enc_file(std::move(file)),
  };
}

enc_file::enc_file(fs_file_t file) : file_(std::move(file)) {}

void enc_file::close() { file_->close(); }

auto enc_file::copy_to(std::string_view new_path, bool overwrite) const
    -> bool {
  return file_->copy_to(new_path, overwrite);
}

void enc_file::flush() const { file_->flush(); }

auto enc_file::move_to(std::string_view path) -> bool {
  return file_->move_to(path);
}

auto enc_file::read(unsigned char *data, std::size_t to_read,
                    std::uint64_t offset, std::size_t *total_read) -> bool {
  if (total_read != nullptr) {
    *total_read = 0U;
  }

  auto file_size{size()};
  if (not file_size.has_value()) {
    return false;
  }

  to_read = utils::calculate_read_size(file_size.value(), to_read, offset);
  if (to_read == 0U) {
    return true;
  }

  std::size_t bytes_read{};
  auto ret{
      utils::encryption::read_encrypted_range(
          {.begin = offset, .end = offset + to_read - 1U},
          utils::encryption::generate_key<utils::hash::hash_256_t>(
              encryption_token_),
          [&](auto &&ct_buffer, auto &&start_offset,
              auto &&end_offset) -> bool {
            ct_buffer.resize(end_offset - start_offset + 1U);
            return file_->read(ct_buffer, start_offset);
          },
          file_size.value(), data, to_read, bytes_read),
  };
  if (ret && total_read != nullptr) {
    *total_read = bytes_read;
  }

  return ret;
}

auto enc_file::remove() -> bool { return file_->remove(); }

auto enc_file::truncate(std::size_t size) -> bool {
  if (size == 0U) {
    return file_->truncate(size);
  }

  auto file_size{this->size()};
  if (not file_size.has_value()) {
    return false;
  }

  if (size == file_size.value()) {
    return true;
  }

  auto chunks{
      size / utils::encryption::encrypting_reader::get_data_chunk_size(),
  };
  auto real_size{
      (chunks * utils::encryption::encrypting_reader::get_data_chunk_size()) +
          (chunks * utils::encryption::encrypting_reader::get_header_size()),
  };
  auto remain{
      size % utils::encryption::encrypting_reader::get_data_chunk_size(),
  };
  if (remain > 0U) {
    real_size +=
        (remain + utils::encryption::encrypting_reader::get_header_size());
  }

  if (size < file_size.value()) {
    if (remain == 0U) {
      return file_->truncate(real_size);
    }

    auto begin_chunk{
        size / utils::encryption::encrypting_reader::get_data_chunk_size(),
    };

    auto offset{
        begin_chunk *
            utils::encryption::encrypting_reader::get_data_chunk_size(),
    };

    std::size_t total_read{};
    data_buffer data(
        utils::encryption::encrypting_reader::get_data_chunk_size());
    if (not i_file::read(data, offset, &total_read)) {
      return false;
    }
    data.resize(remain);

    if (not file_->truncate(real_size)) {
      return false;
    }

    return i_file::write(data, offset);
  }

  /* auto begin_chunk{
      file_size.value() /
          utils::encryption::encrypting_reader::get_data_chunk_size(),
  };
  auto end_chunk{
      utils::divide_with_ceiling(
          file_size.value(),
          utils::encryption::encrypting_reader::get_data_chunk_size()),
  }; */

  return false;
}

auto enc_file::write(const unsigned char * /* data */, std::size_t to_write,
                     std::size_t offset, std::size_t * /* total_written */)
    -> bool {
  auto file_size{size()};
  if (not file_size.has_value()) {
    return false;
  }

  if ((offset + to_write) > file_size.value()) {
    if (not truncate((offset + to_write) - file_size.value())) {
      return false;
    }
  }

  return false;
}

auto enc_file::size() const -> std::optional<std::uint64_t> {
  auto file_size = file_->size();
  if (not file_size.has_value()) {
    return std::nullopt;
  }

  return utils::encryption::encrypting_reader::calculate_decrypted_size(
      file_size.value(), false);
}
} // namespace repertory::utils::file

#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
