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
#include "test.hpp"

extern int PROJECT_TEST_RESULT;

namespace {
static std::recursive_mutex file_mtx{};

static std::vector<std::unique_ptr<repertory::utils::file::i_fs_item>>
    generated_files{};

struct file_deleter final {
  std::string test_output_dir;

  ~file_deleter() {
    generated_files.clear();

    if (PROJECT_TEST_RESULT == 0) {
      EXPECT_TRUE(repertory::utils::file::directory(test_output_dir)
                      .remove_recursively());
    }
  }
};

const auto deleter{
    std::make_unique<file_deleter>(repertory::test::get_test_output_dir()),
};
} // namespace

namespace repertory::test {
auto create_random_file(std::size_t size) -> utils::file::i_file & {
  auto path = generate_test_file_name("random");
  auto file = utils::file::file::open_or_create_file(path);
  EXPECT_TRUE(*file);
  if (*file) {
    data_buffer buf(size);
#if defined(PROJECT_ENABLE_LIBSODIUM)
    randombytes_buf(buf.data(), buf.size());
#else  // !defined(PROJECT_ENABLE_LIBSODIUM)
    thread_local std::mt19937 gen{
        static_cast<std::uint_fast32_t>(std::time(nullptr)) ^
            static_cast<std::uint_fast32_t>(std::random_device{}()),
    };
    std::uniform_int_distribution<std::uint8_t> dis(0U, 255U);
    std::generate(buf.begin(), buf.end(), [&]() -> auto { return dis(gen); });
#endif // defined(PROJECT_ENABLE_LIBSODIUM)

    std::size_t bytes_written{};
    EXPECT_TRUE(file->write(buf, 0U, &bytes_written));
    EXPECT_EQ(size, bytes_written);

    EXPECT_EQ(size, file->size());
  }

  recur_mutex_lock lock{file_mtx};
  generated_files.emplace_back(std::move(file));
  return *dynamic_cast<utils::file::i_file *>(generated_files.back().get());
}

auto generate_test_directory() -> utils::file::i_directory & {
  auto path = utils::path::combine(
      get_test_output_dir(),
      {
          std::string{"test_dir"} + std::to_string(generated_files.size()),
      });

  recur_mutex_lock lock{file_mtx};
  generated_files.emplace_back(std::unique_ptr<utils::file::i_fs_item>(
      new utils::file::directory{path}));

  auto &ret =
      *dynamic_cast<utils::file::i_directory *>(generated_files.back().get());
  EXPECT_TRUE(ret.create_directory());
  return ret;
}

auto generate_test_file_name(std::string_view file_name_no_extension)
    -> std::string {
  auto path = utils::path::combine(
      get_test_output_dir(), {
                                 std::string{file_name_no_extension} +
                                     std::to_string(generated_files.size()),
                             });

  recur_mutex_lock lock{file_mtx};
  generated_files.emplace_back(
      std::unique_ptr<utils::file::i_file>(new utils::file::file{path}));
  return generated_files.back()->get_path();
}

auto get_test_config_dir() -> std::string {
  static auto test_path = ([]() -> std::string {
    auto dir = utils::get_environment_variable("PROJECT_TEST_CONFIG_DIR");
    return utils::path::combine(dir.empty() ? "." : dir, {"test_config"});
  })();

  return test_path;
}

auto get_test_input_dir() -> std::string {
  static auto test_path = ([]() -> std::string {
    auto dir = utils::get_environment_variable("PROJECT_TEST_INPUT_DIR");
    return utils::path::combine(dir.empty() ? "." : dir, {"test_input"});
  })();

  return test_path;
}

auto get_test_output_dir() -> std::string {
  static auto test_path = ([]() -> std::string {
    auto temp = utils::file::create_temp_name("project_test");

#if defined(_WIN32)
    auto path = utils::path::combine("%TEMP%", {temp});
#else  // !defined(_WIN32)
    auto path = utils::path::combine("/tmp", {temp});
#endif // defined(_WIN32)

    if (not utils::file::directory(path).exists()) {
      EXPECT_TRUE(utils::file::directory{path}.create_directory());
    }

    return path;
  })();

  return test_path;
}
} // namespace repertory::test
