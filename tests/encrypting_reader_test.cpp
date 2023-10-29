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
#include "test_common.hpp"

#include "types/repertory.hpp"
#include "utils/encrypting_reader.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
static std::string get_source_file_name() {
  return generate_test_file_name("./", "encrypting_reader");
}

TEST(encrypting_reader, get_encrypted_file_name) {
  const auto source_file_name = get_source_file_name();
  ASSERT_TRUE(utils::file::retry_delete_file(source_file_name));

  const auto token = std::string("moose");
  auto source_file = create_random_file(source_file_name, 1024ul);
  EXPECT_TRUE(source_file != nullptr);
  if (source_file) {
    stop_type stop_requested = false;
    utils::encryption::encrypting_reader reader("test.dat", source_file_name,
                                                stop_requested, token);

    auto file_name = reader.get_encrypted_file_name();
    std::cout << file_name << std::endl;

    EXPECT_EQ(api_error::success,
              utils::encryption::decrypt_file_name(token, file_name));
    std::cout << file_name << std::endl;
    EXPECT_STREQ("test.dat", file_name.c_str());

    source_file->close();
  }

  EXPECT_TRUE(utils::file::retry_delete_file(source_file_name));
}

TEST(encrypting_reader, file_data) {
  const auto source_file_name = get_source_file_name();
  EXPECT_TRUE(utils::file::retry_delete_file(source_file_name));

  const auto token = std::string("moose");
  auto source_file = create_random_file(
      source_file_name,
      8u * utils::encryption::encrypting_reader::get_data_chunk_size());
  EXPECT_TRUE(source_file != nullptr);
  if (source_file) {
    stop_type stop_requested = false;
    utils::encryption::encrypting_reader reader("test.dat", source_file_name,
                                                stop_requested, token);

    for (std::uint8_t i = 0u; i < 8u; i++) {
      data_buffer buffer(
          utils::encryption::encrypting_reader::get_encrypted_chunk_size());
      for (std::uint8_t j = 0u; j < 2u; j++) {
        EXPECT_EQ(buffer.size() / 2u,
                  utils::encryption::encrypting_reader::reader_function(
                      &buffer[(buffer.size() / 2u) * j], buffer.size() / 2u, 1u,
                      &reader));
      }

      data_buffer decrypted_data;
      EXPECT_TRUE(
          utils::encryption::decrypt_data(token, buffer, decrypted_data));

      EXPECT_EQ(utils::encryption::encrypting_reader::get_data_chunk_size(),
                decrypted_data.size());

      std::size_t bytes_read{};
      data_buffer file_data(decrypted_data.size());
      EXPECT_TRUE(source_file->read_bytes(
          &file_data[0u], file_data.size(),
          utils::encryption::encrypting_reader::get_data_chunk_size() * i,
          bytes_read));
      EXPECT_EQ(0, std::memcmp(&file_data[0u], &decrypted_data[0u],
                               file_data.size()));
    }

    source_file->close();
  }
  EXPECT_TRUE(utils::file::retry_delete_file(source_file_name));
}

TEST(encrypting_reader, file_data_in_multiple_chunks) {
  const auto source_file_name = get_source_file_name();
  ASSERT_TRUE(utils::file::retry_delete_file(source_file_name));

  const auto token = std::string("moose");
  auto source_file = create_random_file(
      source_file_name,
      8u * utils::encryption::encrypting_reader::get_data_chunk_size());
  EXPECT_TRUE(source_file != nullptr);
  if (source_file) {
    stop_type stop_requested = false;
    utils::encryption::encrypting_reader reader("test.dat", source_file_name,
                                                stop_requested, token);

    for (std::uint8_t i = 0u; i < 8u; i += 2u) {
      data_buffer buffer(
          utils::encryption::encrypting_reader::get_encrypted_chunk_size() *
          2u);
      EXPECT_EQ(buffer.size(),
                utils::encryption::encrypting_reader::reader_function(
                    &buffer[0u], buffer.size(), 1u, &reader));

      for (std::uint8_t j = 0u; j < 2u; j++) {
        data_buffer decrypted_data;
        const auto offset = (j * (buffer.size() / 2u));
        EXPECT_TRUE(utils::encryption::decrypt_data(
            token,
            data_buffer(buffer.begin() + offset,
                        buffer.begin() + offset + (buffer.size() / 2u)),
            decrypted_data));

        EXPECT_EQ(utils::encryption::encrypting_reader::get_data_chunk_size(),
                  decrypted_data.size());

        std::size_t bytes_read{};
        data_buffer file_data(decrypted_data.size());
        EXPECT_TRUE(source_file->read_bytes(
            &file_data[0u], file_data.size(),
            (utils::encryption::encrypting_reader::get_data_chunk_size() * i) +
                (j *
                 utils::encryption::encrypting_reader::get_data_chunk_size()),
            bytes_read));
        EXPECT_EQ(0, std::memcmp(&file_data[0u], &decrypted_data[0u],
                                 file_data.size()));
      }
    }

    source_file->close();
  }

  EXPECT_TRUE(utils::file::retry_delete_file(source_file_name));
}

