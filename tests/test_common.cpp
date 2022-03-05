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
#include "test_common.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
native_file_ptr create_random_file(std::string path, const size_t &size) {
  native_file_ptr nf;
  if (native_file::create_or_open(path, nf) == api_error::success) {
    EXPECT_TRUE(nf->truncate(0u));

    std::vector<char> buf(size);
    CryptoPP::OS_GenerateRandomBlock(false, reinterpret_cast<CryptoPP::byte *>(&buf[0u]),
                                     buf.size());

    std::size_t bytes_written{};
    nf->write_bytes(&buf[0u], buf.size(), 0u, bytes_written);
    nf->flush();

    std::uint64_t current_size;
    EXPECT_TRUE(utils::file::get_file_size(path, current_size));
    EXPECT_EQ(size, current_size);
  }

  return nf;
}

std::string generate_test_file_name(const std::string &directory,
                                    const std::string &file_name_no_extension) {
  static std::atomic<std::uint32_t> idx(0u);
  return utils::path::absolute(utils::path::combine(
      directory, {file_name_no_extension + utils::string::from_uint32(idx++) + ".dat"}));
}
} // namespace repertory
