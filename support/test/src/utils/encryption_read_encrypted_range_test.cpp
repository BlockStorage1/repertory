/*
 Copyright <2018-2025> <scott.e.graves@protonmail.com>

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

namespace {
[[nodiscard]] auto make_random_plain(std::size_t size)
    -> std::vector<unsigned char> {
  std::vector<unsigned char> ret;
  ret.resize(size);

  constexpr std::size_t chunk_size = 4096U;
  using buf_t = std::array<unsigned char, chunk_size>;

  std::size_t written = 0U;
  while (written < size) {
    auto block = repertory::utils::generate_secure_random<buf_t>();
    auto to_copy = std::min<std::size_t>(chunk_size, size - written);
    std::memcpy(ret.data() + written, block.data(), to_copy);
    written += to_copy;
  }

  return ret;
}

[[nodiscard]] auto
build_encrypted_blob(const std::vector<unsigned char> &plain,
                     const repertory::utils::hash::hash_256_t &key,
                     bool with_kdf,
                     repertory::utils::encryption::kdf_config &kdf)
    -> std::pair<repertory::data_buffer, std::uint64_t> {
  repertory::data_buffer blob;

  if (with_kdf) {
    auto hdr = kdf.to_header();
    blob.insert(blob.end(), hdr.begin(), hdr.end());
  }

  auto data_chunk =
      repertory::utils::encryption::encrypting_reader::get_data_chunk_size();
  std::size_t offset = 0U;

  while (offset < plain.size()) {
    auto take = std::min<std::size_t>(data_chunk, plain.size() - offset);
    repertory::data_buffer buffer;
    repertory::utils::encryption::encrypt_data(key, plain.data() + offset, take,
                                               buffer);
    blob.insert(blob.end(), buffer.begin(), buffer.end());
    offset += take;
  }

  return {std::move(blob), static_cast<std::uint64_t>(plain.size())};
}

[[nodiscard]] auto make_reader(const repertory::data_buffer &cipher_blob)
    -> repertory::utils::encryption::reader_func_t {
  return [&cipher_blob](repertory::data_buffer &out, std::uint64_t start,
                        std::uint64_t end) -> bool {
    if (end < start) {
      return false;
    }

    if (end >= static_cast<std::uint64_t>(cipher_blob.size())) {
      return false;
    }

    auto len = static_cast<std::size_t>(end - start + 1U);
    out.assign(
        std::next(cipher_blob.begin(), static_cast<std::ptrdiff_t>(start)),
        std::next(cipher_blob.begin(),
                  static_cast<std::ptrdiff_t>(start + len)));
    return true;
  };
}
} // namespace

namespace repertory {
class utils_encryption_read_encrypted_range_fixture
    : public ::testing::Test,
      public ::testing::WithParamInterface<bool> {
protected:
  bool uses_kdf{};
  utils::hash::hash_256_t key{};
  utils::encryption::kdf_config kdf{};
  std::size_t chunk{};
  std::size_t plain_sz{};
  std::vector<unsigned char> plain;
  data_buffer cipher_blob;
  std::uint64_t total_size{};
  utils::encryption::reader_func_t reader;

  void SetUp() override {
    uses_kdf = GetParam();

    key =
        uses_kdf
            ? utils::encryption::generate_key<utils::hash::hash_256_t>("moose",
                                                                       kdf)
            : utils::encryption::generate_key<utils::hash::hash_256_t>("moose");
    chunk = utils::encryption::encrypting_reader::get_data_chunk_size();

    plain_sz = (2U * chunk) + (chunk / 2U);

    plain = make_random_plain(plain_sz);
    std::tie(cipher_blob, total_size) =
        build_encrypted_blob(plain, key, uses_kdf, kdf);
    reader = make_reader(cipher_blob);
  }
};

TEST_P(utils_encryption_read_encrypted_range_fixture,
       within_chunk_data_buffer) {
  std::uint64_t end_cap = chunk ? static_cast<std::uint64_t>(chunk) - 1U : 0U;
  std::uint64_t begin = 123U;
  std::uint64_t end = 4567U;
  if (end > end_cap) {
    end = end_cap;
  }

  if (end < begin) {
    begin = end;
  }

  ASSERT_GE(end, begin);
  ASSERT_LT(end, plain_sz);

  http_range range{begin, end};
  data_buffer out;

  ASSERT_TRUE(utils::encryption::read_encrypted_range(range, key, uses_kdf,
                                                      reader, total_size, out));

  std::vector<unsigned char> want(
      plain.begin() + static_cast<std::ptrdiff_t>(begin),
      plain.begin() + static_cast<std::ptrdiff_t>(end) + 1U);
  EXPECT_EQ(out, want);
}

TEST_P(utils_encryption_read_encrypted_range_fixture,
       cross_chunk_boundary_data_buffer) {
  std::uint64_t begin = static_cast<std::uint64_t>(chunk) - 512U;
  std::uint64_t end = begin + 1024U - 1U;
  ASSERT_GE(end, begin);
  ASSERT_LT(end, plain_sz);

  http_range range{begin, end};
  data_buffer out;

  ASSERT_TRUE(utils::encryption::read_encrypted_range(range, key, uses_kdf,
                                                      reader, total_size, out));

  std::vector<unsigned char> want(
      plain.begin() + static_cast<std::ptrdiff_t>(begin),
      plain.begin() + static_cast<std::ptrdiff_t>(end) + 1U);
  EXPECT_EQ(out, want);
}

TEST_P(utils_encryption_read_encrypted_range_fixture,
       multi_chunk_span_data_buffer) {
  std::uint64_t begin = static_cast<std::uint64_t>(chunk) - 10U;
  std::uint64_t end = static_cast<std::uint64_t>(2U * chunk) + 19U;
  ASSERT_GE(end, begin);
  ASSERT_LT(end, plain_sz);

  http_range range{begin, end};
  data_buffer out;

  ASSERT_TRUE(utils::encryption::read_encrypted_range(range, key, uses_kdf,
                                                      reader, total_size, out));

  std::vector<unsigned char> want(
      plain.begin() + static_cast<std::ptrdiff_t>(begin),
      plain.begin() + static_cast<std::ptrdiff_t>(end) + 1U);
  EXPECT_EQ(out, want);
}

TEST_P(utils_encryption_read_encrypted_range_fixture,
       tail_of_file_data_buffer) {
  std::uint64_t begin = static_cast<std::uint64_t>(plain_sz) - 200U;
  std::uint64_t end = static_cast<std::uint64_t>(plain_sz) - 1U;
  ASSERT_GE(end, begin);

  http_range range{begin, end};
  data_buffer out;

  ASSERT_TRUE(utils::encryption::read_encrypted_range(range, key, uses_kdf,
                                                      reader, total_size, out));

  std::vector<unsigned char> want(
      plain.begin() + static_cast<std::ptrdiff_t>(begin),
      plain.begin() + static_cast<std::ptrdiff_t>(end) + 1U);
  EXPECT_EQ(out, want);
}

TEST_P(utils_encryption_read_encrypted_range_fixture, whole_file_data_buffer) {
  std::uint64_t begin = 0U;
  std::uint64_t end = static_cast<std::uint64_t>(plain_sz - 1U);
  ASSERT_GE(end, begin);

  http_range range{begin, end};
  data_buffer out;

  ASSERT_TRUE(utils::encryption::read_encrypted_range(range, key, uses_kdf,
                                                      reader, total_size, out));
  EXPECT_EQ(out, plain);
}

TEST_P(utils_encryption_read_encrypted_range_fixture,
       pointer_sink_cross_chunk_with_array) {
  std::uint64_t begin = static_cast<std::uint64_t>(chunk) - 256U;
  constexpr std::size_t data_len = 2048U;
  std::uint64_t end = begin + data_len - 1U;
  ASSERT_GE(end, begin);
  ASSERT_LT(end, plain_sz);

  http_range range{begin, end};

  std::array<unsigned char, data_len> sink{};
  std::size_t bytes_read = 0U;

  ASSERT_TRUE(utils::encryption::read_encrypted_range(
      range, key, uses_kdf, reader, total_size, sink.data(), sink.size(),
      bytes_read));
  EXPECT_EQ(bytes_read, sink.size());

  std::vector<unsigned char> want(
      plain.begin() + static_cast<std::ptrdiff_t>(begin),
      plain.begin() + static_cast<std::ptrdiff_t>(end) + 1U);
  EXPECT_TRUE(std::equal(sink.begin(), sink.end(), want.begin(), want.end()));
}

TEST_P(utils_encryption_read_encrypted_range_fixture,
       reader_failure_for_both_overloads) {
  std::size_t call_count = 0U;
  auto flaky_reader = [this, &call_count](data_buffer &out, std::uint64_t start,
                                          std::uint64_t end) -> bool {
    if (++call_count == 1U) {
      return false;
    }
    auto len = static_cast<std::size_t>(end - start + 1U);
    out.assign(
        std::next(cipher_blob.begin(), static_cast<std::ptrdiff_t>(start)),
        std::next(cipher_blob.begin(),
                  static_cast<std::ptrdiff_t>(start + len)));
    return true;
  };

  std::uint64_t begin = 0U;
  constexpr std::size_t data_len = 1024U;
  std::uint64_t end = begin + data_len - 1U;
  http_range range{begin, end};

  {
    data_buffer out;
    EXPECT_FALSE(utils::encryption::read_encrypted_range(
        range, key, uses_kdf, flaky_reader, total_size, out));
    EXPECT_TRUE(out.empty());
  }

  call_count = 0U;
  {
    std::array<unsigned char, data_len> buf{};
    std::size_t bytes_read = 0U;
    EXPECT_FALSE(utils::encryption::read_encrypted_range(
        range, key, uses_kdf, flaky_reader, total_size, buf.data(), buf.size(),
        bytes_read));
    EXPECT_EQ(bytes_read, 0U);
  }
}

TEST_P(utils_encryption_read_encrypted_range_fixture,
       invalid_range_end_before_begin) {
  std::uint64_t begin = 100U;
  std::uint64_t end = 99U;
  http_range range{begin, end};

  {
    data_buffer out;
    EXPECT_TRUE(utils::encryption::read_encrypted_range(
        range, key, uses_kdf, reader, total_size, out));
    EXPECT_TRUE(out.empty());
  }

  {
    std::array<unsigned char, 16U> buf{};
    std::size_t bytes_read = 0U;
    EXPECT_TRUE(utils::encryption::read_encrypted_range(
        range, key, uses_kdf, reader, total_size, buf.data(), buf.size(),
        bytes_read));
    EXPECT_EQ(bytes_read, 0U);
  }
}

TEST_P(utils_encryption_read_encrypted_range_fixture, single_byte_read) {
  std::uint64_t pos = 777U;
  if (pos >= plain_sz) {
    pos = plain_sz ? static_cast<std::uint64_t>(plain_sz) - 1U : 0U;
  }

  http_range range{pos, pos};
  data_buffer out;

  ASSERT_TRUE(utils::encryption::read_encrypted_range(range, key, uses_kdf,
                                                      reader, total_size, out));
  ASSERT_EQ(out.size(), 1U);
  EXPECT_EQ(out[0], plain[pos]);

  std::array<unsigned char, 1U> buf{};
  std::size_t bytes_read = 0U;
  ASSERT_TRUE(utils::encryption::read_encrypted_range(
      range, key, uses_kdf, reader, total_size, buf.data(), buf.size(),
      bytes_read));
  EXPECT_EQ(bytes_read, 1U);
  EXPECT_EQ(buf[0], plain[pos]);
}

TEST_P(utils_encryption_read_encrypted_range_fixture,
       begin_at_exact_chunk_boundary) {

  auto begin = static_cast<std::uint64_t>(chunk);
  std::uint64_t end = begin + 1024U - 1U;
  if (end >= plain_sz)
    end = static_cast<std::uint64_t>(plain_sz) - 1U;
  ASSERT_GE(end, begin);

  http_range range{begin, end};
  data_buffer out;

  ASSERT_TRUE(utils::encryption::read_encrypted_range(range, key, uses_kdf,
                                                      reader, total_size, out));
  std::vector<unsigned char> want(
      plain.begin() + static_cast<std::ptrdiff_t>(begin),
      plain.begin() + static_cast<std::ptrdiff_t>(end) + 1U);
  EXPECT_EQ(out, want);
}

TEST_P(utils_encryption_read_encrypted_range_fixture, last_byte_only) {
  std::uint64_t pos = plain_sz ? static_cast<std::uint64_t>(plain_sz) - 1U : 0U;
  http_range range{pos, pos};
  data_buffer out;
  ASSERT_TRUE(utils::encryption::read_encrypted_range(range, key, uses_kdf,
                                                      reader, total_size, out));
  ASSERT_EQ(out.size(), 1U);
  EXPECT_EQ(out[0], plain[pos]);
}

TEST_P(utils_encryption_read_encrypted_range_fixture, tiny_file_whole_read) {
  plain = make_random_plain(37U);
  std::tie(cipher_blob, total_size) =
      build_encrypted_blob(plain, key, uses_kdf, kdf);
  reader = make_reader(cipher_blob);

  http_range range{0U, static_cast<std::uint64_t>(plain.size() - 1U)};
  data_buffer out;
  ASSERT_TRUE(utils::encryption::read_encrypted_range(range, key, uses_kdf,
                                                      reader, total_size, out));
  EXPECT_EQ(out, plain);
}

TEST_P(utils_encryption_read_encrypted_range_fixture,
       pointer_sink_exact_small_window) {
  std::uint64_t begin = 5U;
  std::uint64_t end = begin + 7U;
  ASSERT_GE(end, begin);
  ASSERT_LT(end, plain_sz);
  http_range range{begin, end};

  std::array<unsigned char, 8U> sink{};
  std::size_t bytes_read = 0U;
  ASSERT_TRUE(utils::encryption::read_encrypted_range(
      range, key, uses_kdf, reader, total_size, sink.data(), sink.size(),
      bytes_read));
  EXPECT_EQ(bytes_read, sink.size());
  EXPECT_TRUE(std::equal(sink.begin(), sink.end(),
                         plain.begin() + static_cast<std::ptrdiff_t>(begin)));
}

TEST_P(utils_encryption_read_encrypted_range_fixture,
       range_past_eof_truncates) {
  std::uint64_t begin = static_cast<std::uint64_t>(plain_sz) - 10U;
  std::uint64_t end = static_cast<std::uint64_t>(plain_sz);
  http_range range{begin, end};

  data_buffer out;
  ASSERT_TRUE(utils::encryption::read_encrypted_range(range, key, uses_kdf,
                                                      reader, total_size, out));

  std::size_t expected_len =
      static_cast<std::size_t>(static_cast<std::uint64_t>(plain_sz) - begin);
  std::vector<unsigned char> want(
      plain.begin() + static_cast<std::ptrdiff_t>(begin),
      plain.begin() + static_cast<std::ptrdiff_t>(plain_sz));
  ASSERT_EQ(out.size(), expected_len);
  EXPECT_EQ(out, want);

  std::array<unsigned char, 32U> buf{};
  std::size_t bytes_read = 0U;
  ASSERT_TRUE(utils::encryption::read_encrypted_range(
      range, key, uses_kdf, reader, total_size, buf.data(), buf.size(),
      bytes_read));
  EXPECT_EQ(bytes_read, std::min<std::size_t>(buf.size(), expected_len));
  EXPECT_TRUE(std::equal(buf.begin(), buf.begin() + bytes_read,
                         plain.begin() + static_cast<std::ptrdiff_t>(begin)));
}

TEST_P(utils_encryption_read_encrypted_range_fixture,
       pointer_sink_larger_buffer) {
  std::uint64_t begin = 42U;
  std::uint64_t end = begin + 63U;
  ASSERT_GE(end, begin);
  ASSERT_LT(end, plain_sz);
  http_range range{begin, end};

  std::array<unsigned char, 128U> buf{};
  std::size_t bytes_read = 0U;

  ASSERT_TRUE(utils::encryption::read_encrypted_range(
      range, key, uses_kdf, reader, total_size, buf.data(), buf.size(),
      bytes_read));
  EXPECT_EQ(bytes_read, 64U);
  EXPECT_TRUE(std::equal(buf.begin(), buf.begin() + 64U,
                         plain.begin() + static_cast<std::ptrdiff_t>(begin)));
}

INSTANTIATE_TEST_SUITE_P(no_kdf_and_kdf,
                         utils_encryption_read_encrypted_range_fixture,
                         ::testing::Values(false, true));
} // namespace repertory

#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