TEST(encrypting_reader, file_data_as_stream) {
  const auto source_file_name = get_source_file_name();
  ASSERT_TRUE(utils::file::retry_delete_file(source_file_name));

  const auto token = std::string("moose");
  auto source_file = create_random_file(
      source_file_name,
      8u * utils::encryption::encrypting_reader::get_data_chunk_size());
  EXPECT_TRUE(source_file != nullptr);
  if (source_file) {
    stop_type stop_requested = false;
    utils::encryption::encrypting_reader reader("test.dat", source_file_name,
                                                stop_requested, token);
    auto io_stream = reader.create_iostream();
    EXPECT_FALSE(io_stream->seekg(0, std::ios_base::end).fail());
    EXPECT_TRUE(io_stream->good());
    EXPECT_EQ(reader.get_total_size(),
              static_cast<std::uint64_t>(io_stream->tellg()));
    EXPECT_FALSE(io_stream->seekg(0, std::ios_base::beg).fail());
    EXPECT_TRUE(io_stream->good());

    for (std::uint8_t i = 0u; i < 8u; i++) {
      data_buffer buffer(
          utils::encryption::encrypting_reader::get_encrypted_chunk_size());
      EXPECT_FALSE(io_stream->seekg(i * buffer.size()).fail());
      EXPECT_TRUE(io_stream->good());
      for (std::uint8_t j = 0u; j < 2u; j++) {
        EXPECT_FALSE(
            io_stream
                ->read(&buffer[(buffer.size() / 2u) * j], buffer.size() / 2u)
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
      EXPECT_TRUE(source_file->read_bytes(
          &file_data[0u], file_data.size(),
          utils::encryption::encrypting_reader::get_data_chunk_size() * i,
          bytes_read));
      EXPECT_EQ(0, std::memcmp(&file_data[0u], &decrypted_data[0u],
                               file_data.size()));
    }

    source_file->close();
  }
  EXPECT_TRUE(utils::file::retry_delete_file(source_file_name));
}

TEST(encrypting_reader, file_data_in_multiple_chunks_as_stream) {
  const auto source_file_name = get_source_file_name();
  ASSERT_TRUE(utils::file::retry_delete_file(source_file_name));

  const auto token = std::string("moose");
  auto source_file = create_random_file(
      source_file_name,
      8u * utils::encryption::encrypting_reader::get_data_chunk_size());
  EXPECT_TRUE(source_file != nullptr);
  if (source_file) {
    stop_type stop_requested = false;
    utils::encryption::encrypting_reader reader("test.dat", source_file_name,
                                                stop_requested, token);
    auto io_stream = reader.create_iostream();
    EXPECT_FALSE(io_stream->seekg(0, std::ios_base::end).fail());
    EXPECT_TRUE(io_stream->good());
    EXPECT_EQ(reader.get_total_size(),
              static_cast<std::uint64_t>(io_stream->tellg()));
    EXPECT_FALSE(io_stream->seekg(0, std::ios_base::beg).fail());
    EXPECT_TRUE(io_stream->good());

    for (std::uint8_t i = 0u; i < 8u; i += 2u) {
      data_buffer buffer(
          utils::encryption::encrypting_reader::get_encrypted_chunk_size() *
          2u);
      EXPECT_FALSE(io_stream->read(&buffer[0u], buffer.size()).fail());
      EXPECT_TRUE(io_stream->good());

      for (std::uint8_t j = 0u; j < 2u; j++) {
        data_buffer decrypted_data;
        const auto offset = (j * (buffer.size() / 2u));
        EXPECT_TRUE(utils::encryption::decrypt_data(
            token,
            data_buffer(buffer.begin() + offset,
                        buffer.begin() + offset + (buffer.size() / 2u)),
            decrypted_data));

        EXPECT_EQ(utils::encryption::encrypting_reader::get_data_chunk_size(),
                  decrypted_data.size());

        std::size_t bytes_read{};
        data_buffer file_data(decrypted_data.size());
        EXPECT_TRUE(source_file->read_bytes(
            &file_data[0u], file_data.size(),
            (utils::encryption::encrypting_reader::get_data_chunk_size() * i) +
                (j *
                 utils::encryption::encrypting_reader::get_data_chunk_size()),
            bytes_read));
        EXPECT_EQ(0, std::memcmp(&file_data[0u], &decrypted_data[0u],
                                 file_data.size()));
      }
    }

    source_file->close();
  }
  EXPECT_TRUE(utils::file::retry_delete_file(source_file_name));
}
} // namespace repertory
