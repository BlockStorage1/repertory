/*
 Copyright <2018-2024> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

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
#include "test.hpp"

#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)

namespace repertory {
TEST(utils_encrypting_reader, read_file_data) {
  const auto token = std::string("moose");
  auto &source_file = test::create_random_file(
      8U * utils::encryption::encrypting_reader::get_data_chunk_size());
  EXPECT_TRUE(source_file);
  if (source_file) {
    stop_type stop_requested{false};
    utils::encryption::encrypting_reader reader(
        "test.dat", source_file.get_path(), stop_requested, token);

    for (std::uint8_t i = 0U; i < 8U; i++) {
      data_buffer buffer(
          utils::encryption::encrypting_reader::get_encrypted_chunk_size());
      for (std::uint8_t j = 0U; j < 2U; j++) {
        ASSERT_EQ(
            buffer.size() / 2U,
            utils::encryption::encrypting_reader::reader_function(
                reinterpret_cast<char *>(&buffer[(buffer.size() / 2U) * j]),
                buffer.size() / 2U, 1U, &reader));
      }

      data_buffer decrypted_data;
      EXPECT_TRUE(
          utils::encryption::decrypt_data(token, buffer, decrypted_data));

      EXPECT_EQ(utils::encryption::encrypting_reader::get_data_chunk_size(),
                decrypted_data.size());

      std::size_t bytes_read{};
      data_buffer file_data(decrypted_data.size());
      EXPECT_TRUE(source_file.read(
          file_data,
          utils::encryption::encrypting_reader::get_data_chunk_size() * i,
          &bytes_read));
      EXPECT_EQ(0, std::memcmp(file_data.data(), decrypted_data.data(),
                               file_data.size()));
    }
  }
}

TEST(utils_encrypting_reader, read_file_data_in_multiple_chunks) {
  const auto token = std::string("moose");
  auto &source_file = test::create_random_file(
      8U * utils::encryption::encrypting_reader::get_data_chunk_size());
  EXPECT_TRUE(source_file);
  if (source_file) {
    stop_type stop_requested{false};
    utils::encryption::encrypting_reader reader(
        "test.dat", source_file.get_path(), stop_requested, token);

    for (std::uint8_t i = 0U; i < 8U; i += 2U) {
      data_buffer buffer(
          utils::encryption::encrypting_reader::get_encrypted_chunk_size() *
          2U);
      EXPECT_EQ(buffer.size(),
                utils::encryption::encrypting_reader::reader_function(
                    reinterpret_cast<char *>(buffer.data()), buffer.size(), 1U,
                    &reader));

      for (std::uint8_t j = 0U; j < 2U; j++) {
        data_buffer decrypted_data;
        const auto offset = (j * (buffer.size() / 2U));
        EXPECT_TRUE(utils::encryption::decrypt_data(
            token,
            data_buffer(
                std::next(buffer.begin(), static_cast<std::int64_t>(offset)),
                std::next(buffer.begin(), static_cast<std::int64_t>(
                                              offset + (buffer.size() / 2U)))),
            decrypted_data));

        EXPECT_EQ(utils::encryption::encrypting_reader::get_data_chunk_size(),
                  decrypted_data.size());

        std::size_t bytes_read{};
        data_buffer file_data(decrypted_data.size());
        EXPECT_TRUE(source_file.read(
            file_data,
            (utils::encryption::encrypting_reader::get_data_chunk_size() * i) +
                (j *
                 utils::encryption::encrypting_reader::get_data_chunk_size()),
            &bytes_read));
        EXPECT_EQ(0, std::memcmp(file_data.data(), decrypted_data.data(),
                                 file_data.size()));
      }
    }
  }
}

TEST(utils_encrypting_reader, read_file_data_as_stream) {
  const auto token = std::string("moose");
  auto &source_file = test::create_random_file(
      8U * utils::encryption::encrypting_reader::get_data_chunk_size());
  EXPECT_TRUE(source_file);
  if (source_file) {
    stop_type stop_requested{false};
    utils::encryption::encrypting_reader reader(
        "test.dat", source_file.get_path(), stop_requested, token);
    auto io_stream = reader.create_iostream();
    EXPECT_FALSE(io_stream->seekg(0, std::ios_base::end).fail());
    EXPECT_TRUE(io_stream->good());
    EXPECT_EQ(reader.get_total_size(),
              static_cast<std::uint64_t>(io_stream->tellg()));
    EXPECT_FALSE(io_stream->seekg(0, std::ios_base::beg).fail());
    EXPECT_TRUE(io_stream->good());

    for (std::uint8_t i = 0U; i < 8U; i++) {
      data_buffer buffer(
          utils::encryption::encrypting_reader::get_encrypted_chunk_size());
      EXPECT_FALSE(
          io_stream->seekg(static_cast<std::streamoff>(i * buffer.size()))
              .fail());
      EXPECT_TRUE(io_stream->good());
      for (std::uint8_t j = 0U; j < 2U; j++) {
        EXPECT_FALSE(
            io_stream
                ->read(
                    reinterpret_cast<char *>(&buffer[(buffer.size() / 2U) * j]),
                    static_cast<std::streamsize>(buffer.size()) / 2U)
                .fail());
        EXPECT_TRUE(io_stream->good());
      }

      data_buffer decrypted_data;
      EXPECT_TRUE(
          utils::encryption::decrypt_data(token, buffer, decrypted_data));

      EXPECT_EQ(utils::encryption::encrypting_reader::get_data_chunk_size(),
                decrypted_data.size());

      std::size_t bytes_read{};
      data_buffer file_data(decrypted_data.size());
      EXPECT_TRUE(source_file.read(
          file_data,
          utils::encryption::encrypting_reader::get_data_chunk_size() * i,
          &bytes_read));
      EXPECT_EQ(0, std::memcmp(file_data.data(), decrypted_data.data(),
                               file_data.size()));
    }
  }
}

TEST(utils_encrypting_reader, read_file_data_in_multiple_chunks_as_stream) {
  const auto token = std::string("moose");
  auto &source_file = test::create_random_file(
      8u * utils::encryption::encrypting_reader::get_data_chunk_size());
  EXPECT_TRUE(source_file);
  if (source_file) {
    stop_type stop_requested{false};
    utils::encryption::encrypting_reader reader(
        "test.dat", source_file.get_path(), stop_requested, token);
    auto io_stream = reader.create_iostream();
    EXPECT_FALSE(io_stream->seekg(0, std::ios_base::end).fail());
    EXPECT_TRUE(io_stream->good());
    EXPECT_EQ(reader.get_total_size(),
              static_cast<std::uint64_t>(io_stream->tellg()));
    EXPECT_FALSE(io_stream->seekg(0, std::ios_base::beg).fail());
    EXPECT_TRUE(io_stream->good());

    for (std::uint8_t i = 0U; i < 8U; i += 2U) {
      data_buffer buffer(
          utils::encryption::encrypting_reader::get_encrypted_chunk_size() *
          2U);
      EXPECT_FALSE(io_stream
                       ->read(reinterpret_cast<char *>(buffer.data()),
                              static_cast<std::streamsize>(buffer.size()))
                       .fail());
      EXPECT_TRUE(io_stream->good());

      for (std::uint8_t j = 0U; j < 2U; j++) {
        data_buffer decrypted_data;
        const auto offset = (j * (buffer.size() / 2U));
        EXPECT_TRUE(utils::encryption::decrypt_data(
            token,
            data_buffer(
                std::next(buffer.begin(), static_cast<std::int64_t>(offset)),
                std::next(buffer.begin(), static_cast<std::int64_t>(
                                              offset + (buffer.size() / 2U)))),
            decrypted_data));

        EXPECT_EQ(utils::encryption::encrypting_reader::get_data_chunk_size(),
                  decrypted_data.size());

        std::size_t bytes_read{};
        data_buffer file_data(decrypted_data.size());
        EXPECT_TRUE(source_file.read(
            file_data,
            (utils::encryption::encrypting_reader::get_data_chunk_size() * i) +
                (j *
                 utils::encryption::encrypting_reader::get_data_chunk_size()),
            &bytes_read));
        EXPECT_EQ(0, std::memcmp(file_data.data(), decrypted_data.data(),
                                 file_data.size()));
      }
    }
  }
}
} // namespace repertory

#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
